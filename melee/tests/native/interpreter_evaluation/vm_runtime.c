#include "vm_runtime.h"
#include <limits.h>
#include <math.h>
#include <string.h>

static bool signed_type(unsigned type)
{
    return type == Eval_I8 || type == Eval_I16 || type == Eval_I32 || type == Eval_I64;
}

static unsigned size_of(unsigned type)
{
    switch (type) {
    case Eval_Bool: case Eval_I8: case Eval_U8: return 1;
    case Eval_I16: case Eval_U16: return 2;
    case Eval_I32: case Eval_U32: case Eval_F32: return 4;
    case Eval_I64: case Eval_U64: case Eval_F64: case Eval_Ptr: return 8;
    default: return 0;
    }
}

static EvalValue integer_value(uint64_t value, unsigned type)
{
    EvalValue out = { .u = value };
    switch (type) {
    case Eval_Bool: out.u = value != 0; break;
    case Eval_I8: out.i = (int8_t)value; break;
    case Eval_U8: out.u = (uint8_t)value; break;
    case Eval_I16: out.i = (int16_t)value; break;
    case Eval_U16: out.u = (uint16_t)value; break;
    case Eval_I32: out.i = (int32_t)value; break;
    case Eval_U32: out.u = (uint32_t)value; break;
    default: break;
    }
    return out;
}

static bool truth(EvalValue value, unsigned type)
{
    if (type == Eval_F32) return value.f != 0;
    if (type == Eval_F64) return value.d != 0;
    return value.u != 0;
}

static double floating(EvalValue value, unsigned type)
{
    if (type == Eval_F32) return value.f;
    if (type == Eval_F64) return value.d;
    if (signed_type(type)) return (double)value.i;
    return (double)value.u;
}

static EvalValue convert(EvalValue value, unsigned source, unsigned dest)
{
    EvalValue result = { 0 };
    if (dest == Eval_Bool) result.u = truth(value, source);
    else if (dest == Eval_F32) result.f = (float)floating(value, source);
    else if (dest == Eval_F64) result.d = floating(value, source);
    else if (source == Eval_F32 || source == Eval_F64) {
        double number = floating(value, source);
        result = integer_value(signed_type(dest) ? (uint64_t)(int64_t)number : (uint64_t)number, dest);
    } else result = integer_value(value.u, dest);
    return result;
}

// Preserve each operation's Clang type, including intermediate Float versus
// Double rounding. Integer arithmetic uses explicit width normalization; no
// signed-overflow optimization can change this experimental VM's behavior.
static EvalValue binary(EvalContext* context, unsigned op, unsigned type,
                        EvalValue left, EvalValue right)
{
    EvalValue out = { 0 };
#define FLOAT_OPERATIONS(member) \
    switch (op) { \
    case Eval_Add: out.member = left.member + right.member; break; \
    case Eval_Sub: out.member = left.member - right.member; break; \
    case Eval_Mul: out.member = left.member * right.member; break; \
    case Eval_Div: out.member = left.member / right.member; break; \
    case Eval_Eq: out.u = left.member == right.member; break; \
    case Eval_Ne: out.u = left.member != right.member; break; \
    case Eval_Lt: out.u = left.member < right.member; break; \
    case Eval_Le: out.u = left.member <= right.member; break; \
    case Eval_Gt: out.u = left.member > right.member; break; \
    case Eval_Ge: out.u = left.member >= right.member; break; \
    default: ++context->errors; break; \
    }
    if (type == Eval_F32) { FLOAT_OPERATIONS(f); return out; }
    if (type == Eval_F64) { FLOAT_OPERATIONS(d); return out; }
#undef FLOAT_OPERATIONS
    uint64_t a = left.u, b = right.u;
    bool sign = signed_type(type);
    switch (op) {
    case Eval_Add: return integer_value(a + b, type);
    case Eval_Sub: return integer_value(a - b, type);
    case Eval_Mul: return integer_value(a * b, type);
    case Eval_Div: case Eval_Mod:
        if (b == 0 || (sign && left.i == INT64_MIN && right.i == -1)) {
            ++context->errors; return out;
        }
        if (sign) out.i = op == Eval_Div ? left.i / right.i : left.i % right.i;
        else out.u = op == Eval_Div ? a / b : a % b;
        return integer_value(out.u, type);
    case Eval_And: return integer_value(a & b, type);
    case Eval_Or: return integer_value(a | b, type);
    case Eval_Xor: return integer_value(a ^ b, type);
    case Eval_Shl: case Eval_Shr:
        if (b >= size_of(type) * 8) { ++context->errors; return out; }
        out.u = op == Eval_Shl ? a << b : sign ? (uint64_t)(left.i >> b) : a >> b;
        return integer_value(out.u, type);
    case Eval_Eq: out.u = a == b; break;
    case Eval_Ne: out.u = a != b; break;
    case Eval_Lt: out.u = sign ? left.i < right.i : a < b; break;
    case Eval_Le: out.u = sign ? left.i <= right.i : a <= b; break;
    case Eval_Gt: out.u = sign ? left.i > right.i : a > b; break;
    case Eval_Ge: out.u = sign ? left.i >= right.i : a >= b; break;
    default: ++context->errors; break;
    }
    return out;
}

