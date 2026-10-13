#include "native_script_vm.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static bool failure(NativeScriptDiagnostic* diagnostic, const char* text)
{
    snprintf(diagnostic->message, sizeof(diagnostic->message), "%s", text);
    return false;
}

static bool integral(unsigned type)
{
    return type >= NativeScriptType_I32 && type <= NativeScriptType_U64;
}

static bool signed_type(unsigned type)
{
    return type == NativeScriptType_I32 || type == NativeScriptType_I64;
}

static unsigned width(unsigned type)
{
    return type == NativeScriptType_I32 || type == NativeScriptType_U32 ? 32 : 64;
}

static uint64_t integer_bits(const NativeScriptValue* value)
{
    return signed_type(value->type) ? (uint64_t)value->as.i64 : value->as.u64;
}

static void integer_value(NativeScriptValue* result, unsigned type, uint64_t bits)
{
    *result = (NativeScriptValue){0};
    result->type = (uint8_t)type;
    if (type == NativeScriptType_Bool) result->as.u64 = bits != 0;
    else if (type == NativeScriptType_I32) {
        uint32_t truncated = (uint32_t)bits;
        result->as.i64 = truncated <= INT32_MAX ? (int64_t)truncated :
            -1 - (int64_t)(UINT32_MAX - truncated);
    } else if (type == NativeScriptType_U32) result->as.u64 = (uint32_t)bits;
    else if (type == NativeScriptType_I64)
        result->as.i64 = bits <= INT64_MAX ? (int64_t)bits :
            -1 - (int64_t)(UINT64_MAX - bits);
    else result->as.u64 = bits;
}

bool nativeScriptValue_Truth(const NativeScriptValue* value, bool* result)
{
    switch (value->type) {
    case NativeScriptType_Bool: case NativeScriptType_U32: case NativeScriptType_U64:
        *result = value->as.u64 != 0; return true;
    case NativeScriptType_I32: case NativeScriptType_I64:
        *result = value->as.i64 != 0; return true;
    case NativeScriptType_F32: *result = value->as.f32 != 0; return true;
    case NativeScriptType_F64: *result = value->as.f64 != 0; return true;
    default: return false;
    }
}

static double floating_value(const NativeScriptValue* value)
{
    switch (value->type) {
    case NativeScriptType_F32: return value->as.f32;
    case NativeScriptType_F64: return value->as.f64;
    case NativeScriptType_I32: case NativeScriptType_I64: return (double)value->as.i64;
    default: return (double)value->as.u64;
    }
}

bool nativeScriptValue_Convert(const NativeScriptValue* value, NativeScriptIRType type,
                              NativeScriptValue* result, NativeScriptDiagnostic* diagnostic)
{
    NativeScriptValue original=*value;
    value=&original;
    bool source_selector=nativeScriptType_SelectorLimit(value->type)!=0;
    bool target_selector=nativeScriptType_SelectorLimit(type)!=0;
    if(source_selector||target_selector) {
        if(value->type!=type || value->as.u64>=nativeScriptType_SelectorLimit(type))
            return failure(diagnostic,"Immutable selector needs an exact bounded type; numeric casts are forbidden");
        *result=*value;return true;
    }
    if (value->type == type) { *result = *value; return true; }
    if (type == NativeScriptType_Bool) {
        bool truth;
        if (!nativeScriptValue_Truth(value, &truth))
            return failure(diagnostic, "Boolean conversion requires a numeric value");
        integer_value(result, type, truth); return true;
    }
    bool numeric = integral(value->type) || value->type == NativeScriptType_Bool ||
        value->type == NativeScriptType_F32 || value->type == NativeScriptType_F64;
    if (!numeric) return failure(diagnostic, "Unsupported scalar conversion");
    if (type == NativeScriptType_F32 || type == NativeScriptType_F64) {
        *result = (NativeScriptValue){0}; result->type = (uint8_t)type;
        if (type == NativeScriptType_F32) {
            // Convert integer directly to Float32, avoiding an extra rounding
            // through Float64 for large UInt64/Int64 operands.
            if (signed_type(value->type)) result->as.f32 = (float)value->as.i64;
            else if (integral(value->type) || value->type == NativeScriptType_Bool)
                result->as.f32 = (float)value->as.u64;
            else result->as.f32 = (float)floating_value(value);
        } else result->as.f64 = floating_value(value);
        return true;
    }
    if (!integral(type)) return failure(diagnostic, "Unsupported destination type");
    if (integral(value->type) || value->type == NativeScriptType_Bool) {
        integer_value(result, type, integer_bits(value)); return true;
    }
    double number = floating_value(value), truncated = trunc(number);
    double limit = ldexp(1.0, (int)width(type) - (signed_type(type) ? 1 : 0));
    double minimum = signed_type(type) ? -limit : 0;
    if (!isfinite(number) || truncated < minimum || truncated >= limit)
        return failure(diagnostic, "Floating-to-integer conversion is outside its declared width");
    integer_value(result, type, signed_type(type) ? (uint64_t)(int64_t)truncated : (uint64_t)truncated);
    return true;
}

