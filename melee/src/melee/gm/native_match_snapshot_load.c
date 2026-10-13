#include "native_match_snapshot.h"
#include "native_snapshot_actor_graph.h"
#include "native_snapshot_class_graph.h"
#include "native_match_snapshot_pool.h"
#include "native_versus_control_snapshot.h"
#include "native_texture_identity_snapshot.h"
#include "forward.h"
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
#include <melee/it/native_article_resource_scene.h>
#include <melee/it/native_article_resource_model.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/ef/native_script_effect_snapshot.h>
#include <melee/ef/native_effect_control_snapshot.h>
#include <sysdolphin/baselib/native_particle_control_snapshot.h>
#include <sysdolphin/baselib/native_light_control_snapshot.h>
#include <sysdolphin/baselib/native_material_texture_snapshot.h>
#include <melee/mp/native_collision_state.h>
#include <melee/mp/native_collision_state_internal.h>
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

// Shared direct owner loader; no mutable storage or diagnostic I/O.
// Own immutable captures only. Normal use targets the current boundary; bounded
// older-state experiments always recover the complete preserved endpoint before
// continuing. A refusal is diagnostic failure, not atomic candidate rejection.
// No callback, simulation or render runs while applying or comparing captures.
bool nativeMatchSnapshot_Load(const NativeMatchSnapshotPart* parts,NativeMatchSnapshotLoadReport* optional)
{
    uint64_t start=optional ? SDL_GetTicksNS() : 0;
    NativeMatchSnapshotLoadReport result={.failed_owner=UINT32_MAX};
    if(optional)*optional=result;
    if(!parts)return false;
    for(unsigned i=0;i<NativeMatchSnapshot_Count;++i)
        if(!parts[i].validated || !parts[i].data || !parts[i].bytes)return false;
    if(parts[0].bytes<sizeof(NativeGObjSnapshotHeader))return false;
    NativeGObjSnapshotHeader header;memcpy(&header,parts[0].data,sizeof(header));
    if(header.object_bytes>parts[0].bytes || header.proc_bytes>parts[0].bytes-header.object_bytes)return false;
    // The own current-boundary capture was fully validated above. Its two pool
    // components are the trailing spans, after private scheduler/link tables.
    size_t prefix=parts[0].bytes-header.object_bytes-header.proc_bytes;
    if(prefix<sizeof(header))return false;
    NativeSnapshotActorGraph graph={
        .objects=nativeMatchSnapshotPool_View(&nativeGObjPool,(const uint8_t*)parts[0].data+prefix,header.object_bytes),
        .fighters=nativeMatchSnapshotPool_View(&nativeFighterPool,parts[1].data,parts[1].bytes),
        .ground=nativeMatchSnapshotPool_View(&nativeGroundPool,parts[2].data,parts[2].bytes),
        .items=nativeMatchSnapshotPool_View(&nativeItemPool,parts[3].data,parts[3].bytes)};
    result.actors_valid=nativeSnapshotActorGraph_Validate(&graph);
    result.fighters=graph.fighters.header.used;result.ground=graph.ground.header.used;result.items=graph.items.header.used;
    if(optional)*optional=result;
    if(!result.actors_valid)return false;
    if(!nativeStageStart_ValidateObjects(parts[37].data,parts[37].bytes,graph.objects) ||
       parts[26].bytes!=sizeof(NativeStageSnapshot))return false;
    NativeStageStartSnapshotHeader pending;
    NativeStageSnapshot stage;
    memcpy(&pending,parts[37].data,sizeof(pending));memcpy(&stage,parts[26].data,sizeof(stage));
    if(stage.started!=pending.started || stage.stage.x6A4!=
        (pending.count ? &stage_start_state.nodes[pending.count-1] : NULL))return false;
    NativeSnapshotClassGraphReport classes={0};
    result.classes_valid=nativeSnapshotClassGraph_Validate(parts,&classes);
    result.class_blocks=classes.blocks;result.class_types=classes.types;
    if(optional)*optional=result;
    if(!result.classes_valid)return false;
    if(optional)result.preflight_ns=SDL_GetTicksNS()-start;
#define LOAD(index,call) do { \
    uint64_t before=optional ? SDL_GetTicksNS() : 0; \
    bool restored=(call); \
    if(optional)result.owner_ns[index]=SDL_GetTicksNS()-before; \
    if(!restored) { \
        result.failed_owner=index;if(optional)*optional=result;return false; \
    } \
} while(0)
    LOAD(16,nativeMemory_Load(parts[16].data,parts[16].bytes));
    LOAD(15,OSLoadHeapSnapshot(HSD_GetHeap(),parts[15].data,(u32)parts[15].bytes));
    LOAD(6,nativeObjAlloc_Load(parts[6].data,parts[6].bytes));
    LOAD(7,nativeClass_Load(parts[7].data,parts[7].bytes));
    LOAD(8,nativeHsdReservation_Load(parts[8].data,parts[8].bytes));
    LOAD(0,nativeGObjPool_Load(parts[0].data,parts[0].bytes));
    LOAD(1,nativeFighterPool_Load(parts[1].data,parts[1].bytes));
    LOAD(2,nativeGroundPool_Load(parts[2].data,parts[2].bytes));
    LOAD(3,nativeItemPool_Load(parts[3].data,parts[3].bytes));
    LOAD(4,nativeItemLinkPool_Load(parts[4].data,parts[4].bytes));
    LOAD(5,nativeAnimationPool_Load(parts[5].data,parts[5].bytes));
    LOAD(9,nativeID_Load(parts[9].data,parts[9].bytes));
    LOAD(10,nativeCollisionState_Load(parts[10].data,parts[10].bytes));
    LOAD(11,nativeArticleResource_Load(parts[11].data,parts[11].bytes));
    LOAD(17,nativeRObj_Load(parts[17].data,parts[17].bytes));
    LOAD(18,nativeController_Load(parts[18].data,parts[18].bytes));
    LOAD(19,nativeInputClock_Load(parts[19].data,parts[19].bytes));
    LOAD(20,nativeRandom_Load(parts[20].data,parts[20].bytes));
    LOAD(34,nativeVersusControl_Load(parts[34].data,parts[34].bytes));
    LOAD(35,nativeTextureIdentity_Load(parts[35].data,parts[35].bytes));
    LOAD(21,nativeGmLoop_Load(parts[21].data,parts[21].bytes));
    LOAD(22,nativeGmInput_Load(parts[22].data,parts[22].bytes));
    LOAD(23,nativePlayerSnapshot_Load(parts[23].data,parts[23].bytes));
    LOAD(24,nativeCameraSubjects_Load(parts[24].data,parts[24].bytes));
    LOAD(25,nativeCamera_Load(parts[25].data,parts[25].bytes));
    LOAD(26,nativeStage_Load(parts[26].data,parts[26].bytes));
    LOAD(27,nativeStageHazards_Load(parts[27].data,parts[27].bytes));
    LOAD(28,nativeGreenGrid_Load(parts[28].data,parts[28].bytes));
    LOAD(29,nativeDynamics_Load(parts[29].data,parts[29].bytes));
    LOAD(30,nativeStageGenerator_Load(parts[30].data,parts[30].bytes));
    LOAD(31,nativeStageSpawn_Load(parts[31].data,parts[31].bytes));
    LOAD(32,nativeEffectControl_Load(parts[32].data,parts[32].bytes));
    LOAD(33,nativeParticleControl_Load(parts[33].data,parts[33].bytes));
    LOAD(36,nativeLightControl_Load(parts[36].data,parts[36].bytes));
    LOAD(37,nativeStageStart_Load(parts[37].data,parts[37].bytes));
    LOAD(38,nativeMaterialTexture_Load(parts[38].data,parts[38].bytes));
    LOAD(12,nativeScriptFighter_Load(parts[12].data,parts[12].bytes));
    LOAD(13,nativeScriptItem_Load(parts[13].data,parts[13].bytes));
    LOAD(14,nativeScriptEffect_Load(parts[14].data,parts[14].bytes));
#undef LOAD
    if(optional) {result.total_ns=SDL_GetTicksNS()-start;*optional=result;}
    return true;
}