EvalValue evalVm_Run(EvalContext* context, const EvalProgram* program,
                     unsigned function, const EvalValue* arguments,
                     unsigned argument_count)
{
    EvalValue result = { 0 };
    if (!context || !program) return result;
    if (context->depth == EVAL_MAX_DEPTH || function >= program->function_count ||
        function >= EVAL_MAX_FUNCTIONS) { ++context->errors; return result; }
    const EvalFunction* fn = &program->functions[function];
    if (fn->registers > EVAL_MAX_REGISTERS || fn->local_bytes > EVAL_LOCAL_BYTES ||
        fn->arguments != argument_count || argument_count > fn->registers ||
        fn->first > program->instruction_count || fn->count > program->instruction_count - fn->first) {
        ++context->errors; return result;
    }
    EvalFrame* frame = &context->frames[context->depth++];
    EvalValue* r = frame->registers;
    if (argument_count) memcpy(r, arguments, argument_count * sizeof(*r));
    ++context->entries; ++context->function_entries[function];
    uint64_t errors = context->errors;
    uint32_t pc = fn->first, end = pc + fn->count;
    while (pc < end) {
        const EvalInstruction* ins = &program->code[pc++];
        if (ins->dst >= fn->registers ||
            (ins->opcode != Eval_Call && ins->opcode != Eval_Constant &&
             ins->opcode != Eval_Jump && ins->opcode != Eval_JumpFalse &&
             ins->opcode != Eval_JumpTrue && ins->a >= fn->registers) ||
            ins->b >= fn->registers) { ++context->errors; break; }
        unsigned type = ins->type, bytes = size_of(type);
        switch (ins->opcode) {
        case Eval_Const: r[ins->dst].u = ins->immediate; break;
        case Eval_Constant:
            if (ins->a >= program->constant_count) { ++context->errors; break; }
            r[ins->dst] = program->constants[ins->a]; break;
        case Eval_Move: r[ins->dst] = r[ins->a]; break;
        case Eval_LocalAddress:
            if (ins->immediate >= fn->local_bytes) { ++context->errors; break; }
            r[ins->dst].p = (uintptr_t)(frame->locals + ins->immediate); break;
        case Eval_Load:
            if (!r[ins->a].p || !bytes) { ++context->errors; break; }
            r[ins->dst].u = 0;
            memcpy(&r[ins->dst], (const void*)r[ins->a].p, bytes);
            if (type != Eval_F32 && type != Eval_F64) r[ins->dst] = integer_value(r[ins->dst].u, type);
            break;
        case Eval_Store:
            if (!r[ins->a].p || !bytes) { ++context->errors; break; }
            memcpy((void*)r[ins->a].p, &r[ins->b], bytes); break;
        case Eval_LoadBits: case Eval_StoreBits: {
            unsigned bit = (unsigned)ins->immediate, width = (unsigned)(ins->immediate >> 32);
            bytes = (bit + width + 7) / 8;
            if (!r[ins->a].p || !width || bit >= 8 || bit + width > 64) { ++context->errors; break; }
            uint64_t value = 0, mask = width == 64 ? UINT64_MAX : (UINT64_C(1) << width) - 1;
            memcpy(&value, (const void*)r[ins->a].p, bytes);
            if (ins->opcode == Eval_StoreBits) {
                value = (value & ~(mask << bit)) | ((r[ins->b].u & mask) << bit);
                memcpy((void*)r[ins->a].p, &value, bytes);
            } else {
                value = (value >> bit) & mask;
                if (signed_type(type) && width < 64 && (value & (UINT64_C(1) << (width - 1)))) value |= ~mask;
                r[ins->dst] = integer_value(value, type);
            }
            break;
        }
        case Eval_Convert: r[ins->dst] = convert(r[ins->a], (unsigned)ins->immediate, type); break;
        case Eval_Add: case Eval_Sub: case Eval_Mul: case Eval_Div: case Eval_Mod:
        case Eval_And: case Eval_Or: case Eval_Xor: case Eval_Shl: case Eval_Shr:
        case Eval_Eq: case Eval_Ne: case Eval_Lt: case Eval_Le: case Eval_Gt: case Eval_Ge:
            r[ins->dst] = binary(context, ins->opcode, type, r[ins->a], r[ins->b]); break;
        case Eval_Neg:
            if (type == Eval_F32) r[ins->dst].f = -r[ins->a].f;
            else if (type == Eval_F64) r[ins->dst].d = -r[ins->a].d;
            else r[ins->dst] = integer_value(0 - r[ins->a].u, type);
            break;
        case Eval_Not: r[ins->dst].u = !truth(r[ins->a], type); break;
        case Eval_Invert: r[ins->dst] = integer_value(~r[ins->a].u, type); break;
        case Eval_Jump:
            if (ins->a >= fn->count) { ++context->errors; break; }
            pc = fn->first + ins->a; break;
        case Eval_JumpFalse: case Eval_JumpTrue:
            if ((ins->opcode == Eval_JumpTrue) == truth(r[ins->b], type)) {
                if (ins->a >= fn->count) { ++context->errors; break; }
                pc = fn->first + ins->a;
            }
            break;
        case Eval_Clear:
            if (!r[ins->a].p) { ++context->errors; break; }
            memset((void*)r[ins->a].p, 0, (size_t)ins->immediate); break;
        case Eval_Copy:
            if (!r[ins->a].p || !r[ins->b].p) { ++context->errors; break; }
            memmove((void*)r[ins->a].p, (const void*)r[ins->b].p, (size_t)ins->immediate); break;
        case Eval_Call: case Eval_CallNative:
            if (ins->c > fn->registers - ins->b) { ++context->errors; break; }
            if (ins->opcode == Eval_Call) {
                ++context->interpreted_calls;
                r[ins->dst] = evalVm_Run(context, program, ins->a, r + ins->b, ins->c);
            } else {
                if (!program->native_call || !r[ins->a].p) { ++context->errors; break; }
                ++context->native_calls;
                r[ins->dst] = program->native_call((unsigned)ins->immediate, r[ins->a].p, r + ins->b);
            }
            break;
        case Eval_Return:
            if (type != Eval_Void) result = r[ins->a];
            --context->depth; return result;
        default: ++context->errors; break;
        }
        if (context->errors != errors) break;
    }
    --context->depth;
    return result;
}
