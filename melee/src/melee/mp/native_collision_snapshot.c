#include <melee/gm/native_match_arena.h>
#include "native_collision_snapshot.h"
#include "native_collision_state_internal.h"
#include "mpisland.h"
#include "forward.h"
#include <melee/ft/native_script_runtime.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <string.h>

static bool overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return an > UINTPTR_MAX - x || bn > UINTPTR_MAX - y ||
        (x < y + bn && y < x + an);
}

static bool boundary(void)
{
    return nativeScriptRuntime_Idle() && !nativeHsdReservation_Active() &&
        !HSD_GObj_CurrentInvokedProc && !HSD_GObj_CurrentInvokedProcGObj &&
        !HSD_GObj_RenderCallbackDepth && !HSD_GObj_804D7818 && !HSD_GObj_DelayedProcInfo.flags;
}

static bool binding(const MapCollData* map)
{
    const MapCollData* bound = &nativeCollisionScene.geometry;
    return map->verts == bound->verts && map->lines == bound->lines &&
        map->joints == bound->joints && map->vert_count == bound->vert_count &&
        map->line_count == bound->line_count && map->joint_count == bound->joint_count;
}

static bool ready(void)
{
    return boundary() && nativeCollisionScene.identity &&
        nativeCollisionScene.object_epoch == nativeGObjPool.lifetime &&
        nativeCollisionScene.source == mpLib_804D64B4 && mpLib_804D64B4 &&
        groundCollVtx == nativeCollisionVertices &&
        groundCollLine == nativeCollisionLines && groundCollJoint == nativeCollisionJoints &&
        binding(mpLib_804D64B4) && nativeCollisionState_ValidDescription(mpLib_804D64B4);
}

static bool independent(const void* buffer, size_t bytes)
{
    const MapCollData* map = &nativeCollisionScene.geometry;
    if (!buffer || !bytes || bytes > UINTPTR_MAX - (uintptr_t)buffer) return false;
#define SEPARATE(object) if (overlap(buffer, bytes, &(object), sizeof(object))) return false
    SEPARATE(nativeCollisionScene);
    SEPARATE(nativeCollisionVertices); SEPARATE(nativeCollisionLines); SEPARATE(nativeCollisionJoints);
    SEPARATE(groundCollVtx); SEPARATE(groundCollLine); SEPARATE(groundCollJoint);
    SEPARATE(mpLib_804D64B4); SEPARATE(jointListStart); SEPARATE(jointListEnd);
    SEPARATE(mpLib_80458868); SEPARATE(didCheckBounding); SEPARATE(mpLib_804D64CC);
    SEPARATE(mpColl_804D64AC); SEPARATE(nativeCollisionQuery);
    SEPARATE(mpIsland_80458E88); SEPARATE(nativeCollisionAttributes);
    SEPARATE(nativeCollisionAttributeActive); SEPARATE(nativeCollisionAttributeFree);
#undef SEPARATE
    return !overlap(buffer, bytes, nativeCollisionScene.source, sizeof(*map)) &&
        !overlap(buffer, bytes, map->verts, map->vert_count * sizeof(*map->verts)) &&
        !overlap(buffer, bytes, map->lines, map->line_count * sizeof(*map->lines)) &&
        !overlap(buffer, bytes, map->joints, map->joint_count * sizeof(*map->joints));
}

static size_t array_bytes(const MapCollData* map)
{
    return map->vert_count * (sizeof(CollVtx) + sizeof(Vec2)) +
        map->line_count * (sizeof(CollLine) + sizeof(MapLine)) +
        map->joint_count * (sizeof(CollJoint) + sizeof(MapJoint));
}

size_t nativeCollisionState_SnapshotBytes(void)
{
    return ready() ? sizeof(NativeCollisionSnapshotHeader) +
        array_bytes(&nativeCollisionScene.geometry) : 0;
}

static int joint_index(const CollJoint* joint, unsigned count)
{
    uintptr_t address = (uintptr_t)joint, base = (uintptr_t)nativeCollisionJoints;
    if (!joint) return -1;
    if (address < base || address - base >= count * sizeof(CollJoint) ||
        (address - base) % sizeof(CollJoint)) return -2;
    return (int)((address - base) / sizeof(CollJoint));
}

static int attribute_index(const grDynamicAttr_UnkStruct* attribute)
{
    uintptr_t address = (uintptr_t)attribute, base = (uintptr_t)nativeCollisionAttributes;
    if (!attribute) return -1;
    if (address < base || address - base >= sizeof(nativeCollisionAttributes) ||
        (address - base) % sizeof(*attribute)) return -2;
    return (int)((address - base) / sizeof(*attribute));
}

