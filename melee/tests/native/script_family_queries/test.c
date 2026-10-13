#include "boundary.h"
#include <melee/ft/native_script_binding_family.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/pl/player.h>
#include <melee/ft/native_fighter_cpu.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#pragma clang section data = ""

enum { N=2048, F=64, V=64 };
static char bytes[32768]; static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N*2]; static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F]; static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V]; static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[4096]; static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F]; static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V]; static NativeScriptStorageOwner owners[8];
static NativeScriptParameterSlot parameter_slots[V]; static NativeSourcePackage package,secondary_package;
#define primary_bank package.compiler.bank
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
static MotionState motions[342]; static Fighter fighters[96];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity]; MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static StaticPlayer player_slots[Gm_Player_NumMax];
#define player_member player_slots[2].player_entity[0]
 static HSD_GObj* observed_source; static HSD_GObj* replacement;
static unsigned allocated,checks,cases,pairs,vm_calls,negatives,guard_calls,mutate_at,mutation;
static NativeScriptDiagnostic diagnostic;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"family line%u %s: %s\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)

static void Player_CheckSlot(int slot){CHECK(slot>=0&&slot<Gm_Player_NumMax);}
#include "native_player.inc"
#include "native_family.inc"
static Fighter* ftCo_800A589C(Fighter* f){(void)f;abort();}
#include "native_initializer.inc"

static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* create(uint32_t generation)
{
    CHECK(allocated<95);HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
    Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));f->gobj=object;f->kind=f->behavior_kind=Ft_Kind_CustomFirst;
    f->ft_data=&package.assets.character.data;f->x8_spawnNum=generation;f->player_id=2;
    GObj_InitUserData(object,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(object));return object;
}
static unsigned function_id(const char* name);
void family_guard_observe(void)
{
    if(!observed_source||!mutate_at||++guard_calls!=mutate_at)return;
    unsigned action=mutation;mutation=0;mutate_at=0;Fighter* f=observed_source->user_data;
    switch(action){
    case 1:f->x8_spawnNum++;break;
    case 2:observed_source->user_data=&fighters[95];break;
    case 3:primary_bank.storage_layout.identity++;break;
    case 4:nativeScriptFighter_Detach(observed_source);break;
    case 5:{NativeScriptValue out;NativeScriptDiagnostic d;++vm_calls;CHECK(nativeScriptFighter_Helper(observed_source,f->x8_spawnNum,function_id("Fault"),NULL,0,&out,&d)==NativeScriptVM_Error);break;}
    case 6:HSD_GObjFree(observed_source);replacement=create(777);CHECK(replacement==observed_source);break;
    case 7:package.ready=false;break;
    default:abort();
    }
}
static NativeScriptRuntimeContext context(HSD_GObj* o)
{return(NativeScriptRuntimeContext){.source=o,.source_class=NativeCallbackSource_Fighter,.generation=((Fighter*)o->user_data)->x8_spawnNum,.scene_epoch=nativeGObjPool.lifetime};}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0x123456789abcdef)};}
static bool invoke(NativeScriptRuntimeContext* c,uint32_t id,NativeScriptValue* arg,NativeScriptValue* out)
{return nativeScriptBindingFamily_Invoke(c,id,arg,1,out,&diagnostic);}
static unsigned function_id(const char* name)
{for(unsigned i=0;i<primary_bank.declarations.function_count;++i)if(nativeScriptSource_SpanEquals(&primary_bank.tree,primary_bank.declarations.functions[i].name,name))return i;CHECK(false);return 0;}
static bool helper(HSD_GObj* o,unsigned fn,NativeScriptValue* out)
{++vm_calls;return nativeScriptFighter_Helper(o,((Fighter*)o->user_data)->x8_spawnNum,fn,NULL,0,out,&diagnostic)==NativeScriptVM_Ok;}

