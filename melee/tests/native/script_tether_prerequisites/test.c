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
#include <melee/ft/native_script_binding_capture.h>
#include <melee/ft/native_script_binding_actions.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <math.h>
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
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[1024];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static HSD_JObj joints[1024];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*replacement;
static unsigned checks,fighter_count,comparisons,vm_calls,compile_cases,mutation_cases,boundary_cases;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"tether prerequisites %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
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
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<1024);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);o->hsd_obj=&joints[fighter_count];Fighter* f=&fighters[fighter_count++];
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

static unsigned trace_count,retire_mode,retire_stage;static int parasol;static bool hit,connected;
static HSD_GObj* dead;typedef struct{int event,arg;float a,b;uint32_t flags;}Trace;static Trace trace[32];
static void record(int e,int n,float a,float b){CHECK(trace_count<32);trace[trace_count++]=(Trace){e,n,a,b};}
void target_guard_probe(NativeScriptRuntimeContext* c){(void)c;}
static void integer(HSD_GObj* o,const char* n,int v){NativeScriptValue x={.type=NativeScriptType_I32,.as.i64=v};field(o,n,&x);}
static void floating(HSD_GObj* o,const char* n,float v){NativeScriptValue x={.type=NativeScriptType_F32,.as.f32=v};field(o,n,&x);}
static void prepare(void){
const char* text="fighter:\n    behavior independent\nstate int observed\nstate int mask\nstate float multiplier\nstate float snap_y\nstate float snap_height\nstate bool answer\nstate int fail\nfunc mask_call():\n    capture.mask(state.mask)\n    state.observed = state.observed + 1\nfunc damage_call():\n    state.damage_fall()\n    state.observed = state.observed + 1\nfunc probe_call():\n    state.answer = collision.probe_ledge(state.snap_y, state.snap_height)\n    state.observed = state.observed + 1\nfunc available_call():\n    state.answer = collision.ledge_available()\n    state.observed = state.observed + 1\nfunc jump_call():\n    state.cliff_jump_phase2(state.multiplier)\n    state.observed = state.observed + 1\nfunc failure():\n    state.observed = 1 / state.fail\n";
CHECK(compile_source(text));NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptItemMetadataContext metadata={0};
CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&target_package.program,&diagnostic));
target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static uint32_t helper(const char* name){for(unsigned n=0;n<bank.declarations.function_count;++n)if(nativeScriptSource_SpanEquals(&bank.tree,functions[n].name,name))return n;CHECK(false);return 0;}
static NativeScriptRuntimeContext context(HSD_GObj* o){return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.actor_id=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptVMStatus call(HSD_GObj* o,const char* name){NativeScriptValue r;++vm_calls;return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,helper(name),NULL,0,&r,&diagnostic);}
static void callback(Fighter_GObj* o){unsigned m=retire_mode;retire_mode=0;if(m==1){dead=o;HSD_GObjFree(o);}if(m==2){target_package.program.identity++;nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);}if(m==3)bank.storage_layout.identity++;if(m==4)bank.storage_parameters.identity++;if(m==5)CHECK(call(o,"failure")!=NativeScriptVM_Ok);}
void Fighter_ChangeMotionState(Fighter_GObj* o,FtMotionId id,MotionFlags flags,float frame,float speed,float blend,Fighter_GObj* peer){CHECK(!peer&&blend==0);record(1,id,frame,speed);trace[trace_count-1].flags=flags;Fighter* f=o->user_data;f->motion_id=id;f->cur_anim_frame=frame;if(retire_mode&&retire_stage==1){f->x21EC=callback;f->x21EC(o);}(void)flags;}
void ftAnim_8006EBA4(Fighter_GObj* o){record(2,0,0,0);if(retire_mode&&retire_stage==2)callback(o);}
void ftCommon_8007D5D4(Fighter* f){record(3,0,0,0);f->ground_or_air=GA_Air;}
s32 ftGetParasolStatus(Fighter_GObj* o){(void)o;record(4,parasol,0,0);return parasol;}
void ftCommon_ClampAirDrift(Fighter* f){record(5,0,0,0);if(f->self_vel.x>f->co_attrs.air_drift_max)f->self_vel.x=f->co_attrs.air_drift_max;}
void ftCommon_8007EBAC(Fighter* f,u32 n,u32 arg){record(6,n,0,0);CHECK(arg==0);(void)f;}
bool mpColl_80044164(CollData* c,int* id){record(7,0,c->ledge_snap_y,c->ledge_snap_height);if(hit)*id=19;return hit;}
bool mpColl_800443C4(CollData* c,int* id){record(7,1,c->ledge_snap_y,c->ledge_snap_height);if(hit)*id=23;return hit;}
bool mpLinesConnected(int a,int b){record(8,a,(float)b,0);return connected;}
static HSD_GObj* begin(bool owned,unsigned variant){trace_count=0;dead=NULL;retire_mode=0;HSD_GObj* o=fighter(owned,100+fighter_count);Fighter* f=o->user_data;
f->motion_id=variant&1?ftCo_MS_CliffJumpQuick1:ftCo_MS_CliffJumpSlow1;f->ground_or_air=variant&2?GA_Air:GA_Ground;f->self_vel=(Vec3){12,17,3};f->co_attrs.air_drift_max=5;f->co_attrs.ledge_jump_vertical_velocity=8;f->mv.co.cliffjump.x0=true;f->facing_dir=variant&1?-1:1;f->coll_data.ledge_snap_y=2;f->coll_data.ledge_snap_height=3;f->coll_data.ledge_id_left=19;f->coll_data.ledge_id_right=23;f->coll_data.env_flags=variant&1?Collide_RightLedgeGrab:Collide_LeftLedgeGrab;f->x1A68=37;f->x221E_b6=1;
if(owned){integer(o,"mask",(int[]){0,1,511,32767}[variant%4]);floating(o,"multiplier",(float[]){0,-1,.75f,2}[variant%4]);floating(o,"snap_y",5);floating(o,"snap_height",7);}return o;}
typedef struct{Trace t[32];unsigned n;int motion,ga,mask;Vec3 vel;CollData col;bool jump,flag,result;}Result;
static Result result(HSD_GObj* o,bool answer){Fighter* f=o->user_data;Result r={.n=trace_count,.motion=f->motion_id,.ga=f->ground_or_air,.mask=f->x1A6A,.vel=f->self_vel,.col=f->coll_data,.jump=f->mv.co.cliffjump.x0,.flag=f->x221D_b7,.result=answer};memcpy(r.t,trace,sizeof(trace));return r;}
static void comparisons_check(void){prepare();const char* names[]={"mask_call","damage_call","probe_call","available_call","jump_call"};
for(unsigned op=0;op<5;++op)for(unsigned v=0;v<8;++v){parasol=v&4?0:-1;hit=(v&2)!=0;connected=(v&4)!=0;Result old;for(unsigned pass=0;pass<2;++pass){HSD_GObj* o=begin(pass!=0,v);HSD_GObj* peer=NULL;bool answer=false;
if(op==3){peer=fighter(false,100+fighter_count);Fighter* p=peer->user_data;p->x221D_b7=(v&2)!=0;p->facing_dir=v&1?-1:1;p->mv.co.common.x0=9;}
if(pass){CHECK(call(o,names[op])==NativeScriptVM_Ok);CHECK(field(o,"observed",NULL).as.i64==1);if(op==2||op==3)answer=field(o,"answer",NULL).as.u64!=0;}
else switch(op){case 0:reference_ftCommon_8007E2F4(o->user_data,(int[]){0,1,511,32767}[v%4]);break;case 1:reference_ftCo_80090780(o);break;case 2:answer=reference_nativeFighterGeometry_ProbeLedge(o,5,7);break;case 3:answer=reference_ft_80082E3C(o)==NULL;break;case 4:reference_ftCo_8009B390(o,(float[]){0,-1,.75f,2}[v%4]);break;}
Result now=result(o,answer);CHECK(((Fighter*)o->user_data)->x1A68==37&&((Fighter*)o->user_data)->x221E_b6);if(pass){CHECK(now.n==old.n&&!memcmp(now.t,old.t,now.n*sizeof(Trace)));CHECK(now.motion==old.motion&&now.ga==old.ga&&now.mask==old.mask);CHECK(!memcmp(&now.vel,&old.vel,sizeof(Vec3)));CHECK(!memcmp(&now.col,&old.col,sizeof(CollData)));CHECK(now.jump==old.jump&&now.flag==old.flag&&now.result==old.result);++comparisons;}else old=now;
HSD_GObjFree(o);if(peer)HSD_GObjFree(peer);CHECK(nativeScriptRuntime_Idle());}}
}
static void retirement_check(void){for(unsigned path=0;path<3;++path)for(unsigned stage=1;stage<=(path==2?2:1);++stage)for(unsigned mode=1;mode<=5;++mode){prepare();parasol=path==1?0:-1;HSD_GObj* o=begin(true,0);Fighter* f=o->user_data;NativeScriptRuntimeContext c=context(o);NativeScriptValue r={.type=NativeScriptType_I32,.as.i64=997},arg={.type=NativeScriptType_F32,.as.f32=2};retire_mode=mode;retire_stage=stage;
CHECK(!nativeScriptBindings_Invoke(&c,path==2?NativeScriptBinding_StateCliffJumpPhase2:NativeScriptBinding_StateDamageFall,NULL,0,path==2?&arg:NULL,path==2?1:0,&r,&diagnostic));CHECK(r.type==NativeScriptType_I32&&r.as.i64==997);CHECK(retire_mode==0);for(unsigned n=0;n<trace_count;++n){if(trace[n].event==5||trace[n].event==6)fprintf(stderr,"path %u stage %u mode %u event %d\n",path,stage,mode,trace[n].event);CHECK(trace[n].event!=5&&trace[n].event!=6);}CHECK(f->self_vel.y==17&&f->self_vel.x==12&&f->mv.co.cliffjump.x0);if(dead!=o)HSD_GObjFree(o);CHECK(nativeScriptRuntime_Idle());++mutation_cases;}}
static void negative_check(void){const char* bad[]={"capture.mask(true)","capture.mask(1.5)","capture.mask()","collision.probe_ledge(1)","state.cliff_jump_phase2()","collision.ledge_available(1)","state.damage_fall(1)"};for(unsigned n=0;n<7;++n){char source[500];snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc wrong():\n    %s\n",bad[n]);CHECK(!compile_source(source));}prepare();
HSD_GObj* o=begin(true,0);NativeScriptRuntimeContext c=context(o);NativeScriptValue r={.type=NativeScriptType_I32,.as.i64=991},a[2]={{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_F32,.as.f32=NAN}};
for(unsigned v=0;v<2;++v){a[0].as.i64=v?32768:-1;CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_CaptureMask,NULL,0,a,1,&r,&diagnostic));CHECK(((Fighter*)o->user_data)->x1A6A==0&&r.as.i64==991);++boundary_cases;}
a[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=0};CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_CollisionProbeLedge,NULL,0,a,2,&r,&diagnostic));CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_StateCliffJumpPhase2,NULL,0,a+1,1,&r,&diagnostic));CHECK(trace_count==0);
uint32_t ids[]={NativeScriptBinding_CaptureMask,NativeScriptBinding_StateDamageFall,NativeScriptBinding_CollisionProbeLedge,NativeScriptBinding_CollisionLedgeAvailable,NativeScriptBinding_StateCliffJumpPhase2};a[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=511};a[1]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};for(unsigned n=0;n<5;++n){NativeScriptRuntimeContext wrong=c;wrong.generation++;NativeScriptValue args[2]={{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=1}};CHECK(!nativeScriptBindings_Invoke(&wrong,ids[n],NULL,0,n==0?a:args,n==0||n==4?1:n==2?2:0,&r,&diagnostic));CHECK(r.as.i64==991&&trace_count==0);++boundary_cases;}HSD_GObjFree(o);
}
int main(void){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);comparisons_check();retirement_check();negative_check();printf("{\"status\":\"PASS tether prerequisites\",\"checks\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"retirement_cases\":%u,\"boundary_cases\":%u,\"compile_cases\":%u}\n",checks,comparisons,vm_calls,mutation_cases,boundary_cases,compile_cases);return 0;}
