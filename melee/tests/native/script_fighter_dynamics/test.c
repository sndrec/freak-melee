#include "fixture.h"
#include "boundary.h"
#include <melee/ft/ftdynamics.h>
#include <melee/gr/ground.h>
#include <melee/gr/types.h>
#include <sysdolphin/baselib/jobj.h>

#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum { N=512,I=2048,F=32,V=32 };
typedef struct Buffers {
    char source[32768];NativeScriptSourceNode nodes[N];NativeScriptIRNode ir[I];uint32_t arguments[I];
    NativeScriptFunctionDeclaration functions[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];
    NativeScriptLocal locals[V];NativeScriptInstruction instructions[I];NativeScriptFunctionCode codes[F];
    NativeScriptExecutionFunction entries[F];NativeScriptCompileFunctionScratch graph[F];
    NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[V];NativeScriptParameterSlot slots[V];
} Buffers;
NativeSourcePackage packages[3];static Buffers buffers[3];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static Fighter fighters[512];static unsigned allocated,checks,cases,calls,observations;
unsigned dynamics_matrices;
unsigned dynamics_wind_queries;
StageInfo stage_info;
static unsigned hazard;static HSD_GObj *source,*target,*replacement;static bool mutate_caller;
static NativeScriptDiagnostic diagnostic;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"dynamics %u %s (%s)\n",__LINE__,#v,diagnostic.message);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){return kind>=Ft_Kind_CustomFirst&&kind<Ft_Kind_CustomFirst+3?&packages[kind-Ft_Kind_CustomFirst]:NULL;}
static void configure(NativeScriptCompileBank* bank,Buffers* b)
{
    *bank=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=sizeof(b->source),.tree={.nodes=b->nodes,.capacity=N},
        .expressions={.nodes=b->ir,.node_capacity=I,.arguments=b->arguments,.argument_capacity=I},
        .declarations={.functions=b->functions,.function_capacity=F,.parameters=b->parameters,.parameter_capacity=V,.values=b->values,.value_capacity=V},
        .locals=b->locals,.local_capacity=V,.instructions=b->instructions,.instruction_capacity=I,.function_code=b->codes,.function_code_capacity=F,
        .execution_functions=b->entries,.execution_function_capacity=F,.graph=b->graph,.graph_capacity=F,
        .storage_layout={.fields=b->fields,.field_capacity=V,.owners=b->owners,.owner_capacity=V},.parameter_slots=b->slots,.parameter_slot_capacity=V};
}

static void prepare(unsigned n)
{
 NativeSourcePackage* p=&packages[n];configure(&p->compiler.bank,&buffers[n]);
 const char* code="fighter:\n    behavior independent\nstate int threshold\nstate int mode\nstate int calls\nstate int last_set\nstate int last_count\nstate int zero\naction Dynamics:\n    on tick:\n        position.x = 99\n    on dynamics_interaction_start:\n        state.calls = state.calls + 1\n        state.last_set = event.set\n        state.last_count = event.set_count\n        if state.mode == 1:\n            position.x = 1\n        if state.mode == 2:\n            return 1 / state.zero\n        return state.threshold\n";
 char source_text[2048];
 if(n==1){snprintf(source_text,sizeof(source_text),"%s%s",code,
  "action GroundPolicy:\n    on dynamics_ground_collision:\n        state.calls = state.calls + 1\n        if state.mode == 3:\n            position.x = 2\n        if state.mode == 4:\n            return 1 / state.zero\n        return state.threshold != 0\n");code=source_text;}
 if(n==2)code="fighter:\n    behavior independent\nstate int calls\n";
 NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=code,.source_bytes=(uint32_t)strlen(code),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));
 CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+n,accepted,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->target=Ft_Kind_CustomFirst+n;p->ready=p->assets.ready=true;p->assets.facts.moves=layout;
 gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=p->assets.motions;
 nativeScriptFighter_PublishProgram(p->target,&p->program);
}
static void remove_data(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* create(unsigned bank,bool attached)
{
 CHECK(allocated<ARRAY_SIZE(fighters));HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 Fighter* f=&fighters[allocated++];*f=(Fighter){.gobj=o,.kind=attached?packages[bank].target:Ft_Kind_Mars,.behavior_kind=attached?packages[bank].target:Ft_Kind_Mars,.x8_spawnNum=allocated,.ft_data=attached?&packages[bank].assets.character.data:NULL};
 GObj_InitUserData(o,4,remove_data,f);if(attached)CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
 NativeScriptReceiverView v;CHECK(nativeScriptFighter_View(o,((Fighter*)o->user_data)->x8_spawnNum,&v));
 for(unsigned i=0;i<v.program->value_count;++i)if(!v.program->values[i].owner&&nativeScriptSource_SpanEquals(v.program->tree,v.program->values[i].name,name)){
  NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(v.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&result,&diagnostic));return result;
 }CHECK(false);return(NativeScriptValue){0};
}
static void integer(HSD_GObj* o,const char* name,int n){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=n};field(o,name,&v);}

