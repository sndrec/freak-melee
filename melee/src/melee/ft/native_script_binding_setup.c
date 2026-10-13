#include "native_script_bindings.h"
#include "native_script_expression.h"
#include "native_script_fighter.h"
#include "native_fighter_lifetime.h"
#include "ftcommon.h"
#include "ftcharacter_assets.h"
#include "ftanim.h"
#include "inlines.h"
#include <melee/it/it_266F.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{ if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false; }

static bool command_index(const NativeScriptValue* values,uint32_t count,
    uint32_t* result,NativeScriptDiagnostic* diagnostic)
{
    if(count!=1||!values||!nativeScriptExpression_IsIntegral(values[0].type))return error(diagnostic,"Command requires one integral selector.");
    uint64_t index;
    if(values[0].type==NativeScriptType_I32||values[0].type==NativeScriptType_I64) {
        if(values[0].as.i64<0)return error(diagnostic,"Command selector is negative.");
        index=(uint64_t)values[0].as.i64;
    } else index=values[0].type==NativeScriptType_Bool?values[0].as.u64!=0:values[0].as.u64;
    if(index>=4)return error(diagnostic,"Command selector is outside 0..3.");
    *result=(uint32_t)index;return true;
}

bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    Fighter* fp=((HSD_GObj*)context->source)->user_data;
    if(semantic==NativeScriptBinding_Command) {
        uint32_t index;if(!command_index(selectors,count,&index,diagnostic))return false;
        uint32_t bits=fp->cmd_vars[index];
        *result=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=bits<=INT32_MAX?(int64_t)bits:(int64_t)bits-INT64_C(0x100000000)};return true;
    }
    if(semantic==NativeScriptBinding_AirGravity)result->as.f32=fp->co_attrs.gravity;
    else if(semantic==NativeScriptBinding_AirFriction)result->as.f32=fp->co_attrs.aerial_friction;
    else return error(diagnostic,"Unsupported fighter setup property.");
    return true;
}

bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* input,NativeScriptDiagnostic* diagnostic)
{
    if(semantic!=NativeScriptBinding_Command)return error(diagnostic,"Unsupported setup property write.");
    uint32_t index;if(!command_index(selectors,count,&index,diagnostic))return false;
    if(!input||!nativeScriptExpression_Assignable(NativeScriptType_I32,input->type))return error(diagnostic,"Command value must be integral.");
    NativeScriptValue value;if(!nativeScriptValue_Convert(input,NativeScriptType_I32,&value,diagnostic))return false;
    ((Fighter*)((HSD_GObj*)context->source)->user_data)->cmd_vars[index]=(u32)value.as.i64;return true;
}

