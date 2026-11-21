#include "native_script_bytecode.h"
#include "native_script_receiver_semantics.h"

#include <stdio.h>
#include <string.h>

typedef struct LowerLoop { uint32_t head, breaks; } LowerLoop;
typedef struct Lowering {
    NativeScriptBytecodeBuilder* output;
    const NativeScriptStatementCompiler* input;
    NativeScriptDiagnostic* diagnostic;
    uint32_t next_register, register_count;
    LowerLoop loops[NATIVE_SCRIPT_EXPRESSION_DEPTH];
    unsigned loop_count, depth;
} Lowering;

typedef struct LValue {
    uint32_t semantic;
    uint16_t local, selectors;
    uint8_t selector_count, type, flags;
} LValue;

static bool prepare_error(NativeScriptDiagnostic* diagnostic,
    NativeScriptSourceSpan source, const char* message)
{
    if(diagnostic) {
        diagnostic->source=source;
        snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    }
    return false;
}

static bool register_range(uint32_t first,uint32_t count,uint32_t available)
{ return first<=available && count<=available-first; }

bool nativeScriptBytecode_Prepare(NativeScriptBytecodeProgram* program,
    NativeScriptDiagnostic* diagnostic)
{
    if(!program)return prepare_error(diagnostic,(NativeScriptSourceSpan){0},"Missing bytecode program");
    program->prepared=false;
    if(!program->instructions || !program->functions || (program->source_bytes && !program->source))
        return prepare_error(diagnostic,(NativeScriptSourceSpan){0},"Missing immutable bytecode storage");
    for(uint32_t f=0;f<program->function_count;++f) {
        const NativeScriptFunctionCode* code=&program->functions[f];
        if(code->instruction_count > NATIVE_SCRIPT_CALLBACK_INSTRUCTIONS)
            return prepare_error(diagnostic,code->source,"Function exceeds the callback instruction budget");
        if(!code->instruction_count || code->first_instruction>program->instruction_count ||
           code->instruction_count>program->instruction_count-code->first_instruction ||
           code->local_count>code->register_count || code->parameter_count>code->local_count ||
           code->parameter_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS)
            return prepare_error(diagnostic,code->source,"Invalid bytecode function extent or signature");
        for(uint32_t i=0;i<code->instruction_count;++i) {
            const NativeScriptInstruction* op=&program->instructions[code->first_instruction+i];
            bool destination=op->opcode!=NativeScriptBC_Write && op->opcode!=NativeScriptBC_Jump &&
                op->opcode!=NativeScriptBC_ValidateBinding && op->opcode!=NativeScriptBC_JumpFalse &&
                op->opcode!=NativeScriptBC_JumpTrue && op->opcode!=NativeScriptBC_Return && op->opcode!=NativeScriptBC_Fail;
            if(destination && op->destination>=code->register_count)
                return prepare_error(diagnostic,op->source,"Bytecode destination register is out of range");
            switch(op->opcode) {
            case NativeScriptBC_Constant: {
                uint32_t limit=nativeScriptType_SelectorLimit(op->type);
                if(limit && op->immediate>=limit)
                    return prepare_error(diagnostic,op->source,"Bytecode selector constant is out of range");
                if(op->type==NativeScriptType_Text) {
                    uint32_t offset=(uint32_t)op->immediate,length=(uint32_t)(op->immediate>>32);
                    if(!register_range(offset,length,program->source_bytes))
                        return prepare_error(diagnostic,op->source,"Bytecode text constant is outside its source");
                }
                break;
            }
            case NativeScriptBC_Move: case NativeScriptBC_Convert: case NativeScriptBC_Unary:
                if(op->a>=code->register_count)
                    return prepare_error(diagnostic,op->source,"Bytecode operand register is out of range");
                break;
            case NativeScriptBC_Binary:
                if(op->a>=code->register_count || op->b>=code->register_count)
                    return prepare_error(diagnostic,op->source,"Bytecode binary register is out of range");
                break;
            case NativeScriptBC_Read: case NativeScriptBC_Write: case NativeScriptBC_Invoke: case NativeScriptBC_ValidateBinding:
                if(!register_range(op->a,op->selector_count,code->register_count))
                    return prepare_error(diagnostic,op->source,"Bytecode selector register range is invalid");
                if(op->opcode==NativeScriptBC_Write && op->b>=code->register_count)
                    return prepare_error(diagnostic,op->source,"Bytecode write register is out of range");
                if(op->opcode==NativeScriptBC_Invoke &&
                   (op->argument_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS ||
                    !register_range(op->b,op->argument_count,code->register_count)))
                    return prepare_error(diagnostic,op->source,"Bytecode native argument range is invalid");
                break;
            case NativeScriptBC_Call:
                if(!register_range(op->b,op->argument_count,code->register_count) ||
                   op->semantic>=program->function_count ||
                   program->functions[op->semantic].result_type!=op->type ||
                   program->functions[op->semantic].parameter_count!=op->argument_count)
                    return prepare_error(diagnostic,op->source,"Bytecode helper call signature or register range is invalid");
                break;
            case NativeScriptBC_JumpFalse: case NativeScriptBC_JumpTrue:
                if(op->a>=code->register_count)
                    return prepare_error(diagnostic,op->source,"Bytecode condition register is out of range");
                // Fall through: all branches must remain inside this function.
            case NativeScriptBC_Jump:
                if(op->target<code->first_instruction ||
                   op->target-code->first_instruction>=code->instruction_count)
                    return prepare_error(diagnostic,op->source,"Bytecode branch target escaped its function");
                break;
            case NativeScriptBC_Return:
                if(op->type!=code->result_type ||
                   (op->type!=NativeScriptType_Void && op->a>=code->register_count))
                    return prepare_error(diagnostic,op->source,"Bytecode return signature or register is invalid");
                break;
            case NativeScriptBC_Fail: break;
            default: return prepare_error(diagnostic,op->source,"Unknown bytecode opcode");
            }
            if(i+1==code->instruction_count && op->opcode!=NativeScriptBC_Return &&
               op->opcode!=NativeScriptBC_Fail && op->opcode!=NativeScriptBC_Jump)
                return prepare_error(diagnostic,op->source,"Bytecode function can fall through its end");
        }
    }
    program->prepared=true;
    return true;
}