bool nativeScriptValue_Unary(NativeScriptIROperation operation,
    const NativeScriptValue* value, NativeScriptValue* result, NativeScriptDiagnostic* diagnostic)
{
    if(nativeScriptType_SelectorLimit(value->type)!=0)
        return failure(diagnostic,"Immutable selectors do not support unary operations");
    if (operation == NativeScriptOp_Not) {
        bool truth;
        if (!nativeScriptValue_Truth(value, &truth)) return failure(diagnostic, "Invalid logical operand");
        integer_value(result, NativeScriptType_Bool, !truth); return true;
    }
    if (operation == NativeScriptOp_Positive) { *result = *value; return true; }
    if (operation == NativeScriptOp_Invert || operation == NativeScriptOp_Negative) {
        if (integral(value->type)) {
            uint64_t bits = integer_bits(value);
            integer_value(result, value->type,
                operation == NativeScriptOp_Invert ? ~bits : UINT64_C(0)-bits);
            return true;
        }
        if (operation == NativeScriptOp_Negative && value->type == NativeScriptType_F32) {
            *result = *value; result->as.f32 = -value->as.f32; return true;
        }
        if (operation == NativeScriptOp_Negative && value->type == NativeScriptType_F64) {
            *result = *value; result->as.f64 = -value->as.f64; return true;
        }
    }
    return failure(diagnostic, "Unsupported unary operation or type");
}

static bool float_binary(NativeScriptIROperation op, const NativeScriptValue* left,
    const NativeScriptValue* right, NativeScriptValue* result, NativeScriptDiagnostic* diagnostic)
{
    bool comparison = op >= NativeScriptOp_Equal && op <= NativeScriptOp_GreaterEqual;
    result->type = comparison ? NativeScriptType_Bool : left->type;
    if (left->type == NativeScriptType_F32) {
        float a=left->as.f32, b=right->as.f32;
        switch (op) {
        case NativeScriptOp_Add: result->as.f32=a+b; return true;
        case NativeScriptOp_Subtract: result->as.f32=a-b; return true;
        case NativeScriptOp_Multiply: result->as.f32=a*b; return true;
        case NativeScriptOp_Divide: result->as.f32=a/b; return true;
        case NativeScriptOp_Equal: result->as.u64=a==b; return true;
        case NativeScriptOp_NotEqual: result->as.u64=a!=b; return true;
        case NativeScriptOp_Less: result->as.u64=a<b; return true;
        case NativeScriptOp_LessEqual: result->as.u64=a<=b; return true;
        case NativeScriptOp_Greater: result->as.u64=a>b; return true;
        case NativeScriptOp_GreaterEqual: result->as.u64=a>=b; return true;
        default: return failure(diagnostic,"Unsupported Float32 operation");
        }
    }
    double a=left->as.f64, b=right->as.f64;
    switch (op) {
    case NativeScriptOp_Add: result->as.f64=a+b; return true;
    case NativeScriptOp_Subtract: result->as.f64=a-b; return true;
    case NativeScriptOp_Multiply: result->as.f64=a*b; return true;
    case NativeScriptOp_Divide: result->as.f64=a/b; return true;
    case NativeScriptOp_Equal: result->as.u64=a==b; return true;
    case NativeScriptOp_NotEqual: result->as.u64=a!=b; return true;
    case NativeScriptOp_Less: result->as.u64=a<b; return true;
    case NativeScriptOp_LessEqual: result->as.u64=a<=b; return true;
    case NativeScriptOp_Greater: result->as.u64=a>b; return true;
    case NativeScriptOp_GreaterEqual: result->as.u64=a>=b; return true;
    default: return failure(diagnostic,"Unsupported Float64 operation");
    }
}

