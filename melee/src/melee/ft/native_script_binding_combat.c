#include "native_script_bindings.h"
#include "native_script_fighter.h"
#include "native_fighter_lifetime.h"
#include "native_fighter_damage.h"
#include "native_source_package.h"
#include "ftcommon.h"
#include "ftcoll.h"
#include "ftparts.h"
#include "types.h"
#include "fighter.h"
#include "kinds/ftCommon/ftCo_Damage.h"
#include "native_script_moves.h"
#include <melee/it/item.h>
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/random.h>
#include <stdio.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{ if(diagnostic)snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);return false; }

static bool define_hurtbox(NativeScriptRuntimeContext* context,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    if(!values || !result || count!=11 || values[0].type!=NativeScriptType_I32 ||
       values[0].as.i64<INT32_MIN || values[0].as.i64>INT32_MAX ||
       values[2].type!=NativeScriptType_I32 || values[2].as.i64<HurtHeight_Low ||
       values[2].as.i64>HurtHeight_High || values[3].type!=NativeScriptType_Bool ||
       values[3].as.u64>1)
        return error(diagnostic,"Hurtbox definition requires an Int slot/height and canonical grabbable Bool.");
    for(unsigned i=4;i<11;++i)if(values[i].type!=NativeScriptType_F32)
        return error(diagnostic,"Hurtbox endpoints and scale require Float arguments.");
    uint64_t bone;
    switch(values[1].type) {
    case NativeScriptType_Bool:
        if(values[1].as.u64>1)return error(diagnostic,"Hurtbox bone Bool must be canonical.");
        bone=values[1].as.u64;break;
    case NativeScriptType_I32:
        if(values[1].as.i64<INT32_MIN || values[1].as.i64>INT32_MAX)
            return error(diagnostic,"Hurtbox bone Int exceeds its declared width.");
        // fall through
    case NativeScriptType_I64:bone=(uint64_t)values[1].as.i64;break;
    case NativeScriptType_U32:
        if(values[1].as.u64>UINT32_MAX)return error(diagnostic,"Hurtbox bone UInt exceeds its declared width.");
        // fall through
    case NativeScriptType_U64:bone=values[1].as.u64;break;
    default:return error(diagnostic,"Hurtbox bone must be a raw integral joint index.");
    }
    NativeMatchPoolReference reference;
    if(!context || context->source_class!=NativeCallbackSource_Fighter || !context->scene_epoch ||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference) ||
       reference.lifetime!=context->scene_epoch ||
       ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER ||
       !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Hurtbox definition requires its live physical Fighter.");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source) ||
       !source.source_owned || source.generation!=context->generation ||
       source.reference.lifetime!=context->scene_epoch ||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout ||
       !nativeFighterLifetime_Current(&source))
        return error(diagnostic,"Hurtbox definition requires its retained owned Fighter program/data.");
    Fighter* fighter=source.data;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    if(!package || package->address!=package || !package->ready || !package->assets.ready ||
       package->target!=fighter->kind || package->program.bank!=source.program->bank ||
       !ftPartsTable || ftPartsTable[fighter->kind]!=&package->assets.parts || !fighter->parts ||
       !package->assets.parts.parts_num || package->assets.parts.parts_num>MAX_FT_PARTS ||
       package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count ||
       package->assets.parts.part_to_joint!=package->assets.part_to_joint ||
       package->assets.parts.joint_to_part!=package->assets.joint_to_part ||
       fighter->hurt_capsules_len>sizeof(fighter->hurt_capsules)/sizeof(fighter->hurt_capsules[0]))
        return error(diagnostic,"Hurtbox definition requires its imported joint table and bounded capsule storage.");
    // Match the original source operation: an absent slot/bone is a no-op,
    // never a narrowed wide index or an implicitly appended capsule.
    if(values[0].as.i64>=0 && (uint64_t)values[0].as.i64<fighter->hurt_capsules_len &&
       bone<package->assets.parts.parts_num) {
        if(!fighter->parts[bone].joint)return error(diagnostic,"Hurtbox joint is absent from the current costume.");
        ftHurtboxInit init={.bone_idx=(int)bone,.height=(HurtHeight)values[2].as.i64,
            .is_grabbable=values[3].as.u64!=0,
            .a_offset={values[4].as.f32,values[5].as.f32,values[6].as.f32},
            .b_offset={values[7].as.f32,values[8].as.f32,values[9].as.f32},.scale=values[10].as.f32};
        ftColl_HurtboxInit(fighter,&fighter->hurt_capsules[values[0].as.i64],&init);
    }
    if(!nativeFighterLifetime_Current(&source))
        return error(diagnostic,"Hurtbox source changed before publication.");
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

