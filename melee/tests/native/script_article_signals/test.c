#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_signal.h>
#include <melee/ft/native_script_binding_item_links.h>
#include <melee/ft/native_script_binding_signal_resolve.h>
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
unsigned signal_mode;HSD_GObj* signal_caller;HSD_GObj*(*signal_reuse)(void);static HSD_GObj* replacement;
#define CHECK(v)do{++checks;if(!(v)){fprintf(stderr,"signal %u: %s\n",__LINE__,#v);exit(1);}}while(0)
static uint32_t hash(const char* s){uint32_t h=2166136261u;while(*s){h^=(unsigned char)*s++;h*=16777619u;}return h;}
static void prepare(void){bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
 .declarations={.functions=functions,.function_capacity=F,.parameters=params,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,.instructions=instructions,.instruction_capacity=I,
 .function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int handle\nstate int other\nstate bool delivered\nstate int completed\naction Hooks:\n    on tick:\n        state.delivered = articles.signal(state.handle, Probe, \"Ping\", 2.5)\n        state.completed = state.completed + 1\nfunc LinkOwned():\n    state.delivered = articles.link_owned(state.handle, 0, state.other)\nfunc SendOwned():\n    state.delivered = articles.signal_owned(state.handle, Probe, \"Ping\", 2.5)\n    state.completed = state.completed + 1\narticle Probe states 1:\n    on destroyed:\n        item.unlink(0)\n    state int handle\n    state bool delivered\n    state int completed\n    on signal ReadLink:\n        item.state.handle = item.link_id(0)\n        item.state.delivered = item.link_reciprocal(0, 0)\n    on signal Unlink:\n        item.unlink(0)\n    on signal Forward:\n        item.state.delivered = articles.signal(item.link_id(0), Probe, \"Inner\", signal.value)\n    state float amount\n    on signal Ping:\n        item.state.amount = signal.value\n        item.signal(\"Inner\", signal.value + 1)\n        item.state.completed = item.state.completed + 1\n    on signal Inner:\n        position.x = signal.value\n        item.state.completed = item.state.completed + 10\n    state 0 anim -1:\narticle Other states 1:\n    state int completed\n    on signal Ping:\n        item.state.completed = item.state.completed + 100\n    state 0 anim -1:\n";
 for(unsigned i=0;i<2;++i)assets[i]=(Article){.x0_common_attr=&attr,.x10_modelDesc=&model};
 NativeScriptItemAsset bindings[]={{.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&assets[0]},{.declaration="Other",.kind=It_Kind_Mario_Fire,.article=&assets[1]}};
 NativeScriptItemMetadataContext metadata={.assets=bindings,.asset_count=2};NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 NativeScriptDiagnostic d;const NativeScriptCompileBank* accepted=NULL;if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,NULL,&metadata,&program,&d)){fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&program);
}
static void fighter_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* caller(void){HSD_GObj* g=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);fighter=(Fighter){.gobj=g,.kind=Ft_Kind_CustomFirst,.behavior_kind=Ft_Kind_CustomFirst,.x8_spawnNum=19};GObj_InitUserData(g,4,fighter_remove,&fighter);CHECK(nativeScriptFighter_Attach(g));signal_caller=g;return g;}
static void item_remove(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static HSD_GObj* item(unsigned index){CHECK(created<128);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[index];HSD_GObj* g=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);
 Item* i=&items[created++];*i=(Item){.entity=g,.kind=p->kind,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=override_id?override_id:100+created,.xC4_article_data=(Article*)p->article};
 GObj_InitUserData(g,6,item_remove,i);CHECK(nativeScriptItem_Attach(g,p));return g;}
static HSD_GObj* reuse(void){replacement=item(0);return replacement;}
static NativeScriptValue field(HSD_GObj* g,const char* name,const NativeScriptValue* write){NativeScriptReceiverView view;uint32_t gen=g==signal_caller?19:((Item*)g->user_data)->x1C;
 CHECK(g==signal_caller?nativeScriptFighter_View(g,gen,&view):nativeScriptItem_View(g,gen,&view));NativeScriptDiagnostic d;
 for(uint32_t i=0;i<bank.declarations.value_count;++i)if(values[i].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name)){
  NativeScriptValue out;if(write)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,write,&d));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&d));return out;}
 CHECK(false);return(NativeScriptValue){0};}
static void sethandle(HSD_GObj* g,int32_t id){NativeScriptValue v={.type=NativeScriptType_I32,.as.i64=id};field(g,"handle",&v);}
static void tick(HSD_GObj* g,bool delivered){NativeScriptValue result;CHECK(nativeScriptFighter_Run(g,NativeScriptFighter_Tick,NULL,0, NULL,&result)==NativeScriptVM_Ok);CHECK(field(g,"delivered",NULL).as.u64==delivered);}
static uint32_t codeid(unsigned owner,const char* name){for(unsigned f=0;f<bank.declarations.function_count;++f){NativeScriptEventResolution e;NativeScriptDiagnostic d;
 if(functions[f].owner==owner&&functions[f].callback&&nativeScriptEvents_Resolve(&bank.tree,&nodes[functions[f].source_node],&e,&d)&&e.descriptor->kind==NativeScriptEvent_ArticleSignal&&nativeScriptSource_SpanEquals(&bank.tree,e.signal_name,name))return f;}CHECK(false);return UINT32_MAX;}