bool nativeScriptValue_Binary(NativeScriptIROperation op, const NativeScriptValue* left,
    const NativeScriptValue* right, NativeScriptValue* result, NativeScriptDiagnostic* diagnostic)
{
    NativeScriptValue original_left=*left,original_right=*right;
    left=&original_left;right=&original_right;
    *result = (NativeScriptValue){0};
    if (op == NativeScriptOp_And || op == NativeScriptOp_Or) {
        bool a,b;
        if (!nativeScriptValue_Truth(left,&a) || !nativeScriptValue_Truth(right,&b))
            return failure(diagnostic,"Invalid logical operands");
        integer_value(result,NativeScriptType_Bool,op==NativeScriptOp_And ? a&&b : a||b);
        return true;
    }
    if ((left->type == NativeScriptType_Object || left->type == NativeScriptType_Null ||
         left->type == NativeScriptType_Callable) &&
        (op == NativeScriptOp_Equal || op == NativeScriptOp_NotEqual)) {
        bool equal;
        if (left->type == NativeScriptType_Null && right->type == NativeScriptType_Null) equal=true;
        else if (left->type == NativeScriptType_Object && right->type == NativeScriptType_Object)
            equal=left->as.object.identity==right->as.object.identity &&
                left->as.object.generation==right->as.object.generation && left->as.object.kind==right->as.object.kind;
        else if (left->type == NativeScriptType_Null && right->type == NativeScriptType_Object) equal=right->as.object.identity==0;
        else if (left->type == NativeScriptType_Object && right->type == NativeScriptType_Null) equal=left->as.object.identity==0;
        else if (left->type == NativeScriptType_Callable && right->type == NativeScriptType_Callable) equal=left->as.u64==right->as.u64;
        else if (left->type == NativeScriptType_Null && right->type == NativeScriptType_Callable) equal=right->as.u64==UINT32_MAX;
        else if (left->type == NativeScriptType_Callable && right->type == NativeScriptType_Null) equal=left->as.u64==UINT32_MAX;
        else return failure(diagnostic,"Invalid identity operands");
        integer_value(result,NativeScriptType_Bool,op==NativeScriptOp_Equal ? equal : !equal); return true;
    }
    if (left->type != right->type) return failure(diagnostic,"Binary operands require the resolved common type");
    if (left->type == NativeScriptType_F32 || left->type == NativeScriptType_F64)
        return float_binary(op,left,right,result,diagnostic);
    if (!integral(left->type)) return failure(diagnostic,"Invalid binary operand type");
    uint64_t a=integer_bits(left), b=integer_bits(right), bits=0;
    bool comparison=op>=NativeScriptOp_Equal && op<=NativeScriptOp_GreaterEqual;
    if (comparison) {
        bool sign=signed_type(left->type);
        switch (op) {
        case NativeScriptOp_Equal: bits=a==b; break;
        case NativeScriptOp_NotEqual: bits=a!=b; break;
        case NativeScriptOp_Less: bits=sign ? left->as.i64<right->as.i64 : a<b; break;
        case NativeScriptOp_LessEqual: bits=sign ? left->as.i64<=right->as.i64 : a<=b; break;
        case NativeScriptOp_Greater: bits=sign ? left->as.i64>right->as.i64 : a>b; break;
        default: bits=sign ? left->as.i64>=right->as.i64 : a>=b; break;
        }
        integer_value(result,NativeScriptType_Bool,bits); return true;
    }
    switch (op) {
    case NativeScriptOp_Add: bits=a+b; break;
    case NativeScriptOp_Subtract: bits=a-b; break;
    case NativeScriptOp_Multiply: bits=a*b; break;
    case NativeScriptOp_BitAnd: bits=a&b; break;
    case NativeScriptOp_BitOr: bits=a|b; break;
    case NativeScriptOp_BitXor: bits=a^b; break;
    case NativeScriptOp_ShiftLeft: case NativeScriptOp_ShiftRight: {
        if (b >= width(left->type)) return failure(diagnostic,"Shift count is outside operand width");
        if (op == NativeScriptOp_ShiftLeft) bits=a<<b;
        else {
            bits=a>>b;
            if (b && signed_type(left->type) && left->as.i64<0) bits|=UINT64_MAX<<(64-b);
        }
        break;
    }
    case NativeScriptOp_Divide: case NativeScriptOp_Remainder:
        if (!b) return failure(diagnostic,"Integer division by zero");
        if (signed_type(left->type)) {
            int64_t dividend=left->as.i64, divisor=right->as.i64;
            int64_t minimum=left->type==NativeScriptType_I32 ? INT32_MIN : INT64_MIN;
            if (dividend==minimum && divisor==-1) bits=op==NativeScriptOp_Divide ? a : 0;
            else bits=(uint64_t)(op==NativeScriptOp_Divide ? dividend/divisor : dividend%divisor);
        } else bits=op==NativeScriptOp_Divide ? a/b : a%b;
        break;
    default: return failure(diagnostic,"Unsupported integral operation");
    }
    integer_value(result,left->type,bits); return true;
}
