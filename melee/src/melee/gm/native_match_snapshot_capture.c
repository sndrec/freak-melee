#include "native_match_snapshot.h"
#include "native_match_snapshot_pool.h"
#include "native_versus_control_snapshot.h"
#include "native_texture_identity_snapshot.h"
#include "native_gm_loop_snapshot.h"
#include "native_gm_input_snapshot.h"
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/gr/native_ground_pool.h>
#include <melee/gr/native_green_grid.h>
#include <melee/gr/native_stage_snapshot.h>
#include <melee/gr/native_stage_start_snapshot.h>
#include <melee/gr/native_stage_hazard_snapshot.h>
#include <melee/gr/native_stage_generator_snapshot.h>
#include <melee/gr/native_stage_spawn_snapshot.h>
#include <melee/it/native_item_pool.h>
#include <melee/it/native_item_link_pool.h>
#include <melee/it/native_article_resource_snapshot.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/ef/native_script_effect_snapshot.h>
#include <melee/ef/native_effect_control_snapshot.h>
#include <sysdolphin/baselib/native_particle_control_snapshot.h>
#include <sysdolphin/baselib/native_light_control_snapshot.h>
#include <sysdolphin/baselib/native_material_texture_snapshot.h>
#include <melee/mp/native_collision_state.h>
#include <sysdolphin/baselib/native_gobj_snapshot.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <sysdolphin/baselib/native_objalloc_snapshot.h>
#include <sysdolphin/baselib/native_class_snapshot.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
#include <sysdolphin/baselib/native_id_snapshot.h>
#include <sysdolphin/baselib/native_memory_snapshot.h>
#include <sysdolphin/baselib/native_robj_snapshot.h>
#include <sysdolphin/baselib/native_controller_snapshot.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
#include <melee/lb/native_input_clock_snapshot.h>
#include <melee/lb/native_dynamics_snapshot.h>
#include <melee/pl/native_player_snapshot.h>
#include <melee/cm/native_camera_subject_snapshot.h>
#include <melee/cm/native_camera_snapshot.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/memory.h>
#include <dolphin/os/OSHeapSnapshot.h>
#include <SDL3/SDL_timer.h>
#include <string.h>

static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x || bn>UINTPTR_MAX-y || (x<y+bn && y<x+an);
}
#define MEASURE(index,label,known,query,save,validate) do { \
    if(cursor>=capacity)goto finished; \
    void* scratch=(uint8_t*)backing+cursor;size_t remaining=capacity-cursor; \
    size_t written=0;uint64_t start=optional ? SDL_GetTicksNS() : 0; \
    size_t expected=optional ? (query) : 0; \
    uint64_t queried=optional ? SDL_GetTicksNS() : 0; \
    bool saved=(save);uint64_t saved_at=optional ? SDL_GetTicksNS() : 0; \
    bool valid=saved && written<=remaining && (validate); \
    uint64_t validated_at=optional ? SDL_GetTicksNS() : 0; \
    captured[index]=(NativeMatchSnapshotPart){scratch,written,label,valid}; \
    measured[index]=(NativeMatchSnapshotMeasurement){expected,queried-start,saved_at-queried,validated_at-saved_at,known,saved}; \
    if(!valid || (optional && known && written!=expected))complete=false; \
    if(written>remaining || written>SIZE_MAX-7)goto finished; \
    size_t aligned=(written+7)&~(size_t)7; \
    if(aligned>remaining)goto finished; \
    cursor+=aligned; \
} while(0)

