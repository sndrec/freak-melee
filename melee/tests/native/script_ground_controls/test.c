#include <melee/ft/kinds/ftCommon/ftCo_AttackS4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackHi4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackLw4.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackS3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackHi3.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackLw3.h>
#include <melee/ft/kinds/ftCommon/ftCo_Jump.h>
#include <melee/ft/kinds/ftCommon/ftCo_Dash.h>
#include <melee/ft/kinds/ftCommon/ftCo_Squat.h>
#include <melee/ft/kinds/ftCommon/ftCo_Turn.h>
#include <melee/ft/kinds/ftCommon/ftCo_Walk.h>
#include <melee/ft/kinds/ftCommon/ftCo_SpecialS.h>
#include <melee/ft/kinds/ftCommon/ftCo_Guard.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/it/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/ftwalkcommon.h>
#include "boundary.h"
#include "attack_boundary.h"
#include <melee/ft/native_script_binding_ground_controls.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_receiver.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/kinds/ftCommon/ftCo_AttackAir.h>
#include <melee/ft/kinds/ftCommon/ftCo_SquatWait.h>
#include <melee/ft/fighter.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=4096,F=128,V=128};
static char source_bytes[65536],source[32768];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define bank package.compiler.bank
#define prepared package.program
static MotionState native_motions[410];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind==Ft_Kind_CustomFirst?&package:NULL;}
static unsigned comparisons,primitive_calls,special_comparisons,special_cases,guard_comparisons,guard_cases,catch_comparisons,taunt_comparisons,input_cases;
static NativeScriptMoveLayout layout={.common_count=341,.first=400,.count=10,.ground={402,404,406,408},.air={403,405,407,409}};
static Fighter fighters[8192];static unsigned allocated,checks,cases;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"Ground controls line%u %s\n",__LINE__,#v);exit(1);}}while(0)
static void append(const char* text){CHECK(strlen(source)+strlen(text)<sizeof(source));strcat(source,text);}
static void initialize_bank(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=8192,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void remove_data(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void set_int(HSD_GObj*,const char*,int);
static HSD_GObj* create(void)
{
 CHECK(allocated<ARRAY_SIZE(fighters));HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 Fighter* fighter=&fighters[allocated++];memset(fighter,0,sizeof(*fighter));fighter->gobj=object;fighter->ft_data=&package.assets.character.data;fighter->kind=Ft_Kind_CustomFirst;fighter->behavior_kind=Ft_Kind_CustomFirst;fighter->x8_spawnNum=allocated;fighter->motion_id=-777;fighter->facing_dir=1;fighter->x34_scale=(Vec3){1,1,1};fighter->co_attrs.jab_2_input_window=13;fighter->co_attrs.jab_3_input_window=17;
 GObj_InitUserData(object,4,remove_data,fighter);CHECK(nativeScriptFighter_Attach(object));set_int(object,"motion",402);return object;
}
static NativeScriptValue state(HSD_GObj* object,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));NativeScriptDiagnostic diagnostic={0};
 for(unsigned i=0;i<view.program->value_count;++i)if(!view.program->values[i].owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){
  NativeScriptValue output;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));
  CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&output,&diagnostic));return output;
 }CHECK(false);return(NativeScriptValue){0};
}
static void set_int(HSD_GObj* object,const char* name,int value){NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=value};state(object,name,&input);}


static uint32_t code_id(const char* name)
{
 for(uint32_t i=0;i<bank.declarations.function_count;++i)
  if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;
 CHECK(false);return UINT32_MAX;
}
static bool invoke(HSD_GObj* object,const char* name)
{
 NativeScriptValue output;NativeScriptDiagnostic diagnostic={0};++primitive_calls;
 return nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,code_id(name),NULL,0,&output,&diagnostic)==NativeScriptVM_Ok;
}

static const char* operations[]={"try_side_smash","try_up_smash","try_down_smash","try_side_tilt","try_up_tilt","try_down_tilt","try_jab","try_ground_jump","try_dash","try_crouch","try_turn","try_walk"};
static const uint32_t semantics[]={NativeScriptBinding_ControlsTrySideSmash,NativeScriptBinding_ControlsTryUpSmash,NativeScriptBinding_ControlsTryDownSmash,NativeScriptBinding_ControlsTrySideTilt,NativeScriptBinding_ControlsTryUpTilt,NativeScriptBinding_ControlsTryDownTilt,NativeScriptBinding_ControlsTryJab,NativeScriptBinding_ControlsTryGroundJump,NativeScriptBinding_ControlsTryDash,NativeScriptBinding_ControlsTryCrouch,NativeScriptBinding_ControlsTryTurn,NativeScriptBinding_ControlsTryWalk};
extern bool ground_item_throw;extern unsigned ground_item_use;
void FrozenAttackS3_decideAngle(HSD_GObj*,float);void GroundAttackS3_decideAngle(HSD_GObj*,float);
void FrozenAttackHi3_doEnter(HSD_GObj*);void GroundAttackHi3_doEnter(HSD_GObj*);
void FrozenAttackHi4_doEnter(HSD_GObj*);void GroundAttackHi4_doEnter(HSD_GObj*);
void FrozenAttackLw4_doEnter(HSD_GObj*);void GroundAttackLw4_doEnter(HSD_GObj*);
void FrozenDash_ftCo_Dash_Enter(HSD_GObj*,int);void FrozenSquat_ftCo_Squat_Enter(HSD_GObj*);void GroundSquat_ftCo_Squat_Enter(HSD_GObj*);
void FrozenTurn_ftCo_Turn_Enter(HSD_GObj*,FtMotionId,MotionFlags,float,float,float);void FrozenTurn_ftCo_Turn_Enter_Smash(HSD_GObj*);
void FrozenWalk_Enter(HSD_GObj*,FtMotionId,MotionFlags,float,float,float,float,float,float,float,float);
#include "compile_audit_cases.h"