static bool fail(Lowering* lower, NativeScriptSourceSpan span, const char* message)
{
    lower->diagnostic->source=span;
    snprintf(lower->diagnostic->message,sizeof(lower->diagnostic->message),"%s",message);
    return false;
}

static bool reserve(Lowering* lower, unsigned count, uint16_t* first,
                    NativeScriptSourceSpan source)
{
    if (count > UINT16_MAX-1-lower->next_register)
        return fail(lower,source,"Bytecode register capacity exceeded");
    *first=(uint16_t)lower->next_register;
    lower->next_register+=count;
    if (lower->register_count<lower->next_register) lower->register_count=lower->next_register;
    return true;
}

static bool emit(Lowering* lower, NativeScriptInstruction instruction, uint32_t* at)
{
    if (lower->output->instruction_count==lower->output->instruction_capacity)
        return fail(lower,instruction.source,"Bytecode instruction capacity exceeded");
    if (at) *at=lower->output->instruction_count;
    lower->output->instructions[lower->output->instruction_count++]=instruction;
    return true;
}

static NativeScriptInstruction instruction(unsigned opcode, NativeScriptSourceSpan source)
{
    NativeScriptInstruction result={0};
    result.opcode=(uint8_t)opcode;result.source=source;
    result.destination=UINT16_MAX;
    return result;
}

static bool expression(Lowering*,uint32_t,uint16_t*);

// Preserve every selector argument in the ancestor chain in authored order.
// Ancestor selector nodes are semantic paths, not an extra native call.
static bool selector_nodes(Lowering* lower,uint32_t id,uint32_t nodes[32],unsigned* count,unsigned depth)
{
    if (id==UINT32_MAX) return true;
    if (id>=lower->input->expressions->node_count || depth>NATIVE_SCRIPT_EXPRESSION_DEPTH)
        return fail(lower,(NativeScriptSourceSpan){0},"Selector chain capacity exceeded");
    const NativeScriptIRNode* node=&lower->input->expressions->nodes[id];
    if(depth && (node->flags&NativeScriptFlag_ReceiverCapture) && node->type==NativeScriptType_Object) {
        if(*count==32)return fail(lower,node->source,"Receiver selector capacity exceeded");
        nodes[(*count)++]=id;return true;
    }
    if (node->kind==NativeScriptIR_Name)return true;
    if (node->kind!=NativeScriptIR_Member && node->kind!=NativeScriptIR_Index && node->kind!=NativeScriptIR_Call)
        return fail(lower,node->source,"Receiver chain must retain semantic selectors");
    if (!selector_nodes(lower,node->a,nodes,count,depth+1)) return false;
    if (node->kind==NativeScriptIR_Index) {
        if (*count==32) return fail(lower,node->source,"Selector argument capacity exceeded");
        nodes[(*count)++]=node->b;
    } else if (node->kind==NativeScriptIR_Call) {
        if (node->argument_count>32-*count)
            return fail(lower,node->source,"Selector argument capacity exceeded");
        for (unsigned i=0;i<node->argument_count;++i)
            nodes[(*count)++]=lower->input->expressions->arguments[node->argument_first+i];
    }
    return true;
}

