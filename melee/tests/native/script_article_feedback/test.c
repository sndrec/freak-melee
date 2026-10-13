#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_script_binding_article_feedback.h>
#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_effect_dispatch.h>
#include <melee/ft/native_script_binding_item_model_resolve.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_holdable.h>
#include <melee/it/types.h>
#include <melee/ef/types.h>
#include <melee/ef/inlines.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#pragma clang section data = ""
enum {N=512,I=2048,F=16,V=32};
static char text[8192];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode expressions[I];static uint32_t args[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration fields[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction execution[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField layout[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemProgram specs[2];static NativeScriptItemState states[2];static NativeItemDefinition recipe;static Article article;
static NativeScriptItemModelSelector model_selectors[2][8];static ItemModelDesc model_descriptor;
static Item item;static HSD_JObj roots[2048];static unsigned allocated,checks,spawns,walks,mode;static HSD_JObj* spawned_root;static int spawned_id;static Vec3 spawned_position;static float spawned_scalar;
static HSD_ClassInfo base_class;HSD_JObjInfo hsdJObj;static HSD_ClassInfo* default_class;
static void remove_data(void*);
static HSD_Class* allocate(HSD_ClassInfo* info){if(allocated==2048)abort();if(info->head.obj_size!=(int)sizeof(HSD_JObj))abort();return(HSD_Class*)&roots[allocated++];}
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"article feedback %u: %s\n",__LINE__,#v);exit(1);}}while(0)
static void destroy_class(HSD_Class* object){(void)object;abort();}
static void apply(HSD_GObj* object){
 if(!mode)return;Item* native=object->user_data;
 switch(mode){case 1:++native->x1C;break;case 2:nativeScriptItem_RunHook(object,NativeScriptItemHook_Destroyed,NULL,0,NULL);break;
 case 3:for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].object==object&&nativeScriptItemActors[i].active)nativeScriptItemActors[i].faulted=true;break;
 case 4:object->hsd_obj=&roots[255];break;case 5:++bank.storage_parameters.identity;break;
 case 6:object->obj_kind=HSD_GOBJ_OBJ_NONE;object->hsd_obj=NULL;HSD_GObjFree(object);break;
 case 7:((HSD_Class*)object->hsd_obj)->class_info=&base_class;break;default:abort();}
}
EF_SpawnResult efSync_Spawn(s32 id,HSD_GObj* object,...){va_list list;va_start(list,object);NativeScriptEffectArguments abi=nativeScriptEffectDispatch_Arguments(id);if(abi==NativeScriptEffectArguments_Position||abi==NativeScriptEffectArguments_PositionScalar){spawned_position=*va_arg(list,Vec3*);spawned_root=NULL;if(abi==NativeScriptEffectArguments_PositionScalar)spawned_scalar=*va_arg(list,float*);}else spawned_root=va_arg(list,HSD_JObj*);va_end(list);++spawns;spawned_id=id;apply(object);return(EF_SpawnResult){0};}
EF_ParamEntry efLib_ParamTable[8];
void HSD_JObjWalkTree(HSD_JObj* root,HSD_JObjWalkTreeCallback callback,f32** argument){(void)callback;(void)argument;CHECK(root);++walks;}
void hsd_8039D688(HSD_JObj* root,f32** argument,s32 flags){(void)root;(void)argument;(void)flags;abort();}
void nativeModelEffect_Cancel(EF_Effect* effect){apply(effect->parent_gobj);effect->native_cancelled=true;}
#include "native_bodies.inc"
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner){for(unsigned i=0;i<2;++i)if(specs[i].bank&&specs[i].author_kind==author&&specs[i].owner==owner)return&specs[i];return NULL;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* diagnostic){
 (void)unused;const NativeScriptSourceNode* n=&tree->nodes[node];
 if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 1");
 if(n->kind==NativeScriptSource_Item)return nativeScriptSource_SpanEquals(tree,n->value,"damage 4 angle 45 lifetime 90");
 if(n->kind==NativeScriptSource_State||n->kind==NativeScriptSource_Function)return n->parent<tree->count&&(tree->nodes[n->parent].kind==NativeScriptSource_Article||tree->nodes[n->parent].kind==NativeScriptSource_Item);
 if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,n,&e,diagnostic);}
 return nativeScriptCompile_MinimalMetadata(NULL,tree,node,diagnostic);
}
static void prepare(void){
 bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=expressions,.node_capacity=I,.arguments=args,.argument_capacity=I},
 .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=fields,.value_capacity=V},.locals=locals,.local_capacity=V,
 .instructions=instructions,.instruction_capacity=I,.function_code=code,.function_code_capacity=F,.execution_functions=execution,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
 .storage_layout={.fields=layout,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\narticle Probe states 1:\n    state int effect\n    state int completed\n    on spawned:\n        item.spawn_root_effect(item.state.effect)\n        item.state.completed = item.state.completed + 1\n    on created:\n        item.spawn_effect_at_position(item.state.effect, 12.5, -4.0, 7.0)\n        item.state.completed = item.state.completed + 1\n    on reflected:\n        item.spawn_with_scalar_at_position(item.state.effect, 12.5, -4.0, 7.0, -0.75)\n        item.state.completed = item.state.completed + 1\n        return false\n    on shield_bounced:\n        item.spawn_joint_effect(item.state.effect, \"0/0\")\n        item.state.completed = item.state.completed + 1\n        return false\n    on accessory:\n        item.destroy_effects()\n        item.state.completed = item.state.completed + 1\nitem Snack damage 4 angle 45 lifetime 90:\n    state int effect\n    state int completed\n    on tick:\n        item.spawn_root_effect(item.state.effect)\n        item.state.completed = item.state.completed + 1\n";
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};const NativeScriptCompileBank* accepted=NULL;NativeScriptDiagnostic d;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)){fprintf(stderr,"compile %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 article.x10_modelDesc=&model_descriptor;
 unsigned count=0;for(unsigned i=1;i<bank.tree.count;++i)if(nodes[i].kind==NativeScriptSource_Article||nodes[i].kind==NativeScriptSource_Item){
  NativeScriptItemProgram* p=&specs[count];*p=(NativeScriptItemProgram){.bank=&bank,.identity=123+count,.author_kind=Ft_Kind_CustomFirst,.owner=i,.kind=count?It_Kind_NativeHoldable:It_Kind_Mario_Fire,.recipe_index=count?0:UINT32_MAX,.recipe=count?&recipe:NULL,.article=&article,.states=&states[count],.state_count=1};
  states[count].animation_id=-1;for(unsigned phase=0;phase<3;++phase)states[count].functions[phase]=UINT32_MAX;
  for(unsigned h=0;h<NativeScriptItemHook_Count;++h)p->hooks[h]=UINT32_MAX;++count;
 }
 CHECK(count==2);for(unsigned i=0;i<2;++i){
  CHECK(nativeScriptBindingItemModel_PrepareSelectors(&bank,&specs[i],model_selectors[i],8,&d));
 }
 for(unsigned i=0;i<bank.declarations.function_count;++i){NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&nodes[functions[i].source_node],&e,&d));
  for(unsigned s=0;s<2;++s)if(functions[i].owner==specs[s].owner)specs[s].hooks[s?NativeScriptItemHook_WorkshopTick:!strcmp(e.descriptor->name,"spawned")?NativeScriptItemHook_Spawned:!strcmp(e.descriptor->name,"created")?NativeScriptItemHook_Created:!strcmp(e.descriptor->name,"reflected")?NativeScriptItemHook_Reflected:!strcmp(e.descriptor->name,"shield_bounced")?NativeScriptItemHook_ShieldBounced:NativeScriptItemHook_Accessory]=i;
 }
 base_class=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_Obj)},.init=_hsdClassInit};
 HSD_ClassInfo* info=HSD_CLASS_INFO(&hsdJObj);*info=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj),.parent=&base_class},.alloc=allocate,.init=JObjInit,.destroy=destroy_class};
}
static HSD_GObj* create(unsigned which){mode=0;HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);item=(Item){.entity=object,.kind=specs[which].kind,.x1C=17,.native_script_owner=specs[which].owner,.native_article_author_kind=specs[which].author_kind,.xC4_article_data=&article};
 if(which)item.xDD4_itemVar.native_holdable.owner_kind=specs[which].author_kind;
 object->hsd_obj=HSD_JObjAlloc();object->obj_kind=HSD_GObj_JObjKind;GObj_InitUserData(object,6,remove_data,&item);CHECK(nativeScriptItem_Attach(object,&specs[which]));return object;}
