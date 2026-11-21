#ifndef MELEE_FT_NATIVE_SCRIPT_BYTECODE_H
#define MELEE_FT_NATIVE_SCRIPT_BYTECODE_H

#include "native_script_statement.h"

#define NATIVE_SCRIPT_FUNCTION_SEMANTIC UINT32_C(0x40000000)
#define NATIVE_SCRIPT_SEMANTIC_INDEX UINT32_C(0x3fffffff)

enum { NATIVE_SCRIPT_CALLBACK_INSTRUCTIONS = 65536 };

typedef enum NativeScriptBytecodeOp {
    NativeScriptBC_Constant, NativeScriptBC_Move, NativeScriptBC_Convert,
    NativeScriptBC_Unary, NativeScriptBC_Binary,
    NativeScriptBC_Read, NativeScriptBC_Write, NativeScriptBC_Invoke, NativeScriptBC_ValidateBinding,
    NativeScriptBC_Call, NativeScriptBC_Jump, NativeScriptBC_JumpFalse,
    NativeScriptBC_JumpTrue, NativeScriptBC_Return, NativeScriptBC_Fail,
} NativeScriptBytecodeOp;

// Register ranges for native operations: selectors [a,a+selector_count), direct
// arguments [b,b+argument_count). Calls use b/argument_count only. Semantic IDs
// reference the immutable catalog/program, never native offsets or addresses.
typedef struct NativeScriptInstruction {
    NativeScriptSourceSpan source;
    uint64_t immediate;
    uint32_t semantic, target;
    uint16_t destination, a, b;
    uint8_t selector_count, argument_count;
    uint8_t opcode, type, operation, flags;
} NativeScriptInstruction;

typedef struct NativeScriptFunctionCode {
    NativeScriptSourceSpan source;
    uint32_t first_instruction, instruction_count;
    uint16_t register_count, local_count, parameter_count;
    uint8_t parameter_types[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS];
    uint8_t result_type;
} NativeScriptFunctionCode;

typedef struct NativeScriptBytecodeProgram {
    const char* source;
    uint32_t source_bytes;
    const NativeScriptInstruction* instructions;
    uint32_t instruction_count;
    const NativeScriptFunctionCode* functions;
    uint32_t function_count;
    bool prepared;
} NativeScriptBytecodeProgram;

typedef enum NativeScriptOutputPublication {
    NativeScriptOutputs_None, NativeScriptOutputs_SuccessOnly,
    NativeScriptOutputs_AlwaysAfterGuard,
} NativeScriptOutputPublication;

typedef struct NativeScriptCallLayout {
    uint32_t output_mask, input_output_mask;
    // Prepared intrinsic declaration arguments are compiler literals, not
    // scalar reads of an array/asset object. Their other authored uses retain
    // the normal expression semantics. This storage exists only at compile time.
    uint32_t constant_argument_mask;
    uint64_t constant_arguments[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS];
    uint8_t argument_types[NATIVE_SCRIPT_EXPRESSION_ARGUMENTS];
    uint8_t argument_count;
    uint8_t publication;
} NativeScriptCallLayout;

// Declaration/binding catalog supplies output modes at load. A missing
// description fails compilation; output destinations are captured before
// invocation and published only after the native continuation barrier.
typedef bool (*NativeScriptDescribeCall)(void* context, uint32_t semantic,
    NativeScriptCallLayout*, NativeScriptDiagnostic*);

typedef struct NativeScriptBytecodeBuilder {
    NativeScriptInstruction* instructions;
    uint32_t instruction_count, instruction_capacity;
    NativeScriptFunctionCode* functions;
    uint32_t function_count;
    NativeScriptDescribeCall describe_call;
    void* binding_context;
} NativeScriptBytecodeBuilder;

// Compile an already-resolved statement body. Parameter locals occupy slots
// 0..parameter_count-1. Failure rolls instruction count/function back. Builder
// storage is caller-owned at the content loading transition; no allocations.
bool nativeScriptBytecode_Compile(NativeScriptBytecodeBuilder*,
    const NativeScriptStatementCompiler*, uint32_t body, uint32_t function_id,
    uint16_t parameter_count, NativeScriptDiagnostic*);

// Check immutable instruction/function structure once at content preparation.
// On success the source, instructions, functions and their extents must remain
// unchanged until all executions using this program have retired. Runtime value,
// actor lifetime and VM capacity checks are separate and remain in the VM.
bool nativeScriptBytecode_Prepare(NativeScriptBytecodeProgram*, NativeScriptDiagnostic*);

#endif
