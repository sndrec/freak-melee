#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include "kernels.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage target_package;
#define bank target_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[2048],alternate;static Item items[128];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static HSD_JObj joints[2048];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*replacement;
static unsigned checks,fighter_count,item_count,comparisons,vm_calls,compile_cases,mutation_cases,boundary_cases;static NativeScriptDiagnostic diagnostic;
static unsigned scheduler_pairs,scheduler_cases;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"catch policy %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static bool compile_source(const char* source)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;++compile_cases;
    return nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<sizeof(fighters)/sizeof(*fighters));HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);o->hsd_obj=&joints[fighter_count];Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,
        .x8_spawnNum=id,.ft_data=&target_package.assets.character.data,.cur_pos={1.25f,-2.5f,3.75f}};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static const char* hook_names[]={"catch_allowed","catch_start","catch_frame","catch_exit","catch_wait_release","catch_pull_anchor","catch_pull_ready","catch_pull_follow","capture_wait_enter","air_tether_input"};
static const unsigned hook_ids[]={NativeScriptFighter_CatchAllowed,NativeScriptFighter_CatchStart,NativeScriptFighter_CatchFrame,NativeScriptFighter_CatchExit,NativeScriptFighter_CatchWaitRelease,NativeScriptFighter_CatchPullAnchor,NativeScriptFighter_CatchPullReady,NativeScriptFighter_CatchPullFollow,NativeScriptFighter_CaptureWaitEnter,NativeScriptFighter_AirTetherInput};
static bool with_hooks,reference_mode,throw_policies;bool frames,heavy_classification;
static unsigned pairs,contracts,retirements,mapping_cases,trace_count;
static int reference_observed,decision,retire_mode,retire_motion,mapping;
static HSD_GObj* holder,*caught,*retired,*held_item;
typedef struct Trace{int event,role,argument,observed;float frame,speed;}Trace;
static Trace trace[64];
static int observed(void){return reference_mode?reference_observed:(int)field(holder,"observed",NULL).as.i64;}
void record(int event,HSD_GObj* o,int argument,float frame,float speed)
{CHECK(trace_count<64);trace[trace_count++]=(Trace){event,o==holder?1:o==caught?2:0,argument,observed(),frame,speed};}
static void integer(HSD_GObj* o,const char* name,int n)
{NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=n};field(o,name,&v);}
static void prepare(bool hooks)
{
 char source[12000]="fighter:\n    behavior independent\nstate int observed\nstate int decision\nstate int mode\nfunc intervene():\n    if state.mode == 1:\n        state.enter(341)\n    if state.mode == 2:\n        state.observed = 1 / (state.mode - 2)\n";
 for(unsigned i=0;hooks&&i<10;++i){char part[900];
  snprintf(part,sizeof(part),"%s    on %s:\n        state.observed = state.observed * 11 + %u\n%s        intervene()\n%s",i==0?"action CatchPolicy:\n":"",hook_names[i],i+1,i==8?"        if other.exists:\n            state.observed = state.observed + 100\n":"",(i==0||i==2||i==5||i==6||i==7||i==9)?"        return state.decision != 0\n":"");strcat(source,part);
 }
 if(throw_policies)strcat(source,
  "action ThrowPolicy:\n    on throw_victim_motion:\n        state.observed = event.motion * 1000 + event.victim_motion\n        intervene()\n        return state.decision\n"
  "    on throw_animation:\n        state.observed = state.observed + 100\n        intervene()\n"
  "    on throw_finish:\n        state.observed = state.observed + 200\n        intervene()\n        return state.decision != 0\n"
  "    on heavy_pickup_finish:\n        state.observed = state.observed + 300\n        intervene()\n        return state.decision != 0\n"
  "    on heavy_item_damage:\n        state.observed = state.observed + 400\n        intervene()\n");
 CHECK(compile_source(source));NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptItemMetadataContext metadata={0};
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&target_package.program,&diagnostic));
 target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
 target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
 gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;
 nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);with_hooks=hooks;
}
static void contracts_check(void)
{
 for(unsigned i=0;i<10;++i){const NativeScriptFighterHookContract* c=&nativeScriptFighterHookContracts[hook_ids[i]];
  CHECK(!strcmp(c->name,hook_names[i]));CHECK(c->argument_count==(i==8));CHECK(c->record_bytes==0);if(i==8)CHECK(c->arguments[0]==NativeScriptType_Object);
  CHECK(c->result_type==((i==0||i==2||i==5||i==6||i==7||i==9)?NativeScriptType_Bool:NativeScriptType_Void));++contracts;
 }
 const unsigned scheduler_ids[]={NativeScriptFighter_ThrowVictimMotion,NativeScriptFighter_ThrowAnimation,NativeScriptFighter_ThrowFinish,NativeScriptFighter_HeavyPickupFinish,NativeScriptFighter_HeavyItemDamage,NativeScriptFighter_DynamicsGroundCollision};
 const char* scheduler_names[]={"throw_victim_motion","throw_animation","throw_finish","heavy_pickup_finish","heavy_item_damage","dynamics_ground_collision"};
 const NativeScriptIRType results[]={NativeScriptType_I32,NativeScriptType_Void,NativeScriptType_Bool,NativeScriptType_Bool,NativeScriptType_Void,NativeScriptType_Bool};
 for(unsigned i=0;i<6;++i){const NativeScriptFighterHookContract* c=&nativeScriptFighterHookContracts[scheduler_ids[i]];
  CHECK(!strcmp(c->name,scheduler_names[i]));CHECK(c->result_type==results[i]);CHECK(c->argument_count==(i==0?2:0));CHECK(c->record_bytes==0);
  if(i==0)CHECK(c->arguments[0]==NativeScriptType_I32&&c->arguments[1]==NativeScriptType_I32);++contracts;
 }
 const char* wrong[]={"fighter:\n    behavior independent\naction Policy:\n    on catch_allowed:\n        return \"bad\"\n","fighter:\n    behavior independent\naction Policy:\n    on catch_start:\n        return true\n"};
 for(unsigned i=0;i<2;++i){bool built=compile_source(wrong[i]);if(built){NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptItemMetadataContext metadata={0};static NativeScriptFighterProgram p;CHECK(!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&p,&diagnostic));}++contracts;}
}
static bool policy(HSD_GObj* o,int id){CHECK(o==holder);reference_observed=reference_observed*11+id;return decision!=0;}
static bool allowed(HSD_GObj* o){return policy(o,1);}static void start(HSD_GObj* o){policy(o,2);}
static bool frame_policy(HSD_GObj* o){return policy(o,3);}static void exit_policy(HSD_GObj* o){policy(o,4);}
static void release_policy(HSD_GObj* o){policy(o,5);}static bool anchor(HSD_GObj* o){return policy(o,6);}
static bool ready(HSD_GObj* o){return policy(o,7);}static bool follow(HSD_GObj* o){return policy(o,8);}
static void capture_enter(HSD_GObj* o,HSD_GObj* other){CHECK(other==caught);policy(o,9);reference_observed+=100;}
static bool air_policy(HSD_GObj* o){return policy(o,10);}
static int throw_mapping(HSD_GObj* o,int a,int b){CHECK(o==holder&&a==222&&b==242);record(19,o,mapping,0,0);if(retire_mode){retired=retire_mode==1?holder:caught;retire_mode=0;HSD_GObjFree(retired);}return mapping;}
static NativeFighterDefinition definition={.on_catch_allowed=allowed,.on_catch_start=start,.on_catch_frame=frame_policy,.on_catch_exit=exit_policy,.on_catch_wait_release=release_policy,.on_catch_pull_anchor=anchor,.on_catch_pull_ready=ready,.on_catch_pull_follow=follow,.on_capture_wait_enter=capture_enter,.on_air_tether_input=air_policy};
static bool mapping_mode;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k)
{if(mapping_mode&&k==Ft_Kind_Koopa)return &definition;return reference_mode&&(with_hooks||throw_policies)&&k==Ft_Kind_CustomFirst?&definition:NULL;}
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;}
static void motion_callback(Fighter_GObj* object)
{
 (void)object;unsigned mode=retire_mode;retire_mode=0;
 if(mode==1||mode==2){retired=mode==1?holder:caught;HSD_GObjFree(retired);}
 if(mode==3)((Fighter*)caught->user_data)->victim_gobj=NULL;
 if(mode==4)target_package.program.identity++;
 if(mode==5)bank.storage_layout.identity++;
 if(mode==6)bank.storage_parameters.identity++;
}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId id,MotionFlags flags,float f,float speed,float blend,Fighter_GObj* other)
{
 (void)flags;(void)blend;(void)other;record(1,o,id,f,speed);Fighter* data=o->user_data;data->motion_id=id;data->cur_anim_frame=f;
 // The full motion engine is controlled. Deliver its native callback at the
 // actual reentry boundary instead of mutating a pure script read.
 if(id==retire_motion&&retire_mode){data->x21EC=motion_callback;data->x21EC(o);}
}
static void pair_begin(bool reference)
{
 reference_mode=reference;reference_observed=0;retired=NULL;trace_count=0;retire_mode=0;
 holder=fighter(!reference,100+fighter_count);caught=fighter(false,100+fighter_count);
 Fighter* h=holder->user_data,*c=caught->user_data;h->victim_gobj=caught;c->victim_gobj=holder;h->motion_id=ftCo_MS_Catch;c->motion_id=ftCo_MS_CapturePulledHi;
 h->x1A5C=caught;c->x1A5C=holder;
 h->native_hitlag_link_generation=(uint32_t)c->x8_spawnNum;c->native_hitlag_link_generation=(uint32_t)h->x8_spawnNum;
 static FighterBone bones[4];static ftData_x8 anchor_data;static ftCo_DatAttrs attrs;
 anchor_data.x11=0;attrs.weight=100;attrs.weight_independent_throws_mask=15;target_package.assets.character.data.x8=&anchor_data;target_package.assets.character.data.x0=&attrs;
 bones[0].joint=&joints[0];h->parts=bones;c->parts=bones;h->facing_dir=1;c->facing_dir=-1;h->grab_timer=c->grab_timer=17;h->x221B_b7=1;c->x2226_b2=1;
 h->input.held_buttons[0]=HSD_PAD_LR;h->input.pressed_buttons=HSD_PAD_A;h->mv.co.common.x0=2;
 if(!reference){integer(holder,"decision",decision);integer(holder,"mode",0);}
}
static void pair_end(void){if(held_item){HSD_GObjFree(held_item);held_item=NULL;}if(holder!=retired)HSD_GObjFree(holder);if(caught!=retired)HSD_GObjFree(caught);holder=caught=NULL;CHECK(nativeScriptRuntime_Idle());}
static void operation(unsigned n,bool ref)
{
 switch(n){
 case 0:CHECK((ref?reference_ftCo_Catch_CheckInput(holder):ftCo_Catch_CheckInput(holder))==(decision!=0||!with_hooks));break;
 case 1:(ref?reference_ftCo_800D8A38:ftCo_800D8A38)(holder);break;
 case 2:(ref?reference_ftCo_800D8AE0:ftCo_800D8AE0)(holder);break;
 case 3:(ref?reference_ftCo_Catch_Anim:ftCo_Catch_Anim)(holder);break;
 case 4:(ref?reference_ftCo_CatchDash_Anim:ftCo_CatchDash_Anim)(holder);break;
 case 5:(ref?reference_fn_800D8E30:fn_800D8E30)(holder);break;
 case 6:(ref?reference_fn_800D9CE8:fn_800D9CE8)(holder);break;
 case 7:(ref?reference_ftCo_CatchPull_Anim:ftCo_CatchPull_Anim)(holder);break;
 case 8:(ref?reference_fn_800DA1D8:fn_800DA1D8)(holder);break;
 case 9:(ref?reference_fn_800DA2B0:fn_800DA2B0)(holder);break;
 case 10:(ref?reference_ftCo_CatchWait_IASA:ftCo_CatchWait_IASA)(holder);break;
 case 11:((Fighter*)holder->user_data)->input.pressed_buttons=0;((Fighter*)holder->user_data)->input.lstick[0].x=1;(ref?reference_ftCo_CatchWait_IASA:ftCo_CatchWait_IASA)(holder);break;
 case 12:(ref?reference_fn_800DB6C8:fn_800DB6C8)(caught);break;
 case 13:((Fighter*)caught->user_data)->motion_id=ftCo_MS_CapturePulledLw;(ref?reference_fn_800DB6C8:fn_800DB6C8)(caught);break;
 case 14:(ref?reference_fn_800DA4FC:fn_800DA4FC)(holder);break;
 case 15:(ref?reference_ftCo_800DD4B0:ftCo_800DD4B0)(holder,219);break;
 case 16:(ref?reference_ftCo_800C3B10:ftCo_800C3B10)(holder);break;
 case 17:(ref?reference_fn_800DBBF8:fn_800DBBF8)(caught);break;
 case 18:(ref?reference_ftCo_CaptureWaitLw_Phys_inline:ftCo_CaptureWaitLw_Phys_inline)(caught);break;
 case 19:(ref?reference_fn_800DBED4_inline:fn_800DBED4_inline)(caught);break;
 default:CHECK(false);
 }
}
static void comparisons_check(void)
{
 for(unsigned hooks=0;hooks<2;++hooks){prepare(hooks!=0);for(decision=0;decision<2;++decision)for(unsigned remaining=0;remaining<2;++remaining)for(unsigned n=0;n<20;++n){
  frames=remaining!=0;pair_begin(true);operation(n,true);Trace expected[64];unsigned count=trace_count;memcpy(expected,trace,sizeof(trace));int value=observed();Fighter* h=holder->user_data,*c=caught->user_data;int motion=h->motion_id,peer_motion=c->motion_id;float timer=c->grab_timer;bool flag=h->x221B_b7;HSD_GObjEvent cb=h->accessory1_cb;pair_end();
  pair_begin(false);operation(n,false);CHECK(count==trace_count);CHECK(!memcmp(expected,trace,count*sizeof(Trace)));CHECK(value==observed());h=holder->user_data;c=caught->user_data;CHECK(motion==h->motion_id&&peer_motion==c->motion_id&&timer==c->grab_timer&&flag==h->x221B_b7);CHECK(cb==h->accessory1_cb);pair_end();++pairs;
 }}
}
static void retirement_check(void)
{
 prepare(true);decision=1;frames=false;
 // Actual native motion callbacks may retire either participant during all
 // selected transitions. Later common leaves must not run on cached data.
 const unsigned ops[]={0,6,8,9,10,11,12,13,14,15,17,18,19};
 const int motions[]={ftCo_MS_Catch,ftCo_MS_CatchPull,ftCo_MS_CatchWait,ftCo_MS_CatchWait,ftCo_MS_CatchAttack,ftCo_MS_ThrowF,ftCo_MS_CaptureWaitHi,ftCo_MS_CaptureWaitLw,ftCo_MS_CatchAttack,ftCo_MS_ThrowF,ftCo_MS_CaptureWaitLw,ftCo_MS_CaptureWaitHi,ftCo_MS_CaptureWaitHi};
 for(unsigned i=0;i<sizeof(ops)/sizeof(*ops);++i)for(unsigned mode=1;mode<=(i==0?1:3);++mode){pair_begin(false);retire_mode=mode;retire_motion=motions[i];operation(ops[i],false);CHECK(retire_mode==0);CHECK(trace_count>0);CHECK(trace[trace_count-1].event==1);pair_end();++retirements;}
 // Authored hook faults and nested motion reentry use real Execution/VM.
 for(unsigned hook=0;hook<10;++hook){pair_begin(false);integer(holder,"mode",2);NativeScriptValue result;NativeScriptValue other;unsigned count=0;
  if(hook==8){CHECK(nativeScriptReceiver_CaptureExternal(caught,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&other,&diagnostic));count=1;}
  NativeFighterLifetime saved;CHECK(nativeFighterLifetime_Capture(holder->user_data,&saved));CHECK(nativeScriptFighter_Run(holder,hook_ids[hook],count?&other:NULL,count,NULL,&result)==NativeScriptVM_Error);CHECK(!nativeFighterLifetime_Current(&saved));pair_end();++retirements;
 }
 for(unsigned mode=1;mode<=6;++mode){pair_begin(false);integer(holder,"mode",1);retire_mode=mode;retire_motion=341;uint64_t pi=target_package.program.identity,li=bank.storage_layout.identity,pa=bank.storage_parameters.identity;fn_800D9CE8(holder);CHECK(retire_mode==0);CHECK(trace_count==1&&trace[0].argument==341);target_package.program.identity=pi;bank.storage_layout.identity=li;bank.storage_parameters.identity=pa;pair_end();++retirements;}
 const unsigned fault_ops[]={0,6,10,12};
 for(unsigned i=0;i<4;++i){pair_begin(false);integer(holder,"mode",2);operation(fault_ops[i],false);CHECK(nativeScriptFighter_Status(holder)==NativeScriptVM_Error);
  if(i==0)CHECK(trace_count==1&&trace[0].event==2);
  if(i==1)CHECK(trace_count==0);
  if(i>=2)CHECK(trace_count>0&&trace[trace_count-1].event==12);
  pair_end();++retirements;
 }
}
static void mapping_check(void)
{
 prepare(false);decision=0;mapping_mode=true;definition.on_throw_victim_motion=throw_mapping;
 for(unsigned pass=0;pass<3;++pass){pair_begin(false);nativeScriptFighter_Detach(holder);reference_mode=true;((Fighter*)holder->user_data)->kind=Ft_Kind_Koopa;((Fighter*)caught->user_data)->kind=Ft_Kind_Peach;
  // Keep caught owned so the new exact pair guard is selected.
  ((Fighter*)caught->user_data)->kind=Ft_Kind_CustomFirst;CHECK(nativeScriptFighter_Attach(caught));mapping=pass==0?246:pass==1?-1:243;
  // Controlled animation family is Peach even though caught's own behavior is independent.
  ftCo_800DD4B0(holder,222);CHECK(trace[0].event==19);CHECK(((Fighter*)caught->user_data)->motion_id==(pass==1?242:mapping));pair_end();++mapping_cases;
 }
 for(unsigned mode=1;mode<=2;++mode){pair_begin(false);nativeScriptFighter_Detach(holder);reference_mode=true;((Fighter*)holder->user_data)->kind=Ft_Kind_Koopa;CHECK(nativeScriptFighter_Attach(caught));retire_mode=mode;mapping=246;ftCo_800DD4B0(holder,222);CHECK(retire_mode==0&&trace_count==1&&trace[0].event==19);pair_end();++retirements;}
 definition.on_throw_victim_motion=NULL;mapping_mode=false;
}
static void native_throw_animation(HSD_GObj* o){CHECK(o==holder);if(throw_policies)reference_observed+=100;}
static bool native_throw_finish(HSD_GObj* o){CHECK(o==holder);if(throw_policies)reference_observed+=200;return decision!=0;}
static void run_throw(unsigned which,bool reference)
{
 switch(which){case 0:(reference?reference_ftCo_ThrowF_Anim:ftCo_ThrowF_Anim)(holder);break;
 case 1:(reference?reference_ftCo_ThrowB_Anim:ftCo_ThrowB_Anim)(holder);break;
 case 2:(reference?reference_ftCo_ThrowHi_Anim:ftCo_ThrowHi_Anim)(holder);break;
 case 3:(reference?reference_ftCo_ThrowLw_Anim:ftCo_ThrowLw_Anim)(holder);break;}
}
static void interpreted_throw_policies(void)
{
 const int selections[]={-1,0,239,243,341,INT32_MAX};throw_policies=true;prepare(false);
 for(unsigned i=0;i<sizeof(selections)/sizeof(*selections);++i){
  decision=selections[i];pair_begin(false);ftCo_800DD4B0(holder,222);
  CHECK(field(holder,"observed",NULL).as.i64==222242);
  CHECK(((Fighter*)caught->user_data)->motion_id==(selections[i]>=0&&selections[i]<341?selections[i]:242));
  pair_end();++mapping_cases;
 }
 for(unsigned mode=1;mode<=3;++mode){
  prepare(false);decision=243;pair_begin(false);integer(holder,"mode",mode==3?2:1);
  if(mode<3){retire_mode=mode;retire_motion=341;}
  ftCo_800DD4B0(holder,222);
  CHECK(trace_count==(mode==3?0u:1u));if(mode<3)CHECK(!retire_mode);
  if(mode!=2)CHECK(((Fighter*)caught->user_data)->motion_id==ftCo_MS_CapturePulledHi);
  pair_end();++retirements;
 }
 // Compare each real outer throw scheduler with its pre-cohort native body,
 // using equivalent typed policy callbacks at the controlled decision boundary.
 for(unsigned present=0;present<2;++present)for(unsigned which=0;which<4;++which)
  for(unsigned remaining=0;remaining<2;++remaining)for(unsigned handled=0;handled<2;++handled){
   throw_policies=present!=0;prepare(false);decision=handled;frames=remaining!=0;
   definition.on_throw_animation=native_throw_animation;
   definition.on_throw_finish=present?native_throw_finish:NULL;
   with_hooks=true;pair_begin(true);run_throw(which,true);
   Trace expected[64];unsigned count=trace_count;memcpy(expected,trace,sizeof(trace));pair_end();
   with_hooks=false;pair_begin(false);run_throw(which,false);
   CHECK(trace_count==count&&!memcmp(expected,trace,count*sizeof(*trace)));
   CHECK(field(holder,"observed",NULL).as.i64==(present?(remaining?100:300):0));
   pair_end();++pairs;
 }
 throw_policies=true;prepare(false);frames=false;
 for(unsigned which=0;which<4;++which)for(unsigned mode=1;mode<=3;mode+=2){
  decision=1;pair_begin(false);integer(holder,"mode",mode==3?2:1);
  if(mode<3){retire_mode=mode;retire_motion=341;}
  run_throw(which,false);CHECK(trace_count==(mode==3?0u:1u));if(mode<3)CHECK(!retire_mode);
  pair_end();++retirements;
 }
 throw_policies=false;definition.on_throw_animation=NULL;definition.on_throw_finish=NULL;
}
static void native_heavy_damage(HSD_GObj* o){CHECK(o==holder);reference_observed+=400;}
static bool native_heavy_pickup(HSD_GObj* o){CHECK(o==holder);reference_observed+=300;return decision!=0;}
static void held_setup(bool has)
{
 Fighter* f=holder->user_data;f->throw_flags_b3=false;f->motion_id=ftCo_MS_HeavyGet;
 if(!has)return;
 CHECK(item_count<sizeof(items)/sizeof(*items));held_item=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(held_item);
 Item* data=&items[item_count++];*data=(Item){.entity=held_item,.kind=It_Kind_Box,.x1C=item_count};
 GObj_InitUserData(held_item,4,item_remove,data);f->item_gobj=held_item;
}
static void heavy_policy_cases(void)
{
 for(unsigned present=0;present<2;++present)for(unsigned op=0;op<2;++op)
  for(unsigned has=0;has<2;++has)for(unsigned condition=0;condition<2;++condition)for(unsigned handled=0;handled<2;++handled){
   throw_policies=present!=0;prepare(false);decision=handled;frames=op==0&&condition!=0;heavy_classification=condition!=0;
   definition.on_heavy_item_damage=present?native_heavy_damage:NULL;
   definition.on_heavy_pickup_finish=present?native_heavy_pickup:NULL;
   with_hooks=true;pair_begin(true);held_setup(has!=0);
   if(op)reference_heavy_damage_branch(holder);else reference_ftpickupitem_Anim(holder);
   Trace expected[64];unsigned count=trace_count;memcpy(expected,trace,sizeof(trace));pair_end();
   with_hooks=false;pair_begin(false);held_setup(has!=0);
   if(op)actual_heavy_damage_branch(holder);else ftpickupitem_Anim(holder);
   CHECK(trace_count==count&&!memcmp(expected,trace,count*sizeof(*trace)));
   CHECK(field(holder,"observed",NULL).as.i64==(present&&has&&(op?condition:!condition)?(op?400:300):0));
   pair_end();++pairs;
 }
 throw_policies=true;prepare(false);frames=false;heavy_classification=true;
 for(unsigned op=0;op<2;++op)for(unsigned mode=1;mode<=3;mode+=2){
  decision=1;pair_begin(false);held_setup(true);integer(holder,"mode",mode==3?2:1);
  if(mode==1){retire_mode=1;retire_motion=341;}
  if(op)actual_heavy_damage_branch(holder);else ftpickupitem_Anim(holder);
  CHECK(trace_count==(op?(mode==3?0u:1u):(mode==3?1u:2u)));if(mode==1)CHECK(!retire_mode);
  pair_end();++retirements;
 }
 throw_policies=false;definition.on_heavy_item_damage=NULL;definition.on_heavy_pickup_finish=NULL;
}
int main(void)
{
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);contracts_check();comparisons_check();retirement_check();mapping_check();interpreted_throw_policies();heavy_policy_cases();
 printf("{\"status\":\"PASS catch policy\",\"checks\":%u,\"native_pairs\":%u,\"contracts\":%u,\"retirement_cases\":%u,\"mapping_cases\":%u,\"compile_cases\":%u}\n",checks,pairs,contracts,retirements,mapping_cases,compile_cases);return 0;
}
