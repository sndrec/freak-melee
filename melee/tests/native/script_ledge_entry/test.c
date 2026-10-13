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
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/native_script_binding_ledge.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/ft_081B.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage ledge_package;
#define bank ledge_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Fighter fighters[256],alternate;static unsigned count,checks;static NativeScriptDiagnostic diagnostic;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned mutation_point,mutation_mode;
static unsigned events[32],event_count;static unsigned lifetime_cases,healthy_cases,generic_pairs,generic_cases;
bool valid_floor=true;Vec3 effect_position;HSD_GObj* replacement;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"Ledge entry %u %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int completed\nstate int zero\nstate int nested\nstate bool decision\nfunc GroundLedge():\n    state.decision = collision.check_ground_ledge(0)\n    state.completed = state.completed + 1\nfunc Cliff():\n    state.decision = collision.cliff_catch()\n    state.completed = state.completed + 1\nfunc Aerial():\n    state.decision = controls.try_aerial_jump()\n    state.completed = state.completed + 1\nfunc Enter():\n    state.catch_ledge()\n    state.completed = state.completed + 1\nfunc Fault():\n    state.nested = 5 / state.zero\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,NULL,&ledge_package.program,&diagnostic));ledge_package.address=&ledge_package;ledge_package.target=Ft_Kind_CustomFirst;ledge_package.ready=ledge_package.assets.ready=true;ledge_package.assets.facts.moves=layout;ledge_package.assets.facts.joints=4;ledge_package.assets.parts.parts_num=4;tables[Ft_Kind_CustomFirst]=&ledge_package.assets.parts;gFtDataList[Ft_Kind_CustomFirst]=&ledge_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=ledge_package.assets.motions;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&ledge_package.program);
}
static void removed(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static HSD_GObj* create(void){CHECK(count<ARRAY_SIZE(fighters));HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[count++];*f=(Fighter){.gobj=o,.kind=ledge_package.target,.behavior_kind=ledge_package.target,.x8_spawnNum=10+count,.ft_data=&ledge_package.assets.character.data};GObj_InitUserData(o,4,removed,f);CHECK(nativeScriptFighter_Attach(o));return o;}
static NativeScriptValue state(HSD_GObj* o,const char* name,const NativeScriptValue* input){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&view));for(unsigned i=0;i<bank.declarations.value_count;++i)if(!values[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));return result;}CHECK(false);return(NativeScriptValue){0};}
static bool helper(HSD_GObj* o,const char* name){for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name)){NativeScriptValue result={0};return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,i,NULL,0,&result,&diagnostic)==NativeScriptVM_Ok;}CHECK(false);return false;}

