#include <melee/ft/native_script_bindings.h>
#include <melee/ft/native_script_binding_article_feedback.h>
#include <melee/ft/native_script_expression.h>
#include <melee/ft/native_script_statement.h>
#include <melee/ft/native_script_program.h>
#include <melee/ft/native_callback_lifetime.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,calls;static uint32_t selected,captured_count;static NativeScriptValue captured[4];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"feedback catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&c->generation==17;}
static bool capture(uint32_t semantic,const NativeScriptValue* args,uint32_t count)
{CHECK(count<=4);selected=semantic;captured_count=count;for(unsigned i=0;i<count;++i)captured[i]=args[i];++calls;return false;}
bool nativeScriptBindingFeedback_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)out;(void)d;CHECK(semantic==NativeScriptBinding_EffectsSpawnAtPosition);return capture(semantic,args,count);}
bool nativeScriptBindingArticleFeedback_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)out;(void)d;CHECK(semantic==NativeScriptBinding_ItemSpawnRootEffect||semantic==NativeScriptBinding_ItemDestroyEffects);return capture(semantic,args,count);}
bool nativeScriptBindingSetup_Invoke(NativeScriptRuntimeContext* c,uint32_t semantic,const NativeScriptValue* args,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)out;(void)d;CHECK(semantic==NativeScriptBinding_CallbackMatches);return capture(semantic,args,count);}
static bool resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* d)
{
    if(node->kind==NativeScriptIR_Name && node->text_length==5 && !memcmp(arena->source+node->text_offset,"Pulse",5)){
        *result=(NativeScriptIRResolution){.type=NativeScriptType_Callable,.semantic=NATIVE_SCRIPT_FUNCTION_SEMANTIC|7,.flags=NativeScriptFlag_Readable|NativeScriptFlag_Callable};return true;
    }
    if(node->kind==NativeScriptIR_Name && node->text_length==8 && !memcmp(arena->source+node->text_offset,"Borrowed",8)){
        *result=(NativeScriptIRResolution){.type=NativeScriptType_Callable,.semantic=123,.flags=NativeScriptFlag_Readable|NativeScriptFlag_Callable};return true;
    }
    return nativeScriptBindings_Resolve(context,arena,node,result,d);
}
static NativeScriptIRNode expression(const char* source,uint32_t mask,bool accepted)
{
    NativeScriptIRNode nodes[128];uint32_t args[128],result=0;NativeScriptIRArena arena={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};NativeScriptBindingResolverContext context={mask};diagnostic=(NativeScriptDiagnostic){0};
    CHECK(nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){.length=arena.source_bytes,.line=1,.column=1},resolve,&context,&result,&diagnostic)==accepted);return accepted?nodes[result]:(NativeScriptIRNode){0};
}
static void vm(const char* call,uint32_t mask,uint32_t expected,unsigned argc)
{
    char source[512];snprintf(source,sizeof(source),"func Slice():\n    %s\n",call);NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[128];uint32_t args[128];NativeScriptLocal locals[4];
    NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};NativeScriptBindingResolverContext context={mask};NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=resolve,.binding_context=&context,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic));
    NativeScriptInstruction instructions[128];NativeScriptFunctionCode code[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=128,.functions=code,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&context};CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64];NativeScriptVMFrame frames[4];NativeScriptVM machine={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};NativeScriptRuntimeContext runtime={.source_class=mask==NativeScriptBindings_Article?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
    CHECK(nativeScriptVM_Run(&machine,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&result)==NativeScriptVM_Error);CHECK(calls==1&&selected==expected&&captured_count==argc);CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
int main(void)
{
    const struct{const char* name;uint32_t id,mask;unsigned count;} methods[]={
        {"effects.spawn_at_position(0x8000000000000000,1,-2,0.25)",NativeScriptBinding_EffectsSpawnAtPosition,NativeScriptBindings_Fighter,4},
        {"item.spawn_root_effect(0x8000000000000000)",NativeScriptBinding_ItemSpawnRootEffect,NativeScriptBindings_Article,1},
        {"item.destroy_effects()",NativeScriptBinding_ItemDestroyEffects,NativeScriptBindings_Article,0},
        {"callbacks.matches(0x8000000000000000,Pulse)",NativeScriptBinding_CallbackMatches,NativeScriptBindings_Fighter,2},
        {"callbacks.matches(-1,None)",NativeScriptBinding_CallbackMatches,NativeScriptBindings_Fighter,2},
    };
    for(unsigned i=0;i<sizeof(methods)/sizeof(*methods);++i){NativeScriptIRNode node=expression(methods[i].name,methods[i].mask,true);CHECK(node.semantic==methods[i].id);CHECK(node.type==(i>=3?NativeScriptType_Bool:NativeScriptType_Void));
        CHECK((node.flags&NativeScriptFlag_Pure)==(i>=3?NativeScriptFlag_Pure:0));CHECK((node.flags&NativeScriptFlag_MayInvalidateSource)==(i<3?NativeScriptFlag_MayInvalidateSource:0));
        expression(methods[i].name,methods[i].mask==NativeScriptBindings_Fighter?NativeScriptBindings_Article:NativeScriptBindings_Fighter,false);expression(methods[i].name,NativeScriptBindings_Effect,false);
        vm(methods[i].name,methods[i].mask,methods[i].id,methods[i].count);
        if(i!=2&&i!=4)CHECK(captured[0].type==NativeScriptType_U64&&captured[0].as.u64==UINT64_C(9223372036854775808));
        if(i==0){CHECK(captured[1].type==NativeScriptType_F32&&captured[1].as.f32==1);CHECK(captured[2].type==NativeScriptType_F32&&captured[2].as.f32==-2);CHECK(captured[3].type==NativeScriptType_F32&&captured[3].as.f32==0.25f);}
        if(i==3)CHECK(captured[1].type==NativeScriptType_Callable&&captured[1].as.u64==7);
        if(i==4){CHECK(captured[0].type==NativeScriptType_I32&&captured[0].as.i64==-1);CHECK(captured[1].type==NativeScriptType_Null);}
    }
    const char* invalid[]={"effects.spawn_at_position(2.5,1,2,3)","callbacks.matches(1.5,Pulse)","callbacks.matches(1,7)","callbacks.matches(1,Borrowed)","callbacks.damage_is(Pulse)","callbacks.matches(1,Pulse,0)"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)expression(invalid[i],NativeScriptBindings_Fighter,false);expression("item.spawn_root_effect(3.5)",NativeScriptBindings_Article,false);
    NativeScriptRuntimeContext runtime={.source_class=NativeCallbackSource_Fighter,.generation=17};NativeScriptValue args[2]={{.type=NativeScriptType_F32,.as.f32=1.5f},{.type=NativeScriptType_Null}},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;CHECK(!nativeScriptBindings_Invoke(&runtime,NativeScriptBinding_CallbackMatches,NULL,0,args,2,&out,&diagnostic));CHECK(calls==0&&out.as.u64==UINT64_MAX&&out.type==NativeScriptType_U64);
    printf("{\"status\":\"PASS feedback/query typed catalog/VM routing\",\"checks\":%u,\"native_leaves_unavailable\":true}\n",checks);return 0;
}
