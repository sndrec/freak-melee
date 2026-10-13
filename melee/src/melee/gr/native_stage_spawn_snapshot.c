#include <melee/gm/native_match_control_history.h>
#include "native_stage_spawn_snapshot.h"
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdlib.h>
#include <string.h>

// Immutable source bindings are leased after stage constructors. They are not
// mutable simulation data. Later copies of those same definitions are allowed;
// introducing another definition while history is active requires a new scene.
typedef struct SpawnDefinition {
    grZakoGenerator_Config binding;
    grZakoGenerator_Spawn segments[STAGE_SPAWN_SEGMENTS];
} SpawnDefinition;
static struct {
    uint64_t epoch;
    uint64_t object_epoch;
    unsigned definitions;
    bool active;
    SpawnDefinition bindings[STAGE_SPAWN_CONFIGS];
} scene;

static bool same_definition(const grZakoGenerator_Config* a,
                            const grZakoGenerator_Config* b)
{
    return a->spawn_descs == b->spawn_descs && a->count == b->count &&
        a->callback == b->callback && !memcmp(&a->x10, &b->x10, sizeof(a->x10));
}
void nativeStageSpawn_BeginScene(void)
{
    if(!scene.active || scene.object_epoch!=nativeGObjPool.lifetime) {
        if(scene.epoch==UINT64_MAX)abort();uint64_t epoch=scene.epoch+1;
        memset(&scene,0,sizeof(scene));scene.epoch=epoch;scene.object_epoch=nativeGObjPool.lifetime;
    }
    scene.active = false;
    if (!scene.object_epoch || stage_spawn_state.count > STAGE_SPAWN_CONFIGS) return;
    for (unsigned i = 0; i < stage_spawn_state.count; ++i) {
        const grZakoGenerator_Config* config = &stage_spawn_state.records[i].config;
        if (!config->spawn_descs || !config->callback || config->count <= 0 ||
            config->count > STAGE_SPAWN_SEGMENTS || config->x10 < 0.0f ||
            config->gen || config->spawns != stage_spawn_state.records[i].segments)
            return;
        unsigned definition = 0;
        while (definition < scene.definitions &&
               !same_definition(config, &scene.bindings[definition].binding))
            ++definition;
        if (definition == scene.definitions) {
            SpawnDefinition* target = &scene.bindings[scene.definitions++];
            target->binding = *config;
            memcpy(target->segments, config->spawn_descs,
                   config->count * sizeof(*config->spawn_descs));
        }
    }
    scene.active = true;
}
void nativeStageSpawn_EndScene(void) { scene.active = false; }
static bool boundary(void)
{
    return scene.active && scene.object_epoch==nativeGObjPool.lifetime && nativeGmLoop_SnapshotBoundary() && nativeScriptRuntime_Idle();
}
static bool overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t first = (uintptr_t)a, second = (uintptr_t)b;
    return an > UINTPTR_MAX - first || bn > UINTPTR_MAX - second ||
        (first < second + bn && second < first + an);
}
static bool independent(const void* data, size_t bytes)
{
    if (!data || !bytes || bytes > UINTPTR_MAX - (uintptr_t)data ||
        overlap(data, bytes, &scene, sizeof(scene)) ||
        overlap(data, bytes, &stage_spawn_state, sizeof(stage_spawn_state))) return false;
    for (unsigned i = 0; i < scene.definitions; ++i) {
        const grZakoGenerator_Config* config = &scene.bindings[i].binding;
        if (overlap(data, bytes, config->spawn_descs,
                    config->count * sizeof(*config->spawn_descs))) return false;
    }
    return true;
}
static bool admitted(const grZakoGenerator_Config* config, unsigned slot)
{
    if (slot >= STAGE_SPAWN_CONFIGS || config->gen ||
        config->spawns != stage_spawn_state.records[slot].segments) return false;
    for (unsigned i = 0; i < scene.definitions; ++i) {
        const SpawnDefinition* definition = &scene.bindings[i];
        if (same_definition(config, &definition->binding))
            return !memcmp(config->spawn_descs, definition->segments,
                           config->count * sizeof(*config->spawn_descs));
    }
    return false;
}
size_t nativeStageSpawn_SnapshotBytes(void)
{
    if (!boundary() || stage_spawn_state.count > STAGE_SPAWN_CONFIGS) return 0;
    size_t total = sizeof(NativeStageSpawnSnapshotHeader);
    for (unsigned i = 0; i < stage_spawn_state.count; ++i) {
        const grZakoGenerator_Config* config = &stage_spawn_state.records[i].config;
        if (!admitted(config, i)) return 0;
        total += sizeof(*config) + config->count * sizeof(*config->spawns);
    }
    return total;
}
bool nativeStageSpawn_Validate(const void* data, size_t bytes)
{
    if (!boundary() || bytes < sizeof(NativeStageSpawnSnapshotHeader) ||
        !independent(data, bytes)) return false;
    NativeStageSpawnSnapshotHeader header;
    memcpy(&header, data, sizeof(header));
    if (header.epoch != scene.epoch || header.reserved || header.count > STAGE_SPAWN_CONFIGS)
        return false;
    size_t offset = sizeof(header);
    for (unsigned i = 0; i < header.count; ++i) {
        grZakoGenerator_Config config;
        if (bytes - offset < sizeof(config)) return false;
        memcpy(&config, (const uint8_t*)data + offset, sizeof(config));
        if (!admitted(&config, i)) return false;
        offset += sizeof(config);
        size_t segment_bytes = config.count * sizeof(*config.spawns);
        if (bytes - offset < segment_bytes) return false;
        offset += segment_bytes;
    }
    return offset == bytes;
}
bool nativeStageSpawn_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = nativeStageSpawn_SnapshotBytes();
    if (!bytes || capacity < bytes ||
        !independent(data, bytes) || !independent(written, sizeof(*written)) ||
        overlap(data, bytes, written, sizeof(*written))) return false;
    NativeStageSpawnSnapshotHeader header = { scene.epoch, stage_spawn_state.count, 0 };
    memcpy(data, &header, sizeof(header));
    size_t offset = sizeof(header);
    for (unsigned i = 0; i < header.count; ++i) {
        const StageSpawnRecord* record = &stage_spawn_state.records[i];
        memcpy((uint8_t*)data + offset, &record->config, sizeof(record->config));
        offset += sizeof(record->config);
        size_t segment_bytes = record->config.count * sizeof(*record->segments);
        memcpy((uint8_t*)data + offset, record->segments, segment_bytes);
        offset += segment_bytes;
    }
    *written = bytes;
    return true;
}
bool nativeStageSpawn_Load(const void* data, size_t bytes)
{
    if (!nativeStageSpawn_Validate(data, bytes)) return false;
    NativeStageSpawnSnapshotHeader header;
    memcpy(&header, data, sizeof(header));
    size_t offset = sizeof(header);
    for (unsigned i = 0; i < header.count; ++i) {
        StageSpawnRecord* record = &stage_spawn_state.records[i];
        memcpy(&record->config, (const uint8_t*)data + offset, sizeof(record->config));
        offset += sizeof(record->config);
        size_t segment_bytes = record->config.count * sizeof(*record->segments);
        memcpy(record->segments, (const uint8_t*)data + offset, segment_bytes);
        offset += segment_bytes;
    }
    stage_spawn_state.count = header.count;
    return true;
}

#ifdef MELEE_NATIVE
bool nativeStageSpawn_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(stage_spawn_state.count);
    if(!nativeMatchArena_AddCountedRoot(arena,stage_spawn_state.records,sizeof(*stage_spawn_state.records),
        STAGE_SPAWN_CONFIGS,&stage_spawn_state.count))return false;
#undef ROOT
    return true;
}
#endif