static void prepare(bool specials)
{
 initialize_bank();source[0]=0;
 append("fighter:\n    behavior independent\nstate int motion\nstate int tail\nstate bool result\nstate float lag\nstate int entered\nstate int hook_tail\naction FaultDriver:\n    on tick:\n        position.x = 999\n");
 char line[512];for(unsigned i=0;i<12;++i){snprintf(line,sizeof(line),"func Ground%u():\n    state.result = controls.%s()\n    state.tail = state.tail + 1\n",i,operations[i]);append(line);}
 append("func Ordered():\n    if controls.try_side_smash(): return\n    if controls.try_up_smash(): return\n    if controls.try_down_smash(): return\n    if controls.try_side_tilt(): return\n    if controls.try_up_tilt(): return\n    if controls.try_down_tilt(): return\n    if controls.try_jab(): return\n    if controls.try_ground_jump(): return\n    if controls.try_dash(): return\n    if controls.try_crouch(): return\n    if controls.try_turn(): return\n    controls.try_walk()\nfunc Crouch():\n    state.crouch_wait()\n    state.tail = state.tail + 1\nfunc NeutralLag():\n    state.lag = stats.landing_air_neutral_lag\nfunc BackLag():\n    state.lag = stats.landing_air_back_lag\n");
 const char* names[]={"try_side_special","try_up_special","try_neutral_special","try_down_special"};
 for(unsigned i=0;i<4;++i){snprintf(line,sizeof(line),"func Special%u():\n    state.result = controls.%s()\n    state.tail = state.tail + 1\n",i,names[i]);append(line);}
 append("func Catch():\n    state.result = controls.try_catch()\n    state.tail = state.tail + 1\nfunc Taunt():\n    state.result = controls.try_taunt()\n    state.tail = state.tail + 1\n");
 append("func Guard():\n    state.result = controls.try_guard()\n    state.tail = state.tail + 1\nfunc GuardPriority():\n    if controls.try_guard(): return\n    state.tail = state.tail + 1\n");
 append("func OrderedSpecial():\n    if controls.try_side_special(): return\n    if controls.try_up_special(): return\n    if controls.try_neutral_special(): return\n    if controls.try_down_special(): return\n    state.tail = state.tail + 1\n");
 if(specials)append("move Side:\n    entry explicit\n    on special_s:\n        state.entered = 1\n        position.x = 201\n        state.hook_tail = state.hook_tail + 1\nmove Up:\n    entry explicit\n    on special_hi:\n        state.entered = 2\n        position.x = 202\n        state.hook_tail = state.hook_tail + 1\nmove Neutral:\n    entry explicit\n    on special_n:\n        state.entered = 3\n        position.x = 203\n        state.hook_tail = state.hook_tail + 1\nmove Down:\n    entry explicit\n    on special_lw:\n        state.entered = 4\n        position.x = 204\n        state.hook_tail = state.hook_tail + 1\n");
 if(specials)append("action TauntPolicy:\n    on taunt:\n        state.entered = 5\n        position.x = 205\n        state.hook_tail = state.hook_tail + 1\n");
 append("func HeavyThrow():\n    state.result = controls.try_heavy_item_throw()\n    state.tail = state.tail + 1\n");
 if(specials)append("action HeavyThrowPolicy:\n    on heavy_throw_enter:\n        state.entered = 6\n        position.x = 222\n        state.hook_tail = state.hook_tail + 1\n");
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&prepared,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 package.address=&package;package.target=Ft_Kind_CustomFirst;package.ready=package.assets.ready=true;package.assets.facts.moves=layout;package.assets.motions=native_motions;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=native_motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&prepared);
}
static void input(Fighter* f,unsigned sample)
{
 memset(&f->input,0,sizeof(f->input));f->facing_dir=sample&1?-1:1;f->gr_vel=.15f;f->hitlag_mul=0;f->item_gobj=NULL;f->x197C=NULL;f->is_metal=false;
 const float axis[]={-1,-.3f,0,.3f,1};f->input.lstick[0].x=axis[sample%5];f->input.lstick[0].y=axis[(sample/5)%5];
 f->input.pressed_buttons=sample&2?HSD_PAD_A:0;if(sample%7==0)f->input.pressed_buttons|=HSD_PAD_X;
 if(sample&4){f->input.cstick[0].x=f->input.lstick[0].x;f->input.cstick[0].y=f->input.lstick[0].y;}
 if(sample&8)f->input.held_buttons[0]|=HSD_PAD_PROFILE_CSTICK_TILT;
 if(sample&16)f->input.held_buttons[0]|=HSD_PAD_PROFILE_TAP_JUMP_DISABLED;
 f->x670_timer_lstick_tilt_x=f->x671_timer_lstick_tilt_y=sample&32?200:0;
 f->co_attrs.dash_initial_velocity=2;f->co_attrs.standing_turn_frames=3;f->co_attrs.walk_max_vel=2;f->co_attrs.slow_walk_max=1;f->co_attrs.mid_walk_point=2;f->co_attrs.fast_walk_min=3;
 p_ftCommonData->dash_smash_stick_threshold=.8f;p_ftCommonData->dash_smash_window=4;p_ftCommonData->xCC=.8f;p_ftCommonData->xD0=4;p_ftCommonData->xD4=-.8f;p_ftCommonData->xD8=4;
 p_ftCommonData->x98=.2f;p_ftCommonData->attackhi3_stick_threshold_y=.2f;p_ftCommonData->xB0=-.2f;p_ftCommonData->x90=.5f;p_ftCommonData->x34=-.2f;p_ftCommonData->walk_stick_threshold=.2f;
 p_ftCommonData->tap_jump_threshold=.8f;p_ftCommonData->tap_jump_window=4;p_ftCommonData->walk_fast_stick_threshold=.8f;p_ftCommonData->walk_middle_animation_stick_threshold=.4f;
}
static bool native_input(unsigned op,HSD_GObj* object)
{
 switch(op){case 0:return ftCo_AttackS4_CheckInput(object);case 1:return ftCo_AttackHi4_CheckInput(object);case 2:return ftCo_AttackLw4_CheckInput(object);case 3:return ftCo_AttackS3_CheckInput(object);case 4:return ftCo_AttackHi3_CheckInput(object);case 5:return ftCo_AttackLw3_CheckInput(object);case 6:return ftCo_Attack1_CheckInput(object);case 7:return ftCo_Jump_CheckInput(object);case 8:return ftCo_Dash_CheckInput(object);case 9:return ftCo_800D5FB0(object);case 10:return ftCo_Turn_CheckInput(object);case 11:return ftCo_Walk_CheckInput(object);default:abort();}
}
static void native_pairs(void)
{
 for(unsigned op=0;op<12;++op)for(unsigned sample=0;sample<64;++sample){
  HSD_GObj* object=create();Fighter* f=object->user_data;attack_reset();input(f,sample);Fighter initial=*f;
  bool result=native_input(op,object);Fighter expected=*f;unsigned count=attack_count,trace[64];memcpy(trace,attack_trace,sizeof(trace));
  *f=initial;attack_reset();input(f,sample);char name[32];snprintf(name,sizeof(name),"Ground%u",op);CHECK(invoke(object,name));CHECK(state(object,"result",NULL).as.u64==result);CHECK(state(object,"tail",NULL).as.i64==1);CHECK(attack_count==count&&!memcmp(attack_trace,trace,count*sizeof(unsigned)));CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(!attack_stock);++comparisons;HSD_GObjFree(object);
 }
 HSD_GObj* object=create();Fighter* f=object->user_data;attack_reset();input(f,2);f->input.lstick[0]=(Vec2){1,1};CHECK(invoke(object,"Ordered"));CHECK(attack_last_motion==ftCo_MS_AttackS4Hi);CHECK(attack_count==2);++cases;HSD_GObjFree(object);
}
static void entry(unsigned op,HSD_GObj* object,bool frozen)
{
 switch(op){case 0:if(frozen)FrozenAttackS3_decideAngle(object,.2f);else GroundAttackS3_decideAngle(object,.2f);break;
 case 1:if(frozen)FrozenAttackHi3_doEnter(object);else GroundAttackHi3_doEnter(object);break;
 case 2:if(frozen)FrozenAttackHi4_doEnter(object);else GroundAttackHi4_doEnter(object);break;
 case 3:if(frozen)FrozenAttackLw4_doEnter(object);else GroundAttackLw4_doEnter(object);break;
 case 4:if(frozen)FrozenDash_ftCo_Dash_Enter(object,1);else ftCo_Dash_Enter(object,1);break;
 case 5:if(frozen)FrozenSquat_ftCo_Squat_Enter(object);else GroundSquat_ftCo_Squat_Enter(object);break;
 case 6:if(frozen)FrozenTurn_ftCo_Turn_Enter(object,ftCo_MS_Turn,0,.3f,2,.4f);else ftCo_Turn_Enter(object,ftCo_MS_Turn,0,.3f,2,.4f);break;
 case 7:if(frozen)FrozenTurn_ftCo_Turn_Enter_Smash(object);else ftCo_Turn_Enter_Smash(object);break;
 case 8:if(frozen)FrozenWalk_Enter(object,ftCo_MS_WalkSlow,0,.3f,1,2,3,4,5,6,1);else ftWalkCommon_800DFCA4(object,ftCo_MS_WalkSlow,0,.3f,1,2,3,4,5,6,1);break;default:abort();}
}
static void entry_comparisons(void)
{
 for(unsigned op=0;op<9;++op){HSD_GObj* object=create();Fighter* f=object->user_data;attack_reset();input(f,3);Fighter initial=*f;entry(op,object,true);Fighter expected=*f;unsigned count=attack_count,trace[64];memcpy(trace,attack_trace,sizeof(trace));*f=initial;attack_reset();input(f,3);entry(op,object,false);CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(count==attack_count&&!memcmp(trace,attack_trace,count*sizeof(unsigned)));++comparisons;HSD_GObjFree(object);}
}
static void retirements(void)
{
 for(unsigned op=0;op<9;++op)for(unsigned stage=Attack_Motion;stage<=Attack_Animation;++stage)for(unsigned failure=1;failure<=4;++failure){
  HSD_GObj* object=create();Fighter* f=object->user_data;attack_reset();input(f,3);f->mv.co.squat.x0=99;f->mv.co.walk.fast_anim_rate=91;Fighter initial=*f;attack_stop=stage;attack_failure=failure;entry(op,object,false);
  CHECK(attack_count&&attack_trace[attack_count-1]==stage);CHECK(!attack_stock);
  if(op==4){CHECK(f->x670_timer_lstick_tilt_x==initial.x670_timer_lstick_tilt_x);CHECK(f->gr_vel==initial.gr_vel);}
  if(op==5)CHECK(f->mv.co.squat.x0==initial.mv.co.squat.x0);
  if(op==8)CHECK(f->mv.co.walk.fast_anim_rate==initial.mv.co.walk.fast_anim_rate);
  if(failure==4){object->user_data=f;HSD_GObjFree(object);}else if(failure!=1)HSD_GObjFree(object);++cases;
 }

 for(unsigned failure=1;failure<=2;++failure){HSD_GObj* object=create();attack_reset();input(object->user_data,2);attack_stop=Attack_Pickup;attack_failure=failure;entry(0,object,false);CHECK(attack_count==1&&attack_trace[0]==Attack_Pickup);if(failure==2)HSD_GObjFree(object);++cases;}
 for(unsigned op=0;op<12;++op){
  HSD_GObj* object=create();Fighter* f=object->user_data;attack_reset();input(f,2);
  f->input.lstick[0].x=op==10?-1:1;f->input.lstick[0].y=op==2||op==5||op==9?-1:1;
  if(op==3)f->input.lstick[0].y=0;if(op==4||op==5)f->input.lstick[0].x=.1f;
  attack_stop=Attack_Motion;attack_failure=2;char name[32];snprintf(name,sizeof(name),"Ground%u",op);
  CHECK(!invoke(object,name)&&attack_stop==0&&nativeScriptFighter_Status(object)==NativeScriptVM_Error&&state(object,"tail",NULL).as.i64==0);++cases;HSD_GObjFree(object);
  object=create();f=object->user_data;attack_reset();input(f,2);f->input.lstick[0].x=op==10?-1:1;f->input.lstick[0].y=op==2||op==5||op==9?-1:1;
  if(op==3)f->input.lstick[0].y=0;if(op==4||op==5)f->input.lstick[0].x=.1f;
  NativeScriptRuntimeContext ctx={.source=object,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptDiagnostic d={0};NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=UINT64_C(0xfeedface12345678)},before=out;
  attack_stop=Attack_Motion;attack_failure=2;CHECK(!nativeScriptBindingGroundControls_Invoke(&ctx,semantics[op],NULL,0,&out,&d)&&attack_stop==0&&!memcmp(&out,&before,sizeof(out)));++cases;HSD_GObjFree(object);
 }

}
static void observations(void)
{
 HSD_GObj* object=create();Fighter* f=object->user_data;const uint32_t bits[]={0,0x80000000,0x3fc00000,0x7fc12345,0x7f800000};
 for(unsigned i=0;i<ARRAY_SIZE(bits);++i){memcpy(&f->co_attrs.landingairn_lag,&bits[i],4);memcpy(&f->co_attrs.landingairb_lag,&bits[i],4);CHECK(invoke(object,"NeutralLag"));NativeScriptValue value=state(object,"lag",NULL);CHECK(!memcmp(&value.as.f32,&bits[i],4));CHECK(invoke(object,"BackLag"));value=state(object,"lag",NULL);CHECK(!memcmp(&value.as.f32,&bits[i],4));++cases;}
 attack_reset();CHECK(invoke(object,"Crouch"));CHECK(attack_last_motion==ftCo_MS_SquatWait&&state(object,"tail",NULL).as.i64==1);++cases;
 NativeScriptRuntimeContext ctx={.source=object,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptDiagnostic d={0};NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=0xabcdef},before=out;
 CHECK(!nativeScriptBindingGroundControls_Invoke(&ctx,NativeScriptBinding_ControlsTryDash,&out,1,&out,&d)&&!memcmp(&out,&before,sizeof(out)));
 CHECK(!nativeScriptBindingGroundControls_Read(&ctx,NativeScriptBinding_StatsLandingAirNeutralLag,&out,1,&out,&d)&&!memcmp(&out,&before,sizeof(out)));

 ftCommonData* saved=p_ftCommonData;p_ftCommonData=NULL;CHECK(!nativeScriptBindingGroundControls_Invoke(&ctx,NativeScriptBinding_ControlsTryDash,NULL,0,&out,&d)&&!memcmp(&out,&before,sizeof(out)));p_ftCommonData=saved;++cases;
 nativeScriptFighter_Detach(object);f->kind=f->behavior_kind=Ft_Kind_Mario;f->co_attrs.landingairn_lag=19.25f;CHECK(nativeScriptBindingGroundControls_Read(&ctx,NativeScriptBinding_StatsLandingAirNeutralLag,NULL,0,&out,&d)&&out.type==NativeScriptType_F32&&out.as.f32==19.25f);++cases;
 out=before;CHECK(!nativeScriptBindingGroundControls_Invoke(&ctx,NativeScriptBinding_ControlsTryDash,NULL,0,&out,&d)&&!memcmp(&out,&before,sizeof(out)));++cases;
 ++ctx.generation;CHECK(!nativeScriptBindingGroundControls_Invoke(&ctx,NativeScriptBinding_ControlsTryDash,NULL,0,&out,&d)&&!memcmp(&out,&before,sizeof(out)));++cases;HSD_GObjFree(object);
}

