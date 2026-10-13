#include "boundary.h"
#include "attack_boundary.h"
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
#pragma clang section data = ""
enum{N=4096,F=128,V=128};
static char source_bytes[65536],source[32768];static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[8192];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptFighterProgram program;
static NativeScriptMoveLayout layout={.common_count=341,.first=400,.count=10,.ground={402,404,406,408},.air={403,405,407,409}};
static Fighter fighters[128];static unsigned allocated,checks,cases;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"Attack scheduler line%u %s\n",__LINE__,#v);exit(1);}}while(0)
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
 Fighter* fighter=&fighters[allocated++];memset(fighter,0,sizeof(*fighter));fighter->gobj=object;fighter->kind=Ft_Kind_GameWatch;fighter->behavior_kind=Ft_Kind_GameWatch;fighter->x8_spawnNum=allocated;fighter->motion_id=-777;fighter->facing_dir=1;fighter->x34_scale=(Vec3){1,1,1};fighter->co_attrs.jab_2_input_window=13;fighter->co_attrs.jab_3_input_window=17;
 GObj_InitUserData(object,4,remove_data,fighter);CHECK(nativeScriptFighter_Attach(object));set_int(object,"mapping",402);return object;
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

static void prepare(unsigned mode)
{
 initialize_bank();source[0]=0;
 append("fighter:\n    behavior independent\nstate int tag\nstate int calls\nstate int mapping\nstate int argument\nstate float angle\nstate int tail\nfunc Accessory():\n    state.tail = state.tail + 1\naction FaultDriver:\n    on tick:\n        position.x = 999\n");
 static const char* names[]={"jab_enter","rapid_jab_enter","down_tilt_enter","aerial_attack_motion","aerial_attack_enter","forward_smash_start","forward_smash_enter","crouch_wait_enter"};
 if(mode!=2)for(unsigned i=0;i<ARRAY_SIZE(names);++i){
  if(mode==1&&i==6)continue;
  char line[256];snprintf(line,sizeof(line),"action Hook%u:\n    on %s:\n        state.tag = %u\n        state.calls = state.calls + 1\n        position.x = %u\n",i,names[i],101+i,101+i);append(line);
  if(i==1)append("        callbacks.accessory(Accessory)\n");
  if(i==3)append("        state.argument = event.motion\n        return state.mapping\n");
  if(i==6)append("        state.angle = event.angle\n");
 }
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};
 const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)||!nativeScriptFighter_PrepareProgram(Ft_Kind_GameWatch,accepted,&layout,NULL,&program,&diagnostic)){
  fprintf(stderr,"prepare %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);
 }
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_GameWatch));nativeScriptFighter_PublishProgram(Ft_Kind_GameWatch,&program);
}
static void trace(const unsigned* values,unsigned length)
{
 CHECK(attack_count==length);for(unsigned i=0;i<length;++i)CHECK(attack_trace[i]==values[i]);CHECK(attack_stock==0);
}
#define TRACE(...) do{const unsigned expected[]={__VA_ARGS__};trace(expected,ARRAY_SIZE(expected));}while(0)
static void invoke(unsigned operation,HSD_GObj* object)
{
 switch(operation){
 case 0:Attack1_decideAttack11(object);break;
 case 1:fn_800D6AC4(object);break;
 case 2:AttackLw3_decideFighter(object);break;
 case 3:AttackAir_decideFighter(object);break;
 case 4:AttackS4_decideFighter(object,-1,0.0f);break;
 case 5:fn_800D62C4(object);break;
 case 6:Attack1_doAttack12Rapid(object);break;
 default:CHECK(false);
 }
}
static void replacements(void)
{
 prepare(0);HSD_GObj* object=create();Fighter* fighter=object->user_data;
 for(unsigned operation=0;operation<7;++operation){
  attack_reset();attack_pickup=true;attack_held=true;int before=fighter->motion_id;invoke(operation,object);++cases;
  switch(operation){
  case 0:TRACE(101);CHECK(fighter->motion_id==before);break;
  case 1:TRACE(102);CHECK(fighter->motion_id==before&&nativeScriptFighter_MotionBound(object,NativeScriptMotion_Accessory));break;
  case 2:TRACE(103);CHECK(fighter->motion_id==before);break;
  case 3:TRACE(104,Attack_Motion,Attack_Animation,105);CHECK(fighter->motion_id==402&&state(object,"argument",NULL).as.i64==ftCo_MS_AttackAirN);break;
  case 4:TRACE(107);CHECK(fighter->motion_id==before&&fighter->facing_dir==-1&&state(object,"angle",NULL).as.f32==0);break;
  case 5:TRACE(Attack_Motion,Attack_HeldSetup,Attack_CommonFlags,108);CHECK(fighter->motion_id==ftCo_MS_SquatWait);break;
  case 6:TRACE(101);CHECK(fighter->motion_id==before);break;
  }
 }
 attack_reset();AttackS4_decideFighter(object,1,0.375f);TRACE(107);CHECK(state(object,"angle",NULL).as.f32==0.375f&&fighter->facing_dir==1);++cases;
 // Direct common rapid helper retains native pickup early return. This does not
 // pretend the unavailable authored attack.enter_rapid_jab binding is installed.
 attack_reset();attack_pickup=true;fighter->throw_flags=0xABC;ftCo_800D6B00(object,403);TRACE(Attack_Pickup);CHECK(fighter->throw_flags==0xABC);++cases;
 attack_reset();attack_pickup=false;ftCo_800D6B00(object,403);TRACE(Attack_Pickup,Attack_Motion,Attack_Animation);CHECK(fighter->motion_id==403);++cases;
 const struct{int supplied;int accepted;} mappings[]={{0,0},{340,340},{400,400},{409,409},{341,ftCo_MS_AttackAirN},{399,ftCo_MS_AttackAirN},{410,ftCo_MS_AttackAirN},{-1,ftCo_MS_AttackAirN},{INT32_MIN,ftCo_MS_AttackAirN},{INT32_MAX,ftCo_MS_AttackAirN}};
 for(unsigned i=0;i<ARRAY_SIZE(mappings);++i){set_int(object,"mapping",mappings[i].supplied);attack_reset();AttackAir_decideFighter(object);
  TRACE(104,Attack_Motion,Attack_Animation,105);CHECK(fighter->motion_id==mappings[i].accepted);++cases;
 }
 HSD_GObjFree(object);prepare(1);object=create();fighter=object->user_data;
 attack_reset();AttackS4_decideFighter(object,-1,0.0f);TRACE(Attack_Motion,Attack_Animation,106);CHECK(fighter->motion_id==ftCo_MS_AttackS4S);++cases;
 HSD_GObjFree(object);
}
static void defaults(void)
{
 prepare(2);HSD_GObj* object=create();Fighter* fighter=object->user_data;
 for(unsigned operation=0;operation<7;++operation){attack_reset();attack_held=true;invoke(operation,object);++cases;
  if(operation<3||operation==6)TRACE(Attack_Pickup,Attack_Motion,Attack_Animation);
  else if(operation==5)TRACE(Attack_Motion,Attack_HeldSetup,Attack_CommonFlags);
  else TRACE(Attack_Motion,Attack_Animation);
  CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Ok);
 }
 for(unsigned operation=0;operation<3;++operation){attack_reset();attack_pickup=true;fighter->motion_id=-123;invoke(operation,object);TRACE(Attack_Pickup);CHECK(fighter->motion_id==-123);++cases;}
 attack_reset();Attack1_doAttack12(object);TRACE(Attack_Pickup,Attack_Motion);CHECK(fighter->motion_id==ftCo_MS_Attack12&&fighter->hitlag_mul==17);++cases;
 attack_reset();Attack1_doAttack13(object);TRACE(Attack_Pickup,Attack_Motion);CHECK(fighter->motion_id==ftCo_MS_Attack13);++cases;
 attack_reset();ftCo_800D638C(object);TRACE(Attack_Motion);++cases;
 HSD_GObjFree(object);
}
static void common_stops(void)
{
 prepare(2);
 // Every reached common boundary is checked with physical retirement, a real
 // VM fault while the GObj remains live, and native generation advancement.
 for(unsigned operation=0;operation<6;++operation)for(unsigned stage=1;stage<=5;++stage){
  bool reached=stage==Attack_Motion||(stage==Attack_Pickup&&operation<3)||(stage==Attack_Animation&&operation<5)||(operation==5&&(stage==Attack_HeldSetup||stage==Attack_CommonFlags));
  if(!reached)continue;
  for(unsigned failure=1;failure<=3;++failure){
   HSD_GObj* object=create();Fighter* fighter=object->user_data;NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
   fighter->hitlag_mul=999;fighter->mv.co.attack100.x0=true;fighter->mv.co.attack100.x4=true;
   attack_reset();attack_held=true;attack_stop=stage;attack_failure=failure;invoke(operation,object);++cases;
   CHECK(attack_count&&attack_trace[attack_count-1]==stage&&attack_stock==0);
   if(failure==1)CHECK(!nativeMatchPool_Resolve(reference));
   else {
    CHECK(nativeMatchPool_Resolve(reference)==object);
    CHECK(nativeScriptFighter_Status(object)==(failure==2?NativeScriptVM_Error:NativeScriptVM_Ok));
    if(operation==0)CHECK(fighter->hitlag_mul==999);
    if(operation==1)CHECK(fighter->mv.co.attack100.x0&&fighter->mv.co.attack100.x4);
    HSD_GObjFree(object);
   }
   CHECK(nativeScriptRuntime_Idle());
  }
 }
}
static void callback_stops(void)
{
 prepare(0);
 const unsigned points[]={101,102,103,104,107,108,101};
 for(unsigned operation=0;operation<7;++operation)for(unsigned failure=1;failure<=2;++failure){
  HSD_GObj* object=create();NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
  attack_reset();attack_held=true;attack_stop=points[operation];attack_failure=failure;invoke(operation,object);++cases;
  CHECK(attack_count&&attack_trace[attack_count-1]==points[operation]&&attack_stock==0);
  if(failure==1)CHECK(!nativeMatchPool_Resolve(reference));else{CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);HSD_GObjFree(object);}
  CHECK(nativeScriptRuntime_Idle());
 }
 // Notification after air startup: both preceding native stages survive; fault
 // in the source notification still cannot fall through to a stock handler.
 HSD_GObj* object=create();attack_reset();attack_stop=105;attack_failure=2;AttackAir_decideFighter(object);TRACE(104,Attack_Motion,Attack_Animation,105);
 CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);HSD_GObjFree(object);++cases;
 prepare(1);
 for(unsigned stage=Attack_Motion;stage<=Attack_Animation;++stage)for(unsigned failure=1;failure<=2;++failure){object=create();attack_reset();attack_stop=stage;attack_failure=failure;AttackS4_decideFighter(object,1,0.0f);
  CHECK(attack_trace[attack_count-1]==stage&&attack_stock==0);if(failure==2){CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);HSD_GObjFree(object);}++cases;
 }
 object=create();attack_reset();attack_stop=106;attack_failure=2;AttackS4_decideFighter(object,1,0.0f);TRACE(Attack_Motion,Attack_Animation,106);
 CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);HSD_GObjFree(object);++cases;
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
 replacements();defaults();common_stops();callback_stops();CHECK(nativeScriptRuntime_Idle()&&nativeScriptFighter_CanPublish(Ft_Kind_GameWatch));
 printf("{\"status\":\"PASS connected attack scheduler\",\"checks\":%u,\"cases\":%u,\"stock_dispatches\":%u}\n",checks,cases,attack_stock);return 0;
}