static bool selectors(Lowering* lower,uint32_t node,uint16_t* first,uint8_t* count)
{
    uint32_t nodes[32];unsigned size=0;
    if (!selector_nodes(lower,node,nodes,&size,0)) return false;
    NativeScriptSourceSpan span=lower->input->expressions->nodes[node].source;
    if (!reserve(lower,size,first,span)) return false;
    *count=(uint8_t)size;
    for (unsigned i=0;i<size;++i) {
        uint16_t value;
        if (!expression(lower,nodes[i],&value)) return false;
        const NativeScriptIRNode* root=&lower->input->expressions->nodes[nodes[i]];
        if((root->flags&NativeScriptFlag_ReceiverCapture) &&
            (root->semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)==NATIVE_SCRIPT_RECEIVER_ROOT) {
            // This root Read was emitted immediately by expression(). Required
            // receiver/schema checks happen before later indices/RHS/arguments.
            NativeScriptInstruction* read=&lower->output->instructions[lower->output->instruction_count-1];
            if((lower->input->expressions->nodes[node].semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)!=NATIVE_SCRIPT_RECEIVER_EXISTS)
                read->semantic|=NATIVE_SCRIPT_RECEIVER_REQUIRE_PRESENT;
            if(lower->input->expressions->nodes[node].flags&NativeScriptFlag_ReceiverOwned)
                read->semantic|=NATIVE_SCRIPT_RECEIVER_REQUIRE_OWNED;
        }
        NativeScriptInstruction copy=instruction(NativeScriptBC_Move,span);
        copy.destination=(uint16_t)(*first+i);copy.a=value;
        if (!emit(lower,copy,NULL)) return false;
    }
    return true;
}

static bool lvalue(Lowering* lower,uint32_t id,LValue* target)
{
    if (id>=lower->input->expressions->node_count)
        return fail(lower,(NativeScriptSourceSpan){0},"Missing typed assignment target");
    const NativeScriptIRNode* node=&lower->input->expressions->nodes[id];
    if (!(node->flags&NativeScriptFlag_Writable)) return fail(lower,node->source,"Target is not writable");
    *target=(LValue){.semantic=node->semantic,.local=UINT16_MAX,.type=node->type,.flags=(uint8_t)node->flags};
    if (node->semantic&NATIVE_SCRIPT_LOCAL_SEMANTIC) {
        uint32_t local=node->semantic&NATIVE_SCRIPT_SEMANTIC_INDEX;
        if (local>=lower->input->local_count) return fail(lower,node->source,"Local semantic ID is out of range");
        target->local=(uint16_t)local;return true;
    }
    if (node->semantic==UINT32_MAX || (node->semantic&NATIVE_SCRIPT_FUNCTION_SEMANTIC))
        return fail(lower,node->source,"Invalid binding assignment target");
    if(!selectors(lower,id,&target->selectors,&target->selector_count))return false;
    if((node->semantic&NATIVE_SCRIPT_RECEIVER_DOMAIN_MASK)==NATIVE_SCRIPT_RECEIVER_DOMAIN) {
        NativeScriptInstruction validate=instruction(NativeScriptBC_ValidateBinding,node->source);
        validate.semantic=node->semantic;validate.a=target->selectors;validate.selector_count=target->selector_count;
        if(!emit(lower,validate,NULL))return false;
    }
    return true;
}

