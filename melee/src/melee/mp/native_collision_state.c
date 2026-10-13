#include "native_collision_state_internal.h"
#include "native_collision_query.h"
#include <melee/ft/native_script_runtime.h>
#include <melee/gm/native_rollback_pages.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <string.h>

// Fixed process storage remains in .mstate for the production page oracle.
// Explicit snapshots copy only the active scene prefixes.
_Alignas(16) CollVtx nativeCollisionVertices[NATIVE_COLLISION_VERTICES];
_Alignas(16) CollLine nativeCollisionLines[NATIVE_COLLISION_LINES];
_Alignas(16) CollJoint nativeCollisionJoints[NATIVE_COLLISION_JOINTS];
NativeCollisionScene nativeCollisionScene;

MapCollData* mpLib_804D64B4;
CollVtx* groundCollVtx;
CollLine* groundCollLine;
CollJoint* groundCollJoint;
CollJoint* jointListStart;
CollJoint* jointListEnd;
mpCollisionBox mpLib_80458868[2];
bool didCheckBounding;
s32 mpLib_804D64CC;
int mpColl_804D64AC;
NativeCollisionQueryState nativeCollisionQuery;

grDynamicAttr_UnkStruct nativeCollisionAttributes[NATIVE_COLLISION_ATTRIBUTES];
grDynamicAttr_UnkStruct* nativeCollisionAttributeActive;
grDynamicAttr_UnkStruct* nativeCollisionAttributeFree;

static bool range(s16 start, s16 count, unsigned total)
{
    return count >= 0 && (!count ||
        (start >= 0 && (unsigned)start <= total && (unsigned)count <= total - start));
}

bool nativeCollisionState_ValidDescription(const MapCollData* map)
{
    return map && map->verts && map->lines && map->joints &&
        map->vert_count > 0 && map->vert_count <= NATIVE_COLLISION_VERTICES &&
        map->line_count > 0 && map->line_count <= NATIVE_COLLISION_LINES &&
        map->joint_count > 0 && map->joint_count <= NATIVE_COLLISION_JOINTS &&
        range(map->floor_start, map->floor_count, map->line_count) &&
        range(map->ceiling_start, map->ceiling_count, map->line_count) &&
        range(map->right_wall_start, map->right_wall_count, map->line_count) &&
        range(map->left_wall_start, map->left_wall_count, map->line_count) &&
        range(map->dynamic_start, map->dynamic_count, map->line_count);
}

bool nativeCollisionState_ValidJoint(const MapJoint* joint, unsigned vertices, unsigned lines)
{
    return range(joint->floor_start, joint->floor_count, lines) &&
        range(joint->ceiling_start, joint->ceiling_count, lines) &&
        range(joint->right_wall_start, joint->right_wall_count, lines) &&
        range(joint->left_wall_start, joint->left_wall_count, lines) &&
        range(joint->dynamic_start, joint->dynamic_count, lines) &&
        range(joint->vtx_start, joint->vtx_count, vertices);
}

static bool link(s16 index, unsigned lines)
{
    return index == -1 || (index >= 0 && (unsigned)index < lines);
}

bool nativeCollisionState_ValidLine(const MapLine* line, unsigned vertices, unsigned lines)
{
    return line->v0_idx < vertices && line->v1_idx < vertices &&
        link(line->prev_id0, lines) && link(line->next_id0, lines) &&
        link(line->prev_id1, lines) && link(line->next_id1, lines);
}

bool nativeCollisionState_BeginScene(MapCollData* map)
{
    if (nativeRollbackPages_IsInitialized() || !nativeScriptRuntime_Idle() ||
        nativeHsdReservation_Active() || HSD_GObj_CurrentInvokedProc ||
        HSD_GObj_CurrentInvokedProcGObj || HSD_GObj_RenderCallbackDepth || HSD_GObj_804D7818 ||
        HSD_GObj_DelayedProcInfo.flags || !nativeGObjPool.lifetime ||
        nativeCollisionScene.identity == UINT64_MAX ||
        !nativeCollisionState_ValidDescription(map)) return false;
    for (int i = 0; i < map->line_count; ++i)
        if (!nativeCollisionState_ValidLine(&map->lines[i], map->vert_count, map->line_count)) return false;
    for (int i = 0; i < map->joint_count; ++i)
        if (!nativeCollisionState_ValidJoint(&map->joints[i], map->vert_count, map->line_count)) return false;
    nativeCollisionScene.source = map;
    nativeCollisionScene.geometry = *map;
    nativeCollisionScene.object_epoch = nativeGObjPool.lifetime;
    ++nativeCollisionScene.identity;
    groundCollVtx = nativeCollisionVertices;
    groundCollLine = nativeCollisionLines;
    groundCollJoint = nativeCollisionJoints;
    memset(groundCollVtx, 0, map->vert_count * sizeof(*groundCollVtx));
    memset(groundCollLine, 0, map->line_count * sizeof(*groundCollLine));
    memset(groundCollJoint, 0, map->joint_count * sizeof(*groundCollJoint));
    return true;
}