typedef struct SecondaryCompileStorage {
 char bytes[32768];NativeScriptSourceNode syntax[N];NativeScriptIRNode expressions[N*2];uint32_t arguments[N*2];
 NativeScriptFunctionDeclaration declarations[F];NativeScriptParameterDeclaration parameters[V];NativeScriptValueDeclaration values[V];NativeScriptLocal locals[64];
 NativeScriptInstruction instructions[4096];NativeScriptFunctionCode code[F];NativeScriptExecutionFunction functions[F];NativeScriptCompileFunctionScratch graph[F];
 NativeScriptStorageField fields[V];NativeScriptStorageOwner owners[8];NativeScriptParameterSlot parameters_slots[V];
} SecondaryCompileStorage;
static SecondaryCompileStorage secondary_compile;
static void prepare_secondary(const char* source)
{
 SecondaryCompileStorage* a=&secondary_compile;NativeSourcePackage* p=&secondary_package;
 p->compiler.bank=(NativeScriptCompileBank){.source_storage=a->bytes,.source_capacity=sizeof(a->bytes),.tree={.nodes=a->syntax,.capacity=N},.expressions={.nodes=a->expressions,.node_capacity=N*2,.arguments=a->arguments,.argument_capacity=N*2},
 .declarations={.functions=a->declarations,.function_capacity=F,.parameters=a->parameters,.parameter_capacity=V,.values=a->values,.value_capacity=V},.locals=a->locals,.local_capacity=64,
 .instructions=a->instructions,.instruction_capacity=4096,.function_code=a->code,.function_code_capacity=F,.execution_functions=a->functions,.execution_function_capacity=F,.graph=a->graph,.graph_capacity=F,
 .storage_layout={.fields=a->fields,.field_capacity=V,.owners=a->owners,.owner_capacity=8},.parameter_slots=a->parameters_slots,.parameter_slot_capacity=V};
 NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
 CHECK(nativeScriptCompile_Build(&p->compiler.bank,&request,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst+1,accepted,&layout,NULL,&p->program,&diagnostic));
 p->address=p;p->ready=p->assets.ready=true;p->target=Ft_Kind_CustomFirst+1;p->assets.motions=motions;p->assets.facts.moves=layout;
 gFtDataList[p->target]=&p->assets.character.data;ftData_CharacterStateTables[p->target]=motions;
 CHECK(nativeScriptFighter_CanPublish(p->target));nativeScriptFighter_PublishProgram(p->target,&p->program);CHECK(nativeScriptFighter_Program(p->target)!=&p->program);
 slots[1]=(NativeFighterSlot){.family=&families[0],.member_index=1,.source=p,.character_kind=ChKind_CustomFirst,.fighter_kind=p->target};
}
static void prepare(void)
{
    primary_bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},.expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},
        .declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},.locals=locals,.local_capacity=64,
        .instructions=instructions,.instruction_capacity=4096,.function_code=code,.function_code_capacity=F,.execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=parameter_slots,.parameter_slot_capacity=V};
    const char* bad[]={"family.member_id()","family.member_id(0, 1)","family.member_id(0.5)","family.member_id(\"x\")","family.retire_companion_on_sleep()","family.retire_companion_on_sleep(true, false)","family.retire_companion_on_sleep(1)","family.retire_companion_on_sleep(0.5)","family.missing(0)"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i){char source[512];snprintf(source,sizeof(source),"fighter:\n    behavior independent\nfunc Bad():\n    %s\n",bad[i]);NativeScriptCompileRequest r={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
        CHECK(!nativeScriptCompile_Build(&primary_bank,&r,&accepted,&diagnostic)&&!accepted);++negatives;++cases;}
    static ItemAttr attr;static ItemModelDesc model;static Article article={.x0_common_attr=&attr,.x10_modelDesc=&model};
    NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Link_Arrow,.article=&article};NativeScriptItemMetadataContext metadata={.motions=&layout,.assets=&asset,.asset_count=1};
    const char* article_bad[]={"let int id = family.member_id(0)","family.retire_companion_on_sleep(true)"};
    for(unsigned i=0;i<2;++i){char source[512];snprintf(source,sizeof(source),"fighter:\n    behavior independent\narticle Probe states 1:\n    on spawned:\n        %s\n    state 0 anim -1:\n",article_bad[i]);
        NativeScriptCompileRequest r={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};const NativeScriptCompileBank* accepted=NULL;
        CHECK(!nativeScriptCompile_Build(&primary_bank,&r,&accepted,&diagnostic)&&!accepted);CHECK(strstr(diagnostic.message,"context")!=NULL);++negatives;++cases;}
    const char* source="fighter:\n    behavior independent\nstate int evaluations\nstate int history_hint\nfunc Primary() -> int:\n    return family.member_id(0)\nfunc Companion() -> int:\n    return family.member_id(1)\nfunc Next() -> int:\n    state.evaluations = state.evaluations + 1\n    return 0\nfunc Once() -> int:\n    return family.member_id(Next())\nfunc Enable():\n    family.retire_companion_on_sleep(true)\nfunc Disable():\n    family.retire_companion_on_sleep(false)\nfunc Fault():\n    let int zero = 0\n    let int invalid = 1 / zero\naction Hooks:\n    on cpu_prepare:\n        state.history_hint = event.history_source\n        return event.history_source\n";
    NativeScriptCompileRequest r={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&primary_bank,&r,&accepted,&diagnostic));CHECK(nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&package.program,&diagnostic));
    package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;package.assets.motions=motions;package.assets.facts.moves=layout;
    gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
    CHECK(nativeScriptFighter_Program(Ft_Kind_CustomFirst)!=&package.program);
    families[0]=(NativeFighterFamily){.members=slots,.character_kind=ChKind_CustomFirst,.active=true,.metadata={.mode=NativeFighterFamily_Single,.member_count=1}};
    slots[0]=(NativeFighterSlot){.family=&families[0],.member_index=0,.source=&package,.character_kind=ChKind_CustomFirst,.fighter_kind=Ft_Kind_CustomFirst,.active=true};
    player_slots[2].player_character=ChKind_CustomFirst;player_slots[2].transformed[0]=0;player_slots[2].transformed[1]=1;
    prepare_secondary(source);
}

