#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_article_reference.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/lb/lb_00B0.h>
#include <macros.h>
#include <melee/ft/native_script_receiver_semantics.h>
#include <melee/ft/native_script_actor_access.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/class.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/types.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum {N=2048,I=8192,F=64,V=128};
static char bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration params[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptFighterProgram program;static Article assets[2];static ItemAttr attr;static ItemModelDesc model;static Fighter fighter;static Item items[128];static unsigned created,checks;static uint32_t override_id;
unsigned reference_mode;HSD_GObj* reference_caller;HSD_GObj* reference_victim;HSD_GObj*(*reference_reuse)(void);static HSD_GObj* replacement;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"signal %u: %s\n",__LINE__,#v);exit(1);}}while(0)
static uint32_t hash(const char* s){uint32_t h=2166136261u;while(*s){h^=(unsigned char)*s++;h*=16777619u;}return h;}
static void prepare(void){bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
 .declarations={.functions=functions,.function_capacity=F,.parameters=params,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,
 .function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int handle\nstate int observed\nstate int completed\nstate bool exists\nstate float x\nstate float y\nstate float z\nstate bool translated\nstate float px\nstate float py\nstate float pz\nstate float nx\nstate float ny\nstate float nz\nstate float bx\nstate float by\nstate float bz\nfunc ReadBone() -> bool:\n    return articles.read_bone_position(state.handle, Probe, 6, state.bx, state.by, state.bz)\nfunc BadIndex() -> int:\n    position.x = 1\n    return 1\naction Hooks:\n    on tick:\n        state.observed = articles.ref(state.handle, Probe).state.nums[1]\n        state.exists = articles.ref(state.handle, Probe).exists\n        state.translated = articles.read_primary_translation(state.handle, Probe, state.x, state.y, state.z)\n        state.px = articles.ref(state.handle, Probe).position.x\n        state.py = articles.ref(state.handle, Probe).position.y\n        state.pz = articles.ref(state.handle, Probe).position.z\n        state.nx = articles.ref(state.handle, Probe).surface_normal.x\n        state.ny = articles.ref(state.handle, Probe).surface_normal.y\n        state.nz = articles.ref(state.handle, Probe).surface_normal.z\n        state.completed = state.completed + 1\n    on late_update:\n        state.observed = articles.ref(state.handle, Probe).state.nums[BadIndex()]\n        state.completed = state.completed + 1\narticle Probe states 1:\n    state int nums[2]\n    state bool marked\n    state float amount\n    state 0 anim -1:\narticle Other states 1:\n    state int nums[2]\n    state 0 anim -1:\n";
 for(unsigned i=0;i<2;++i)assets[i]=(Article){.x0_common_attr=&attr,.x10_modelDesc=&model};
 NativeScriptItemAsset bindings[]={{.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&assets[0]},{.declaration="Other",.kind=It_Kind_Mario_Fire,.article=&assets[1]}};
 NativeScriptItemMetadataContext metadata={.assets=bindings,.asset_count=2};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 NativeScriptDiagnostic d;const NativeScriptCompileBank* accepted=NULL;if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,NULL,&metadata,&program,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&program);
}
static void fighter_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* caller(void){HSD_GObj* g=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);fighter=(Fighter){.gobj=g,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.x8_spawnNum=19};GObj_InitUserData(g,4,fighter_remove,&fighter);CHECK(nativeScriptFighter_Attach(g));reference_caller=g;return g;}
static void item_remove(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static HSD_GObj* item(unsigned index){CHECK(created<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[index];HSD_GObj* g=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);
 Item* i=&items[created++];*i=(Item){.entity=g,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=override_id?override_id:100+created,.xC4_article_data=(Article*)p->article};
 GObj_InitUserData(g,6,item_remove,i);CHECK(nativeScriptItem_Attach(g,p));return g;}
static HSD_GObj* reuse(void){replacement=item(0);return replacement;}
static NativeScriptValue field(HSD_GObj* g,const char* name,const NativeScriptValue* write){NativeScriptReceiverView view;uint32_t gen=g==reference_caller?19:((Item*)g->user_data)->x1C;
 CHECK(g==reference_caller?nativeScriptFighter_View(g,gen,&view):nativeScriptItem_View(g,gen,&view));NativeScriptDiagnostic d;
 for(uint32_t i=0;i<bank.declarations.value_count;++i)if(values[i].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){
  NativeScriptValue out;if(write)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,write,&d));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&d));return out;}
 CHECK(false);return(NativeScriptValue){0};}
static void sethandle(HSD_GObj* g,int32_t id){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=id};field(g,"handle",&v);}