static bool publish(Lowering* lower,const LValue* target,uint16_t value,NativeScriptSourceSpan span)
{
    NativeScriptInstruction write=instruction(target->local!=UINT16_MAX ? NativeScriptBC_Move : NativeScriptBC_Write,span);
    write.type=target->type;write.flags=target->flags;write.b=value;
    if (target->local!=UINT16_MAX) { write.destination=target->local;write.a=value; }
    else { write.semantic=target->semantic;write.a=target->selectors;write.selector_count=target->selector_count; }
    return emit(lower,write,NULL);
}

static bool capture(Lowering* lower,const LValue* target,uint16_t destination,NativeScriptSourceSpan span)
{
    NativeScriptInstruction read=instruction(target->local!=UINT16_MAX ? NativeScriptBC_Move : NativeScriptBC_Read,span);
    read.destination=destination;read.type=target->type;read.flags=target->flags;
    if (target->local!=UINT16_MAX) read.a=target->local;
    else { read.semantic=target->semantic;read.a=target->selectors;read.selector_count=target->selector_count; }
    return emit(lower,read,NULL);
}

static bool call(Lowering* lower,const NativeScriptIRNode* node,uint16_t* result)
{
    bool helper=(node->semantic&NATIVE_SCRIPT_FUNCTION_SEMANTIC)!=0;
    if (node->semantic==UINT32_MAX || (node->semantic&NATIVE_SCRIPT_LOCAL_SEMANTIC))
        return fail(lower,node->source,"Call requires a resolved binding or helper code ID");
    NativeScriptCallLayout layout={0};
    if (!helper) {
        if (!lower->output->describe_call || !lower->output->describe_call(lower->output->binding_context,node->semantic,&layout,lower->diagnostic))
            return fail(lower,node->source,"Native call has no typed output/argument description");
        if (layout.argument_count!=node->argument_count)
            return fail(lower,node->source,"Native call arity differs from its typed layout");
    }
    uint16_t selector_first=0,args_first;
    uint8_t selector_count=0;
    if (!helper && !selectors(lower,node->a,&selector_first,&selector_count)) return false;
    if (node->argument_count>32 || !reserve(lower,node->argument_count,&args_first,node->source)) return false;
    if(node->argument_count<32&&(layout.constant_argument_mask>>node->argument_count))
        return fail(lower,node->source,"Prepared constant mask exceeds native call arity");
    LValue outputs[32];
    for (unsigned i=0;i<node->argument_count;++i) {
        uint32_t id=lower->input->expressions->arguments[node->argument_first+i];
        uint16_t value,destination=(uint16_t)(args_first+i);
        uint32_t bit=UINT32_C(1)<<i;
        if(layout.constant_argument_mask&bit) {
            if((layout.output_mask|layout.input_output_mask)&bit)
                return fail(lower,node->source,"Prepared constant cannot be an output argument");
            uint64_t constant=layout.constant_arguments[i];
            uint8_t type=layout.argument_types[i];
            if((type!=NativeScriptType_I32&&type!=NativeScriptType_U32)||
               (type==NativeScriptType_I32&&((int64_t)constant<INT32_MIN||(int64_t)constant>INT32_MAX))||
               (type==NativeScriptType_U32&&constant>UINT32_MAX))
                return fail(lower,node->source,"Prepared declaration constant has an unsupported type");
            NativeScriptInstruction literal=instruction(NativeScriptBC_Constant,node->source);
            literal.destination=destination;literal.type=type;
            literal.immediate=constant;
            if(!emit(lower,literal,NULL))return false;
        } else if (layout.output_mask&bit) {
            if (!lvalue(lower,id,&outputs[i])) return false;
            if (layout.input_output_mask&bit) {
                if (!capture(lower,&outputs[i],destination,node->source)) return false;
            } else {
                NativeScriptInstruction zero=instruction(NativeScriptBC_Constant,node->source);
                zero.destination=destination;zero.type=outputs[i].type;
                if (!emit(lower,zero,NULL)) return false;
            }
        } else {
            if (!expression(lower,id,&value)) return false;
            NativeScriptInstruction copy=instruction(NativeScriptBC_Move,node->source);
            copy.destination=destination;copy.a=value;
            if (!emit(lower,copy,NULL)) return false;
        }
        if (!helper && layout.argument_types[i]!=NativeScriptType_Invalid) {
            NativeScriptInstruction convert=instruction(NativeScriptBC_Convert,node->source);
            convert.destination=destination;convert.a=destination;convert.type=layout.argument_types[i];
            if (!emit(lower,convert,NULL)) return false;
        }
    }
    if (!reserve(lower,1,result,node->source)) return false;
    NativeScriptInstruction invoke=instruction(helper ? NativeScriptBC_Call : NativeScriptBC_Invoke,node->source);
    invoke.destination=*result;invoke.type=node->type;invoke.flags=(uint8_t)node->flags;
    invoke.semantic=helper ? node->semantic&NATIVE_SCRIPT_SEMANTIC_INDEX : node->semantic;
    invoke.a=selector_first;invoke.selector_count=selector_count;invoke.b=args_first;
    invoke.argument_count=(uint8_t)node->argument_count;invoke.immediate=layout.output_mask;
    if (!emit(lower,invoke,NULL)) return false;
    uint32_t skip=UINT32_MAX;
    if (layout.output_mask && layout.publication==NativeScriptOutputs_SuccessOnly) {
        if (node->type!=NativeScriptType_Bool) return fail(lower,node->source,"Success-only outputs require a Boolean result");
        NativeScriptInstruction branch=instruction(NativeScriptBC_JumpFalse,node->source);branch.a=*result;
        if (!emit(lower,branch,&skip)) return false;
    }
    if (layout.output_mask && layout.publication==NativeScriptOutputs_None)
        return fail(lower,node->source,"Output argument has no publication policy");
    for (unsigned i=0;i<node->argument_count;++i)
        if ((layout.output_mask&(UINT32_C(1)<<i)) &&
            !publish(lower,&outputs[i],(uint16_t)(args_first+i),node->source)) return false;
    if (skip!=UINT32_MAX) lower->output->instructions[skip].target=lower->output->instruction_count;
    return true;
}

