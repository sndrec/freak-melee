#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efalt.h>
#include <melee/ef/native_model_effect.h>
#include <melee/cm/camera.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/psappsrt.h>
#include "controlled_article.inc"
#include "abi/abi/expected.inc"

HSD_ObjAllocData efAsync_AllocData;
EF_EffectUsage efLib_Usage;
static EF_QueuedEffect packets[16];static bool occupied[16],allocation_failure,generator_failure;
static HSD_Generator generators[256];static HSD_psAppSRT app_srt[256];
static HSD_GObjProc proc;static HSD_GObj* active_object;
static unsigned used,allocations,releases,deliveries,generator_count,cases,pairs,vm_calls;
static int delivered_id[256];static HSD_JObj* delivered_joint[256];static float delivered_rotation[256];
static Vec3 delivered_position[256];static float delivered_scalar[256];
static NativeScriptItemModelSelector selectors[2][16];static ItemModelDesc model_desc;

void* HSD_ObjAlloc(HSD_ObjAllocData* data){CHECK(data==&efAsync_AllocData);++allocations;if(allocation_failure)return NULL;for(unsigned i=0;i<16;++i)if(!occupied[i]){occupied[i]=true;++used;memset(&packets[i],0,sizeof(packets[i]));return&packets[i];}return NULL;}
void HSD_ObjFree(HSD_ObjAllocData* data,void* p){CHECK(data==&efAsync_AllocData);for(unsigned i=0;i<16;++i)if(p==&packets[i]){CHECK(occupied[i]&&used);occupied[i]=false;--used;++releases;return;}abort();}
void HSD_ObjAllocInit(HSD_ObjAllocData* data,size_t size,u32 alignment){CHECK(data==&efAsync_AllocData&&size==sizeof(EF_QueuedEffect)&&alignment==sizeof(void*));}
HSD_Generator* hsd_8039F05C(s32 link,s32 bank_id,s32 id){CHECK(link==0&&bank_id==id/1000);if(generator_failure)return NULL;CHECK(generator_count<256);HSD_Generator* g=&generators[generator_count];memset(g,0,sizeof(*g));g->appsrt=&app_srt[generator_count++];memset(g->appsrt,0,sizeof(*g->appsrt));apply(active_object);return g;}
HSD_psAppSRT* psAddGeneratorAppSRT_begin(HSD_Generator* generator,s32 kind){(void)generator;(void)kind;abort();}
void hsd_8039D4DC(HSD_Generator* generator){(void)generator;abort();}
void nativeModelEffect_Spawn(Fighter_GObj* object,const NativeFighterModelEffect* policy,s32 part,Vec3* position,bool deferred){(void)object;(void)policy;(void)part;(void)position;(void)deferred;abort();}
void Camera_RequestQuake(CmQuakeKind id,Vec3* position){(void)id;(void)position;abort();}
// Geometry is a controlled leaf. The actual queue still chooses when to sample
// the borrowed joint; offsets are unused by the four admitted queue modes.
void lb_8000B1CC(HSD_JObj* node,Vec3* offset,Vec3* position){CHECK(node&&position&&!offset);*position=node->translate;}
EF_SpawnResult efAlt_Spawn(s32 id,HSD_GObj* object,va_list vlist_arg){CHECK(object==active_object);EF_SpawnResult spawned={0};switch(id){
#include "native_alt_cases.inc"
default:abort();}return spawned;}
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* object,...){CHECK(object==active_object);EF_SpawnResult spawned={0};va_list vlist;va_start(vlist,object);
// New position leaves copy the exact typed arguments after the actual native
// queue samples its joint. The full recipe ABI is audited by the source corpus;
// particle/model construction for these recipes is outside this fixture.
if(id==0x4CE||id==0x4CF){CHECK(deliveries<256);delivered_position[deliveries]=*va_arg(vlist,Vec3*);delivered_scalar[deliveries]=id==0x4CF?*va_arg(vlist,float*):0;apply(object);}
else if(id<0x4BA)spawned=efAlt_Spawn(id,object,vlist);else switch(id){
#include "native_sync_cases.inc"
default:abort();}va_end(vlist);CHECK(deliveries<256);delivered_id[deliveries]=id;delivered_joint[deliveries]=spawned.object.generator?spawned.object.generator->jobj:NULL;delivered_rotation[deliveries]=spawned.object.generator?spawned.object.generator->appsrt->rot.y:0;++deliveries;return spawned;}

