#include "native_script_vm.h"

#include <stdio.h>
#include <string.h>

static NativeScriptVMStatus fail(NativeScriptVM* vm,NativeScriptRuntimeContext* context,
    uint32_t function,NativeScriptSourceSpan source,const char* message)
{
    vm->diagnostic.source=source;
    snprintf(vm->diagnostic.message,sizeof(vm->diagnostic.message),"%s",message);
    vm->error_function=function;vm->error_frame=context->frame;vm->error_actor=context->actor_id;
    return NativeScriptVM_Error;
}

static NativeScriptVMStatus retired(NativeScriptVM* vm,NativeScriptRuntimeContext* context,
    uint32_t function,NativeScriptSourceSpan source)
{
    fail(vm,context,function,source,"Script callback source retired or changed generation");
    return NativeScriptVM_SourceRetired;
}

static NativeScriptVMStatus push(NativeScriptVM* vm,const NativeScriptBytecodeProgram* program,
    NativeScriptRuntimeContext* context,uint32_t function,const NativeScriptValue* arguments,
    uint32_t argument_count,uint16_t destination)
{
    if (function>=program->function_count)
        return fail(vm,context,function,(NativeScriptSourceSpan){0},"Helper function ID is out of range");
    const NativeScriptFunctionCode* code=&program->functions[function];
    if (argument_count!=code->parameter_count || (argument_count && !arguments))
        return fail(vm,context,function,code->source,"Entry argument count does not match function signature");
    if (vm->depth==vm->frame_capacity || vm->register_top>vm->register_capacity ||
        code->register_count>vm->register_capacity-vm->register_top)
        return fail(vm,context,function,code->source,"VM frame or register pool exhausted");
    // Reserve the whole straight-line body once. Only a call or a taken
    // backward branch can execute more instructions than this reservation.
    if (code->instruction_count > vm->remaining_instructions) {
        vm->remaining_instructions = 0;
        return fail(vm,context,function,code->source,"Callback instruction budget exhausted");
    }
    vm->remaining_instructions -= code->instruction_count;
    NativeScriptValue parameters[32];
    for (unsigned i=0;i<argument_count;++i)
        if (!nativeScriptValue_Convert(&arguments[i],(NativeScriptIRType)code->parameter_types[i],
                &parameters[i],&vm->diagnostic))
            return fail(vm,context,function,code->source,"Helper argument conversion failed");
    NativeScriptValue* values=vm->registers+vm->register_top;
    memset(values,0,code->register_count*sizeof(*values));
    if (argument_count) memcpy(values,parameters,argument_count*sizeof(*values));
    vm->frames[vm->depth++]=(NativeScriptVMFrame){function,code->first_instruction,vm->register_top,destination};
    vm->register_top+=code->register_count;
    return NativeScriptVM_Ok;
}

static void constant(const NativeScriptInstruction* instruction,NativeScriptValue* value)
{
    *value=(NativeScriptValue){0};value->type=instruction->type;
    switch (instruction->type) {
    case NativeScriptType_F32: {
        uint32_t bits=(uint32_t)instruction->immediate;memcpy(&value->as.f32,&bits,sizeof(bits));break;
    }
    case NativeScriptType_F64: memcpy(&value->as.f64,&instruction->immediate,sizeof(double));break;
    case NativeScriptType_I32: {
        uint32_t bits=(uint32_t)instruction->immediate;
        value->as.i64=bits<=INT32_MAX ? (int64_t)bits : -1-(int64_t)(UINT32_MAX-bits);break;
    }
    case NativeScriptType_I64:
        value->as.i64=instruction->immediate<=INT64_MAX ? (int64_t)instruction->immediate :
            -1-(int64_t)(UINT64_MAX-instruction->immediate);break;
    case NativeScriptType_Text:
        value->as.text.offset=(uint32_t)instruction->immediate;
        value->as.text.length=(uint32_t)(instruction->immediate>>32);break;
    case NativeScriptType_Bool: value->as.u64=instruction->immediate!=0;break;
    case NativeScriptType_U32: value->as.u64=(uint32_t)instruction->immediate;break;
    case NativeScriptType_PoseDestination:case NativeScriptType_PoseGraph:case NativeScriptType_HitboxDefinition:
        value->as.u64=instruction->immediate;break;
    default: value->as.u64=instruction->immediate;break;
    }
}

