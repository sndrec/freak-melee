#include "native_match_state_admission.h"
#include "native_match_snapshot.h"
#include "native_match_control_history.h"
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/ef/native_script_effect_snapshot.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#include <sysdolphin/baselib/native_animation_pool.h>
#include <stdio.h>

bool nativeMatchState_Admit(void* scratch, size_t bytes)
{
    NativeMatchSnapshotPart parts[NativeMatchSnapshot_Count]={0};
    size_t captured;
    if (!nativeMatchSnapshot_Capture(parts,scratch,bytes,&captured,NULL)) {
        for(unsigned i=0;i<NativeMatchSnapshot_Count;++i)
            if(!parts[i].validated)fprintf(stderr,"[match-admission] owner=%s bytes=%zu refused\n",
                parts[i].name ? parts[i].name : "missing",parts[i].bytes);
        return false;
    }
#define ADMIT(call) do { if(!(call)) {fprintf(stderr,"[match-admission] %s refused\n",#call);return false;} } while(0)
    ADMIT(nativeHsdArena_AdmitHistory());
    ADMIT(nativeAnimationPool_AdmitHistory(&nativeHsdArena));
    ADMIT(nativeScriptFighter_AdmitHistory(&nativeMatchSceneArena));
    ADMIT(nativeScriptItem_AdmitHistory(&nativeMatchSceneArena));
    ADMIT(nativeScriptEffect_AdmitHistory(&nativeMatchSceneArena));
    ADMIT(nativeMatchControls_AdmitHistory(&nativeMatchSceneArena));
#undef ADMIT
    return true;
}
