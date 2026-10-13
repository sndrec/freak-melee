#include "native_script_binding_guard.h"
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include "ftcoll.h"
#include "ftparts.h"
#include "kinds/ftCommon/ftCo_Guard.h"
#include "kinds/ftCommon/ftCo_Escape.h"
#include "kinds/ftCommon/ftCo_ItemThrow.h"
#include "kinds/ftCommon/ftCo_Pass.h"
#include <melee/lb/types.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}

static bool capture(NativeScriptRuntimeContext* c,NativeFighterLifetime* saved,
    NativeScriptDiagnostic* d)
{
    NativeMatchPoolReference reference;
    if(!c||c->source_class!=NativeCallbackSource_Fighter||!c->scene_epoch||
       !nativeMatchPool_Reference(&nativeGObjPool,c->source,&reference)||
       reference.lifetime!=c->scene_epoch||
       ((HSD_GObj*)c->source)->classifier!=HSD_GOBJ_CLASS_FIGHTER||
       !((HSD_GObj*)c->source)->user_data||!nativeScriptBindings_SourceAlive(c)||
       !nativeFighterLifetime_Capture(((HSD_GObj*)c->source)->user_data,saved)||
       saved->generation!=c->generation||saved->reference.lifetime!=c->scene_epoch||
       !saved->source_owned)
        return error(d,"Guard operation requires its live owned source Fighter");
    if(!nativeScriptBindings_SourceFighterLayout(c,d))return false;
    return nativeFighterLifetime_Current(saved)||
        error(d,"Guard source changed during owned layout admission");
}

bool nativeScriptBindingGuard_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
    NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(!out)return error(d,"Guard property requires an output");
    NativeFighterLifetime saved;if(!capture(c,&saved,d))return false;
    Fighter* f=saved.data;NativeScriptValue value={.type=NativeScriptType_Bool};
    switch(semantic) {
    case NativeScriptBinding_ReflectorActive:value.as.u64=f->reflecting;break;
    case NativeScriptBinding_ReflectorKeepOwner:value.as.u64=f->x2218_b4;break;
    case NativeScriptBinding_ReflectorHitDirection:value.type=NativeScriptType_F32;value.as.f32=f->ReflectAttr.x1A2C_reflectHitDirection;break;
    case NativeScriptBinding_GuardActive:value.as.u64=f->x221B_b0;break;
    case NativeScriptBinding_GuardReactive:value.as.u64=f->x221B_b1;break;
    case NativeScriptBinding_GuardSkipContactDetails:value.as.u64=f->x221B_b2;break;
    case NativeScriptBinding_GuardFrontOnly:value.as.u64=f->x221B_b3;break;
    case NativeScriptBinding_GuardFilteredAttacksOnly:value.as.u64=f->x221B_b4;break;
    case NativeScriptBinding_GuardMeleeHitlag:value.type=NativeScriptType_F32;value.as.f32=f->shield_unk0;break;
    case NativeScriptBinding_GuardArticleHitlag:value.type=NativeScriptType_F32;value.as.f32=f->shield_unk1;break;
    case NativeScriptBinding_GuardHitDamage:value.type=NativeScriptType_I32;value.as.i64=f->x19A4;break;
    case NativeScriptBinding_GuardHitFacing:value.type=NativeScriptType_F32;value.as.f32=f->specialn_facing_dir;break;
    case NativeScriptBinding_GuardElapsed:value.type=NativeScriptType_F32;value.as.f32=f->mv.co.guard.x0;break;
    case NativeScriptBinding_GuardReleaseRequested:value.type=NativeScriptType_Bool;value.as.u64=f->mv.co.guard.xC;break;
    case NativeScriptBinding_GuardMinimumHoldFrames:value.type=NativeScriptType_F32;value.as.f32=f->mv.co.guard.x10;break;
    case NativeScriptBinding_GuardReflectFrames:value.type=NativeScriptType_F32;value.as.f32=f->mv.co.guard.x14;break;
    case NativeScriptBinding_GuardPowershieldFrames:value.type=NativeScriptType_F32;value.as.f32=f->mv.co.guard.x18;break;
    case NativeScriptBinding_GuardInputLockFrames:value.type=NativeScriptType_I32;value.as.i64=f->mv.co.guard.x1C;break;
    case NativeScriptBinding_GuardInterruptMode:value.type=NativeScriptType_I32;value.as.i64=f->mv.co.guard.x20;break;
    case NativeScriptBinding_GuardDelayedActionFrames:value.type=NativeScriptType_I32;value.as.i64=f->mv.co.guard.x24;break;
    case NativeScriptBinding_GuardVolumeEnabled:value.type=NativeScriptType_Bool;value.as.u64=f->x221A_b7;break;
    case NativeScriptBinding_GuardNewPowershield:value.type=NativeScriptType_Bool;value.as.u64=f->x221C_b3;break;
    case NativeScriptBinding_GuardReflectWindow:value.type=NativeScriptType_Bool;value.as.u64=f->x221C_b1;break;
    case NativeScriptBinding_GuardPowershieldWindow:value.type=NativeScriptType_Bool;value.as.u64=f->x221C_b2;break;
    case NativeScriptBinding_ShieldHealth:value.type=NativeScriptType_F32;value.as.f32=f->shield_health;break;
    case NativeScriptBinding_ShieldLightAmount:value.type=NativeScriptType_F32;value.as.f32=f->lightshield_amount;break;
    case NativeScriptBinding_ShieldBone:
        if(!f->ft_data||!f->ft_data->x8)return error(d,"Shield bone data is unavailable");
        value.type=NativeScriptType_U32;value.as.u64=f->ft_data->x8->x11;break;
    case NativeScriptBinding_ShieldInitialSize:value.type=NativeScriptType_F32;value.as.f32=f->co_attrs.initial_shield_size;break;
    default:return error(d,"Unknown guard property");
    }
    if(!nativeFighterLifetime_Current(&saved))return error(d,"Guard source retired before property publication");
    *out=value;return true;
}