void ledge_observe(HSD_GObj* o,unsigned phase){
 CHECK(event_count<ARRAY_SIZE(events));events[event_count++]=phase;
 if(phase!=mutation_point||!mutation_mode)return;
 unsigned mode=mutation_mode;mutation_mode=0;
 if(mode==1)HSD_GObjFree(o);
 else if(mode==2){HSD_GObjFree(o);replacement=create();}
 else if(mode==3){alternate=*(Fighter*)o->user_data;o->user_data=&alternate;}
 else if(mode==4)((Fighter*)o->user_data)->x8_spawnNum++;
 else if(mode==5)CHECK(!helper(o,"Fault"));
 else if(mode==6)o->classifier=HSD_GOBJ_CLASS_ITEM;
 else if(mode==7){++ledge_package.program.identity;++((NativeScriptFighterProgram*)nativeScriptFighter_Program(ledge_package.target))->identity;}
 else if(mode==8)++bank.storage_layout.identity;
 else if(mode==9)++bank.storage_parameters.identity;
}
static void init(HSD_GObj* o,int left,int air){
 Fighter* f=o->user_data;static FtSFX sfx; sfx.x28=42;ledge_package.assets.character.data.x4C_sfx=&sfx;
 f->ground_or_air=air?GA_Air:GA_Ground;f->facing_dir=left?-1:1;
 f->coll_data.env_flags=left?Collide_LeftLedgeGrab:Collide_RightLedgeGrab;
 f->coll_data.ledge_id_left=13;f->coll_data.ledge_id_right=17;
 f->x68C_transNPos=(Vec3){0,4.5f,2.25f};f->self_vel=(Vec3){9,8,7};f->x74_self_accel=(Vec3){6,5,4};
 f->x8c_kb_vel=(Vec3){3,2,1};f->gr_vel=11;f->x221D_b7=0;f->x1A6A=0;
 event_count=0;mutation_point=mutation_mode=0;memset(&effect_position,0,sizeof(effect_position));
}
static void compare(Fighter* a,Fighter* b){
 CHECK(a->facing_dir==b->facing_dir&&a->motion_id==b->motion_id&&a->ground_or_air==b->ground_or_air);
 CHECK(!memcmp(&a->cur_pos,&b->cur_pos,sizeof(Vec3))&&!memcmp(&a->self_vel,&b->self_vel,sizeof(Vec3)));
 CHECK(!memcmp(&a->x74_self_accel,&b->x74_self_accel,sizeof(Vec3))&&!memcmp(&a->x8c_kb_vel,&b->x8c_kb_vel,sizeof(Vec3)));
 CHECK(a->mv.co.cliff.ledge_id==b->mv.co.cliff.ledge_id&&a->x221D_b7==b->x221D_b7&&a->x1A6A==b->x1A6A&&a->x209A==b->x209A);
 CHECK(a->ecb_lock==b->ecb_lock&&a->x1968_jumpsUsed==b->x1968_jumpsUsed);
}
static void generic(void)
{
 common_data.x480=.25f;
 const char* names[]={"GroundLedge","Cliff","Aerial"};
 const uint32_t ids[]={NativeScriptBinding_CollisionCheckGroundLedge,NativeScriptBinding_CollisionCliffCatch,NativeScriptBinding_ControlsTryAerialJump};
 for(unsigned method=0;method<3;++method)for(unsigned mask=0;mask<8;++mask){
  HSD_GObj*o=create();init(o,1,1);Fighter*f=o->user_data;valid_floor=mask&1;f->x2064_ledgeCooldown=mask&2;f->can_multijump=mask&4;Fighter initial=*f;
  bool expected=method==0?ft_CheckGroundAndLedge(o,0):method==1?ftCliffCommon_80081298(o):ftCo_800CB870(o);Fighter after=*f;unsigned trace[32],n=event_count;memcpy(trace,events,sizeof(trace));
  *f=initial;event_count=0;CHECK(helper(o,names[method]));compare(f,&after);CHECK(state(o,"decision",NULL).as.u64==expected&&state(o,"completed",NULL).as.i64==1);CHECK(n==event_count&&!memcmp(trace,events,n*sizeof(unsigned)));++generic_pairs;HSD_GObjFree(o);
 }
 for(unsigned method=0;method<3;++method)for(unsigned mode=1;mode<=9;++mode)for(unsigned vm=0;vm<2;++vm){
  HSD_GObj*o=create();init(o,1,1);Fighter*f=o->user_data;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));
  uint64_t program_identity=ledge_package.program.identity,layout_identity=bank.storage_layout.identity,parameter_identity=bank.storage_parameters.identity;
  mutation_point=method==0?11:method==1?1:12;mutation_mode=mode;valid_floor=true;
  NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=f->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};NativeScriptValue arg={.type=NativeScriptType_I32},out={.type=NativeScriptType_U64,.as.u64=99},sentinel=out;
  if(vm)CHECK(!helper(o,names[method]));else {bool accepted=nativeScriptBindingLedge_Invoke(&c,ids[method],&arg,method==0?1:0,&out,&diagnostic);if(accepted)fprintf(stderr,"Generic accepted method%u mode%u\n",method,mode);CHECK(!accepted&&!memcmp(&out,&sentinel,sizeof(out)));}
  CHECK(!mutation_mode);if(mode!=1&&mode!=2){NativeScriptValue value;for(unsigned d=0;d<bank.declarations.value_count;++d)if(nativeScriptSource_SpanEquals(&bank.tree,values[d].name,"completed")){CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|d,NULL,0,&value,&diagnostic));CHECK(value.as.i64==0);}}
  ledge_package.program.identity=program_identity;((NativeScriptFighterProgram*)nativeScriptFighter_Program(ledge_package.target))->identity=program_identity;bank.storage_layout.identity=layout_identity;bank.storage_parameters.identity=parameter_identity;
  if(mode==2)HSD_GObjFree(replacement);else if(mode!=1){o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}++generic_cases;
 }
}
int main(void){
 prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 common_data.x5F0=123;
 for(unsigned floor=0;floor<2;++floor)for(unsigned left=0;left<2;++left)for(unsigned air=0;air<2;++air){
  HSD_GObj* o=create();HSD_GObj* reference=create();init(o,left,air);valid_floor=floor;
  CHECK(helper(o,"Enter"));CHECK(state(o,"completed",NULL).as.i64==1);
  unsigned actual_events[32],actual_count=event_count;memcpy(actual_events,events,sizeof(events));Vec3 actual_effect=effect_position;
  init(reference,left,air);reference_ftCliffCommon_80081370(reference);compare(o->user_data,reference->user_data);
  CHECK(event_count==actual_count&&!memcmp(events,actual_events,event_count*sizeof(*events)));CHECK(!memcmp(&effect_position,&actual_effect,sizeof(Vec3)));
  ++healthy_cases;HSD_GObjFree(o);HSD_GObjFree(reference);
 }
 unsigned phases[]={1,2,3,4,5,6,9};
 for(unsigned i=0;i<ARRAY_SIZE(phases);++i)for(unsigned mode=1;mode<=6;++mode){
  HSD_GObj* o=create();init(o,1,0);Fighter* f=o->user_data;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));
  valid_floor=phases[i]!=6;mutation_point=phases[i];mutation_mode=mode;
  CHECK(!helper(o,"Enter"));CHECK(events[event_count-1]==phases[i]);
  if(mode!=1&&mode!=2){NativeScriptValue value;for(unsigned d=0;d<bank.declarations.value_count;++d)if(nativeScriptSource_SpanEquals(&bank.tree,values[d].name,"completed")){CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|d,NULL,0,&value,&diagnostic));CHECK(value.as.i64==0);}}
  if(phases[i]==1)CHECK(!f->x221D_b7&&f->x1A6A==0);if(phases[i]<6)CHECK(f->x1A6A==0);
  if(mode==2){CHECK(replacement==o);CHECK(state(replacement,"completed",NULL).as.i64==0);HSD_GObjFree(replacement);}
  else if(mode!=1){o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);}++lifetime_cases;
 }
 generic();
 HSD_GObj* o=create();init(o,1,0);NativeScriptRuntimeContext context={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum};NativeScriptValue result={.type=NativeScriptType_I64,.as.i64=77};
 CHECK(!nativeScriptBindingLedge_Invoke(&context,NativeScriptBinding_StateCatchLedge,NULL,1,&result,&diagnostic));CHECK(result.type==NativeScriptType_I64&&result.as.i64==77&&event_count==0);
 context.source_class=NativeCallbackSource_Item;CHECK(!nativeScriptBindingLedge_Invoke(&context,NativeScriptBinding_StateCatchLedge,NULL,0,&result,&diagnostic));CHECK(event_count==0);HSD_GObjFree(o);
 CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS ledge entry\",\"checks\":%u,\"healthy_comparisons\":%u,\"lifetime_cases\":%u,\"generic_pairs\":%u,\"generic_lifetime_cases\":%u}\n",checks,healthy_cases,lifetime_cases,generic_pairs,generic_cases);return 0;
}
