#include <melee/gm/native_match_control_history.h>
#include "native_player_snapshot.h"
#include "player_internal.h"
#include <melee/gm/gm_16A2.h>
#include <melee/gm/native_gm_loop_snapshot.h>
#include <melee/ft/native_fighter_pool.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdlib.h>
#include <string.h>

// MELEE_ROLLBACK_HOST_STATE: frozen resident definitions and owner epochs.
// player_slots and the counters remain in their actual production modules.
static struct {
    uint64_t epoch;
    NativeMatchPool objects, fighters;
    ftMapping mappings[ChKind_RuntimeCapacity];
    pl_804D6470_t* common;
    pl_804D6470_t common_value;
    bool active;
} scene;

void nativePlayerSnapshot_BeginScene(void)
{
    if (scene.epoch == UINT64_MAX) abort();
    ++scene.epoch;
    scene.active = false;
    if (!nativeMatchPool_Validate(&nativeGObjPool) ||
        !nativeMatchPool_Validate(&nativeFighterPool)) return;
    scene.objects = nativeGObjPool;
    scene.fighters = nativeFighterPool;
    memcpy(scene.mappings, ftMapping_list, sizeof(scene.mappings));
    scene.common = pl_804D6470;
    if (scene.common) memcpy(&scene.common_value, scene.common, sizeof(scene.common_value));
    scene.active = true;
}

void nativePlayerSnapshot_EndScene(void) { scene.active = false; }

static bool same_storage(const NativeMatchPool* live, const NativeMatchPool* fixed)
{
    return live->payload == fixed->payload && live->slots == fixed->slots &&
        live->free_order == fixed->free_order && live->lifetime == fixed->lifetime &&
        live->capacity == fixed->capacity && live->stride == fixed->stride &&
        live->object_size == fixed->object_size;
}

static bool boundary(void)
{
    return scene.active && nativeGmLoop_SnapshotBoundary() &&
        same_storage(&nativeGObjPool, &scene.objects) &&
        same_storage(&nativeFighterPool, &scene.fighters) &&
        !memcmp(ftMapping_list, scene.mappings, sizeof(scene.mappings)) &&
        pl_804D6470 == scene.common &&
        (!scene.common || !memcmp(scene.common, &scene.common_value, sizeof(scene.common_value)));
}

static bool overlap(const void* first, size_t an, const void* second, size_t bn)
{
    uintptr_t a = (uintptr_t)first, b = (uintptr_t)second;
    return an > UINTPTR_MAX-a || bn > UINTPTR_MAX-b || (a < b+bn && b < a+an);
}

static bool independent(const void* data, size_t bytes)
{
    if (!data || !bytes || bytes > UINTPTR_MAX-(uintptr_t)data) return false;
#define EXCLUDE(root) if (overlap(data, bytes, &(root), sizeof(root))) return false
    EXCLUDE(scene); EXCLUDE(player_slots); EXCLUDE(ftMapping_list);
    EXCLUDE(pl_804D6470); EXCLUDE(Player_AllocData);
    EXCLUDE(staleAttackInstance); EXCLUDE(unk_804D6480);
    EXCLUDE(nativeGObjPool); EXCLUDE(nativeFighterPool);
#undef EXCLUDE
    if (scene.common && overlap(data, bytes, scene.common, sizeof(*scene.common))) return false;
    const NativeMatchPool* pools[] = { &scene.objects, &scene.fighters };
    for (unsigned i = 0; i < 2; ++i) {
        const NativeMatchPool* pool = pools[i];
        if (overlap(data, bytes, pool->payload, pool->stride*pool->capacity) ||
            overlap(data, bytes, pool->slots, sizeof(*pool->slots)*pool->capacity) ||
            overlap(data, bytes, pool->free_order, sizeof(*pool->free_order)*pool->capacity)) return false;
    }
    return true;
}

static bool empty_reference(NativeMatchPoolReference ref)
{
    return !ref.pool && !ref.lifetime && !ref.slot && !ref.generation;
}

// Structural checks accept older generations without reading their current
// payload. Load later requires the companion pools to have resurrected them.
static bool reference_shape(NativeMatchPoolReference ref, const NativeMatchPool* pool)
{
    return ref.pool == pool && ref.lifetime == pool->lifetime &&
        ref.slot < pool->capacity && ref.generation != 0;
}

static const void* reference_address(NativeMatchPoolReference ref)
{
    return ref.pool->payload + ref.slot*ref.pool->stride;
}

