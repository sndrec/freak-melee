#include "boundary.h"
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_trail.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itsword.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma clang section data = ""

enum { N=1024,F=32,V=32 };
static char bytes[16384]; static NativeScriptSourceNode syntax[N];
static NativeScriptIRNode expressions[N*2]; static uint32_t arguments[N*2];
static NativeScriptFunctionDeclaration declarations[F]; static NativeScriptParameterDeclaration parameters[V];
static NativeScriptValueDeclaration values[V]; static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[2048]; static NativeScriptFunctionCode code[F];
static NativeScriptExecutionFunction functions[F]; static NativeScriptCompileFunctionScratch graph[F];
static NativeScriptStorageField fields[V]; static NativeScriptStorageOwner owners[8];
static NativeScriptParameterSlot slots[V]; static NativeScriptParameterSupply supplies[V];
static NativeSourcePackage package;
static NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};
static MotionState motions[342]; static Fighter fighters[96]; static FighterBone bones[50];
static HSD_JObj joints[50]; static struct HSD_JObjInfo bad_class;
struct HSD_JObjInfo hsdJObj;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity]; FighterPartsTable** ftPartsTable=tables;
static FighterPartsTable stock_parts={.parts_num=50}; static ftLk_DatAttrs attrs;
static unsigned checks,cases,pairs,vm_calls,allocated,injection,reads;
unsigned trail_legacy_queries;
#define CHECK(x) do { ++checks; if(!(x)){fprintf(stderr,"trail line%u %s\n",__LINE__,#x);exit(1);} } while(0)

const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{ return kind==Ft_Kind_CustomFirst ? &package : NULL; }
itSword_UnkBytes* it_80285300(Item_GObj* o){(void)o;abort();}
void it_802852B8(Item_GObj* o,float* a,float* b){(void)o;(void)a;(void)b;abort();}
HSD_JObj* it_80285314(Item_GObj* o){(void)o;abort();}
#include "native_trail.inc"
#include "trail_sources.h"
#define bank package.compiler.bank

// A controlled pure observation supplies no trail values. It permits exact
// failed/retired invocation tests after a preceding scratch record write.
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{
    (void)selectors;(void)d;
    if(semantic!=NativeScriptBinding_PositionX || count || !nativeScriptBindings_SourceAlive(c))return false;
    ++reads;HSD_GObj* o=c->source;Fighter* f=o->user_data;unsigned mode=injection;injection=0;
    if(mode==1)return false;
    if(mode==2)HSD_GObjFree(o);
    if(mode==3){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));nativeScriptStorage_Destroy(view.storage);}
    if(mode==4)++f->x8_spawnNum;
    if(mode==5)o->user_data=&fighters[95];
    if(mode==6)o->classifier=HSD_GOBJ_CLASS_ITEM;
    *out=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=3.0f};return true;
}