static uint32_t declaration(const char* name,uint32_t owner){for(uint32_t i=0;i<bank.declarations.value_count;++i)if(values[i].owner==owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name))return i;CHECK(false);return 0;}
static HSD_ClassInfo class_info;HSD_JObjInfo hsdJObj;
static void model_reentry(void);
static void bone_reentry(HSD_JObj* joint);
void HSD_JObjSetupMatrixSub(HSD_JObj* joint){bone_reentry(joint);}
#include "actual_bone_position.inc"
#include "class_check.inc"
static HSD_JObj model_root,alternate_root;static HSD_GObj* queried_root;static bool replace_root;
static void model_reentry(void){if(replace_root){replace_root=false;queried_root->hsd_obj=&alternate_root;}}
static void setarray(HSD_GObj* object,uint32_t id,int32_t value){NativeScriptReceiverView view;NativeScriptDiagnostic d;uint32_t gen=((Item*)object->user_data)->x1C;
 CHECK(nativeScriptItem_View(object,gen,&view));NativeScriptValue index={.type=NativeScriptType_I32,.as.i64=1},input={.type=NativeScriptType_I32,.as.i64=value};CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|id,&index,1,&input,&d));}
static void free_item(HSD_GObj* object){object->obj_kind=HSD_GOBJ_OBJ_NONE;object->hsd_obj=NULL;HSD_GObjFree(object);}
static void frame(HSD_GObj* caller,NativeScriptReceiverScope* scope,NativeScriptRuntimeContext* context){
 *context=(NativeScriptRuntimeContext){.source=caller,.source_class=NativeCallbackSource_Fighter,.generation=19,.actor_id=19,.scene_epoch=nativeGObjPool.lifetime};
 scope->pool=nativeScriptRuntime_Receivers();NativeScriptDiagnostic d;CHECK(nativeScriptReceiver_Push(scope->pool,context,nativeGObjPool.lifetime,&bank.declarations,Ft_Kind_CustomFirst,nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope->frame,&d));}
