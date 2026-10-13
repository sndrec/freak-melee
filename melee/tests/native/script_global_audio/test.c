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
#include <melee/lb/lbaudio_ax.h>
#include <sysdolphin/baselib/axdriver.h>

#include <melee/ft/native_script_binding_audio.h>


static Article articles[3];static ItemAttr attributes[3];static ItemModelDesc models[3];
static NativeScriptDiagnostic diagnostic;static Item items[256];static unsigned item_count,checks,cases,pairs,vm_calls;
void presentation_check(bool ok,const char* expression,unsigned line)
{++checks;if(!ok){fprintf(stderr,"global audio %u: %s (%u:%u %s)\n",line,expression,diagnostic.source.line,diagnostic.source.column,diagnostic.message);exit(1);}}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return source.ready&&source.target==kind?&source:NULL;}
static void configure(void)
{
 source.compiler.bank=(NativeScriptCompileBank){.source_storage=text,.source_capacity=sizeof(text),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N,.arguments=arguments,.argument_capacity=N},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=32,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=call_graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
}
static void prepare(void)
{
 const char* authored="fighter:\n    behavior independent\n"
 "func Audio(int id, int volume, int pan) -> int:\n    return audio.play_global(id, volume, pan)\n"
 "article Probe states 1:\n    state int handle\n    state int samples\n    state int order\n"
 "    func Sample(int v) -> int:\n        item.state.samples = item.state.samples + 1\n        item.state.order = item.state.order * 10 + v\n        return v\n"
 "    on picked_up:\n        let int voice = audio.play_global(Sample(1), Sample(2), Sample(3))\n        item.state.handle = voice\n"
 "    on dropped:\n        audio.play_global(4, 127, 64)\n"
 "    on destroyed:\n        item.state.handle = audio.play_global(5, 128, -1)\n    state 0 anim -1:\n"
 "article Second states 1:\n    state int handle\n    on picked_up:\n        item.state.handle = audio.play_global(-1, -1, 128)\n    state 0 anim -1:\n"
 "item Holdable damage 4 angle 45 lifetime 90:\n    on tick:\n        return\n";
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
static NativeScriptValue state(HSD_GObj* object,const char* name)
{
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(object,((Item*)object->user_data)->x1C,&view));
 for(unsigned n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
  NativeScriptValue result;CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
 }CHECK(false);return(NativeScriptValue){0};
}
static void set_state(HSD_GObj* o,const char* name,NativeScriptValue value)
{
 NativeScriptReceiverView view;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&view));
 for(unsigned n=0;n<view.program->value_count;++n)if(view.program->values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[n].name,name)){
  CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&value,&diagnostic));return;
 }CHECK(false);
}
// Unselected native setup/part paths must never claim audio fixture evidence.
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){(void)j;abort();}
u32 HSD_JObjGetFlags(HSD_JObj* j){(void)j;abort();}
void HSD_JObjClearFlags(HSD_JObj* j,u32 flags){(void)j;(void)flags;abort();}
void lb_8000B1CC(HSD_JObj* j,Vec3* a,Vec3* b){(void)j;(void)a;(void)b;abort();}
typedef struct AudioCall{int id,volume,pan,track,channel;} AudioCall;
static AudioCall observed;
static unsigned calls,mutation;
static int handle;
static HSD_GObj* selected;
static NativeScriptReceiverView selected_view;
// Silent argument/result boundary. Native lbAudio helpers execute unchanged.
int AXDriver_8038CFF4(int id,u8 volume,u8 pan,int track,int channel)
{
 ++calls;observed=(AudioCall){id,volume,pan,track,channel};
 if(mutation){unsigned mode=mutation;mutation=0;
  if(mode==1)selected_view.storage->live=false;
  if(mode==2)++source.compiler.bank.storage_layout.identity;
  if(mode==3)++source.compiler.bank.storage_parameters.identity;
  if(mode==4)++selected_view.storage->generation;
  if(mode==5)++selected_view.storage->owner;
  if(mode==6){if(selected->classifier==HSD_GOBJ_CLASS_ITEM)++((Item*)selected->user_data)->x1C;else ++((Fighter*)selected->user_data)->x8_spawnNum;}
 }
 return handle;
}
static NativeScriptValue integer(int64_t v){return(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=v};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xabcdef123456789)};}
static void arguments3(NativeScriptValue* a,int id,int volume,int pan)
{a[0]=integer(id);a[1]=integer(volume);a[2]=integer(pan);}
static void compare(void)
{
 const int inputs[][3]={{0,0,0},{1,127,64},{-1,-1,128},{-9999,128,-3},{-10000,300,300},{INT32_MIN,-1073741824,1073741823},{0x83d60,1073741823,-1073741824},{0x83d61,INT32_MIN,INT32_MAX},{INT32_MAX,INT32_MAX,INT32_MIN},{271045,126,126},{7,1,2}};
 const int handles[]={-1,0,91,INT32_MIN,INT32_MAX};
 for(unsigned owner=0;owner<2;++owner)for(unsigned n=0;n<sizeof(inputs)/sizeof(inputs[0]);++n){
  HSD_GObj* o=make(owner);NativeScriptRuntimeContext c=context(o);NativeScriptValue a[3];arguments3(a,inputs[n][0],inputs[n][1],inputs[n][2]);
  Item before=*(Item*)o->user_data;handle=handles[n%5];CHECK(lbAudioAx_800237A8(inputs[n][0],inputs[n][1],inputs[n][2])==handle);AudioCall expected=observed;unsigned old=calls;
  NativeScriptValue out=sentinel();CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,NULL,0,a,3,&out,&diagnostic));
  CHECK(calls==old+1&&out.type==NativeScriptType_I32&&out.as.i64==handle&&!memcmp(&observed,&expected,sizeof(expected))&&!memcmp(o->user_data,&before,sizeof(before)));
  CHECK(observed.track==0&&observed.channel==7);if(inputs[n][0]>=0x83d61)CHECK(observed.id==0x83d60&&observed.volume==0&&observed.pan==128);
  ++pairs;++cases;HSD_GObjFree(o);
 }
}
static void article_vm(void)
{
 for(unsigned n=0;n<5;++n){HSD_GObj* o=make(n==4?1:0);NativeScriptValue out=sentinel();handle=n==2?-1:n==3?INT32_MIN:91;unsigned old=calls;
  NativeScriptReceiverView retained;CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&retained));
  uint32_t hook=n==1?NativeScriptItemHook_Dropped:n==2?NativeScriptItemHook_Destroyed:NativeScriptItemHook_PickedUp;
  if(n==2)((Item*)o->user_data)->native_removal_in_progress=true;
  CHECK(nativeScriptItem_RunHook(o,hook,NULL,0,&out)==NativeScriptVM_Ok&&out.type==NativeScriptType_Void&&calls==old+1);
  if(n==2){bool found=false;for(unsigned field=0;field<retained.storage->layout->field_count;++field){const NativeScriptStorageField* f=&retained.storage->layout->fields[field];if(f->owner==retained.storage->owner&&nativeScriptSource_SpanEquals(retained.program->tree,retained.program->values[field].name,"handle")){int32_t bits;memcpy(&bits,retained.storage->state+f->offset,4);CHECK(bits==handle&&!retained.storage->live);found=true;break;}}CHECK(found);}
  else if(n!=1)CHECK(state(o,"handle").as.i64==handle);
  if(n==0||n==3){CHECK(state(o,"samples").as.i64==3&&state(o,"order").as.i64==123);CHECK(observed.id==1&&observed.volume==4&&observed.pan==6);}
  if(n==1)CHECK(observed.id==4&&observed.volume==254&&observed.pan==128);
  if(n==2)CHECK(observed.id==5&&observed.volume==255&&observed.pan==0);
  if(n==4)CHECK(observed.id==-1&&observed.volume==0&&observed.pan==255);
  ++vm_calls;++cases;HSD_GObjFree(o);
 }
}
static void rejected(void)
{
 for(unsigned bad=0;bad<32;++bad){HSD_GObj* o=make(0);Item* i=o->user_data;CHECK(nativeScriptItem_View(o,i->x1C,&selected_view));NativeScriptStorageInstance storage=*selected_view.storage;
  NativeScriptRuntimeContext c=context(o);NativeScriptValue a[3];arguments3(a,1,127,64);uint64_t epoch=nativeGObjPool.lifetime;Item original=*i;unsigned count=3;uint32_t semantic=NativeScriptBinding_AudioPlayGlobal;
  switch(bad){case 0:a[0].type=NativeScriptType_Bool;break;case 1:a[1].type=NativeScriptType_F32;break;case 2:a[2].type=NativeScriptType_F64;break;case 3:a[0].type=NativeScriptType_U32;break;case 4:a[1].type=NativeScriptType_I64;break;case 5:a[2].type=NativeScriptType_U64;break;case 6:a[0].as.i64=(int64_t)INT32_MAX+1;break;case 7:a[1].as.i64=(int64_t)INT32_MIN-1;break;case 8:a[2].as.i64=(int64_t)INT32_MAX+1;break;
   case 9:a[1].as.i64=-1073741825;break;case 10:a[1].as.i64=1073741824;break;case 11:a[2].as.i64=-1073741825;break;case 12:a[2].as.i64=1073741824;break;case 13:count=2;break;case 14:semantic=NativeScriptBinding_StageLineExists;break;case 15:++c.generation;break;case 16:++c.scene_epoch;break;case 17:c.source_class=NativeCallbackSource_Fighter;break;case 18:selected_view.storage->live=false;break;case 19:++selected_view.storage->owner;break;case 20:++selected_view.storage->generation;break;case 21:++i->native_script_owner;break;case 22:++i->native_article_author_kind;break;case 23:i->xC4_article_data=&articles[1];break;case 24:o->classifier=HSD_GOBJ_CLASS_FIGHTER;break;case 25:++nativeGObjPool.lifetime;break;case 26:selected_view.storage->layout=NULL;break;case 27:selected_view.storage->parameters=NULL;break;case 28:i->kind=It_Kind_Coin;break;case 29:i->entity=NULL;break;case 30:c.source=NULL;break;case 31:count=4;break;}
  Item before=*i;NativeScriptValue saved[3];memcpy(saved,a,sizeof(a));NativeScriptValue out=sentinel(),old=out;unsigned prior=calls;
  CHECK(!nativeScriptBindingAudio_Invoke(&c,semantic,a,count,&out,&diagnostic));CHECK(calls==prior&&!memcmp(saved,a,sizeof(a))&&!memcmp(&old,&out,sizeof(out))&&!memcmp(i,&before,sizeof(before)));
  *i=original;*selected_view.storage=storage;nativeGObjPool.lifetime=epoch;o->classifier=HSD_GOBJ_CLASS_ITEM;++cases;HSD_GObjFree(o);
 }
 HSD_GObj* o=make(0);NativeScriptRuntimeContext c=context(o);NativeScriptValue a[3],out=sentinel();arguments3(a,1,127,64);unsigned prior=calls;
 CHECK(!nativeScriptBindingAudio_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,NULL,3,&out,&diagnostic));CHECK(!nativeScriptBindingAudio_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,a,3,NULL,&diagnostic));HSD_GObjFree(o);
 CHECK(!nativeScriptBindingAudio_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,a,3,&out,&diagnostic)&&calls==prior);++cases;
 // Central Host gate must not perform implicit Bool/Float/wide integer casts.
 for(unsigned n=0;n<4;++n){o=make(0);c=context(o);arguments3(a,1,127,64);a[n%3].type=n==0?NativeScriptType_Bool:n==1?NativeScriptType_F32:n==2?NativeScriptType_U64:NativeScriptType_I64;out=sentinel();NativeScriptValue old=out;prior=calls;
  CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,NULL,0,a,3,&out,&diagnostic)&&calls==prior&&!memcmp(&out,&old,sizeof(out)));++cases;HSD_GObjFree(o);
 }
}
static void publication(void)
{
 for(unsigned mode=1;mode<=6;++mode){HSD_GObj* o=make(0);Item before=*(Item*)o->user_data;NativeScriptRuntimeContext c=context(o);CHECK(nativeScriptItem_View(o,c.generation,&selected_view));NativeScriptStorageInstance storage=*selected_view.storage;
  uint64_t layout=source.compiler.bank.storage_layout.identity,parameters=source.compiler.bank.storage_parameters.identity;NativeScriptValue a[3],out=sentinel(),old=out;arguments3(a,1,127,64);unsigned prior=calls;
  selected=o;mutation=mode;CHECK(!nativeScriptBindingAudio_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,a,3,&out,&diagnostic));CHECK(calls==prior+1&&!mutation&&!memcmp(&out,&old,sizeof(out)));
  *(Item*)o->user_data=before;*selected_view.storage=storage;source.compiler.bank.storage_layout.identity=layout;source.compiler.bank.storage_parameters.identity=parameters;++cases;HSD_GObjFree(o);
 }
 HSD_GObj* o=make(0);CHECK(nativeScriptItem_View(o,((Item*)o->user_data)->x1C,&selected_view));set_state(o,"handle",integer(77));uint64_t layout=source.compiler.bank.storage_layout.identity;
 selected=o;mutation=2;NativeScriptValue out=sentinel(),old=out;unsigned prior=calls;CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_PickedUp,NULL,0,&out)==NativeScriptVM_Error);++vm_calls;
 CHECK(calls==prior+1&&!memcmp(&out,&old,sizeof(out)));source.compiler.bank.storage_layout.identity=layout;CHECK(state(o,"handle").as.i64==77&&state(o,"samples").as.i64==3);++cases;HSD_GObjFree(o);
}
static void compiler_negatives(void)
{
 const char* calls_bad[]={"audio.play_global(true, 2, 3)","audio.play_global(1, false, 3)","audio.play_global(1, 2, true)","audio.play_global(1.0, 2, 3)","audio.play_global(1, 2.5, 3)","audio.play_global(1, 2, 3.0)","audio.play_global(1, 2)","audio.play_global(1, 2, 3, 4)","audio.play_global(\"bad\", 2, 3)"};
 NativeSourceBank scratch={0};CHECK(nativeSourceBank_Allocate(&scratch,&diagnostic));
 for(unsigned n=0;n<9;++n){char bad[1024];snprintf(bad,sizeof(bad),"fighter:\n    behavior independent\nfunc Bad() -> int:\n    return %s\n",calls_bad[n]);NativeScriptCompileRequest request={.source=bad,.source_bytes=(uint32_t)strlen(bad)};const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&scratch.bank,&request,&accepted,&diagnostic)&&!scratch.bank.valid);++cases;}
 nativeSourceBank_Release(&scratch);
}
static void fighter_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void fighter_audio(void)
{
 static Fighter fighter;HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
 fighter=(Fighter){.gobj=o,.kind=source.target,.behavior_kind=source.target,.x8_spawnNum=4001,.ft_data=&source.assets.character.data};
 gFtDataList[source.target]=&source.assets.character.data;ftData_CharacterStateTables[source.target]=source.assets.motions;GObj_InitUserData(o,4,fighter_remove,&fighter);CHECK(nativeScriptFighter_Attach(o));
 NativeScriptRuntimeContext c={.source=o,.source_class=NativeCallbackSource_Fighter,.generation=4001,.scene_epoch=nativeGObjPool.lifetime};uint32_t function=UINT32_MAX;const NativeScriptProgramIndex* p=&source.compiler.bank.declarations;
 for(unsigned f=0;f<p->function_count;++f)if(!p->functions[f].owner&&nativeScriptSource_SpanEquals(p->tree,p->functions[f].name,"Audio"))function=f;CHECK(function!=UINT32_MAX);
 const int expected_handles[]={-1,INT32_MIN,INT32_MAX};
 for(unsigned n=0;n<3;++n){NativeScriptValue a[3],out=sentinel();arguments3(a,n?0x83d61:271045,n?INT32_MAX:127,n?INT32_MIN:64);handle=expected_handles[n];Fighter before=fighter;
  CHECK(lbAudioAx_800237A8((int)a[0].as.i64,(int)a[1].as.i64,(int)a[2].as.i64)==handle);AudioCall expected=observed;unsigned prior=calls;
  CHECK(nativeScriptFighter_Helper(o,4001,function,a,3,&out,&diagnostic)==NativeScriptVM_Ok&&out.type==NativeScriptType_I32&&out.as.i64==handle&&calls==prior+1&&!memcmp(&observed,&expected,sizeof(expected))&&!memcmp(&fighter,&before,sizeof(before)));++pairs;++cases;++vm_calls;
 }
 CHECK(nativeScriptFighter_View(o,4001,&selected_view));NativeScriptStorageInstance storage=*selected_view.storage;NativeScriptValue a[3],out=sentinel(),old=out;arguments3(a,1,127,64);selected=o;mutation=4;unsigned prior=calls;
 CHECK(!nativeScriptBindingAudio_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,a,3,&out,&diagnostic)&&calls==prior+1&&!memcmp(&out,&old,sizeof(out)));*selected_view.storage=storage;++cases;
 ++c.generation;prior=calls;CHECK(!nativeScriptBindingAudio_Invoke(&c,NativeScriptBinding_AudioPlayGlobal,a,3,&out,&diagnostic)&&calls==prior);++cases;HSD_GObjFree(o);
}
int main(void)
{
 HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();compiler_negatives();compare();article_vm();rejected();publication();fighter_audio();CHECK(nativeScriptRuntime_Idle());
 printf("{\"status\":\"PASS global audio\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"actor_vm_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