static void reset_queue(void){CHECK(!used);allocations=releases=deliveries=generator_count=0;allocation_failure=generator_failure=false;memset(occupied,0,sizeof(occupied));proc.s_link=8;HSD_GObj_CurrentInvokedProc=NULL;efAsync_QueueInit();}
static HSD_GObj* actor(unsigned which){HSD_GObj* g=create(which);active_object=g;HSD_JObj* root=g->hsd_obj;root->child=HSD_JObjAlloc();root->child->next=HSD_JObjAlloc();root->child->child=HSD_JObjAlloc();root->child->child->next=HSD_JObjAlloc();item.facing_dir=-.75f;return g;}
static void done(HSD_GObj* g){mode=0;efAsync_QueueClear(&item.xBC0);cleanup(g);CHECK(!used);}
static const NativeScriptItemModelSelector* selector_recipe(unsigned which,unsigned queue_mode){for(unsigned i=0;i<specs[which].model_selector_count;++i)if(selectors[which][i].channel==queue_mode)return&selectors[which][i];abort();}
static bool call(HSD_GObj* g,const NativeScriptItemModelSelector* s,int32_t id,NativeScriptValue* result){NativeScriptRuntimeContext c=context(g);NativeScriptValue a[3]={{.type=NativeScriptType_I32,.as.i64=id},{.type=NativeScriptType_Text,.as.text={s->path_offset,s->path_length}},{.type=NativeScriptType_I32,.as.i64=s->channel}};NativeScriptDiagnostic d;return nativeScriptBindingArticleFeedback_Invoke(&c,s->semantic,a,3,result,&d);}
static HSD_JObj* joint(HSD_GObj* g,const NativeScriptItemModelSelector* s){HSD_JObj* n=g->hsd_obj;for(unsigned i=0;n&&i<s->depth;++i){n=n->child;for(unsigned j=0;n&&j<s->path[i];++j)n=n->next;}return n;}

static void compare(void){const int ids[]={0x4E9,0x4EA,0x4EB,0x4F1,0x510,0x48D,0x49A,0x4B0};const float facings[]={-1,0,1,NAN,-INFINITY,INFINITY};
 for(unsigned m=0;m<2;++m)for(unsigned i=0;i<8;++i){if(!m&&i>1)continue;for(unsigned f=0;f<6;++f)for(unsigned schedule=0;schedule<3;++schedule){
  reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,m?3:0);HSD_JObj* n=joint(g,s);item.facing_dir=facings[f];if(schedule){proc.s_link=schedule==1?8:9;HSD_GObj_CurrentInvokedProc=&proc;}
  NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};unsigned refs=HSD_OBJ(n)->ref_count;CHECK(call(g,s,ids[i],&result)&&result.type==NativeScriptType_Void);
  EF_QueuedEffect copy={0};if(schedule==1){CHECK(used==1&&!deliveries&&HSD_OBJ(n)->ref_count==refs);copy=*(EF_QueuedEffect*)item.xBC0;CHECK(copy.spawn_kind==s->channel&&copy.gfx_id==ids[i]&&copy.jobj==n&&!copy.native_policy);if(m)CHECK(!memcmp(&copy.params.x,&facings[f],sizeof(float)));item.facing_dir=37;efAsync_QueueFlush(g,&item.xBC0);}
  CHECK(deliveries==1&&!used&&allocations==1&&releases==1&&delivered_id[0]==ids[i]&&delivered_joint[0]==n&&HSD_OBJ(n)->ref_count==refs+1);float rotation=delivered_rotation[0];
  CHECK(i<2?rotation==0:rotation==(facings[f]<0?-(float)M_PI_2:(float)M_PI_2));
  reset_queue();item.facing_dir=facings[f];if(schedule){proc.s_link=schedule==1?8:9;HSD_GObj_CurrentInvokedProc=&proc;}
  if(m)efAsync_Spawn(g,&item.xBC0,3,ids[i],n,&item.facing_dir);else efAsync_Spawn(g,&item.xBC0,0,ids[i],n);
  if(schedule==1){EF_QueuedEffect* q=item.xBC0;CHECK(q->spawn_kind==copy.spawn_kind&&q->gfx_id==copy.gfx_id&&q->jobj==copy.jobj&&q->native_policy==copy.native_policy);if(m)CHECK(!memcmp(&q->params.x,&copy.params.x,sizeof(float)));item.facing_dir=37;efAsync_QueueFlush(g,&item.xBC0);}
  CHECK(deliveries==1&&!used&&delivered_joint[0]==n&&delivered_id[0]==ids[i]&&delivered_rotation[0]==rotation);++pairs;++cases;done(g);
 }}}
