#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/types.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_script_item_actor_internal.h>
#include <melee/it/native_script_item_snapshot.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SourceBytes=8192, Nodes=256, Expressions=1024, Functions=32, Fields=64 };
typedef struct Buffers {
    char source[SourceBytes];
    NativeScriptSourceNode nodes[Nodes];
    NativeScriptIRNode expressions[Expressions]; uint32_t arguments[Expressions];
    NativeScriptFunctionDeclaration functions[Functions];
    NativeScriptParameterDeclaration parameters[64];
    NativeScriptValueDeclaration fields[Fields]; NativeScriptLocal locals[32];
    NativeScriptInstruction instructions[2048]; NativeScriptFunctionCode code[Functions];
    NativeScriptExecutionFunction executions[Functions]; NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField storage[Fields]; NativeScriptStorageOwner owners[8];
    NativeScriptParameterSlot slots[Fields]; NativeScriptParameterSupply supplies[Fields];
} Buffers;
static Buffers buffers[3];
static NativeScriptCompileBank banks[3];
static NativeScriptItemProgram specs[2];
static NativeScriptItemState states[2];
static Article article_asset;
static Item items[2]; static Fighter creator_data;
static NativeScriptDiagnostic diagnostic;
static unsigned checks,cases,vm_calls;
static void static_data_remove(void* data) { (void)data; }
static uint8_t packed[32768],bad[32768],actor_before[sizeof(nativeScriptItemActors)];
static uint16_t index_before[NATIVE_GOBJ_CAPACITY];
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"%u: %s (%s)\n",(unsigned)__LINE__,#x,diagnostic.message); exit(1); } } while(0)

static const char source[]=
    "fighter:\n    behavior independent\n"
    "state int root_secret\nparameter float gain = 1.25\n"
    "attribute float nan_value 4\nattribute float zero_value 8\n"
    "parameter uint word = 0x80000001\nattribute bool flag 16\nattribute int signed_word 20\nattribute byte byte_value 24\n"
    "func Shared() -> float:\n    return program.attrs.gain\n"
    "func SharedEntity() -> int:\n    return program.entity_params.Other.only_other\n"
    "article Probe states 1:\n"
    "    parameter int gain = 7\n    state int tag\n"
    "    func RootGain() -> float:\n        return program.attrs.gain\n"
    "    func LocalGain() -> int:\n        return attrs.gain\n"
    "    func ItemGain() -> int:\n        return item.params.gain\n"
    "    func EntityGain() -> int:\n        return program.entity_params.Probe.gain\n"
    "    func OtherGain() -> int:\n        return program.entity_params.Other.only_other\n"
    "    func EntityNan() -> float:\n        return program.entity_params.Other.nan_value\n"
    "    func EntityZero() -> float:\n        return program.entity_params.Other.zero_value\n"
    "    func Nan() -> float:\n        return program.attrs.nan_value\n"
    "    func Zero() -> float:\n        return program.attrs.zero_value\n"
    "    func Word() -> uint:\n        return program.attrs.word\n"
    "    func Flag() -> bool:\n        return program.attrs.flag\n"
    "    func Signed() -> int:\n        return program.attrs.signed_word\n"
    "    func Byte() -> uint:\n        return program.attrs.byte_value\n"
    "    on created:\n        item.state.tag = item.state.tag + int(program.attrs.gain) + attrs.gain\n"
    "    state 0 anim -1:\n"
    "article Other states 1:\n    parameter int only_other = 3\n    attribute float nan_value 4\n    attribute float zero_value 8\n    state 0 anim -1:\n";