static bool expression(Lowering* lower,uint32_t id,uint16_t* result)
{
    if (id>=lower->input->expressions->node_count)
        return fail(lower,(NativeScriptSourceSpan){0},"Missing typed expression");
    if (++lower->depth>NATIVE_SCRIPT_EXPRESSION_DEPTH)
        return fail(lower,lower->input->expressions->nodes[id].source,"Lowering expression depth exceeded");
    const NativeScriptIRNode* node=&lower->input->expressions->nodes[id];
    bool ok=false;
    NativeScriptInstruction code=instruction(NativeScriptBC_Constant,node->source);
    code.type=node->type;code.flags=(uint8_t)node->flags;
    switch (node->kind) {
    case NativeScriptIR_Integer: case NativeScriptIR_Floating: case NativeScriptIR_Boolean:
    case NativeScriptIR_Null: case NativeScriptIR_String: case NativeScriptIR_Selector:
        if (!reserve(lower,1,result,node->source)) break;
        code.destination=*result;code.immediate=node->bits;
        if (node->kind==NativeScriptIR_String) code.immediate=((uint64_t)node->text_length<<32)|node->text_offset;
        ok=emit(lower,code,NULL);break;
    case NativeScriptIR_Name: case NativeScriptIR_Member: case NativeScriptIR_Index:
        if (node->semantic&NATIVE_SCRIPT_LOCAL_SEMANTIC) {
            uint32_t local=node->semantic&NATIVE_SCRIPT_SEMANTIC_INDEX;
            if (local>=lower->input->local_count) { fail(lower,node->source,"Local semantic ID is out of range");break; }
            *result=(uint16_t)local;ok=true;break;
        }
        if (node->type==NativeScriptType_Callable && (node->semantic&NATIVE_SCRIPT_FUNCTION_SEMANTIC)) {
            if (!reserve(lower,1,result,node->source)) break;
            code.destination=*result;code.immediate=node->semantic&NATIVE_SCRIPT_SEMANTIC_INDEX;
            ok=emit(lower,code,NULL);break;
        }
        if (node->type==NativeScriptType_Namespace ||
            (node->type==NativeScriptType_Callable && (node->semantic&NATIVE_SCRIPT_RECEIVER_CLASS_MASK)!=NATIVE_SCRIPT_RECEIVER_HELPER) || node->semantic==UINT32_MAX) {
            fail(lower,node->source,"Namespace/callee is not an executable scalar value");break;
        }
        if (!reserve(lower,1,result,node->source)) break;
        code.opcode=NativeScriptBC_Read;code.destination=*result;code.semantic=node->semantic;
        if (!selectors(lower,id,&code.a,&code.selector_count)) break;
        ok=emit(lower,code,NULL);break;
    case NativeScriptIR_Call: ok=call(lower,node,result);break;
    case NativeScriptIR_Convert: case NativeScriptIR_Unary:
        if (!expression(lower,node->a,&code.a) || !reserve(lower,1,result,node->source)) break;
        code.opcode=node->kind==NativeScriptIR_Convert ? NativeScriptBC_Convert : NativeScriptBC_Unary;
        code.destination=*result;code.operation=node->operation;ok=emit(lower,code,NULL);break;
    case NativeScriptIR_Binary:
        if (node->operation==NativeScriptOp_And || node->operation==NativeScriptOp_Or) {
            uint16_t left,right;uint32_t skip;
            if (!expression(lower,node->a,&left) || !reserve(lower,1,result,node->source)) break;
            code.opcode=NativeScriptBC_Move;code.destination=*result;code.a=left;
            if (!emit(lower,code,NULL)) break;
            NativeScriptInstruction branch=instruction(node->operation==NativeScriptOp_And ? NativeScriptBC_JumpFalse : NativeScriptBC_JumpTrue,node->source);
            branch.a=left;
            if (!emit(lower,branch,&skip) || !expression(lower,node->b,&right)) break;
            code.a=right;
            if (!emit(lower,code,NULL)) break;
            lower->output->instructions[skip].target=lower->output->instruction_count;ok=true;
        } else {
            uint16_t left,right;
            if (!expression(lower,node->a,&left) || !reserve(lower,1,&code.a,node->source)) break;
            // Capture even a local operand before the right expression can
            // synchronously call an authored helper or engine operation.
            NativeScriptInstruction copy=instruction(NativeScriptBC_Move,node->source);copy.destination=code.a;copy.a=left;
            if (!emit(lower,copy,NULL) || !expression(lower,node->b,&right) || !reserve(lower,1,result,node->source)) break;
            code.opcode=NativeScriptBC_Binary;code.destination=*result;code.b=right;code.operation=node->operation;
            ok=emit(lower,code,NULL);
        }
        break;
    default: fail(lower,node->source,"Unsupported typed expression kind");break;
    }
    --lower->depth;
    return ok;
}

