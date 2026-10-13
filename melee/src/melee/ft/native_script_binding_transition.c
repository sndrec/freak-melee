#include "native_script_binding_transition.h"
#include "native_script_moves.h"
#include "native_callback_lifetime.h"
#include "fighter.h"
#include "ftanim.h"
#include "ftcommon.h"
#include "types.h"
#include "kinds/ftCommon/forward.h"
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false;}
static bool motionId(const NativeScriptValue* value,uint32_t* output,NativeScriptDiagnostic* diagnostic)
{
    if(!nativeScriptExpression_IsIntegral(value->type))return error(diagnostic,"Motion target must be integral");
    uint64_t id;
    if(value->type==NativeScriptType_I32 || value->type==NativeScriptType_I64){
        if(value->as.i64<0)return error(diagnostic,"Motion target is negative");id=(uint64_t)value->as.i64;
    } else id=value->as.u64;
    if(id>UINT16_MAX)return error(diagnostic,"Motion target exceeds the engine's state domain");
    *output=(uint32_t)id;return true;
}
bool nativeScriptBindingTransition_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    (void)count;(void)result;
    if(!context || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Motion source is unavailable or retired");
    HSD_GObj* object=context->source;Fighter* fighter=object->user_data;
    const NativeScriptMoveLayout* layout=nativeScriptBindings_SourceFighterLayout(context,diagnostic);
    if(!layout)return false;
    uint32_t motion;if(!motionId(&values[0],&motion,diagnostic))return false;
    if(!nativeScriptMoves_Contains(layout,motion))
        return error(diagnostic,"Motion target is outside the installed fighter's actual table");
    uint32_t flags=0;float frame=1,speed=1;bool begin=false;
    switch(semantic) {
    case NativeScriptBinding_StateEnter:break;
    case NativeScriptBinding_StateEnterFrame:frame=values[1].as.f32;break;
    case NativeScriptBinding_StateEnterSpeed:frame=values[1].as.f32;speed=values[2].as.f32;break;
    case NativeScriptBinding_StateBegin:frame=0;begin=true;break;
    case NativeScriptBinding_StateEnterFlags:flags=(uint32_t)values[1].as.u64;frame=0;break;
    case NativeScriptBinding_StateEnterAt:flags=(uint32_t)values[1].as.u64;frame=values[2].as.f32;break;
    case NativeScriptBinding_StateEnterAtSpeed:
        flags=(uint32_t)values[1].as.u64;frame=values[2].as.f32;speed=values[3].as.f32;break;
    case NativeScriptBinding_StateSwitchToAir:
        flags=(uint32_t)values[1].as.u64;ftCommon_8007D5D4(fighter);
        if(!nativeScriptBindings_SourceAlive(context) || !nativeScriptBindings_SourceFighterLayout(context,diagnostic))return error(diagnostic,"Source retired during ground-to-air transition");
        frame=fighter->cur_anim_frame;break;
    case NativeScriptBinding_StateSwitchToGround:
        flags=(uint32_t)values[1].as.u64;ftCommon_8007D7FC(fighter);
        if(!nativeScriptBindings_SourceAlive(context) || !nativeScriptBindings_SourceFighterLayout(context,diagnostic))return error(diagnostic,"Source retired during air-to-ground transition");
        frame=fighter->cur_anim_frame;break;
    default:return error(diagnostic,"Context-dependent transition was not lowered to explicit owned data");
    }
    Fighter_ChangeMotionState(object,(FtMotionId)motion,(MotionFlags)flags,frame,speed,0,NULL);
    if(!nativeScriptBindings_SourceAlive(context) || !nativeScriptBindings_SourceFighterLayout(context,diagnostic))return error(diagnostic,"Source retired during motion change");
    if(begin) {
        ftAnim_8006EBA4(object);
        if(!nativeScriptBindings_SourceFighterLayout(context,diagnostic))return false;
    }
    return true;
}