bool nativeMatchSnapshot_Capture(NativeMatchSnapshotPart parts[NativeMatchSnapshot_Count],
    void* backing,size_t capacity,size_t* bytes,NativeMatchSnapshotMeasurement* optional)
{
    const size_t directory=sizeof(*parts)*NativeMatchSnapshot_Count;
    const size_t metrics=sizeof(*optional)*NativeMatchSnapshot_Count;
    if(!parts || !backing || !capacity || !bytes || (uintptr_t)backing%8 ||
       capacity>UINTPTR_MAX-(uintptr_t)backing ||
       overlap(parts,directory,backing,capacity) || overlap(bytes,sizeof(*bytes),backing,capacity) ||
       overlap(parts,directory,bytes,sizeof(*bytes)) ||
       (optional && (overlap(optional,metrics,backing,capacity) ||
                     overlap(optional,metrics,parts,directory) || overlap(optional,metrics,bytes,sizeof(*bytes)))))return false;
    NativeMatchSnapshotPart captured[NativeMatchSnapshot_Count]={0};
    NativeMatchSnapshotMeasurement measured[NativeMatchSnapshot_Count]={0};
    size_t cursor=0;bool complete=true;
    MEASURE(0, "gobj", true, nativeGObjPool_SnapshotBytes(),
        nativeGObjPool_Save(scratch,remaining, &written),
        true); // The GObj writer already validates the complete saved graph.
    MEASURE(1, "fighter", true, nativeMatchSnapshotPool_Bytes(&nativeFighterPool),
        nativeFighterPool_Save(scratch,remaining, &written),
        nativeFighterPool_ValidateSnapshot(scratch, written));
    MEASURE(2, "ground", true, nativeMatchSnapshotPool_Bytes(&nativeGroundPool),
        nativeGroundPool_Save(scratch,remaining, &written),
        nativeGroundPool_ValidateSnapshot(scratch, written));
    MEASURE(3, "item", true, nativeMatchSnapshotPool_Bytes(&nativeItemPool),
        nativeItemPool_Save(scratch,remaining, &written),
        nativeItemPool_ValidateSnapshot(scratch, written));
    MEASURE(4, "item-link", true, nativeMatchSnapshotPool_Bytes(&nativeItemLinkPool),
        nativeItemLinkPool_Save(scratch,remaining, &written),
        nativeItemLinkPool_ValidateSnapshot(scratch, written));
    MEASURE(5, "animation", true, nativeAnimationPool_SnapshotBytes(),
        nativeAnimationPool_Save(scratch,remaining, &written),
        true); // The writer checks pool metadata and the saved animation graph.
    MEASURE(6, "objalloc", true, nativeObjAlloc_SnapshotBytes(),
        nativeObjAlloc_Save(scratch,remaining, &written),
        true); // Descriptor registry/state and heap geometry are checked by Save.
    MEASURE(7, "class", true, nativeClass_SnapshotBytes(),
        nativeClass_Save(scratch,remaining, &written),
        true); // The writer checks counters, definitions and retained table ownership.
    MEASURE(8, "hsd-reservations", true, nativeHsdReservation_SnapshotBytes(),
        nativeHsdReservation_Save(scratch,remaining, &written),
        true); // Geometry and each pool are validated before writer publication.
    MEASURE(9, "id", true, sizeof(NativeIdSnapshot),
        nativeID_Save(scratch,remaining, &written),
        nativeID_ValidateSnapshot(scratch, written));
    MEASURE(10, "collision", true, nativeCollisionState_SnapshotBytes(),
        nativeCollisionState_Save(scratch,remaining, &written),
        nativeCollisionState_ValidateSnapshot(scratch, written));
    MEASURE(11, "article-resources", true, nativeArticleResource_SnapshotBytes(),
        nativeArticleResource_Save(scratch,remaining, &written),
        nativeArticleResource_ValidateSnapshot(scratch, written));
    MEASURE(12, "fighter-vm", false, 0,
        nativeScriptFighter_Save(scratch,remaining, &written),
        nativeScriptFighter_ValidateSnapshot(scratch, written));
    MEASURE(13, "item-vm", false, 0,
        nativeScriptItem_Save(scratch,remaining, &written),
        nativeScriptItem_ValidateSnapshot(scratch, written));
    MEASURE(14, "effect-vm", false, 0,
        nativeScriptEffect_Save(scratch,remaining, &written),
        nativeScriptEffect_ValidateSnapshot(scratch, written));
    // Selected HSD heap metadata only, not payload or the remaining OS heaps.
    OSHeapHandle heap = HSD_GetHeap();
    u32 heap_written = 0;
    MEASURE(15, "selected-os-heap", true, OSHeapSnapshotBytes(heap),
        (OSSaveHeapSnapshot(heap, scratch,remaining, &heap_written) ?
            (written = heap_written, true) : (written = heap_written, false)),
        true); // The direct heap writer guarantees valid own captures.
    MEASURE(16, "hsd-memory", true, sizeof(NativeMemorySnapshot),
        nativeMemory_Save(scratch,remaining, &written),
        nativeMemory_ValidateSnapshot(scratch, written));
    MEASURE(17, "robj-expression", true, nativeRObj_SnapshotBytes(),
        nativeRObj_Save(scratch,remaining, &written),
        nativeRObj_ValidateSnapshot(scratch, written));
    MEASURE(18, "controller", true, nativeController_SnapshotBytes(),
        nativeController_Save(scratch,remaining, &written),
        nativeController_ValidateSnapshot(scratch, written));
    MEASURE(19, "input-clock", true, sizeof(NativeInputClockSnapshot),
        nativeInputClock_Save(scratch,remaining, &written),
        nativeInputClock_ValidateSnapshot(scratch, written));
    MEASURE(20, "rng", true, sizeof(NativeRandomSnapshot),
        nativeRandom_Save(scratch,remaining, &written),
        nativeRandom_ValidateSnapshot(scratch, written));
    MEASURE(21, "gm-loop", true, sizeof(NativeGmLoopSnapshot),
        nativeGmLoop_Save(scratch,remaining, &written),
        nativeGmLoop_ValidateSnapshot(scratch, written));
    MEASURE(22, "gm-input", true, sizeof(NativeGmInputSnapshot),
        nativeGmInput_Save(scratch,remaining, &written),
        nativeGmInput_ValidateSnapshot(scratch, written));
    MEASURE(23, "player", true, sizeof(NativePlayerSnapshot),
            nativePlayerSnapshot_Save(scratch,remaining, &written),
            nativePlayerSnapshot_Validate(scratch, written));
    MEASURE(24, "camera-subjects", true, nativeCameraSubjects_SnapshotBytes(),
            nativeCameraSubjects_Save(scratch,remaining, &written),
            nativeCameraSubjects_Validate(scratch, written));
    MEASURE(25, "camera", true, sizeof(NativeCameraSnapshot),
            nativeCamera_Save(scratch,remaining, &written),
            nativeCamera_Validate(scratch, written));
    MEASURE(26, "stage", true, sizeof(NativeStageSnapshot),
            nativeStage_Save(scratch,remaining, &written),
            nativeStage_Validate(scratch, written));
    MEASURE(27, "stage-hazards", true, sizeof(NativeStageHazardSnapshot),
            nativeStageHazards_Save(scratch,remaining, &written),
            nativeStageHazards_Validate(scratch, written));
    MEASURE(28, "green-grid", true, nativeGreenGrid_SnapshotBytes(),
            nativeGreenGrid_Save(scratch,remaining, &written),
            nativeGreenGrid_Validate(scratch, written));
    MEASURE(29, "dynamics", true, sizeof(NativeDynamicsSnapshot),
            nativeDynamics_Save(scratch,remaining, &written),
            nativeDynamics_Validate(scratch, written));
    MEASURE(30, "stage-generator", true, sizeof(NativeStageGeneratorSnapshot),
            nativeStageGenerator_Save(scratch,remaining, &written),
            nativeStageGenerator_Validate(scratch, written));
    MEASURE(31, "stage-spawns", true, nativeStageSpawn_SnapshotBytes(),
            nativeStageSpawn_Save(scratch,remaining, &written),
            nativeStageSpawn_Validate(scratch, written));
    MEASURE(32, "effect-controls", true, sizeof(NativeEffectControlSnapshot),
            nativeEffectControl_Save(scratch,remaining, &written),
            nativeEffectControl_Validate(scratch, written));
    MEASURE(33, "particle-controls", true, sizeof(NativeParticleControlSnapshot),
            nativeParticleControl_Save(scratch,remaining, &written),
            nativeParticleControl_Validate(scratch, written));
    MEASURE(34,"versus-controls",true,sizeof(NativeVersusControlSnapshot),
        nativeVersusControl_Save(scratch,remaining,&written),
        nativeVersusControl_Validate(scratch,written));
    MEASURE(35,"texture-identities",true,sizeof(NativeTextureIdentitySnapshot),
        nativeTextureIdentity_Save(scratch,remaining,&written),
        nativeTextureIdentity_Validate(scratch,written));
    MEASURE(36,"light-controls",true,sizeof(NativeLightControlSnapshot),
        nativeLightControl_Save(scratch,remaining,&written),nativeLightControl_Validate(scratch,written));
    MEASURE(37,"stage-start",true,nativeStageStart_SnapshotBytes(),
        nativeStageStart_Save(scratch,remaining,&written),nativeStageStart_Validate(scratch,written));
    MEASURE(38,"material-textures",true,sizeof(NativeMaterialTextureSnapshot),
        nativeMaterialTexture_Save(scratch,remaining,&written),nativeMaterialTexture_Validate(scratch,written));
    memcpy(parts,captured,sizeof(captured));if(optional)memcpy(optional,measured,sizeof(measured));
    *bytes=cursor;return complete;
finished:
    memcpy(parts,captured,sizeof(captured));if(optional)memcpy(optional,measured,sizeof(measured));
    *bytes=cursor;return false;
}
#undef MEASURE
