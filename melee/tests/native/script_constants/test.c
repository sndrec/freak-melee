#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/forward.h>
#include <melee/lb/forward.h>
#include <melee/mp/forward.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,host_reads;
#define CHECK(x) do {++checks;if (!(x)) {fprintf(stderr,"line %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static NativeScriptSourceNode syntax[256];
static NativeScriptIRNode nodes[1024];
static uint32_t arguments[1024];
static NativeScriptFunctionDeclaration declarations[16];
static NativeScriptParameterDeclaration parameters[32];
static NativeScriptValueDeclaration values[32];
static NativeScriptLocal locals[64];
static NativeScriptInstruction instructions[1024];
static NativeScriptFunctionCode functions[16];
static NativeScriptValue registers[128];
static NativeScriptVMFrame frames[8];

static bool resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* output,NativeScriptDiagnostic* diagnostic)
{
    int result=nativeScriptConstants_TryResolve(context,arena,node,output,diagnostic);
    if (!result) {diagnostic->source=node->source;snprintf(diagnostic->message,sizeof(diagnostic->message),"Unknown constant");}
    return result>0;
}

static bool alive(NativeScriptRuntimeContext* context) {(void)context;return true;}
static bool read_value(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    (void)context;(void)semantic;(void)selectors;(void)count;(void)value;(void)diagnostic;
    ++host_reads;return false;
}

static void constant(const char* name,NativeScriptIRType type,uint64_t bits)
{
    NativeScriptConstant value;CHECK(nativeScriptConstants_Lookup(name,(uint32_t)strlen(name),&value));
    CHECK(value.type==type);CHECK(value.bits==bits);CHECK(value.source_evidence[0]);
}

static void catalog(void)
{
    NativeScriptConstant previous={0},value;
    for (uint32_t i=0;i<nativeScriptConstants_Count();++i) {
        CHECK(nativeScriptConstants_At(i,&value));
        if (i) CHECK(strcmp(previous.name,value.name)<0);
        NativeScriptConstant found;CHECK(nativeScriptConstants_Lookup(value.name,(uint32_t)strlen(value.name),&found));
        CHECK(found.bits==value.bits && found.type==value.type && found.family==value.family);
        CHECK(value.type==NativeScriptType_I32 || value.type==NativeScriptType_U32);
        CHECK(value.source_evidence[0]);previous=value;
    }
    constant("LeftHand",NativeScriptType_I32,FtPart_L1stNb);
    constant("RightThumb",NativeScriptType_I32,FtPart_RThumbNb);
    constant("It_Kind_Mario_Fire",NativeScriptType_I32,It_Kind_Mario_Fire);
    constant("It_PKind_Tosakinto",NativeScriptType_I32,It_PKind_Start);
    constant("ftCo_MS_None",NativeScriptType_I32,UINT64_MAX);
    constant("Motion.held_fall_special",NativeScriptType_I32,ftCo_MS_ItemParasolFallSpecial);
    constant("Transition.common_ground_air",NativeScriptType_U32,ftCommon_GroundAirColl_MF);
    constant("Ft_MF_Unk31",NativeScriptType_U32,Ft_MF_Unk31);
    constant("Collision.floor_mask",NativeScriptType_I32,Collide_FloorMask);
    constant("ITEM_HOLD_12",NativeScriptType_I32,ITEM_HOLD_12);
    constant("ITEM_ANIM_UPDATE",NativeScriptType_I32,ITEM_ANIM_UPDATE);
    constant("Hitbox.enabled",NativeScriptType_I32,HitCapsule_Enabled);
    NativeScriptConstant before=value;
    CHECK(!nativeScriptConstants_Lookup("It_Kind_NotReal",15,&value));CHECK(!memcmp(&before,&value,sizeof(value)));
    CHECK(!nativeScriptConstants_Lookup("\"LeftHand\"",10,&value));
    CHECK(!nativeScriptConstants_Lookup("lefthand",8,&value));
    CHECK(!nativeScriptConstants_Lookup("NeutralSpecial",14,&value));
    CHECK(!nativeScriptConstants_At(nativeScriptConstants_Count(),&value));
    CHECK(!nativeScriptConstants_Lookup("LeftHand",8,NULL));
}

