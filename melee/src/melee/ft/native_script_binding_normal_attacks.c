#include "native_script_binding_normal_attacks.h"
#include "native_script_bindings.h"
#include "native_fighter_lifetime.h"
#include "native_script_moves.h"
#include "ft_081B.h"
#include "kinds/ftCommon/ftCo_Attack1.h"
#include "kinds/ftCommon/ftCo_Attack100.h"
#include "kinds/ftCommon/ftpickupitem.h"
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}

static bool current(NativeScriptRuntimeContext* context,const NativeFighterLifetime* source,
                    NativeScriptDiagnostic* diagnostic)
{
    if(context->source!=source->object || context->source_class!=NativeCallbackSource_Fighter ||
       context->generation!=source->generation || context->scene_epoch!=source->reference.lifetime ||
       !nativeFighterLifetime_Current(source) || !nativeScriptBindings_SourceAlive(context) ||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source->program->bank->motion_layout)
        return error(diagnostic,"Normal attack source, owned program or layout retired before continuation");
    return true;
}

static bool helper(const NativeFighterLifetime* source,const NativeScriptValue* value,
                   uint32_t* function,NativeScriptDiagnostic* diagnostic)
{
    if(value->type!=NativeScriptType_Callable || value->as.u64>UINT32_MAX)
        return error(diagnostic,"Normal attack completion requires an owned helper code ID");
    uint32_t id=(uint32_t)value->as.u64;
    const NativeScriptProgramIndex* program=&source->program->bank->declarations;
    if(id>=program->function_count)return error(diagnostic,"Normal attack completion helper is outside its program");
    const NativeScriptFunctionDeclaration* declaration=&program->functions[id];
    if(declaration->owner || declaration->callback || !declaration->signature_resolved ||
       declaration->parameter_count || declaration->result_type!=NativeScriptType_Void)
        return error(diagnostic,"Normal attack completion must be a same-program root zero-argument Void helper");
    *function=id;return true;
}

bool nativeScriptBindingNormalAttacks_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    uint32_t expected;
    switch(semantic) {
    case NativeScriptBinding_AttackEnterJab:case NativeScriptBinding_AttackEnterRapidJab:
    case NativeScriptBinding_AttackUpdateRapidJab:case NativeScriptBinding_CollisionAirAerialLandingCallback:expected=1;break;
    case NativeScriptBinding_ItemsTryGroundPickup:expected=0;break;
    default:return error(diagnostic,"Unknown normal attack operation");
    }
    if(!context || !output || count!=expected || (count&&!arguments) ||
       context->source_class!=NativeCallbackSource_Fighter || !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Normal attack requires its live Fighter context and exact argument shape");
    Fighter* fighter=((HSD_GObj*)context->source)->user_data;
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(fighter,&source) || !source.source_owned ||
       !current(context,&source,diagnostic))
        return error(diagnostic,"Normal attack requires its exact published source actor and imported layout");
    NativeScriptValue result={.type=NativeScriptType_Void};
    if(semantic==NativeScriptBinding_AttackEnterJab || semantic==NativeScriptBinding_AttackEnterRapidJab) {
        if(arguments[0].type!=NativeScriptType_I32 || arguments[0].as.i64<0 ||
           arguments[0].as.i64>UINT16_MAX ||
           !nativeScriptMoves_Contains(&source.program->bank->motion_layout,(uint32_t)arguments[0].as.i64))
            return error(diagnostic,"Normal attack motion is outside the published common and owned motion table");
        if(semantic==NativeScriptBinding_AttackEnterJab) {
            result.type=NativeScriptType_Bool;
            result.as.u64=ftCo_Attack11_EnterMotion(source.object,(FtMotionId)arguments[0].as.i64);
        } else ftCo_800D6B00(source.object,(enum_t)arguments[0].as.i64);
    } else if(semantic==NativeScriptBinding_ItemsTryGroundPickup) {
        result.type=NativeScriptType_Bool;result.as.u64=ftpickupitem_80094790(source.object);
    } else {
        uint32_t function;if(!helper(&source,&arguments[0],&function,diagnostic))return false;
        bool complete=semantic==NativeScriptBinding_AttackUpdateRapidJab
            ?ftCo_RapidJab_ShouldFinish(source.object):ft_80081D0C(source.object)==GA_Air;
        if(!current(context,&source,diagnostic))return false;
        if(complete) {
            NativeScriptValue value;
            if(nativeScriptFighter_Helper(source.object,source.generation,function,NULL,0,&value,diagnostic)!=NativeScriptVM_Ok)
                return error(diagnostic,"Normal attack completion helper faulted or retired its source");
            if(value.type!=NativeScriptType_Void)return error(diagnostic,"Normal attack completion returned a non-Void value");
        }
    }
    if(!current(context,&source,diagnostic))return false;
    *output=result;return true;
}