typedef struct Trace {int bone,threshold,colliders;float y,scale;bool floor,ground;} Trace;
static Trace trace[16];static unsigned steps;
static HSD_JObj graphs[512][21];static struct DynamicsData chains[512][20];
static ArticleDynamicBones imported_bones;static ftDynamics imported_dynamics;
static struct FighterBone parts[512][20];
static unsigned trigger;
static void shape(HSD_GObj* o,int count)
{
 Fighter* f=o->user_data;unsigned k=(unsigned)(f-fighters);
 HSD_JObj* root=graphs[k];memset(root,0,sizeof(graphs[k]));memset(chains[k],0,sizeof(chains[k]));
 for(unsigned i=0;i<ARRAY_SIZE(graphs[k]);++i){((HSD_Class*)&root[i])->class_info=HSD_CLASS_INFO(&hsdJObj);root[i].flags=JOBJ_MTX_DIRTY;root[i].scale=(Vec3){1,1,1};}
 root->scale.x=7;o->hsd_obj=root;o->obj_kind=HSD_GObj_JObjKind;
 f->dynamics_num=count;f->x34_scale.y=1;f->cur_pos.y=19;f->ground_or_air=GA_Ground;f->parts=parts[k];
 for(int i=0;i<count;++i){HSD_JObj* a=root+1+i*2;HSD_JObj* b=a+1;a->child=b;a->parent=root;b->parent=a;
  chains[k][i*2].desc.lb_unk0.jobj=a;chains[k][i*2].next=&chains[k][i*2+1];chains[k][i*2+1].desc.lb_unk0.jobj=b;
  f->dynamic_bone_sets[i]=(BoneDynamicsDesc){.bone_id=i,.dyn_desc={.data=&chains[k][i*2],.count=2}};
  f->parts[i*2].joint=a;f->parts[i*2+1].joint=b;imported_bones.array[i].bone_id=i*2;
 }
 imported_dynamics.dynamicsNum=count;imported_dynamics.ftDynamicBones=&imported_bones;
 for(unsigned i=0;i<3;++i)packages[i].assets.character.data.x2C=&imported_dynamics;
 hazard=observations=steps=dynamics_matrices=dynamics_wind_queries=0;replacement=NULL;trigger=1;
}
void dynamics_step(DynamicsDesc* d,void* p,int n,float y,bool floor,Fighter_Part bone,int first,bool ground)
{
 (void)p;if(!d->data)return;CHECK(steps<ARRAY_SIZE(trace));
 trace[steps++]=(Trace){.bone=bone,.threshold=first,.colliders=n,.y=y,.scale=((HSD_JObj*)source->hsd_obj)->scale.x,.floor=floor,.ground=ground};
}
void dynamics_observe(HSD_GObj* o)
{
 ++observations;if(!hazard || observations!=trigger)return;
 Fighter* f=o->user_data;unsigned mode=hazard;hazard=0;
 switch(mode){
 case 1:HSD_GObjFree(o);break;
 case 2:HSD_GObjFree(o);replacement=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(replacement==o);break;
 case 3:o->user_data=NULL;break;
 case 4:++f->x8_spawnNum;break;
 case 5:o->classifier=HSD_GOBJ_CLASS_ITEM;break;
 case 6:f->ft_data=NULL;break;
 case 7:ftCo_UnloadDynamicBones(f);break;
 case 8:f->dynamic_bone_sets[0].dyn_desc.data=&chains[511][0];break;
 case 9:++f->dynamic_bone_sets[0].dyn_desc.count;break;
 case 10:f->dynamic_bone_sets[0].dyn_desc.data->next=NULL;break;
 case 11:f->dynamic_bone_sets[0].dyn_desc.data->desc.lb_unk0.jobj=&graphs[511][0];break;
 case 12:o->hsd_obj=&graphs[511][0];break;
 case 13:((HSD_Class*)o->hsd_obj)->class_info=NULL;break;
 case 14:f->native_attachment_dynamic_mask=1;break;
 case 15:++f->dynamic_bone_sets[0].bone_id;break;
 case 16:++f->x166C;break;
 case 17:scheduler_fail_write=true;{NativeScriptValue v;CHECK(nativeScriptFighter_Run(o,NativeScriptFighter_Tick,NULL,0, NULL,&v)==NativeScriptVM_Error);}scheduler_fail_write=false;break;
 default:CHECK(false);
 }
}
static void cleanup(HSD_GObj* o,Fighter* f,unsigned mode)
{
 if(mode!=1&&mode!=2){o->user_data=f;o->classifier=HSD_GOBJ_CLASS_FIGHTER;o->hsd_obj=graphs[f-fighters];HSD_GObjFree(o);}
 if(replacement)HSD_GObjFree(replacement);
}
void Reference_ftCo_8009DD94(HSD_GObj*,bool);
void Reference_ftCo_8009E7B4(Fighter*,u8 (*)[2]);
static void source_cases(void)
{
 int thresholds[]={INT32_MIN,-8,0,3,8,INT32_MAX};
 for(unsigned flat=0;flat<2;++flat)for(int count=0;count<5;++count)for(unsigned t=0;t<ARRAY_SIZE(thresholds);++t){
  source=create(0,true);Fighter* f=source->user_data;shape(source,count);stage_info.grkind=flat?Gr_Kind_Flatzone:Gr_Kind_Battle;
  integer(source,"threshold",thresholds[t]);f->x2228_b1=t&1;ftCo_8009E0A8(source);++calls;
  CHECK(steps==(unsigned)count&&field(source,"calls",NULL).as.i64==count&&!dynamics_wind_queries);
  for(unsigned i=0;i<steps;++i)CHECK(trace[i].threshold==thresholds[t]&&trace[i].bone==(int)i&&trace[i].ground&&trace[i].y==19&&trace[i].floor==(bool)(t&1)&&trace[i].scale==(flat&&count?1:7));
  if(count){CHECK(field(source,"last_set",NULL).as.i64==count-1);CHECK(field(source,"last_count",NULL).as.i64==count);}
  CHECK(((HSD_JObj*)source->hsd_obj)->scale.x==7);++cases;cleanup(source,f,0);
 }
 source=create(2,true);Fighter* f=source->user_data;shape(source,3);stage_info.grkind=Gr_Kind_Flatzone;ftCo_8009E0A8(source);++calls;
 CHECK(steps==3&&field(source,"calls",NULL).as.i64==0);for(unsigned i=0;i<steps;++i)CHECK(!trace[i].threshold);++cases;cleanup(source,f,0);
}
static void ground_policy_cases(void)
{
 for(unsigned policy=0;policy<2;++policy)for(unsigned air=0;air<2;++air)for(int count=1;count<4;++count){
  source=create(1,true);Fighter* f=source->user_data;shape(source,count);stage_info.grkind=Gr_Kind_Battle;
  f->ground_or_air=air?GA_Air:GA_Ground;integer(source,"threshold",policy);
  ftCo_8009E0A8(source);++calls;
  CHECK(steps==(unsigned)count&&field(source,"calls",NULL).as.i64==count+1);
  for(unsigned i=0;i<steps;++i)CHECK(trace[i].ground==(bool)policy);
  CHECK(((HSD_JObj*)source->hsd_obj)->scale.x==7);++cases;cleanup(source,f,0);
 }
 for(unsigned mode=1;mode<=17;++mode){
  source=create(1,true);Fighter* f=source->user_data;shape(source,3);HSD_JObj* root=source->hsd_obj;
  stage_info.grkind=Gr_Kind_Flatzone;integer(source,"mode",3);hazard=mode;trigger=1;
  ftCo_8009E0A8(source);++calls;CHECK(!hazard&&observations==1&&!steps&&!dynamics_matrices);
  bool restored=(mode>=7&&mode<=11)||(mode>=14&&mode<=17);CHECK(root->scale.x==(restored?7:1));
  ++cases;cleanup(source,f,mode);
 }
 source=create(1,true);Fighter* f=source->user_data;shape(source,3);stage_info.grkind=Gr_Kind_Flatzone;
 integer(source,"mode",4);ftCo_8009E0A8(source);++calls;
 CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_Error&&!steps&&!dynamics_matrices&&((HSD_JObj*)source->hsd_obj)->scale.x==7);
 ++cases;cleanup(source,f,0);
}
static void animation_reset_cases(void)
{
 for(unsigned mode=0;mode<3;++mode){source=create(0,true);Fighter* f=source->user_data;shape(source,3);stage_info.grkind=Gr_Kind_Battle;
  f->anim_id=0;f->x2227_b6=mode==0;f->x594_b3=mode==1;u8 blend[1][2]={{0,0}};ftCo_8009E7B4(f,blend);
  CHECK(!dynamics_wind_queries&&!steps&&field(source,"calls",NULL).as.i64==0);
  for(int i=0;i<3;++i)CHECK(f->dynamic_bone_sets[i].bone_id==(mode<2?0x100:0));
  ++cases;cleanup(source,f,0);
 }
}
static void ordinary_pairs(void)
{
 FighterKind kinds[]={Ft_Kind_Mars,Ft_Kind_Peach,Ft_Kind_Mewtwo,Ft_Kind_Koopa,Ft_Kind_GKoops};
 for(unsigned k=0;k<ARRAY_SIZE(kinds);++k)for(unsigned air=0;air<2;++air)for(unsigned flat=0;flat<2;++flat)for(unsigned scaled=0;scaled<2;++scaled)for(int count=1;count<4;++count){
  source=create(0,false);Fighter* f=source->user_data;shape(source,count);f->kind=f->behavior_kind=kinds[k];f->ground_or_air=air?GA_Air:GA_Ground;f->x34_scale.y=scaled?0.5f:1;f->motion_id=0xDF;
  stage_info.grkind=flat?Gr_Kind_Flatzone:Gr_Kind_Battle;Reference_ftCo_8009DD94(source,true);Trace expected[16];unsigned n=steps;memcpy(expected,trace,sizeof(trace));
  steps=0;for(unsigned i=0;i<ARRAY_SIZE(graphs[0]);++i)graphs[f-fighters][i].flags=JOBJ_MTX_DIRTY;
  f->x2228_b1=true;ftCo_8009E0A8(source);CHECK(n==steps&&!memcmp(expected,trace,n*sizeof(*trace)));CHECK(((HSD_JObj*)source->hsd_obj)->scale.x==7);++cases;cleanup(source,f,0);
 }
}
static void failure_cases(void)
{
 for(unsigned second=0;second<2;++second)for(unsigned mode=1;mode<=17;++mode){
  source=create(0,true);Fighter* f=source->user_data;shape(source,3);HSD_JObj* root=source->hsd_obj;
  stage_info.grkind=Gr_Kind_Flatzone;integer(source,"mode",1);hazard=mode;trigger=second+1;
  ftCo_8009E0A8(source);++calls;CHECK(!hazard&&observations==trigger&&steps==second);CHECK(!dynamics_matrices);
  bool restored=(mode>=7&&mode<=11)||(mode>=14&&mode<=17);CHECK(root->scale.x==(restored?7:1));++cases;cleanup(source,f,mode);
 }
 source=create(0,true);Fighter* f=source->user_data;shape(source,3);stage_info.grkind=Gr_Kind_Flatzone;integer(source,"mode",2);ftCo_8009E0A8(source);++calls;
 CHECK(nativeScriptFighter_Status(source)==NativeScriptVM_Error&&!steps&&!dynamics_matrices&&((HSD_JObj*)source->hsd_obj)->scale.x==7);++cases;cleanup(source,f,0);
 // Malformed initial count and descriptor chains are rejected before mutation.
 for(unsigned mode=0;mode<4;++mode){source=create(0,true);f=source->user_data;shape(source,3);stage_info.grkind=Gr_Kind_Flatzone;
  if(mode==0)f->dynamics_num=Ft_Dynamics_NumMax;else if(mode==1)f->dynamics_num=-1;else if(mode==2)f->dynamic_bone_sets[0].dyn_desc.count=3;else f->dynamic_bone_sets[0].dyn_desc.data=NULL;
  ftCo_8009E0A8(source);CHECK(!steps&&field(source,"calls",NULL).as.i64==0&&((HSD_JObj*)source->hsd_obj)->scale.x==7);++cases;cleanup(source,f,0);
 }
}
int main(void)
{
 HSD_CLASS_INFO(&hsdJObj)->head.flags=1;HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);
 prepare(0);prepare(1);prepare(2);HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
 source_cases();ordinary_pairs();failure_cases();ground_policy_cases();animation_reset_cases();CHECK(nativeScriptRuntime_Idle());
 for(unsigned i=0;i<3;++i)CHECK(!nativeScriptFighter_ReferencesProgram(&packages[i].compiler.bank));
 printf("{\"status\":\"PASS prepared dynamics\",\"checks\":%u,\"cases\":%u,\"scheduler_calls\":%u,\"ordinary_pairs\":120}\n",checks,cases,calls);return 0;
}
// Unselected setup APIs must not turn a missing fixture dependency into success.
void ftCommon_8007F76C(Fighter_GObj* o){(void)o;abort();}
void ftCommon_UnlockECB(Fighter* f){(void)f;abort();}