bool nativePlayerSnapshot_Validate(const void* data, size_t bytes)
{
    if (!boundary() || bytes != sizeof(NativePlayerSnapshot) || !independent(data, bytes)) return false;
    NativePlayerSnapshotHeader header;
    // Read only the fixed header here; inspect one player at a time below.
    memcpy(&header, data, sizeof(header));
    if (header.epoch != scene.epoch || header.object_epoch != scene.objects.lifetime ||
        header.fighter_epoch != scene.fighters.lifetime || header.common_data != scene.common ||
        !header.stale_instance || !header.attack_instance || header.reserved) return false;
    HSD_GObj* seen[Gm_Player_NumMax*2] = {0};
    const void* fighter_seen[Gm_Player_NumMax*2] = {0};
    unsigned count = 0;
    for (unsigned slot = 0; slot < Gm_Player_NumMax; ++slot) {
        StaticPlayer player;
        NativePlayerRoots roots;
        memcpy(&player, (const u8*)data + offsetof(NativePlayerSnapshot, players) + slot*sizeof(player), sizeof(player));
        memcpy(&roots, (const u8*)data + offsetof(NativePlayerSnapshot, roots) + slot*sizeof(roots), sizeof(roots));
        if ((unsigned)player.player_character >= ChKind_RuntimeCapacity ||
            (unsigned)player.slot_type > Gm_PKind_NA || (unsigned)player.player_state > 2 ||
            player.transformed[0] > 1 || player.transformed[1] != 1-player.transformed[0] ||
            (unsigned)player.stale_moves.current_index >= 10 ||
            (player.struct_func && player.struct_func != fn_8016A488)) return false;
        for (unsigned member = 0; member < 2; ++member) {
            HSD_GObj* object = player.player_entity[member];
            if (!object) {
                if (!empty_reference(roots.object[member]) || !empty_reference(roots.fighter[member])) return false;
                continue;
            }
            if (!reference_shape(roots.object[member], &nativeGObjPool) ||
                reference_address(roots.object[member]) != object ||
                !reference_shape(roots.fighter[member], &nativeFighterPool)) return false;
            const void* fighter = reference_address(roots.fighter[member]);
            for (unsigned i = 0; i < count; ++i)
                if (seen[i] == object || fighter_seen[i] == fighter) return false;
            seen[count] = object; fighter_seen[count++] = fighter;
        }
    }
    return true;
}

bool nativePlayerSnapshot_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = sizeof(NativePlayerSnapshot);
    if (!boundary() || capacity < bytes || !independent(data, bytes) ||
        !independent(written, sizeof(*written)) || overlap(data, bytes, written, sizeof(*written))) return false;
    NativePlayerSnapshot state = {0};
    state.header.epoch = scene.epoch;
    state.header.object_epoch = scene.objects.lifetime;
    state.header.fighter_epoch = scene.fighters.lifetime;
    state.header.common_data = scene.common;
    state.header.stale_instance = staleAttackInstance;
    state.header.attack_instance = unk_804D6480;
    memcpy(state.players, player_slots, sizeof(state.players));
    for (unsigned slot = 0; slot < Gm_Player_NumMax; ++slot) {
        if ((unsigned)state.players[slot].player_character >= ChKind_RuntimeCapacity) return false;
        const ftMapping* mapping = &scene.mappings[state.players[slot].player_character];
        for (unsigned member = 0; member < 2; ++member) {
            HSD_GObj* object = state.players[slot].player_entity[member];
            if (!object) continue;
            if (!nativeMatchPool_Reference(&nativeGObjPool, object, &state.roots[slot].object[member]) ||
                object->classifier != HSD_GOBJ_CLASS_FIGHTER ||
                !nativeMatchPool_Reference(&nativeFighterPool, object->user_data, &state.roots[slot].fighter[member])) return false;
            Fighter* actor = object->user_data;
            if (actor->gobj != object || actor->player_id != slot ||
                (actor->kind != mapping->internal_id && actor->kind != mapping->extra_internal_id)) return false;
        }
    }
    if (!nativePlayerSnapshot_Validate(&state, bytes)) return false;
    memcpy(data, &state, bytes);
    *written = bytes;
    return true;
}

bool nativePlayerSnapshot_Load(const void* data, size_t bytes)
{
    if (!nativePlayerSnapshot_Validate(data, bytes)) return false;
    NativePlayerSnapshot state;
    memcpy(&state, data, sizeof(state));
    // Resolve every root and its actual Fighter before modifying ANY player.
    // Physical residency alone cannot validate freed/reused actor generations.
    for (unsigned slot = 0; slot < Gm_Player_NumMax; ++slot) {
        const ftMapping* mapping = &scene.mappings[state.players[slot].player_character];
        for (unsigned member = 0; member < 2; ++member) {
            HSD_GObj* object = state.players[slot].player_entity[member];
            if (!object) continue;
            Fighter* actor = nativeMatchPool_Resolve(state.roots[slot].fighter[member]);
            if (nativeMatchPool_Resolve(state.roots[slot].object[member]) != object ||
                !actor || object->classifier != HSD_GOBJ_CLASS_FIGHTER || object->user_data != actor ||
                actor->gobj != object || actor->player_id != slot ||
                (actor->kind != mapping->internal_id && actor->kind != mapping->extra_internal_id)) return false;
        }
    }
    memcpy(player_slots, state.players, sizeof(state.players));
    staleAttackInstance = state.header.stale_instance;
    unk_804D6480 = state.header.attack_instance;
    return true;
}

#ifdef MELEE_NATIVE
bool nativePlayer_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v)))return false; } while(0)
    ROOT(player_slots);
    ROOT(staleAttackInstance);
    ROOT(unk_804D6480);
#undef ROOT
    return true;
}
#endif
