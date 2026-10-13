#include <melee/ft/native_script_execution.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"%u: %s (%s)\n",__LINE__,#x,vm.diagnostic.message);exit(1);}} while(0)
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static HSD_GObj actor,article;
static Fighter fighter;
static Item item;
bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* generation)
{(void)object;(void)generation;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t generation)
{(void)object;(void)generation;return NULL;}

static const char source[]=
    "state float total\nstate int history\nparameter float gain = 0.25\n"
    "func Boost(float value) -> float:\n    return value + program.attrs.gain\n"
    "action Hooks:\n"
    "    on tick:\n        state.total = Boost(state.total + velocity.x)\n        position.y = state.total\n"
    "    on cpu_prepare:\n        state.history = event.history_source\n        if event.history_source < 0: return event.history_source\n"
    "    on throw_finish:\n        return state.total\n"
    "article projectile states 1:\n"
    "    state float total\n    parameter float gain = 0.5\n"
    "    on signal add:\n        item.state.total = item.state.total + signal.value + item.params.gain + program.attrs.gain\n"
    "    state 0 anim -1:\n        on animation:\n            return item.state.total\n";
static NativeScriptSourceNode syntax[256];
static NativeScriptIRNode expressions[512];
static uint32_t arguments[512];
static NativeScriptFunctionDeclaration declarations[16];
static NativeScriptParameterDeclaration parameter_declarations[32];
static NativeScriptValueDeclaration value_declarations[32];
static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[512];
static NativeScriptFunctionCode functions[16];
static NativeScriptValue registers[512];
static NativeScriptVMFrame frames[16];
static NativeScriptVM vm={.registers=registers,.register_capacity=512,.frames=frames,.frame_capacity=16};
static NativeScriptEventPayloadFrame payload_frames[16];
static NativeScriptEventPayloadPool payloads={.frames=payload_frames,.capacity=16};
static NativeScriptStorageField fields[32];
static NativeScriptStorageOwner owners[8];
static NativeScriptParameterSlot slots[32];
static uint8_t state_bytes[64],item_bytes[64];
static NativeScriptStorageInstance state,item_state;
static NativeScriptSourceTree tree={source,sizeof(source)-1,syntax,0,256};
static NativeScriptIRArena arena={source,sizeof(source)-1,expressions,0,512,arguments,0,512};
static NativeScriptProgramIndex program={.tree=&tree,.expressions=&arena,
    .functions=declarations,.function_capacity=16,.parameters=parameter_declarations,.parameter_capacity=32,
    .values=value_declarations,.value_capacity=32,.event_signature=nativeScriptEvents_Signature};
typedef struct Resolver {
    NativeScriptBindingResolverContext native;
    NativeScriptEventPayloadResolverContext payload;
    bool event;
} Resolver;

static bool resolve(void* user,const NativeScriptIRArena* input,const NativeScriptIRNode* node,
    NativeScriptIRResolution* output,NativeScriptDiagnostic* diagnostic)
{
    Resolver* resolver=user;
    if(resolver->event) {
        int result=nativeScriptEventPayload_TryResolve(&resolver->payload,input,node,output,diagnostic);
        if(result)return result>0;
    }
    return nativeScriptBindings_Resolve(&resolver->native,input,node,output,diagnostic);
}