bool nativeScriptBindingGuard_Write(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* value,NativeScriptDiagnostic* d)
{
    bool floating=semantic==NativeScriptBinding_GuardMeleeHitlag||semantic==NativeScriptBinding_GuardArticleHitlag||
        semantic==NativeScriptBinding_GuardElapsed||semantic==NativeScriptBinding_GuardMinimumHoldFrames||
        semantic==NativeScriptBinding_GuardReflectFrames||semantic==NativeScriptBinding_GuardPowershieldFrames||
        semantic==NativeScriptBinding_ShieldHealth;
    bool integer=semantic==NativeScriptBinding_GuardInputLockFrames||semantic==NativeScriptBinding_GuardInterruptMode||semantic==NativeScriptBinding_GuardDelayedActionFrames;
    bool boolean=semantic==NativeScriptBinding_ReflectorKeepOwner||semantic==NativeScriptBinding_GuardActive||semantic==NativeScriptBinding_GuardReactive||
        semantic==NativeScriptBinding_GuardSkipContactDetails||semantic==NativeScriptBinding_GuardFrontOnly||
        semantic==NativeScriptBinding_GuardFilteredAttacksOnly||
        semantic==NativeScriptBinding_GuardReleaseRequested||semantic==NativeScriptBinding_GuardVolumeEnabled||
        semantic==NativeScriptBinding_GuardNewPowershield||semantic==NativeScriptBinding_GuardReflectWindow||
        semantic==NativeScriptBinding_GuardPowershieldWindow;
    if(!value||(!floating&&!boolean&&!integer)||
       (floating?value->type!=NativeScriptType_F32:integer?
        (value->type!=NativeScriptType_I32||value->as.i64<INT32_MIN||value->as.i64>INT32_MAX):
        (value->type!=NativeScriptType_Bool||value->as.u64>1)))
        return error(d,"Guard write requires its writable property and exact scalar type");
    NativeFighterLifetime saved;if(!capture(c,&saved,d))return false;
    Fighter* f=saved.data;
    switch(semantic) {
    case NativeScriptBinding_ReflectorKeepOwner:f->x2218_b4=value->as.u64!=0;break;
    case NativeScriptBinding_GuardActive:f->x221B_b0=value->as.u64!=0;break;
    case NativeScriptBinding_GuardReactive:f->x221B_b1=value->as.u64!=0;break;
    case NativeScriptBinding_GuardSkipContactDetails:f->x221B_b2=value->as.u64!=0;break;
    case NativeScriptBinding_GuardFrontOnly:f->x221B_b3=value->as.u64!=0;break;
    case NativeScriptBinding_GuardFilteredAttacksOnly:f->x221B_b4=value->as.u64!=0;break;
    case NativeScriptBinding_GuardMeleeHitlag:f->shield_unk0=value->as.f32;break;
    case NativeScriptBinding_GuardArticleHitlag:f->shield_unk1=value->as.f32;break;
    case NativeScriptBinding_GuardElapsed:f->mv.co.guard.x0=value->as.f32;break;
    case NativeScriptBinding_GuardReleaseRequested:f->mv.co.guard.xC=value->as.u64!=0;break;
    case NativeScriptBinding_GuardMinimumHoldFrames:f->mv.co.guard.x10=value->as.f32;break;
    case NativeScriptBinding_GuardReflectFrames:f->mv.co.guard.x14=value->as.f32;break;
    case NativeScriptBinding_GuardPowershieldFrames:f->mv.co.guard.x18=value->as.f32;break;
    case NativeScriptBinding_GuardInputLockFrames:f->mv.co.guard.x1C=value->as.i64;break;
    case NativeScriptBinding_GuardInterruptMode:f->mv.co.guard.x20=value->as.i64;break;
    case NativeScriptBinding_GuardDelayedActionFrames:f->mv.co.guard.x24=value->as.i64;break;
    case NativeScriptBinding_GuardVolumeEnabled:f->x221A_b7=value->as.u64!=0;break;
    case NativeScriptBinding_GuardNewPowershield:f->x221C_b3=value->as.u64!=0;break;
    case NativeScriptBinding_GuardReflectWindow:f->x221C_b1=value->as.u64!=0;break;
    case NativeScriptBinding_GuardPowershieldWindow:f->x221C_b2=value->as.u64!=0;break;
    case NativeScriptBinding_ShieldHealth:f->shield_health=value->as.f32;break;
    default:return error(d,"Guard property is read-only");
    }
    return nativeFighterLifetime_Current(&saved)||error(d,"Guard source retired after property write");
}