static void expressions(void)
{
    const char* accepted[]={"LeftHand + 1","Motion.fall_special","Ft_MF_Unk31 | Transition.skip_attack_count","\"LeftHand\""};
    NativeScriptDiagnostic diagnostic;
    for (unsigned i=0;i<sizeof(accepted)/sizeof(*accepted);++i) {
        const char* text=accepted[i];NativeScriptIRArena arena={.source=text,.source_bytes=(uint32_t)strlen(text),
            .nodes=nodes,.node_capacity=1024,.arguments=arguments,.argument_capacity=1024};uint32_t result;
        CHECK(nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){.length=arena.source_bytes,.line=1,.column=1},resolve,NULL,&result,&diagnostic));
        for (uint32_t j=0;j<arena.node_count;++j) {
            CHECK(!(nodes[j].flags & NativeScriptFlag_Literal));
            if (nodes[j].kind==NativeScriptIR_Integer) CHECK(nodes[j].semantic==NATIVE_SCRIPT_SEMANTIC_NONE);
        }
        if (i==1) CHECK(nodes[result].kind==NativeScriptIR_Integer && nodes[result].bits==ftCo_MS_FallSpecial);
        if (i==3) CHECK(nodes[result].kind==NativeScriptIR_String && nodes[result].type==NativeScriptType_Text);
    }
    const char* rejected[]={"It_Kind_NoSuch","Motion.no_such","Motion[0]","Motion()","LeftHand()","Motion.fall_special.extra"};
    for (unsigned i=0;i<sizeof(rejected)/sizeof(*rejected);++i) {
        const char* text=rejected[i];NativeScriptIRArena arena={.source=text,.source_bytes=(uint32_t)strlen(text),
            .nodes=nodes,.node_capacity=1024,.arguments=arguments,.argument_capacity=1024};uint32_t result;
        CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){.length=arena.source_bytes,.line=9,.column=1},resolve,NULL,&result,&diagnostic));
        CHECK(diagnostic.message[0] && diagnostic.source.line==9);CHECK(!arena.node_count && !arena.argument_count);
    }
    NativeScriptIRArena arena={.source="Motion",.source_bytes=6,.nodes=nodes,.node_count=1};
    NativeScriptIRNode name={.kind=NativeScriptIR_Name,.text_length=6,.a=NATIVE_SCRIPT_NODE_NONE};
    NativeScriptIRResolution space;NativeScriptConstant value;
    CHECK(nativeScriptConstants_LookupNode(&arena,&name,&value,&space,&diagnostic)==2);
    nodes[0]=name;nodes[0].type=space.type;nodes[0].semantic=space.semantic;
    NativeScriptIRNode bad={.kind=NativeScriptIR_Member,.a=0,.text_offset=100,.text_length=1};
    CHECK(nativeScriptConstants_LookupNode(&arena,&bad,&value,&space,&diagnostic)==-1);
    CHECK(nativeScriptConstants_TryResolve(NULL,&arena,&name,NULL,&diagnostic)==-1);
}

static void direct_program(void)
{
    const char* text=
        "func fire() -> int:\n    return It_Kind_Mario_Fire + LeftHand\n"
        "func flags() -> bool:\n    return (Ft_MF_Unk31 | Transition.skip_attack_count) == 0x82000000\n"
        "func motion() -> int:\n    return Motion.held_fall_special\n"
        "func negative() -> int:\n    return ftCo_MS_None\n"
        "func category() -> int:\n    return ITEM_HOLD_12 + ITEM_UNK_MATO\n"
        "func shadow() -> int:\n    let int LeftHand = 7\n    return LeftHand\n";
    NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=256};
    NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024,
        .arguments=arguments,.argument_capacity=1024};NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=declarations,.function_capacity=16,
        .parameters=parameters,.parameter_capacity=32,.values=values,.value_capacity=32};
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));CHECK(index.function_count==6);
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=locals,.local_capacity=64};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=1024,
        .functions=functions,.function_count=index.function_count};
    for (uint32_t i=0;i<index.function_count;++i) {
        NativeScriptStatementCompiler body;
        if (!nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic)) {
            fprintf(stderr,"body %u line %u: %s\n",i,diagnostic.source.line,diagnostic.message);exit(1);
        }
        CHECK(nativeScriptBytecode_Compile(&builder,&body,declarations[i].source_node,i,0,&diagnostic));
    }
    for (uint32_t i=0;i<builder.instruction_count;++i) CHECK(instructions[i].opcode!=NativeScriptBC_Read && instructions[i].opcode!=NativeScriptBC_Invoke);
    NativeScriptBytecodeProgram program={text,tree.source_bytes,instructions,builder.instruction_count,functions,index.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptVM vm={.registers=registers,.register_capacity=128,.frames=frames,.frame_capacity=8};
    NativeScriptVMHost host={.source_alive=alive,.read=read_value};NativeScriptRuntimeContext context={0};
    const int64_t expected[]={It_Kind_Mario_Fire+FtPart_L1stNb,1,
        ftCo_MS_ItemParasolFallSpecial,-1,ITEM_HOLD_12+ITEM_UNK_MATO,7};
    for (uint32_t i=0;i<index.function_count;++i) {
        NativeScriptValue result;CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,i,NULL,0,&result)==NativeScriptVM_Ok);
        CHECK(i==1 ? result.as.u64==(uint64_t)expected[i] : result.as.i64==expected[i]);
    }
    CHECK(host_reads==0);
    CHECK(vm.depth==0 && vm.register_top==0);
}

