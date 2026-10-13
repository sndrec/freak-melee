#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include "fixture.h"
#include <melee/ft/native_script_binding_targets.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
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
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
unsigned guard_calls,mutate_at,mutation;HSD_GObj* query_caller,*query_target,*replacement;HSD_GObj*(*target_reuse)(void);
static unsigned checks,fighter_count,item_count,comparisons,vm_calls,compile_cases,mutation_cases,boundary_cases;static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"target query %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
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
    const char* source="fighter:\n    behavior independent\nstate int handle\nstate int observed\nfunc query(int reference) -> int:\n    return targets.motion(reference)\naction Queries:\n    on tick:\n        state.observed = query(state.handle)\narticle Probe states 1:\n    state int handle\n    state int observed\n    on signal Query:\n        item.state.observed = targets.motion(item.state.handle)\n    state 0 anim -1:\n";
    CHECK(compile_source(source));
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};
    NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
    CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,&bank,&layout,&metadata,&target_package.program,&diagnostic));
    target_package.address=&target_package;target_package.target=Ft_Kind_CustomFirst;target_package.ready=target_package.assets.ready=true;target_package.assets.facts.moves=layout;
    target_package.assets.parts.parts_num=4;target_package.assets.facts.joints=4;tables[Ft_Kind_CustomFirst]=&target_package.assets.parts;
    gFtDataList[Ft_Kind_CustomFirst]=&target_package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=target_package.assets.motions;
    nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&target_package.program);
}
static void compiler_contract(void)
{
    const char* accepted[]={"targets.motion(0)","targets.motion(-1)","targets.motion(4294967295)","targets.motion(16777217)","targets.motion(true)"};
    char source[1024];
    for(unsigned n=0;n<sizeof(accepted)/sizeof(*accepted);++n){snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc query() -> int:\n    return %s\n",accepted[n]);CHECK(compile_source(source));}
    const char* rejected[]={"targets.motion(1.5)","targets.motion()","targets.motion(1, 2)","targets.motion(\"foreign\")"};
    for(unsigned n=0;n<sizeof(rejected)/sizeof(*rejected);++n){snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc query() -> int:\n    return %s\n",rejected[n]);CHECK(!compile_source(source));}
    const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(NativeScriptBinding_TargetsMotion);
    CHECK(descriptor&&descriptor->result_type==NativeScriptType_I32&&descriptor->argument_count==1);
}
static void fighter_remove(void* p){nativeScriptFighter_Detach(((Fighter*)p)->gobj);}
static void item_remove(void* p){nativeScriptItem_Detach(((Item*)p)->entity);}
static HSD_GObj* fighter(bool owned,uint32_t id)
{
    CHECK(fighter_count<128);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);Fighter* f=&fighters[fighter_count++];
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
static uint32_t hash(const char* name){uint32_t h=2166136261u;while(*name){h^=(unsigned char)*name++;h*=16777619u;}return h;}
static HSD_GObj* reuse(void){replacement=fighter(false,UINT32_C(0x80000005));return replacement;}
void target_guard_probe(NativeScriptRuntimeContext* c)
{
    if(c->source!=query_caller)return;++guard_calls;if(!mutation||guard_calls!=mutate_at)return;
    unsigned mode=mutation;mutation=0;
    if(mode==1)((Fighter*)query_target->user_data)->x8_spawnNum++;
    if(mode==2)((Fighter*)query_target->user_data)->kind=Ft_Kind_Zelda;
    if(mode==3){alternate=*(Fighter*)query_target->user_data;query_target->user_data=&alternate;}
    if(mode==4){HSD_GObjFree(query_target);if(target_reuse)target_reuse();}
    if(mode==5)HSD_GObjFree(query_caller);
    if(mode==6){NativeScriptValue result;CHECK(nativeScriptItem_RunHook(query_caller,NativeScriptItemHook_Destroyed,NULL,0,&result)==NativeScriptVM_Ok);}
    if(mode==7){for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==query_caller)nativeScriptItemActors[i].faulted=true;}
    if(mode==8)query_target->classifier=HSD_GOBJ_CLASS_ITEM;
}
int main(void)
{
    compiler_contract();prepare();HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
    HSD_GObj* f=fighter(true,21),*a=item(),*foreign=fighter(false,UINT32_C(0x80000005));
    NativeScriptValue input={.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000005)},output;
    const int states[]={INT32_MIN,-1,0,1,2,340,341,642,16777217,INT32_MAX};
    for(unsigned caller_kind=0;caller_kind<2;++caller_kind)for(unsigned n=0;n<sizeof(states)/sizeof(*states);++n){
        HSD_GObj* caller=caller_kind?a:f;((Fighter*)foreign->user_data)->motion_id=states[n];
        NativeScriptRuntimeContext c=context(caller);
        CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_I32&&output.as.i64==states[n]);
        CHECK(output.as.i64==nativeFighterTarget_Motion(UINT32_C(0x80000005)));++comparisons;
        NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=-2147483643};field(caller,"handle",&id);
        if(caller_kind)CHECK(nativeScriptItem_RunSignal(caller,hash("Query"),0,&output)==NativeScriptVM_Ok);
        else CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0, NULL,&output)==NativeScriptVM_Ok);
        CHECK(field(caller,"observed",NULL).as.i64==states[n]);++vm_calls;
    }
    for(unsigned caller_kind=0;caller_kind<2;++caller_kind){NativeScriptRuntimeContext c=context(caller_kind?a:f);
        const uint32_t missing[]={0,123456,UINT32_MAX};
        for(unsigned n=0;n<3;++n){input=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=missing[n]};
            CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.type==NativeScriptType_I32&&output.as.i64==-1);
            CHECK(output.as.i64==nativeFighterTarget_Motion(missing[n]));++comparisons;}
        HSD_GObj* maximum=fighter(false,UINT32_MAX);((Fighter*)maximum->user_data)->motion_id=2;
        input=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=-1};CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.as.i64==2);
        input=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT32_MAX};CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.as.i64==2);HSD_GObjFree(maximum);
        NativeScriptValue malformed[]={{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000005)},{.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_F32,.as.f32=2}};
        for(unsigned n=0;n<3;++n){output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
            CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&malformed[n],1,&output,&diagnostic));CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);}
    }
    const uint32_t ids[]={UINT32_C(16777217),UINT32_C(0xfffffffe),UINT32_MAX};
    for(unsigned n=0;n<sizeof(ids)/sizeof(*ids);++n){
        HSD_GObj* target=fighter(false,ids[n]);((Fighter*)target->user_data)->motion_id=642+n;
        int32_t signed_id;memcpy(&signed_id,&ids[n],sizeof(signed_id));
        for(unsigned caller_kind=0;caller_kind<2;++caller_kind){HSD_GObj* caller=caller_kind?a:f;
            NativeScriptValue id={.type=NativeScriptType_I32,.as.i64=signed_id};field(caller,"handle",&id);
            if(caller_kind)CHECK(nativeScriptItem_RunSignal(caller,hash("Query"),0,&output)==NativeScriptVM_Ok);
            else CHECK(nativeScriptFighter_Run(caller,NativeScriptFighter_Tick,NULL,0,NULL,&output)==NativeScriptVM_Ok);
            CHECK(field(caller,"observed",NULL).as.i64==nativeFighterTarget_Motion(ids[n]));++comparisons;++vm_calls;
        }
        HSD_GObjFree(target);
    }
    for(unsigned caller_kind=0;caller_kind<2;++caller_kind){
        HSD_GObj* caller=caller_kind?a:f;input=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=21};
        ((Fighter*)f->user_data)->motion_id=16777217;NativeScriptRuntimeContext valid=context(caller);
        NativeScriptReceiverView view;CHECK(nativeScriptActors_Lookup(NULL,caller,valid.source_class,valid.generation,&view));
        CHECK(nativeScriptBindingTargets_Invoke(&valid,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic));
        CHECK(output.type==NativeScriptType_I32&&output.as.i64==nativeFighterTarget_Motion(21));++comparisons;
        for(unsigned mode=0;mode<5;++mode){NativeScriptRuntimeContext invalid=valid;
            const NativeScriptStorageLayout* layout=view.storage->layout;bool bank_valid=bank.valid;
            if(mode==0)invalid.scene_epoch++;
            if(mode==1)invalid.generation++;
            if(mode==2)invalid.source_class=NativeCallbackSource_ModelEffect;
            if(mode==3)view.storage->layout=NULL;
            if(mode==4)bank.valid=false;
            output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
            CHECK(!nativeScriptBindingTargets_Invoke(&invalid,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic));
            CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++boundary_cases;
            view.storage->layout=layout;bank.valid=bank_valid;
        }
        // Bool retains the shared target API's canonical 0/1 interpretation.
        HSD_GObj* one=fighter(false,1);((Fighter*)one->user_data)->motion_id=642;
        input=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
        CHECK(nativeScriptBindingTargets_Invoke(&valid,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.as.i64==642);
        CHECK(output.as.i64==nativeFighterTarget_Motion(1));++comparisons;
        input.as.u64=0;CHECK(nativeScriptBindingTargets_Invoke(&valid,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.as.i64==-1);
        HSD_GObjFree(one);
    }
    NativeScriptRuntimeContext unowned=context(foreign);input=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=21};
    output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
    CHECK(!nativeScriptBindingTargets_Invoke(&unowned,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic));
    CHECK(output.type==NativeScriptType_U64&&output.as.u64==99);++boundary_cases;
    HSD_GObj* wrong=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_FIGHTER,0);static Fighter wrongdata;wrongdata=(Fighter){.gobj=wrong,.x8_spawnNum=345,.motion_id=642};wrong->user_data=&wrongdata;
    NativeScriptRuntimeContext c=context(f);input=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=345};CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.as.i64==-1);HSD_GObjFree(wrong);
    HSD_GObjFree(foreign);
    input=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000005)};
    CHECK(nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic)&&output.as.i64==-1);
    CHECK(output.as.i64==nativeFighterTarget_Motion(UINT32_C(0x80000005)));++comparisons;
    for(unsigned caller_kind=0;caller_kind<2;++caller_kind)for(unsigned mode=1;mode<=8;++mode){if(!caller_kind&&(mode==6||mode==7))continue;
        query_caller=caller_kind?item():fighter(true,30+fighter_count);query_target=fighter(false,UINT32_C(0x80000005));c=context(query_caller);input=(NativeScriptValue){.type=NativeScriptType_U32,.as.u64=UINT32_C(0x80000005)};
        output=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};guard_calls=0;mutate_at=caller_kind?3:4;mutation=mode;target_reuse=mode==4?reuse:NULL;
        CHECK(!nativeScriptBindingTargets_Invoke(&c,NativeScriptBinding_TargetsMotion,&input,1,&output,&diagnostic));CHECK(mutation==0&&output.type==NativeScriptType_U64&&output.as.u64==99);++mutation_cases;
        if(mode==4){CHECK(replacement==query_target);HSD_GObjFree(replacement);}else HSD_GObjFree(query_target);if(mode!=5)HSD_GObjFree(query_caller);query_caller=NULL;target_reuse=NULL;
    }
    HSD_GObjFree(a);HSD_GObjFree(f);CHECK(nativeScriptRuntime_Idle());printf("{\"status\":\"PASS motion\",\"checks\":%u,\"native_comparisons\":%u,\"vm_calls\":%u,\"compile_cases\":%u,\"mutation_cases\":%u,\"boundary_cases\":%u}\n",checks,comparisons,vm_calls,compile_cases,mutation_cases,boundary_cases);return 0;
}
