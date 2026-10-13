#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_damage.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftcommon.h>
#include <melee/ef/eflib.h>
#include <melee/ef/types.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/generator.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,F=64,V=64};
static char bytes[32768];static NativeScriptSourceNode syntax[N];static NativeScriptIRNode expressions[N*2];static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[4096];static NativeScriptFunctionCode code[F];static NativeScriptExecutionFunction functions[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[8];static NativeScriptParameterSlot slots[V];
static NativeSourcePackage package;
#define bank package.compiler.bank
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=10,.ground={343,345,347,349},.air={344,346,348,350}};
static Fighter fighters[32];static MotionState motions[351];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static unsigned allocated,checks,cases,pairs,vm_calls;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"effects line%u %s\n",__LINE__,#x);exit(1);}}while(0)
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&package:NULL;}
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static void prepare(void)
{
 bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},.declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,.storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
 const char* source="fighter:\n    behavior independent\nstate int pre\nstate int post\nstate int tail\nfunc Pre():\n    state.pre = state.pre + 1\n    effects.pause()\nfunc Post():\n    state.post = state.post + 1\n    effects.resume()\nfunc NativeHitlag():\n    effects.pause_during_hitlag()\nfunc ScriptHitlag():\n    callbacks.pre_hitlag(Pre)\n    callbacks.post_hitlag(Post)\nfunc ClearHitlag():\n    callbacks.pre_hitlag(None)\n    callbacks.post_hitlag(None)\nfunc Pause():\n    effects.pause()\nfunc Resume():\n    effects.resume()\nfunc Clear():\n    effects.clear_all()\nfunc Destroy():\n    effects.destroy_all()\n    state.tail = 99\nfunc Fault():\n    let int zero = 0\n    state.tail = 1 / zero\n";
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
 if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&package.program,&d)){
  fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);
 }
 package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;package.assets.facts.moves=layout;
 gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
 CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
}
static HSD_GObj* create(void)
{
 CHECK(allocated<32);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));
 f->gobj=object;f->kind=f->behavior_kind=Ft_Kind_CustomFirst;f->ft_data=&package.assets.character.data;f->x8_spawnNum=allocated;f->player_id=2;f->x34_scale.y=1;
 GObj_InitUserData(object,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(object));return object;
}
static uint32_t function_id(const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,declarations[i].name,name))return i;CHECK(false);return UINT32_MAX;}
static bool helper(HSD_GObj* object,const char* name,const NativeScriptValue* args,unsigned count,NativeScriptValue* out)
{
 ++vm_calls;NativeScriptDiagnostic d={0};bool result=nativeScriptFighter_Helper(object,((Fighter*)object->user_data)->x8_spawnNum,function_id(name),args,count,out,&d)==NativeScriptVM_Ok;
 if(!result&&strcmp(name,"Destroy") && strcmp(name,"Fault"))fprintf(stderr,"helper %s %s\n",name,d.message);return result;
}
static NativeScriptRuntimeContext context(HSD_GObj* object)
{return(NativeScriptRuntimeContext){.source=object,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)object->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789abcdef)};}

