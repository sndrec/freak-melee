#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_pose.h>
#include <melee/ft/native_script_constants.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_script_vm.h>
#include <melee/ft/native_callback_lifetime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,calls;static uint64_t graph_bits=7;static NativeScriptValue captured[5];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"pose catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&c->generation==17;}
bool nativeScriptBindingPose_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* result,NativeScriptDiagnostic* d)
{
    (void)c;(void)result;(void)d;CHECK(semantic==NativeScriptBinding_PoseCopyGraph);CHECK(count==2);++calls;
    for(uint32_t i=0;i<count;++i)captured[i]=args[i];return false; // No native assets/runtime here.
}
static bool resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* d)
{
    if(node->kind==NativeScriptIR_Name&&node->text_length==5&&!memcmp(arena->source+node->text_offset,"Graph",5)){
        *result=(NativeScriptIRResolution){.type=NativeScriptType_PoseGraph,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,.flags=NativeScriptFlag_Literal,.literal_bits=graph_bits};return true;
    }
    int constant=nativeScriptConstants_TryResolve(NULL,arena,node,result,d);if(constant)return constant>0;
    return nativeScriptBindings_Resolve(context,arena,node,result,d);
}
static void expression(const char* text,bool accepted,uint8_t type,uint64_t bits)
{
    NativeScriptIRNode nodes[128];uint32_t args[128],result;NativeScriptIRArena arena={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};
    NativeScriptBindingResolverContext context={NativeScriptBindings_Fighter};
    CHECK(nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){.length=arena.source_bytes,.line=1,.column=1},resolve,&context,&result,&diagnostic)==accepted);
    if(accepted){CHECK(nodes[result].type==type);if(type==NativeScriptType_PoseDestination||type==NativeScriptType_PoseGraph)CHECK(nodes[result].kind==NativeScriptIR_Selector&&nodes[result].bits==bits);}
}
static void typed_values(void)
{
    for(unsigned type=NativeScriptType_PoseDestination;type<=NativeScriptType_PoseGraph;++type){
        uint64_t limit=type==NativeScriptType_PoseDestination?2:16;bool truth;
        for(uint64_t bits=0;bits<limit+1;++bits){NativeScriptValue value={.type=type,.as.u64=bits},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX},before=out;
            CHECK(nativeScriptValue_Convert(&value,(NativeScriptIRType)type,&out,&diagnostic)==(bits<limit));
            if(bits==limit)CHECK(!memcmp(&out,&before,sizeof(out)));
            CHECK(!nativeScriptValue_Truth(&value,&truth));
            CHECK(!nativeScriptValue_Convert(&value,NativeScriptType_F64,&out,&diagnostic));
            CHECK(!nativeScriptValue_Unary(NativeScriptOp_Positive,&value,&out,&diagnostic));
            CHECK(!nativeScriptValue_Binary(NativeScriptOp_Add,&value,&value,&out,&diagnostic));
        }
        NativeScriptValue integer={.type=NativeScriptType_U32,.as.u64=1},out;
        CHECK(!nativeScriptValue_Convert(&integer,(NativeScriptIRType)type,&out,&diagnostic));
    }
}
static void vm(void)
{
    const char* text="func Slice():\n    pose.copy_graph(Pose.blend, Graph)\n";
    NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[128];uint32_t args[128];NativeScriptLocal locals[4];
    NativeScriptSourceTree tree={.source=text,.source_bytes=(uint32_t)strlen(text),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=text,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};
    NativeScriptBindingResolverContext context={NativeScriptBindings_Fighter};NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=resolve,.binding_context=&context,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};
    CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic));
    NativeScriptInstruction instructions[128];NativeScriptFunctionCode code[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=128,.functions=code,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&context};
    CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={text,tree.source_bytes,instructions,builder.instruction_count,code,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64];NativeScriptVMFrame frames[4];NativeScriptVM machine={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};
    NativeScriptRuntimeContext runtime={.source_class=NativeCallbackSource_Fighter,.generation=17};NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
    CHECK(nativeScriptVM_Run(&machine,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&result)==NativeScriptVM_Error);
    if(calls!=1)fprintf(stderr,"VM pose call count %u: %s\n",calls,machine.diagnostic.message);
    CHECK(calls==1&&result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
    CHECK(captured[0].type==NativeScriptType_PoseDestination&&captured[0].as.u64==1);
    CHECK(captured[1].type==NativeScriptType_PoseGraph&&captured[1].as.u64==7);
    // Hand-built Constant/Move exercises typed closed values without arithmetic.
    NativeScriptInstruction moves[3]={{.opcode=NativeScriptBC_Constant,.type=NativeScriptType_PoseGraph,.destination=0,.immediate=15},
        {.opcode=NativeScriptBC_Move,.destination=1,.a=0},{.opcode=NativeScriptBC_Return,.type=NativeScriptType_PoseGraph,.a=1}};
    NativeScriptFunctionCode function={.first_instruction=0,.instruction_count=3,.register_count=2,.result_type=NativeScriptType_PoseGraph};program=(NativeScriptBytecodeProgram){"",0,moves,3,&function,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }
    CHECK(nativeScriptVM_Run(&machine,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&result)==NativeScriptVM_Ok);CHECK(result.type==NativeScriptType_PoseGraph&&result.as.u64==15);
    moves[0].immediate=16;result=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT64_MAX};CHECK(!nativeScriptBytecode_Prepare(&program,&diagnostic));CHECK(!program.prepared);CHECK(nativeScriptVM_Run(&machine,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&result)==NativeScriptVM_Error);CHECK(result.type==NativeScriptType_U64);
}
int main(void)
{
    expression("Pose.live",true,NativeScriptType_PoseDestination,0);expression("Pose.blend",true,NativeScriptType_PoseDestination,1);expression("Graph",true,NativeScriptType_PoseGraph,7);
    const char* bad[]={"Pose.live + 1","-Graph","not Pose.live","Pose.live == Pose.blend","int(Graph)","float(Pose.live)","pose.evaluate(0)","pose.copy_graph(Pose.live, 1, 7)","pose.copy_to_live(1.5)","pose.load_animation(Pose.live, 2.5)"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)expression(bad[i],false,0,0);
    graph_bits=16;expression("Graph",false,0,0);graph_bits=UINT64_MAX;expression("Graph",false,0,0);graph_bits=7;
    typed_values();vm();printf("{\"status\":\"PASS closed pose literals/value/VM marshalling\",\"checks\":%u,\"native_leaf_unavailable\":true}\n",checks);return 0;
}
