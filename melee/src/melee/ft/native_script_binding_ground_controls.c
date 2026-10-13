#include "native_script_binding_ground_controls.h"
#include "native_fighter_lifetime.h"
#include "ftcommon.h"
#include "ftdata.h"
#include "fighter.h"
#include "kinds/ftCommon/ftCo_AttackS4.h"
#include "kinds/ftCommon/ftCo_AttackHi4.h"
#include "kinds/ftCommon/ftCo_AttackLw4.h"
#include "kinds/ftCommon/ftCo_AttackS3.h"
#include "kinds/ftCommon/ftCo_AttackHi3.h"
#include "kinds/ftCommon/ftCo_AttackLw3.h"
#include "kinds/ftCommon/ftCo_Attack1.h"
#include "kinds/ftCommon/ftCo_Jump.h"
#include "kinds/ftCommon/ftCo_Dash.h"
#include "kinds/ftCommon/ftCo_Squat.h"
#include "kinds/ftCommon/ftCo_SquatWait.h"
#include "kinds/ftCommon/ftCo_Turn.h"
#include "kinds/ftCommon/ftCo_Walk.h"
#include "kinds/ftCommon/ftCo_SpecialS.h"
#include "kinds/ftCommon/ftCo_Guard.h"
#include "kinds/ftCommon/ftCo_AppealS.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftCo_ItemThrow.h"
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* message)
{if(d)snprintf(d->message,sizeof(d->message),"%s",message);return false;}

static bool current(NativeScriptRuntimeContext* context,
    const NativeFighterLifetime* source,NativeScriptDiagnostic* d)
{
    if(context->source!=source->object || context->source_class!=NativeCallbackSource_Fighter ||
       context->generation!=source->generation || context->scene_epoch!=source->reference.lifetime ||
       !nativeFighterLifetime_Current(source) || !nativeScriptBindings_SourceAlive(context))
        return error(d,"Ground control Fighter identity, owned storage or program retired");
    return true;
}

bool nativeScriptBindingGroundControls_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)selectors;
    if(!context || !output || count || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context))return error(d,"Landing lag requires a live Fighter and no selectors");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source) || !current(context,&source,d))return false;
    NativeScriptValue value={.type=NativeScriptType_F32};
    switch(semantic) {
    case NativeScriptBinding_StatsLandingAirNeutralLag:value.as.f32=source.data->co_attrs.landingairn_lag;break;
    case NativeScriptBinding_StatsLandingAirBackLag:value.as.f32=source.data->co_attrs.landingairb_lag;break;
    default:return error(d,"Unknown ground control observation");
    }
    if(!current(context,&source,d))return false;
    *output=value;return true;
}

bool nativeScriptBindingGroundControls_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    (void)arguments;
    NativeMatchPoolReference reference;
    if(!context || !output || count || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference) || reference.lifetime!=context->scene_epoch ||
       !nativeScriptBindings_SourceAlive(context))return error(d,"Ground arbitration requires its live Fighter and no arguments");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source) ||
       !source.source_owned || !current(context,&source,d) ||
       nativeScriptBindings_SourceFighterLayout(context,d)!=&source.program->bank->motion_layout)
        return error(d,"Ground arbitration requires its exact owned source program and motion layout");
    if(!p_ftCommonData)return error(d,"Ground arbitration common input data is unavailable");
    NativeScriptValue value={.type=NativeScriptType_Bool};
    switch(semantic) {
    case NativeScriptBinding_ControlsTryCatch:value.as.u64=ftCo_Catch_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryHeavyItemThrow:value.as.u64=ftCo_80094EA4(source.object);break;
    case NativeScriptBinding_ControlsTryTaunt:value.as.u64=ftCo_800DE9D8(source.object);break;
    case NativeScriptBinding_ControlsTryGuard:value.as.u64=ftCo_80091A4C(source.object);break;
    case NativeScriptBinding_ControlsTrySideSpecial:value.as.u64=ftCo_SpecialS_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryUpSpecial:value.as.u64=ftCo_Attack100_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryNeutralSpecial:value.as.u64=ftCo_800D6824(source.object);break;
    case NativeScriptBinding_ControlsTryDownSpecial:value.as.u64=ftCo_800D68C0(source.object);break;
    case NativeScriptBinding_ControlsTrySideSmash:value.as.u64=ftCo_AttackS4_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryUpSmash:value.as.u64=ftCo_AttackHi4_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryDownSmash:value.as.u64=ftCo_AttackLw4_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTrySideTilt:value.as.u64=ftCo_AttackS3_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryUpTilt:value.as.u64=ftCo_AttackHi3_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryDownTilt:value.as.u64=ftCo_AttackLw3_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryJab:value.as.u64=ftCo_Attack1_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryGroundJump:value.as.u64=ftCo_Jump_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryDash:value.as.u64=ftCo_Dash_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryCrouch:value.as.u64=ftCo_800D5FB0(source.object);break;
    case NativeScriptBinding_ControlsTryTurn:value.as.u64=ftCo_Turn_CheckInput(source.object);break;
    case NativeScriptBinding_ControlsTryWalk:value.as.u64=ftCo_Walk_CheckInput(source.object);break;
    case NativeScriptBinding_StateCrouchWait:value.type=NativeScriptType_Void;ftCo_800D638C(source.object);break;
    default:return error(d,"Unknown ground arbitration operation");
    }
    if(!current(context,&source,d) ||
       nativeScriptBindings_SourceFighterLayout(context,d)!=&source.program->bank->motion_layout)return false;
    *output=value;return true;
}