static void comparisons(void)
{
    const uint32_t ids[]={1,16777217,UINT32_C(0x80000001),UINT32_MAX};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i){HSD_GObj* o=create(ids[i]);player_member=o;NativeScriptRuntimeContext c=context(o);NativeScriptValue out;
        const NativeScriptValue indices[]={{.type=NativeScriptType_I32,.as.i64=0},{.type=NativeScriptType_U32,.as.u64=1},{.type=NativeScriptType_I64,.as.i64=2},{.type=NativeScriptType_I32,.as.i64=-1},{.type=NativeScriptType_I64,.as.i64=INT64_MIN},{.type=NativeScriptType_U64,.as.u64=UINT64_C(0x100000000)},{.type=NativeScriptType_U64,.as.u64=UINT64_MAX},{.type=NativeScriptType_Bool,.as.u64=0},{.type=NativeScriptType_Bool,.as.u64=1}};
        for(unsigned j=0;j<sizeof(indices)/sizeof(*indices);++j){NativeScriptValue arg=indices[j];CHECK(invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,&out));uint32_t actual=(uint32_t)out.as.i64;
            CHECK(out.type==NativeScriptType_I32&&actual==(j==0||j==7?nativeFighterRuntime_MemberId(o,0):0));++pairs;++cases;}
        CHECK(helper(o,function_id("Primary"),&out)&&(uint32_t)out.as.i64==ids[i]);CHECK(helper(o,function_id("Companion"),&out)&&out.as.i64==0);CHECK(helper(o,function_id("Once"),&out)&&(uint32_t)out.as.i64==ids[i]);
        NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,ids[i],&view));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC,NULL,0,&out,&diagnostic)&&out.as.i64==1);
        ((Fighter*)o->user_data)->x2222_b4=((Fighter*)o->user_data)->x2222_b6=((Fighter*)o->user_data)->x2222_b7=1;
        for(unsigned enabled=0;enabled<2;++enabled){Fighter expected=*(Fighter*)o->user_data;expected.x2222_b5=enabled;NativeScriptValue arg={.type=NativeScriptType_Bool,.as.u64=enabled};
            CHECK(invoke(&c,NativeScriptBinding_FamilyRetireCompanionOnSleep,&arg,&out)&&out.type==NativeScriptType_Void);CHECK(!memcmp(&expected,o->user_data,sizeof(expected)));++pairs;++cases;}
        CHECK(helper(o,function_id("Enable"),&out)&&((Fighter*)o->user_data)->x2222_b5);CHECK(helper(o,function_id("Disable"),&out)&&!((Fighter*)o->user_data)->x2222_b5);
        player_member=NULL;NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=0};CHECK(invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,&out)&&out.as.i64==nativeFighterRuntime_MemberId(o,0)&&!out.as.i64);++pairs;++cases;
        ((Fighter*)o->user_data)->player_id=Gm_Player_NumMax;CHECK(invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,&out)&&!out.as.i64);++cases;HSD_GObjFree(o);
    }
}

