#include "native_article_resource_scene.h"
#include "native_article_resource_internal.h"
#include "native_item_link_pool.h"
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/gm/native_rollback_pages.h>
#include <melee/gm/native_match_arena.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>

// Recipe publication/retirement only. Stock and authored ItemLink payloads
// share nativeItemLinkPool; this scene owns no second backing or allocator.
static uint64_t scene_epoch;
static uint64_t snapshot_identity;

static bool boundary(void)
{
    return !(nativeMatchSceneArena.lifetime && nativeMatchSceneArena.sealed) && !nativeRollbackPages_IsInitialized() && nativeScriptRuntime_Idle() &&
        !nativeHsdReservation_Active() && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_RenderCallbackDepth && !HSD_GObj_804D7818 &&
        !HSD_GObj_DelayedProcInfo.flags;
}

bool nativeArticleResourceScene_Ready(void)
{
    return scene_epoch && scene_epoch == nativeGObjPool.lifetime && nativeItemLinkPool_Ready();
}

bool nativeArticleResourceScene_Prepare(void)
{
    if (!boundary() || scene_epoch || snapshot_identity == UINT64_MAX || !nativeItemLinkPool_Ready()) return false;
    for (unsigned kind = 0; kind < Ft_Kind_RuntimeCapacity; ++kind) {
        const NativeSourcePackage* package = nativeFighterRuntime_SourcePackage((FighterKind)kind);
        if (!package) continue;
        if (package->address != package || !package->ready || !package->assets.ready ||
            package->program.items.count > NATIVE_SCRIPT_ENTITY_LIMIT) return false;
        for (unsigned i = 0; i < package->program.items.count; ++i) {
            const NativeScriptItemProgram* program = &package->program.items.programs[i];
            if (!nativeArticleResource_ValidateRecipe(program->resource_capacity,
                    program->resource_reserved_part, package->assets.parts.parts_num)) return false;
        }
    }
    scene_epoch = nativeGObjPool.lifetime;
    ++snapshot_identity;
    return true;
}

bool nativeArticleResourceScene_Retire(void)
{
    if (!boundary()) return false;
    if (!scene_epoch) return true;
    nativeArticleResource_Clear();
    for (unsigned i = 0; i < NATIVE_ARTICLE_RESOURCE_COLLECTIONS; ++i)
        if (nativeArticleResourceCollections[i].owner.object) return false;
    // Each collection's actual userdata callback retires its shared-pool node.
    // Live stock tethers may remain until the ordinary GObj scene teardown.
    scene_epoch = 0;
    return true;
}

uint64_t nativeArticleResourceScene_SnapshotIdentity(void)
{ return nativeArticleResourceScene_Ready() ? snapshot_identity : 0; }

bool nativeArticleResourceScene_BufferIndependent(const void* buffer,size_t bytes)
{
    uintptr_t p=(uintptr_t)buffer;
    return buffer && bytes<=UINTPTR_MAX-p &&
        !(p<(uintptr_t)&scene_epoch+sizeof(scene_epoch) && (uintptr_t)&scene_epoch<p+bytes) &&
        !(p<(uintptr_t)&snapshot_identity+sizeof(snapshot_identity) && (uintptr_t)&snapshot_identity<p+bytes);
}
