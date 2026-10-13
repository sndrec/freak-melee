#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_source_bank.h>
#include <melee/ft/native_script_binding_presentation.h>

#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_pool.h>

#include <melee/ft/ftdata.h>
#include <sysdolphin/baselib/gobjplink.h>




#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""
enum{N=1024,F=32,V=64};
static char text[16384];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N];static uint32_t arguments[N];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[32];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch call_graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
NativeScriptCompileBank presentation_bank;unsigned presentation_checks;static NativeSourcePackage source;
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/itmaplib.h>
static NativeScriptDiagnostic diagnostic;
static Article articles[3];static ItemAttr attributes[3];static ItemModelDesc models[3];
static NativeScriptDiagnostic diagnostic;static Item items[256];static unsigned item_count,checks,cases,pairs,vm_calls;
void presentation_check(bool ok,const char* expression,unsigned line)
{++checks;if(!ok){fprintf(stderr,"ground-state %u: %s (%u:%u %s)\n",line,expression,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return source.ready&&source.target==kind?&source:NULL;}
static void configure(void)
{
 source.compiler.bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void prepare(void)
{
 const char* authored="fighter:\n    behavior independent\n"
 "article Probe states 1:\n    state bool observed\n    state int tail\n    on picked_up:\n        item.grounded = true\n        item.state.observed = item.grounded\n        item.state.tail = item.state.tail + 1\n    on dropped:\n        item.grounded = false\n        item.state.observed = item.grounded\n        item.state.tail = item.state.tail + 1\n    on signal Fault:\n        item.state.tail = 1 / item.state.tail\n    on destroyed:\n        item.grounded = false\n        item.state.tail = item.state.tail + 1\n    state 0 anim -1:\n"
 "article Second states 1:\n    on picked_up:\n        item.grounded = true\n    on dropped:\n        item.grounded = false\n    state 0 anim -1:\n"
 "item Holdable damage 4 angle 45 lifetime 90:\n    on tick:\n        item.grounded = true\n";
 configure();for(unsigned i=0;i<3;++i)articles[i]=(Article){.x0_common_attr=&attributes[i],.x10_modelDesc=&models[i]};
 NativeScriptItemAsset assets[]={{.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&articles[0]},{.declaration="Second",.kind=It_Kind_GameWatch_Panic,.article=&articles[1]},{.declaration="Holdable",.kind=It_Kind_NativeHoldable,.article=&articles[2]}};
 NativeScriptItemMetadataContext metadata={.assets=assets,.asset_count=3};
 source.assets.facts.moves=(NativeScriptMoveLayout){.common_count=341,.first=341,.count=1};
 NativeScriptCompileRequest request={.source=authored,.source_bytes=(uint32_t)strlen(authored),.motion_layout=&source.assets.facts.moves,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
 const NativeScriptCompileBank* accepted=NULL;CHECK(nativeScriptCompile_Build(&source.compiler.bank,&request,&accepted,&diagnostic));
 source.target=Ft_Kind_CustomFirst;CHECK(nativeScriptFighter_PrepareProgram(source.target,&source.compiler.bank,&source.assets.facts.moves,&metadata,&source.program,&diagnostic));
 source.address=&source;source.ready=source.assets.ready=true;CHECK(nativeScriptFighter_CanPublish(source.target));nativeScriptFighter_PublishProgram(source.target,&source.program);
 CHECK(nativeScriptFighter_Program(source.target)->items.count==3);
}
static void remove_item(void* data){nativeScriptItem_Detach(((Item*)data)->entity);}
static const NativeScriptItemProgram* spec(unsigned index){return &nativeScriptFighter_Program(source.target)->items.programs[index];}
static HSD_GObj* make(unsigned which)
{
 CHECK(item_count<256);const NativeScriptItemProgram* p=spec(which);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(o);Item* i=&items[item_count++];
 *i=(Item){.entity=o,.kind=p->kind,.x1C=1000+item_count,.msid=0,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.xC4_article_data=(Article*)p->article,.xCC_item_attr=p->article?p->article->x0_common_attr:NULL,.pos={1,-0.0f,3},.x40_vel={4,5,6}};
 if(p->recipe){i->xDD4_itemVar.native_holdable.owner_kind=p->author_kind;i->xDD4_itemVar.native_holdable.definition_index=p->recipe_index;}
 GObj_InitUserData(o,6,remove_item,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Item,.generation=((Item*)o->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue boolean(unsigned v){return(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=v};}
static bool write(HSD_GObj* o,NativeScriptValue value)
{NativeScriptRuntimeContext c=context(o);return nativeScriptBindings_Write(&c,NativeScriptBinding_ItemGrounded,NULL,0,&value,&diagnostic);}
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,((Item*)object->user_data)->x1C,&view));
 for(unsigned n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
  NativeScriptValue result;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
 }CHECK(false);return(NativeScriptValue){0};
}
static void compare(void)
{
 for(unsigned which=0;which<2;++which)for(unsigned initial=0;initial<3;++initial)for(unsigned marker=0;marker<2;++marker)for(unsigned v=0;v<2;++v){
  HSD_GObj* o=make(which);Item* i=o->user_data;i->ground_or_air=(GroundOrAir)initial;i->xDC8_word.flags.x1F=marker;Item expected=*i;
  if(v)it_802762B0(&expected);else it_802762BC(&expected);
  CHECK(write(o,boolean(v)));CHECK(!memcmp(i,&expected,sizeof(*i)));
  NativeScriptRuntimeContext c=context(o);NativeScriptValue got;CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemGrounded,NULL,0,&got,&diagnostic)&&got.type==NativeScriptType_Bool&&got.as.u64==v);
  ++pairs;++cases;HSD_GObjFree(o);
 }
 for(unsigned which=0;which<2;++which)for(unsigned v=0;v<2;++v){
  HSD_GObj* o=make(which);Item* i=o->user_data;i->ground_or_air=GA_Air;i->xDC8_word.flags.x1F=1;Item expected=*i;if(v)it_802762B0(&expected);else it_802762BC(&expected);
  NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptItem_RunHook(o,v?NativeScriptItemHook_PickedUp:NativeScriptItemHook_Dropped,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void);++vm_calls;
  CHECK(!memcmp(i,&expected,sizeof(*i)));if(!which){CHECK(state(o,"observed").as.u64==v);CHECK(state(o,"tail").as.i64==1);}++pairs;++cases;HSD_GObjFree(o);
 }
}
static void rejected(void)
{
 for(unsigned state=0;state<5;++state)for(unsigned v=0;v<2;++v){HSD_GObj* o=make(2);Item* i=o->user_data;i->msid=state;Item before=*i;CHECK(!write(o,boolean(v)));CHECK(strstr(diagnostic.message,"selected by its motion"));CHECK(!memcmp(i,&before,sizeof(*i)));NativeScriptRuntimeContext c=context(o);NativeScriptValue out;CHECK(nativeScriptBindings_Read(&c,NativeScriptBinding_ItemGrounded,NULL,0,&out,&diagnostic)&&out.as.u64==(state==1));++cases;HSD_GObjFree(o);}
 for(unsigned bad=0;bad<16;++bad){HSD_GObj* o=make(0);Item* i=o->user_data;NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,i->x1C,&view));Item before=*i;NativeScriptStorageInstance saved=*view.storage;NativeScriptStorageLayout foreign_layout={0};NativeScriptStorageParameters foreign_parameters={0};uint64_t epoch=nativeGObjPool.lifetime;NativeScriptRuntimeContext c=context(o);NativeScriptValue value=boolean(1);
  switch(bad){case 0:value.type=NativeScriptType_I32;break;case 1:value.as.u64=2;break;case 2:c.source_class=NativeCallbackSource_Fighter;break;case 3:++c.generation;break;case 4:++c.scene_epoch;break;case 5:view.storage->live=false;break;case 6:++i->native_script_owner;break;case 7:++i->native_article_author_kind;break;case 8:i->xC4_article_data=&articles[1];break;case 9:o->classifier=HSD_GOBJ_CLASS_FIGHTER;break;case 10:++nativeGObjPool.lifetime;break;case 11:view.storage->layout=&foreign_layout;break;case 12:view.storage->parameters=&foreign_parameters;break;case 13:++view.storage->owner;break;case 14:++view.storage->generation;break;case 15:c.source_class=NativeCallbackSource_ModelEffect;break;}
  Item invalid=*i;CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_ItemGrounded,NULL,0,&value,&diagnostic));CHECK(!memcmp(i,&invalid,sizeof(*i)));
  *i=before;nativeGObjPool.lifetime=epoch;o->classifier=HSD_GOBJ_CLASS_ITEM;*view.storage=saved;++cases;HSD_GObjFree(o);
 }
 HSD_GObj* o=make(0);NativeScriptRuntimeContext c=context(o);Item* i=o->user_data;Item before=*i;NativeScriptValue value=boolean(1);HSD_GObjFree(o);CHECK(!nativeScriptBindings_Write(&c,NativeScriptBinding_ItemGrounded,NULL,0,&value,&diagnostic));CHECK(!memcmp(i,&before,sizeof(*i)));++cases;
 o=make(2);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};before=*(Item*)o->user_data;CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_WorkshopTick,NULL,0,&out)==NativeScriptVM_Error);CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(o->user_data,&before,sizeof(before)));++vm_calls;++cases;HSD_GObjFree(o);
 o=make(0);i=o->user_data;before=*i;out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 const NativeScriptItemProgram* p=spec(0);CHECK(p->signal_count==1);CHECK(nativeScriptItem_RunSignal(o,p->signals[0].signal_id,0,&out)==NativeScriptVM_Error);++vm_calls;
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&nativeScriptItem_Status(o)==NativeScriptVM_Error);
 CHECK(!write(o,boolean(1))&&!memcmp(i,&before,sizeof(before)));++cases;HSD_GObjFree(o);
 o=make(0);i=o->user_data;i->native_removal_in_progress=true;i->ground_or_air=GA_Ground;i->xDC8_word.flags.x1F=1;before=*i;it_802762BC(&before);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Destroyed,NULL,0,&out)==NativeScriptVM_Ok);CHECK(!memcmp(i,&before,sizeof(before)));++vm_calls;++pairs;++cases;HSD_GObjFree(o);
}
static void compiler_negatives(void)
{
 const char* assignments[]={"item.grounded = 1","item.grounded = 1.0","item.grounded = None"};
 NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
 NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&articles[0]};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
 for(unsigned n=0;n<4;++n){char bad[512];
  if(n<3)snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\narticle Probe states 1:\n    on picked_up:\n        %s\n    state 0 anim -1:\n",assignments[n]);
  else snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\nfunc Bad():\n    item.grounded = true\n");
  NativeScriptCompileRequest request={.source=bad,.source_bytes=(uint32_t)strlen(bad),.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic));CHECK(!scratch.bank.valid);++cases;
 }
 nativeSourceBank_Release(&scratch);
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();compiler_negatives();compare();rejected();CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS Item ground state\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"actor_vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