static void vm_contract(void){for(unsigned which=0;which<2;++which)for(unsigned schedule=0;schedule<2;++schedule){reset_queue();HSD_GObj* g=actor(which);value(g,"effect",0x4EB,true);NativeScriptValue out;
  if(schedule)HSD_GObj_CurrentInvokedProc=&proc;CHECK(nativeScriptItem_RunHook(g,which?NativeScriptItemHook_WorkshopTick:NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void);++vm_calls;
  value(g,"completed",1,false);if(schedule){CHECK(used==1&&!deliveries);item.facing_dir=1;efAsync_QueueFlush(g,&item.xBC0);}CHECK(deliveries==1&&delivered_joint[0]==(which?g->hsd_obj:((HSD_JObj*)g->hsd_obj)->child->child->next)&&delivered_rotation[0]==-(float)M_PI_2);done(g);++cases;}
 for(unsigned schedule=0;schedule<2;++schedule){reset_queue();HSD_GObj* g=actor(0);value(g,"effect",0x4E9,true);if(schedule)HSD_GObj_CurrentInvokedProc=&proc;NativeScriptValue out;
  CHECK(nativeScriptItem_RunHook(g,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void);++vm_calls;value(g,"completed",1,false);
  if(schedule)efAsync_QueueFlush(g,&item.xBC0);CHECK(deliveries==1&&delivered_joint[0]==g->hsd_obj&&delivered_rotation[0]==0);done(g);++cases;}
 reset_queue();HSD_GObj* g=actor(0);HSD_GObj_CurrentInvokedProc=&proc;NativeScriptValue out;const NativeScriptItemModelSelector* s=selector_recipe(0,3);
 for(unsigned i=0;i<17;++i)CHECK(call(g,s,i%2?0x4EB:0x4EA,&out));CHECK(used==16&&!deliveries&&allocations==17);efAsync_QueueFlush(g,&item.xBC0);CHECK(deliveries==16&&!used);for(unsigned i=0;i<16;++i)CHECK(delivered_id[i]==((15-i)%2?0x4EB:0x4EA));done(g);++cases;
 reset_queue();g=actor(0);HSD_GObj_CurrentInvokedProc=&proc;CHECK(call(g,s,0x4EB,&out));efAsync_QueueClear(&item.xBC0);CHECK(!used&&!deliveries&&releases==1);done(g);++cases;
 for(unsigned fail=0;fail<2;++fail){reset_queue();g=actor(0);allocation_failure=!fail;generator_failure=fail;CHECK(call(g,s,0x4EB,&out));CHECK(!used&&(fail?deliveries==1&&!delivered_joint[0]:!deliveries));done(g);++cases;}
}
static void frontend(void){const char* calls[]={"item.spawn_async_joint_effect(0x4EB, \"0\", 3.0)","item.spawn_async_joint_effect(0x4EB, \"0\", true)","item.spawn_async_joint_effect(0x4EB, \"0\", item.state.effect)","item.spawn_async_joint_effect(0x4EB, \"0\", 2)","item.spawn_async_joint_effect(0x4EB, \"256\", 3)","item.spawn_async_joint_effect(0x4EB, \"0/\", 3)","item.spawn_async_joint_effect(0x4EB, \"0/0/0/0/0/0/0/0/0/0/0/0/0\", 3)","item.spawn_async_joint_effect(0x4EB, 0, 3)","item.spawn_async_joint_effect(0x4EB)","item.spawn_async_joint_effect(true, \"0\", 3)","item.spawn_async_joint_effect(1259.0, \"0\", 3)","item.spawn_async_joint_effect(0x4CF, \"\", 4)","item.spawn_async_joint_effect(0x4CF, \"\", 1, 1)","item.spawn_async_joint_effect(0x4CF, \"\", 4, \"bad\")","item.spawn_async_joint_effect(0x4CF, \"\", 4, 1, 2)"};
 for(unsigned i=0;i<sizeof(calls)/sizeof(*calls);++i){char source[1024];snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle Probe states 1:\n    state int effect\n    on created:\n        %s\n",calls[i]);NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d;bool ok=nativeScriptCompile_Build(&bank,&request,&accepted,&d);if(ok||accepted)fprintf(stderr,"frontend %u ok%d accepted%d: %s\n",i,ok,accepted!=NULL,d.message);CHECK(!ok&&!accepted);++cases;}
 const char* source="fighter:\n    behavior independent\n    on spawn:\n        item.spawn_async_joint_effect(0x4EB, \"0\", 3)\n";NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d;CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)&&!accepted);++cases;
}
static void negatives(void){for(unsigned failure=0;failure<13;++failure){reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,3);NativeScriptRuntimeContext c=context(g);NativeScriptValue a[3]={{.type=NativeScriptType_I32,.as.i64=0x4EB},{.type=NativeScriptType_Text,.as.text={s->path_offset,s->path_length}},{.type=NativeScriptType_I32,.as.i64=3}},out={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic d;uint32_t semantic=s->semantic;uint64_t identity=bank.storage_parameters.identity;
 if(failure==0)a[0].type=NativeScriptType_F32;else if(failure==1)a[0].as.i64=INT64_C(0x1000004eb);else if(failure==2)a[1].as.text.offset++;else if(failure==3)a[2].as.i64=0;else if(failure==4)a[0].as.i64=1001;else if(failure==5)semantic=NativeScriptBinding_ItemSpawnAsyncJointEffect;
 else if(failure==6)++item.x1C;else if(failure==7)bank.valid=false;else if(failure==8)specs[0].model_selector_count=0;else if(failure==9)HSD_CLASS_METHOD(joint(g,s))=&base_class;else if(failure==10)c.source_class=NativeCallbackSource_Fighter;else if(failure==11)++c.scene_epoch;else specs[0].model_selectors=selectors[1];
 CHECK(!nativeScriptBindingArticleFeedback_Invoke(&c,semantic,a,3,&out,&d)&&out.type==NativeScriptType_U64&&out.as.u64==99&&!allocations);item.x1C=17;bank.valid=true;bank.storage_parameters.identity=identity;specs[0].model_selectors=selectors[0];specs[0].model_selector_count=5;done(g);++cases;}
 for(unsigned missing=0;missing<3;++missing){reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,3);if(missing==0)g->hsd_obj=NULL;else if(missing==1)((HSD_JObj*)g->hsd_obj)->child=NULL;NativeScriptValue out;CHECK(call(g,s,missing==2?-1:0x4EB,&out)&&out.type==NativeScriptType_Void&&!allocations);done(g);++cases;}
 for(unsigned mutation=1;mutation<=7;++mutation){reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,3);NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,g,&reference));uint64_t identity=bank.storage_parameters.identity;mode=mutation;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!call(g,s,0x4EB,&out)&&out.type==NativeScriptType_U64&&out.as.u64==99&&deliveries==1&&!used);mode=0;bank.storage_parameters.identity=identity;if(nativeMatchPool_Resolve(reference)){item.x1C=17;done(g);}++cases;}
}
static bool position_call(HSD_GObj* object,const NativeScriptItemModelSelector* selector,int32_t effect,float scalar,NativeScriptValue* output)
{
 NativeScriptRuntimeContext c=context(object);NativeScriptDiagnostic d;
 NativeScriptValue arguments[4]={{.type=NativeScriptType_I32,.as.i64=effect},
  {.type=NativeScriptType_Text,.as.text={selector->path_offset,selector->path_length}},
  {.type=NativeScriptType_I32,.as.i64=selector->channel},{.type=NativeScriptType_F32,.as.f32=scalar}};
 return nativeScriptBindingArticleFeedback_Invoke(&c,selector->semantic,arguments,selector->argument_count,output,&d);
}
static void position_modes(void)
{
 const float scalars[]={0,-0.0f,-2.5f,0.1f,1.25f};
 for(unsigned which=0;which<2;++which)for(unsigned v=0;v<5;++v)for(unsigned schedule=0;schedule<3;++schedule){
  reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,which?4:1);
  HSD_JObj* n=joint(g,s);n->translate=(Vec3){12.5f,-0.0f,-7.25f};
  if(schedule){proc.s_link=schedule==1?8:9;HSD_GObj_CurrentInvokedProc=&proc;}
  int id=which?0x4CF:0x4CE;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(position_call(g,s,id,scalars[v],&out)&&out.type==NativeScriptType_Void);
  EF_QueuedEffect copy={0};
  if(schedule==1){CHECK(used==1&&!deliveries);copy=*(EF_QueuedEffect*)item.xBC0;
   CHECK(copy.spawn_kind==s->channel&&copy.jobj==n&&copy.gfx_id==id);
   if(which)CHECK(!memcmp(&copy.params.x,&scalars[v],sizeof(float)));
   n->translate=(Vec3){91,-4,3};efAsync_QueueFlush(g,&item.xBC0);
  }
  CHECK(deliveries==1&&!used&&allocations==1&&releases==1&&delivered_id[0]==id);
  CHECK(!memcmp(&delivered_position[0],&n->translate,sizeof(Vec3)));
  if(which)CHECK(!memcmp(&delivered_scalar[0],&scalars[v],sizeof(float)));
  Vec3 expected_position=delivered_position[0];float expected_scalar=delivered_scalar[0];
  reset_queue();n->translate=(Vec3){12.5f,-0.0f,-7.25f};
  if(schedule){proc.s_link=schedule==1?8:9;HSD_GObj_CurrentInvokedProc=&proc;}
  float native_scalar=scalars[v];
  if(which)efAsync_Spawn(g,&item.xBC0,4,id,n,&native_scalar);else efAsync_Spawn(g,&item.xBC0,1,id,n);
  if(schedule==1){CHECK(used==1);EF_QueuedEffect* q=item.xBC0;
   CHECK(q->spawn_kind==copy.spawn_kind&&q->gfx_id==copy.gfx_id&&q->jobj==copy.jobj);
   native_scalar=77;n->translate=(Vec3){91,-4,3};efAsync_QueueFlush(g,&item.xBC0);
  }
  CHECK(deliveries==1&&!used&&!memcmp(&expected_position,&delivered_position[0],sizeof(Vec3)));
  if(which)CHECK(!memcmp(&expected_scalar,&delivered_scalar[0],sizeof(float)));
  done(g);++pairs;++cases;
 }
 for(unsigned which=0;which<2;++which)for(unsigned schedule=0;schedule<2;++schedule){
  reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,which?4:1);
  HSD_JObj* n=joint(g,s);n->translate=(Vec3){1,2,3};value(g,"effect",which?0x4CF:0x4CE,true);value(g,"scale_raw",5,true);
  if(schedule)HSD_GObj_CurrentInvokedProc=&proc;NativeScriptValue out;
  CHECK(nativeScriptItem_RunHook(g,which?NativeScriptItemHook_StageLeft:NativeScriptItemHook_StageLanded,NULL,0,&out)==NativeScriptVM_Ok);++vm_calls;
  value(g,"completed",1,false);
  if(schedule){CHECK(used==1&&!deliveries);n->translate=(Vec3){7,8,9};value(g,"scale_raw",99,true);efAsync_QueueFlush(g,&item.xBC0);}
  CHECK(deliveries==1&&!memcmp(&delivered_position[0],&n->translate,sizeof(Vec3)));
  if(which)CHECK(delivered_scalar[0]==1.25f);done(g);++cases;
 }
 for(unsigned which=0;which<2;++which)for(unsigned failure=0;failure<7;++failure){
  if(!which&&(failure==3||failure==4))continue;
  reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,which?4:1);
  int id=failure==0?0x4E9:failure==1?(which?0x447:0x4CC):failure==2?(which?0x4CE:0x4CF):which?0x4CF:0x4CE;
  float scalar=failure==3?NAN:failure==4?INFINITY:1;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  if(failure<3||(which&&failure<5))CHECK(!position_call(g,s,id,scalar,&out)&&!allocations&&out.as.u64==99);
  else if(failure==5){allocation_failure=true;CHECK(position_call(g,s,id,scalar,&out)&&!used&&!deliveries&&out.type==NativeScriptType_Void);}
  else if(failure==6){((HSD_JObj*)g->hsd_obj)->child=NULL;g->hsd_obj=NULL;CHECK(position_call(g,s,id,scalar,&out)&&!allocations);}
  done(g);++cases;
 }
 for(unsigned which=0;which<2;++which)for(unsigned mutation=1;mutation<=7;++mutation){
  reset_queue();HSD_GObj* g=actor(0);const NativeScriptItemModelSelector* s=selector_recipe(0,which?4:1);
  NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,g,&reference));uint64_t identity=bank.storage_parameters.identity;
  mode=mutation;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(!position_call(g,s,which?0x4CF:0x4CE,1,&out)&&out.type==NativeScriptType_U64&&out.as.u64==99&&deliveries==1&&!used);
  mode=0;bank.storage_parameters.identity=identity;if(nativeMatchPool_Resolve(reference)){item.x1C=17;done(g);}++cases;
 }
}
int main(void){prepare();article.x10_modelDesc=&model_desc;for(unsigned i=0;i<2;++i){NativeScriptDiagnostic d;CHECK(nativeScriptBindingItemModel_PrepareSelectors(&bank,&specs[i],selectors[i],16,&d));}CHECK(specs[0].model_selector_count==5&&specs[1].model_selector_count==1);
 for(int id=0;id<=UINT16_MAX;++id)CHECK(nativeScriptEffectDispatch_Arguments(id)==expected[id]);
 HSD_GObjLibInitDataType configuration={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&configuration);compare();vm_contract();negatives();position_modes();frontend();CHECK(nativeScriptRuntime_Idle()&&!nativeScriptItem_HasActors(Ft_Kind_CustomFirst));printf("{\"status\":\"PASS Item async joint effect\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"actor_vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;}