static bool joint_index(const NativeScriptValue* value,uint64_t* index,bool* negative,
    NativeScriptDiagnostic* d)
{
    *negative=false;
    switch(value->type) {
    case NativeScriptType_Bool:
        if(value->as.u64>1)return error(d,"Guard joint Bool must be canonical");
        *index=value->as.u64;return true;
    case NativeScriptType_I32:
        if(value->as.i64<INT32_MIN||value->as.i64>INT32_MAX)
            return error(d,"Guard joint I32 exceeds its declared width");
        // fall through
    case NativeScriptType_I64:
        *negative=value->as.i64<0;*index=(uint64_t)value->as.i64;return true;
    case NativeScriptType_U32:
        if(value->as.u64>UINT32_MAX)return error(d,"Guard joint U32 exceeds its declared width");
        // fall through
    case NativeScriptType_U64:*index=value->as.u64;return true;
    default:return error(d,"Guard raw joint index must be integral");
    }
}

static bool helper(const NativeFighterLifetime* saved,const NativeScriptValue* value,
    uint32_t* function,NativeScriptDiagnostic* d)
{
    if(value->type==NativeScriptType_Null){*function=UINT32_MAX;return true;}
    if(value->type!=NativeScriptType_Callable||value->as.u64>UINT32_MAX)
        return error(d,"Guard reaction requires an owned helper or None");
    const NativeScriptProgramIndex* index=saved->view.program;
    if(value->as.u64>=index->function_count)return error(d,"Guard helper is outside its owned program");
    const NativeScriptFunctionDeclaration* declaration=&index->functions[value->as.u64];
    if(declaration->owner||declaration->callback||declaration->parameter_count||
       declaration->result_type!=NativeScriptType_Void)
        return error(d,"Guard helper must be root-owned, zero-argument and Void");
    *function=(uint32_t)value->as.u64;return true;
}