static void build(void)
{
    NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    CHECK(nativeScriptProgram_Index(&program,&diagnostic));
    Resolver resolver={0};
    NativeScriptProgramCompiler compiler={.index=&program,.locals=locals,.local_capacity=64,
        .native_bindings=resolve,.native_binding_context=&resolver};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=512,
        .functions=functions,.function_count=program.function_count,
        .describe_call=nativeScriptBindings_DescribeCall,.binding_context=&resolver.native};
    for(uint32_t i=0;i<program.function_count;++i) {
        resolver.native.context_mask=declarations[i].owner?NativeScriptBindings_Article:NativeScriptBindings_Fighter;
        resolver.event=declarations[i].callback;
        if(resolver.event)CHECK(nativeScriptEventPayload_ResolverContext(&tree,declarations[i].source_node,&resolver.payload,&diagnostic));
        NativeScriptStatementCompiler body;
        if(!nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic)) {
            fprintf(stderr,"source %u: %s\n",diagnostic.source.line,diagnostic.message);exit(1);
        }
        CHECK(nativeScriptBytecode_Compile(&builder,&body,declarations[i].source_node,i,
            (uint16_t)declarations[i].parameter_count,&diagnostic));
    }
    static NativeScriptStorageLayout layout={.fields=fields,.field_capacity=32,.owners=owners,.owner_capacity=8};
    static NativeScriptStorageParameters parameters;
    CHECK(nativeScriptStorage_BuildLayout(&layout,&program,&diagnostic));
    CHECK(nativeScriptStorage_PrepareParameters(&layout,slots,32,NULL,0,&parameters,&diagnostic));
    CHECK(nativeScriptStorage_Create(&layout,&parameters,0,UINT32_C(0x80000001),UINT32_C(0x80000001),
        state_bytes,64,&state,&diagnostic));
    uint32_t item_owner=declarations[4].owner;
    CHECK(nativeScriptStorage_Create(&layout,&parameters,item_owner,7,7,item_bytes,64,&item_state,&diagnostic));
    static NativeScriptBytecodeProgram code;
    code=(NativeScriptBytecodeProgram){source,sizeof(source)-1,instructions,builder.instruction_count,functions,program.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&code,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    static NativeScriptExecutionFunction entries[16];
    CHECK(nativeScriptExecution_BuildFunctions(&program,&code,entries,16,&diagnostic));
    NativeScriptExecution execution={.declarations=&program,.code=&code,.storage=&state,.vm=&vm,
        .payloads=&payloads,.functions=entries,.function_count=program.function_count};
    CHECK(nativeScriptExecution_Bind(&execution,&diagnostic));
    NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};
    fighter.self_vel.x=0.5f;
    for(uint32_t tick=0;tick<128;++tick) {
        CHECK(nativeScriptExecution_Run(&execution,&actor,NativeCallbackSource_Fighter,UINT32_C(0x80000001),tick, 0,1,NULL,0, NULL,&result)==NativeScriptVM_Ok);
        CHECK(result.type==NativeScriptType_Void&&fighter.cur_pos.y==0.75f*(tick+1));
        CHECK(vm.depth==0&&vm.register_top==0&&payloads.depth==0);
    }
    NativeScriptValue history={.type=NativeScriptType_I32,.as.i64=-2147483647};
    CHECK(nativeScriptExecution_Run(&execution,&actor,NativeCallbackSource_Fighter,UINT32_C(0x80000001),128, 0,2,&history,1, NULL,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_I32&&result.as.i64==history.as.i64);
    CHECK(nativeScriptExecution_Run(&execution,&actor,NativeCallbackSource_Fighter,UINT32_C(0x80000001),128, 0,3,NULL,0, NULL,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Bool&&result.as.u64==1);
    execution.storage=&item_state;
    CHECK(nativeScriptExecution_Bind(&execution,&diagnostic));
    NativeScriptValue addition={.type=NativeScriptType_F32,.as.f32=2.5f};
    CHECK(nativeScriptExecution_Run(&execution,&article,NativeCallbackSource_Item,7,128, 0,4,&addition,1, NULL,&result)==NativeScriptVM_Ok);
    NativeScriptValue item_total;
    uint32_t item_total_id=UINT32_MAX;
    for(uint32_t i=0;i<program.value_count;++i)
        if(value_declarations[i].owner==item_owner&&nativeScriptSource_SpanEquals(&tree,value_declarations[i].name,"total"))item_total_id=i;
    CHECK(item_total_id!=UINT32_MAX);
    CHECK(nativeScriptStorage_Read(&item_state,NATIVE_SCRIPT_STATE_SEMANTIC|item_total_id,NULL,0,&item_total,&diagnostic));
    CHECK(item_total.type==NativeScriptType_F32&&item_total.as.f32==3.25f);
    CHECK(nativeScriptExecution_Run(&execution,&article,NativeCallbackSource_Item,7,128, 0,5,NULL,0, NULL,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_Bool&&result.as.u64==1);
    NativeScriptValue sentinel={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};result=sentinel;
    CHECK(nativeScriptExecution_Run(&execution,&article,NativeCallbackSource_Item,7,129, 0,1,NULL,0, NULL,&result)==NativeScriptVM_Error);
    CHECK(!memcmp(&result,&sentinel,sizeof(result)));
    execution.storage=&state;
    CHECK(nativeScriptExecution_Bind(&execution,&diagnostic));
    history.type=NativeScriptType_F32;
    CHECK(nativeScriptExecution_Run(&execution,&actor,NativeCallbackSource_Fighter,UINT32_C(0x80000001),129, 0,2,&history,1, NULL,&result)==NativeScriptVM_Error);
    CHECK(payloads.depth==0&&vm.depth==0&&vm.register_top==0&&!memcmp(&result,&sentinel,sizeof(result)));
    ++fighter.x8_spawnNum;
    CHECK(nativeScriptExecution_Run(&execution,&actor,NativeCallbackSource_Fighter,UINT32_C(0x80000001),130, 0,1,NULL,0, NULL,&result)==NativeScriptVM_SourceRetired);
    CHECK(!memcmp(&result,&sentinel,sizeof(result)));
    // The root actor has retired. The article's pinned program still owns its
    // immutable parameters; shared reads require no creator/owner lookup.
    execution.storage=&item_state;
    CHECK(nativeScriptExecution_Bind(&execution,&diagnostic));
    addition.as.f32=0;
    CHECK(nativeScriptExecution_Run(&execution,&article,NativeCallbackSource_Item,7,130, 0,4,&addition,1, NULL,&result)==NativeScriptVM_Ok);
    CHECK(nativeScriptStorage_Read(&item_state,NATIVE_SCRIPT_STATE_SEMANTIC|item_total_id,NULL,0,&item_total,&diagnostic));
    CHECK(item_total.as.f32==4.0f&&vm.depth==0&&vm.register_top==0&&payloads.depth==0);
    printf("{\"checks\":%u,\"ticks\":128,\"functions\":%u,\"status\":\"PASS actual actor, storage, event payload and interpreter entry\"}\n",checks,program.function_count);
}

int main(void)
{
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=&actor;heads[HSD_GOBJ_PLINK_ITEM]=&article;
    actor.user_data=&fighter;fighter.gobj=&actor;fighter.x8_spawnNum=-2147483647;
    article.user_data=&item;item.entity=&article;item.x1C=7;
    build();return 0;
}
