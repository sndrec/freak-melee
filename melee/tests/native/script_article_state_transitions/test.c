#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "boundary.h"
#include <melee/ft/native_script_binding_spawn.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/item.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
static unsigned checks,cases,pairs,vm_calls,animation_calls;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"transition %u: %s (%s / %s)\n",__LINE__,#x,diagnostic.message,nativeScriptRuntime_VM()->diagnostic.message);exit(1);}}while(0)
enum{N=2048,F=32,V=64};
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[64];static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode codes[F];
static NativeScriptExecutionFunction execution[F];static NativeScriptCompileFunctionScratch scratch[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeScriptCompileBank bank;static NativeScriptItemProgram program;static NativeScriptItemState states[2];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static ItemStateDesc descriptions[2];
static ItemStateArray animations={descriptions,2};static ItemStateTable native_states[2];
static Item instances[8];static HSD_JObj joints[8];static unsigned allocated;
static unsigned injection;static bool nested_animation;static HSD_GObj* replacement;
static bool deferred_stopped;
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{return author==program.author_kind&&owner==program.owner?&program:NULL;}
static NativeScriptItemActor* actor(HSD_GObj* object)
{for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];return NULL;}
static uint32_t field(const char* name)
{for(unsigned i=0;i<bank.declarations.value_count;++i)if(values[i].owner==program.owner&&nativeScriptSource_SpanEquals(&bank.tree,values[i].name,name))return NATIVE_SCRIPT_STATE_SEMANTIC|i;CHECK(false);return 0;}
static void set(HSD_GObj* object,const char* name,NativeScriptValue v)
{CHECK(nativeScriptStorage_Write(&actor(object)->storage,field(name),NULL,0,&v,&diagnostic));}
static uint64_t read(HSD_GObj* object,const char* name)
{NativeScriptValue v;CHECK(nativeScriptStorage_Read(&actor(object)->storage,field(name),NULL,0,&v,&diagnostic));return v.as.u64;}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t i,NativeScriptDiagnostic* d)
{(void)unused;const NativeScriptSourceNode* n=&tree->nodes[i];if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 2");if(n->kind==NativeScriptSource_ArticleState||n->kind==NativeScriptSource_State||n->kind==NativeScriptSource_Function)return true;if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution e;return nativeScriptEvents_Resolve(tree,n,&e,d);}return nativeScriptCompile_MinimalMetadata(NULL,tree,i,d);}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=codes,.function_code_capacity=F,.execution_functions=execution,.execution_function_capacity=F,.graph=scratch,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\narticle It_Kind_Mario_Fire states 2:\n    state int flags\n    state int tail\n    state int phase\n    on picked_up:\n        item.change_state(1)\n        item.state.tail = item.state.tail + 1\n    on reflected:\n        item.change_state_flags(0, 2)\n        return false\n    on accessory:\n        item.state.phase = 77\n    on damage_received:\n        let int zero = item.state.phase\n        item.state.tail = 1 / zero\n        return false\n    state 0 anim 0:\n        on physics:\n            item.change_state_flags(1, item.state.flags)\n            item.state.tail = item.state.tail + 1\n    state 1 anim 1:\n        on physics:\n            item.state.phase = item.state.phase + 100\n";
 const NativeScriptCompileBank* accepted=NULL;NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};CHECK(nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic));
 article.x0_common_attr=&attributes;article.x10_modelDesc=&model;article.xC_itemStates=&animations;
 program=(NativeScriptItemProgram){.bank=&bank,.identity=123,.author_kind=Ft_Kind_Mario,.kind=It_Kind_Mario_Fire,.recipe_index=UINT32_MAX,.article=&article,.states=states,.state_count=2};
 for(unsigned i=0;i<NativeScriptItemHook_Count;++i)program.hooks[i]=UINT32_MAX;
 for(unsigned i=0;i<2;++i){states[i].animation_id=(int32_t)i;native_states[i].anim_id=(int32_t)i;for(unsigned p=0;p<3;++p)states[i].functions[p]=UINT32_MAX;}
 for(unsigned i=0;i<bank.tree.count;++i)if(bank.tree.nodes[i].kind==NativeScriptSource_Article)program.owner=i;
 for(unsigned f=0;f<bank.declarations.function_count;++f)if(functions[f].callback){NativeScriptEventResolution e;CHECK(nativeScriptEvents_Resolve(&bank.tree,&bank.tree.nodes[functions[f].source_node],&e,&diagnostic));if(e.descriptor->kind==NativeScriptEvent_ArticleState)states[e.motion].functions[NativeScriptItemPhase_Physics]=f;else if(!strcmp(e.descriptor->name,"picked_up"))program.hooks[NativeScriptItemHook_PickedUp]=f;else if(!strcmp(e.descriptor->name,"reflected"))program.hooks[NativeScriptItemHook_Reflected]=f;else if(!strcmp(e.descriptor->name,"accessory"))program.hooks[NativeScriptItemHook_Accessory]=f;else if(!strcmp(e.descriptor->name,"damage_received"))program.hooks[NativeScriptItemHook_DamageReceived]=f;}
 CHECK(nativeScriptItem_ProgramValid(&program));scheduler_spec=&program;
}
static HSD_GObj* create(bool attached)
{
 CHECK(allocated<8);unsigned i=allocated++;HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
 Item* item=&instances[i];memset(item,0,sizeof(*item));memset(&joints[i],0,sizeof(joints[i]));item->entity=object;item->x1C=allocated;item->kind=program.kind;item->msid=-1;item->xC4_article_data=&article;item->xCC_item_attr=&attributes;item->xBC_itemStateContainer=native_states;item->scl=1;item->facing_dir=1;
 if(attached){item->native_article_author_kind=program.author_kind;item->native_script_owner=program.owner;}
 GObj_InitUserData(object,6,scheduler_userdata_remove,item);object->hsd_obj=&joints[i];if(attached)CHECK(nativeScriptItem_Attach(object,&program));return object;
}
void transition_stage(HSD_GObj* object)
{
 unsigned mode=injection;injection=0;if(!mode)return;
 NativeScriptItemActor* a=actor(object);CHECK(a);
 if(mode==1){Item_80268E5C(object,0,ITEM_ANIM_UPDATE);CHECK(nativeScriptItem_BindCallback(object,NativeScriptItemCallback_Accessory,program.hooks[NativeScriptItemHook_Accessory]));}
 else if(mode==2){a->storage.live=false;}
 else if(mode==3){NativeScriptValue output;CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_DamageReceived,NULL,0,&output)==NativeScriptVM_Error);}
 else if(mode==4){Item_8026A8EC(object);}
 else if(mode==5){object->hsd_obj=&joints[7];}
 else if(mode==6){bank.storage_layout.identity^=1;}
 else if(mode==7){program.identity^=1;}
 else if(mode==8){object->classifier=HSD_GOBJ_CLASS_FIGHTER;}
 else if(mode==9){HSD_GObjFree(object);replacement=create(true);}
 else if(mode==10){((Item*)object->user_data)->owner=NULL;}
}
void transition_animation(HSD_GObj* object)
{++animation_calls;if(nested_animation){nested_animation=false;Item_80268E5C(object,0,ITEM_ANIM_UPDATE);CHECK(nativeScriptItem_BindCallback(object,NativeScriptItemCallback_Accessory,program.hooks[NativeScriptItemHook_Accessory]));}}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Item,.generation=((Item*)object->user_data)->x1C,.scene_epoch=nativeGObjPool.lifetime};}
static bool invoke(HSD_GObj* object,uint32_t semantic,NativeScriptValue* args,unsigned n,NativeScriptValue* out)
{NativeScriptRuntimeContext c=context(object);return nativeScriptBindingSpawn_Invoke(&c,semantic,args,n,out,&diagnostic);}
static void seed(Item* item,unsigned flags)
{item->xC40=9;item->xC44=3.5f;item->xCC8_knockback=17;item->x5CC_currentAnimFrame=33;item->x524_cmd.loop_count=2;item->x524_cmd.timer=11;item->xDC8_word.flags.x16=1;item->xDC8_word.flags.x2=(flags>>8)&1;item->xD09=12;item->xDD0_flag.b5=1;}
static void compare(const Item* a,const Item* b)
{CHECK(a->msid==b->msid);CHECK(a->anim_id==b->anim_id);CHECK(a->xC40==b->xC40);CHECK(a->xCC8_knockback==b->xCC8_knockback);CHECK(a->x5CC_currentAnimFrame==b->x5CC_currentAnimFrame);CHECK(a->xD0_itemStateDesc==b->xD0_itemStateDesc);CHECK(a->x524_cmd.u==b->x524_cmd.u);CHECK(a->x524_cmd.loop_count==b->x524_cmd.loop_count);CHECK(a->x524_cmd.timer==b->x524_cmd.timer);CHECK(a->xD09==b->xD09);CHECK(a->xDD0_flag.b5==b->xDD0_flag.b5);}
static void deferred_transition(HSD_GObj* object)
{NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=77},before=out;injection=4;++vm_calls;CHECK(nativeScriptItem_RunHook(object,NativeScriptItemHook_PickedUp,NULL,0,&out)==NativeScriptVM_SourceRetired);CHECK(!memcmp(&before,&out,sizeof(out)));deferred_stopped=nativeScriptItem_IsAttached(object)&&((Item*)object->user_data)->native_removal_in_progress&&actor(object)->storage.live==false;}
int main(void)
{
 prepare();HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);scheduler_reset();
 HSD_GObj* owned=create(true);HSD_GObj* reference=create(false);NativeScriptValue args[2]={{.type=NativeScriptType_I32,.as.i64=1},{.type=NativeScriptType_U32}},output;
 for(unsigned i=0;i<512;++i){args[1].as.u64=i;seed(owned->user_data,i);seed(reference->user_data,i);unsigned before=scheduler_tail;Item_80268E5C(reference,1,(Item_StateChangeFlags)i);unsigned native_tail=scheduler_tail-before;before=scheduler_tail;CHECK(invoke(owned,NativeScriptBinding_ItemChangeStateFlags,args,2,&output));CHECK(output.type==NativeScriptType_Void);CHECK(scheduler_tail-before==native_tail);compare(owned->user_data,reference->user_data);++pairs;}
 ++cases;NativeScriptValue result;CHECK(nativeScriptItem_RunHook(owned,NativeScriptItemHook_Reflected,NULL,0,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(((Item*)owned->user_data)->msid==0&&!result.as.u64);
 seed(owned->user_data,0);seed(reference->user_data,0);Item_80268E5C(reference,1,ITEM_ANIM_UPDATE);CHECK(nativeScriptItem_RunHook(owned,NativeScriptItemHook_PickedUp,NULL,0,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(((Item*)owned->user_data)->msid==1&&read(owned,"tail")==1);compare(owned->user_data,reference->user_data);++pairs;++cases;
 Item_80268E5C(owned,0,0);set(owned,"flags",(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=2});CHECK(nativeScriptItem_RunPhase(owned,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(((Item*)owned->user_data)->msid==1&&read(owned,"tail")==2);CHECK(nativeScriptItem_RunPhase(owned,NativeScriptItemPhase_Physics,&result)==NativeScriptVM_Ok);++vm_calls;CHECK(read(owned,"phase")==100);++cases;
 set(owned,"phase",(NativeScriptValue){.type=NativeScriptType_I32});injection=1;CHECK(invoke(owned,NativeScriptBinding_ItemChangeStateFlags,args,2,&output));CHECK(((Item*)owned->user_data)->msid==0&&nativeScriptItem_HookBound(owned,NativeScriptItemHook_Accessory));++cases;
 nested_animation=true;args[1].as.u64=2;CHECK(invoke(owned,NativeScriptBinding_ItemChangeStateFlags,args,2,&output));CHECK(((Item*)owned->user_data)->msid==0&&nativeScriptItem_HookBound(owned,NativeScriptItemHook_Accessory));++cases;
 args[1].as.u64=UINT32_MAX;CHECK(invoke(owned,NativeScriptBinding_ItemChangeStateFlags,args,2,&output));++cases;
 CHECK(nativeScriptItem_BindCallback(owned,NativeScriptItemCallback_Accessory,program.hooks[NativeScriptItemHook_Accessory]));CHECK(invoke(owned,NativeScriptBinding_ItemChangeState,args,1,&output));CHECK(!nativeScriptItem_HookBound(owned,NativeScriptItemHook_Accessory));++cases;
 NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_C(0xdeadbeef12345678)};
 for(unsigned mode=0;mode<5;++mode){NativeScriptValue bad[2]={args[0],args[1]};if(mode==0)bad[0].as.i64=-1;else if(mode==1)bad[0].as.i64=2;else if(mode==2)bad[0].type=NativeScriptType_F32;else if(mode==3)bad[1].as.u64=UINT64_C(0x100000000);else bad[1].type=NativeScriptType_F32;output=sentinel;uint64_t revision=nativeScriptItem_Revision(owned);CHECK(!invoke(owned,NativeScriptBinding_ItemChangeStateFlags,bad,2,&output));CHECK(!memcmp(&output,&sentinel,sizeof(output))&&nativeScriptItem_Revision(owned)==revision);++cases;}
 NativeScriptItemActor* a=actor(owned);uint64_t revision=a->revision;a->revision=UINT64_MAX;output=sentinel;CHECK(!invoke(owned,NativeScriptBinding_ItemChangeState,args,1,&output));CHECK(!memcmp(&output,&sentinel,sizeof(output)));a->revision=revision;++cases;
 for(unsigned mode=2;mode<=10;++mode){if(mode==4||mode==9)continue;uint64_t layout=bank.storage_layout.identity,id=program.identity;void* saved_model=owned->hsd_obj;u16 classifier=owned->classifier;bool live=actor(owned)->storage.live;bool faulted=actor(owned)->faulted;injection=mode;output=sentinel;bool okay=invoke(owned,NativeScriptBinding_ItemChangeStateFlags,args,2,&output);CHECK(okay==(mode==10));if(mode!=10)CHECK(!memcmp(&output,&sentinel,sizeof(output)));bank.storage_layout.identity=layout;program.identity=id;owned->hsd_obj=saved_model;owned->classifier=classifier;actor(owned)->storage.live=live;actor(owned)->faulted=faulted;++cases;}
 injection=3;set(owned,"phase",(NativeScriptValue){.type=NativeScriptType_I32});uint64_t tail=read(owned,"tail");CHECK(nativeScriptItem_RunHook(owned,NativeScriptItemHook_PickedUp,NULL,0,&result)==NativeScriptVM_Error);++vm_calls;CHECK(read(owned,"tail")==tail);actor(owned)->faulted=false;++cases;
 injection=4;NativeMatchPoolReference retained;CHECK(nativeMatchPool_Reference(&nativeGObjPool,owned,&retained));output=sentinel;CHECK(!invoke(owned,NativeScriptBinding_ItemChangeState,args,1,&output));CHECK(!memcmp(&output,&sentinel,sizeof(output))&&nativeMatchPool_Resolve(retained)==NULL);++cases;
 owned=create(true);CHECK(nativeMatchPool_Reference(&nativeGObjPool,owned,&retained));injection=9;output=sentinel;CHECK(!invoke(owned,NativeScriptBinding_ItemChangeState,args,1,&output));CHECK(!memcmp(&output,&sentinel,sizeof(output))&&nativeMatchPool_Resolve(retained)==NULL&&replacement);++cases;
 Item_8026A8EC(replacement);HSD_GObjFree(reference);
 HSD_GObj* deferred=create(true);CHECK(nativeMatchPool_Reference(&nativeGObjPool,deferred,&retained));HSD_GObj_SetupProc(deferred,deferred_transition,0);HSD_GObj_80390CFC();CHECK(deferred_stopped&&nativeMatchPool_Resolve(retained)==NULL&&nativeScriptRuntime_Idle());++cases;
 printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"animation_calls\":%u}\n",checks,cases,pairs,vm_calls,animation_calls);return 0;
}
