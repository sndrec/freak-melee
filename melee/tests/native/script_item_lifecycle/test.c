#include "boundary.h"
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_binding_item_lifecycle.h>
#include <melee/it/native_item_lifetime.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_3F14.h>
#include <math.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
static unsigned checks,cases,vm_calls,physics_calls,collision_calls,destroyed_calls,expiry_pairs;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"lifecycle %u: %s\n",__LINE__,#x);exit(1);}}while(0)
enum{N=1024,F=32,V=32};
static char source_bytes[16384];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemPrograms prepared;
static Article article;static ItemAttr attributes;static ItemModelDesc model;static NativeScriptItemAsset assets[2];
static Item items[128];static HSD_JObj joints[128];static unsigned next_item;static NativeScriptDiagnostic diagnostic;
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{for(unsigned i=0;i<prepared.count;++i)if(prepared.programs[i].author_kind==author&&prepared.programs[i].owner==owner)return &prepared.programs[i];return NULL;}
static void prepare(void)
{
 article.x0_common_attr=&attributes;article.x10_modelDesc=&model;
 assets[0]=(NativeScriptItemAsset){.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};assets[1]=assets[0];assets[1].declaration="Reentrant";
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\narticle Probe states 2:\n    state int count\n    on spawned:\n            item.state.count = 1\n    on signal expiry:\n            item.begin_expiry()\n            item.state.count = item.state.count + 10\n    on signal manual:\n            item.use_manual_lifetime()\n    on signal expire_tick:\n            if item.expire_tick(): item.state.count = item.state.count + 100\n            else: item.state.count = item.state.count + 1\n    on signal refresh:\n            item.state.count = item.state.count + 1000\n            item.refresh_physics()\n            item.state.count = item.state.count + 10000\n    on signal remove:\n            item.state.count = item.state.count + 1000\n            item.remove()\n            item.state.count = item.state.count + 10000\n    on destroyed:\n            item.state.count = item.state.count + 100\n            position.x = 3\n    state 0 anim -1:\n        on physics:\n            item.state.count = item.state.count + 1\n            position.x = 1\n        on collision:\n            item.state.count = item.state.count + 10\n            position.x = 2\n            return false\n    state 1 anim -1:\n        on physics:\n            pass\narticle Reentrant states 1:\n    state int count\n    on spawned:\n            item.state.count = 1\n    on signal remove:\n            item.remove()\n            item.state.count = 90000\n    on destroyed:\n            item.state.count = item.state.count + 100\n            item.remove()\n            item.state.count = 80000\n    state 0 anim -1:\n";
 NativeScriptItemMetadataContext metadata={.assets=assets,.asset_count=2};NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)){fprintf(stderr,"compile %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}CHECK(accepted==&bank);
 CHECK(nativeScriptItemPrograms_Prepare(&bank,Ft_Kind_CustomFirst,901,&metadata,&prepared,&diagnostic));CHECK(prepared.count==2);
 for(unsigned i=0;i<prepared.count;++i)CHECK(nativeScriptItem_ProgramValid(&prepared.programs[i]));
}
static HSD_GObj* create(unsigned program)
{
 CHECK(next_item<128);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);Item* item=&items[next_item];
 NativeScriptItemProgram* p=&prepared.programs[program];*item=(Item){.entity=object,.x1C=100+next_item,.msid=-1,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.xC4_article_data=&article,.xCC_item_attr=&attributes,.ground_or_air=GA_Air};
 GObj_InitUserData(object,6,scheduler_userdata_remove,item);object->hsd_obj=&joints[next_item++];CHECK(nativeScriptItem_Attach(object,p));CHECK(scheduler_spawned(object));scheduler_state(object,0);return object;
}
static NativeScriptItemActor* owner(HSD_GObj* object)
{for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];CHECK(false);return NULL;}
static int count(HSD_GObj* object)
{
 NativeScriptItemActor* a=owner(object);for(unsigned i=0;i<bank.declarations.value_count;++i)if(values[i].owner==a->program->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,"count")){NativeScriptValue value;if(!nativeScriptStorage_Read(&a->storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic)){fprintf(stderr,"Cannot read live source count: %s\n",diagnostic.message);exit(1);}return(int)value.as.i64;}CHECK(false);return -1;
}
static uint32_t hash(const char* text){uint32_t value=2166136261;while(*text){value^=(unsigned char)*text++;value*=16777619;}return value;}
static NativeScriptVMStatus signal(HSD_GObj* object,const char* name,NativeScriptValue* out)
{++vm_calls;return nativeScriptItem_RunSignal(object,hash(name),0,out);}
static void ok_signal(HSD_GObj* object,const char* name)
{NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(signal(object,name,&out)==NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_Void);}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Item,.generation=(uint32_t)((Item*)object->user_data)->x1C,.scene_epoch=ref.lifetime};}
enum { Healthy, Fault, Remove, Model, Data, Generation, Schema, Bank, State, Owner };
static unsigned injection, phase;
static Item replacement_data;
static HSD_JObj replacement_joint;
static NativeScriptStorageLayout replacement_layout;
bool lifecycle_observe(NativeScriptRuntimeContext* c,const NativeScriptValue* value)
{
    Item_GObj* object=c->source;Item* item=object->user_data;
    unsigned current=(unsigned)value->as.f32;
    if(current==1)++physics_calls;else if(current==2)++collision_calls;else if(current==3)++destroyed_calls;
    if(current!=phase)return true;
    switch(injection){
    case Fault:return false;
    case Remove:injection=Healthy;Item_8026A8EC(object);break;
    case Model:object->hsd_obj=&replacement_joint;break;
    case Data:replacement_data=*item;object->user_data=&replacement_data;break;
    case Generation:++item->x1C;break;
    case Schema:replacement_layout=bank.storage_layout;owner(object)->storage.layout=&replacement_layout;break;
    case Bank:bank.valid=false;break;
    case State:Item_80268E5C(object,1,0);break;
    case Owner:item->owner=object;break;
    }
    return true;
}
static void rejected(NativeScriptRuntimeContext* c,uint32_t semantic,unsigned argc)
{
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingItemLifecycle_Invoke(c,semantic,NULL,argc,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
}
static bool deferred_seen;
static void expiry_contract(Item_GObj* object)
{
 Item* item=object->user_data;ItemCommonData* common=it_804D6D28;
 const float timers[]={0.0f,-0.0f,-1.0f,0.5f,1.0f,2.0f,60.0f,INFINITY,-INFINITY,NAN};
 for(unsigned i=0;i<sizeof(timers)/sizeof(*timers);++i){
     item->xD44_lifeTimer=60.0f;item->xD48_halfLifeTimer=17.0f;
     item->xDCF_flag.b2=false;item->xDD1_flag.b0=false;item->xDC8_word.flags.x1A=true;
     item->xDC8_word.flags.x15=true;item->xDCC_flag.b3=true;
     Item expected=*item;HSD_GObj reference={.user_data=&expected};common->xF8=timers[i];
     it_8026B3A8(&reference);ok_signal(object,"manual");
     CHECK(memcmp(item,&expected,sizeof(*item))==0);
     it_8027518C(&reference);int before=count(object);ok_signal(object,"expiry");
     CHECK(count(object)==before+10&&memcmp(item,&expected,sizeof(*item))==0);
     CHECK(memcmp(&item->xD44_lifeTimer,&timers[i],sizeof(float))==0);
     CHECK(item->xD48_halfLifeTimer==17.0f&&!item->xDC8_word.flags.x15&&item->xDCC_flag.b3);
     // ExpireTick itself does not remove: its Bool controls authored policy.
     bool expired=it_802751D8(&reference);before=count(object);ok_signal(object,"expire_tick");
     CHECK(count(object)==before+(expired?100:1)&&memcmp(item,&expected,sizeof(*item))==0);
     CHECK(nativeScriptItem_Status(object)==NativeScriptVM_Ok);++expiry_pairs;++cases;
 }
 NativeScriptRuntimeContext c=context(object);Item before=*item;
 it_804D6D28=NULL;rejected(&c,NativeScriptBinding_ItemBeginExpiry,0);
 CHECK(memcmp(item,&before,sizeof(*item))==0);it_804D6D28=common;
 rejected(&c,NativeScriptBinding_ItemBeginExpiry,1);
 NativeScriptRuntimeContext invalid=c;invalid.generation++;rejected(&invalid,NativeScriptBinding_ItemBeginExpiry,0);
 invalid=c;invalid.scene_epoch++;rejected(&invalid,NativeScriptBinding_ItemBeginExpiry,0);
 invalid=c;invalid.source_class=NativeCallbackSource_Fighter;rejected(&invalid,NativeScriptBinding_ItemBeginExpiry,0);
 owner(object)->faulted=true;rejected(&c,NativeScriptBinding_ItemBeginExpiry,0);owner(object)->faulted=false;
 CHECK(memcmp(item,&before,sizeof(*item))==0);
 // Common-data failure through actual VM stops following state writes too.
 Item_GObj* failing=create(0);it_804D6D28=NULL;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(signal(failing,"expiry",&out)==NativeScriptVM_Error&&count(failing)==1);
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);it_804D6D28=common;Item_8026A8EC(failing);++cases;
 common->xF8=3.5f;ok_signal(object,"expiry");static unsigned char snapshot[2*1024*1024];size_t bytes;
 Item saved=*item;int saved_count=count(object);CHECK(nativeScriptItem_Save(snapshot,sizeof(snapshot),&bytes));
 ok_signal(object,"expire_tick");*item=saved;CHECK(nativeScriptItem_Load(snapshot,bytes));
 CHECK(count(object)==saved_count&&item->xD44_lifeTimer==3.5f);ok_signal(object,"expire_tick");CHECK(item->xD44_lifeTimer==2.5f);++cases;
}
static void deferred_proc(HSD_GObj* object)
{
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
 CHECK(signal(object,"remove",&out)==NativeScriptVM_SourceRetired);
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
 NativeMatchPoolReference reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&reference));
 CHECK(nativeScriptItem_IsAttached(object)&&nativeScriptItem_Spec(object)&&!owner(object)->storage.live);
 CHECK(((Item*)object->user_data)->native_removal_in_progress);
 int retained_count;memcpy(&retained_count,owner(object)->bytes,4);CHECK(retained_count==1101);deferred_seen=true;
}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
 Item_GObj* a=create(0);Item* item=a->user_data;
 Item_GObj* expiry=create(0);expiry_contract(expiry);Item_8026A8EC(expiry);
 item->x40_vel=(Vec3){2,3,4};item->x70_nudge=(Vec3){1,2,3};item->x58_vec_unk=(Vec3){3,4,5};item->x64_vec_unk2=(Vec3){4,5,6};
 ok_signal(a,"refresh");CHECK(count(a)==11012&&physics_calls==1&&collision_calls==1);
 CHECK(item->xDCC_flag.b3==1&&item->pos.x==2&&item->pos.y==14&&item->pos.z==18);
 CHECK(((HSD_JObj*)a->hsd_obj)->translate.x==2);++cases;
 item->xDC8_word.flags.x9=true;unsigned before_physics=physics_calls;ok_signal(a,"refresh");
 CHECK(physics_calls==before_physics&&collision_calls==2&&item->xDCC_flag.b3);item->xDC8_word.flags.x9=false;++cases;
 // Native Item state/owner transitions are mutations, not lifetime changes.
 for(unsigned action=State;action<=Owner;++action){Item_GObj* o=create(0);injection=action;phase=1;
     ok_signal(o,"refresh");CHECK(((Item*)o->user_data)->xDCC_flag.b3);CHECK(count(o)==(action==State?11002:11012));
     injection=Healthy;Item_8026A8EC(o);++cases;}
 // Full retained identity prevents physics/collision cached tails after a
 // successful callback, beyond native generation checks alone.
 for(unsigned selected_phase=1;selected_phase<=2;++selected_phase)
 for(unsigned action=Fault;action<=Bank;++action){Item_GObj* o=create(0);Item* original=o->user_data;
     NativeScriptItemActor* actor=owner(o);const NativeScriptStorageLayout* layout=actor->storage.layout;
     uint32_t gen=original->x1C;void* model=o->hsd_obj;NativeMatchPoolReference reference;
     CHECK(nativeMatchPool_Reference(&nativeGObjPool,o,&reference));injection=action;phase=selected_phase;
     unsigned pc=physics_calls,cc=collision_calls;NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
     NativeScriptVMStatus status=signal(o,"refresh",&out);CHECK(status!=NativeScriptVM_Ok);
     CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
     CHECK(physics_calls==pc+1&&collision_calls==cc+(selected_phase==2));
     CHECK(!original->xDCC_flag.b3);
     injection=Healthy;bank.valid=true;
     if(action==Remove){CHECK(nativeMatchPool_Resolve(reference)==NULL&&scheduler_destroyed==1001+(selected_phase==1?1:11)+100);}
     else {o->user_data=original;o->hsd_obj=model;original->x1C=gen;actor->storage.layout=layout;
         CHECK(count(o)==1001+(selected_phase==1?1:11));Item_8026A8EC(o);}
     ++cases;
 }
 // Packed actor data restores only when the native Item peer is restored too.
 static unsigned char packed[2*1024*1024];size_t bytes;Item saved=*item;
 int saved_count=count(a);CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));phase=0;ok_signal(a,"refresh");
 *item=saved;CHECK(nativeScriptItem_Load(packed,bytes));CHECK(count(a)==saved_count);ok_signal(a,"refresh");CHECK(count(a)==saved_count+11011);++cases;
 NativeScriptRuntimeContext c=context(a),changed=c;rejected(&c,NativeScriptBinding_ItemRefreshPhysics,1);
 rejected(&c,UINT32_MAX,0);changed.generation++;rejected(&changed,NativeScriptBinding_ItemRefreshPhysics,0);
 changed=c;changed.scene_epoch++;rejected(&changed,NativeScriptBinding_ItemRemove,0);
 changed=c;changed.source_class=NativeCallbackSource_Fighter;rejected(&changed,NativeScriptBinding_ItemRemove,0);
 owner(a)->faulted=true;rejected(&c,NativeScriptBinding_ItemRefreshPhysics,0);owner(a)->faulted=false;
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};NativeMatchPoolReference reference;
 Item_GObj* o=create(0);CHECK(nativeMatchPool_Reference(&nativeGObjPool,o,&reference));
 CHECK(signal(o,"remove",&out)==NativeScriptVM_SourceRetired);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);
 CHECK(nativeMatchPool_Resolve(reference)==NULL&&scheduler_destroyed==1101);++cases;
 // Remove while the Destroyed VM is already running still stops that VM.
 o=create(1);CHECK(nativeMatchPool_Reference(&nativeGObjPool,o,&reference));
 CHECK(signal(o,"remove",&out)==NativeScriptVM_SourceRetired);CHECK(nativeMatchPool_Resolve(reference)==NULL&&scheduler_destroyed==101);++cases;
 o=create(0);CHECK(nativeMatchPool_Reference(&nativeGObjPool,o,&reference));
 HSD_GObj_SetupProc(o,deferred_proc,0);HSD_GObj_80390CFC();CHECK(deferred_seen&&nativeMatchPool_Resolve(reference)==NULL);++cases;
 // Old retained context cannot remove a replacement at the identical address.
 c=context(a);CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&reference));Item_8026A8EC(a);o=create(0);
 CHECK(o==a&&nativeMatchPool_Resolve(reference)==NULL);rejected(&c,NativeScriptBinding_ItemRemove,0);CHECK(count(o)==1);Item_8026A8EC(o);
 CHECK(nativeScriptItem_RetireScene()&&nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS owned Item expiry, physics refresh and terminating removal\",\"checks\":%u,\"cases\":%u,\"source_vm_signals\":%u,\"physics_callbacks\":%u,\"collision_callbacks\":%u,\"destroyed_callbacks\":%u,\"native_expiry_pairs\":%u}\n",checks,cases,vm_calls,physics_calls,collision_calls,destroyed_calls,expiry_pairs);return 0;
}
