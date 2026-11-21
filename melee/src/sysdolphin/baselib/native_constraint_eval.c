#include "bytecode.h"
#include "native_constraint.h"
#include "random.h"
#include "util.h"

#include <math.h>
#include <stdint.h>

typedef union ConstraintValue {
    u32 bits;
    s32 integer;
    f32 number;
} ConstraintValue;

static s32 float_to_integer(f32 value)
{
    if (isnan(value) || value <= -0x1p31f) return INT32_MIN;
    if (value >= 0x1p31f) return INT32_MAX;
    return (s32)value;
}

float HSD_ByteCodeEval(u8* pc, const f32* args, s32 nb_args)
{
    (void)nb_args; // The DAT loader checks argument indices and every stack depth.
    if (!pc) return 0.0f;
    ConstraintValue stack[HSD_CONSTRAINT_STACK];
    unsigned depth = 0;
    for (;;) {
        unsigned op = *pc++;
        unsigned operand;
        switch (op) {
        case Cns_Nop: continue;
        case Cns_Return:
            return isfinite(stack[depth - 1].number) ? stack[depth - 1].number : 0.0f;
        case Cns_Argument: case Cns_Jump: case Cns_JumpIf:
            operand = (unsigned)pc[0] << 8 | pc[1];
            pc += 2;
            if (op == Cns_Argument) stack[depth++].number = args[operand];
            else if (op == Cns_Jump || stack[--depth].bits) pc += operand;
            continue;
        case Cns_Drop: depth -= *pc++; continue;
        case Cns_Copy:
            operand = *pc++;
            stack[depth] = stack[depth - 1 - operand];
            ++depth;
            continue;
        case Cns_Literal:
            stack[depth++].bits = (u32)pc[0] << 24 | (u32)pc[1] << 16 | (u32)pc[2] << 8 | pc[3];
            pc += 4;
            continue;
        }

        ConstraintValue* top = &stack[depth - 1];
        switch (op) {
        case Cns_ToInt: top->integer = float_to_integer(top->number); continue;
        case Cns_ToFloat: top->number = (f32)top->integer; continue;
        case Cns_NegFloat: top->number = -top->number; continue;
        case Cns_NegInt: top->bits = 0u - top->bits; continue;
        case Cns_RandomBit: top->integer = HSD_Randi(2); continue;
        case Cns_RandomFloat: top->number = HSD_Randf(); continue;
        case Cns_Sin: top->number = sinf((f32)(DEG_TO_RAD * top->number)); continue;
        case Cns_Cos: top->number = cosf((f32)(DEG_TO_RAD * top->number)); continue;
        case Cns_Tan: top->number = tanf((f32)(DEG_TO_RAD * top->number)); continue;
        case Cns_Asin: top->number = (f32)(RAD_TO_DEG * asinf(top->number)); continue;
        case Cns_Acos: top->number = (f32)(RAD_TO_DEG * acosf(top->number)); continue;
        case Cns_Atan: top->number = (f32)(RAD_TO_DEG * atanf(top->number)); continue;
        case Cns_Log: top->number = logf(top->number); continue;
        case Cns_Exp: top->number = expf(top->number); continue;
        case Cns_AbsFloat:
            if (top->number < 0) top->number = -top->number;
            continue;
        case Cns_AbsInt:
            if (top->integer < 0) top->bits = 0u - top->bits;
            continue;
        case Cns_Sqrt: top->number = sqrtf(top->number); continue;
        case Cns_Not: top->integer = !top->bits; continue;
        }

        ConstraintValue right = *top;
        --depth;
        ConstraintValue* left = &stack[depth - 1];
        switch (op) {
        case Cns_AddFloat: left->number += right.number; break;
        case Cns_SubFloat: left->number -= right.number; break;
        case Cns_MulFloat: left->number *= right.number; break;
        case Cns_DivFloat: left->number /= right.number; break;
        case Cns_ModFloat: left->number = fmodf(left->number, right.number); break;
        case Cns_AddInt: left->bits += right.bits; break;
        case Cns_SubInt: left->bits -= right.bits; break;
        case Cns_MulInt: left->bits *= right.bits; break;
        case Cns_DivInt:
            if (!right.integer) left->integer = 0;
            else if (right.integer != -1 || left->integer != INT32_MIN) left->integer /= right.integer;
            break;
        case Cns_ModInt:
            if (!right.integer || right.integer == -1) left->integer = 0;
            else left->integer %= right.integer;
            break;
        case Cns_Pow: left->number = left->number == 0 ? 0 : powf(left->number, right.number); break;
        case Cns_MinFloat: if (left->number > right.number) *left = right; break;
        case Cns_MaxFloat: if (left->number < right.number) *left = right; break;
        case Cns_MinInt: if (left->integer > right.integer) *left = right; break;
        case Cns_MaxInt: if (left->integer < right.integer) *left = right; break;
        case Cns_Atan2:
            left->number = right.number == 0 ? (left->number >= 0 ? 90.0f : -90.0f)
                : (f32)(RAD_TO_DEG * atan2f(left->number, right.number));
            break;
        case Cns_RandomInt: {
            // The full signed range is 2^32 values. Scale one 16-bit RNG draw.
            u32 sample = (u32)HSD_Rand();
            if (left->integer <= right.integer) {
                uint64_t count = (uint64_t)(right.bits - left->bits) + 1;
                left->bits += (u32)((count * sample) >> 16);
            }
            break;
        }
        case Cns_LtInt: left->integer = left->integer < right.integer; break;
        case Cns_GtInt: left->integer = left->integer > right.integer; break;
        case Cns_LeInt: left->integer = left->integer <= right.integer; break;
        case Cns_GeInt: left->integer = left->integer >= right.integer; break;
        case Cns_EqInt: left->integer = left->integer == right.integer; break;
        case Cns_NeInt: left->integer = left->integer != right.integer; break;
        case Cns_And: left->integer = left->bits && right.bits; break;
        case Cns_Or: left->integer = left->bits || right.bits; break;
        case Cns_Xor: left->integer = !!left->bits != !!right.bits; break;
        case Cns_LtFloat: left->integer = left->number < right.number; break;
        case Cns_GtFloat: left->integer = left->number > right.number; break;
        case Cns_LeFloat: left->integer = left->number <= right.number; break;
        case Cns_GeFloat: left->integer = left->number >= right.number; break;
        case Cns_EqFloat: left->integer = left->number == right.number; break;
        case Cns_NeFloat: left->integer = left->number != right.number; break;
        case Cns_BitAnd: left->bits &= right.bits; break;
        case Cns_BitOr: left->bits |= right.bits; break;
        case Cns_BitXor: left->bits ^= right.bits; break;
        }
    }
}