static bool supply(void* user,const NativeScriptProgramIndex* p,
    const NativeScriptParameterSupply** out,uint32_t* count,NativeScriptDiagnostic* d)
{
    (void)user;(void)d;
    const struct SwordAttrs* a=&attrs.x64;
    const char* names[]={"sword_inner_fade","sword_outer_fade","sword_start_alpha","sword_end_alpha",
        "sword_inner_red","sword_inner_green","sword_inner_blue","sword_outer_red","sword_outer_green","sword_outer_blue",
        "sword_bone","sword_inner_distance","sword_outer_distance"};
    const NativeScriptValue data[]={
        {.type=NativeScriptType_F32,.as.f32=a->x0},{.type=NativeScriptType_F32,.as.f32=a->x4},
        {.type=NativeScriptType_I32,.as.i64=a->x8},{.type=NativeScriptType_I32,.as.i64=a->x9},
        {.type=NativeScriptType_I32,.as.i64=a->xA},{.type=NativeScriptType_I32,.as.i64=a->xB},
        {.type=NativeScriptType_I32,.as.i64=a->xC},{.type=NativeScriptType_I32,.as.i64=a->xE},
        {.type=NativeScriptType_I32,.as.i64=a->xF},{.type=NativeScriptType_I32,.as.i64=a->x10},
        {.type=NativeScriptType_I32,.as.i64=a->x14},{.type=NativeScriptType_F32,.as.f32=a->x18},
        {.type=NativeScriptType_F32,.as.f32=a->x1C}};
    uint32_t n=0;
    for(unsigned i=0;i<p->value_count;++i){if(!p->values[i].parameter)continue;bool found=false;
        for(unsigned j=0;j<13;++j)if(nativeScriptSource_SpanEquals(p->tree,p->values[i].name,names[j])){
            supplies[n++]=(NativeScriptParameterSupply){.semantic=NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,.owner=0,.value=data[j]};found=true;break;}
        if(!found)return false;
    }
    CHECK(n==13);*out=supplies;*count=n;return true;
}
static void prepare(const char* body)
{
    bank=(NativeScriptCompileBank){.source_storage=bytes,.source_capacity=sizeof(bytes),.tree={.nodes=syntax,.capacity=N},
        .expressions={.nodes=expressions,.node_capacity=N*2,.arguments=arguments,.argument_capacity=N*2},
        .declarations={.functions=declarations,.function_capacity=F,.parameters=parameters,.parameter_capacity=V,.values=values,.value_capacity=V},
        .locals=locals,.local_capacity=64,.instructions=instructions,.instruction_capacity=2048,.function_code=code,.function_code_capacity=F,
        .execution_functions=functions,.execution_function_capacity=F,.graph=graph,.graph_capacity=F,
        .storage_layout={.fields=fields,.field_capacity=V,.owners=owners,.owner_capacity=8},.parameter_slots=slots,.parameter_slot_capacity=V};
    char source[8192];const char* prefix="fighter:\n    behavior independent\n";
    const char* params="parameter float sword_inner_fade = 0\nparameter float sword_outer_fade = 0\nparameter int sword_start_alpha = 0\nparameter int sword_end_alpha = 0\nparameter int sword_inner_red = 0\nparameter int sword_inner_green = 0\nparameter int sword_inner_blue = 0\nparameter int sword_outer_red = 0\nparameter int sword_outer_green = 0\nparameter int sword_outer_blue = 0\nparameter int sword_bone = 0\nparameter float sword_inner_distance = 0\nparameter float sword_outer_distance = 0\n";
    CHECK(snprintf(source,sizeof(source),"%s%s%s",prefix,params,body)>0);
    NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.motion_layout=&layout,.parameter_provider=supply};
    NativeScriptDiagnostic d={0};const NativeScriptCompileBank* accepted=NULL;
    if(!nativeScriptCompile_Build(&bank,&request,&accepted,&d)||!nativeScriptFighter_PrepareProgram(Ft_Kind_CustomFirst,accepted,&layout,NULL,&package.program,&d)){
        fprintf(stderr,"prepare %u:%u %s\n",d.source.line,d.source.column,d.message);exit(1);}
    package.address=&package;package.ready=package.assets.ready=true;package.target=Ft_Kind_CustomFirst;
    package.assets.motions=motions;package.assets.facts.moves=layout;package.assets.facts.joints=49;package.assets.facts.omission_count=1;
    package.assets.parts.parts_num=50;package.assets.parts.part_to_joint=package.assets.part_to_joint;
    package.assets.parts.joint_to_part=package.assets.joint_to_part;tables[Ft_Kind_CustomFirst]=&package.assets.parts;
    tables[Ft_Kind_Link]=tables[Ft_Kind_CLink]=&stock_parts;
    gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;ftData_CharacterStateTables[Ft_Kind_CustomFirst]=motions;
    CHECK(nativeScriptFighter_CanPublish(Ft_Kind_CustomFirst));nativeScriptFighter_PublishProgram(Ft_Kind_CustomFirst,&package.program);
}
static void userdata_remove(void* data){nativeScriptFighter_Detach(((Fighter*)data)->gobj);}
static HSD_GObj* create(void)
{
    CHECK(allocated<95);HSD_GObj* o=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(o);
    Fighter* f=&fighters[allocated++];memset(f,0,sizeof(*f));f->gobj=o;f->kind=f->behavior_kind=Ft_Kind_CustomFirst;
    f->ft_data=&package.assets.character.data;f->x8_spawnNum=allocated;f->parts=bones;
    GObj_InitUserData(o,4,userdata_remove,f);CHECK(nativeScriptFighter_Attach(o));return o;
}
static NativeFighterTrail seed(void){NativeFighterTrail t;memset(&t,0xA5,sizeof(t));return t;}
static NativeScriptValue sentinel(void){return(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xabcdef1234567890)};}
static NativeScriptVMStatus run(HSD_GObj* o,NativeScriptEventRecordIO* io)
{++vm_calls;return nativeScriptFighter_Run(o,NativeScriptFighter_Trail,NULL,0,io,NULL);}
static NativeScriptVMStatus execute(HSD_GObj* o,NativeScriptEventRecordIO* io,NativeScriptValue* out)
{
    Fighter* f=o->user_data;NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(o,f->x8_spawnNum,&view));
    NativeScriptExecution e={.declarations=&bank.declarations,.code=&bank.code,.storage=view.storage,
        .vm=nativeScriptRuntime_VM(),.payloads=nativeScriptRuntime_Payloads(),.functions=bank.execution_functions,
        .function_count=bank.declarations.function_count,.fighter_kind=Ft_Kind_CustomFirst,
        .receiver_lookup=nativeScriptActors_Lookup,.receiver_helper=nativeScriptActors_Helper};NativeScriptDiagnostic d={0};
    CHECK(nativeScriptExecution_Bind(&e,&d));++vm_calls;
    NativeScriptVMStatus result=nativeScriptExecution_Run(&e,o,NativeCallbackSource_Fighter,f->x8_spawnNum,17,nativeGObjPool.lifetime,
        package.program.hooks[NativeScriptFighter_Trail],NULL,0,io,out);
    if(result!=NativeScriptVM_Ok)fprintf(stderr,"execution status%u %s\n",result,e.vm->diagnostic.message);
    return result;
}
static void pairs_case(unsigned form)
{
    prepare(form?trail_source_1:trail_source_0);HSD_GObj* o=create();Fighter* f=o->user_data;
    Fighter stock={.kind=form?Ft_Kind_CLink:Ft_Kind_Link,.parts=bones,.dat_attrs=&attrs};HSD_GObj native={.user_data=&stock};
    NativeFighterTrail expected;HSD_JObj* expected_anchor=NULL;CHECK(read_trail(&native,&expected,&expected_anchor));
    NativeFighterTrail got=seed();NativeScriptEventRecordIO io={&got,sizeof(got)};CHECK(run(o,&io)==NativeScriptVM_Ok);
    if(memcmp(&got,&expected,sizeof(got))){uint32_t a[14],b[14];memcpy(a,&got,sizeof(a));memcpy(b,&expected,sizeof(b));
        for(unsigned i=0;i<14;++i)fprintf(stderr,"field%u got%08x native%08x\n",i,a[i],b[i]);}
    CHECK(!memcmp(&got,&expected,sizeof(got)));++pairs;++cases;
    NativeScriptValue result=sentinel();got=seed();CHECK(execute(o,&io,&result)==NativeScriptVM_Ok);
    CHECK(!memcmp(&got,&expected,sizeof(got))&&result.type==NativeScriptType_Void);++pairs;++cases;
    HSD_JObj* anchor=NULL;static Fighter before;before=*f;CHECK(read_trail(o,&got,&anchor));
    CHECK(anchor==expected_anchor&&anchor==bones[attrs.x64.x14].joint&&!memcmp(&got,&expected,sizeof(got))&&!memcmp(f,&before,sizeof(*f)));++pairs;++cases;
    // Rig/package and anchor failures are reader admission failures, not native
    // callback fallback. Restore every peer before the next grouped case.
    for(unsigned i=0;i<12;++i){++cases;HSD_JObj* old=bones[attrs.x64.x14].joint;anchor=(HSD_JObj*)(uintptr_t)1;
        switch(i){case 0:f->parts=NULL;break;case 1:package.assets.parts.parts_num=0;break;
        case 2:package.assets.facts.omission_count=0;break;case 3:tables[Ft_Kind_CustomFirst]=NULL;break;
        case 4:package.assets.parts.part_to_joint=NULL;break;case 5:bones[attrs.x64.x14].joint=NULL;break;
        case 6:old->object.parent.class_info=(HSD_ClassInfo*)&bad_class;break;
        case 7:((HSD_ClassInfo*)&hsdJObj)->head.obj_size=1;break;case 8:f->behavior_kind=Ft_Kind_Link;break;
        case 9:gFtDataList[Ft_Kind_CustomFirst]=NULL;break;case 10:package.assets.facts.moves.count=2;break;
        case 11:o->classifier=HSD_GOBJ_CLASS_ITEM;break;}
        CHECK(!read_trail(o,&got,&anchor)&&anchor==(HSD_JObj*)(uintptr_t)1);
        f->parts=bones;package.assets.parts.parts_num=50;package.assets.facts.omission_count=1;
        tables[Ft_Kind_CustomFirst]=&package.assets.parts;package.assets.parts.part_to_joint=package.assets.part_to_joint;
        bones[attrs.x64.x14].joint=old;old->object.parent.class_info=(HSD_ClassInfo*)&hsdJObj;
        ((HSD_ClassInfo*)&hsdJObj)->head.obj_size=sizeof(HSD_JObj);f->behavior_kind=Ft_Kind_CustomFirst;
        gFtDataList[Ft_Kind_CustomFirst]=&package.assets.character.data;package.assets.facts.moves=layout;
        o->classifier=HSD_GOBJ_CLASS_FIGHTER;
    }
    CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Ok);HSD_GObjFree(o);
}
static void wrong_io(void)
{
    prepare(trail_source_0);
    for(unsigned i=0;i<6;++i){++cases;HSD_GObj* o=create();NativeFighterTrail value=seed(),before=value;
        NativeScriptEventRecordIO io={&value,sizeof(value)};if(i%3==0)io.bytes--;if(i%3==1)io.data=NULL;
        NativeScriptEventRecordIO* ptr=i%3==2?NULL:&io;NativeScriptValue out=sentinel(),saved=out;
        if(i<3)CHECK(run(o,ptr)==NativeScriptVM_Error);else CHECK(execute(o,ptr,&out)==NativeScriptVM_Error);
        CHECK(!memcmp(&value,&before,sizeof(value))&&!memcmp(&out,&saved,sizeof(out)));CHECK(nativeScriptRuntime_Idle());HSD_GObjFree(o);
    }
}
static void failure_cases(void)
{
    prepare("action Ribbon:\n    on trail:\n        trail.bone = 2\n        let float observed = position.x\n        trail.axis = 1\n");
    for(unsigned mode=1;mode<=5;++mode)for(unsigned direct=0;direct<2;++direct){++cases;HSD_GObj* o=create();Fighter* f=o->user_data;
        NativeFighterTrail value=seed(),before=value;NativeScriptEventRecordIO io={&value,sizeof(value)};NativeScriptValue out=sentinel(),saved=out;
        injection=mode;NativeScriptVMStatus status=direct?execute(o,&io,&out):run(o,&io);CHECK(status!=NativeScriptVM_Ok);
        CHECK(!memcmp(&value,&before,sizeof(value))&&!memcmp(&out,&saved,sizeof(out)));CHECK(!injection&&nativeScriptRuntime_Idle());
        if(mode!=2){o->user_data=f;o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);} }
    // The reader must not fall through to stock/advanced policy after a fault.
    HSD_GObj* o=create();NativeFighterTrail value=seed();NativeScriptEventRecordIO io={&value,sizeof(value)};injection=1;
    CHECK(run(o,&io)==NativeScriptVM_Error&&nativeScriptFighter_Status(o)==NativeScriptVM_Error);
    HSD_JObj* anchor=(HSD_JObj*)(uintptr_t)1;CHECK(!read_trail(o,&value,&anchor)&&anchor==(HSD_JObj*)(uintptr_t)1);++cases;HSD_GObjFree(o);
    // Class substitution is rejected by the actual reader after VM success;
    // the raw VM provider does not classify this as source retirement.
    o=create();anchor=(HSD_JObj*)(uintptr_t)1;injection=6;
    CHECK(!read_trail(o,&value,&anchor)&&anchor==(HSD_JObj*)(uintptr_t)1);++cases;
    o->classifier=HSD_GOBJ_CLASS_FIGHTER;HSD_GObjFree(o);
    // Attach bound the original immutable storage schema.
    o=create();value=seed();NativeFighterTrail before=value;io=(NativeScriptEventRecordIO){&value,sizeof(value)};
    uint64_t identity=bank.storage_layout.identity;bank.storage_layout.identity^=1;
    CHECK(run(o,&io)==NativeScriptVM_Error&&!memcmp(&value,&before,sizeof(value)));++cases;
    bank.storage_layout.identity=identity;HSD_GObjFree(o);
}
static void invalid_values(void)
{
    const char* bodies[]={"trail.axis = 0", "trail.bone = -1", "trail.bone = 50", "trail.bone = 2\n        trail.axis = 3",
        "trail.bone = 2\n        trail.start_alpha = 256", "trail.bone = 2\n        trail.outer_red = -1",
        "trail.bone = 2\n        trail.inner_distance = attrs.sword_inner_distance"};
    for(unsigned i=0;i<7;++i){++cases;char source[512];snprintf(source,sizeof(source),"action Ribbon:\n    on trail:\n        %s\n",bodies[i]);
        float saved=attrs.x64.x18;if(i==6)attrs.x64.x18=NAN;prepare(source);attrs.x64.x18=saved;
        HSD_GObj* o=create();NativeFighterTrail value;HSD_JObj* anchor=(HSD_JObj*)(uintptr_t)1;
        CHECK(!read_trail(o,&value,&anchor)&&anchor==(HSD_JObj*)(uintptr_t)1);CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Ok);HSD_GObjFree(o);
    }
    prepare("func Unrelated():\n    let int count = 1\n");HSD_GObj* o=create();NativeFighterTrail value;HSD_JObj* anchor=(HSD_JObj*)(uintptr_t)1;
    CHECK(!nativeScriptFighter_HasHook(o,NativeScriptFighter_Trail)&&!read_trail(o,&value,&anchor));
    CHECK(nativeScriptFighter_Status(o)==NativeScriptVM_Ok&&anchor==(HSD_JObj*)(uintptr_t)1);++cases;HSD_GObjFree(o);
}
int main(void)
{
    HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&config);
    ((HSD_ClassInfo*)&hsdJObj)->head.flags=1;((HSD_ClassInfo*)&hsdJObj)->head.obj_size=sizeof(HSD_JObj);
    ((HSD_ClassInfo*)&bad_class)->head.flags=1;((HSD_ClassInfo*)&bad_class)->head.obj_size=sizeof(HSD_JObj);
    for(unsigned i=0;i<50;++i){joints[i].object.parent.class_info=(HSD_ClassInfo*)&hsdJObj;bones[i].joint=&joints[i];}
    attrs.x64=(struct SwordAttrs){.x0=.15f,.x4=.85f,.x8=192,.x9=0,.xA=255,.xB=93,.xC=17,.xE=101,.xF=142,.x10=255,.x14=38,.x18=2.25f,.x1C=13.5f};
    pairs_case(0);attrs.x64.x0=.67f;attrs.x64.x14=49;attrs.x64.x18=-0.0f;attrs.x64.x1C=27.125f;pairs_case(1);
    unsigned legacy=trail_legacy_queries;wrong_io();failure_cases();invalid_values();CHECK(nativeScriptRuntime_Idle());
    CHECK(trail_legacy_queries==legacy);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"native_pairs\":%u,\"explicit_vm_calls\":%u,\"observation_reads\":%u}\n",checks,cases,pairs,vm_calls,reads);return 0;
}
