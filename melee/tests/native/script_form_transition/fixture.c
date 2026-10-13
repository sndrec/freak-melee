#include "fixture.h"
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftanim.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

NativeSourcePackage package;
NativeScriptFighterProgram selected;
HSD_GObj object;Fighter fighter;
NativeScriptStorageInstance storage;
NativeScriptVMStatus actor_status;
const NativeScriptProgramIndex* view_program;
unsigned change_mode,change_count,start_count,order_count,order[8];ChangeRecord change;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static MotionState owned_motions[8];

// Selected package and actor lookup are controlled publication boundaries.
// The production SourceFighterLayout validates every supplied relationship.
void fixture_select(void)
{
    memset(&object,0,sizeof(object));memset(&fighter,0,sizeof(fighter));memset(heads,0,sizeof(heads));
    memset(&change,0,sizeof(change));change_count=start_count=order_count=change_mode=0;
    object.user_data=&fighter;fighter.gobj=&object;fighter.kind=fighter.behavior_kind=Ft_Kind_CustomFirst;
    fighter.x8_spawnNum=17;fighter.motion_id=341;fighter.cur_anim_frame=19;fighter.frame_speed_mul=3;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&object;
    package.address=&package;package.ready=package.assets.ready=true;package.target=fighter.kind;
    package.assets.facts.moves=package.compiler.bank.motion_layout;
    package.assets.motions=owned_motions;fighter.ft_data=&package.assets.character.data;
    gFtDataList[fighter.kind]=fighter.ft_data;ftData_CharacterStateTables[fighter.kind]=owned_motions;
    package.program.bank=&package.compiler.bank;package.program.identity=UINT64_C(0xcafe1234);
    selected=package.program;actor_status=NativeScriptVM_Ok;view_program=&selected.bank->declarations;
}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==package.target?&package:NULL;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind)
{return kind==package.target?&selected:NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* actor)
{return actor==&object?actor_status:NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* actor,uint32_t generation,NativeScriptReceiverView* view)
{
    if(actor!=&object||!view||!storage.live||storage.generation!=generation)return false;
    *view=(NativeScriptReceiverView){.program=view_program,.storage=&storage};return true;
}
bool nativeModelEffect_Snapshot(HSD_GObj* actor,uint32_t* generation)
{(void)actor;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* actor,uint32_t generation)
{(void)actor;(void)generation;return NULL;}

// Actual seven-argument engine ABI captured at its native boundary. No authored
// decision runs here. Modes represent reentry while installing the motion.
void Fighter_ChangeMotionState(Fighter_GObj* actor,FtMotionId motion,MotionFlags flags,
    float frame,float rate,float blend,Fighter_GObj* target)
{
    if(change_count++||order_count>=8)abort();order[order_count++]=1;
    change=(ChangeRecord){actor,motion,flags,frame,rate,blend,target};
    Fighter* native=actor->user_data;native->motion_id=motion;native->cur_anim_frame=frame;native->frame_speed_mul=rate;
    switch(change_mode){
    case ChangePhysicalRemove:heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;actor->user_data=(void*)1;break;
    case ChangeGeneration:++native->x8_spawnNum;break;
    case ChangeLogicalRetire:storage.live=false;break;
    case ChangeFault:actor_status=NativeScriptVM_Error;break;
    case ChangeDataReplace:native->ft_data=NULL;break;
    default:break;
    }
}
void ftAnim_8006EBA4(Fighter_GObj* actor)
{if(actor!=&object||order_count>=8)abort();++start_count;order[order_count++]=2;}
void ftCommon_8007D5D4(Fighter* native){(void)native;abort();}
void ftCommon_8007D7FC(Fighter* native){(void)native;abort();}

// The isolated fixture has no historical C-definition receiver owners.
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{(void)kind;return NULL;}
