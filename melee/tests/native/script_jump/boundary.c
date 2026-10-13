#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
/* Actual engine bodies; source view, motion/assets, audio and input leaves controlled. */
#include "boundary.h"
#include <melee/ft/native_script_binding_jump.h>
#include <melee/ft/native_script_binding_transition.h>
#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/ft_0D27.h>
#include <melee/ft/ft_0C8C.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_AerialAction.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_PassiveWall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/mp/mplib.h>
#include <melee/ef/efsync.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NativeSourcePackage jump_package;
NativeScriptStorageInstance jump_storage;
NativeScriptVM jump_vm;
NativeScriptRuntimeContext jump_context;
Fighter jump_actor,jump_reference;
HSD_GObj jump_object,jump_reference_object;
HSD_JObj jump_root;
NativeScriptVMStatus jump_status=NativeScriptVM_Ok;
unsigned jump_changes,jump_reset,jump_sounds,jump_worn,jump_dispatches;
unsigned jump_stage,jump_stop_at,jump_fault_at,jump_retire_at;
bool jump_hook=true;
char jump_order[64];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static MotionState motions[12];static FighterBone parts[1];
static HSD_GObj* heads[16];HSD_GObj** HSD_GObjPLinkHead=heads;
static ftCommonData common;ftCommonData* p_ftCommonData=&common;
static struct Fighter_804D6524_t scale;
struct Fighter_804D6524_t* Fighter_804D6524=&scale;
static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=part_tables;
static __typeof__(*jump_package.assets.character.data.x4C_sfx) sounds;

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&jump_package:NULL;}
bool nativeScriptItem_HasActors(FighterKind kind){(void)kind;return false;}
bool nativeScriptEffect_HasActors(FighterKind kind){(void)kind;return false;}
bool nativeScriptFighter_HasActors(FighterKind kind){(void)kind;return false;}
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object)
{return object==&jump_object?jump_status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{
    if(object!=&jump_object || !view || generation!=jump_storage.generation || !jump_storage.live)return false;
    *view=(NativeScriptReceiverView){&jump_package.compiler.bank.declarations,&jump_storage};return true;
}
bool nativeScriptFighter_IsAttached(Fighter_GObj* object){return object==&jump_object;}
bool nativeScriptFighter_HasHook(Fighter_GObj* object,uint32_t hook)
{return object==&jump_object && jump_hook && hook==NativeScriptFighter_AerialJump;}
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* object,uint32_t hook,const NativeScriptValue* args,uint32_t count, NativeScriptEventRecordIO* record,NativeScriptValue* result)
{(void)record;
    (void)args;(void)result;
    if(object!=&jump_object || hook!=NativeScriptFighter_AerialJump || count)return NativeScriptVM_Error;
    ++jump_dispatches;const NativeScriptFighterProgram* program=nativeScriptFighter_Program(jump_actor.kind);
    NativeScriptVMStatus status=jumpFixture_Run(program->hooks[hook]);
    if(status!=NativeScriptVM_Ok)jump_status=status;
    return status;
}
void jumpFixture_Retire(unsigned mode)
{
    if(mode==1) {jump_storage.live=false;jump_status=NativeScriptVM_SourceRetired;}
    else if(mode==2)jump_status=NativeScriptVM_Error;
    else {heads[HSD_GOBJ_PLINK_FIGHTER]=&jump_reference_object;jump_object.user_data=(void*)1;}
}
void jumpFixture_Reset(void)
{
    jump_actor=(Fighter){.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.gobj=&jump_object,
        .x8_spawnNum=17,.x1968_jumpsUsed=1,.ground_or_air=GA_Air,.x671_timer_lstick_tilt_y=9,
        .co_attrs={.max_jumps=2,.jump_v_initial_velocity=3,.air_jump_v_multiplier=3,.air_jump_h_multiplier=3}};
    jump_actor.input.lstick[0].x=0.5f;jump_actor.x197C=(HSD_GObj*)1;
    jump_actor.ft_data=&jump_package.assets.character.data;
    jump_reference=jump_actor;jump_reference.kind=Ft_Kind_Mario;jump_reference.behavior_kind=Ft_Kind_Mario;
    jump_reference.gobj=&jump_reference_object;
    jump_object=(HSD_GObj){.user_data=&jump_actor,.next=&jump_reference_object,.hsd_obj=&jump_root};
    jump_reference_object=(HSD_GObj){.user_data=&jump_reference};heads[HSD_GOBJ_PLINK_FIGHTER]=&jump_object;
    jump_package.address=&jump_package;jump_package.ready=jump_package.assets.ready=true;jump_package.target=Ft_Kind_CustomFirst;
    jump_package.assets.facts.moves=jump_package.compiler.bank.motion_layout;
    jump_package.assets.motions=motions;gFtDataList[jump_actor.kind]=jump_actor.ft_data;ftData_CharacterStateTables[jump_actor.kind]=motions;
    sounds.x14=37;jump_package.assets.character.data.x4C_sfx=&sounds;
    parts[0].joint=&jump_root;jump_actor.parts=parts;jump_root=(HSD_JObj){.rotate={.y=2},.flags=JOBJ_MTX_INDEP_SRT};
    jump_package.assets.parts.parts_num=1;ftPartsTable[jump_actor.kind]=&jump_package.assets.parts;
    jump_status=NativeScriptVM_Ok;jump_storage.live=true;jump_storage.generation=17;
    jump_context=(NativeScriptRuntimeContext){.source=&jump_object,.source_class=NativeCallbackSource_Fighter,.generation=17};
    jump_changes=jump_reset=jump_sounds=jump_worn=jump_dispatches=jump_stage=jump_stop_at=jump_fault_at=jump_retire_at=0;
    jump_hook=true;jump_order[0]=0;
    common.tap_jump_threshold=0.7f;common.tap_jump_window=5;common.x78=0.3f;
    common.x768=5;common.x76C=0.5f;common.x770=5;common.x774=7;
    common.x310=2;scale.x30=1;
}
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* source)
{
    (void)flags;(void)blend;(void)source;Fighter* actor=object->user_data;++jump_changes;
    actor->motion_id=id;actor->cur_anim_frame=frame;actor->frame_speed_mul=rate;
    if(jump_fault_at==20 && object==&jump_object)jumpFixture_Retire(2);
    if(jump_retire_at==20 && object==&jump_object)jumpFixture_Retire(3);
}
void ft_80088770(Fighter* actor){++jump_reset;if(actor==&jump_actor && jump_retire_at==21)jumpFixture_Retire(1);}
void ft_80088328(Fighter* actor,enum_t sound,u8 volume,u8 pan)
{if(sound!=37 || volume!=127 || pan!=64)abort();++jump_sounds;if(actor==&jump_actor && jump_retire_at==22)jumpFixture_Retire(1);}
void ft_PlaySFX(Fighter* actor,enum_t sound,u8 volume,u8 pan){(void)actor;if(sound!=0x11b || volume!=127 || pan!=64)abort();++jump_worn;}
void ftCommon_8007D5D4(Fighter* actor){actor->ground_or_air=GA_Air;}
bool ft_800D2D0C(Fighter_GObj* object){(void)object;return false;}
void ft_800D2E7C(Fighter_GObj* object,Vec3* velocity){(void)object;(void)velocity;abort();}
void ftNs_JumpAerial_Enter(Fighter_GObj* object){(void)object;abort();}
void ftYs_JumpAerial_Enter(Fighter_GObj* object){(void)object;abort();}
void ftPe_JumpAerial_Enter(Fighter_GObj* object){(void)object;abort();}
void ftMt_JumpAerial_Enter(Fighter_GObj* object){(void)object;abort();}
void ftCo_JumpAerial_Phys_Cb(Fighter_GObj* object){(void)object;abort();}
s32 ftCo_800D7268(Fighter* actor){(void)actor;return 0;}
bool ftCo_800D72A0(Fighter* actor){(void)actor;return false;}
static bool input(unsigned stage,HSD_GObj* object)
{
    jump_order[jump_stage++]=(char)('A'+stage-1);jump_order[jump_stage]=0;
    if(object==&jump_object && jump_fault_at==stage)jumpFixture_Retire(2);
    if(object==&jump_object && jump_retire_at==stage)jumpFixture_Retire(1);
    return jump_stop_at==stage;
}
bool ftCo_800C5240(Fighter_GObj* object){if(jump_stage)input(9,object);return false;}
bool ftCo_SpecialAir_CheckInput(Fighter_GObj* object){return input(1,object);}
bool ftCo_80095328(Fighter_GObj* object,bool* callback,struct NativeItemLifetime* retained){(void)callback;(void)retained;return input(2,object);}
bool ftCo_800D7100(Fighter_GObj* object){return input(3,object);}
bool ftCo_800C3B10(Fighter_GObj* object){return input(4,object);}
bool ftCo_80099A58(Fighter_GObj* object){return input(5,object);}
bool ftCo_AttackAir_CheckItemThrowInput(Fighter_GObj* object){return input(6,object);}
bool ftCo_800D705C(Fighter_GObj* object){return input(7,object);}
bool ftCo_AerialAction_CheckManual(Fighter_GObj* object){return input(8,object);}
bool ftCo_AerialAction_CheckAutomatic(Fighter_GObj* object){return input(10,object);}
bool ftCo_800CEE70(Fighter_GObj* object){return input(11,object);}
float ftCo_CalcYScaledKnockback(float value,float scale_y,float factor){return value*scale_y*factor;}
bool mpGetSpeed(int index,Vec3* ecb,Vec3* speed){(void)index;(void)ecb;*speed=(Vec3){0};return false;}
void ftCo_800C1E64(Fighter_GObj* object,int motion,int timer,int exponent,float facing)
{(void)motion;(void)timer;(void)exponent;(void)facing;++jump_changes;if(object==&jump_object && jump_retire_at==30)jumpFixture_Retire(3);if(object==&jump_object && jump_retire_at==31)jumpFixture_Retire(1);}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint){joint->flags|=JOBJ_MTX_DIRTY;}
bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* generation){(void)object;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t generation){(void)object;(void)generation;return NULL;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return NULL;}
void __assert(char* file,u32 line,char* text){fprintf(stderr,"assert %s:%lu %s\n",file,line,text);abort();}
#define ABSENT_RW(f) bool f##_Read(NativeScriptRuntimeContext*c,uint32_t s,NativeScriptValue*r,NativeScriptDiagnostic*d){(void)c;(void)s;(void)r;(void)d;return false;} \
bool f##_Write(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*v,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)d;return false;}
ABSENT_RW(nativeScriptBindingKinetics)
ABSENT_RW(nativeScriptBindingCombat)
bool nativeScriptBindingState_Read(NativeScriptRuntimeContext*c,uint32_t s,NativeScriptValue*r,NativeScriptDiagnostic*d){(void)c;(void)s;(void)r;(void)d;return false;}
#define ABSENT_CALL(f) bool f##_Invoke(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*v,uint32_t n,NativeScriptValue*r,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
ABSENT_CALL(nativeScriptBindingKinetics) ABSENT_CALL(nativeScriptBindingCombat) ABSENT_CALL(nativeScriptBindingState) ABSENT_CALL(nativeScriptBindingSetup) ABSENT_CALL(nativeScriptBindingTransition)
bool nativeScriptBindingSetup_Read(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*v,uint32_t n,NativeScriptValue*r,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
bool nativeScriptBindingSetup_Write(NativeScriptRuntimeContext*c,uint32_t s,const NativeScriptValue*v,uint32_t n,const NativeScriptValue*r,NativeScriptDiagnostic*d){(void)c;(void)s;(void)v;(void)n;(void)r;(void)d;return false;}
#include "jump_actual.h"