static void patch_chain(Lowering* lower,uint32_t first,uint32_t destination)
{
    while (first!=UINT32_MAX) {
        uint32_t next=lower->output->instructions[first].target;
        lower->output->instructions[first].target=destination;first=next;
    }
}

static bool block(Lowering* lower,uint32_t parent)
{
    if (++lower->depth>NATIVE_SCRIPT_EXPRESSION_DEPTH)
        return fail(lower,lower->input->tree->nodes[parent].source,"Lowering statement depth exceeded");
    const NativeScriptSourceTree* tree=lower->input->tree;
    for (uint32_t at=tree->nodes[parent].first_child;at!=UINT32_MAX;at=tree->nodes[at].next) {
        const NativeScriptSourceNode* node=&tree->nodes[at];
        lower->next_register=lower->input->local_count;
        uint16_t value;
        switch (node->kind) {
        case NativeScriptSource_Local: {
            uint32_t local=UINT32_MAX;
            for (uint32_t i=0;i<lower->input->local_count;++i) {
                const NativeScriptLocal* field=&lower->input->locals[i];
                if (field->scope==node->parent && field->name.offset==node->name.offset) { local=i;break; }
            }
            if (local==UINT32_MAX) return fail(lower,node->source,"Local declaration has no semantic slot");
            if (!expression(lower,node->expression,&value)) return false;
            NativeScriptInstruction copy=instruction(NativeScriptBC_Move,node->source);
            copy.destination=(uint16_t)local;copy.a=value;
            if (!emit(lower,copy,NULL)) return false;
            break;
        }
        case NativeScriptSource_Assign: {
            LValue target;
            if (!lvalue(lower,node->target_expression,&target) || !expression(lower,node->expression,&value) ||
                !publish(lower,&target,value,node->source)) return false;
            break;
        }
        case NativeScriptSource_Expression:
            if (!expression(lower,node->expression,&value)) return false;
            break;
        case NativeScriptSource_Return: {
            NativeScriptInstruction result=instruction(NativeScriptBC_Return,node->source);
            result.type=(uint8_t)lower->input->return_type;
            if (result.type!=NativeScriptType_Void && !expression(lower,node->expression,&result.a)) return false;
            if (!emit(lower,result,NULL)) return false;
            break;
        }
        case NativeScriptSource_If: {
            uint32_t arm=at,ends=UINT32_MAX;
            for (;;) {
                const NativeScriptSourceNode* branch_node=&tree->nodes[arm];
                uint32_t next_arm=branch_node->next,skip=UINT32_MAX;
                if (branch_node->kind!=NativeScriptSource_Else) {
                    if (!expression(lower,branch_node->expression,&value)) return false;
                    NativeScriptInstruction branch=instruction(NativeScriptBC_JumpFalse,branch_node->source);branch.a=value;
                    if (!emit(lower,branch,&skip)) return false;
                }
                if (!block(lower,arm)) return false;
                NativeScriptInstruction end=instruction(NativeScriptBC_Jump,branch_node->source);end.target=ends;
                if (!emit(lower,end,&ends)) return false;
                if (skip!=UINT32_MAX) lower->output->instructions[skip].target=lower->output->instruction_count;
                at=arm;
                if (next_arm==UINT32_MAX || (tree->nodes[next_arm].kind!=NativeScriptSource_ElseIf && tree->nodes[next_arm].kind!=NativeScriptSource_Else)) break;
                arm=next_arm;
                lower->next_register=lower->input->local_count;
            }
            patch_chain(lower,ends,lower->output->instruction_count);break;
        }
        case NativeScriptSource_While: {
            if (lower->loop_count==NATIVE_SCRIPT_EXPRESSION_DEPTH) return fail(lower,node->source,"Loop nesting capacity exceeded");
            uint32_t head=lower->output->instruction_count,exit;
            if (!expression(lower,node->expression,&value)) return false;
            NativeScriptInstruction branch=instruction(NativeScriptBC_JumpFalse,node->source);branch.a=value;
            if (!emit(lower,branch,&exit)) return false;
            lower->loops[lower->loop_count++]=(LowerLoop){head,UINT32_MAX};
            if (!block(lower,at)) return false;
            NativeScriptInstruction back=instruction(NativeScriptBC_Jump,node->source);back.target=head;
            if (!emit(lower,back,NULL)) return false;
            lower->output->instructions[exit].target=lower->output->instruction_count;
            patch_chain(lower,lower->loops[--lower->loop_count].breaks,lower->output->instruction_count);break;
        }
        case NativeScriptSource_Break: case NativeScriptSource_Continue: {
            if (!lower->loop_count) return fail(lower,node->source,"Loop control has no enclosing loop");
            LowerLoop* loop=&lower->loops[lower->loop_count-1];
            NativeScriptInstruction branch=instruction(NativeScriptBC_Jump,node->source);
            branch.target=node->kind==NativeScriptSource_Break ? loop->breaks : loop->head;
            if (!emit(lower,branch,node->kind==NativeScriptSource_Break ? &loop->breaks : NULL)) return false;
            break;
        }
        case NativeScriptSource_Pass: break;
        default: return fail(lower,node->source,"Unsupported executable statement kind");
        }
    }
    --lower->depth;return true;
}

