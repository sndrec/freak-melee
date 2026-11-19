#include "native_script_binding_air_physics.h"
#include "native_fighter_lifetime.h"
#include "fighter.h"
#include "ft_081B.h"
#include "ft_084E.h"
#include "ftcliffcommon.h"
#include "ftcommon.h"
#include "ftwalljump.h"
#include "kinds/ftCommon/ftCo_FallSpecial.h"
#include <stdio.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

static bool current(NativeScriptRuntimeContext* context,
    const NativeFighterLifetime* source,const void* model,
    const ftCommonData* common,NativeScriptDiagnostic* d)
{
    if(context->source!=source->object || context->source_class!=NativeCallbackSource_Fighter ||
       context->generation!=source->generation || context->scene_epoch!=source->reference.lifetime ||
       !nativeFighterLifetime_Current(source) || !nativeScriptBindings_SourceAlive(context) ||
       source->object->hsd_obj!=model || p_ftCommonData!=common)
        return error(d,"Air physics source, owned schema, model or common data retired");
    return true;
}

bool nativeScriptBindingAirPhysics_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(!context || !output || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context))return error(d,"Air physics requires its live Fighter");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source) ||
       !source.source_owned ||
       nativeScriptBindings_SourceFighterLayout(context,d)!=&source.program->bank->motion_layout)
        return error(d,"Air physics requires its selected owned Fighter data and motion layout");
    const void* model=source.object->hsd_obj;
    const ftCommonData* common=p_ftCommonData;
    if(!current(context,&source,model,common,d))return false;
    switch(semantic) {
    case NativeScriptBinding_PhysicsAirDriftScaled:
        if (!common || count != 1 || !arguments || arguments[0].type != NativeScriptType_F32 ||
            !isfinite(arguments[0].as.f32) || arguments[0].as.f32 <= 0)
            return error(d, "Air drift requires a positive finite acceleration multiplier");
        ftCommon_ApplyScaledAirDrift(source.data, arguments[0].as.f32);
        break;
    case NativeScriptBinding_PhysicsAirDriftWithLimits:
        if(!arguments || count!=3)return error(d,"Bounded air drift requires threshold, acceleration and speed Floats");
        for(unsigned i=0;i<3;++i)
            if(arguments[i].type!=NativeScriptType_F32 || !isfinite(arguments[i].as.f32))
                return error(d,"Bounded air-drift inputs must be finite Floats");
        if(!common)return error(d,"Bounded air drift requires common fastfall data");
        ft_80084E1C(source.object,arguments[0].as.f32,arguments[1].as.f32,arguments[2].as.f32);
        break;
    case NativeScriptBinding_PhysicsAnimationVerticalVelocity:
        if(count)return error(d,"Animation vertical velocity takes no arguments");
        ft_800851C0(source.object);
        break;
    case NativeScriptBinding_PhysicsAnimationVerticalVelocityAdjusted:
        if(count)return error(d,"Adjusted animation velocity takes no arguments");
        // The native Item relation is only a presence test. Never dereference it
        // or require bunny data on the non-multiplying branch.
        if(source.data->x197C && !Fighter_804D6520)
            return error(d,"Bunny animation velocity modifier is unavailable");
        ft_800851D0(source.object);
        break;
    case NativeScriptBinding_CollisionAirLandingCallback: {
        if(count!=1 || !arguments || arguments[0].type!=NativeScriptType_Callable ||
           arguments[0].as.u64>=source.program->bank->declarations.function_count)
            return error(d,"Air landing needs one owned root Void helper; None is forbidden");
        uint32_t function=(uint32_t)arguments[0].as.u64;
        const NativeScriptFunctionDeclaration* declaration=
            &source.program->bank->declarations.functions[function];
        if(!declaration->signature_resolved || declaration->owner || declaration->callback ||
           declaration->parameter_count || declaration->result_type!=NativeScriptType_Void)
            return error(d,"Air landing helper ownership or zero-argument Void ABI is invalid");
        if(!common)return error(d,"Air landing common collision data is unavailable");
        bool landed=ftCommon_ResolveAirLanding(source.object,ftCo_80096CC8);
        if(!current(context,&source,model,common,d))return false;
        if(landed) {
            NativeScriptValue result;
            if(nativeScriptFighter_Helper(source.object,source.generation,function,NULL,0,&result,d)!=NativeScriptVM_Ok)
                return false;
            if(!current(context,&source,model,common,d))return false;
        } else {
            bool wall=ftWallJump_8008169C(source.object);
            if(!current(context,&source,model,common,d))return false;
            if(!wall) {
                (void)ftCliffCommon_80081298(source.object);
                if(!current(context,&source,model,common,d))return false;
            }
        }
        break;
    }
    default:return error(d,"Unknown air physics operation");
    }
    if(!current(context,&source,model,common,d))return false;
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