static void remove_data(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static void cleanup(HSD_GObj* object){if(nativeScriptItem_Status(object)==NativeScriptVM_SourceRetired&&!object->user_data)return;object->obj_kind=HSD_GOBJ_OBJ_NONE;object->hsd_obj=NULL;HSD_GObjFree(object);}
static void value(HSD_GObj* object,const char* name,int32_t input,bool write){NativeScriptReceiverView view;NativeScriptDiagnostic d;CHECK(nativeScriptItem_View(object,17,&view));
 for(unsigned i=0;i<bank.declarations.value_count;++i)if(fields[i].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,fields[i].name,name)){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=input};
  if(write)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&v,&d));else CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&v,&d)&&v.as.i64==input);return;}CHECK(false);}
static NativeScriptRuntimeContext context(HSD_GObj* object){return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Item,.generation=17,.scene_epoch=nativeGObjPool.lifetime};}
#include "scalar_cases.h"
#include "joint_effect_cases.h"
int main(void){prepare();HSD_GObjLibInitDataType configuration={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&configuration);
 for(unsigned which=0;which<2;++which)for(int id=1272;id<=1274;++id){HSD_GObj* object=create(which);value(object,"effect",id,true);NativeScriptValue out;
  unsigned before=spawns;CHECK(nativeScriptItem_RunHook(object,which?NativeScriptItemHook_WorkshopTick:NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void);
  CHECK(spawns==before+1&&spawned_id==id&&spawned_root==object->hsd_obj&&item.owner==NULL);value(object,"completed",1,false);cleanup(object);}
 const NativeScriptValue rejected[]={ {.type=NativeScriptType_I64,.as.i64=-1},{.type=NativeScriptType_U64,.as.u64=UINT64_MAX},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x1000004f8)},
 {.type=NativeScriptType_I32,.as.i64=1146},{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_I32,.as.i64=1001},{.type=NativeScriptType_F32,.as.f32=1272} };
 for(unsigned i=0;i<sizeof(rejected)/sizeof(*rejected);++i){HSD_GObj* object=create(0);NativeScriptRuntimeContext c=context(object);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic d;unsigned before=spawns;
  CHECK(!nativeScriptBindingArticleFeedback_Invoke(&c,NativeScriptBinding_ItemSpawnRootEffect,&rejected[i],1,&out,&d));CHECK(spawns==before&&out.type==NativeScriptType_U64&&out.as.u64==99);cleanup(object);}
 for(unsigned mutation=1;mutation<=7;++mutation){HSD_GObj* object=create(0);value(object,"effect",1272,true);NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
  uint64_t identity=bank.storage_parameters.identity;mode=mutation;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_Spawned,NULL,0,&out)!=NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
  bank.storage_parameters.identity=identity;mode=0;if(nativeMatchPool_Resolve(reference)){if(mutation==1)item.x1C=17;cleanup(object);}}
 // Actual destruction body clears parameter references and calls controlled
 // particle tree leaves; ordinary cleanup needs no creator or retained target.
 HSD_GObj* clear=create(0);CHECK(nativeScriptItem_BindCallback(clear,NativeScriptItemCallback_Accessory,specs[0].hooks[NativeScriptItemHook_Accessory]));
 efLib_ParamTable[0].gobj=clear;unsigned before_walks=walks;NativeScriptValue result;
 CHECK(nativeScriptItem_RunHook(clear,NativeScriptItemHook_Accessory,NULL,0,&result)==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);
 CHECK(!efLib_ParamTable[0].gobj&&walks==before_walks+1);value(clear,"completed",1,false);cleanup(clear);
 for(unsigned list=0;list<2;++list)for(unsigned mutation=0;mutation<=7;++mutation){
  clear=create(0);CHECK(nativeScriptItem_BindCallback(clear,NativeScriptItemCallback_Accessory,specs[0].hooks[NativeScriptItemHook_Accessory]));
  HSD_GObj* child=GObj_Create(0,list?HSD_GOBJ_PLINK_EFFECT_UNK1:HSD_GOBJ_PLINK_EFFECT_UNK0,0);CHECK(child);
  EF_Effect effect={.gobj=child,.parent_gobj=clear,.native_policy=(const struct NativeFighterModelEffect*)&article};
  child->user_data=&effect;NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,clear,&ref));
  uint64_t identity=bank.storage_parameters.identity;mode=mutation;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
  NativeScriptVMStatus status=nativeScriptItem_RunHook(clear,NativeScriptItemHook_Accessory,NULL,0,&result);
  if(mutation){CHECK(status!=NativeScriptVM_Ok&&result.type==NativeScriptType_U64&&result.as.u64==99);}
  else {CHECK(status==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);value(clear,"completed",1,false);}
  CHECK(effect.native_cancelled);mode=0;bank.storage_parameters.identity=identity;child->user_data=NULL;HSD_GObjFree(child);
  if(nativeMatchPool_Resolve(ref)){if(mutation==1)item.x1C=17;cleanup(clear);}
 }
 // Root metadata is an engine-typed extent proof, not arbitrary allocation validation.
 for(unsigned failure=0;failure<4;++failure){clear=create(0);NativeScriptRuntimeContext c=context(clear);NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=1272};
  result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};unsigned before=spawns;
  if(failure==0)clear->hsd_obj=NULL;else if(failure==1)clear->obj_kind=HSD_GOBJ_OBJ_NONE;
  else if(failure==2)HSD_CLASS_METHOD(clear->hsd_obj)=&base_class;else HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_Obj);
  NativeScriptDiagnostic d;CHECK(!nativeScriptBindingArticleFeedback_Invoke(&c,NativeScriptBinding_ItemSpawnRootEffect,&id,1,&result,&d));
  CHECK(spawns==before&&result.type==NativeScriptType_U64&&result.as.u64==99);HSD_CLASS_INFO(&hsdJObj)->head.obj_size=sizeof(HSD_JObj);cleanup(clear);
 }
 // Every native Position ABI is admitted according to its audited context.
 for(int id=0;id<=UINT16_MAX;++id){uint32_t requirements;
  if(!nativeScriptEffectDispatch_PositionRequirements(id,&requirements))continue;
  for(unsigned no_root=0;no_root<2;++no_root){HSD_GObj* object=create(0);
   if(no_root){object->hsd_obj=NULL;object->obj_kind=HSD_GOBJ_OBJ_NONE;}
   value(object,"effect",id,true);unsigned before=spawns;NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
   bool admissible=!(requirements&NativeScriptEffectPosition_FighterParent)&&(!no_root||!(requirements&NativeScriptEffectPosition_Root));
   NativeScriptVMStatus status=nativeScriptItem_RunHook(object,NativeScriptItemHook_Created,NULL,0,&result);
   if(admissible){CHECK(status==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);CHECK(spawns==before+1&&spawned_id==id&&!spawned_root);
    CHECK(spawned_position.x==12.5f&&spawned_position.y==-4.0f&&spawned_position.z==7.0f);value(object,"completed",1,false);
   }else CHECK(status!=NativeScriptVM_Ok&&result.type==NativeScriptType_U64&&result.as.u64==99&&spawns==before);
   cleanup(object);
  }
 }
 // Position effects retain the same actor and owned storage through native calls.
 for(unsigned mutation=1;mutation<=7;++mutation){HSD_GObj* object=create(0);value(object,"effect",1040,true);
  NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));uint64_t identity=bank.storage_parameters.identity;
  mode=mutation;NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};
  NativeScriptVMStatus status=nativeScriptItem_RunHook(object,NativeScriptItemHook_Created,NULL,0,&result);
  // Root-independent effects do not dereference root class metadata.
  if(mutation==7){CHECK(status==NativeScriptVM_Ok&&result.type==NativeScriptType_Void);value(object,"completed",1,false);}
  else CHECK(status!=NativeScriptVM_Ok&&result.type==NativeScriptType_U64&&result.as.u64==99);
  mode=0;bank.storage_parameters.identity=identity;if(nativeMatchPool_Resolve(ref)){if(mutation==1)item.x1C=17;cleanup(object);}
 }
 // Bad ABI, integer width and non-finite coordinates reject without native effects.
 for(unsigned failure=0;failure<7;++failure){HSD_GObj* object=create(0);NativeScriptRuntimeContext c=context(object);
  NativeScriptValue a[4]={{.type=NativeScriptType_I32,.as.i64=1040},{.type=NativeScriptType_F32,.as.f32=1},{.type=NativeScriptType_F32,.as.f32=2},{.type=NativeScriptType_F32,.as.f32=3}};
  if(failure==0)a[0]=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000410)};
  else if(failure==1)a[0].as.i64=-1;else if(failure==2)a[0].as.i64=1272;
  else if(failure==3)a[0]=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1040};
  else a[1+failure-4].as.f32=NAN;
  NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic d;unsigned before=spawns;
  CHECK(!nativeScriptBindingArticleFeedback_Invoke(&c,NativeScriptBinding_ItemSpawnEffectAtPosition,a,4,&result,&d));
  CHECK(spawns==before&&result.type==NativeScriptType_U64&&result.as.u64==99);cleanup(object);
 }
 scalar_cases();
 joint_effect_cases();
 CHECK(nativeScriptRuntime_Idle()&&!nativeScriptItem_HasActors(Ft_Kind_CustomFirst));printf("{\"status\":\"PASS owned Article feedback\",\"checks\":%u,\"spawns\":%u}\n",checks,spawns);return 0;}