NativeScriptVMStatus nativeScriptVM_Run(NativeScriptVM* vm,const NativeScriptBytecodeProgram* program,
    const NativeScriptVMHost* host,NativeScriptRuntimeContext* context,uint32_t function,
    const NativeScriptValue* arguments,uint32_t argument_count,NativeScriptValue* result)
{
    if (!vm || !program || !host || !context || !result || !vm->registers || !vm->frames ||
        !program->instructions || !program->functions || !host->source_alive) return NativeScriptVM_Error;
    uint32_t original_depth=vm->depth,original_top=vm->register_top;
    memset(&vm->diagnostic,0,sizeof(vm->diagnostic));
    if(!program->prepared)return fail(vm,context,function,(NativeScriptSourceSpan){0},"Bytecode program has not been prepared");
    NativeScriptVMStatus status;
    if (!host->source_alive(context)) return retired(vm,context,function,(NativeScriptSourceSpan){0});
    if (!original_depth) vm->remaining_instructions = NATIVE_SCRIPT_CALLBACK_INSTRUCTIONS;
    status=push(vm,program,context,function,arguments,argument_count,UINT16_MAX);
    if (status!=NativeScriptVM_Ok) return status;
    while (vm->depth>original_depth) {
        NativeScriptVMFrame* frame=&vm->frames[vm->depth-1];
        const NativeScriptFunctionCode* code=&program->functions[frame->function];
        const NativeScriptInstruction* instruction=&program->instructions[frame->instruction++];
        NativeScriptValue* values=vm->registers+frame->register_base;
        NativeScriptValue value={0};bool ok=true;
        bool destination=instruction->opcode!=NativeScriptBC_Write && instruction->opcode!=NativeScriptBC_Jump &&
            instruction->opcode!=NativeScriptBC_ValidateBinding &&
            instruction->opcode!=NativeScriptBC_JumpFalse && instruction->opcode!=NativeScriptBC_JumpTrue &&
            instruction->opcode!=NativeScriptBC_Return && instruction->opcode!=NativeScriptBC_Fail;
        switch (instruction->opcode) {
        case NativeScriptBC_Constant:
            constant(instruction,&value);
            break;
        case NativeScriptBC_Move:
            value=values[instruction->a];
            if(nativeScriptType_SelectorLimit(value.type)&&value.as.u64>=nativeScriptType_SelectorLimit(value.type))ok=false;
            break;
        case NativeScriptBC_Convert:
            ok=nativeScriptValue_Convert(&values[instruction->a],(NativeScriptIRType)instruction->type,&value,&vm->diagnostic);break;
        case NativeScriptBC_Unary:
            ok=nativeScriptValue_Unary((NativeScriptIROperation)instruction->operation,&values[instruction->a],&value,&vm->diagnostic);break;
        case NativeScriptBC_Binary:
            ok=nativeScriptValue_Binary((NativeScriptIROperation)instruction->operation,
                &values[instruction->a],&values[instruction->b],&value,&vm->diagnostic);break;
        case NativeScriptBC_Read: case NativeScriptBC_Write: case NativeScriptBC_Invoke: case NativeScriptBC_ValidateBinding: {
            const NativeScriptValue* selectors=values+instruction->a;
            if(instruction->opcode==NativeScriptBC_ValidateBinding) {
                ok=host->validate && host->validate(context,instruction->semantic,selectors,instruction->selector_count,&vm->diagnostic);
            } else if (instruction->opcode==NativeScriptBC_Read) {
                ok=host->read && host->read(context,instruction->semantic,selectors,
                    instruction->selector_count,&value,&vm->diagnostic);
            } else if (instruction->opcode==NativeScriptBC_Write) {
                ok=host->write && host->write(context,instruction->semantic,selectors,
                    instruction->selector_count,&values[instruction->b],&vm->diagnostic);
            } else {
                uint8_t types[32];
                for (unsigned i=0;i<instruction->argument_count;++i) types[i]=values[instruction->b+i].type;
                ok=host->invoke && host->invoke(context,instruction->semantic,selectors,instruction->selector_count,
                    values+instruction->b,instruction->argument_count,&value,&vm->diagnostic);
                if (ok)
                    for (unsigned i=0;i<instruction->argument_count;++i)
                        if ((instruction->immediate&(UINT64_C(1)<<i)) && values[instruction->b+i].type!=types[i]) {
                            snprintf(vm->diagnostic.message,sizeof(vm->diagnostic.message),"Native output type differs from its declared destination");
                            ok=false;break;
                        }
            }
            // Native reentry can retire or recycle the actor. Check before
            // publishing result/output registers or any caller continuation.
            if ((instruction->flags&NativeScriptFlag_MayInvalidateSource) && !host->source_alive(context)) {
                status=retired(vm,context,frame->function,instruction->source);goto finished;
            }
            if (ok && instruction->opcode!=NativeScriptBC_Write && instruction->opcode!=NativeScriptBC_ValidateBinding && value.type!=instruction->type) {
                snprintf(vm->diagnostic.message,sizeof(vm->diagnostic.message),"Native result type differs from its declared signature");ok=false;
            }
            break;
        }
        case NativeScriptBC_Call:
            status=push(vm,program,context,instruction->semantic,values+instruction->b,instruction->argument_count,instruction->destination);
            if (status!=NativeScriptVM_Ok) goto finished;
            continue;
        case NativeScriptBC_Jump: case NativeScriptBC_JumpFalse: case NativeScriptBC_JumpTrue: {
            bool jump=true;
            if (instruction->opcode!=NativeScriptBC_Jump) {
                bool truth;
                if (!nativeScriptValue_Truth(&values[instruction->a],&truth)) { ok=false;break; }
                jump=instruction->opcode==NativeScriptBC_JumpTrue ? truth : !truth;
            }
            if (jump) {
                if (instruction->target < frame->instruction) {
                    uint32_t repeated = frame->instruction - instruction->target;
                    if (repeated > vm->remaining_instructions) {
                        vm->remaining_instructions = 0;
                        status = fail(vm,context,frame->function,instruction->source,
                                      "Callback instruction budget exhausted");
                        goto finished;
                    }
                    vm->remaining_instructions -= repeated;
                }
                frame->instruction=instruction->target;
            }
            break;
        }
        case NativeScriptBC_Return: {
            value.type=NativeScriptType_Void;
            if (instruction->type!=NativeScriptType_Void) value=values[instruction->a];
            if (value.type!=code->result_type) { ok=false;break; }
            uint16_t return_destination=frame->return_destination;
            vm->register_top=frame->register_base;--vm->depth;
            if (vm->depth==original_depth) { *result=value;status=NativeScriptVM_Ok;goto finished; }
            NativeScriptVMFrame* caller=&vm->frames[vm->depth-1];
            vm->registers[caller->register_base+return_destination]=value;
            continue;
        }
        case NativeScriptBC_Fail:
            snprintf(vm->diagnostic.message,sizeof(vm->diagnostic.message),"Non-void function reached its end without returning");ok=false;break;
        default: ok=false;break;
        }
        if (!ok) {
            char message[sizeof(vm->diagnostic.message)];
            snprintf(message,sizeof(message),"%s",vm->diagnostic.message[0] ? vm->diagnostic.message : "Invalid bytecode operand or unsupported operation");
            status=fail(vm,context,frame->function,instruction->source,message);break;
        }
        if (destination) values[instruction->destination]=value;
    }
finished:
    vm->depth=original_depth;vm->register_top=original_top;
    return status;
}