static NativeScriptValue text(void){for(unsigned i=0;i<bank.expressions.node_count;++i)if(ir[i].kind==NativeScriptIR_String)return(NativeScriptValue){.type=NativeScriptType_Text,.as.text={ir[i].text_offset,ir[i].text_length}};CHECK(false);return(NativeScriptValue){0};}
static uint32_t owned_helper(void)
{
 for(unsigned i=0;i<bank.declarations.function_count;++i)
  if(!functions[i].owner&&!functions[i].callback&&nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,"SendOwned"))return i;
 CHECK(false);return UINT32_MAX;
}
static bool send_owned(HSD_GObj* caller)
{
 NativeScriptValue output;NativeScriptDiagnostic diagnostic;
 CHECK(nativeScriptFighter_Helper(caller,19,owned_helper(),NULL,0,&output,&diagnostic)==NativeScriptVM_Ok);
 return field(caller,"delivered",NULL).as.u64!=0;
}
static void owned_admission(HSD_GObj* f,HSD_GObj* target,HSD_GObj* wrong,HSD_GObj* foreign)
{
 Item* item_data=target->user_data;
 sethandle(f,item_data->x1C);
 int64_t before=field(target,"completed",NULL).as.i64;
 CHECK(nativeScriptItem_SetAttachmentOwner(target,f));
 // Both live ownership roles are required; foreign damage credit is insufficient.
 item_data->owner=foreign;CHECK(!send_owned(f));
 item_data->owner=f;CHECK(send_owned(f));
 CHECK(field(target,"completed",NULL).as.i64==before+11);
 CHECK(nativeScriptItem_SetAttachmentOwner(target,foreign));CHECK(!send_owned(f));
 CHECK(nativeScriptItem_SetAttachmentOwner(target,NULL));CHECK(!send_owned(f));
 CHECK(nativeScriptItem_SetAttachmentOwner(target,f));
 sethandle(f,-1);CHECK(!send_owned(f));
 ((Item*)wrong->user_data)->owner=f;CHECK(nativeScriptItem_SetAttachmentOwner(wrong,f));
 sethandle(f,((Item*)wrong->user_data)->x1C);CHECK(!send_owned(f));
 CHECK(field(wrong,"completed",NULL).as.i64==0);
 sethandle(f,item_data->x1C);
}
#include "actual_links.h"
#include "link_cases.h"
int main(void){prepare();HSD_GObjLibInitDataType configuration={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&configuration);HSD_GObj* f=caller();HSD_GObj* target=item(0),*wrong=item(1);
 sethandle(f,((Item*)target->user_data)->x1C);HSD_GObj* foreign=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter foreign_data;foreign_data=(Fighter){.gobj=foreign,.kind=Ft_Kind_Link,.x8_spawnNum=27};foreign->user_data=&foreign_data;((Item*)target->user_data)->owner=foreign;tick(f,true);CHECK(field(target,"completed",NULL).as.i64==11&&field(target,"amount",NULL).as.f32==2.5f&&((Item*)target->user_data)->pos.x==3.5f);
 owned_admission(f,target,wrong,foreign);link_cases(f,foreign);
 sethandle(f,((Item*)wrong->user_data)->x1C);tick(f,false);CHECK(field(wrong,"completed",NULL).as.i64==0);sethandle(f,-1);tick(f,false);
 unsigned owner=nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0].owner;uint32_t function=codeid(owner,"Inner");
 NativeScriptRuntimeContext c={.source=target,.source_class=NativeCallbackSource_Item,.generation=((Item*)target->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue input[]={text(),{.type=NativeScriptType_F32,.as.f32=1}},output;NativeScriptDiagnostic d;uint32_t semantic=NATIVE_SCRIPT_SIGNAL_DOMAIN|(NativeScriptSignal_Self<<20)|function;
 input[1].as.f32=NAN;CHECK(nativeScriptBindingSignal_Invoke(&c,semantic,input,2,&output,&d)&&output.as.u64==1&&isnan(((Item*)target->user_data)->pos.x));
 input[1].as.f32=INFINITY;CHECK(nativeScriptBindingSignal_Invoke(&c,semantic,input,2,&output,&d)&&isinf(((Item*)target->user_data)->pos.x));
 for(unsigned failure=0;failure<3;++failure){input[0]=text();input[1].as.f32=1;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 if(failure==0)input[0].as.text.offset=UINT32_MAX;else if(failure==1)input[0].as.text.length=UINT32_MAX;else input[0].as.text.offset=bank.code.source_bytes;
 CHECK(!nativeScriptBindingSignal_Invoke(&c,semantic,input,2,&output,&d));CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);}
 // Full signed/unsigned32 creation IDs never round or truncate into another item.
 uint32_t target_sem=NATIVE_SCRIPT_SIGNAL_DOMAIN|(NativeScriptSignal_Target<<20)|codeid(owner,"Ping");
 NativeScriptRuntimeContext fc={.source=f,.source_class=NativeCallbackSource_Fighter,.generation=19,.scene_epoch=nativeGObjPool.lifetime};
 NativeScriptValue target_args[]={{.type=NativeScriptType_U64,.as.u64=123456},{.type=NativeScriptType_I32,.as.i64=0},text(),{.type=NativeScriptType_F32,.as.f32=1}};
 CHECK(nativeScriptBindingSignal_Invoke(&fc,target_sem,target_args,4,&output,&d)&&output.as.u64==0);
 target_args[0].as.u64=UINT64_C(0x100000065);output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingSignal_Invoke(&fc,target_sem,target_args,4,&output,&d)&&output.type==NativeScriptType_U64&&output.as.u64==99);
 override_id=UINT32_C(0x80000005);HSD_GObj* wide=item(0);override_id=0;target_args[0]=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000005)};
 CHECK(nativeScriptBindingSignal_Invoke(&fc,target_sem,target_args,4,&output,&d)&&output.as.u64==1);CHECK(field(wide,"completed",NULL).as.i64==11);
 target_args[0]=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=(int64_t)UINT32_C(0x80000005)-INT64_C(0x100000000)};
 CHECK(nativeScriptBindingSignal_Invoke(&fc,target_sem,target_args,4,&output,&d)&&output.as.u64==1);CHECK(field(wide,"completed",NULL).as.i64==22);
 target_args[3].as.f32=NAN;CHECK(nativeScriptBindingSignal_Invoke(&fc,target_sem,target_args,4,&output,&d)&&output.as.u64==0);CHECK(field(wide,"completed",NULL).as.i64==22);HSD_GObjFree(wide);
 HSD_GObj* faulted=item(0);for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==faulted)nativeScriptItemActors[i].faulted=true;
 target_args[0]= (NativeScriptValue){.type=NativeScriptType_U32,.as.u64=((Item*)faulted->user_data)->x1C};target_args[3].as.f32=1;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingSignal_Invoke(&fc,target_sem,target_args,4,&output,&d)&&output.type==NativeScriptType_U64&&output.as.u64==99);CHECK(nativeScriptFighter_Status(f)==NativeScriptVM_Ok);HSD_GObjFree(faulted);
 // Successful deletion/slot reuse belongs to recipient, while caller completes.
 for(unsigned mutation=1;mutation<=4;++mutation){HSD_GObj* t=item(0);sethandle(f,((Item*)t->user_data)->x1C);signal_mode=mutation;signal_reuse=mutation==4?reuse:NULL;
 NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};NativeScriptVMStatus status=nativeScriptFighter_Run(f,NativeScriptFighter_Tick,NULL,0, NULL,&out);
 if(mutation==1||mutation==4){CHECK(status==NativeScriptVM_Ok&&field(f,"delivered",NULL).as.u64==1);if(mutation==4){CHECK(replacement==t&&field(replacement,"completed",NULL).as.i64==0);HSD_GObjFree(replacement);}}
 else {CHECK(status!=NativeScriptVM_Ok&&out.type==NativeScriptType_U64&&out.as.u64==99);if(mutation==2)f=caller();else {HSD_GObjFree(t);HSD_GObjFree(f);f=caller();}}
 signal_mode=0;signal_reuse=NULL;
 }
 // Owned dispatch retains the same deletion/fault rules and exact caller data.
 for(unsigned mutation=1;mutation<=5;++mutation) {
  HSD_GObj* t=item(0);((Item*)t->user_data)->owner=f;
  CHECK(nativeScriptItem_SetAttachmentOwner(t,f));sethandle(f,((Item*)t->user_data)->x1C);
  signal_mode=mutation;signal_reuse=mutation==4?reuse:NULL;
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};NativeScriptDiagnostic d;
  bool delivered=nativeScriptFighter_Helper(f,19,owned_helper(),NULL,0,&out,&d)==NativeScriptVM_Ok;
  if(mutation==1||mutation==4) {
   CHECK(delivered&&field(f,"delivered",NULL).as.u64==1);
   if(mutation==4){CHECK(replacement==t&&field(replacement,"completed",NULL).as.i64==0);HSD_GObjFree(replacement);}
  } else {
   CHECK(!delivered&&out.type==NativeScriptType_U64&&out.as.u64==99);
   if(mutation!=2){HSD_GObjFree(t);HSD_GObjFree(f);}f=caller();
  }
  signal_mode=0;signal_reuse=NULL;
 }
 HSD_GObjFree(target);HSD_GObjFree(wrong);HSD_GObjFree(f);HSD_GObjFree(foreign);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS real Article signals\",\"checks\":%u}\n",checks);return 0;}