static void failures(void)
{
    HSD_GObj* o=create(55);player_member=o;NativeScriptRuntimeContext c=context(o);NativeScriptValue out=sentinel(),saved=out;
    const NativeScriptValue invalid[]={{.type=NativeScriptType_F32,.as.f32=0},{.type=NativeScriptType_I32,.as.i64=INT64_MAX},{.type=NativeScriptType_U32,.as.u64=UINT64_MAX},{.type=NativeScriptType_Bool,.as.u64=2}};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i){NativeScriptValue arg=invalid[i];CHECK(!invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,&out)&&!memcmp(&saved,&out,sizeof(out)));++negatives;++cases;}
    for(unsigned i=0;i<3;++i){NativeScriptValue arg=i==0?(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1}:(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=i+1};Fighter before=*(Fighter*)o->user_data;
        CHECK(!invoke(&c,NativeScriptBinding_FamilyRetireCompanionOnSleep,&arg,&out)&&!memcmp(&saved,&out,sizeof(out))&&!memcmp(&before,o->user_data,sizeof(before)));++negatives;++cases;}
    NativeScriptValue arg={.type=NativeScriptType_I32,.as.i64=0};
    for(unsigned i=0;i<5;++i){NativeScriptRuntimeContext bad=c;if(i==0)bad.source_class=NativeCallbackSource_Item;if(i==1)bad.generation++;if(i==2)bad.scene_epoch++;if(i==3)bad.source=NULL;if(i==4)bad.scene_epoch=0;
        CHECK(!invoke(&bad,NativeScriptBinding_FamilyMemberId,&arg,&out)&&!memcmp(&saved,&out,sizeof(out)));++negatives;++cases;}
    CHECK(!nativeScriptBindingFamily_Invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,0,&out,&diagnostic));CHECK(!nativeScriptBindingFamily_Invoke(&c,NativeScriptBinding_FamilyMemberId,NULL,1,&out,&diagnostic));
    HSD_GObjFree(o);
    for(unsigned operation=0;operation<2;++operation)for(unsigned mode=1;mode<=7;++mode){o=create(100+mode);player_member=o;c=context(o);out=sentinel();saved=out;Fighter* f=o->user_data;uint64_t schema=primary_bank.storage_layout.identity;
        arg=operation?(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1}:(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=0};
        // Native operations have no callback. Setter mutations occur during
        // admission, before its field write; query mutations exercise withheld
        // publication at the explicitly controlled pure guard observation.
        observed_source=o;guard_calls=0;mutate_at=operation?2:4;mutation=mode;CHECK(!invoke(&c,operation?NativeScriptBinding_FamilyRetireCompanionOnSleep:NativeScriptBinding_FamilyMemberId,&arg,&out));CHECK(!mutation&&!memcmp(&saved,&out,sizeof(out)));observed_source=NULL;
        if(operation)CHECK(!f->x2222_b5);
        primary_bank.storage_layout.identity=schema;package.ready=true;if(mode==2)o->user_data=f;if(mode==6)HSD_GObjFree(replacement);else HSD_GObjFree(o);++negatives;++cases;}
    o=create(88);player_member=o;c=context(o);out=sentinel();saved=out;++vm_calls;CHECK(nativeScriptFighter_Helper(o,88,function_id("Fault"),NULL,0,&out,&diagnostic)==NativeScriptVM_Error);
    arg=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!invoke(&c,NativeScriptBinding_FamilyMemberId,&arg,&out)&&!memcmp(&saved,&out,sizeof(out)));++negatives;++cases;HSD_GObjFree(o);
    CHECK(nativeFighterRuntime_MemberId(NULL,0)==0);CHECK(nativeFighterRuntime_MemberId(NULL,1)==0);
}

#include "paired_history_cases.h"
int main(void)
{
    HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);prepare();comparisons();paired_history_cases();failures();CHECK(nativeScriptRuntime_Idle());
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"vm_calls\":%u,\"negatives\":%u}\n",checks,cases,pairs,vm_calls,negatives);return 0;
}