static bool set_bone_status(NativeScriptRuntimeContext* context,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    if(!values||!result||count!=2||values[0].type!=NativeScriptType_I32||
       values[0].as.i64<INT32_MIN||values[0].as.i64>INT32_MAX||
       values[1].type!=NativeScriptType_I32||values[1].as.i64<HurtCapsule_Enabled||
       values[1].as.i64>HurtCapsule_Intangible)
        return error(diagnostic,"Bone hurt status requires a raw joint Int and normal/invincible/intangible status.");
    NativeMatchPoolReference reference;
    if(!context||context->source_class!=NativeCallbackSource_Fighter||!context->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,context->source,&reference)||
       reference.lifetime!=context->scene_epoch||
       ((HSD_GObj*)context->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Bone hurt status requires its live physical Fighter.");
    NativeFighterLifetime source;
    if(!nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source)||
       !source.source_owned||source.generation!=context->generation||
       source.reference.lifetime!=context->scene_epoch||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout||
       !nativeFighterLifetime_Current(&source))
        return error(diagnostic,"Bone hurt status requires its retained owned Fighter program/data.");
    Fighter* fighter=source.data;
    const NativeSourcePackage* package=nativeFighterRuntime_SourcePackage(fighter->kind);
    if(!package||package->address!=package||!package->ready||!package->assets.ready||
       package->target!=fighter->kind||package->program.bank!=source.program->bank||
       !ftPartsTable||ftPartsTable[fighter->kind]!=&package->assets.parts||!fighter->parts||
       !package->assets.parts.parts_num||package->assets.parts.parts_num>MAX_FT_PARTS||
       package->assets.parts.parts_num!=package->assets.facts.joints+package->assets.facts.omission_count||
       package->assets.parts.part_to_joint!=package->assets.part_to_joint||
       package->assets.parts.joint_to_part!=package->assets.joint_to_part||
       fighter->hurt_capsules_len>sizeof(fighter->hurt_capsules)/sizeof(fighter->hurt_capsules[0]))
        return error(diagnostic,"Bone hurt status requires its imported joint table and bounded capsules.");
    int32_t bone=(int32_t)values[0].as.i64;
    if(bone>=0&&(uint32_t)bone<package->assets.parts.parts_num)
        for(uint32_t i=0;i<fighter->hurt_capsules_len;++i)
            if(fighter->hurt_capsules[i].capsule.bone_idx==bone) {
                // Native changes the first matching capsule only. Enabling one
                // capsule does not clear the fighter's aggregate hurt flag.
                ftColl_8007B128(source.object,bone,(HurtCapsuleState)values[1].as.i64);
                break;
            }
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

bool nativeScriptBindingCombat_Read(NativeScriptRuntimeContext* context,uint32_t semantic,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_DamageReactionSuppressed ||
       semantic==NativeScriptBinding_DamageKnockbackLevel) {
        NativeFighterLifetime source;
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source)||
           !source.source_owned||source.generation!=context->generation||
           source.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&source)||!p_ftCommonData)
            return error(diagnostic,"Damage reaction observation requires its live owned Fighter and common parameters.");
        if(semantic==NativeScriptBinding_DamageReactionSuppressed)
            *result=(NativeScriptValue){.type=NativeScriptType_Bool,
                .as.u64=ftCo_8008E984(source.data)};
        else
            *result=(NativeScriptValue){.type=NativeScriptType_I32,
                .as.i64=ftCo_Damage_CalcKnockbackLevel(ftCo_ScaleBy154(source.data->dmg.kb_applied))};
        return true;
    }
    if(semantic==NativeScriptBinding_DamageReverseKnockdownPose) {
        NativeFighterLifetime saved;
        if(!context||!result||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||
           !saved.source_owned||saved.generation!=context->generation||
           saved.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&saved))
            return error(diagnostic,"Knockdown pose policy requires its live owned Fighter.");
        *result=(NativeScriptValue){.type=NativeScriptType_Bool,
            .as.u64=saved.data->x2226_b1};
        return true;
    }
    Fighter* fp=((HSD_GObj*)context->source)->user_data;
    switch(semantic) {
    case NativeScriptBinding_DamageSongImmune:result->as.u64=fp->x2222_b2;break;
    case NativeScriptBinding_DamageArmor:result->as.f32=fp->dmg.armor0;break;
    case NativeScriptBinding_DamageJumpArmor:result->as.f32=fp->dmg.armor1;break;
    case NativeScriptBinding_DamageInHitstun:result->as.u64=fp->x221C_b6;break;
    case NativeScriptBinding_HurtNormal:result->as.i64=HurtCapsule_Enabled;break;
    case NativeScriptBinding_HurtInvincible:result->as.i64=HurtCapsule_Disabled;break;
    case NativeScriptBinding_HurtIntangible:result->as.i64=HurtCapsule_Intangible;break;
    case NativeScriptBinding_HurtboxesStatus:result->as.i64=ftColl_8007B868(context->source);break;
    default:return error(diagnostic,"Unsupported combat property.");
    }
    return true;
}