static bool block_returns(const NativeScriptSourceTree* tree,uint32_t parent,unsigned depth)
{
    if (depth>NATIVE_SCRIPT_EXPRESSION_DEPTH) return false;
    for (uint32_t at=tree->nodes[parent].first_child;at!=UINT32_MAX;at=tree->nodes[at].next) {
        const NativeScriptSourceNode* node=&tree->nodes[at];
        if (node->kind==NativeScriptSource_Return) return true;
        if (node->kind==NativeScriptSource_Break || node->kind==NativeScriptSource_Continue) return false;
        if (node->kind==NativeScriptSource_If) {
            uint32_t arm=at;bool complete=true,has_else=false;
            for (;;) {
                const NativeScriptSourceNode* branch=&tree->nodes[arm];
                if (!block_returns(tree,arm,depth+1)) complete=false;
                if (branch->kind==NativeScriptSource_Else) has_else=true;
                at=arm;
                uint32_t next=branch->next;
                if (next==UINT32_MAX || (tree->nodes[next].kind!=NativeScriptSource_ElseIf && tree->nodes[next].kind!=NativeScriptSource_Else)) break;
                arm=next;
            }
            if (complete && has_else) return true;
        }
        // A while can execute zero times or exit through break; no infinite-loop
        // or constant-condition termination assumption contributes a return.
    }
    return false;
}

