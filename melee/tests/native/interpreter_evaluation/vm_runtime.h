#ifndef FREAK_EVALUATION_VM_RUNTIME_H
#define FREAK_EVALUATION_VM_RUNTIME_H

// Isolated runtime experiment. No production package or script format changes.
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef union EvalValue {
    uint64_t u;
    int64_t i;
    double d;
    float f;
    uintptr_t p;
} EvalValue;

typedef enum EvalType {
    Eval_Void, Eval_Bool, Eval_I8, Eval_U8, Eval_I16, Eval_U16,
    Eval_I32, Eval_U32, Eval_I64, Eval_U64, Eval_F32, Eval_F64, Eval_Ptr
} EvalType;

typedef enum EvalOpcode {
    Eval_Const, Eval_Constant, Eval_Move, Eval_LocalAddress,
    Eval_Load, Eval_Store, Eval_LoadBits, Eval_StoreBits,
    Eval_Convert, Eval_Add, Eval_Sub, Eval_Mul, Eval_Div, Eval_Mod,
    Eval_And, Eval_Or, Eval_Xor, Eval_Shl, Eval_Shr,
    Eval_Eq, Eval_Ne, Eval_Lt, Eval_Le, Eval_Gt, Eval_Ge,
    Eval_Neg, Eval_Not, Eval_Invert,
    Eval_Jump, Eval_JumpFalse, Eval_JumpTrue,
    Eval_Clear, Eval_Copy, Eval_Call, Eval_CallNative, Eval_Return
} EvalOpcode;

// Operand fields are register indices except for Jump targets and Call function
// indices. Call arguments occupy a contiguous register range b..b+c.
// Convert uses type=destination, immediate=source type. Load/Store addresses
// register a; Store's value is b. Bit access uses byte address a, immediate
// low32=bit offset in that byte/high32=width. CallNative uses a=callee pointer,
// immediate=generated typed signature index. Return uses a or Eval_Void.
typedef struct EvalInstruction {
    uint8_t opcode, type;
    uint16_t dst, a, b, c;
    uint64_t immediate;
} EvalInstruction;

typedef struct EvalFunction {
    uint32_t first, count;
    uint16_t registers, local_bytes, arguments;
    uint8_t result_type;
    const char* name;
} EvalFunction;

typedef EvalValue (*EvalNativeCall)(unsigned signature, uintptr_t callee,
                                  const EvalValue* arguments);
typedef struct EvalProgram {
    const EvalInstruction* code;
    const EvalFunction* functions;
    const EvalValue* constants;
    EvalNativeCall native_call;
    uint32_t instruction_count, constant_count;
    uint16_t function_count;
} EvalProgram;

enum { EVAL_MAX_REGISTERS = 2048, EVAL_LOCAL_BYTES = 8192,
       EVAL_MAX_DEPTH = 32, EVAL_MAX_FUNCTIONS = 1024 };
typedef struct EvalFrame {
    EvalValue registers[EVAL_MAX_REGISTERS];
    _Alignas(16) uint8_t locals[EVAL_LOCAL_BYTES];
} EvalFrame;

typedef struct EvalContext {
    EvalFrame frames[EVAL_MAX_DEPTH];
    uint64_t entries, native_calls, interpreted_calls, errors;
    uint64_t function_entries[EVAL_MAX_FUNCTIONS];
    unsigned depth;
} EvalContext;

EvalValue evalVm_Run(EvalContext*, const EvalProgram*, unsigned function,
                     const EvalValue* arguments, unsigned argument_count);

#endif
