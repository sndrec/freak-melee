#include "native_script_execution.h"
#include "native_callback_lifetime.h"
#include "native_script_binding_receiver.h"
#include "native_script_runtime.h"

#include <stdio.h>
#include <string.h>

typedef struct Invocation {
    NativeScriptExecution* execution;
    NativeScriptEventPayloadHandle payload;
    bool has_payload;
    NativeScriptReceiverScope receivers;
} Invocation;

static bool error(NativeScriptDiagnostic* diagnostic,const char* message)
{
    if (diagnostic) snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    return false;
}

static bool storage_alive(NativeScriptRuntimeContext* context)
{
    Invocation* invocation=context?context->user:NULL;
    NativeScriptExecution* execution=invocation?invocation->execution:NULL;
    NativeScriptStorageInstance* storage=execution?execution->storage:NULL;
    return storage && storage->live && storage->identity==context->actor_id &&
        storage->generation==context->generation;
}

static bool alive(NativeScriptRuntimeContext* context)
{
    return storage_alive(context) && nativeScriptBindings_SourceAlive(context);
}

static bool read(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    Invocation* invocation=context->user;
    uint32_t domain=semantic&UINT32_C(0xf0000000);
    // The VM admits the source at entry, and native/receiver/property/payload
    // boundaries validate it around reentry. Local storage/table access cannot
    // reenter the engine; retain its mutable instance identity without another
    // actor-list walk before the binding's own source check.
    if (!storage_alive(context))return error(diagnostic,"Script instance retired before read");
    if ((semantic&NATIVE_SCRIPT_TABLE_DOMAIN_MASK)==NATIVE_SCRIPT_TABLE_SEMANTIC)
        return nativeScriptProgram_ReadTable(invocation->execution->declarations,
            invocation->execution->owner,semantic,selectors,count,result,diagnostic);
    if((semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)==NATIVE_SCRIPT_RECEIVER_DOMAIN)
        return nativeScriptBindingReceiver_Read(&invocation->receivers,context,semantic,selectors,count,result,diagnostic);
    if (domain==NATIVE_SCRIPT_STATE_SEMANTIC || domain==NATIVE_SCRIPT_PARAMETER_SEMANTIC)
        return nativeScriptStorage_Read(invocation->execution->storage,semantic,selectors,count,result,diagnostic);
    if (domain==NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC) {
        if (!invocation->has_payload || count)
            return error(diagnostic,"Event value has no active invocation or has unsupported selectors");
        NativeScriptVM* vm=invocation->execution->vm;
        if (!vm->depth || vm->frames[vm->depth-1].function>=invocation->execution->function_count ||
            !invocation->execution->functions[vm->frames[vm->depth-1].function].callback)
            return error(diagnostic,"Helper cannot read an implicit event payload");
        return nativeScriptEventPayload_Read(invocation->execution->payloads,invocation->payload,
            context,alive,semantic,result,diagnostic);
    }
    return nativeScriptBindings_Read(context,semantic,selectors,count,result,diagnostic);
}

static bool write(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t count,const NativeScriptValue* value,
    NativeScriptDiagnostic* diagnostic)
{
    Invocation* invocation=context->user;
    uint32_t domain=semantic&UINT32_C(0xf0000000);
    if (!storage_alive(context))return error(diagnostic,"Script instance retired before write");
    if ((semantic&NATIVE_SCRIPT_TABLE_DOMAIN_MASK)==NATIVE_SCRIPT_TABLE_SEMANTIC)
        return error(diagnostic,"Immutable tables cannot be written at runtime");
    if((semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)==NATIVE_SCRIPT_RECEIVER_DOMAIN)
        return nativeScriptBindingReceiver_Write(&invocation->receivers,context,semantic,selectors,count,value,diagnostic);
    if (domain==NATIVE_SCRIPT_STATE_SEMANTIC || domain==NATIVE_SCRIPT_PARAMETER_SEMANTIC)
        return nativeScriptStorage_Write(invocation->execution->storage,semantic,selectors,count,value,diagnostic);
    if (domain==NATIVE_SCRIPT_EVENT_VALUE_SEMANTIC) {
        NativeScriptVM* vm=invocation->execution->vm;
        if(!invocation->has_payload || count || !vm->depth ||
           vm->frames[vm->depth-1].function>=invocation->execution->function_count ||
           !invocation->execution->functions[vm->frames[vm->depth-1].function].callback)
            return error(diagnostic,"Helper cannot write an implicit event output record");
        return nativeScriptEventPayload_Write(invocation->execution->payloads,invocation->payload,
            context,alive,semantic,value,diagnostic);
    }
    return nativeScriptBindings_Write(context,semantic,selectors,count,value,diagnostic);
}