static NativeScriptValue sentinel(void)
{ return (NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_C(0xfedcba9876543210)}; }
static uint32_t bits(NativeScriptValue value)
{ uint32_t result; if(value.type==NativeScriptType_F32)memcpy(&result,&value.as.f32,4); else result=(uint32_t)value.as.u64; return result; }
static uint32_t value_id(const NativeScriptCompileBank* bank,uint32_t owner,const char* name)
{
    for(uint32_t i=0;i<bank->declarations.value_count;++i) {
        const NativeScriptValueDeclaration* v=&bank->declarations.values[i];
        if(v->owner==owner && nativeScriptSource_SpanEquals(&bank->tree,v->name,name))return i;
    }
    CHECK(false); return UINT32_MAX;
}
static uint32_t helper(const NativeScriptCompileBank* bank,uint32_t owner,const char* name)
{
    for(uint32_t i=0;i<bank->declarations.function_count;++i) {
        const NativeScriptFunctionDeclaration* f=&bank->declarations.functions[i];
        if(f->owner==owner && !f->callback && nativeScriptSource_SpanEquals(&bank->tree,f->name,name))return i;
    }
    CHECK(false); return UINT32_MAX;
}
static NativeScriptItemActor* actor(Item_GObj* object)
{
    for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)
        if(nativeScriptItemActors[i].active && nativeScriptItemActors[i].object==object)return &nativeScriptItemActors[i];
    CHECK(false); return NULL;
}
const NativeScriptItemProgram* nativeScriptItem_Program(FighterKind author,uint32_t owner)
{
    for(unsigned i=0;i<2;++i)if(specs[i].bank && specs[i].author_kind==author && specs[i].owner==owner)return &specs[i];
    return NULL;
}
// Controlled entity schema/assets: real parser, signatures, typed bodies and
// compilation are used; no metadata line is silently accepted.
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t node,NativeScriptDiagnostic* d)
{
    (void)unused; const NativeScriptSourceNode* n=&tree->nodes[node];
    if(n->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,n->value,"states 1");
    if(n->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,n->value,"0 anim -1");
    if(n->kind==NativeScriptSource_Function || n->kind==NativeScriptSource_State ||
       n->kind==NativeScriptSource_Parameter || n->kind==NativeScriptSource_Attribute) {
        if(n->parent<tree->count && tree->nodes[n->parent].kind==NativeScriptSource_Article)return true;
        // Root attributes are typed imports validated by the real index/provider.
        if(n->kind==NativeScriptSource_Attribute && n->parent==0)return true;
    }
    if(n->kind==NativeScriptSource_Callback){NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,n,&event,d);}
    return nativeScriptCompile_MinimalMetadata(NULL,tree,node,d);
}
static bool supply(void* context,const NativeScriptProgramIndex* index,const NativeScriptParameterSupply** output,
    uint32_t* count,NativeScriptDiagnostic* d)
{
    (void)d; unsigned bank_number=*(unsigned*)context; Buffers* b=&buffers[bank_number]; *count=0;
    for(uint32_t i=0;i<index->value_count;++i) {
        const NativeScriptValueDeclaration* v=&index->values[i]; if(!v->imported)continue;
        uint32_t raw=0;
        switch(v->import_offset) {
        case 4:raw=bank_number?UINT32_C(0x7fc23456):UINT32_C(0x7fc12345);break;
        case 8:raw=bank_number?0:UINT32_C(0x80000000);break;
        case 16:raw=bank_number?0:1;break;
        case 20:raw=bank_number?UINT32_C(0x80000000):UINT32_C(0xffffffff);break;
        case 24:raw=bank_number?0:UINT32_C(0xfe);break;
        default:return false;
        }
        NativeScriptValue value={.type=v->type};
        if(v->type==NativeScriptType_F32)memcpy(&value.as.f32,&raw,4);
        else if(v->type==NativeScriptType_I32)value.as.i64=raw<=INT32_MAX?(int64_t)raw:-1-(int64_t)(UINT32_MAX-raw);
        else value.as.u64=raw;
        b->supplies[(*count)++]=(NativeScriptParameterSupply){NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,v->owner,value};
    }
    // Explicit root override differs between two programs with identical names.
    if(bank_number<2)for(uint32_t i=0;i<index->value_count;++i)
        if(index->values[i].owner==0 && nativeScriptSource_SpanEquals(index->tree,index->values[i].name,"gain"))
            b->supplies[(*count)++]=(NativeScriptParameterSupply){NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,0,
                {.type=NativeScriptType_F32,.as.f32=bank_number?5.5f:1.25f}};
    if(bank_number<2)for(uint32_t i=0;i<index->value_count;++i)
        if(index->values[i].owner==0 && nativeScriptSource_SpanEquals(index->tree,index->values[i].name,"word"))
            b->supplies[(*count)++]=(NativeScriptParameterSupply){NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,0,
                {.type=NativeScriptType_U32,.as.u64=bank_number?UINT32_MAX:UINT32_C(0x80000001)}};
    for(uint32_t i=0;i<index->value_count;++i)
        if(index->values[i].parameter&&index->values[i].owner&&nativeScriptSource_SpanEquals(index->tree,index->values[i].name,"only_other"))
            b->supplies[(*count)++]=(NativeScriptParameterSupply){NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,index->values[i].owner,
                {.type=NativeScriptType_I32,.as.i64=bank_number?13:3}};
    *output=b->supplies;return true;
}
static void reserve(unsigned number)
{
    Buffers* b=&buffers[number]; banks[number]=(NativeScriptCompileBank){
        .source_storage=b->source,.source_capacity=SourceBytes,.tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=Expressions,.arguments=b->arguments,.argument_capacity=Expressions},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=64,.values=b->fields,.value_capacity=Fields},
        .locals=b->locals,.local_capacity=32,.instructions=b->instructions,.instruction_capacity=2048,
        .function_code=b->code,.function_code_capacity=Functions,.execution_functions=b->executions,.execution_function_capacity=Functions,
        .graph=b->graph,.graph_capacity=Functions,.storage_layout={.fields=b->storage,.field_capacity=Fields,.owners=b->owners,.owner_capacity=8},
        .parameter_slots=b->slots,.parameter_slot_capacity=Fields};
}
static void prepare(unsigned number)
{
    reserve(number);NativeScriptCompileRequest request={.source=source,.source_bytes=sizeof(source)-1,
        .metadata=metadata,.parameter_provider=supply,.parameter_context=&number};const NativeScriptCompileBank* accepted=NULL;
    CHECK(nativeScriptCompile_Build(&banks[number],&request,&accepted,&diagnostic));CHECK(accepted==&banks[number]);
    NativeScriptItemProgram* p=&specs[number];uint32_t owner=UINT32_MAX;
    for(uint32_t i=0;i<banks[number].tree.count;++i)if(banks[number].tree.nodes[i].kind==NativeScriptSource_Article){owner=i;break;}
    CHECK(owner!=UINT32_MAX);*p=(NativeScriptItemProgram){.bank=&banks[number],.identity=number+100,
        .author_kind=number?Ft_Kind_Luigi:Ft_Kind_Mario,.kind=It_Kind_Mario_Fire,.recipe_index=UINT32_MAX,.owner=owner,
        .article=&article_asset,.states=&states[number],.state_count=1,.drawing=NativeScriptItemDrawing_Standard};
    for(unsigned h=0;h<NativeScriptItemHook_Count;++h)p->hooks[h]=UINT32_MAX;
    states[number].animation_id=-1;for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)states[number].functions[phase]=UINT32_MAX;
    for(uint32_t i=0;i<banks[number].declarations.function_count;++i)
        if(banks[number].declarations.functions[i].callback && banks[number].declarations.functions[i].owner==owner)p->hooks[NativeScriptItemHook_Created]=i;
    CHECK(nativeScriptItem_ProgramValid(p));++cases;
}
static Item_GObj* create(unsigned number,Fighter_GObj* creator)
{
    Item_GObj* object=GObj_Create(HSD_GOBJ_CLASS_ITEM,HSD_GOBJ_PLINK_ITEM,0);CHECK(object);
    Item* item=&items[number];item->entity=object;item->kind=specs[number].kind;item->x1C=(s32)(number+7);item->msid=-1;
    item->native_article_author_kind=specs[number].author_kind;item->native_script_owner=specs[number].owner;item->xC4_article_data=&article_asset;
    GObj_InitUserData(object,HSD_GOBJ_CLASS_ITEM,static_data_remove,item);nativeItem_CaptureCreator(object,creator);
    CHECK(nativeScriptItem_Attach(object,&specs[number]));return object;
}
static NativeScriptValue run(Item_GObj* object,const char* name)
{
    NativeScriptItemActor* a=actor(object);NativeScriptValue result=sentinel();++vm_calls;
    CHECK(nativeScriptExecution_Run(&a->execution,object,NativeCallbackSource_Item,a->generation,vm_calls,
        nativeScriptItemSceneEpoch,helper(a->program->bank,a->program->owner,name),NULL,0, NULL,&result)==NativeScriptVM_Ok);
    CHECK(nativeScriptRuntime_Idle());return result;
}
static void values(Item_GObj* object,unsigned number)
{
    NativeScriptValue value=run(object,"RootGain");CHECK(value.type==NativeScriptType_F32 && value.as.f32==(number?5.5f:1.25f));
    value=run(object,"LocalGain");CHECK(value.type==NativeScriptType_I32 && value.as.i64==7);
    value=run(object,"ItemGain");CHECK(value.type==NativeScriptType_I32 && value.as.i64==7);
    value=run(object,"EntityGain");CHECK(value.type==NativeScriptType_I32 && value.as.i64==7);
    value=run(object,"OtherGain");CHECK(value.type==NativeScriptType_I32 && value.as.i64==(number?13:3));
    value=run(object,"EntityNan");CHECK(value.type==NativeScriptType_F32 && bits(value)==(number?UINT32_C(0x7fc23456):UINT32_C(0x7fc12345)));
    value=run(object,"EntityZero");CHECK(value.type==NativeScriptType_F32 && bits(value)==(number?0:UINT32_C(0x80000000)));
    value=run(object,"Nan");CHECK(value.type==NativeScriptType_F32 && bits(value)==(number?UINT32_C(0x7fc23456):UINT32_C(0x7fc12345)));
    value=run(object,"Zero");CHECK(value.type==NativeScriptType_F32 && bits(value)==(number?0:UINT32_C(0x80000000)));
    value=run(object,"Word");CHECK(value.type==NativeScriptType_U32 && value.as.u64==(number?UINT32_MAX:UINT32_C(0x80000001)));
    value=run(object,"Flag");CHECK(value.type==NativeScriptType_Bool && value.as.u64==!number);
    value=run(object,"Signed");CHECK(value.type==NativeScriptType_I32 && value.as.i64==(number?INT32_MIN:-1));++cases;
    value=run(object,"Byte");CHECK(value.type==NativeScriptType_U32 && value.as.u64==(number?0:254));
}
static void denied_read(NativeScriptStorageInstance* storage,uint32_t semantic)
{
    NativeScriptValue result=sentinel(),before=result;
    CHECK(!nativeScriptStorage_Read(storage,semantic,NULL,0,&result,&diagnostic));CHECK(!memcmp(&result,&before,sizeof(result)));++cases;
}
static void runtime_denials(Item_GObj* object)
{
    NativeScriptItemActor* a=actor(object);NativeScriptCompileBank* b=&banks[0];uint32_t root=value_id(b,0,"gain");
    uint32_t root_semantic=NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|root;
    denied_read(&a->storage,NATIVE_SCRIPT_PARAMETER_SEMANTIC|root);
    denied_read(&a->storage,NATIVE_SCRIPT_STATE_SEMANTIC|value_id(b,0,"root_secret"));
    NativeScriptValue package_value;
    CHECK(nativeScriptStorage_Read(&a->storage,NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|value_id(b,specs[0].owner,"gain"),NULL,0,&package_value,&diagnostic));
    CHECK(package_value.type==NativeScriptType_I32&&package_value.as.i64==7);
    denied_read(&a->storage,NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|value_id(b,specs[0].owner,"tag"));
    uint32_t other_owner=b->storage_layout.owners[2].owner;
    uint32_t entity_semantic=NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|value_id(b,other_owner,"only_other");
    CHECK(nativeScriptStorage_Read(&a->storage,entity_semantic,NULL,0,&package_value,&diagnostic));
    CHECK(package_value.type==NativeScriptType_I32&&package_value.as.i64==3);
    NativeScriptValue denied_write={.type=NativeScriptType_I32,.as.i64=99};
    NativeScriptParameterSlot entity_before=b->parameter_slots[entity_semantic&NATIVE_SCRIPT_STORAGE_INDEX_MASK];
    CHECK(!nativeScriptStorage_Write(&a->storage,entity_semantic,NULL,0,&denied_write,&diagnostic));
    CHECK(!memcmp(&entity_before,&b->parameter_slots[entity_semantic&NATIVE_SCRIPT_STORAGE_INDEX_MASK],sizeof(entity_before)));
    b->parameter_slots[entity_semantic&NATIVE_SCRIPT_STORAGE_INDEX_MASK].resolved=false;
    denied_read(&a->storage,entity_semantic);
    b->parameter_slots[entity_semantic&NATIVE_SCRIPT_STORAGE_INDEX_MASK]=entity_before;
    denied_read(&a->storage,NATIVE_SCRIPT_PARAMETER_SEMANTIC|value_id(b,other_owner,"only_other"));
    denied_read(&a->storage,NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|Fields);
    NativeScriptValue input={.type=NativeScriptType_F32,.as.f32=77};NativeScriptParameterSlot saved[Fields];memcpy(saved,b->parameter_slots,sizeof(saved));
    CHECK(!nativeScriptStorage_Write(&a->storage,root_semantic,NULL,0,&input,&diagnostic));CHECK(!memcmp(saved,b->parameter_slots,sizeof(saved)));++cases;
    uint32_t nan=value_id(b,0,"nan_value");NativeScriptParameterSlot slot=b->parameter_slots[nan];
    b->parameter_slots[nan].resolved=false;denied_read(&a->storage,NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|nan);
    NativeScriptValue result=sentinel(),before=result;++vm_calls;
    CHECK(nativeScriptExecution_Run(&a->execution,object,NativeCallbackSource_Item,a->generation,vm_calls,nativeScriptItemSceneEpoch,
        helper(b,specs[0].owner,"Nan"),NULL,0, NULL,&result)==NativeScriptVM_Error);
    CHECK(!memcmp(&result,&before,sizeof(result)) && nativeScriptRuntime_Idle());b->parameter_slots[nan]=slot;
    b->parameter_slots[nan].value.type=NativeScriptType_U32;denied_read(&a->storage,NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC|nan);b->parameter_slots[nan]=slot;
    const NativeScriptStorageParameters* parameters=a->storage.parameters;a->storage.parameters=&banks[1].storage_parameters;
    result=before;++vm_calls;CHECK(nativeScriptExecution_Run(&a->execution,object,NativeCallbackSource_Item,a->generation,vm_calls,nativeScriptItemSceneEpoch,
        helper(b,specs[0].owner,"RootGain"),NULL,0, NULL,&result)==NativeScriptVM_Error);CHECK(!memcmp(&result,&before,sizeof(result)));
    a->storage.parameters=parameters;uint64_t layout_identity=b->storage_parameters.layout_identity;
    ++b->storage_parameters.layout_identity;denied_read(&a->storage,root_semantic);b->storage_parameters.layout_identity=layout_identity;
    uint32_t count=b->storage_parameters.count;--b->storage_parameters.count;denied_read(&a->storage,root_semantic);b->storage_parameters.count=count;
    const NativeScriptParameterSlot* slots=b->storage_parameters.slots;b->storage_parameters.slots=NULL;denied_read(&a->storage,root_semantic);b->storage_parameters.slots=slots;
    uint64_t table_identity=b->storage_parameters.identity;b->storage_parameters.identity=0;denied_read(&a->storage,root_semantic);b->storage_parameters.identity=table_identity;
    nativeScriptStorage_Destroy(&a->storage);denied_read(&a->storage,root_semantic);
    result=before;++vm_calls;CHECK(nativeScriptExecution_Run(&a->execution,object,NativeCallbackSource_Item,a->generation,vm_calls,nativeScriptItemSceneEpoch,
        helper(b,specs[0].owner,"RootGain"),NULL,0, NULL,&result)==NativeScriptVM_Error);CHECK(!memcmp(&result,&before,sizeof(result)));a->storage.live=true;
    NativeScriptStorageParameters output={.identity=123};NativeScriptStorageParameters saved_output=output;
    NativeScriptParameterSlot slot_output[Fields];memcpy(slot_output,buffers[2].slots,sizeof(slot_output));
    NativeScriptParameterSupply invalid={root_semantic,0,input};
    CHECK(!nativeScriptStorage_PrepareParameters(&b->storage_layout,buffers[2].slots,Fields,&invalid,1,&output,&diagnostic));
    CHECK(!memcmp(&output,&saved_output,sizeof(output)) && !memcmp(slot_output,buffers[2].slots,sizeof(slot_output)));++cases;
    ++items[0].x1C;result=before;++vm_calls;
    CHECK(nativeScriptExecution_Run(&a->execution,object,NativeCallbackSource_Item,a->generation,vm_calls,nativeScriptItemSceneEpoch,
        helper(b,specs[0].owner,"RootGain"),NULL,0, NULL,&result)==NativeScriptVM_SourceRetired);
    CHECK(!memcmp(&result,&before,sizeof(result)) && nativeScriptRuntime_Idle());--items[0].x1C;++cases;
}
static void compile_denials(void)
{
    const char* denied[]={"program.attrs.gain = 2", "item.state.tag = program.state.root_secret",
        "item.state.tag = program.attrs.root_secret", "item.state.tag = program.attrs.only_other",
        "item.state.tag = attrs.only_other", "item.state.tag = program.attrs.missing", "item.state.tag = program.attrs.gain[0]",
        "item.state.tag = Shared()", "program.entity_params.Probe.gain = 9",
        "item.state.tag = program.entity_params.Probe.tag", "item.state.tag = program.entity_params.Missing.gain",
        "item.state.tag = program.entity_params.Probe.missing"};
    uint64_t accepted_identity=banks[0].storage_parameters.identity;char accepted_source[SourceBytes];memcpy(accepted_source,buffers[0].source,sizeof(accepted_source));
    for(unsigned i=0;i<sizeof(denied)/sizeof(*denied);++i) {
        char text[1024];int bytes=snprintf(text,sizeof(text),"fighter:\n    behavior independent\nparameter float gain = 1\nstate int root_secret\n"
            "func Shared() -> int:\n    return 1\narticle Probe states 1:\n    parameter int gain = 7\n    state int tag\n    on created:\n        %s\n    state 0 anim -1:\n"
            "article Other states 1:\n    parameter int only_other = 3\n    state 0 anim -1:\n",denied[i]);CHECK(bytes>0 && (unsigned)bytes<sizeof(text));
        reserve(2);NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)bytes,.metadata=metadata};
        const NativeScriptCompileBank* accepted=&banks[0];CHECK(!nativeScriptCompile_Build(&banks[2],&request,&accepted,&diagnostic));
        CHECK(diagnostic.message[0] && accepted==&banks[0] && banks[0].valid && banks[0].storage_parameters.identity==accepted_identity);
        CHECK(!memcmp(accepted_source,buffers[0].source,sizeof(accepted_source)));++cases;
    }
    reserve(2);const char* unsupported="fighter:\n    behavior independent\nattribute uint word 12\n";
    NativeScriptCompileRequest request={.source=unsupported,.source_bytes=(uint32_t)strlen(unsupported),.metadata=metadata};
    const NativeScriptCompileBank* accepted=&banks[0];CHECK(!nativeScriptCompile_Build(&banks[2],&request,&accepted,&diagnostic));
    CHECK(strstr(diagnostic.message,"explicit word-width contract") && accepted==&banks[0]);++cases;
}
static void snapshots(Item_GObj* first,Item_GObj* second)
{
    NativeScriptValue out=sentinel();CHECK(nativeScriptItem_RunHook(first,NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Ok);
    CHECK(nativeScriptItem_RunHook(second,NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Ok);vm_calls+=2;
    size_t bytes=0;CHECK(nativeScriptItem_Save(packed,sizeof(packed),&bytes));CHECK(bytes<sizeof(packed));
    CHECK(nativeScriptItem_RunHook(first,NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Ok);++vm_calls;
    NativeScriptItemActor* a=actor(first);uint32_t tag=value_id(&banks[0],specs[0].owner,"tag");
    CHECK(nativeScriptStorage_Read(&a->storage,NATIVE_SCRIPT_STATE_SEMANTIC|tag,NULL,0,&out,&diagnostic) && out.as.i64==16);
    CHECK(nativeScriptItem_Load(packed,bytes));a=actor(first);
    CHECK(a->storage.parameters==&banks[0].storage_parameters && actor(second)->storage.parameters==&banks[1].storage_parameters);
    CHECK(nativeScriptStorage_Read(&a->storage,NATIVE_SCRIPT_STATE_SEMANTIC|tag,NULL,0,&out,&diagnostic) && out.as.i64==8);
    values(first,0);values(second,1);++cases;
    // A real actor hook reading an unresolved root field faults its owner,
    // leaves caller output untouched, and recovers only through packed restore.
    uint32_t root_gain=value_id(&banks[0],0,"gain");NativeScriptParameterSlot original=banks[0].parameter_slots[root_gain];
    banks[0].parameter_slots[root_gain].resolved=false;out=sentinel();NativeScriptValue before=out;
    CHECK(nativeScriptItem_RunHook(first,NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Error);++vm_calls;
    CHECK(!memcmp(&out,&before,sizeof(out)) && actor(first)->faulted && nativeScriptRuntime_Idle());
    banks[0].parameter_slots[root_gain]=original;CHECK(nativeScriptItem_Status(first)==NativeScriptVM_Error);
    CHECK(nativeScriptItem_Load(packed,bytes) && nativeScriptItem_Status(first)==NativeScriptVM_Ok);++cases;
    memcpy(bad,packed,bytes);NativeScriptItemSnapshotRecord record;memcpy(&record,bad+sizeof(NativeScriptItemSnapshotHeader),sizeof(record));
    ++record.parameter_identity;memcpy(bad+sizeof(NativeScriptItemSnapshotHeader),&record,sizeof(record));
    memcpy(actor_before,nativeScriptItemActors,sizeof(actor_before));memcpy(index_before,nativeScriptItemActorByObject,sizeof(index_before));
    CHECK(!nativeScriptItem_Load(bad,bytes));CHECK(!memcmp(actor_before,nativeScriptItemActors,sizeof(actor_before)) && !memcmp(index_before,nativeScriptItemActorByObject,sizeof(index_before)));++cases;
    uint64_t identity=banks[0].storage_parameters.identity;++banks[0].storage_parameters.identity;
    CHECK(!nativeScriptItem_Load(packed,bytes));CHECK(!memcmp(actor_before,nativeScriptItemActors,sizeof(actor_before)));banks[0].storage_parameters.identity=identity;++cases;
}
int main(void)
{
    prepare(0);prepare(1);compile_denials();
    HSD_GObjLibInitDataType config={.p_link_max=32,.gx_link_max=2,.gproc_pri_max=8};HSD_GObj_80391304(&config);
    CHECK(nativeScriptItemSceneEpoch && nativeScriptItemSceneEpoch==nativeGObjPool.lifetime);
    Fighter_GObj* creator=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(creator);
    creator_data.gobj=creator;creator_data.kind=Ft_Kind_Mario;creator_data.x8_spawnNum=55;GObj_InitUserData(creator,HSD_GOBJ_CLASS_FIGHTER,static_data_remove,&creator_data);
    Item_GObj* first=create(0,creator);Item_GObj* second=create(1,NULL);
    CHECK(nativeScriptItem_Creator(first)==creator);values(first,0);values(second,1);
    NativeMatchPoolReference creator_reference;CHECK(nativeMatchPool_Reference(&nativeGObjPool,creator,&creator_reference));HSD_GObjFree(creator);
    CHECK(!nativeMatchPool_Resolve(creator_reference) && !nativeScriptItem_Creator(first));
    values(first,0);values(second,1);++cases;
    runtime_denials(first);values(first,0);snapshots(first,second);
    CHECK(nativeScriptRuntime_Idle() && nativeScriptItem_ReferencesProgram(&banks[0]) && nativeScriptItem_ReferencesProgram(&banks[1]));
    printf("{\"status\":\"PASS\",\"checks\":%u,\"cases\":%u,\"vm_calls\":%u,\"banks\":2,\"article_actors\":2}\n",checks,cases,vm_calls);return 0;
}
