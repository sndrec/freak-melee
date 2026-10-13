#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage movement_package;
#define bank movement_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Fighter fighters[128],alternate;static unsigned count,checks;static NativeScriptDiagnostic diagnostic;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned mutation_point,mutation_mode,write_count,grounded_tail,timer_tail,trace,drop_calls,float_calls,donor_calls,animation_calls;
bool invoke_grounded,dead_fall;HSD_GObj* replacement;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Movement scheduler %u %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
int32_t nativeFighterHeldMotion_Select(Fighter_GObj*,int32_t,int32_t);
bool ftCo_Mushroom_SelectAirResume(Fighter_GObj*,int);
bool ftCo_AerialAction_CheckManual(Fighter_GObj*);bool ftCo_AerialAction_CheckAutomatic(Fighter_GObj*);
void ft_8008A348(Fighter_GObj*,float);void ftCo_ItemParasolOpen_Enter(Fighter_GObj*,float);
void ftCo_800CF3C8(Fighter_GObj*);
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate bool handled\nstate int mapped\nstate int first\nstate int previous\nstate int calls\nstate float blend\nstate int completed\nfunc WaitIntrinsic():\n    state.wait()\n    state.completed = state.completed + 1\nfunc BlendIntrinsic():\n    state.wait_blend(-0.375)\n    state.completed = state.completed + 1\naction Movement:\n    on held_motion:\n        state.first = event.motion\n        state.previous = event.previous_motion\n        position.x = 1\n        return state.mapped\n    on air_resume_motion:\n        state.previous = event.previous_motion\n        position.x = 1\n        return state.mapped\n    on neutral_enter:\n        state.blend = event.blend\n        position.x = 2\n    on wait_enter:\n        position.x = 6\n    on grounded_motion_setup:\n        position.x = 8\n    on held_timer_expired:\n        position.x = 1\n        return state.handled\n    on aerial_action_manual:\n        position.x = 1\n        return state.handled\n    on aerial_action_automatic:\n        position.x = 1\n        return state.handled\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_Peach,&bank,&layout,NULL,&movement_package.program,&diagnostic));movement_package.address=&movement_package;movement_package.target=Ft_Kind_Peach;movement_package.ready=movement_package.assets.ready=true;movement_package.assets.facts.moves=layout;movement_package.assets.facts.joints=4;movement_package.assets.parts.parts_num=4;tables[Ft_Kind_Peach]=&movement_package.assets.parts;gFtDataList[Ft_Kind_Peach]=&movement_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_Peach]=movement_package.assets.motions;nativeScriptFighter_PublishProgram(Ft_Kind_Peach,&movement_package.program);
}
static void removed(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* create(void){CHECK(count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[count++];*f=(Fighter){.gobj=o,.kind=movement_package.target,.behavior_kind=movement_package.target,.x8_spawnNum=10+count,.ft_data=&movement_package.assets.character.data};GObj_InitUserData(o,4,removed,f);CHECK(nativeScriptFighter_Attach(o));return o;}
static NativeScriptValue state(HSD_GObj* o,const char* name,const NativeScriptValue* input){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));for(unsigned i=0;i<bank.declarations.value_count;++i)if(!values[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));return result;}CHECK(false);return(NativeScriptValue){0};}
static void set(HSD_GObj* o,const char* name,int value){NativeScriptValue v={.type=!strcmp(name,"handled")?NativeScriptType_Bool:NativeScriptType_I32,.as.i64=value};state(o,name,&v);}
void movement_mutate(HSD_GObj* o,unsigned point){if(!mutation_mode||(mutation_point&&mutation_point!=point))return;unsigned mode=mutation_mode;mutation_mode=0;if(mode==1)HSD_GObjFree(o);if(mode==2){HSD_GObjFree(o);replacement=create();}if(mode==3){alternate=*(Fighter*)o->user_data;o->user_data=&alternate;}if(mode==4)((Fighter*)o->user_data)->x8_spawnNum++;}
static bool helper(HSD_GObj* o,const char* name){for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name)){NativeScriptValue result={0};return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,i,NULL,0,&result,&diagnostic)==NativeScriptVM_Ok;}CHECK(false);return false;}
int32_t reference_nativeFighterHeldMotion_Select(Fighter_GObj*,int32_t,int32_t);
bool reference_ftCo_Mushroom_SelectAirResume(Fighter_GObj*,int);
void reference_ft_8008A348(Fighter_GObj*,float);
bool reference_ftCo_AerialAction_CheckManual(Fighter_GObj*);
bool reference_ftCo_AerialAction_CheckAutomatic(Fighter_GObj*);
static void reset(HSD_GObj* o){set(o,"handled",0);trace=0;drop_calls=0;donor_calls=0;grounded_tail=timer_tail=0;invoke_grounded=dead_fall=false;mutation_point=0;}
int main(void)
{
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);HSD_GObj* o=create();Fighter* f=o->user_data;
 CHECK(nativeFighterHeldMotion_Select(NULL,1,2)==-1);HSD_GObj invalid={0};CHECK(nativeFighterHeldMotion_Select(&invalid,1,2)==-1);
 reset(o);set(o,"mapped",341);CHECK(nativeFighterHeldMotion_Select(o,ftCo_MS_ItemParasolOpen,123)==341);CHECK(state(o,"first",NULL).as.i64==ftCo_MS_ItemParasolOpen&&state(o,"previous",NULL).as.i64==123);
 int mappings[]={-1,342,INT32_MAX,ftCo_MS_FallAerial,ftCo_MS_FallSpecial,ftCo_MS_ItemParasolFallSpecial};
 for(unsigned i=0;i<ARRAY_SIZE(mappings);++i){set(o,"mapped",mappings[i]);int expected=mappings[i]>=0&&mappings[i]<342?mappings[i]:ftCo_MS_FallAerial;CHECK(nativeFighterHeldMotion_Select(o,ftCo_MS_FallAerial,100)==expected);CHECK(ftCo_Mushroom_SelectAirResume(o,ftCo_MS_FallAerial));CHECK(state(o,"previous",NULL).as.i64==ftCo_MS_FallAerial);CHECK(f->mv.co.mushroom.resume_air==(expected==ftCo_MS_FallAerial?ftCo_FallAerial_Enter:expected==ftCo_MS_FallSpecial||expected==ftCo_MS_ItemParasolFallSpecial?ftCo_800968C8:ftCo_Fall_Enter));}
 reset(o);f->ground_or_air=GA_Air;dead_fall=true;ft_8008A348(o,-0.25f);CHECK(trace==123456&&f->motion_id==ftCo_MS_Wait&&state(o,"blend",NULL).as.f32==-0.25f&&donor_calls==0);
 reset(o);f->ground_or_air=GA_Ground;f->x2221_b5=true;f->x2221_b7=false;movement_grounded(o);CHECK(trace==8&&grounded_tail==1&&!f->x2221_b5&&f->x2221_b7&&f->x2180==6);
 reset(o);f->x2221_b4=true;f->x2104=1;movement_timer(o);CHECK(!f->x2221_b4&&f->x2104==0&&drop_calls==1&&timer_tail==1);set(o,"handled",1);f->x2221_b4=true;f->x2104=1;movement_timer(o);CHECK(drop_calls==1&&timer_tail==2);
 CHECK(ftCo_AerialAction_CheckManual(o)&&ftCo_AerialAction_CheckAutomatic(o));set(o,"handled",0);CHECK(!ftCo_AerialAction_CheckManual(o)&&!ftCo_AerialAction_CheckAutomatic(o)&&float_calls==0);
 reset(o);set(o,"mapped",341);f->ground_or_air=GA_Air;ftCo_ItemParasolOpen_Enter(o,0.75f);CHECK(f->motion_id==341&&animation_calls==1&&!f->x2221_b7);ftCo_800CF3C8(o);CHECK(f->motion_id==341);
 HSD_GObjFree(o);
 for(unsigned hook=0;hook<8;++hook)for(unsigned mode=1;mode<=5;++mode){o=create();f=o->user_data;reset(o);set(o,"mapped",341);f->ground_or_air=GA_Ground;f->x2221_b4=true;f->x2104=1;NativeFighterLifetime saved;CHECK(nativeFighterLifetime_Capture(f,&saved));mutation_mode=mode;
 switch(hook){case 0:CHECK(nativeFighterHeldMotion_Select(o,1,2)==-1);break;case 1:CHECK(!ftCo_Mushroom_SelectAirResume(o,1));break;case 2:ft_8008A348(o,1);CHECK(f->motion_id==0);break;case 3:movement_grounded(o);CHECK(grounded_tail==0&&!f->x2221_b7);break;case 4:movement_timer(o);CHECK(drop_calls==0&&timer_tail==0);break;case 5:CHECK(ftCo_AerialAction_CheckManual(o));break;case 6:CHECK(ftCo_AerialAction_CheckAutomatic(o));break;case 7:ftCo_ItemParasolOpen_Enter(o,0);CHECK(f->motion_id==0);break;}
 CHECK(!nativeFighterLifetime_Current(&saved));if(mode==2){CHECK(replacement==o);HSD_GObjFree(replacement);}else if(mode!=1)HSD_GObjFree(o);}
 // Wait cleanup faults/retirement are terminal after native common tails.
 for(unsigned phase=0;phase<3;++phase)for(unsigned mode=1;mode<=(phase==2?4:5);++mode){o=create();f=o->user_data;reset(o);set(o,"mapped",341);f->ground_or_air=GA_Ground;NativeFighterLifetime saved;CHECK(nativeFighterLifetime_Capture(f,&saved));mutation_mode=mode;mutation_point=phase==0?6:phase==1?8:99;
  if(phase==0){ft_8008A348(o,-0.0f);CHECK(trace==2356&&f->motion_id==ftCo_MS_Wait);}
  else{invoke_grounded=phase==1;f->x2221_b7=true;unsigned animations_before=animation_calls;ftCo_ItemParasolOpen_Enter(o,0.5f);CHECK(f->motion_id==341&&f->x2221_b7);if(phase==1)CHECK(grounded_tail==0&&animation_calls==animations_before);else CHECK(animation_calls==animations_before+1);}
  CHECK(!nativeFighterLifetime_Current(&saved));if(mode==2){CHECK(replacement==o);HSD_GObjFree(replacement);}else if(mode!=1)HSD_GObjFree(o);
 }
 // Missing hooks retain common mapping/timer/arbitration and never donor paths.
 for(unsigned hook=NativeScriptFighter_HeldMotion;hook<=NativeScriptFighter_AerialActionAutomatic;++hook)movement_package.program.hooks[hook]=UINT32_MAX;
 movement_package.program.hooks[NativeScriptFighter_WaitEnter]=UINT32_MAX;CHECK(nativeScriptFighter_CanPublish(Ft_Kind_Peach));nativeScriptFighter_PublishProgram(Ft_Kind_Peach,&movement_package.program);
 o=create();f=o->user_data;reset(o);f->u.pe.has_float=true;f->u.pe.parasol_gobj_0=o;f->input.held_buttons[0]=HSD_PAD_XY;f->input.lstick[0].y=-1;CHECK(nativeFighterHeldMotion_Select(o,ftCo_MS_ItemParasolOpen,341)==ftCo_MS_ItemParasolOpen);CHECK(ftCo_Mushroom_SelectAirResume(o,ftCo_MS_FallAerial)&&f->mv.co.mushroom.resume_air==ftCo_FallAerial_Enter);CHECK(!ftCo_AerialAction_CheckManual(o)&&!ftCo_AerialAction_CheckAutomatic(o));f->ground_or_air=GA_Air;ft_8008A348(o,0);CHECK(trace==135&&donor_calls==0);f->x2221_b4=true;f->x2104=1;movement_timer(o);CHECK(drop_calls==1);HSD_GObjFree(o);
 // Unchanged HEAD sampler uses a genuine unowned Mario GObj. Source Peach
 // absent-hook common tails must agree, without inheriting Peach union rules.
 HSD_GObj* stock=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter original={.gobj=stock,.kind=Ft_Kind_Mario,.x8_spawnNum=UINT32_C(0x80000001)};GObj_InitUserData(stock,4,removed,&original);
 o=create();f=o->user_data;reset(o);
 int common[]={ftCo_MS_FallAerial,ftCo_MS_FallSpecial,ftCo_MS_ItemParasolFallSpecial,ftCo_MS_Wait,341};
 for(unsigned i=0;i<ARRAY_SIZE(common);++i){CHECK(nativeFighterHeldMotion_Select(o,common[i],-1)==reference_nativeFighterHeldMotion_Select(stock,common[i],-1));CHECK(ftCo_Mushroom_SelectAirResume(o,common[i])==reference_ftCo_Mushroom_SelectAirResume(stock,common[i]));CHECK(f->mv.co.mushroom.resume_air==original.mv.co.mushroom.resume_air);}
 for(unsigned air=0;air<2;++air)for(unsigned dead=0;dead<2;++dead){f->ground_or_air=original.ground_or_air=air?GA_Air:GA_Ground;dead_fall=dead;trace=0;ft_8008A348(o,-1.25f);unsigned actual_trace=trace;trace=0;reference_ft_8008A348(stock,-1.25f);CHECK(trace==actual_trace&&f->motion_id==original.motion_id&&f->cur_pos.z==original.cur_pos.z);}
 CHECK(ftCo_AerialAction_CheckManual(o)==reference_ftCo_AerialAction_CheckManual(stock));CHECK(ftCo_AerialAction_CheckAutomatic(o)==reference_ftCo_AerialAction_CheckAutomatic(stock));HSD_GObjFree(o);HSD_GObjFree(stock);
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&bank.motion_layout,NULL,&movement_package.program,&diagnostic));movement_package.target=Ft_Kind_CustomFirst;tables[Ft_Kind_CustomFirst]=&movement_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&movement_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=movement_package.assets.motions;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&movement_package.program);
 o=create();f=o->user_data;
 reset(o);f->ground_or_air=GA_Air;CHECK(helper(o,"WaitIntrinsic"));CHECK(trace==12356&&state(o,"blend",NULL).as.f32==0&&state(o,"completed",NULL).as.i64==1);
 reset(o);f->ground_or_air=GA_Air;CHECK(helper(o,"BlendIntrinsic"));CHECK(trace==12356&&state(o,"blend",NULL).as.f32==-0.375f&&state(o,"completed",NULL).as.i64==2);
 HSD_GObjFree(o);
 for(unsigned blend=0;blend<2;++blend)for(unsigned mode=1;mode<=5;++mode){o=create();f=o->user_data;reset(o);NativeScriptReceiverView saved_view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&saved_view));mutation_mode=mode;mutation_point=6;bool success=helper(o,blend?"BlendIntrinsic":"WaitIntrinsic");if(success)fprintf(stderr,"intrinsic unexpected success blend%u mode%u trace%u\n",blend,mode,trace);CHECK(!success);if(mode==2){CHECK(replacement==o);CHECK(state(replacement,"completed",NULL).as.i64==0);HSD_GObjFree(replacement);}else if(mode!=1){NativeScriptValue completed;for(unsigned i=0;i<bank.declarations.value_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,values[i].name,"completed")){CHECK(nativeScriptStorage_Read(saved_view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&completed,&diagnostic));CHECK(completed.as.i64==0);}HSD_GObjFree(o);}}
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS movement scheduler\",\"checks\":%u,\"hooks\":8,\"lifetime_cases\":64}\n",checks);return 0;
}