static void rejected_bodies(void)
{
    const char* rejected[]={
        "func invalid() -> int:\n    LeftHand = 9\n    return 1\n",
        "func invalid() -> int:\n    Motion.fall_special = 9\n    return 1\n",
        "func invalid() -> int:\n    return Motion\n",
        "func invalid() -> int:\n    return It_Kind_NotReal\n",
    };
    for (unsigned i=0;i<sizeof(rejected)/sizeof(*rejected);++i) {
        const char* text=rejected[i];
        NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=256};
        NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024,
            .arguments=arguments,.argument_capacity=1024};NativeScriptDiagnostic diagnostic;
        CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
        NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=declarations,.function_capacity=16,
            .parameters=parameters,.parameter_capacity=32,.values=values,.value_capacity=32};
        CHECK(nativeScriptProgram_Index(&index,&diagnostic));CHECK(index.function_count==1);
        NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=locals,.local_capacity=64};
        NativeScriptStatementCompiler body;
        CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
        CHECK(diagnostic.message[0] && diagnostic.source.line==2);
    }
}

static unsigned default_resolves;
static bool default_resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* output,NativeScriptDiagnostic* diagnostic)
{
    CHECK(context==&default_resolves);++default_resolves;
    if (node->kind==NativeScriptIR_Name) {
        const char* name=arena->source+node->text_offset;
#define NAME(s) (node->text_length==sizeof(s)-1 && !memcmp(name,s,sizeof(s)-1))
        if (NAME("exact_f32")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_F32,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
                .flags=NativeScriptFlag_Literal,.literal_bits=UINT32_C(0x3eaaaaab)};return true;
        }
        if (NAME("runtime")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_Namespace,.semantic=1};return true;
        }
        if (NAME("runtime_value")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=1,.flags=NativeScriptFlag_Readable};return true;
        }
        if (NAME("runtime_namespace")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_Namespace,.semantic=1,.flags=NativeScriptFlag_Readable};return true;
        }
        if (NAME("impure_literal")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
                .flags=NativeScriptFlag_Literal|NativeScriptFlag_MayInvalidateSource,.literal_bits=1};return true;
        }
        if (NAME("bad_bool")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_Bool,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
                .flags=NativeScriptFlag_Literal,.literal_bits=2};return true;
        }
        if (NAME("bad_u32")) {
            *output=(NativeScriptIRResolution){.type=NativeScriptType_U32,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,
                .flags=NativeScriptFlag_Literal,.literal_bits=UINT64_C(0x100000000)};return true;
        }
#undef NAME
    }
    if (node->kind==NativeScriptIR_Member && arena->nodes[node->a].semantic==1) {
        *output=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=2,.flags=NativeScriptFlag_Readable};return true;
    }
    return resolve(NULL,arena,node,output,diagnostic);
}

static NativeScriptProgramIndex default_index(NativeScriptSourceTree* tree,NativeScriptIRArena* arena)
{
    return (NativeScriptProgramIndex){.tree=tree,.expressions=arena,.functions=declarations,.function_capacity=16,
        .parameters=parameters,.parameter_capacity=32,.values=values,.value_capacity=32,
        .constant_resolver=default_resolve,.constant_context=&default_resolves};
}

