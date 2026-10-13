#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_fighter_snapshot.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""
enum{N=2048,I=8192,F=64,V=128};
NativeSourcePackage target_package;
#define bank target_package.compiler.bank
static char source_bytes[32768];static NativeScriptSourceNode nodes[N];static NativeScriptIRNode ir[I];static uint32_t arguments[I];
static NativeScriptFunctionDeclaration functions[F];static NativeScriptParameterDeclaration parameters[V];static NativeScriptValueDeclaration values[V];
static NativeScriptLocal locals[V];static NativeScriptInstruction instructions[I];static NativeScriptFunctionCode codes[F];static NativeScriptExecutionFunction entries[F];static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V];static NativeScriptStorageOwner owners[V];static NativeScriptParameterSlot slots[V];
static Article article;static ItemAttr attributes;static ItemModelDesc model;static Fighter fighters[128],alternate;static Item items[64];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=tables;
static HSD_JObj joints[128];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*replacement;
static unsigned checks,fighter_count,item_count,comparisons,vm_calls,compile_cases,mutation_cases,boundary_cases;static NativeScriptDiagnostic diagnostic;
static unsigned scheduler_pairs,scheduler_cases;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"callback velocity %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
static bool compile_source(const char* source)
{
    bank=(NativeScriptCompileBank){.source_storage=source_bytes,.source_capacity=sizeof(source_bytes),.tree={.nodes=nodes,.capacity=N},.expressions={.nodes=ir,.node_capacity=I,.arguments=arguments,.argument_capacity=I},
        .declarations={.functions=functions,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=V,
        .instructions=instructions,.instruction_capacity=I,.function_code=codes,.function_code_capacity=F,.execution_functions=entries,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=V},.parameter_slots=slots,.parameter_slot_capacity=V};
    article=(Article){.x0_common_attr=&attributes,.x10_modelDesc=&model};NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
    const NativeScriptCompileBank* accepted=NULL;++compile_cases;
    return nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic);
}
static void prepare(void)
{
    const char* source="fighter:\n    behavior independent\nstate int handle\nstate int observed\nfunc authored():\n    state.observed = state.observed + 1\nfunc before():\n    state.observed = state.observed + 10\n    if state.handle == 1:\n        callbacks.accessory_before(None)\n    if state.handle == 2:\n        callbacks.accessory_before(replacement_before)\n    if state.handle == 3:\n        state.observed = 1 / (state.handle - 3)\nfunc replacement_before():\n    state.observed = state.observed + 20\nfunc during_hitlag():\n    state.observed = state.observed + 100\n    if state.handle == 1:\n        callbacks.accessory_hitlag(None)\n    if state.handle == 3:\n        state.observed = 1 / (state.handle - 3)\nfunc failure():\n    state.observed = 1 / state.handle\nfunc wrong_result() -> int:\n    return 1\nfunc wrong_argument(int value):\n    pass\naction Queries:\n    on tick:\n        callbacks.reflect(authored)\n        callbacks.align_velocity_to_facing()\n    on reflect:\n        state.observed = state.observed + 2\narticle Probe states 1:\n    state int handle\n    state int observed\n    state 0 anim -1:\n";
    CHECK(compile_source(source));
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);o->hsd_obj=&joints[fighter_count];Fighter* f=&fighters[fighter_count++];
    *f=(Fighter){.gobj=o,.kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,.behavior_kind=owned?Ft_Kind_CustomFirst:Ft_Kind_Link,
        .x8_spawnNum=id,.ft_data=&target_package.assets.character.data,.cur_pos={1.25f,-2.5f,3.75f}};
    GObj_InitUserData(o,4,fighter_remove,f);if(owned)CHECK(nativeScriptFighter_Attach(o));return o;
}
static HSD_GObj* item(void)
{
    CHECK(item_count<64);const NativeScriptItemProgram* p=&nativeScriptFighter_Program(Ft_Kind_CustomFirst)->items.programs[0];
    HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);Item* i=&items[item_count++];
    *i=(Item){.entity=o,.kind=p->kind,.msid=-1,.native_script_owner=p->owner,.native_article_author_kind=p->author_kind,.x1C=100+item_count,.xC4_article_data=&article};
    GObj_InitUserData(o,6,item_remove,i);CHECK(nativeScriptItem_Attach(o,p));return o;
}
static NativeScriptValue field(HSD_GObj* o,const char* name,const NativeScriptValue* input)
{
    bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;
    NativeScriptReceiverView view;CHECK(f?nativeScriptFighter_View(o,gen,&view):nativeScriptItem_View(o,gen,&view));
    for(unsigned n=0;n<bank.declarations.value_count;++n)if(values[n].owner==view.storage->owner&&nativeScriptSource_SpanEquals(&bank.tree,values[n].name,name)){
        NativeScriptValue result;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|n,NULL,0,&result,&diagnostic));return result;
    }CHECK(false);return(NativeScriptValue){0};
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{bool f=o->classifier==HSD_GOBJ_CLASS_FIGHTER;uint32_t gen=f?((Fighter*)o->user_data)->x8_spawnNum:((Item*)o->user_data)->x1C;return(NativeScriptRuntimeContext){.source=o,.source_class=f?NativeCallbackSource_Fighter:NativeCallbackSource_Item,.generation=gen,.actor_id=gen,.scene_epoch=nativeGObjPool.lifetime};}
static HSD_GObj* reuse(void){replacement=fighter(false,UINT32_C(0x80000005));return replacement;}
static void original_callback(HSD_GObj* object){(void)object;abort();}
static uint32_t function(const char* name)
{for(unsigned i=0;i<bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,name))return i;CHECK(false);return UINT32_MAX;}
void target_guard_probe(NativeScriptRuntimeContext* c)
{
    if(c->source!=query_caller)return;++guard_calls;if(!mutation||guard_calls!=mutate_at)return;
    unsigned mode=mutation;mutation=0;Fighter* data=query_caller->user_data;
    if(mode==1)data->x8_spawnNum++;
    if(mode==2)data->kind=Ft_Kind_Zelda;
    if(mode==3){alternate=*data;query_caller->user_data=&alternate;}
    if(mode==4){HSD_GObjFree(query_caller);replacement=reuse();}
    if(mode==5)bank.storage_layout.identity++;
    if(mode==6)bank.storage_parameters.identity++;
    if(mode==7)target_package.program.identity++;
    if(mode==8)data->ft_data=NULL;
    if(mode==9)nativeScriptFighter_Detach(query_caller);
}
static void compiler_contract(void)
{
    CHECK(compile_source("fighter:\n    behavior independent\nfunc install():\n    callbacks.align_velocity_to_facing()\n"));
    CHECK(!compile_source("fighter:\n    behavior independent\nfunc install():\n    callbacks.align_velocity_to_facing(1)\n"));
    CHECK(!compile_source("fighter:\n    behavior independent\nfunc install() -> int:\n    return callbacks.align_velocity_to_facing()\n"));
    CHECK(!compile_source("fighter:\n    behavior independent\narticle Probe states 1:\n    on created:\n        callbacks.align_velocity_to_facing()\n    state 0 anim -1:\n"));
    const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(NativeScriptBinding_CallbackAlignVelocityToFacing);
    CHECK(d&&d->argument_count==0&&d->result_type==NativeScriptType_Void&&(d->flags&NativeScriptFlag_SourceOnly));
    const char* slots[]={"accessory_before","accessory_hitlag"};
    for(unsigned n=0;n<2;++n){char source[512];
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc owned():\n    pass\nfunc install():\n    callbacks.%s(owned)\n    callbacks.%s(None)\n    if callbacks.matches(Callback.%s, owned):\n        pass\n",slots[n],slots[n],slots[n]);CHECK(compile_source(source));
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc install():\n    callbacks.%s(1)\n",slots[n]);CHECK(!compile_source(source));
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc install():\n    callbacks.%s()\n",slots[n]);CHECK(!compile_source(source));
        snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle Probe states 1:\n    on created:\n        callbacks.%s(None)\n    state 0 anim -1:\n",slots[n]);CHECK(!compile_source(source));
    }
}