static const uint32_t special_ids[]={NativeScriptBinding_ControlsTrySideSpecial,NativeScriptBinding_ControlsTryUpSpecial,NativeScriptBinding_ControlsTryNeutralSpecial,NativeScriptBinding_ControlsTryDownSpecial};
static bool special_native(unsigned op,HSD_GObj* o)
{switch(op){case 0:return ftCo_SpecialS_CheckInput(o);case 1:return ftCo_Attack100_CheckInput(o);case 2:return ftCo_800D6824(o);case 3:return ftCo_800D68C0(o);default:abort();}}
static void special_input(Fighter* f,unsigned age,float x,bool floor,bool ignore,float facing)
{
 attack_reset();ground_special_reset();f->facing_dir=facing;f->gr_vel=3.125f;f->co_attrs.specials_ground_speed_retention=.75f;
 f->input.pressed_buttons=0;f->input.lstick[0]=(Vec2){x,0};f->x688=f->x686=f->x689=f->x687=age;
 f->coll_data.floor.index=floor?3:-1;f->coll_data.floor.flags=7;package.assets.facts.ignore_surface_friction=ignore;
 p_ftCommonData->x220=.5f;
}
static void special_pairs(void)
{
 const unsigned ages[]={0,1,255};const float sticks[]={-1,-.5f,-.499f,-0.0f,.5f,1,NAN};
 for(unsigned op=0;op<4;++op)for(unsigned age=0;age<3;++age)for(unsigned x=0;x<ARRAY_SIZE(sticks);++x)for(unsigned mode=0;mode<8;++mode){
  HSD_GObj* o=create();Fighter* f=o->user_data;special_input(f,ages[age],sticks[x],mode&1,mode&2,mode&4?-1:1);Fighter initial=*f;
  bool result=special_native(op,o);Fighter expected=*f;unsigned count=attack_count,trace[64],friction=ground_special_friction_reads;memcpy(trace,attack_trace,sizeof(trace));
  CHECK(result==(ages[age]==0));CHECK(state(o,"entered",NULL).as.i64==(result?(int)op+1:0));
  *f=initial;set_int(o,"entered",0);set_int(o,"hook_tail",0);special_input(f,ages[age],sticks[x],mode&1,mode&2,mode&4?-1:1);
  char name[32];snprintf(name,sizeof(name),"Special%u",op);CHECK(invoke(o,name));CHECK(state(o,"result",NULL).type==NativeScriptType_Bool&&state(o,"result",NULL).as.u64==result);
  CHECK(state(o,"entered",NULL).as.i64==(result?(int)op+1:0));CHECK(state(o,"hook_tail",NULL).as.i64==(result?1:0));CHECK(state(o,"tail",NULL).as.i64==1);
  CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(count==attack_count&&!memcmp(trace,attack_trace,count*sizeof(unsigned))&&friction==ground_special_friction_reads);CHECK(!attack_stock);
  ++comparisons;++special_comparisons;HSD_GObjFree(o);
 }
 for(unsigned selected=0;selected<5;++selected){HSD_GObj*o=create();Fighter*f=o->user_data;special_input(f,1,1,true,false,1);
  if(selected==0)f->x688=0;else if(selected==1)f->x686=0;else if(selected==2)f->x689=0;else if(selected==3)f->x687=0;
  CHECK(invoke(o,"OrderedSpecial"));CHECK(state(o,"entered",NULL).as.i64==(selected==4?0:(int)selected+1));CHECK(attack_count==(selected==4?0u:1u));CHECK(state(o,"tail",NULL).as.i64==(selected==4?1:0));CHECK(!attack_stock);++cases;++special_cases;HSD_GObjFree(o);
 }
}
static void special_retirements(void)
{
 // Actual authored entry field write calls a controlled native leaf. The
 // existing leaf executes physical removal, a real nested VM fault, native
 // generation replacement or userdata retirement inside this synchronous hook.
 for(unsigned op=0;op<4;++op)for(unsigned failure=1;failure<=4;++failure)for(unsigned vm=0;vm<2;++vm){
  HSD_GObj*o=create();Fighter*f=o->user_data;special_input(f,0,1,true,false,1);NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,o,&reference));
  NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=0x123456789abcdef},before=out;NativeScriptDiagnostic d={0};
  attack_stop=201+op;attack_failure=failure;
  if(vm){char name[32];snprintf(name,sizeof(name),"Special%u",op);CHECK(!invoke(o,name));}
  else CHECK(!nativeScriptBindingGroundControls_Invoke(&c,special_ids[op],NULL,0,&out,&d)&&!memcmp(&out,&before,sizeof(out)));
  CHECK(attack_count==1&&attack_trace[0]==201+op&&!attack_stock);
  if(failure==4)o->user_data=f;
  if(nativeMatchPool_Resolve(reference)){CHECK(state(o,"tail",NULL).as.i64==0);HSD_GObjFree(o);}
  else CHECK(failure==1);
  ++cases;++special_cases;
 }
}
static void special_admission(void)
{
 HSD_GObj*o=create();Fighter*f=o->user_data;special_input(f,0,1,true,false,1);NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));NativeScriptStorageInstance saved=*view.storage;
 for(unsigned mode=0;mode<5;++mode){NativeScriptRuntimeContext bad=c;uint64_t epoch=nativeGObjPool.lifetime;
  switch(mode){case 0:++bad.scene_epoch;break;case 1:++bad.generation;break;case 2:bad.source_class=NativeCallbackSource_Item;break;case 3:view.storage->live=false;break;case 4:package.ready=false;break;}
  for(unsigned op=0;op<4;++op){Fighter before=*f;NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;NativeScriptDiagnostic d;
   CHECK(!nativeScriptBindingGroundControls_Invoke(&bad,special_ids[op],NULL,0,&out,&d)&&!memcmp(&out,&sentinel,sizeof(out))&&!memcmp(f,&before,sizeof(*f)));++cases;++special_cases;
  }
  *view.storage=saved;nativeGObjPool.lifetime=epoch;package.ready=true;
 }
 HSD_GObjFree(o);
}
static void special_missing_hooks(void)
{
 prepare(false);
 for(unsigned op=0;op<4;++op){HSD_GObj*o=create();Fighter*f=o->user_data;special_input(f,0,-1,true,false,1);
  CHECK(!nativeScriptFighter_HasHook(o,NativeScriptFighter_SpecialS)&&!nativeScriptFighter_HasHook(o,NativeScriptFighter_SpecialHi)&&!nativeScriptFighter_HasHook(o,NativeScriptFighter_SpecialN)&&!nativeScriptFighter_HasHook(o,NativeScriptFighter_SpecialLw));
  // NULL stock slot plus attached source is enough for the existing kernels.
  ftData_SpecialS[f->kind]=ftData_SpecialHi[f->kind]=ftData_SpecialN[f->kind]=ftData_SpecialLw[f->kind]=NULL;
  Fighter initial=*f;CHECK(special_native(op,o));Fighter expected=*f;*f=initial;
  char name[32];snprintf(name,sizeof(name),"Special%u",op);CHECK(invoke(o,name));CHECK(state(o,"result",NULL).as.u64==1&&state(o,"tail",NULL).as.i64==1&&state(o,"entered",NULL).as.i64==0);
  CHECK(!memcmp(f,&expected,sizeof(*f))&&attack_count==0&&!attack_stock);if(!op)CHECK(f->facing_dir==-1&&f->gr_vel<initial.gr_vel);++cases;++special_cases;++comparisons;++special_comparisons;HSD_GObjFree(o);
 }
}
static void guard_pairs(void)
{
 const unsigned pressed[]={0,HSD_PAD_L,HSD_PAD_R,HSD_PAD_L|HSD_PAD_R,HSD_PAD_B};
 const unsigned held[]={0,HSD_PAD_LR,HSD_PAD_L,HSD_PAD_R};
 const unsigned timers[]={0,1,2,3,254,255};const int windows[]={-1,0,1,3,256};
 const float health[]={0,-0.0f,1,-1,NAN};
 HSD_GObj*o=create();Fighter*f=o->user_data;
 for(unsigned p=0;p<ARRAY_SIZE(pressed);++p)for(unsigned h=0;h<ARRAY_SIZE(held);++h)
 for(unsigned t=0;t<ARRAY_SIZE(timers);++t)for(unsigned w=0;w<ARRAY_SIZE(windows);++w)for(unsigned s=0;s<ARRAY_SIZE(health);++s){
  set_int(o,"tail",0);attack_reset();f->motion_id=402;f->input.pressed_buttons=pressed[p];f->input.held_buttons[0]=held[h];
  f->trigger_analog_timer=timers[t];f->shield_health=health[s];p_ftCommonData->powershield_input_window=windows[w];Fighter initial=*f;
  bool result=ftCo_80091A4C(o);Fighter expected=*f;unsigned count=attack_count,trace[64];memcpy(trace,attack_trace,sizeof(trace));
  int motion=attack_last_motion;CHECK(count==(result?1u:0u));CHECK(!result||motion==ftCo_MS_GuardOn||motion==ftCo_MS_GuardReflect);
  *f=initial;attack_reset();CHECK(invoke(o,"Guard"));CHECK(state(o,"result",NULL).type==NativeScriptType_Bool&&state(o,"result",NULL).as.u64==result);
  CHECK(state(o,"tail",NULL).as.i64==1);CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(count==attack_count&&!memcmp(trace,attack_trace,count*sizeof(unsigned))&&motion==attack_last_motion);CHECK(!attack_stock);
  ++comparisons;++guard_comparisons;
 }
 // Reflect has priority over held guard, including depleted health. Exact
 // timer boundary falls through to common start, and plain L/R held bits
 // alone are not the native derived HSD_PAD_LR input bit.
 for(unsigned sample=0;sample<4;++sample){set_int(o,"tail",0);attack_reset();f->input.pressed_buttons=sample<2?HSD_PAD_L:0;f->input.held_buttons[0]=sample==3?HSD_PAD_L:HSD_PAD_LR;
  f->trigger_analog_timer=sample==1?3:2;f->shield_health=sample==2?0:1;p_ftCommonData->powershield_input_window=3;
  CHECK(invoke(o,"GuardPriority"));CHECK(attack_last_motion==(sample==0?ftCo_MS_GuardReflect:sample==1?ftCo_MS_GuardOn:-1));CHECK(state(o,"tail",NULL).as.i64==(sample<2?0:1));++cases;++guard_cases;
 }
 HSD_GObjFree(o);
}
static void guard_admission(void)
{
 HSD_GObj*o=create();Fighter*f=o->user_data;attack_reset();f->input.held_buttons[0]=HSD_PAD_LR;f->shield_health=1;
 NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));NativeScriptStorageInstance saved=*view.storage;
 for(unsigned sample=0;sample<6;++sample){NativeScriptRuntimeContext bad=c;ftCommonData* common=p_ftCommonData;
  switch(sample){case 0:++bad.scene_epoch;break;case 1:++bad.generation;break;case 2:bad.source_class=NativeCallbackSource_Item;break;case 3:view.storage->live=false;break;case 4:package.ready=false;break;case 5:p_ftCommonData=NULL;break;}
  Fighter initial=*f;NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=99},out=sentinel;NativeScriptDiagnostic d={0};
  CHECK(!nativeScriptBindingGroundControls_Invoke(&bad,NativeScriptBinding_ControlsTryGuard,NULL,0,&out,&d)&&!memcmp(&out,&sentinel,sizeof(out))&&!memcmp(f,&initial,sizeof(*f))&&!attack_count);
  *view.storage=saved;package.ready=true;p_ftCommonData=common;++cases;++guard_cases;
 }
 HSD_GObjFree(o);
}
static void catch_pairs(void)
{
 const unsigned pressed[]={0,HSD_PAD_A,HSD_PAD_B,HSD_PAD_A|HSD_PAD_B};const unsigned held[]={0,HSD_PAD_LR,HSD_PAD_L,HSD_PAD_R};
 Item item={0};HSD_GObj held_item={.user_data=&item};HSD_GObj*o=create();Fighter*f=o->user_data;
 for(unsigned p=0;p<ARRAY_SIZE(pressed);++p)for(unsigned h=0;h<ARRAY_SIZE(held);++h)for(unsigned mode=0;mode<4;++mode){
  attack_reset();set_int(o,"tail",0);f->input.pressed_buttons=pressed[p];f->input.held_buttons[0]=held[h];f->item_gobj=mode&1?&held_item:NULL;ground_item_throw=mode&2;
  Fighter initial=*f;bool expected=ftCo_Catch_CheckInput(o);Fighter after=*f;unsigned trace[64],count=attack_count;memcpy(trace,attack_trace,sizeof(trace));
  *f=initial;attack_reset();CHECK(invoke(o,"Catch"));CHECK(state(o,"result",NULL).as.u64==expected&&state(o,"tail",NULL).as.i64==1);CHECK(!memcmp(f,&after,sizeof(*f))&&count==attack_count&&!memcmp(trace,attack_trace,count*sizeof(unsigned)));CHECK(!attack_stock);
  if((mode&3)==3)CHECK(expected&&count==1&&trace[0]==220);++comparisons;++catch_comparisons;
 }
 f->item_gobj=NULL;ground_item_throw=false;HSD_GObjFree(o);
}
static void taunt_pairs(bool hook)
{
 const unsigned pressed[]={0,HSD_PAD_DPADUP,HSD_PAD_DPADLEFT,HSD_PAD_A,HSD_PAD_DPADUP|HSD_PAD_A};HSD_GObj*o=create();Fighter*f=o->user_data;
 for(unsigned p=0;p<ARRAY_SIZE(pressed);++p)for(unsigned left=0;left<2;++left){
  attack_reset();ground_taunt_bonus=0;set_int(o,"tail",0);set_int(o,"entered",0);set_int(o,"hook_tail",0);f->input.pressed_buttons=pressed[p];f->facing_dir=left?-1:1;Fighter initial=*f;
  bool expected=ftCo_800DE9D8(o);Fighter after=*f;unsigned trace[64],count=attack_count,bonus=ground_taunt_bonus;memcpy(trace,attack_trace,sizeof(trace));int entered=state(o,"entered",NULL).as.i64;
  *f=initial;attack_reset();ground_taunt_bonus=0;set_int(o,"entered",0);set_int(o,"hook_tail",0);CHECK(invoke(o,"Taunt"));CHECK(state(o,"result",NULL).as.u64==expected&&state(o,"tail",NULL).as.i64==1&&state(o,"entered",NULL).as.i64==entered);
  CHECK(!memcmp(f,&after,sizeof(*f))&&count==attack_count&&!memcmp(trace,attack_trace,count*sizeof(unsigned))&&bonus==ground_taunt_bonus);CHECK(bonus==(expected?1u:0u));CHECK(!expected||entered==(hook?5:0));CHECK(!attack_stock);++comparisons;++taunt_comparisons;
 }
 HSD_GObjFree(o);
}
static void input_retirements(void)
{
 for(unsigned failure=1;failure<=4;++failure)for(unsigned vm=0;vm<2;++vm){
  HSD_GObj*o=create();Fighter*f=o->user_data;attack_reset();ground_taunt_bonus=0;f->input.pressed_buttons=HSD_PAD_DPADUP;NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,o,&reference));
  NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},before=out;NativeScriptDiagnostic d={0};attack_stop=205;attack_failure=failure;
  if(vm)CHECK(!invoke(o,"Taunt"));else CHECK(!nativeScriptBindingGroundControls_Invoke(&c,NativeScriptBinding_ControlsTryTaunt,NULL,0,&out,&d)&&!memcmp(&out,&before,sizeof(out)));
  CHECK(!ground_taunt_bonus&&attack_count==1&&attack_trace[0]==205);if(failure==4)o->user_data=f;
  if(nativeMatchPool_Resolve(reference)){CHECK(state(o,"tail",NULL).as.i64==0);HSD_GObjFree(o);}else CHECK(failure==1);++cases;++input_cases;
 }
 HSD_GObj*o=create();Fighter*f=o->user_data;NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));NativeScriptStorageInstance saved=*view.storage;const uint32_t ids[]={NativeScriptBinding_ControlsTryCatch,NativeScriptBinding_ControlsTryTaunt};
 for(unsigned mode=0;mode<5;++mode){NativeScriptRuntimeContext bad=c;switch(mode){case 0:++bad.scene_epoch;break;case 1:++bad.generation;break;case 2:bad.source_class=NativeCallbackSource_Item;break;case 3:view.storage->live=false;break;case 4:package.ready=false;break;}
  for(unsigned op=0;op<2;++op){Fighter initial=*f;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},before=out;NativeScriptDiagnostic d={0};CHECK(!nativeScriptBindingGroundControls_Invoke(&bad,ids[op],NULL,0,&out,&d)&&!memcmp(&out,&before,sizeof(out))&&!memcmp(f,&initial,sizeof(*f)));++cases;++input_cases;}
  *view.storage=saved;package.ready=true;
 }
 HSD_GObjFree(o);
}
#include "heavy_item_cases.h"
int main(void)
{
 compile_audit_cases();prepare(true);HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
 native_pairs();entry_comparisons();retirements();observations();special_pairs();special_retirements();special_admission();guard_pairs();guard_admission();catch_pairs();taunt_pairs(true);input_retirements();special_missing_hooks();taunt_pairs(false);heavy_pairs();heavy_hooks();CHECK(nativeScriptRuntime_Idle()&&!nativeScriptFighter_ReferencesProgram(&bank));
 printf("{\"status\":\"PASS connected ground controls\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"source_invocations\":%u,\"special_native_pairs\":%u,\"special_cases\":%u,\"guard_native_pairs\":%u,\"guard_cases\":%u,\"catch_native_pairs\":%u,\"taunt_native_pairs\":%u,\"new_input_cases\":%u,\"heavy_native_pairs\":%u,\"heavy_cases\":%u}\n",checks,cases,comparisons,primitive_calls,special_comparisons,special_cases,guard_comparisons,guard_cases,catch_comparisons,taunt_comparisons,input_cases,heavy_comparisons,heavy_cases);return 0;
}