EF_ParamEntry efLib_ParamTable[8];
static EF_Effect effects[8];static HSD_GObj* effect_objects[8];
static unsigned effect_used,effect_frees,walks,callback_mode;static HSD_GObj* cancel_owner;
void HSD_JObjWalkTree(HSD_JObj* object,HSD_JObjWalkTreeCallback cb,f32** args){(void)object;(void)cb;(void)args;++walks;}
void hsd_8039D688(HSD_JObj* object,f32** args,s32 n){(void)object;(void)args;(void)n;abort();}
void nativeModelEffect_Cancel(EF_Effect* effect)
{
    HSD_GObj* o=effect->gobj;effect->native_cancelled=true;HSD_GObjFree(o);
    unsigned mode=callback_mode;callback_mode=0;
    if(mode==1){NativeScriptValue v=sentinel();CHECK(!helper(cancel_owner,"Fault",NULL,0,&v));}
    if(mode==2)HSD_GObjFree(cancel_owner);
    if(mode==3)((HSD_GObj*)cancel_owner)->user_data=&fighters[31];
    if(mode==4)++bank.storage_layout.identity;
}
void nativeFighterDamage_Self(Fighter_GObj* o,float v){(void)o;(void)v;abort();}
void nativeFighterDamage_Heal(Fighter_GObj* o,float v){(void)o;(void)v;abort();}
void ftColl_8007B0C0(Fighter_GObj* o,HurtCapsuleState v){(void)o;(void)v;abort();}
void ftColl_8007B62C(Fighter_GObj* o,int v){(void)o;(void)v;abort();}
s32 ftColl_8007B868(Fighter_GObj* o){(void)o;abort();}
s32 HSD_Randi(s32 v){(void)v;abort();}
#include "native_effects.inc"
static void effect_removed(void* data){(void)data;++effect_frees;}
static void lists(HSD_GObj* owner,HSD_GObj* foreign,bool native)
{
    CHECK(!effect_used);effect_frees=walks=0;memset(effects,0,sizeof(effects));
    for(unsigned i=0;i<4;++i){HSD_GObj* o=GObj_Create(0,i<2?HSD_GOBJ_PLINK_EFFECT_UNK0:HSD_GOBJ_PLINK_EFFECT_UNK1,0);CHECK(o);
        effect_objects[i]=o;effects[i].gobj=o;effects[i].parent_gobj=i%2?foreign:owner;effects[i].state_flags=(u8)(0x80|i);
        if(native&&i%2==0)effects[i].native_policy=(void*)1;GObj_InitUserData(o,1,effect_removed,&effects[i]);++effect_used;}
    memset(efLib_ParamTable,0,sizeof(efLib_ParamTable));efLib_ParamTable[0].gobj=owner;efLib_ParamTable[1].gobj=foreign;
}
static void clear_lists(void)
{
    for(unsigned i=0;i<effect_used;++i){NativeMatchPoolReference r;if(nativeMatchPool_Reference(&nativeGObjPool,effect_objects[i],&r))HSD_GObjFree(effect_objects[i]);}
    effect_used=0;
}
static int state(HSD_GObj* object,const char* name)
{
    NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));
    for(unsigned i=0;i<view.program->value_count;++i)if(nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){
        NativeScriptValue v;NativeScriptDiagnostic d={0};CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&v,&d));return(int)v.as.i64;}
    abort();
}
static bool dispatch(HSD_GObj* object,uint32_t slot)
{
    // Scheduling is controlled. Ownership/selection/dispatch below is actual.
    if(nativeScriptFighter_CallbackBound(object,slot))return nativeScriptFighter_RunCallback(object,slot);
    Fighter* f=object->user_data;HSD_GObjEvent cb=slot==NativeScriptFighterCallback_PreHitlag?f->pre_hitlag_cb:f->post_hitlag_cb;
    if(cb)cb(object);return true;
}
static void ownership(HSD_GObj* owner,HSD_GObj* foreign)
{
    const char* names[]={"Pause","Resume"};
    for(unsigned op=0;op<2;++op)for(unsigned seed=0;seed<8;++seed){++cases;lists(owner,foreign,false);
        for(unsigned i=0;i<4;++i)effects[i].state_flags=(u8)((i%2?0:0x80)|seed);u8 before[4],expected[4];
        for(unsigned i=0;i<4;++i)before[i]=effects[i].state_flags;
        if(op)efLib_ResumeAll(owner);else efLib_PauseAll(owner);for(unsigned i=0;i<4;++i){expected[i]=effects[i].state_flags;effects[i].state_flags=before[i];}
        NativeScriptValue out=sentinel();CHECK(helper(owner,names[op],NULL,0,&out));CHECK(out.type==NativeScriptType_Void);
        for(unsigned i=0;i<4;++i){CHECK(effects[i].state_flags==expected[i]);if(i%2)CHECK(effects[i].state_flags==before[i]);}++pairs;clear_lists();}
    ++cases;lists(owner,foreign,false);NativeScriptValue out=sentinel();CHECK(helper(owner,"ScriptHitlag",NULL,0,&out));
    CHECK(nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_PreHitlag));CHECK(dispatch(owner,NativeScriptFighterCallback_PreHitlag));CHECK(state(owner,"pre")==1);
    CHECK(dispatch(owner,NativeScriptFighterCallback_PostHitlag)&&state(owner,"post")==1);
    CHECK(helper(owner,"NativeHitlag",NULL,0,&out));Fighter* f=owner->user_data;
    CHECK(f->pre_hitlag_cb==efLib_PauseAll&&f->post_hitlag_cb==efLib_ResumeAll);
    CHECK(!nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_PreHitlag));CHECK(dispatch(owner,NativeScriptFighterCallback_PreHitlag));
    CHECK(effects[0].state_flags==(EF_STATE_ASYNC|EF_STATE_ENTERING_PAUSE)&&effects[1].state_flags==0x81);
    CHECK(dispatch(owner,NativeScriptFighterCallback_PostHitlag)&&effects[2].state_flags==EF_STATE_ASYNC);
    CHECK(state(owner,"pre")==1&&state(owner,"post")==1);
    CHECK(helper(owner,"ScriptHitlag",NULL,0,&out)&&f->pre_hitlag_cb==NULL&&f->post_hitlag_cb==NULL);
    CHECK(dispatch(owner,NativeScriptFighterCallback_PreHitlag)&&state(owner,"pre")==2);
    CHECK(helper(owner,"ClearHitlag",NULL,0,&out));CHECK(!nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_PreHitlag));
    CHECK(!nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_PostHitlag)&&!f->pre_hitlag_cb&&!f->post_hitlag_cb);
    clear_lists();
    for(unsigned clear=0;clear<2;++clear)for(unsigned native=0;native<2;++native){++cases;lists(owner,foreign,native!=0);f->x2219_b0=true;
        CHECK(helper(owner,clear?"Clear":"Destroy",NULL,0,&out));CHECK(effect_frees==2&&efLib_ParamTable[0].gobj==NULL&&efLib_ParamTable[1].gobj==foreign);
        CHECK(f->x2219_b0==(clear==0));NativeMatchPoolReference r;CHECK(nativeMatchPool_Reference(&nativeGObjPool,effect_objects[1],&r));CHECK(nativeMatchPool_Reference(&nativeGObjPool,effect_objects[3],&r));clear_lists();}
}
static void negatives(HSD_GObj* owner)
{
    for(unsigned mode=0;mode<7;++mode){++cases;NativeScriptRuntimeContext c=context(owner);NativeScriptValue out=sentinel(),saved=out;NativeScriptDiagnostic d={0};Fighter* f=owner->user_data;
        uint32_t count=0;if(mode==0)c.source_class=NativeCallbackSource_Item;if(mode==1)++c.generation;if(mode==2)++c.scene_epoch;
        if(mode==3)count=1;if(mode==4)owner->classifier=HSD_GOBJ_CLASS_ITEM;if(mode==5)package.ready=false;if(mode==6)f->behavior_kind=Ft_Kind_Link;
        CHECK(!nativeScriptBindingCombat_Invoke(&c,NativeScriptBinding_EffectsPause,NULL,count,&out,&d));CHECK(!memcmp(&out,&saved,sizeof(out)));
        owner->classifier=HSD_GOBJ_CLASS_FIGHTER;package.ready=true;f->behavior_kind=Ft_Kind_CustomFirst;}
    for(unsigned mode=1;mode<=4;++mode){++cases;HSD_GObj* o=create();Fighter* data=o->user_data;lists(o,owner,true);cancel_owner=o;callback_mode=mode;
        NativeScriptValue out=sentinel(),saved=out;uint64_t identity=bank.storage_layout.identity;
        CHECK(!helper(o,"Destroy",NULL,0,&out)&&!memcmp(&out,&saved,sizeof(out)));CHECK(effect_frees>=1&&nativeScriptRuntime_Idle());
        bank.storage_layout.identity=identity;if(mode!=2){o->user_data=data;HSD_GObjFree(o);}clear_lists();}
}
static void recipient_case(HSD_GObj* owner,HSD_GObj* recipient)
{
    ++cases;nativeScriptFighter_Detach(recipient);((Fighter*)recipient->user_data)->kind=Ft_Kind_None;
    lists(recipient,owner,false);NativeScriptRuntimeContext caller=context(owner);caller.actor_id=caller.generation;
    NativeScriptReceiverScope scope={.pool=nativeScriptRuntime_Receivers()};NativeScriptDiagnostic d={0};
    CHECK(nativeScriptReceiver_Push(scope.pool,&caller,nativeGObjPool.lifetime,&bank.declarations,Ft_Kind_CustomFirst,
        nativeScriptActors_Lookup,nativeScriptActors_Helper,NULL,&scope.frame,&d));
    NativeScriptValue external,retained,out=sentinel();CHECK(nativeScriptReceiver_CaptureExternal(recipient,NativeCallbackSource_Fighter,nativeGObjPool.lifetime,&external,&d));
    CHECK(nativeScriptReceiver_Import(&scope,&caller,&external,nativeGObjPool.lifetime,&retained,&d));
    CHECK(nativeScriptBindingReceiver_Invoke(&scope,&caller,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_EffectsPause,&retained,1,NULL,0,&out,&d));
    CHECK(out.type==NativeScriptType_Void&&effects[0].state_flags==(EF_STATE_ASYNC|EF_STATE_ENTERING_PAUSE)&&effects[1].state_flags==0x81);
    out=sentinel();NativeScriptValue before=out;
    CHECK(!nativeScriptBindingReceiver_Invoke(&scope,&caller,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_EffectsPauseDuringHitlag,&retained,1,NULL,0,&out,&d));
    CHECK(!memcmp(&out,&before,sizeof(out)));HSD_GObj* saved=recipient;HSD_GObjFree(recipient);
    HSD_GObj* replacement=create();NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeGObjPool,replacement,&ref));
    CHECK(replacement==saved);out=before;
    CHECK(!nativeScriptBindingReceiver_Invoke(&scope,&caller,NATIVE_SCRIPT_RECEIVER_NATIVE|NativeScriptBinding_EffectsResume,&retained,1,NULL,0,&out,&d));
    CHECK(!memcmp(&out,&before,sizeof(out)));CHECK(nativeScriptReceiver_Pop(scope.pool,scope.frame,&d));
    clear_lists();HSD_GObjFree(replacement);
}
static void callback_snapshot(HSD_GObj* owner)
{
    ++cases;NativeScriptValue out=sentinel();CHECK(helper(owner,"ScriptHitlag",NULL,0,&out));
    static uint8_t saved[65536];size_t used;CHECK(nativeScriptFighter_Save(saved,sizeof(saved),&used));
    CHECK(helper(owner,"NativeHitlag",NULL,0,&out));CHECK(!nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_PreHitlag));
    // Component restore requires matching native callback slots as a peer.
    Fighter* f=owner->user_data;f->pre_hitlag_cb=NULL;f->post_hitlag_cb=NULL;
    CHECK(nativeScriptFighter_Load(saved,used));CHECK(nativeScriptFighter_CallbackBound(owner,NativeScriptFighterCallback_PreHitlag));
    int count=state(owner,"pre");CHECK(dispatch(owner,NativeScriptFighterCallback_PreHitlag)&&state(owner,"pre")==count+1);
    CHECK(helper(owner,"ClearHitlag",NULL,0,&out));
    ++cases;HSD_GObj* fault=create();CHECK(nativeScriptFighter_BindCallback(fault,NativeScriptFighterCallback_PreHitlag,function_id("Fault")));
    CHECK(!dispatch(fault,NativeScriptFighterCallback_PreHitlag)&&nativeScriptFighter_Status(fault)==NativeScriptVM_Error);HSD_GObjFree(fault);
}
int main(void)
{
    HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();
    HSD_GObj* owner=create(),*foreign=create();ownership(owner,foreign);negatives(owner);callback_snapshot(owner);recipient_case(owner,foreign);
    HSD_GObjFree(owner);CHECK(nativeScriptRuntime_Idle());
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_helper_calls\":%u}\n",checks,cases,pairs,vm_calls);return 0;
}
