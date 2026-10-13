#include "native_script_binding_held_state.h"
#include "native_script_bindings.h"
#include "native_script_expression.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "ftcommon.h"
#include "ftdata.h"
#include "kinds/ftCommon/ftCo_ItemParasolOpen.h"
#include "kinds/ftCommon/forward.h"
#include <stdio.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

static bool current(NativeScriptRuntimeContext* c,const NativeFighterLifetime* s,NativeScriptDiagnostic* d)
{
    return c->source==s->object && c->source_class==NativeCallbackSource_Fighter &&
        c->generation==s->generation && c->scene_epoch==s->reference.lifetime &&
        nativeFighterLifetime_Current(s) && nativeScriptBindings_SourceAlive(c) &&
        nativeScriptBindings_SourceFighterLayout(c,d)==&s->program->bank->motion_layout;
}
static bool capture(NativeScriptRuntimeContext* c,NativeFighterLifetime* s,NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference r;
    if(!c || c->source_class!=NativeCallbackSource_Fighter ||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&r) || r.lifetime!=c->scene_epoch ||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,s) ||
       !s->source_owned || !current(c,s,d))
        return error(d,"Held state requires its exact live owned Fighter and motion layout");
    return true;
}

bool nativeScriptBindingHeldState_Write(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    if(semantic!=NativeScriptBinding_MotionID || !value || !nativeScriptExpression_IsIntegral(value->type))
        return error(d,"Current motion ID requires an integral value");
    uint64_t motion;
    if(value->type==NativeScriptType_I32 || value->type==NativeScriptType_I64) {
        if(value->as.i64<0)return error(d,"Current motion ID is negative");
        motion=(uint64_t)value->as.i64;
    } else motion=value->as.u64;
    NativeFighterLifetime s;if(!capture(c,&s,d))return false;
    if(motion>UINT16_MAX || !nativeScriptMoves_Contains(&s.program->bank->motion_layout,(uint32_t)motion))
        return error(d,"Current motion ID is outside the published motion domain");
    // This deliberately writes the native ID only. Entry/animation work belongs
    // to the explicit state operation that follows in authored code.
    s.data->motion_id=(FtMotionId)motion;
    return current(c,&s,d) || error(d,"Motion ID source retired before completion");
}

bool nativeScriptBindingHeldState_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* d)
{
    if(semantic!=NativeScriptBinding_StateHeldOpen || !arguments || count!=1 || !output ||
       arguments[0].type!=NativeScriptType_F32)
        return error(d,"Held-open entry requires one Float blend and an output");
    NativeFighterLifetime s;if(!capture(c,&s,d))return false;
    const NativeSourcePackage* source=nativeFighterRuntime_SourcePackage(s.kind);
    const NativeScriptMoveLayout* layout=&s.program->bank->motion_layout;
    if(!source || !nativeScriptMoves_Contains(layout,ftCo_MS_ItemParasolOpen) ||
       (layout->common_count&&!source->assets.common_motions) || (layout->count&&!source->assets.motions) ||
       s.data->x18!=(s32)layout->first || s.data->x1C_actionStateList!=source->assets.common_motions ||
       s.data->x20_actionStateList!=source->assets.motions ||
       !s.data->x24 || s.data->x24!=source->assets.character.data.xC ||
       s.data->x58C!=source->assets.facts.actions)
        return error(d,"Held-open motion and animation peers differ from owned assets");
    if(!p_ftCommonData || !isfinite(p_ftCommonData->x59C) ||
       (double)p_ftCommonData->x59C < -2147483648.0 || (double)p_ftCommonData->x59C >= 2147483648.0)
        return error(d,"Held-open common timer cannot be converted to its native integer");
    const ftCommonData* common=p_ftCommonData;
    ftCo_ItemParasolOpen_Enter(s.object,arguments[0].as.f32);
    if(!current(c,&s,d) || p_ftCommonData!=common)
        return error(d,"Held-open source or common data retired during entry");
    *output=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
