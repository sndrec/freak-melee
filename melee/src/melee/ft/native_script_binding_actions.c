#include "native_script_binding_actions.h"
#include "native_script_bindings.h"
#include "native_script_fighter.h"
#include "native_callback_lifetime.h"
#include "native_fighter_runtime.h"
#include "native_fighter_lifetime.h"
#include "ftcpuattack.h"
#include "ftcmdscript.h"
#include "ftcpu_profile.h"
#include "ftcommon.h"
#include "ftparts.h"
#include "ftdata.h"
#include "fighter.h"
#include "types.h"
#include "kinds/ftCommon/ftCo_Wait.h"
#include "kinds/ftCommon/ftCo_Fall.h"
#include "kinds/ftCommon/ftCo_AppealS.h"
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
bool nativeScriptBindingActions_Read(NativeScriptRuntimeContext* context,uint32_t semantic,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!context || !out || context->source_class!=NativeCallbackSource_Fighter || !nativeScriptBindings_SourceAlive(context))return error(d,"Fighter action property source is unavailable");
    Fighter* fp=((HSD_GObj*)context->source)->user_data;NativeScriptValue value={.type=NativeScriptType_I32};
    if(semantic==NativeScriptBinding_CPUGrabAttempts)value.as.i64=fp->cpu.x94;
    else if(semantic==NativeScriptBinding_CPULevel)value.as.i64=fp->cpu.level;
    else if(semantic==NativeScriptBinding_CPUBodyHalfWidth){value.type=NativeScriptType_F32;memcpy(&value.as.f32,&fp->cpu.x564,4);}
    else return error(d,"Unknown fighter action property");
    *out=value;return true;
}
bool nativeScriptBindingActions_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic==NativeScriptBinding_ControlsTauntActions) {
        if(!context || !out || count || context->source_class!=NativeCallbackSource_Fighter ||
           !nativeScriptBindings_SourceFighterLayout(context,d))
            return error(d,"Taunt inputs require a live owned Fighter and no arguments");
        NativeFighterLifetime saved;Fighter* fighter=((HSD_GObj*)context->source)->user_data;
        if(!nativeFighterLifetime_Capture(fighter,&saved))return error(d,"Taunt input source identity is unavailable");
        ftCo_AppealS_IASA(context->source);
        if(!nativeFighterLifetime_Current(&saved) || !nativeScriptBindings_SourceFighterLayout(context,d))
            return error(d,"Taunt input source retired/faulted during a native input decision");
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_CollisionGroundWithNudge ||
       (semantic>=NativeScriptBinding_CollisionGroundStandard && semantic<=NativeScriptBinding_CollisionAirSpecialLanding))
        return nativeScriptBindingCollision_Invoke(context,semantic,args,count,out,d);
    if(semantic>=NativeScriptBinding_EffectsSpawn || (semantic>=NativeScriptBinding_Reflector && semantic<=NativeScriptBinding_ReflectorUpdate))
        return nativeScriptBindingFeedback_Invoke(context,semantic,args,count,out,d);
    (void)count;
    if(!context || !out || context->source_class!=NativeCallbackSource_Fighter || !nativeScriptBindings_SourceAlive(context))return error(d,"Fighter action source is unavailable");
    HSD_GObj* object=context->source;Fighter* fp=object->user_data;NativeScriptValue value={.type=NativeScriptType_Void};
    bool owned=nativeFighterRuntime_SourcePackage(fp->kind)!=NULL;
    if(owned && !nativeScriptBindings_SourceFighterLayout(context,d))return false;
    switch(semantic) {
    case NativeScriptBinding_CPURunScript: {
        int64_t index=args[0].as.i64;
        // The previous source emitter deliberately ignored invalid script IDs.
        if(index>0 && index<FT_CPU_SCRIPT_COUNT) {
            if(!Fighter_804D64FC || !Fighter_804D64FC->cmdscripts[index])return error(d,"CPU common script data is absent");
            ftCo_800B4880(fp,(int)index);
        }
        break;
    }
    case NativeScriptBinding_CPUAddCandidate:ftCo_CpuAddAttackCandidate(fp,(int)args[0].as.i64);break;
    case NativeScriptBinding_CPURangedUnsafe: {
        if(!isfinite(args[0].as.f32))return error(d,"CPU ranged distance is not finite");
        Fighter* target=fp->cpu.x44;HSD_GObj* target_object=NULL;uint32_t generation=0;
        if(target) {
            if(HSD_GObjPLinkHead)for(HSD_GObj* at=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];at;at=at->next)
                if(at->user_data==target && nativeCallbackSource_Snapshot(at,NativeCallbackSource_Fighter,&generation)){target_object=at;break;}
            if(!target_object)return error(d,"CPU ranged target is not a live fighter");
        }
        value.type=NativeScriptType_Bool;value.as.u64=ftCo_CpuRangedAttackUnsafe(fp,target,args[0].as.f32);
        if(target_object && !nativeCallbackSource_Alive(target_object,generation,NativeCallbackSource_Fighter))return error(d,"CPU ranged target retired during collision query");
        break;
    }
    case NativeScriptBinding_ControlsGroundActions:ftCo_Wait_IASA(object);break;
    case NativeScriptBinding_ControlsAirActions:(void)ftCo_Fall_IASA_Inner(object);break;
    case NativeScriptBinding_FacingFromInput:ftCommon_UpdateFacing(fp);break;
    case NativeScriptBinding_VisualsHideGroup:
        if((uint64_t)args[0].as.i64>1)return error(d,"Visibility group exceeds the native two-group schema");
        ftParts_80074A4C(object,(int)args[0].as.i64,0);break;
    case NativeScriptBinding_ArticlesStopCue:
        if(!nativeScriptFighter_BindMotionPhase(object,NativeScriptMotion_Accessory,UINT32_MAX,false))return error(d,"Cue clearing requires a live source motion phase");break;
    default:return error(d,"Unknown fighter action operation");
    }
    if(!nativeScriptBindings_SourceAlive(context) || (owned && !nativeScriptBindings_SourceFighterLayout(context,d)))return error(d,"Fighter action source retired before publication");
    *out=value;return true;
}