static bool invoke(NativeScriptRuntimeContext* context,uint32_t semantic,
    const NativeScriptValue* selectors,uint32_t selector_count,
    NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result,
    NativeScriptDiagnostic* diagnostic)
{
    Invocation* invocation=context->user;
    if (!storage_alive(context))return error(diagnostic,"Script instance retired before native invocation");
    if((semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)==NATIVE_SCRIPT_RECEIVER_DOMAIN)
        return nativeScriptBindingReceiver_Invoke(&invocation->receivers,context,semantic,selectors,selector_count,arguments,count,result,diagnostic);
    if (semantic&UINT32_C(0xf0000000))return error(diagnostic,"Native invocation uses a non-native semantic identity");
    NativeScriptValue inputs[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS];
    if(count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS || (count&&!arguments))return error(diagnostic,"Native input range is invalid");
    for(unsigned i=0;i<count;++i) {
        inputs[i]=arguments[i];
        if(inputs[i].type==NativeScriptType_Object &&
            !nativeScriptReceiver_Export(&invocation->receivers,context,&arguments[i],&inputs[i],diagnostic))return false;
    }
    NativeScriptValue output={0};
    if(!nativeScriptBindings_Invoke(context,semantic,selectors,selector_count,inputs,count,&output,diagnostic))return false;
    if(!alive(context))return error(diagnostic,"Script instance/source retired during native invocation");
    for(unsigned i=0;i<count;++i)if(inputs[i].type!=NativeScriptType_Object)arguments[i]=inputs[i];
    *result=output;
    return true;
}

static bool validate(NativeScriptRuntimeContext* context,uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,NativeScriptDiagnostic* diagnostic)
{
    Invocation* invocation=context->user;
    if(!storage_alive(context))return error(diagnostic,"Script instance retired before assignment target validation");
    if((semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)!=NATIVE_SCRIPT_RECEIVER_DOMAIN)
        return error(diagnostic,"Unknown captured assignment target domain");
    return nativeScriptBindingReceiver_ValidateTarget(&invocation->receivers,context,semantic,selectors,count,diagnostic);
}
static const NativeScriptVMHost host={alive,read,write,invoke,validate};

static uint32_t owner_source(const NativeScriptProgramIndex* program,uint32_t owner)
{
    if (!owner)return NativeCallbackSource_Fighter;
    if (owner>=program->tree->count)return UINT32_MAX;
    unsigned kind=program->tree->nodes[owner].kind;
    if (kind==NativeScriptSource_Article || kind==NativeScriptSource_Item)return NativeCallbackSource_Item;
    if (kind==NativeScriptSource_Effect)return NativeCallbackSource_ModelEffect;
    return UINT32_MAX;
}

bool nativeScriptExecution_BuildFunctions(const NativeScriptProgramIndex* program,
    const NativeScriptBytecodeProgram* code,NativeScriptExecutionFunction* functions,
    uint32_t capacity,NativeScriptDiagnostic* diagnostic)
{
    if (!program || !program->tree || !program->functions || !code || !code->functions ||
        !functions || capacity<program->function_count || code->function_count!=program->function_count ||
        code->source!=program->tree->source || code->source_bytes!=program->tree->source_bytes)
        return error(diagnostic,"Code and declaration program do not match");
    // Validate the complete candidate before publishing any function records.
    for (uint32_t pass=0;pass<2;++pass)for (uint32_t i=0;i<program->function_count;++i) {
        const NativeScriptFunctionDeclaration* declaration=&program->functions[i];
        const NativeScriptFunctionCode* function=&code->functions[i];
        NativeScriptExecutionFunction record={.source=declaration->name,.owner=declaration->owner,
            .source_class=owner_source(program,declaration->owner),.event_id=UINT32_MAX,.callback=declaration->callback};
        if (record.source_class==UINT32_MAX || !declaration->signature_resolved || !function->instruction_count ||
            function->first_instruction>code->instruction_count ||
            function->instruction_count>code->instruction_count-function->first_instruction ||
            function->result_type!=declaration->result_type || function->parameter_count!=declaration->parameter_count)
            return error(diagnostic,"Function code, ownership or signature is unresolved");
        for (uint32_t p=0;p<declaration->parameter_count;++p)
            if (function->parameter_types[p]!=program->parameters[declaration->parameter_first+p].type)
                return error(diagnostic,"Compiled parameter type differs from its declaration");
        if (declaration->callback) {
            if (declaration->source_node>=program->tree->count)
                return error(diagnostic,"Callback source identity is out of range");
            NativeScriptEventResolution event;
            if (!nativeScriptEvents_Resolve(program->tree,&program->tree->nodes[declaration->source_node],&event,diagnostic))return false;
            if (event.owner_node!=declaration->owner)return error(diagnostic,"Callback owner differs from its declaration");
            record.event_id=event.event_id;
        }
        if (pass)functions[i]=record;
    }
    return true;
}