static bool attributes(const NativeCollisionSnapshotHeader* header)
{
    for (unsigned i = 0; i < NATIVE_COLLISION_ATTRIBUTES; ++i)
        if (attribute_index(header->attributes[i].next) == -2) return false;
    unsigned visited = 0;
    const grDynamicAttr_UnkStruct* roots[] = {header->attribute_active, header->attribute_free};
    for (unsigned list = 0; list < 2; ++list) {
        const grDynamicAttr_UnkStruct* current = roots[list];
        while (current) {
            int i = attribute_index(current);
            if (i < 0 || (visited & (1u << i))) return false;
            visited |= 1u << i;
            current = header->attributes[i].next;
        }
    }
    // grDynamicAttr_801CA1C0 can unlink a non-head expired record without
    // putting it on the free chain. All physical records are saved, including
    // those unreachable from either root; the lists need not cover the pool.
    return true;
}

static bool joint_list(const NativeCollisionSnapshotHeader* header, const uint8_t* joints)
{
    unsigned count = header->description.joint_count;
    uint8_t visited[NATIVE_COLLISION_JOINTS / 8] = {0};
    const CollJoint* current = header->list_start;
    const CollJoint* last = NULL;
    while (current) {
        int i = joint_index(current, count);
        if (i < 0 || (visited[i / 8] & (1u << (i % 8)))) return false;
        visited[i / 8] |= (uint8_t)(1u << (i % 8));
        CollJoint joint;
        memcpy(&joint, joints + i * sizeof(joint), sizeof(joint));
        last = current;
        current = joint.next;
    }
    if (last != header->list_end) return false;
    for (unsigned i = 0; i < count; ++i) {
        CollJoint joint;
        memcpy(&joint, joints + i * sizeof(joint), sizeof(joint));
        bool member = (visited[i / 8] & (1u << (i % 8))) != 0;
        if (member != ((joint.flags & CollJoint_Enabled) != 0) ||
            joint.inner != &header->description.joints[i] ||
            joint_index(joint.next, count) == -2) return false;
    }
    return true;
}

bool nativeCollisionState_ValidateSnapshot(const void* buffer, size_t bytes)
{
    if (!ready() || bytes != nativeCollisionState_SnapshotBytes() || !independent(buffer, bytes)) return false;
    NativeCollisionSnapshotHeader header;
    memcpy(&header, buffer, sizeof(header));
    const MapCollData* map = &header.description;
    if (header.identity != nativeCollisionScene.identity ||
        header.object_epoch != nativeCollisionScene.object_epoch ||
        header.source != nativeCollisionScene.source || !binding(map) ||
        !nativeCollisionState_ValidDescription(map) || !attributes(&header)) return false;
    const uint8_t* cursor = (const uint8_t*)buffer + sizeof(header);
    cursor += map->vert_count * sizeof(CollVtx);
    const uint8_t* lines = cursor;
    cursor += map->line_count * sizeof(CollLine);
    const uint8_t* joints = cursor;
    cursor += map->joint_count * sizeof(CollJoint) + map->vert_count * sizeof(Vec2);
    for (int i = 0; i < map->line_count; ++i) {
        CollLine runtime;
        MapLine source;
        memcpy(&runtime, lines + i * sizeof(runtime), sizeof(runtime));
        memcpy(&source, cursor, sizeof(source)); cursor += sizeof(source);
        if ((runtime.x0 && runtime.x0 != &map->lines[i]) ||
            !nativeCollisionState_ValidLine(&source, map->vert_count, map->line_count)) return false;
    }
    for (int i = 0; i < map->joint_count; ++i) {
        MapJoint source;
        memcpy(&source, cursor, sizeof(source)); cursor += sizeof(source);
        if (!nativeCollisionState_ValidJoint(&source, map->vert_count, map->line_count)) return false;
    }
    return joint_list(&header, joints);
}

