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
#include <melee/ft/ftwalkcommon.h>
#include "boundary.h"
#include "air_boundary.h"
#include <melee/ft/native_script_binding_air_physics.h>
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
static unsigned comparisons,primitive_calls;
static NativeScriptMoveLayout layout={.common_count=341,.first=400,.count=10,.ground={402,404,406,408},.air={403,405,407,409}};
static Fighter fighters[1024];static unsigned allocated,checks,cases;
#define CHECK(v) do{++checks;if(!(v)){fprintf(stderr,"Air physics line%u %s\n",__LINE__,#v);exit(1);}}while(0)
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

void Frozen_ft_80083090(Fighter_GObj*,bool (*)(Fighter_GObj*,int),HSD_GObjEvent);
void Frozen_ft_800831CC(Fighter_GObj*,bool (*)(Fighter_GObj*,int),HSD_GObjEvent);
void Frozen_ft_800835B0(Fighter_GObj*,bool (*)(Fighter_GObj*,int),HSD_GObjEvent);
void Frozen_ft_800851C0(Fighter_GObj*);
void Frozen_ft_800851D0(Fighter_GObj*);
#include <melee/ft/ft_084E.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/mp/mplib.h>
static void prepare(void)
{
 const char* invalid[]={"collision.air_landing_callback(None)","collision.air_landing_callback(0)"};
 for(unsigned i=0;i<ARRAY_SIZE(invalid);++i){
  initialize_bank();snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc Wrong() -> int:\n    return 1\nfunc WithArgument(float value):\n    pass\nfunc Test():\n    %s\n",invalid[i]);
  NativeScriptCompileRequest negative={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic diagnostic={0};
  CHECK(!nativeScriptCompile_Build(&bank,&negative,&accepted,&diagnostic));CHECK(!accepted&&diagnostic.message[0]);++cases;
 }
 initialize_bank();
 strcpy(source,"fighter:\n    behavior independent\nstate int motion\nstate int tail\nstate int landed\naction FaultDriver:\n    on tick:\n        position.x = 999\nfunc Land():\n    state.landed = state.landed + 1\n    position.x = 123\nfunc Resolve():\n    collision.air_landing_callback(Land)\n    state.tail = state.tail + 1\nfunc PlainVelocity():\n    physics.animation_vertical_velocity()\n    state.tail = state.tail + 1\nfunc Velocity():\n    physics.animation_vertical_velocity_adjusted()\n    state.tail = state.tail + 1\nfunc Wrong() -> int:\n    return 1\nfunc WithArgument(float value):\n    state.tail = state.tail + 1\n");
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d={0};
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&prepared,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 package.address=&package;package.target=Ft_Kind_CustomFirst;package.ready=package.assets.ready=true;package.assets.facts.moves=layout;package.assets.motions=native_motions;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=native_motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&prepared);
}
static void callback(Fighter_GObj* object){((Fighter*)object->user_data)->cur_pos.x=123;air_point(Air_Helper,object);}
static void settings(HSD_GObj* object,unsigned sample)
{
 air_reset();Fighter* f=object->user_data;f->cur_pos=(Vec3){1,2,3};f->coll_data.cur_pos=(Vec3){-1,-2,-3};f->facing_dir=sample%3==0?-1:sample%3==1?0:1;
 f->x2064_ledgeCooldown=sample&1;f->x2224_b2=sample&2;air_contact=sample&4;air_environment=sample&8;air_wall=sample&16;air_cliff=sample&32;
}
static void native_call(unsigned operation,HSD_GObj* object,bool frozen)
{
 switch(operation){case 0:if(frozen)Frozen_ft_80083090(object,ftCo_80096CC8,callback);else ft_80083090(object,ftCo_80096CC8,callback);break;
 case 1:if(frozen)Frozen_ft_800831CC(object,ftCo_80096CC8,callback);else ft_800831CC(object,ftCo_80096CC8,callback);break;
 case 2:if(frozen)Frozen_ft_800835B0(object,ftCo_80096CC8,callback);else ft_800835B0(object,ftCo_80096CC8,callback);break;default:abort();}
}
static void healthy(void)
{
 for(unsigned sample=0;sample<64;++sample){
  HSD_GObj* o=create();Fighter* f=o->user_data;settings(o,sample);Fighter initial=*f;
  Frozen_ft_800831CC(o,ftCo_80096CC8,callback);Fighter expected=*f;unsigned n=air_count,trace[32];memcpy(trace,air_trace,sizeof(trace));
  *f=initial;settings(o,sample);CHECK(invoke(o,"Resolve"));CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(air_count==n&&!memcmp(trace,air_trace,n*sizeof(unsigned)));CHECK(state(o,"tail",NULL).as.i64==1);CHECK(state(o,"landed",NULL).as.i64==((sample&4)&&!(sample&8)?1:0));++comparisons;HSD_GObjFree(o);
 }
 for(unsigned op=0;op<3;++op)for(unsigned sample=0;sample<64;++sample){
  HSD_GObj* o=create();Fighter* f=o->user_data;settings(o,sample);Fighter initial=*f;native_call(op,o,true);Fighter expected=*f;unsigned n=air_count,trace[32];memcpy(trace,air_trace,sizeof(trace));*f=initial;settings(o,sample);native_call(op,o,false);CHECK(!memcmp(f,&expected,sizeof(*f)));CHECK(air_count==n&&!memcmp(trace,air_trace,n*sizeof(unsigned)));++comparisons;HSD_GObjFree(o);
 }
 HSD_GObj* o=create();Fighter* f=o->user_data;
 const int lines[]={-1,0,0,0};const u32 flags[]={0,0,LINE_FLAG_PLATFORM,LINE_FLAG_PLATFORM};const float y[]={1,0,.25f,.25001f};
 for(unsigned i=0;i<4;++i){settings(o,4);air_line=lines[i];air_flags=flags[i];f->input.lstick[0].y=y[i];CHECK(invoke(o,"Resolve"));CHECK(air_trace[air_count-1]==(i==1||i==3?Air_Helper:Air_Cliff));++cases;}
 HSD_GObjFree(o);
}
static void velocity(void)
{
 struct Fighter_804D6520_t bunny={0};struct Fighter_804D6520_t* saved=Fighter_804D6520;Fighter_804D6520=&bunny;
 const uint32_t bits[]={0,0x80000000,0x3fc00000,0xbf800000,0x7f800000,0xff800000,0x7fc12345};
 for(unsigned b=0;b<2;++b)for(unsigned i=0;i<ARRAY_SIZE(bits);++i)for(unsigned j=0;j<ARRAY_SIZE(bits);++j){
  HSD_GObj* o=create();Fighter* f=o->user_data;memcpy(&f->x6A4_transNOffset.y,&bits[i],4);memcpy(&bunny.x14,&bits[j],4);f->x197C=b?(HSD_GObj*)(uintptr_t)1:NULL;
  Frozen_ft_800851D0(o);float expected=f->self_vel.y;f->self_vel.y=99;CHECK(invoke(o,"Velocity"));CHECK(!memcmp(&expected,&f->self_vel.y,4));CHECK(state(o,"tail",NULL).as.i64==1);++comparisons;HSD_GObjFree(o);
 }
 HSD_GObj* o=create();Fighter* f=o->user_data;Fighter_804D6520=NULL;f->x197C=NULL;f->x6A4_transNOffset.y=-3;CHECK(invoke(o,"Velocity")&&f->self_vel.y==-3);HSD_GObjFree(o);++cases;
 o=create();f=o->user_data;f->x197C=(HSD_GObj*)(uintptr_t)1;f->self_vel.y=17;CHECK(!invoke(o,"Velocity")&&f->self_vel.y==17&&state(o,"tail",NULL).as.i64==0);++cases;HSD_GObjFree(o);Fighter_804D6520=saved;
}
static void plain_velocity(void)
{
 const uint32_t bits[]={0,0x80000000,0x3fc00000,0xbf800000,0x7f800000,0xff800000,0x7fc12345};
 struct Fighter_804D6520_t* saved=Fighter_804D6520;
 // Plain root motion has no Bunny Hood multiplier or data dependency, even
 // with an attached Item. Preserve the other velocity axes and Float bits.
 Fighter_804D6520=NULL;
 for(unsigned attached=0;attached<2;++attached)for(unsigned i=0;i<ARRAY_SIZE(bits);++i){
  HSD_GObj* o=create();Fighter* f=o->user_data;
  f->x197C=attached?(HSD_GObj*)(uintptr_t)1:NULL;
  f->self_vel=(Vec3){17,99,-23};memcpy(&f->x6A4_transNOffset.y,&bits[i],4);
  Frozen_ft_800851C0(o);Vec3 expected=f->self_vel;f->self_vel.y=99;
  CHECK(invoke(o,"PlainVelocity"));CHECK(!memcmp(&expected,&f->self_vel,sizeof(expected)));
  CHECK(state(o,"tail",NULL).as.i64==1);++comparisons;++cases;HSD_GObjFree(o);
 }
 HSD_GObj* o=create();NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,
  .generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue extra={.type=NativeScriptType_F32,.as.f32=1},out={.type=NativeScriptType_U64,.as.u64=99};
 NativeScriptDiagnostic diagnostic;
 CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_PhysicsAnimationVerticalVelocity,&extra,1,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);HSD_GObjFree(o);++cases;
 Fighter_804D6520=saved;
}
static NativeScriptRuntimeContext context(HSD_GObj* o){return (NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static void cleanup(HSD_GObj* o,Fighter* original,unsigned failure)
{
 if(failure==1)return;
 if(failure==6){HSD_GObjFree(o);return;}
 o->user_data=original;o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);
}
static void mutation(void)
{
 const unsigned stages[]={Air_MapA,Air_MapE,Air_Environment,Air_Wall,Air_Cliff,Air_Helper};
 for(unsigned i=0;i<ARRAY_SIZE(stages);++i)for(unsigned failure=1;failure<=7;++failure){
  HSD_GObj* o=create();Fighter* f=o->user_data;settings(o,stages[i]==Air_MapA?1:stages[i]==Air_Helper?4:0);Vec3 before=f->cur_pos;
  NativeScriptRuntimeContext c=context(o);NativeScriptValue arg={.type=NativeScriptType_Callable,.as.u64=code_id("Land")};NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_C(0xfeedface12345678)},saved=result;NativeScriptDiagnostic d={0};air_stop=stages[i];air_failure=failure;
  CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_CollisionAirLandingCallback,&arg,1,&result,&d));CHECK(!memcmp(&result,&saved,sizeof(result)));CHECK(air_stop==0&&air_trace[air_count-1]==stages[i]);
  if(stages[i]==Air_MapA||stages[i]==Air_MapE)CHECK(!memcmp(&before,&f->cur_pos,sizeof(before)));
  cleanup(o,f,failure);++cases;
 }
 for(unsigned op=0;op<3;++op)for(unsigned stage=Air_MapE;stage<=Air_Cliff;++stage){
  if(stage==Air_Helper)continue;HSD_GObj* o=create();Fighter* f=o->user_data;settings(o,0);if(op==0&&stage==Air_Wall){HSD_GObjFree(o);continue;}
  air_stop=stage;air_failure=2;native_call(op,o,false);CHECK(air_stop==0&&air_trace[air_count-1]==stage);CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Error);++cases;HSD_GObjFree(o);(void)f;
 }
 // A real nested owned helper fault withholds native result and authored tail.
 HSD_GObj* o=create();settings(o,4);air_stop=Air_Helper;air_failure=2;CHECK(!invoke(o,"Resolve"));CHECK(state(o,"tail",NULL).as.i64==0&&nativeScriptFighter_Status(o)==NativeScriptVM_Error);++cases;HSD_GObjFree(o);
}
static void malformed(void)
{
 HSD_GObj* o=create();NativeScriptRuntimeContext c=context(o);NativeScriptDiagnostic d={0};NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=123},saved=out;
 NativeScriptValue bad[]={{.type=NativeScriptType_Null},{.type=NativeScriptType_Callable,.as.u64=UINT64_MAX},{.type=NativeScriptType_Callable,.as.u64=code_id("Wrong")},{.type=NativeScriptType_Callable,.as.u64=code_id("WithArgument")},{.type=NativeScriptType_Callable,.as.u64=prepared.hooks[NativeScriptFighter_Tick]},{.type=NativeScriptType_I32,.as.i64=code_id("Land")}};
 for(unsigned i=0;i<ARRAY_SIZE(bad);++i){air_reset();CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_CollisionAirLandingCallback,&bad[i],1,&out,&d));CHECK(!memcmp(&out,&saved,sizeof(out))&&!air_count);++cases;}
 air_reset();CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_CollisionAirLandingCallback,NULL,0,&out,&d)&&!air_count);++cases;
 CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_PhysicsAnimationVerticalVelocityAdjusted,bad,1,&out,&d)&&!memcmp(&out,&saved,sizeof(out)));++cases;HSD_GObjFree(o);
 o=create();c=context(o);air_reset();package.assets.facts.moves.count++;CHECK(!nativeScriptBindingAirPhysics_Invoke(&c,NativeScriptBinding_PhysicsAnimationVerticalVelocityAdjusted,NULL,0,&out,&d)&&!memcmp(&out,&saved,sizeof(out))&&!air_count);package.assets.facts.moves.count--;HSD_GObjFree(o);++cases;
}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
 healthy();velocity();plain_velocity();mutation();malformed();CHECK(nativeScriptRuntime_Idle()&&!nativeScriptFighter_ReferencesProgram(&bank));
 printf("{\"status\":\"PASS connected air physics\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"source_invocations\":%u}\n",checks,cases,comparisons,primitive_calls);return 0;
}