bool nativeScriptExecution_Bind(NativeScriptExecution* execution,NativeScriptDiagnostic* diagnostic)
{
    if (!execution)return error(diagnostic,"Missing interpreted execution view");
    execution->bound=false;
    NativeScriptStorageInstance* storage=execution->storage;
    if (!execution->declarations || !execution->code || !execution->code->prepared || !execution->vm || !execution->functions ||
        execution->function_count!=execution->declarations->function_count || !storage || !storage->live ||
        !storage->layout || storage->layout->program!=execution->declarations)
        return error(diagnostic,"Execution code, metadata and live storage do not match");
    const NativeScriptStorageOwner* owner=nativeScriptStorage_Owner(storage->layout,storage->owner);
    if (!owner || (owner->parameter_count && !nativeScriptStorage_ParametersReady(storage->layout,
            storage->parameters,storage->owner,diagnostic)))return false;
    execution->owner=storage->owner;execution->identity=storage->identity;execution->generation=storage->generation;
    execution->layout_identity=storage->layout->identity;
    execution->parameter_identity=storage->parameters?storage->parameters->identity:0;
    execution->bound=true;return true;
}

static NativeScriptVMStatus invalid(NativeScriptExecution* execution,uint32_t function,
    uint32_t frame,uint32_t actor,NativeScriptSourceSpan source,const char* message)
{
    if (execution && execution->vm) {
        NativeScriptVM* vm=execution->vm;
        vm->diagnostic=(NativeScriptDiagnostic){.source=source};
        error(&vm->diagnostic,message);
        vm->error_function=function;vm->error_frame=frame;vm->error_actor=actor;
    }
    return NativeScriptVM_Error;
}

