#include "native_match_control_history.h"
#include "native_match_os_heap.h"
#include <stdio.h>
#include <melee/ft/fighter_collision_state.h>
#include <melee/gr/native_green_grid.h>
#include <melee/it/native_article_resource_snapshot.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/mp/native_collision_snapshot.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>
// Registration is construction work; normal save/load only traverse the sealed
// spans. Each owner retains responsibility for its real storage and lifetime.
bool nativeMatchControls_AdmitHistory(NativeMatchArena* arena)
{
    bool valid=true;
    if(!nativeRandom_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeRandom refused\n");valid=false;}
    if(!nativeMemory_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeMemory refused\n");valid=false;}
    if(!nativeID_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeID refused\n");valid=false;}
    if(!nativeGmInput_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeGmInput refused\n");valid=false;}
    if(!nativeGmLoop_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeGmLoop refused\n");valid=false;}
    if(!nativeInputClock_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeInputClock refused\n");valid=false;}
    if(!nativePlayer_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativePlayer refused\n");valid=false;}
    if(!nativeCameraSubjects_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeCameraSubjects refused\n");valid=false;}
    if(!nativeCamera_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeCamera refused\n");valid=false;}
    if(!nativeStage_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeStage refused\n");valid=false;}
    if(!nativeStageHazards_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeStageHazards refused\n");valid=false;}
    if(!nativeStageGenerator_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeStageGenerator refused\n");valid=false;}
    if(!nativeStageSpawn_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeStageSpawn refused\n");valid=false;}
    if(!nativeStageStart_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeStageStart refused\n");valid=false;}
    if(!nativeDynamics_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeDynamics refused\n");valid=false;}
    if(!nativeEffectControl_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeEffectControl refused\n");valid=false;}
    if(!nativeParticleControl_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeParticleControl refused\n");valid=false;}
    if(!nativeLightControl_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeLightControl refused\n");valid=false;}
    if(!nativeMaterialTexture_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeMaterialTexture refused\n");valid=false;}
    if(!nativeVersusControl_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeVersusControl refused\n");valid=false;}
    if(!nativeController_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeController refused\n");valid=false;}
    if(!nativeRObj_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeRObj refused\n");valid=false;}
    if(!nativeTextureIdentity_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=nativeTextureIdentity refused\n");valid=false;}
    if(!nativeCollisionState_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=collision refused\n");valid=false;}
    if(!nativeHsdReservation_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=reservations refused\n");valid=false;}
    if(!nativeGreenGrid_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=green-grid refused\n");valid=false;}
    if(!nativeArticleResource_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=article-resources refused\n");valid=false;}
    if(!nativeArticleModelScene_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=article-models refused\n");valid=false;}
    if(!nativeMatchArena_AddRoot(arena,&fighter_collision_state,sizeof(fighter_collision_state))){fprintf(stderr,"[snapshot-census] control-admission owner=fighter-collision refused\n");valid=false;}
    if(valid && !nativeMatchOsHeap_AdmitHistory(arena)){fprintf(stderr,"[snapshot-census] control-admission owner=os-heap refused\n");valid=false;}
    return valid;
}