void reference_accessory_scheduler(Fighter_GObj*);
static HSD_GObj* scheduler_object;
static bool scheduler_reference;
static int reference_observed;
static unsigned scheduler_mutation,trace_count;
static struct { int event,observed;Vec3 translation; } trace[8];
static int observed(void)
{return scheduler_reference?reference_observed:(int)field(scheduler_object,"observed",NULL).as.i64;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint)
{
    CHECK(scheduler_object&&scheduler_object->hsd_obj==joint&&trace_count<8);
    trace[trace_count].event=2;trace[trace_count].observed=observed();trace[trace_count++].translation=joint->translate;
}
static void native_before(HSD_GObj* object)
{CHECK(object==scheduler_object);if(scheduler_reference)reference_observed+=10;else{NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=observed()+10};field(object,"observed",&value);}}
static void native_hitlag(HSD_GObj* object)
{CHECK(object==scheduler_object);if(scheduler_reference)reference_observed+=100;else{NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=observed()+100};field(object,"observed",&value);}}
static void native_after(HSD_GObj* object)
{
    CHECK(object==scheduler_object&&trace_count<8);
    trace[trace_count].event=1;trace[trace_count].observed=observed();trace[trace_count++].translation=((HSD_JObj*)object->hsd_obj)->translate;
    Fighter* data=object->user_data;data->cur_pos.y+=1;
    unsigned mode=scheduler_mutation;scheduler_mutation=0;
    if(mode==1)data->x8_spawnNum++;
    if(mode==2){alternate=*data;object->user_data=&alternate;}
    if(mode==3){HSD_GObjFree(object);replacement=reuse();}
    if(mode==4){NativeScriptValue output;CHECK(nativeScriptFighter_Helper(object,data->x8_spawnNum,function("failure"),NULL,0,&output,&diagnostic)==NativeScriptVM_Error);}
    if(mode==5)nativeScriptFighter_Detach(object);
    if(mode==6)((NativeScriptFighterProgram*)nativeScriptFighter_Program(data->kind))->identity++;
    if(mode==7)bank.storage_layout.identity++;
    if(mode==8)bank.storage_parameters.identity++;
}
static void set_integer(HSD_GObj* object,const char* name,int value)
{NativeScriptValue input={.type=NativeScriptType_I32,.as.i64=value};field(object,name,&input);}
static void bind_accessory(HSD_GObj* object,unsigned slot,uint32_t helper)
{
    NativeScriptRuntimeContext c=context(object);NativeScriptValue input={.type=helper==UINT32_MAX?NativeScriptType_Null:NativeScriptType_Callable,.as.u64=helper},output={0};
    uint32_t semantic=slot==NativeScriptFighterCallback_AccessoryBefore?NativeScriptBinding_CallbackAccessoryBefore:NativeScriptBinding_CallbackAccessoryHitlag;
    CHECK(nativeScriptBindings_Invoke(&c,semantic,NULL,0,&input,1,&output,&diagnostic));CHECK(output.type==NativeScriptType_Void);
}
static bool matching(HSD_GObj* object,unsigned slot,uint32_t helper)
{
    NativeScriptRuntimeContext c=context(object);NativeScriptValue input[]={{.type=NativeScriptType_I32,.as.i64=slot},{.type=helper==UINT32_MAX?NativeScriptType_Null:NativeScriptType_Callable,.as.u64=helper}},output={0};
    CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CallbackMatches,NULL,0,input,2,&output,&diagnostic));CHECK(output.type==NativeScriptType_Bool);return output.as.u64!=0;
}
static void accessory_contract(void)
{
    const unsigned before=NativeScriptFighterCallback_AccessoryBefore,hitlag=NativeScriptFighterCallback_AccessoryHitlag;
    const uint32_t first=function("before"),second=function("replacement_before"),during=function("during_hitlag");
    HSD_GObj* object=fighter(true,110);Fighter* data=object->user_data;
    bind_accessory(object,before,first);bind_accessory(object,hitlag,during);
    CHECK(data->accessory2_cb==NULL&&data->accessory3_cb==NULL&&matching(object,before,first)&&matching(object,hitlag,during));
    nativeScriptFighter_ResetMotionCallbacks(object);nativeScriptFighter_SelectMotion(object,341);
    CHECK(matching(object,before,first)&&matching(object,hitlag,during));++scheduler_cases;
    for(unsigned s=before;s<=hitlag;++s){
        CHECK(!nativeScriptFighter_BindCallback(object,s,function("wrong_argument")));
        CHECK(!nativeScriptFighter_BindCallback(object,s,function("wrong_result")));
        bool result;CHECK(!nativeScriptFighter_CallbackMatches(object,s,function("wrong_argument"),&result));
        CHECK(!nativeScriptFighter_CallbackMatches(object,s,function("wrong_result"),&result));
    }
    static unsigned char snapshot[65536];size_t written=0;
    CHECK(nativeScriptFighter_Save(snapshot,sizeof(snapshot),&written));CHECK(nativeScriptFighter_ValidateSnapshot(snapshot,written));
    bind_accessory(object,before,second);bind_accessory(object,hitlag,UINT32_MAX);
    CHECK(!matching(object,before,first)&&matching(object,before,second)&&matching(object,hitlag,UINT32_MAX));
    CHECK(nativeScriptFighter_Load(snapshot,written));CHECK(matching(object,before,first)&&matching(object,hitlag,during));
    NativeScriptFighterSnapshotRecord* record=(void*)(snapshot+sizeof(NativeScriptFighterSnapshotHeader));
    uint32_t saved=record->callbacks[before];record->callbacks[before]=function("wrong_result");
    CHECK(!nativeScriptFighter_ValidateSnapshot(snapshot,written));record->callbacks[before]=saved;CHECK(nativeScriptFighter_ValidateSnapshot(snapshot,written));++scheduler_cases;
    for(unsigned s=before;s<=hitlag;++s){
        HSD_GObjEvent* pointer=s==before?&data->accessory2_cb:&data->accessory3_cb;*pointer=original_callback;
        CHECK(!matching(object,s,s==before?first:during)&&!matching(object,s,UINT32_MAX));
        CHECK(!nativeScriptFighter_CallbackBound(object,s));*pointer=NULL;CHECK(matching(object,s,UINT32_MAX));
        bind_accessory(object,s,s==before?first:during);CHECK(*pointer==NULL);++scheduler_cases;
    }
    set_integer(object,"observed",17);data->x8_spawnNum++;nativeScriptFighter_NewLife(object,data->x8_spawnNum);
    CHECK(matching(object,before,UINT32_MAX)&&matching(object,hitlag,UINT32_MAX));CHECK(field(object,"observed",NULL).as.i64==17);++scheduler_cases;
    HSD_GObjFree(object);
    for(unsigned disabled=0;disabled<2;++disabled)for(unsigned lag=0;lag<2;++lag)for(unsigned have_before=0;have_before<2;++have_before)for(unsigned have_after=0;have_after<2;++have_after){
        HSD_GObj* stock=fighter(false,130+fighter_count);Fighter* expected=stock->user_data;object=fighter(true,130+fighter_count);data=object->user_data;
        expected->x221F_b3=data->x221F_b3=disabled;expected->x2219_b5=data->x2219_b5=lag;
        expected->accessory2_cb=have_before?native_before:NULL;expected->accessory3_cb=native_hitlag;expected->accessory1_cb=have_after?native_after:NULL;
        data->accessory1_cb=have_after?native_after:NULL;data->accessory4_cb=original_callback;data->throw_flags_b0=true;
        if(have_before)bind_accessory(object,before,first);bind_accessory(object,hitlag,during);
        scheduler_object=stock;scheduler_reference=true;reference_observed=0;trace_count=0;
        reference_accessory_scheduler(stock);int final_observed=reference_observed;unsigned count=trace_count;
        unsigned char expected_trace[sizeof(trace)];memcpy(expected_trace,trace,sizeof(trace));Vec3 translation=((HSD_JObj*)stock->hsd_obj)->translate;
        scheduler_object=object;scheduler_reference=false;trace_count=0;memset(trace,0,sizeof(trace));
        Fighter_CallAcessoryCallbacks_8006C624(object);
        CHECK(observed()==final_observed&&trace_count==count&&memcmp(trace,expected_trace,count*sizeof(trace[0]))==0);
        CHECK(memcmp(&translation,&((HSD_JObj*)object->hsd_obj)->translate,sizeof(translation))==0);
        CHECK(memcmp(&expected->cur_pos,&data->cur_pos,sizeof(Vec3))==0&&data->throw_flags_b0&&data->accessory4_cb==original_callback);
        ++scheduler_pairs;HSD_GObjFree(object);HSD_GObjFree(stock);
    }
    for(unsigned mode=1;mode<=3;++mode){
        object=fighter(true,170+fighter_count);data=object->user_data;bind_accessory(object,before,first);set_integer(object,"handle",mode);
        scheduler_object=object;scheduler_reference=false;trace_count=0;data->accessory1_cb=native_after;
        Fighter_CallAcessoryCallbacks_8006C624(object);
        if(mode==3){CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error&&trace_count==0);}
        else{
            CHECK(observed()==10&&trace_count==3);CHECK(matching(object,before,mode==1?UINT32_MAX:second));
            trace_count=0;Fighter_CallAcessoryCallbacks_8006C624(object);CHECK(observed()==(mode==1?10:30));
        }
        ++scheduler_cases;HSD_GObjFree(object);
    }
    for(unsigned mode=1;mode<=3;mode+=2){
        object=fighter(true,190+fighter_count);data=object->user_data;bind_accessory(object,hitlag,during);set_integer(object,"handle",mode);
        scheduler_object=object;scheduler_reference=false;trace_count=0;data->x2219_b5=true;data->accessory1_cb=data->accessory2_cb=original_callback;
        Fighter_CallAcessoryCallbacks_8006C624(object);CHECK(trace_count==0);
        if(mode==1)CHECK(observed()==100&&matching(object,hitlag,UINT32_MAX));else CHECK(nativeScriptFighter_Status(object)==NativeScriptVM_Error);
        ++scheduler_cases;HSD_GObjFree(object);
    }
    object=fighter(true,190+fighter_count);data=object->user_data;bind_accessory(object,before,first);bind_accessory(object,hitlag,during);
    data->accessory2_cb=native_before;data->accessory3_cb=native_hitlag;scheduler_object=object;scheduler_reference=false;trace_count=0;
    Fighter_CallAcessoryCallbacks_8006C624(object);CHECK(observed()==10&&trace_count==1&&!matching(object,before,first));
    data->x2219_b5=true;trace_count=0;Fighter_CallAcessoryCallbacks_8006C624(object);CHECK(observed()==110&&trace_count==0&&!matching(object,hitlag,during));
    data->accessory2_cb=data->accessory3_cb=NULL;CHECK(matching(object,before,UINT32_MAX)&&matching(object,hitlag,UINT32_MAX));
    ++scheduler_cases;HSD_GObjFree(object);
    for(unsigned mode=1;mode<=8;++mode){
        object=fighter(true,200+fighter_count);data=object->user_data;bind_accessory(object,before,first);
        scheduler_object=object;scheduler_reference=false;trace_count=0;data->accessory1_cb=native_after;
        uint64_t program=nativeScriptFighter_Program(data->kind)->identity,layout=bank.storage_layout.identity,parameters=bank.storage_parameters.identity;
        scheduler_mutation=mode;Fighter_CallAcessoryCallbacks_8006C624(object);
        CHECK(scheduler_mutation==0&&trace_count==2);++scheduler_cases;
        ((NativeScriptFighterProgram*)nativeScriptFighter_Program(Ft_Kind_CustomFirst))->identity=program;bank.storage_layout.identity=layout;bank.storage_parameters.identity=parameters;
        if(mode==3){CHECK(replacement==object);HSD_GObjFree(replacement);}else HSD_GObjFree(object);
    }
    scheduler_object=NULL;CHECK(nativeScriptRuntime_Idle());
}
int main(void)
{
    compiler_contract();prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* object=fighter(true,21);Fighter* data=object->user_data;NativeScriptRuntimeContext c=context(object);NativeScriptValue output;
    uint32_t helper=function("authored");
    const uint32_t bits[]={0,UINT32_C(0x80000000),UINT32_C(0x3fa00000),UINT32_C(0xc0200000),UINT32_C(0x7f800000),UINT32_C(0xff800000),UINT32_C(0x7fc12345),UINT32_C(0xffc12345)};
    const float facing[]={-1,1};
    for(unsigned n=0;n<sizeof(bits)/sizeof(*bits);++n)for(unsigned f=0;f<2;++f){
        memcpy(&data->gr_vel,&bits[n],sizeof(float));memcpy(&data->self_vel.x,&bits[(n+3)%8],sizeof(float));
        data->self_vel.y=-3.75f;data->self_vel.z=4.25f;data->facing_dir=facing[f];
        CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Reflect,helper));
        CHECK(nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Reflect));
        Fighter before=*data,expected=before;expected.x21F8=ftCommon_8007F76C;
        output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CallbackAlignVelocityToFacing,NULL,0,NULL,0,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_Void&&memcmp(data,&expected,sizeof(expected))==0);
        CHECK(!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Reflect));
        bool matches=true;CHECK(nativeScriptFighter_CallbackMatches(object,NativeScriptFighterCallback_Reflect,helper,&matches)&&!matches);
        CHECK(nativeScriptFighter_CallbackMatches(object,NativeScriptFighterCallback_Reflect,UINT32_MAX,&matches)&&!matches);
        HSD_GObj native={.user_data=&expected};ftCommon_8007F76C(&native);data->x21F8(object);
        CHECK(memcmp(data,&expected,sizeof(expected))==0);++comparisons;
        CHECK(field(object,"observed",NULL).as.i64==0);
        CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CallbackAlignVelocityToFacing,NULL,0,NULL,0,&output,&diagnostic));
        CHECK(memcmp(data,&expected,sizeof(expected))==0);
    }
    data->x21F8=original_callback;CHECK(nativeScriptFighter_Run(object,NativeScriptFighter_Tick,NULL,0,NULL,&output)==NativeScriptVM_Ok);++vm_calls;
    CHECK(data->x21F8==ftCommon_8007F76C&&!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Reflect));
    CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Reflect,helper));
    CHECK(data->x21F8==NULL&&nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Reflect));
    CHECK(nativeScriptFighter_RunCallback(object,NativeScriptFighterCallback_Reflect));++vm_calls;
    CHECK(field(object,"observed",NULL).as.i64==1);
    CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CallbackAlignVelocityToFacing,NULL,0,NULL,0,&output,&diagnostic));
    CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CallbackTrackReflect,NULL,0,NULL,0,&output,&diagnostic));
    CHECK(data->x21F8==NULL&&nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Reflect));
    CHECK(nativeScriptFighter_RunCallback(object,NativeScriptFighterCallback_Reflect));++vm_calls;
    CHECK(field(object,"observed",NULL).as.i64==3);
    CHECK(nativeScriptBindings_Invoke(&c,NativeScriptBinding_CallbackAlignVelocityToFacing,NULL,0,NULL,0,&output,&diagnostic));
    nativeScriptFighter_ResetMotionCallbacks(object);
    CHECK(data->x21F8==ftCommon_8007F76C&&!nativeScriptFighter_CallbackBound(object,NativeScriptFighterCallback_Reflect));
    CHECK(nativeScriptFighter_BindCallback(object,NativeScriptFighterCallback_Reflect,UINT32_MAX));CHECK(data->x21F8==NULL);
    HSD_GObj* article_object=item(),*stock=fighter(false,30);
    NativeScriptReceiverView caller_view;CHECK(nativeScriptFighter_View(object,c.generation,&caller_view));
    for(unsigned mode=0;mode<7;++mode){NativeScriptRuntimeContext invalid=c;uint32_t count=0;NativeScriptValue arg={.type=NativeScriptType_I32};
        bool valid=bank.valid,live=caller_view.storage->live;
        if(mode==0)invalid.scene_epoch++;
        if(mode==1)invalid.generation++;
        if(mode==2)invalid=context(article_object);
        if(mode==3)invalid=context(stock);
        if(mode==4)count=1;
        if(mode==5)bank.valid=false;
        if(mode==6)caller_view.storage->live=false;
        data->x21F8=original_callback;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingSetup_Invoke(&invalid,NativeScriptBinding_CallbackAlignVelocityToFacing,&arg,count,&output,&diagnostic));
        CHECK(data->x21F8==original_callback&&output.type==NativeScriptType_U64&&output.as.u64==99);++boundary_cases;
        bank.valid=valid;caller_view.storage->live=live;
    }
    for(unsigned mode=1;mode<=9;++mode){query_caller=fighter(true,40+fighter_count);c=context(query_caller);data=query_caller->user_data;
        data->x21F8=original_callback;uint64_t layout=bank.storage_layout.identity,parameters=bank.storage_parameters.identity,program=target_package.program.identity;
        guard_calls=0;mutate_at=2;mutation=mode;output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
        CHECK(!nativeScriptBindingSetup_Invoke(&c,NativeScriptBinding_CallbackAlignVelocityToFacing,NULL,0,&output,&diagnostic));
        CHECK(mutation==0&&output.type==NativeScriptType_U64&&output.as.u64==99);++mutation_cases;
        if(mode!=4)CHECK(((Fighter*)query_caller->user_data)->x21F8==original_callback);
        bank.storage_layout.identity=layout;bank.storage_parameters.identity=parameters;target_package.program.identity=program;
        if(mode==4){CHECK(replacement==query_caller);HSD_GObjFree(replacement);}else HSD_GObjFree(query_caller);query_caller=NULL;
    }
    HSD_GObjFree(stock);HSD_GObjFree(article_object);HSD_GObjFree(object);CHECK(nativeScriptRuntime_Idle());accessory_contract();
    printf("{\"status\":\"PASS callback velocity and accessory slots\",\"checks\":%u,\"native_comparisons\":%u,\"vm_calls\":%u,\"compile_cases\":%u,\"mutation_cases\":%u,\"boundary_cases\":%u,\"accessory_scheduler_pairs\":%u,\"accessory_cases\":%u}\n",checks,comparisons,vm_calls,compile_cases,mutation_cases,boundary_cases,scheduler_pairs,scheduler_cases);return 0;
}