#include "physical_cases.h"
#include "bone_cases.h"
static HSD_GObj* reuse_reference(void){replacement=item(0);return replacement;}
int main(void){prepare();HSD_GObjLibInitDataType configuration={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&configuration);HSD_GObj* f=caller(),*target=item(0),*wrong=item(1);
 uint32_t owner=nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0].owner,nums=declaration("nums",owner);
 setarray(target,nums,77);sethandle(f,((Item*)target->user_data)->x1C);
 HSD_CLASS_INFO(&hsdJObj)->head.flags=1;class_info=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj),.parent=HSD_CLASS_INFO(&hsdJObj)}};
 model_root=(HSD_JObj){.translate={2,4,6}};HSD_CLASS_METHOD(&model_root)=&class_info;target->hsd_obj=&model_root;target->obj_kind=HSD_GObj_JObjKind;((Item*)target->user_data)->pos=(Vec3){100,200,300};((Item*)target->user_data)->xDD4_itemVar.native_article.surface_normal=(Vec3){.25f,-0.0f,.75f};
 NativeScriptValue out;CHECK(nativeScriptFighter_Run(f,NativeScriptFighter_Tick,NULL,0, NULL,&out)==NativeScriptVM_Ok);CHECK(field(f,"observed",NULL).as.i64==77&&field(f,"exists",NULL).as.u64&&field(f,"translated",NULL).as.u64);CHECK(field(f,"x",NULL).as.f32==2&&field(f,"y",NULL).as.f32==4&&field(f,"z",NULL).as.f32==6);
 CHECK(field(f,"px",NULL).as.f32==100&&field(f,"py",NULL).as.f32==200&&field(f,"pz",NULL).as.f32==300);
 CHECK(field(f,"nx",NULL).as.f32==.25f&&signbit(field(f,"ny",NULL).as.f32)&&field(f,"nz",NULL).as.f32==.75f);
 NativeScriptReceiverScope scope;NativeScriptRuntimeContext c;NativeScriptDiagnostic d;frame(f,&scope,&c);
 NativeScriptValue captured;CHECK(nativeScriptReceiver_CaptureArticle(&scope,&c,((Item*)target->user_data)->x1C,owner,&captured,&d));
 physical_cases(&scope,&c,target,owner,&captured);
 bone_cases(&scope,&c,target,owner);
 NativeScriptValue selectors[]={captured,{.type=NativeScriptType_U64,.as.u64=UINT64_MAX}};
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|nums,selectors,2,&out,&d)&&out.type==NativeScriptType_I32&&!out.as.i64);
 selectors[1]=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|nums,selectors,2,&out,&d)&&out.as.i64==77);
 uint32_t marked=declaration("marked",owner),amount=declaration("amount",owner);NativeScriptValue mark={.type=NativeScriptType_Bool,.as.u64=1},number={.type=NativeScriptType_F32,.as.f32=-3.25f};field(target,"marked",&mark);field(target,"amount",&number);
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|marked,&captured,1,&out,&d)&&out.type==NativeScriptType_Bool&&out.as.u64==1);
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|amount,&captured,1,&out,&d)&&out.type==NativeScriptType_F32&&out.as.f32==-3.25f);
 for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==target)nativeScriptItemActors[i].faulted=true;
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|amount,&captured,1,&out,&d)&&out.as.f32==-3.25f);
 for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==target)nativeScriptItemActors[i].faulted=false;
 NativeScriptValue model_args[]={{.type=NativeScriptType_U32,.as.u64=((Item*)target->user_data)->x1C},{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_F32,.as.f32=11},{.type=NativeScriptType_F32,.as.f32=22},{.type=NativeScriptType_F32,.as.f32=33}};
 for(unsigned absent_model=0;absent_model<2;++absent_model){model_args[0].as.u64=absent_model?((Item*)target->user_data)->x1C:99999;if(absent_model)target->hsd_obj=NULL;
  CHECK(nativeScriptBindingArticleReference_Invoke(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION|owner,NULL,0,model_args,5,&out,&d)&&out.type==NativeScriptType_Bool&&!out.as.u64);
  CHECK(model_args[2].as.f32==11&&model_args[3].as.f32==22&&model_args[4].as.f32==33);target->hsd_obj=&model_root;}
 model_args[0].as.u64=((Item*)target->user_data)->x1C;HSD_CLASS_METHOD(&model_root)=NULL;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingArticleReference_Invoke(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION|owner,NULL,0,model_args,5,&out,&d));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&model_args[2].as.f32==11);HSD_CLASS_METHOD(&model_root)=&class_info;
 queried_root=target;replace_root=true;CHECK(!nativeScriptBindingArticleReference_Invoke(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION|owner,NULL,0,model_args,5,&out,&d));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&model_args[2].as.f32==11);target->hsd_obj=&model_root;
 HSD_GObj* foreign=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter foreign_data;foreign_data=(Fighter){.gobj=foreign,.kind=Ft_Kind_Link,.x8_spawnNum=UINT32_C(0x80000001)};foreign->user_data=&foreign_data;((Item*)target->user_data)->owner=foreign;
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_OWNER|owner,&captured,1,&out,&d)&&out.type==NativeScriptType_U32&&out.as.u64==UINT32_C(0x80000001));
 ((Item*)target->user_data)->native_removal_in_progress=true;
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|nums,selectors,2,&out,&d)&&out.as.i64==77);CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_OWNER|owner,&captured,1,&out,&d)&&!out.as.u64);((Item*)target->user_data)->native_removal_in_progress=false;
 NativeScriptValue absent;CHECK(nativeScriptReceiver_CaptureArticle(&scope,&c,((Item*)wrong->user_data)->x1C,owner,&absent,&d));selectors[0]=absent;
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|nums,selectors,2,&out,&d)&&out.as.i64==0);
 NativeScriptValue bad=absent;bad.as.object.kind=NATIVE_SCRIPT_RECEIVER_RETAINED_KIND|NativeCallbackSource_Fighter;
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|marked,&bad,1,&out,&d));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
 CHECK(!nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_OWNER|999,&absent,1,&out,&d));
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|marked,&absent,1,&out,&d)&&out.type==NativeScriptType_Bool&&!out.as.u64);
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|amount,&absent,1,&out,&d)&&out.type==NativeScriptType_F32&&out.as.f32==0);
 free_item(target);target=NULL;selectors[0]=captured;selectors[1]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1};CHECK(!nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|nums,selectors,2,&out,&d));
 CHECK(nativeScriptBindingArticleReference_Read(&scope,&c,NATIVE_SCRIPT_RECEIVER_ARTICLE_EXISTS|owner,&captured,1,&out,&d)&&!out.as.u64);CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&d));
 // Frontend must retain receiver before evaluating an invalidating index helper.
 for(unsigned mutation=1;mutation<=2;++mutation){target=item(0);setarray(target,nums,99);sethandle(f,((Item*)target->user_data)->x1C);reference_victim=target;reference_mode=mutation;reference_reuse=mutation==2?reuse_reference:NULL;
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptFighter_Run(f,NativeScriptFighter_LateUpdate,NULL,0, NULL,&out)!=NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
 if(mutation==2){CHECK(replacement==target);free_item(replacement);}HSD_GObjFree(f);f=caller();reference_mode=0;reference_reuse=NULL;}
 free_item(wrong);HSD_GObjFree(f);HSD_GObjFree(foreign);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS captured Article refs\",\"checks\":%u}\n",checks);return 0;}