static void parameter_defaults(void)
{
    const char* text=
        "parameter uint mask = Transition.skip_attack_count\n"
        "parameter uint high = Ft_MF_Unk31\n"
        "parameter int motion = ftCo_MS_Wait\n"
        "parameter int negative = ftCo_MS_None\n"
        "parameter float joint = LeftHand + 0.25\n"
        "parameter float rounded = 16777217\n"
        "parameter float fraction = exact_f32\n"
        "parameter bool lazy_or = true or (1 / 0 == 1)\n"
        "parameter bool lazy_and = false and (1 / 0 == 1)\n"
        "parameter int wrapped = 2147483647 + 1\n"
        "parameter int motion_alias = Motion.fall_special\n"
        "func readonly() -> int:\n    attrs.motion = 5\n    return 1\n";
    NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=256};
    NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024,
        .arguments=arguments,.argument_capacity=1024};NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptProgramIndex index=default_index(&tree,&arena);
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));CHECK(index.value_count==11);CHECK(default_resolves>0);
    CHECK(arena.node_count==0 && arena.argument_count==0);
    CHECK(values[0].default_value.type==NativeScriptType_U32 && values[0].default_value.as.u64==Ft_MF_SkipAttackCount);
    CHECK(values[1].default_value.type==NativeScriptType_U32 && values[1].default_value.as.u64==Ft_MF_Unk31);
    CHECK(values[2].default_value.type==NativeScriptType_I32 && values[2].default_value.as.i64==ftCo_MS_Wait);
    CHECK(values[3].default_value.type==NativeScriptType_I32 && values[3].default_value.as.i64==-1);
    CHECK(values[4].default_value.type==NativeScriptType_F32 && values[4].default_value.as.f32==FtPart_L1stNb+0.25f);
    CHECK(values[5].default_value.type==NativeScriptType_F32 && values[5].default_value.as.f32==16777216.0f);
    uint32_t fraction;memcpy(&fraction,&values[6].default_value.as.f32,4);CHECK(fraction==UINT32_C(0x3eaaaaab));
    CHECK(values[7].default_value.type==NativeScriptType_Bool && values[7].default_value.as.u64==1);
    CHECK(values[8].default_value.type==NativeScriptType_Bool && values[8].default_value.as.u64==0);
    CHECK(values[9].default_value.type==NativeScriptType_I32 && values[9].default_value.as.i64==INT32_MIN);
    CHECK(values[10].default_value.type==NativeScriptType_I32 && values[10].default_value.as.i64==ftCo_MS_FallSpecial);
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=locals,.local_capacity=64};
    NativeScriptStatementCompiler body;CHECK(!nativeScriptProgram_CompileBody(&compiler,0,&body,&diagnostic));
    CHECK(diagnostic.source.line==13);

    const char* rejected[]={
        "parameter int invalid = Motion.not_real\n",
        "parameter int invalid = runtime_value\n",
        "parameter int invalid = runtime.value\n",
        "parameter int invalid = impure_literal\n",
        "parameter int invalid = runtime_namespace.value\n",
        "parameter int invalid = LeftHand()\n",
        "parameter int invalid = Motion[0]\n",
        "parameter bool invalid = bad_bool\n",
        "parameter uint invalid = bad_u32\n",
        "parameter bool invalid = true or (runtime_value == 0)\n",
        "parameter int invalid = 1 / 0\n",
        "parameter bool invalid = LeftHand\n",
        "parameter int invalid = 0.5\n",
        "parameter bool invalid = true or (\"a\" == \"a\")\n",
    };
    for (unsigned i=0;i<sizeof(rejected)/sizeof(*rejected);++i) {
        const char* source_text=rejected[i];
        tree=(NativeScriptSourceTree){.source=source_text,.source_bytes=(uint32_t)strlen(source_text),.nodes=syntax,.capacity=256};
        arena=(NativeScriptIRArena){.source=source_text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024,
            .arguments=arguments,.argument_capacity=1024};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
        index=default_index(&tree,&arena);NativeScriptValueDeclaration before;
        memset(&before,0xa5,sizeof(before));values[0]=before;
        CHECK(!nativeScriptProgram_Index(&index,&diagnostic));
        CHECK(!index.function_count && !index.parameter_count && !index.value_count);
        CHECK(diagnostic.message[0] && diagnostic.source.line==1);
        CHECK(!memcmp(&values[0],&before,sizeof(before)));CHECK(arena.node_count==0 && arena.argument_count==0);
    }
    const char* no_provider="parameter int invalid = LeftHand\n";
    tree=(NativeScriptSourceTree){.source=no_provider,.source_bytes=(uint32_t)strlen(no_provider),.nodes=syntax,.capacity=256};
    arena=(NativeScriptIRArena){.source=no_provider,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));index=default_index(&tree,&arena);index.constant_resolver=NULL;
    CHECK(!nativeScriptProgram_Index(&index,&diagnostic));CHECK(!index.value_count);

    const char* numeric="parameter int literal = -1\nparameter bool lazy = true or (1 / 0 == 1)\n";
    tree=(NativeScriptSourceTree){.source=numeric,.source_bytes=(uint32_t)strlen(numeric),.nodes=syntax,.capacity=256};
    arena=(NativeScriptIRArena){.source=numeric,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));index=default_index(&tree,&arena);index.constant_resolver=NULL;
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));CHECK(index.value_count==2);
    CHECK(values[0].default_value.as.i64==-1 && values[1].default_value.as.u64==1);

    char exhausted[512]="parameter int exhausted = 1";
    for (unsigned i=0;i<70;++i) strcat(exhausted," + 1");strcat(exhausted,"\n");
    tree=(NativeScriptSourceTree){.source=exhausted,.source_bytes=(uint32_t)strlen(exhausted),.nodes=syntax,.capacity=256};
    arena=(NativeScriptIRArena){.source=exhausted,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024};
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));index=default_index(&tree,&arena);
    NativeScriptValueDeclaration before=values[0];
    CHECK(!nativeScriptProgram_Index(&index,&diagnostic));CHECK(!index.value_count && !index.function_count && !index.parameter_count);
    CHECK(strstr(diagnostic.message,"exhausted")!=NULL && diagnostic.source.line==1);
    CHECK(!memcmp(&values[0],&before,sizeof(before)));CHECK(arena.node_count==0 && arena.argument_count==0);
}