bool nativeScriptBytecode_Compile(NativeScriptBytecodeBuilder* builder,
    const NativeScriptStatementCompiler* input,uint32_t body,uint32_t function_id,
    uint16_t parameters,NativeScriptDiagnostic* diagnostic)
{
    if (!builder || !input || !input->tree || !input->expressions || !diagnostic ||
        !builder->instructions || !builder->functions || function_id>=builder->function_count ||
        body>=input->tree->count || parameters>input->local_count || parameters>32 || input->local_count>=UINT16_MAX) return false;
    uint32_t first=builder->instruction_count;
    Lowering lower={.output=builder,.input=input,.diagnostic=diagnostic,
        .next_register=input->local_count,.register_count=input->local_count};
    memset(diagnostic,0,sizeof(*diagnostic));
    NativeScriptFunctionCode function={.source=input->tree->nodes[body].source,
        .first_instruction=first,.local_count=(uint16_t)input->local_count,
        .parameter_count=parameters,.result_type=(uint8_t)input->return_type};
    for (unsigned i=0;i<parameters;++i) function.parameter_types[i]=(uint8_t)input->locals[i].type;
    const NativeScriptDefaultReturn* fallback=&input->default_return;
    if (fallback->kind!=NativeScriptDefaultReturn_Reject &&
        input->tree->nodes[body].kind!=NativeScriptSource_Callback)
        return fail(&lower,function.source,"Implicit typed return belongs to an audited event callback");
    if (fallback->kind!=NativeScriptDefaultReturn_Reject && fallback->type!=input->return_type)
        return fail(&lower,function.source,"Event default return type differs from its callback signature");
    if (fallback->kind==NativeScriptDefaultReturn_Parameter &&
        (fallback->argument>=parameters || input->locals[fallback->argument].type!=input->return_type))
        return fail(&lower,function.source,"Event default return does not name a matching incoming parameter");
    if (fallback->kind>NativeScriptDefaultReturn_Parameter)
        return fail(&lower,function.source,"Unknown event default return policy");
    if (!block(&lower,body)) { builder->instruction_count=first;return false; }
    if (input->return_type!=NativeScriptType_Void && fallback->kind==NativeScriptDefaultReturn_Reject && !block_returns(input->tree,body,0)) {
        builder->instruction_count=first;
        return fail(&lower,function.source,"Typed function has a path that reaches its end without returning");
    }
    if (input->return_type==NativeScriptType_Void) {
        NativeScriptInstruction result=instruction(NativeScriptBC_Return,function.source);result.type=NativeScriptType_Void;
        if (!emit(&lower,result,NULL)) { builder->instruction_count=first;return false; }
    } else if (fallback->kind!=NativeScriptDefaultReturn_Reject) {
        NativeScriptInstruction result=instruction(NativeScriptBC_Return,function.source);result.type=(uint8_t)input->return_type;
        if (fallback->kind==NativeScriptDefaultReturn_Parameter) result.a=(uint16_t)fallback->argument;
        else {
            if (!nativeScriptExpression_IsNumeric(input->return_type)) {
                builder->instruction_count=first;return fail(&lower,function.source,"Event default constant must have a supported scalar type");
            }
            if (!reserve(&lower,1,&result.a,function.source)) { builder->instruction_count=first;return false; }
            NativeScriptInstruction value=instruction(NativeScriptBC_Constant,function.source);
            value.type=(uint8_t)input->return_type;value.immediate=fallback->bits;value.destination=result.a;
            if (!emit(&lower,value,NULL)) { builder->instruction_count=first;return false; }
        }
        if (!emit(&lower,result,NULL)) { builder->instruction_count=first;return false; }
    } else {
        // Keeps all branch targets inside the function range and diagnoses a
        // malformed continuation. Static return analysis makes it unreachable.
        NativeScriptInstruction unreachable=instruction(NativeScriptBC_Fail,function.source);
        if (!emit(&lower,unreachable,NULL)) { builder->instruction_count=first;return false; }
    }
    function.instruction_count=builder->instruction_count-first;
    function.register_count=(uint16_t)(lower.register_count ? lower.register_count : 1);
    builder->functions[function_id]=function;return true;
}