NativeScriptVMStatus nativeScriptExecution_Run(NativeScriptExecution* execution,
    void* actor,uint32_t source_class,uint32_t generation,uint32_t frame,uint64_t scene_epoch,
    uint32_t function,const NativeScriptValue* arguments,uint32_t count, NativeScriptEventRecordIO* record,
    NativeScriptValue* result)
{
    if (!execution || !execution->declarations || !execution->code || !execution->vm ||
        !execution->storage || !execution->bound || !result)
        return invalid(execution,function,frame,generation,(NativeScriptSourceSpan){0},"Missing interpreted execution inputs");
    const NativeScriptProgramIndex* program=execution->declarations;
    if (!execution->functions || function>=execution->function_count)
        return invalid(execution,function,frame,generation,(NativeScriptSourceSpan){0},"Code and declaration program do not match");
    const NativeScriptExecutionFunction* declaration=&execution->functions[function];
    NativeScriptSourceSpan source=declaration->source;
    if(record && (!declaration->callback || !record->data ||
                   !record->bytes || record->bytes>NATIVE_SCRIPT_EVENT_RECORD_BYTES))
        return invalid(execution,function,frame,generation,source,"Output record requires a bounded native event callback");
    if (!execution->storage->layout || execution->storage->layout->program!=program ||
        execution->storage->layout->identity!=execution->layout_identity ||
        (execution->storage->parameters?execution->storage->parameters->identity:0)!=execution->parameter_identity ||
        execution->storage->owner!=execution->owner || execution->owner!=declaration->owner || declaration->source_class!=source_class ||
        execution->storage->identity!=execution->identity || execution->storage->generation!=execution->generation ||
        execution->storage->generation!=generation || execution->storage->identity!=generation)
        return invalid(execution,function,frame,generation,source,"Actor class, declaration owner or storage lifetime does not match");
    NativeScriptVM* vm=execution->vm;
    NativeScriptRuntimeContext context={0};
    if (!nativeScriptBindings_Context(actor,source_class,generation,&context,&vm->diagnostic)) {
        vm->error_function=function;vm->error_frame=frame;vm->error_actor=generation;
        return NativeScriptVM_SourceRetired;
    }
    context.frame=frame;context.scene_epoch=scene_epoch;
    Invocation invocation={.execution=execution};context.user=&invocation;
    if (!alive(&context))return invalid(execution,function,frame,generation,source,"Script storage is retired");
    invocation.receivers.pool=nativeScriptRuntime_Receivers();
    if(!nativeScriptReceiver_Push(invocation.receivers.pool,&context,scene_epoch,program,execution->fighter_kind,
        execution->receiver_lookup,execution->receiver_helper,execution->receiver_user,&invocation.receivers.frame,&vm->diagnostic))return NativeScriptVM_Error;
    NativeScriptValue imported[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS];
    if(count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS || (count&&!arguments)) {
        NativeScriptDiagnostic cleanup;nativeScriptReceiver_Pop(invocation.receivers.pool,invocation.receivers.frame,&cleanup);
        return invalid(execution,function,frame,generation,source,"Entry argument range exceeds reserved receiver inputs");
    }
    for(unsigned i=0;i<count;++i) {
        imported[i]=arguments[i];
        if(arguments[i].type==NativeScriptType_Object &&
            !nativeScriptReceiver_Import(&invocation.receivers,&context,&arguments[i],scene_epoch,&imported[i],&vm->diagnostic)) {
            NativeScriptDiagnostic cleanup;nativeScriptReceiver_Pop(invocation.receivers.pool,invocation.receivers.frame,&cleanup);return NativeScriptVM_Error;
        }
    }
    arguments=imported;
    if (declaration->callback) {
        if (!execution->payloads) {
            NativeScriptDiagnostic cleanup;nativeScriptReceiver_Pop(invocation.receivers.pool,invocation.receivers.frame,&cleanup);
            return invalid(execution,function,frame,generation,source,"Event callback needs a reserved payload pool");
        }
        if (!nativeScriptEventPayload_Push(execution->payloads,declaration->event_id,&context,alive,arguments,count,record,
                &invocation.payload,&vm->diagnostic)) {
            NativeScriptDiagnostic cleanup;nativeScriptReceiver_Pop(invocation.receivers.pool,invocation.receivers.frame,&cleanup);
            vm->error_function=function;vm->error_frame=frame;vm->error_actor=generation;return NativeScriptVM_Error;
        }
        invocation.has_payload=true;
        arguments=nativeScriptEventPayload_Arguments(execution->payloads,invocation.payload,&context,alive,
            &count,&vm->diagnostic);
        if (!arguments) {
            NativeScriptDiagnostic cleanup;
            nativeScriptEventPayload_Pop(execution->payloads,invocation.payload,&cleanup);
            nativeScriptReceiver_Pop(invocation.receivers.pool,invocation.receivers.frame,&cleanup);
            return NativeScriptVM_Error;
        }
    }
    NativeScriptValue output;
    NativeScriptVMStatus status=nativeScriptVM_Run(vm,execution->code,&host,&context,function,arguments,count,&output);
    uint8_t record_output[NATIVE_SCRIPT_EVENT_RECORD_BYTES];
    if(status==NativeScriptVM_Ok && record &&
       !nativeScriptEventPayload_ExportRecord(execution->payloads,invocation.payload,&context,alive,
           record_output,record->bytes,&vm->diagnostic)) {
        vm->error_function=function;vm->error_frame=frame;vm->error_actor=generation;
        status=NativeScriptVM_Error;
    }
    if (invocation.has_payload) {
        NativeScriptDiagnostic cleanup={0};
        if (!nativeScriptEventPayload_Pop(execution->payloads,invocation.payload,&cleanup)) {
            if (status==NativeScriptVM_Ok) {
                vm->diagnostic=cleanup;vm->error_function=function;vm->error_frame=frame;vm->error_actor=generation;
                status=NativeScriptVM_Error;
            }
        }
    }
    NativeScriptDiagnostic receiver_cleanup={0};
    if(!nativeScriptReceiver_Pop(invocation.receivers.pool,invocation.receivers.frame,&receiver_cleanup) && status==NativeScriptVM_Ok) {
        vm->diagnostic=receiver_cleanup;status=NativeScriptVM_Error;
    }
    if(status==NativeScriptVM_Ok && !alive(&context))status=NativeScriptVM_SourceRetired;
    if (status==NativeScriptVM_Ok) {
        if(record)memcpy(record->data,record_output,record->bytes);
        *result=output;
    }
    return status;
}