static void unsigned_contexts(void)
{
    const char* text=
        "func unsigned_roundtrip(uint flags) -> uint:\n    let uint cached = flags\n    return cached | Transition.skip_attack_count\n"
        "func unsigned_caller() -> uint:\n    return unsigned_roundtrip(Ft_MF_Unk31)\n";
    NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=256};
    NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=1024,
        .arguments=arguments,.argument_capacity=1024};NativeScriptDiagnostic diagnostic;
    CHECK(nativeScriptSource_Parse(&tree,&diagnostic));NativeScriptProgramIndex index=default_index(&tree,&arena);
    CHECK(nativeScriptProgram_Index(&index,&diagnostic));CHECK(index.function_count==2);
    CHECK(declarations[0].result_type==NativeScriptType_U32 && parameters[0].type==NativeScriptType_U32);
    NativeScriptProgramCompiler compiler={.index=&index,.native_bindings=resolve,.locals=locals,.local_capacity=64};
    NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=1024,
        .functions=functions,.function_count=index.function_count};
    for (uint32_t i=0;i<index.function_count;++i) {
        NativeScriptStatementCompiler body;CHECK(nativeScriptProgram_CompileBody(&compiler,i,&body,&diagnostic));
        CHECK(nativeScriptBytecode_Compile(&builder,&body,declarations[i].source_node,i,(uint16_t)declarations[i].parameter_count,&diagnostic));
    }
    CHECK(functions[0].parameter_types[0]==NativeScriptType_U32);
    NativeScriptBytecodeProgram program={text,tree.source_bytes,instructions,builder.instruction_count,functions,index.function_count,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    NativeScriptVM vm={.registers=registers,.register_capacity=128,.frames=frames,.frame_capacity=8};
    NativeScriptVMHost host={.source_alive=alive,.read=read_value};NativeScriptRuntimeContext context={0};NativeScriptValue result;
    CHECK(nativeScriptVM_Run(&vm,&program,&host,&context,1,NULL,0,&result)==NativeScriptVM_Ok);
    CHECK(result.type==NativeScriptType_U32 && result.as.u64==(uint32_t)(Ft_MF_Unk31|Ft_MF_SkipAttackCount));
    CHECK(host_reads==0 && vm.depth==0 && vm.register_top==0);
    const char* unsupported[]={"state uint value\n","attribute uint value 0\n"};
    for (unsigned i=0;i<2;++i) {
        tree=(NativeScriptSourceTree){.source=unsupported[i],.source_bytes=(uint32_t)strlen(unsupported[i]),.nodes=syntax,.capacity=256};
        CHECK(!nativeScriptSource_Parse(&tree,&diagnostic));CHECK(diagnostic.message[0] && diagnostic.source.line==1);
    }
}

int main(void)
{
    catalog();expressions();direct_program();rejected_bodies();parameter_defaults();unsigned_contexts();
    printf("{\"result\":\"pass\",\"checks\":%u,\"constants\":%u,\"functions\":8,\"parameter_defaults\":11,\"host_reads\":%u}\n",
        checks,nativeScriptConstants_Count(),host_reads);return 0;
}