bool nativeCollisionState_Save(void* buffer, size_t capacity, size_t* written)
{
    size_t bytes = nativeCollisionState_SnapshotBytes();
    if (!bytes || capacity < bytes || !independent(buffer, capacity) ||
        !independent(written, sizeof(*written)) || overlap(buffer, capacity, written, sizeof(*written))) return false;
    NativeCollisionSnapshotHeader header = {0};
    header.identity = nativeCollisionScene.identity;
    header.object_epoch = nativeCollisionScene.object_epoch;
    header.source = nativeCollisionScene.source;
    header.description = *mpLib_804D64B4;
    header.list_start = jointListStart; header.list_end = jointListEnd;
    memcpy(header.bounds, mpLib_80458868, sizeof(header.bounds));
    header.islands = mpIsland_80458E88;
    memcpy(header.attributes, nativeCollisionAttributes, sizeof(header.attributes));
    header.attribute_active = nativeCollisionAttributeActive;
    header.attribute_free = nativeCollisionAttributeFree;
    header.update_counter = mpLib_804D64CC; header.checked_bounds = didCheckBounding;
    header.collision_counter = mpColl_804D64AC; header.query = nativeCollisionQuery;
    uint8_t* cursor = (uint8_t*)buffer;
#define WRITE(object, count) do { size_t n = (count) * sizeof(*(object)); memcpy(cursor, object, n); cursor += n; } while (0)
    WRITE(&header, 1);
    WRITE(groundCollVtx, header.description.vert_count);
    WRITE(groundCollLine, header.description.line_count);
    WRITE(groundCollJoint, header.description.joint_count);
    WRITE(header.description.verts, header.description.vert_count);
    WRITE(header.description.lines, header.description.line_count);
    WRITE(header.description.joints, header.description.joint_count);
#undef WRITE
    if (!nativeCollisionState_ValidateSnapshot(buffer, bytes)) return false;
    *written = bytes;
    return true;
}

bool nativeCollisionState_Load(const void* buffer, size_t bytes)
{
    if (!nativeCollisionState_ValidateSnapshot(buffer, bytes)) return false;
    NativeCollisionSnapshotHeader header;
    memcpy(&header, buffer, sizeof(header));
    const uint8_t* cursor = (const uint8_t*)buffer + sizeof(header);
#define READ(object, count) do { size_t n = (count) * sizeof(*(object)); memcpy(object, cursor, n); cursor += n; } while (0)
    READ(groundCollVtx, header.description.vert_count);
    READ(groundCollLine, header.description.line_count);
    READ(groundCollJoint, header.description.joint_count);
    READ(header.description.verts, header.description.vert_count);
    READ(header.description.lines, header.description.line_count);
    READ(header.description.joints, header.description.joint_count);
#undef READ
    *mpLib_804D64B4 = header.description;
    jointListStart = header.list_start; jointListEnd = header.list_end;
    memcpy(mpLib_80458868, header.bounds, sizeof(header.bounds));
    mpIsland_80458E88 = header.islands;
    memcpy(nativeCollisionAttributes, header.attributes, sizeof(header.attributes));
    nativeCollisionAttributeActive = header.attribute_active;
    nativeCollisionAttributeFree = header.attribute_free;
    mpLib_804D64CC = header.update_counter; didCheckBounding = header.checked_bounds;
    mpColl_804D64AC = header.collision_counter; nativeCollisionQuery = header.query;
    return true;
}

// Scene construction only. Source geometry is mutable too: moving platforms
// update both the runtime collision cache and the archive-backed map arrays.
bool nativeCollisionState_AdmitHistory(NativeMatchArena* arena)
{
    if (!ready() || !arena || arena->lifetime != nativeCollisionScene.object_epoch)
        return false;
#define ROOT(value) do { if (!nativeMatchArena_AddRoot(arena,&(value),sizeof(value))) return false; } while (0)
    ROOT(groundCollVtx); ROOT(groundCollLine); ROOT(groundCollJoint);
    ROOT(mpLib_804D64B4); ROOT(jointListStart); ROOT(jointListEnd);
    ROOT(mpLib_80458868); ROOT(didCheckBounding); ROOT(mpLib_804D64CC);
    ROOT(mpColl_804D64AC); ROOT(nativeCollisionQuery); ROOT(mpIsland_80458E88);
    ROOT(nativeCollisionAttributes); ROOT(nativeCollisionAttributeActive);
    ROOT(nativeCollisionAttributeFree);
#undef ROOT
    const MapCollData* map = &nativeCollisionScene.geometry;
#define ARRAY(address,count) do { if (!nativeMatchArena_AddRoot(arena,address,(size_t)(count)*sizeof(*(address)))) return false; } while (0)
    ARRAY(mpLib_804D64B4,1);
    ARRAY(groundCollVtx,map->vert_count); ARRAY(groundCollLine,map->line_count);
    ARRAY(groundCollJoint,map->joint_count);
    ARRAY(map->verts,map->vert_count); ARRAY(map->lines,map->line_count);
    ARRAY(map->joints,map->joint_count);
#undef ARRAY
    // nativeCollisionScene is the immutable map/epoch lease, not tick state.
    return true;
}