bool nativeScriptBindingSetup_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_CueConsume||semantic==NativeScriptBinding_CollisionUnlockECB) {
        bool cue=semantic==NativeScriptBinding_CueConsume;
        if(!context||!result||count!=(cue?1u:0u)||
           (cue&&(!values||values[0].type!=NativeScriptType_I32||values[0].as.i64<0||values[0].as.i64>7))||
           context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Cue/ECB operation requires its live owned Fighter and typed arguments (cue bit 0..7).");
        NativeFighterLifetime caller;
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller)||
           !caller.source_owned||caller.object!=context->source||
           caller.generation!=context->generation||caller.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&caller.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&caller))
            return error(diagnostic,"Cue/ECB source program, storage or scene changed.");
        if(!cue) {
            ftCommon_UnlockECB(caller.data);
            *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
        }
        bool raised=false;Fighter* fp=caller.data;
        // Named fields retain the native compiler's bitfield layout; do not
        // assume a PPC bit mask when interpreting on a different architecture.
        #define CONSUME_CUE(bit) case bit:raised=fp->throw_flags_b##bit;fp->throw_flags_b##bit=false;break
        switch((unsigned)values[0].as.i64) {
        CONSUME_CUE(0);CONSUME_CUE(1);CONSUME_CUE(2);CONSUME_CUE(3);
        CONSUME_CUE(4);CONSUME_CUE(5);CONSUME_CUE(6);CONSUME_CUE(7);
        }
        #undef CONSUME_CUE
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=raised};return true;
    }
    if(semantic==NativeScriptBinding_CallbackAlignVelocityToFacing) {
        if(!context||!result||count||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Facing-aligned velocity callback requires the live owned Fighter source and no arguments.");
        NativeFighterLifetime caller;
        if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&caller)||!caller.source_owned||
           caller.generation!=context->generation||caller.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,NULL)!=&caller.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&caller))
            return error(diagnostic,"Facing-aligned velocity caller has changed actor, program, storage or scene identity.");
        if(!nativeScriptFighter_BindCallback(caller.object,NativeScriptFighterCallback_Reflect,UINT32_MAX)||
           !nativeFighterLifetime_Current(&caller))
            return error(diagnostic,"Facing-aligned velocity callback requires current owned Reflect storage.");
        caller.data->x21F8=ftCommon_8007F76C;
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_CallbackMatches) {
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||count!=2||!values||
           !nativeScriptBindings_SourceAlive(context)||!nativeScriptExpression_IsIntegral(values[0].type))
            return error(diagnostic,"Callback identity needs a live Fighter, integral slot and owned helper or None");
        uint64_t slot;
        if(values[0].type==NativeScriptType_I32||values[0].type==NativeScriptType_I64) {
            if(values[0].as.i64<0)return error(diagnostic,"Callback slot is negative");slot=(uint64_t)values[0].as.i64;
        } else slot=values[0].as.u64;
        if(slot>=NativeScriptFighterCallback_Count)return error(diagnostic,"Callback slot exceeds the instance schema");
        uint32_t function=UINT32_MAX;
        if(values[1].type==NativeScriptType_Callable) {
            if(values[1].as.u64>UINT32_MAX)return error(diagnostic,"Callback helper ID exceeds its source program");
            function=(uint32_t)values[1].as.u64;
        } else if(values[1].type!=NativeScriptType_Null)return error(diagnostic,"Callback identity needs an owned helper or None");
        bool matches;
        if(!nativeScriptFighter_CallbackMatches(context->source,(uint32_t)slot,function,&matches))
            return error(diagnostic,"Callback identity actor, helper ownership or zero-argument Void ABI is invalid");
        if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Callback query source retired before publication");
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=matches};return true;
    }
    if(semantic>=NativeScriptBinding_CallbackAnimation&&semantic<=NativeScriptBinding_CallbackAccessoryCue) {
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context)||count!=1||!values)
            return error(diagnostic,"Motion callback needs a live Fighter and one helper or None.");
        uint32_t function=UINT32_MAX;
        if(values[0].type==NativeScriptType_Callable) {
            if(values[0].as.u64>UINT32_MAX)return error(diagnostic,"Motion callback function ID exceeds its program domain.");
            function=(uint32_t)values[0].as.u64;
        } else if(values[0].type!=NativeScriptType_Null)
            return error(diagnostic,"Motion callback needs an owned helper or None.");
        bool cue=semantic==NativeScriptBinding_CallbackAccessoryCue;
        uint32_t phase=cue?NativeScriptMotion_Accessory:semantic-NativeScriptBinding_CallbackAnimation;
        if(!nativeScriptFighter_BindMotionPhase(context->source,phase,function,cue))
            return error(diagnostic,"Motion callback actor, ownership or zero-argument Void ABI is invalid.");
        if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Motion callback source retired before publication.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_CallbackAccessoryBefore||semantic==NativeScriptBinding_CallbackAccessoryHitlag) {
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||count!=1||!values||
           !nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Accessory callback requires a live owned Fighter and one helper or None.");
        uint32_t function=UINT32_MAX;
        if(values[0].type==NativeScriptType_Callable) {
            if(values[0].as.u64>UINT32_MAX)return error(diagnostic,"Accessory callback helper exceeds its program domain.");
            function=(uint32_t)values[0].as.u64;
        } else if(values[0].type!=NativeScriptType_Null)return error(diagnostic,"Accessory callback needs an owned helper or None.");
        uint32_t slot=semantic==NativeScriptBinding_CallbackAccessoryBefore?
            NativeScriptFighterCallback_AccessoryBefore:NativeScriptFighterCallback_AccessoryHitlag;
        if(!nativeScriptFighter_BindCallback(context->source,slot,function))return error(diagnostic,"Accessory callback ownership or zero-argument Void ABI is invalid.");
        if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Accessory callback source retired before publication.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_ItemsSetHeldVisible) {
        if(!context||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
           !nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Held visibility requires its live Fighter source");
        HSD_GObj* object=context->source;Fighter* fp=object->user_data;
        NativeFighterLifetime saved;
        if(count!=1||!values||!result||values[0].type!=NativeScriptType_Bool||values[0].as.u64>1||
           !nativeFighterLifetime_Capture(fp,&saved)||!saved.source_owned||
           saved.object!=object||saved.generation!=context->generation||saved.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout)
            return error(diagnostic,"Held visibility requires a Bool and its retained owned Fighter");
        ftCommon_8007F5CC(object,values[0].as.u64!=0);
        if(!nativeFighterLifetime_Current(&saved)||!nativeScriptBindings_SourceAlive(context)||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout)
            return error(diagnostic,"Held visibility source retired or changed during its callback");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    HSD_GObj* object=context->source;Fighter* fp=object->user_data;
    if(semantic==NativeScriptBinding_Command)return nativeScriptBindingSetup_Read(context,semantic,values,count,result,diagnostic);
    if((semantic>=NativeScriptBinding_CallbackDamage&&semantic<=NativeScriptBinding_CallbackStateChanged) ||
       (semantic>=NativeScriptBinding_CallbackReflectHit&&semantic<=NativeScriptBinding_CallbackShieldHit) ||
       semantic==NativeScriptBinding_CallbackSecondaryDamage) {
        uint32_t slot=semantic==NativeScriptBinding_CallbackSecondaryDamage?NativeScriptFighterCallback_SecondaryDamage:
            semantic>=NativeScriptBinding_CallbackReflectHit?
            NativeScriptFighterCallback_ReflectHit+semantic-NativeScriptBinding_CallbackReflectHit:
            semantic-NativeScriptBinding_CallbackDamage;
        uint32_t function=UINT32_MAX;
        if(values[0].type==NativeScriptType_Callable) {
            if(values[0].as.u64>UINT32_MAX)return error(diagnostic,"Callback function ID is outside its program domain.");
            function=(uint32_t)values[0].as.u64;
        } else if(values[0].type!=NativeScriptType_Null)return error(diagnostic,"Callback needs an owned helper or None.");
        return nativeScriptFighter_BindCallback(object,slot,function)?true:error(diagnostic,"Callback actor, ownership or zero-argument Void ABI is unsupported.");
    }
    switch(semantic) {
    case NativeScriptBinding_CueReverseConsume:result->as.u64=fp->throw_flags_b3;fp->throw_flags_b3=false;break;
    case NativeScriptBinding_CueArticleConsume:result->as.u64=fp->throw_flags_b0;fp->throw_flags_b0=false;break;
    case NativeScriptBinding_CueThrowConsume:result->as.u64=fp->throw_flags_b1;fp->throw_flags_b1=false;break;
    case NativeScriptBinding_CueThrowReset:fp->throw_flags=0;break;
    case NativeScriptBinding_CallbackClearDamageDeath:
        if(!nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Damage,UINT32_MAX)||
           !nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Death2,UINT32_MAX))return error(diagnostic,"Callback clearing requires an attached owned actor.");
        break;
    case NativeScriptBinding_CallbackTrackHitlag: {
        uint32_t before=nativeScriptFighter_CallbackFunction(object,NativeScriptFighterCallback_PreHitlag),after=nativeScriptFighter_CallbackFunction(object,NativeScriptFighterCallback_PostHitlag);
        if(before==UINT32_MAX||after==UINT32_MAX)return error(diagnostic,"Hitlag tracking needs both owned Void event handlers.");
        if(!nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_PreHitlag,before)||
           !nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_PostHitlag,after))return error(diagnostic,"Hitlag callback ABI or ownership is invalid.");
        break;
    }
    case NativeScriptBinding_CallbackTrackDamage:case NativeScriptBinding_CallbackTrackDeath2:
    case NativeScriptBinding_CallbackTrackDeath3:case NativeScriptBinding_CallbackTrackDealDamage:
    case NativeScriptBinding_CallbackTrackStateChange:
    case NativeScriptBinding_CallbackTrackReflectHit:case NativeScriptBinding_CallbackTrackReflect:
    case NativeScriptBinding_CallbackTrackHurtboxDetect:case NativeScriptBinding_CallbackTrackShieldHit: {
        uint32_t slot=semantic==NativeScriptBinding_CallbackTrackDamage?NativeScriptFighterCallback_Damage:
            semantic==NativeScriptBinding_CallbackTrackDeath2?NativeScriptFighterCallback_Death2:
            semantic==NativeScriptBinding_CallbackTrackDeath3?NativeScriptFighterCallback_Death3:
            semantic==NativeScriptBinding_CallbackTrackDealDamage?NativeScriptFighterCallback_DealDamage:
            semantic==NativeScriptBinding_CallbackTrackReflectHit?NativeScriptFighterCallback_ReflectHit:
            semantic==NativeScriptBinding_CallbackTrackReflect?NativeScriptFighterCallback_Reflect:
            semantic==NativeScriptBinding_CallbackTrackHurtboxDetect?NativeScriptFighterCallback_HurtboxDetect:
            semantic==NativeScriptBinding_CallbackTrackShieldHit?NativeScriptFighterCallback_ShieldHit:NativeScriptFighterCallback_StateChange;
        uint32_t function=nativeScriptFighter_CallbackFunction(object,slot);
        if(function==UINT32_MAX||!nativeScriptFighter_BindCallback(object,slot,function))return error(diagnostic,"Callback tracking needs its owned matching Void event.");
        break;
    }
    case NativeScriptBinding_FighterEnableWalljump:fp->can_walljump=true;break;
    case NativeScriptBinding_FighterEnableMultiJump:
        if(!fp->dat_attrs_backup||fp->dat_attrs!=fp->dat_attrs_backup)return error(diagnostic,"Multi-jump setup requires selected instance-owned attribute scratch.");
        fp->can_multijump=true;fp->x2D0=fp->dat_attrs;break;
    case NativeScriptBinding_FighterCopyExtAttributes:case NativeScriptBinding_FighterReloadExtAttributes:
        if(!fp->ft_data||!fp->ft_data->ext_attr||!fp->dat_attrs_backup||fp->dat_attrs_backup==fp->ft_data->ext_attr)
            return error(diagnostic,"Fighter extended attribute source/instance scratch is missing or aliases immutable data.");
        if(semantic==NativeScriptBinding_FighterReloadExtAttributes&&fp->dat_attrs!=fp->dat_attrs_backup)
            return error(diagnostic,"Attribute reload requires initialized instance-owned scratch.");
        memcpy(fp->dat_attrs_backup,fp->ft_data->ext_attr,FT_CHARACTER_EXT_ATTR_CAPACITY);fp->dat_attrs=fp->dat_attrs_backup;break;
    case NativeScriptBinding_FighterItemPickup:case NativeScriptBinding_FighterItemDrop:
        if((uint64_t)values[1].as.i64>1||(uint64_t)values[2].as.i64>1)return error(diagnostic,"Item/hand visibility selectors must be 0 or 1.");
        if(semantic==NativeScriptBinding_FighterItemPickup) {
            if(!fp->item_gobj)return error(diagnostic,"Item pickup presentation requires a live held item.");
            if(!itIsHeavy(fp->item_gobj)) {
                int hold=itGetHoldKind(fp->item_gobj);
                if(hold>=1&&hold<=4)ftAnim_80070FB4(object,(int)values[1].as.i64,hold==1?1:hold==2?0:hold==3?2:3);
                if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Pickup source retired during item animation.");
                if(values[0].as.u64)ftAnim_80070C48(object,(int)values[2].as.i64);
            }
        } else {
            ftAnim_80070FB4(object,(int)values[1].as.i64,-1);
            if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Drop source retired during item animation.");
            if(values[0].as.u64)ftAnim_80070CC4(object,(int)values[2].as.i64);
        }
        break;
    case NativeScriptBinding_FighterItemVisible:case NativeScriptBinding_FighterItemInvisible:
        if((uint64_t)values[0].as.i64>1)return error(diagnostic,"Visibility selector must be 0 or 1.");
        if(!fp->item_gobj)return error(diagnostic,"Item visibility requires a held item.");
        if(!itIsHeavy(fp->item_gobj)) {
            if(semantic==NativeScriptBinding_FighterItemVisible)ftAnim_80070C48(object,(int)values[0].as.i64);
            else ftAnim_80070CC4(object,(int)values[0].as.i64);
        }
        break;
    case NativeScriptBinding_FighterKnockbackEnter:case NativeScriptBinding_FighterKnockbackExit:
        if((uint64_t)values[0].as.i64>255)return error(diagnostic,"Knockback presentation selector must be 0..255.");
        ftAnim_800704F0(object,(int)values[0].as.i64,semantic==NativeScriptBinding_FighterKnockbackEnter?3.0f:0.0f);
        if(!nativeScriptBindings_SourceAlive(context))return error(diagnostic,"Knockback source retired between animation channels.");
        ftAnim_800704F0(object,0,semantic==NativeScriptBinding_FighterKnockbackEnter?3.0f:0.0f);break;
    default:return error(diagnostic,"Unsupported fighter setup operation.");
    }
    return true;
}