static bool shield_joint(const NativeFighterLifetime* saved,NativeScriptDiagnostic* d)
{
    Fighter* f=saved->data;
    const NativeSourcePackage* p=nativeFighterRuntime_SourcePackage(f->kind);
    if(!p||!p->ready||p->address!=p||!p->assets.ready||p->target!=f->kind||
       p->program.bank!=saved->program->bank||!ftPartsTable||
       ftPartsTable[f->kind]!=&p->assets.parts||!f->parts||!f->ft_data->x8||
       !p->assets.parts.parts_num||p->assets.parts.parts_num>MAX_FT_PARTS||
       p->assets.parts.parts_num!=p->assets.facts.joints+p->assets.facts.omission_count||
       p->assets.parts.part_to_joint!=p->assets.part_to_joint||
       p->assets.parts.joint_to_part!=p->assets.joint_to_part)
        return error(d,"Shared shield operation requires its exact owned joint table");
    FighterBone* parts=f->parts;
    struct ftData_x8* aux=f->ft_data->x8;
    unsigned index=aux->x11,count=p->assets.parts.parts_num;
    if(index>=count)return error(d,"Owned shield joint is outside its costume rig");
    HSD_JObj* joint=parts[index].joint;
    HSD_ClassInfo* class_info=joint?HSD_CLASS_METHOD(joint):NULL;
    if(!joint||!class_info||class_info->head.obj_size<(int)sizeof(*joint)||
       !hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
        return error(d,"Owned shield joint lacks its native JObj class and extent");
    if(!nativeFighterLifetime_Current(saved)||nativeFighterRuntime_SourcePackage(f->kind)!=p||
       !p->ready||p->address!=p||!p->assets.ready||p->target!=f->kind||
       p->program.bank!=saved->program->bank||!ftPartsTable||ftPartsTable[f->kind]!=&p->assets.parts||
       f->parts!=parts||f->ft_data->x8!=aux||aux->x11!=index||
       p->assets.parts.parts_num!=count||
       p->assets.parts.parts_num!=p->assets.facts.joints+p->assets.facts.omission_count||
       p->assets.parts.part_to_joint!=p->assets.part_to_joint||
       p->assets.parts.joint_to_part!=p->assets.joint_to_part||
       parts[index].joint!=joint||HSD_CLASS_METHOD(joint)!=class_info||
       class_info->head.obj_size<(int)sizeof(*joint))
        return error(d,"Owned shield rig or source changed during class admission");
    return true;
}

bool nativeScriptBindingGuard_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    if(semantic>=NativeScriptBinding_GuardScaleJoint&&semantic<=NativeScriptBinding_CollisionGuardHit) {
        if(!out||count)return error(d,"Shared shield operation requires no arguments and an output");
        NativeFighterLifetime saved;if(!capture(c,&saved,d))return false;
        bool uses_joint=semantic==NativeScriptBinding_GuardScaleJoint||
            semantic==NativeScriptBinding_GuardInstallShield||
            semantic==NativeScriptBinding_GuardInstallReflector||
            semantic==NativeScriptBinding_GuardTickPowershield;
        if(uses_joint&&!shield_joint(&saved,d))return false;
        if((uses_joint||semantic==NativeScriptBinding_GuardDrain||
            semantic>=NativeScriptBinding_ControlsTryPowershield)&&!p_ftCommonData)
            return error(d,"Shared shield tuning data is unavailable");
        NativeScriptValue value={.type=NativeScriptType_Void};
        switch(semantic) {
        case NativeScriptBinding_GuardScaleJoint: {
            float scale;
            if(!ftCo_ShieldScale(saved.data,&scale))
                return error(d,"Shield radius query retired or replaced its source rig");
            Vec3 value={scale,scale,scale};
            HSD_JObjSetScale(saved.data->parts[saved.data->ft_data->x8->x11].joint,&value);
            break;
        }
        case NativeScriptBinding_GuardInstallShield:ftCo_80092450(saved.object);break;
        case NativeScriptBinding_GuardInstallReflector: {
            uint32_t function=saved.program->callbacks[NativeScriptFighterCallback_ReflectHit];
            if(function==UINT32_MAX)return error(d,"Common reflector requires an owned on reflect_hit callback");
            ftCo_8009370C(saved.object,NULL);
            if(!nativeFighterLifetime_Current(&saved)||
               !nativeScriptFighter_BindCallback(saved.object,NativeScriptFighterCallback_ReflectHit,function))
                return error(d,"Common reflector could not bind its owned reaction");
            break;
        }
        case NativeScriptBinding_GuardConsumeReleaseInput:ftCo_80092BCC(saved.object);break;
        case NativeScriptBinding_GuardDrain:value.type=NativeScriptType_Bool;value.as.u64=ftCo_800925A4(saved.object);break;
        case NativeScriptBinding_GuardTickPowershield:ftCo_80093BC0(saved.object);break;
        case NativeScriptBinding_GuardTrackSDI:
            if(!nativeScriptFighter_BindCallback(saved.object,NativeScriptFighterCallback_PostHitlag,UINT32_MAX))
                return error(d,"Shield SDI could not replace its interpreted post-hitlag callback");
            saved.data->hitlag_cb=ftCo_80093240;
            saved.data->post_hitlag_cb=ftCo_800932DC;
            break;
        case NativeScriptBinding_GuardEnterHold:ftCo_800928CC(saved.object);break;
        case NativeScriptBinding_GuardEnterRelease:ftCo_80092BE8(saved.object);break;
        case NativeScriptBinding_ControlsTryPowershield:value.type=NativeScriptType_Bool;value.as.u64=ftCo_80093694(saved.object);break;
        case NativeScriptBinding_ControlsTryGuardItemThrow:value.type=NativeScriptType_Bool;value.as.u64=ftCo_8009515C(saved.object);break;
        case NativeScriptBinding_ControlsTrySpotdodge:value.type=NativeScriptType_Bool;value.as.u64=ftCo_80099794(saved.object);break;
        case NativeScriptBinding_ControlsTryGuardSpotdodge:value.type=NativeScriptType_Bool;value.as.u64=ftCo_8009980C(saved.object);break;
        case NativeScriptBinding_ControlsTryRoll:value.type=NativeScriptType_Bool;value.as.u64=ftCo_8009917C(saved.object);break;
        case NativeScriptBinding_ControlsTryGuardPlatformDrop:value.type=NativeScriptType_Bool;value.as.u64=ftCo_8009A080(saved.object);break;
        case NativeScriptBinding_ControlsGuardReflectActions:ftCo_GuardReflect_IASA(saved.object);break;
        case NativeScriptBinding_PhysicsGuardMotion:ftCo_Guard_Phys(saved.object);break;
        case NativeScriptBinding_PhysicsGuardHitMotion:ftCo_GuardSetOff_Phys(saved.object);break;
        case NativeScriptBinding_CollisionGuardStandard:ftCo_Guard_Coll(saved.object);break;
        case NativeScriptBinding_CollisionGuardHit:ftCo_GuardSetOff_Coll(saved.object);break;
        default:return error(d,"Unknown shared shield operation");
        }
        if(!nativeFighterLifetime_Current(&saved)||!nativeScriptBindings_SourceFighterLayout(c,d))
            return error(d,"Shared shield source retired before output publication");
        *out=value;return true;
    }
    if(!out||(semantic!=NativeScriptBinding_GuardCreate&&semantic!=NativeScriptBinding_GuardUpdatePosition)||
       (semantic==NativeScriptBinding_GuardCreate?count!=6||!args:count!=0))
        return error(d,"Guard operation argument contract is invalid");
    NativeFighterLifetime saved;if(!capture(c,&saved,d))return false;
    if(semantic==NativeScriptBinding_GuardUpdatePosition) {
        ftColl_8007AEE0(c->source);
        if(!nativeFighterLifetime_Current(&saved))return error(d,"Guard source retired during position refresh");
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    uint64_t index;bool negative;uint32_t function;
    if(!joint_index(&args[0],&index,&negative,d)||!helper(&saved,&args[5],&function,d))return false;
    for(unsigned i=1;i<5;++i)if(args[i].type!=NativeScriptType_F32)
        return error(d,"Guard position and radius require Float arguments");
    Fighter* f=saved.data;const NativeSourcePackage* p=nativeFighterRuntime_SourcePackage(f->kind);
    if(!p||!p->ready||p->address!=p||!p->assets.ready||p->target!=f->kind||
       p->program.bank!=saved.program->bank||!ftPartsTable||ftPartsTable[f->kind]!=&p->assets.parts||
       !f->parts||!p->assets.parts.parts_num||p->assets.parts.parts_num>MAX_FT_PARTS||
       p->assets.parts.parts_num!=p->assets.facts.joints+p->assets.facts.omission_count||
       p->assets.parts.part_to_joint!=p->assets.part_to_joint||p->assets.parts.joint_to_part!=p->assets.joint_to_part)
        return error(d,"Guard requires its exact imported costume joint table");
    // Native source admission deliberately makes invalid numeric volumes a
    // healthy no-op. Do not narrow a wide index into a valid different joint.
    if(negative||index>=p->assets.parts.parts_num||!isfinite(args[4].as.f32)||args[4].as.f32<0) {
        if(!nativeFighterLifetime_Current(&saved))return error(d,"Guard source retired before no-op publication");
        *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
    }
    FighterBone* parts=f->parts;uint32_t parts_count=p->assets.parts.parts_num;
    HSD_JObj* joint=parts[index].joint;
    HSD_ClassInfo* joint_class=joint?HSD_CLASS_METHOD(joint):NULL;
    if(!joint||!joint_class||joint_class->head.obj_size<(int)sizeof(*joint)||
       !hsdObjIsDescendantOf((HSD_Obj*)joint,HSD_CLASS_INFO(&hsdJObj)))
        return error(d,"Guard selected joint lacks a native JObj class and extent");
    // Native class initialization may reenter. Do not use an admitted but
    // subsequently replaced object, rig or actor when copying the descriptor.
    if(!nativeFighterLifetime_Current(&saved)||nativeFighterRuntime_SourcePackage(f->kind)!=p||
       !p->ready||p->address!=p||!p->assets.ready||p->target!=f->kind||p->program.bank!=saved.program->bank||
       !ftPartsTable||ftPartsTable[f->kind]!=&p->assets.parts||f->parts!=parts||
       p->assets.parts.parts_num!=parts_count||
       p->assets.parts.parts_num!=p->assets.facts.joints+p->assets.facts.omission_count||
       p->assets.parts.part_to_joint!=p->assets.part_to_joint||p->assets.parts.joint_to_part!=p->assets.joint_to_part||
       parts[index].joint!=joint||
       HSD_CLASS_METHOD(joint)!=joint_class||joint_class->head.obj_size<(int)sizeof(*joint))
        return error(d,"Guard joint or source changed during class admission");
    ShieldDesc shape={.bone=(int)index,.pos={args[1].as.f32,args[2].as.f32,args[3].as.f32},.radius=args[4].as.f32};
    ftColl_8007B1B8(c->source,&shape,NULL);
    if(!nativeFighterLifetime_Current(&saved))return error(d,"Guard source retired during creation");
    if(!nativeScriptFighter_BindCallback(c->source,NativeScriptFighterCallback_ShieldHit,function))
        return error(d,"Guard reaction could not bind its validated owned helper");
    if(!nativeFighterLifetime_Current(&saved))return error(d,"Guard source retired before creation publication");
    *out=(NativeScriptValue){.type=NativeScriptType_Void};return true;
}