bool nativeScriptBindingCombat_Write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_DamageReverseKnockdownPose) {
        NativeFighterLifetime saved;
        if(!context||!value||value->type!=NativeScriptType_Bool||value->as.u64>1||
           context->source_class!=NativeCallbackSource_Fighter||!nativeScriptBindings_SourceAlive(context)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&saved)||
           !saved.source_owned||saved.generation!=context->generation||
           saved.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&saved.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&saved))
            return error(diagnostic,"Knockdown pose policy requires its owned Fighter and canonical Bool.");
        saved.data->x2226_b1=value->as.u64!=0;
        return true;
    }
    Fighter* fp=((HSD_GObj*)context->source)->user_data;
    switch(semantic) {
    case NativeScriptBinding_DamageSongImmune:fp->x2222_b2=value->as.u64!=0;break;
    case NativeScriptBinding_DamageArmor:fp->dmg.armor0=value->as.f32;break;
    case NativeScriptBinding_DamageJumpArmor:fp->dmg.armor1=value->as.f32;break;
    default:return error(diagnostic,"Unsupported combat property write.");
    }
    return true;
}

static bool interrupt_cleanup(NativeScriptRuntimeContext* context,uint32_t count,
    NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(!result || count)return error(diagnostic,"Interruption cleanup takes no arguments");
    NativeFighterLifetime source;
    if(!context || context->source_class!=NativeCallbackSource_Fighter ||
       !nativeScriptBindings_SourceAlive(context) ||
       !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source) ||
       !source.source_owned || source.generation!=context->generation ||
       source.reference.lifetime!=context->scene_epoch ||
       nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout ||
       !nativeFighterLifetime_Current(&source))
        return error(diagnostic,"Interruption cleanup requires its exact live owned Fighter");
    // Shared interruption work: stop two voice channels, then invoke damage
    // and death cleanup in native order. Clearing registrations omits the work.
    ftCommon_8007DB58(source.object);
    if(!nativeFighterLifetime_Current(&source) || !nativeScriptBindings_SourceAlive(context))
        return error(diagnostic,"Interruption cleanup retired or faulted its source before publication");
    *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}

