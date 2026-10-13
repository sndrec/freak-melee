#include <melee/ft/native_script_compile.h>
#include <melee/ft/native_script_binding_article_lifecycle.h>
#include <melee/ft/native_script_binding_receiver.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_script_item_actor.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum{Bytes=16384,Nodes=512,Expr=2048,Functions=64,Values=64,Instructions=4096};
typedef struct Buffers{
    char source[Bytes];NativeScriptSourceNode nodes[Nodes];NativeScriptIRNode expressions[Expr];uint32_t arguments[Expr];
    NativeScriptFunctionDeclaration functions[Functions];NativeScriptParameterDeclaration parameters[128];NativeScriptValueDeclaration values[Values];NativeScriptLocal locals[64];
    NativeScriptInstruction instructions[Instructions];NativeScriptFunctionCode code[Functions];NativeScriptExecutionFunction execution[Functions];NativeScriptCompileFunctionScratch graph[Functions];
    NativeScriptStorageField fields[Values];NativeScriptStorageOwner owners[16];NativeScriptParameterSlot slots[Values];
}Buffers;
static Buffers buffers[2];static NativeScriptCompileBank banks[2];static unsigned checks,calls;static uint32_t captured_semantic,captured_count;static NativeScriptValue captured[2];static NativeScriptDiagnostic diagnostic;
#define CHECK(x)do{++checks;if(!(x)){fprintf(stderr,"lifecycle catalog %u: %s (%s)\n",__LINE__,#x,diagnostic.message);exit(1);}}while(0)
bool nativeScriptBindings_SourceAlive(NativeScriptRuntimeContext* c){return c&&c->generation==17;}
// No engine Item actor is scheduled by this frontend fixture.
Fighter_GObj* nativeScriptItem_Creator(Item_GObj* object){(void)object;return NULL;}
Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* object){(void)object;return NULL;}
bool nativeScriptBindingArticleLifecycle_Invoke(NativeScriptRuntimeContext* c,uint32_t id,const NativeScriptValue* a,uint32_t count,NativeScriptValue* out,NativeScriptDiagnostic* d)
{(void)c;(void)out;(void)d;CHECK(id>=NativeScriptBinding_ItemClearCurrentOwner&&id<=NativeScriptBinding_ArticlesBindAuxiliary);CHECK(count<=2);captured_semantic=id;captured_count=count;for(unsigned i=0;i<count;++i)captured[i]=a[i];++calls;return false;}
static void setup(unsigned index)
{
    Buffers* b=&buffers[index];banks[index]=(NativeScriptCompileBank){.source_storage=b->source,.source_capacity=Bytes,.tree={.nodes=b->nodes,.capacity=Nodes},
        .expressions={.nodes=b->expressions,.node_capacity=Expr,.arguments=b->arguments,.argument_capacity=Expr},
        .declarations={.functions=b->functions,.function_capacity=Functions,.parameters=b->parameters,.parameter_capacity=128,.values=b->values,.value_capacity=Values},
        .locals=b->locals,.local_capacity=64,.instructions=b->instructions,.instruction_capacity=Instructions,.function_code=b->code,.function_code_capacity=Functions,
        .execution_functions=b->execution,.execution_function_capacity=Functions,.graph=b->graph,.graph_capacity=Functions,
        .storage_layout={.fields=b->fields,.field_capacity=Values,.owners=b->owners,.owner_capacity=16},.parameter_slots=b->slots,.parameter_slot_capacity=Values};
}
static bool metadata(void* unused,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* d)
{
    (void)unused;const NativeScriptSourceNode* node=&tree->nodes[at];
    if(node->kind==NativeScriptSource_Article)return nativeScriptSource_SpanEquals(tree,node->value,"states 1");
    if(node->kind==NativeScriptSource_ArticleState)return nativeScriptSource_SpanEquals(tree,node->value,"0 anim -1");
    if(node->kind==NativeScriptSource_Function&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article)return true;
    if(node->kind==NativeScriptSource_State&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article)return true;
    if(node->kind==NativeScriptSource_Parameter&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article)return true;
    if(node->kind==NativeScriptSource_Callback&&node->parent<tree->count&&tree->nodes[node->parent].kind==NativeScriptSource_Article){NativeScriptEventResolution event;return nativeScriptEvents_Resolve(tree,node,&event,d);}
    return nativeScriptCompile_MinimalMetadata(NULL,tree,at,d);
}
static uint32_t named(const NativeScriptCompileBank* b,const char* name)
{for(unsigned f=0;f<b->declarations.function_count;++f)if(nativeScriptSource_SpanEquals(&b->tree,b->declarations.functions[f].name,name))return f;CHECK(false);return 0;}
static void call(const char* text,uint32_t mask,uint32_t expected,unsigned argc,bool accepted)
{
    char source[256];snprintf(source,sizeof(source),"func Slice():\n    %s\n",text);NativeScriptSourceNode syntax[32];NativeScriptIRNode nodes[128];uint32_t args[128];NativeScriptLocal locals[4];NativeScriptSourceTree tree={.source=source,.source_bytes=(uint32_t)strlen(source),.nodes=syntax,.capacity=32};CHECK(nativeScriptSource_Parse(&tree,&diagnostic));
    NativeScriptIRArena arena={.source=source,.source_bytes=tree.source_bytes,.nodes=nodes,.node_capacity=128,.arguments=args,.argument_capacity=128};NativeScriptBindingResolverContext binding={mask};NativeScriptStatementCompiler body={.tree=&tree,.expressions=&arena,.bindings=nativeScriptBindings_Resolve,.binding_context=&binding,.locals=locals,.local_capacity=4,.return_type=NativeScriptType_Void};CHECK(nativeScriptStatement_Compile(&body,1,&diagnostic)==accepted);if(!accepted)return;
    NativeScriptInstruction instructions[128];NativeScriptFunctionCode code[1];NativeScriptBytecodeBuilder builder={.instructions=instructions,.instruction_capacity=128,.functions=code,.function_count=1,.describe_call=nativeScriptBindings_DescribeCall,.binding_context=&binding};CHECK(nativeScriptBytecode_Compile(&builder,&body,1,0,0,&diagnostic));
    NativeScriptBytecodeProgram program={source,tree.source_bytes,instructions,builder.instruction_count,code,1,false}; { NativeScriptDiagnostic preparation={0}; if(!nativeScriptBytecode_Prepare(&program,&preparation)){fprintf(stderr,"bytecode preparation: %s\n",preparation.message);abort();} }NativeScriptValue registers[64];NativeScriptVMFrame frames[4];NativeScriptVM vm={.registers=registers,.register_capacity=64,.frames=frames,.frame_capacity=4};NativeScriptRuntimeContext runtime={.source_class=mask==NativeScriptBindings_Article?NativeCallbackSource_Item:NativeCallbackSource_Fighter,.generation=17};NativeScriptValue result={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;
    CHECK(nativeScriptVM_Run(&vm,&program,&nativeScriptBindingHost,&runtime,0,NULL,0,&result)==NativeScriptVM_Error);CHECK(calls==1&&captured_semantic==expected&&captured_count==argc);CHECK(result.type==NativeScriptType_U64&&result.as.u64==UINT64_MAX);
}
static void receiver(void)
{
    const char* source="fighter:\n    behavior independent\nstate int shared\nfunc Helper():\n    pass\narticle Bolt states 1:\n    func Exists() -> bool:\n        return attachment_owner.exists\n    func Motion() -> int:\n        return attachment_owner.motion\n    func Owned() -> int:\n        return attachment_owner.state.shared\n    func Invoke():\n        attachment_owner.helpers.Helper()\n    state 0 anim -1:\n";setup(0);const NativeScriptCompileBank* accepted=NULL;NativeScriptCompileRequest request={.source=source,.source_bytes=(uint32_t)strlen(source),.metadata=metadata};CHECK(nativeScriptCompile_Build(&banks[0],&request,&accepted,&diagnostic));CHECK(accepted==&banks[0]);
    unsigned captured_roots=0;
    for(unsigned i=0;i<accepted->code.instruction_count;++i){const NativeScriptInstruction* op=&accepted->code.instructions[i];if(op->opcode==NativeScriptBC_Read&&(op->semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)==NATIVE_SCRIPT_RECEIVER_ROOT){CHECK((op->semantic&255)==NativeScriptReceiver_AttachmentOwner);++captured_roots;}}
    CHECK(captured_roots==4);CHECK(accepted->code.functions[named(accepted,"Exists")].result_type==NativeScriptType_Bool);
    const char* role="attachment_owner";NativeScriptIRArena arena={.source=role,.source_bytes=(uint32_t)strlen(role)};NativeScriptBindingReceiverResolver resolver={.program=&banks[0].declarations,.source_class=NativeCallbackSource_ModelEffect};NativeScriptIRNode node={.kind=NativeScriptIR_Name,.text_length=arena.source_bytes};NativeScriptIRResolution result;
    CHECK(nativeScriptBindingReceiver_TryResolve(&resolver,&arena,&node,&result,&diagnostic)==-1);
}
int main(void)
{
    const struct{const char* text;uint32_t id;unsigned count;} operations[]={
        {"item.clear_current_owner()",NativeScriptBinding_ItemClearCurrentOwner,0},{"item.clear_attachment_owner()",NativeScriptBinding_ItemClearAttachmentOwner,0},
        {"item.detach_owner()",NativeScriptBinding_ItemDetachOwner,0},{"item.attach_to_owner(0x8000000000000000)",NativeScriptBinding_ItemAttachToOwner,1},
        {"item.attach_to_owner(-1,true)",NativeScriptBinding_ItemAttachToOwnerAux,2},{"articles.bind_auxiliary(0x8000000000000000)",NativeScriptBinding_ArticlesBindAuxiliary,1}};
    for(unsigned i=0;i<6;++i){uint32_t mask=i==5?NativeScriptBindings_Fighter:NativeScriptBindings_Article;const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(operations[i].id);CHECK(d&&d->contexts==mask&&d->argument_count==operations[i].count);CHECK((d->flags&(NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource))==(NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource));call(operations[i].text,mask,operations[i].id,operations[i].count,true);call(operations[i].text,mask==NativeScriptBindings_Article?NativeScriptBindings_Fighter:NativeScriptBindings_Article,0,0,false);call(operations[i].text,NativeScriptBindings_Effect,0,0,false);
        if(i==3||i==5)CHECK(captured[0].type==NativeScriptType_U64&&captured[0].as.u64==UINT64_C(9223372036854775808));if(i==4){CHECK(captured[0].type==NativeScriptType_I32&&captured[0].as.i64==-1);CHECK(captured[1].type==NativeScriptType_Bool&&captured[1].as.u64==1);}}
    call("item.attach_to_owner(1.5)",NativeScriptBindings_Article,0,0,false);call("item.attach_to_owner(1,1)",NativeScriptBindings_Article,0,0,false);call("item.attach_to_owner(1,true,false)",NativeScriptBindings_Article,0,0,false);call("item.attach_to_owner_aux(1,true)",NativeScriptBindings_Article,0,0,false);call("articles.bind_auxiliary(1.5)",NativeScriptBindings_Fighter,0,0,false);call("item.clear_current_owner(0)",NativeScriptBindings_Article,0,0,false);
    NativeScriptRuntimeContext runtime={.source_class=NativeCallbackSource_Item,.generation=17};NativeScriptValue input={.type=NativeScriptType_F32,.as.f32=1.5f},out={.type=NativeScriptType_U64,.as.u64=UINT64_MAX};calls=0;CHECK(!nativeScriptBindings_Invoke(&runtime,NativeScriptBinding_ItemAttachToOwner,NULL,0,&input,1,&out,&diagnostic));CHECK(calls==0&&out.as.u64==UINT64_MAX);receiver();
    printf("{\"status\":\"PASS Article lifecycle catalog/VM and attachment receiver typing\",\"checks\":%u,\"native_lifecycle_unavailable\":true,\"whole_assets_accepted\":false}\n",checks);return 0;
}
