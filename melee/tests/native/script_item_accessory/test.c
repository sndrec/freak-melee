#include "boundary.h"
#include <melee/ft/native_script_item_program.h>
#include <melee/ft/native_script_binding_item.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
static unsigned checks,cases,vm_calls;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"accessory %u: %s\n",__LINE__,#x);exit(1);}}while(0)
enum{N=1024,F=32,V=32};
static char source_bytes[16384];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemPrograms prepared;
static Article article;static ItemAttr attributes;static ItemModelDesc model;static NativeScriptItemAsset assets[2];
static Item items[32];static HSD_JObj joints[32];static unsigned next_item;static NativeScriptDiagnostic diagnostic;
static unsigned native_calls;extern unsigned accessory_tail_calls;
static void native_accessory(HSD_GObj* object){(void)object;++native_calls;}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{for(unsigned i=0;i<prepared.count;++i)if(prepared.programs[i].author_kind==author&&prepared.programs[i].owner==owner)return &prepared.programs[i];return NULL;}
static void prepare(void)
{
 article.x0_common_attr=&attributes;article.x10_modelDesc=&model;
 assets[0]=(NativeScriptItemAsset){.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};assets[1]=assets[0];assets[1].declaration="Empty";
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* text="fighter:\n    behavior independent\narticle Probe states 2:\n    state int count\n    func Track():\n        item.track_accessory()\n    func Clear():\n        item.clear_accessory()\n    func Helper():\n        item.state.count = item.state.count + 7\n    on spawned:\n            item.state.count = 1\n    on created:\n            pass\n    on signal activate:\n            Track()\n    on signal clear:\n            Clear()\n    on accessory:\n            item.state.count = item.state.count + 3\n            position.x = item.state.count\n    on touched:\n            return false\n    state 0 anim -1:\n    state 1 anim -1:\narticle Empty states 1:\n    on signal activate:\n            item.track_accessory()\n    on signal clear:\n            item.clear_accessory()\n    state 0 anim -1:\n";
 NativeScriptItemMetadataContext metadata={.assets=assets,.asset_count=2};NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)){fprintf(stderr,"compile %u:%u %s\n",diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}CHECK(accepted==&bank);
 CHECK(nativeScriptItemPrograms_Prepare(&bank,Ft_Kind_CustomFirst,901,&metadata,&prepared,&diagnostic));CHECK(prepared.count==2);
 for(unsigned i=0;i<prepared.count;++i)CHECK(nativeScriptItem_ProgramValid(&prepared.programs[i]));
}
static HSD_GObj* create(unsigned program)
{
 CHECK(next_item<32);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);Item* item=&items[next_item];
 NativeScriptItemProgram* p=&prepared.programs[program];*item=(Item){.entity=object,.x1C=100+next_item,.msid=-1,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.xC4_article_data=&article,.xCC_item_attr=&attributes,.ground_or_air=GA_Air};
 GObj_InitUserData(object,6,scheduler_userdata_remove,item);object->hsd_obj=&joints[next_item++];CHECK(nativeScriptItem_Attach(object,p));CHECK(scheduler_spawned(object));scheduler_state(object,0);return object;
}
static NativeScriptItemActor* owner(HSD_GObj* object)
{for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];CHECK(false);return NULL;}
static int count(HSD_GObj* object)
{
 NativeScriptItemActor* a=owner(object);for(unsigned i=0;i<bank.declarations.value_count;++i)if(values[i].owner==a->program->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,"count")){NativeScriptValue value;CHECK(nativeScriptStorage_Read(&a->storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&value,&diagnostic));return(int)value.as.i64;}CHECK(false);return -1;
}
static uint32_t hash(const char* text){uint32_t value=2166136261;while(*text){value^=(unsigned char)*text++;value*=16777619;}return value;}
static NativeScriptVMStatus signal(HSD_GObj* object,const char* name,NativeScriptValue* out)
{++vm_calls;return nativeScriptItem_RunSignal(object,hash(name),0,out);}
static void ok_signal(HSD_GObj* object,const char* name)
{NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(signal(object,name,&out)==NativeScriptVM_Ok);CHECK(out.type==NativeScriptType_Void);}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,object,&ref));return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Item,.generation=(uint32_t)((Item*)object->user_data)->x1C,.scene_epoch=ref.lifetime};}
static void rejected(NativeScriptRuntimeContext* c)
{NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingItem_Invoke(c,NativeScriptBinding_ItemTrackAccessory,NULL,0,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;}
static uint32_t helper(uint32_t owner_id,const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(declarations[i].owner==owner_id&&nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;CHECK(false);return UINT32_MAX;}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
 HSD_GObj* a=create(0);Item* item=a->user_data;CHECK(count(a)==1&&!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));scheduler_accessory(a);CHECK(count(a)==1);++cases;
 ok_signal(a,"activate");CHECK(nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory)&&item->on_accessory==NULL);scheduler_accessory(a);CHECK(count(a)==4&&item->pos.x==4);++cases;
 item->xDC8_word.flags.x9=true;scheduler_accessory(a);CHECK(count(a)==4);item->xDC8_word.flags.x9=false;scheduler_accessory(a);CHECK(count(a)==7);++cases;
 ok_signal(a,"clear");CHECK(!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));scheduler_accessory(a);CHECK(count(a)==7);++cases;
 ok_signal(a,"activate");scheduler_state(a,1);CHECK(!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));scheduler_accessory(a);CHECK(count(a)==7);ok_signal(a,"activate");scheduler_accessory(a);CHECK(count(a)==10);++cases;
 item->on_accessory=native_accessory;scheduler_accessory(a);CHECK(native_calls==1&&!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory)&&count(a)==10);ok_signal(a,"activate");CHECK(item->on_accessory==NULL);scheduler_accessory(a);CHECK(count(a)==13&&native_calls==1);++cases;
 item->on_accessory=native_accessory;ok_signal(a,"clear");CHECK(item->on_accessory==NULL&&!nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));++cases;
 ok_signal(a,"activate");static unsigned char packed[2*1024*1024];size_t bytes;CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));ok_signal(a,"clear");CHECK(nativeScriptItem_Load(packed,bytes));CHECK(nativeScriptItem_HookBound(a,NativeScriptItemHook_Accessory));scheduler_accessory(a);CHECK(count(a)==16);++cases;
 NativeScriptRuntimeContext c=context(a);NativeScriptItemProgram* p=&prepared.programs[0];uint32_t hook=p->hooks[NativeScriptItemHook_Accessory];uint32_t invalid[]={UINT32_MAX,bank.declarations.function_count,helper(p->owner,"Helper"),p->hooks[NativeScriptItemHook_Created],p->hooks[NativeScriptItemHook_Touched],prepared.programs[1].signals[0].function};
 for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){p->hooks[NativeScriptItemHook_Accessory]=invalid[i];item->on_accessory=native_accessory;uint32_t prior=owner(a)->callbacks[NativeScriptItemCallback_Accessory];rejected(&c);CHECK(item->on_accessory==native_accessory&&owner(a)->callbacks[NativeScriptItemCallback_Accessory]==prior);p->hooks[NativeScriptItemHook_Accessory]=hook;}
 item->on_accessory=NULL;
 NativeScriptRuntimeContext changed=c;changed.source_class=NativeCallbackSource_Fighter;rejected(&changed);changed=c;changed.generation++;rejected(&changed);changed=c;changed.scene_epoch++;rejected(&changed);
 uint64_t identity=p->identity;p->identity++;rejected(&c);p->identity=identity;
 const NativeScriptCompileBank* pinned=p->bank;p->bank=NULL;rejected(&c);p->bank=pinned;
 NativeScriptItemActor* actor=owner(a);actor->faulted=true;rejected(&c);actor->faulted=false;
 NativeScriptStorageLayout layout=bank.storage_layout;const NativeScriptStorageLayout* original=actor->storage.layout;actor->storage.layout=&layout;rejected(&c);actor->storage.layout=original;
 NativeScriptStorageParameters parameters_copy=bank.storage_parameters;const NativeScriptStorageParameters* original_params=actor->storage.parameters;actor->storage.parameters=&parameters_copy;rejected(&c);actor->storage.parameters=original_params;
 actor->storage.live=false;rejected(&c);actor->storage.live=true;
 u16 classifier=a->classifier;a->classifier=HSD_GOBJ_CLASS_FIGHTER;rejected(&c);a->classifier=classifier;
 uint32_t source_owner=item->native_script_owner;item->native_script_owner++;rejected(&c);item->native_script_owner=source_owner;
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(!nativeScriptBindingItem_Invoke(&c,NativeScriptBinding_ItemTrackAccessory,NULL,1,&out,&diagnostic));CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);++cases;
 HSD_GObj* empty=create(1);((Item*)empty->user_data)->on_accessory=native_accessory;NativeScriptRuntimeContext empty_context=context(empty);rejected(&empty_context);CHECK(((Item*)empty->user_data)->on_accessory==native_accessory);ok_signal(empty,"clear");CHECK(((Item*)empty->user_data)->on_accessory==NULL);++cases;
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(signal(empty,"activate",&out)==NativeScriptVM_Error);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&nativeScriptItem_Status(empty)==NativeScriptVM_Error);Item_8026A8EC(empty);++cases;
 HSD_GObj* failing=create(0);ok_signal(failing,"activate");unsigned tail=accessory_tail_calls;scheduler_fail_write=true;scheduler_accessory(failing);scheduler_fail_write=false;CHECK(nativeScriptItem_Status(failing)==NativeScriptVM_Error&&accessory_tail_calls==tail);Item_8026A8EC(failing);++cases;
 HSD_GObj* retiring=create(0);ok_signal(retiring,"activate");NativeMatchPoolReference retired;CHECK(nativeMatchPool_Reference(&nativeGObjPool,retiring,&retired));tail=accessory_tail_calls;scheduler_remove=true;scheduler_accessory(retiring);CHECK(nativeMatchPool_Resolve(retired)==NULL&&accessory_tail_calls==tail);++cases;
 c=context(a);NativeMatchPoolReference removed;CHECK(nativeMatchPool_Reference(&nativeGObjPool,a,&removed));Item_8026A8EC(a);HSD_GObj* replacement=create(0);CHECK(replacement==a&&nativeMatchPool_Resolve(removed)==NULL);rejected(&c);CHECK(!nativeScriptItem_HookBound(replacement,NativeScriptItemHook_Accessory));Item_8026A8EC(replacement);++cases;
 CHECK(nativeScriptItem_RetireScene());printf("{\"status\":\"PASS owned Item accessory registration and native scheduler\",\"checks\":%u,\"cases\":%u,\"source_vm_signals\":%u,\"prepared_entities\":%u}\n",checks,cases,vm_calls,prepared.count);return 0;
}