bool nativeScriptBindingCombat_Invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* values,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* diagnostic)
{
    if(semantic==NativeScriptBinding_DamageInterruptCleanup)
        return interrupt_cleanup(context,count,result,diagnostic);
    if(semantic==NativeScriptBinding_DamageUpdateTimer || semantic==NativeScriptBinding_PhysicsDamageMotion){
        if(!result || count)return error(diagnostic,"Damage timer/physics operations take no arguments.");
        NativeFighterLifetime source;
        if(!context || context->source_class!=NativeCallbackSource_Fighter ||
           !nativeScriptBindings_SourceAlive(context) ||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source) ||
           !source.source_owned || source.generation!=context->generation ||
           source.reference.lifetime!=context->scene_epoch ||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout ||
           !nativeFighterLifetime_Current(&source) || !p_ftCommonData)
            return error(diagnostic,"Damage timer/physics requires its exact owned Fighter program/storage and common parameters.");
        if(semantic==NativeScriptBinding_DamageUpdateTimer)ftCo_8008F744(source.object);
        else ftCo_Damage_Phys(source.object);
        if(!nativeFighterLifetime_Current(&source) || !nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Damage timer/physics retired its actor or program before publication.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_DamageEnter || semantic==NativeScriptBinding_DamageDefaultReaction) {
        bool specific=semantic==NativeScriptBinding_DamageEnter;
        if(!values||!result||count!=(specific?2:1)||
           values[specific?1:0].type!=NativeScriptType_F32||!isfinite(values[specific?1:0].as.f32)||
           (specific&&(values[0].type!=NativeScriptType_I32||values[0].as.i64< -1||values[0].as.i64>UINT16_MAX)))
            return error(diagnostic,"Damage entry requires a motion Int (-1 or an owned motion) and finite facing Float; default reaction takes only facing.");
        NativeFighterLifetime source;
        if(!context||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source)||
           !source.source_owned||source.generation!=context->generation||
           source.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&source)||!p_ftCommonData)
            return error(diagnostic,"Damage entry requires its exact owned Fighter program/storage and common parameters.");
        if(specific&&values[0].as.i64>=0&&
           !nativeScriptMoves_Contains(&source.program->bank->motion_layout,(uint32_t)values[0].as.i64))
            return error(diagnostic,"Damage motion is outside the owned Fighter's installed table.");
        if(specific)ftCo_8008DCE0(source.object,(int)values[0].as.i64,values[1].as.f32);
        else ftCo_8008E908(source.object,values[0].as.f32);
        if(context->source!=source.object||context->generation!=source.generation||
           context->scene_epoch!=source.reference.lifetime||!nativeFighterLifetime_Current(&source)||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout)
            return error(diagnostic,"Damage source retired or changed during native reaction callbacks.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    if(semantic==NativeScriptBinding_ArticlesNewCollisionGroup) {
        NativeFighterLifetime source;
        if(!context||!result||count||context->source_class!=NativeCallbackSource_Fighter||
           !nativeScriptBindings_SourceAlive(context)||
           !nativeFighterLifetime_Capture(((HSD_GObj*)context->source)->user_data,&source)||
           !source.source_owned||source.generation!=context->generation||
           source.reference.lifetime!=context->scene_epoch||
           nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=&source.program->bank->motion_layout||
           !nativeFighterLifetime_Current(&source))
            return error(diagnostic,"Item hit-group allocation requires its live owned Fighter and no arguments.");
        // Preserve all 32 bits in the language's signed Int word, including
        // IDs past INT32_MAX. Article creation accepts the same word form.
        uint32_t group=Item_8026AE60();
        *result=(NativeScriptValue){.type=NativeScriptType_I32,
            .as.i64=group<=INT32_MAX?(int64_t)group:-1-(int64_t)(UINT32_MAX-group)};
        return true;
    }
    if(semantic==NativeScriptBinding_HurtboxesDefine)
        return define_hurtbox(context,values,count,result,diagnostic);
    if(semantic==NativeScriptBinding_HurtboxesSetBone)
        return set_bone_status(context,values,count,result,diagnostic);
    if(semantic>=NativeScriptBinding_EffectsClearAll && semantic<=NativeScriptBinding_EffectsPauseDuringHitlag) {
        if(!context || !result || count || context->source_class!=NativeCallbackSource_Fighter)
            return error(diagnostic,"Effect control requires a live Fighter and no arguments.");
        HSD_GObj* object=context->source;
        NativeMatchPoolReference reference;
        if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference) ||
           object->classifier!=HSD_GOBJ_CLASS_FIGHTER || !object->user_data ||
           !nativeScriptBindings_SourceAlive(context))
            return error(diagnostic,"Effect control requires a retained physical Fighter.");
        NativeFighterLifetime source;
        if(!nativeFighterLifetime_Capture(object->user_data,&source) || source.object!=object ||
           source.generation!=context->generation || source.reference.lifetime!=context->scene_epoch ||
           (source.source_owned && nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=
                &source.program->bank->motion_layout))
            return error(diagnostic,"Effect control source identity or owned program is unavailable.");
        switch(semantic) {
        case NativeScriptBinding_EffectsClearAll:ftCommon_8007DB24(object);break;
        case NativeScriptBinding_EffectsDestroyAll:efLib_DestroyAll(object);break;
        case NativeScriptBinding_EffectsPause:efLib_PauseAll(object);break;
        case NativeScriptBinding_EffectsResume:efLib_ResumeAll(object);break;
        case NativeScriptBinding_EffectsPauseDuringHitlag:
            if(!source.source_owned ||
               !nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_PreHitlag,UINT32_MAX) ||
               !nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_PostHitlag,UINT32_MAX))
                return error(diagnostic,"Effect hitlag callbacks require an attached owned fighter.");
            source.data->pre_hitlag_cb=efLib_PauseAll;
            source.data->post_hitlag_cb=efLib_ResumeAll;
            break;
        default:return error(diagnostic,"Unsupported effect control.");
        }
        if(!nativeFighterLifetime_Current(&source) || !nativeScriptBindings_SourceAlive(context) ||
           (source.source_owned && nativeScriptBindings_SourceFighterLayout(context,diagnostic)!=
                &source.program->bank->motion_layout))
            return error(diagnostic,"Effect control source changed during native work.");
        *result=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    (void)count;
    HSD_GObj* object=context->source;
    // HSD_Randi multiplies signed Int operands before division. The largest
    // bound whose product with every actual 16-bit draw is defined is 32768.
    if(semantic==NativeScriptBinding_RandomFloat){result->as.f32=HSD_Randf();return true;}
    if(semantic==NativeScriptBinding_RandomInt||semantic==NativeScriptBinding_RandomIntExcluding) {
        int32_t upper=(int32_t)values[0].as.i64;
        if(upper>32768)return error(diagnostic,"Random Int bound exceeds the defined native range 32768.");
        if(semantic==NativeScriptBinding_RandomInt){result->as.i64=upper>0?HSD_Randi(upper):0;return true;}
        if(upper<=0){result->as.i64=-1;return true;}
        int32_t first=(int32_t)values[1].as.i64,second=(int32_t)values[2].as.i64;
        if(first<0||first>=upper)first=upper;
        if(second<0||second>=upper||second==first)second=upper;
        if(second<first){int32_t swap=first;first=second;second=swap;}
        int32_t remaining=upper-(first<upper)-(second<upper);
        if(!remaining){result->as.i64=-1;return true;}
        int32_t selected=HSD_Randi(remaining);
        if(selected>=first)++selected;
        if(selected>=second)++selected;
        result->as.i64=selected;return true;
    }
    switch(semantic) {
    case NativeScriptBinding_FighterSelfDamage:
        if(((Fighter*)object->user_data)->kind==Ft_Kind_MasterH||((Fighter*)object->user_data)->kind==Ft_Kind_CrezyH)
            return error(diagnostic,"Self damage cannot delegate boss stamina behavior.");
        nativeFighterDamage_Self(object,values[0].as.f32);break;
    case NativeScriptBinding_DamageHeal:nativeFighterDamage_Heal(object,values[0].as.f32);break;
    case NativeScriptBinding_HurtboxesSetAll:case NativeScriptBinding_HurtboxesSetBodyStatus: {
        if(values[0].as.i64<HurtCapsule_Enabled||values[0].as.i64>HurtCapsule_Intangible)
            return error(diagnostic,"Hurt status must be normal, invincible or intangible.");
        Fighter* fp=object->user_data;
        if(semantic==NativeScriptBinding_HurtboxesSetAll) {
            if(fp->hurt_capsules_len>sizeof(fp->hurt_capsules)/sizeof(fp->hurt_capsules[0]))return error(diagnostic,"Hurt capsule count exceeds fighter storage.");
            ftColl_8007B0C0(object,(HurtCapsuleState)values[0].as.i64);
        } else ftColl_8007B62C(object,(int)values[0].as.i64);
        break;
    }
    default:return error(diagnostic,"Unsupported combat operation.");
    }
    return true;
}
