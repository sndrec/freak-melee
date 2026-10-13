#ifndef MELEE_FT_NATIVE_SCRIPT_IR_H
#define MELEE_FT_NATIVE_SCRIPT_IR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Immutable semantic IR built while loading content. No native offsets, machine
// addresses, generated C fragments or runtime parser allocations occur here.
typedef enum NativeScriptIRType {
    NativeScriptType_Invalid, NativeScriptType_Void, NativeScriptType_Bool,
    NativeScriptType_I32, NativeScriptType_U32, NativeScriptType_I64,
    NativeScriptType_U64, NativeScriptType_F32, NativeScriptType_F64,
    NativeScriptType_Text, NativeScriptType_Null, NativeScriptType_Namespace,
    NativeScriptType_Object, NativeScriptType_Callable,
    NativeScriptType_PoseDestination, NativeScriptType_PoseGraph,
    NativeScriptType_HitboxDefinition,
} NativeScriptIRType;

enum { NATIVE_SCRIPT_POSE_DESTINATIONS=2, NATIVE_SCRIPT_POSE_GRAPHS=16,
       NATIVE_SCRIPT_HITBOX_DEFINITIONS=64 };

// Closed immutable selectors are not numeric identities or editable state.
static inline uint32_t nativeScriptType_SelectorLimit(unsigned type)
{
    switch(type) {
    case NativeScriptType_PoseDestination:return NATIVE_SCRIPT_POSE_DESTINATIONS;
    case NativeScriptType_PoseGraph:return NATIVE_SCRIPT_POSE_GRAPHS;
    case NativeScriptType_HitboxDefinition:return NATIVE_SCRIPT_HITBOX_DEFINITIONS;
    default:return 0;
    }
}

typedef struct NativeScriptSourceSpan {
    uint32_t offset, length, line, column;
} NativeScriptSourceSpan;

typedef struct NativeScriptDiagnostic {
    NativeScriptSourceSpan source;
    char message[192];
} NativeScriptDiagnostic;

typedef enum NativeScriptIRKind {
    NativeScriptIR_Integer, NativeScriptIR_Floating, NativeScriptIR_Boolean,
    NativeScriptIR_String, NativeScriptIR_Null, NativeScriptIR_Name,
    NativeScriptIR_Member, NativeScriptIR_Index, NativeScriptIR_Call,
    NativeScriptIR_Convert, NativeScriptIR_Unary, NativeScriptIR_Binary,
    NativeScriptIR_Selector,
} NativeScriptIRKind;

typedef enum NativeScriptIROperation {
    NativeScriptOp_None, NativeScriptOp_Positive, NativeScriptOp_Negative,
    NativeScriptOp_Not, NativeScriptOp_Invert, NativeScriptOp_Add,
    NativeScriptOp_Subtract, NativeScriptOp_Multiply, NativeScriptOp_Divide,
    NativeScriptOp_Remainder, NativeScriptOp_BitAnd, NativeScriptOp_BitOr,
    NativeScriptOp_BitXor, NativeScriptOp_ShiftLeft, NativeScriptOp_ShiftRight,
    NativeScriptOp_Equal, NativeScriptOp_NotEqual, NativeScriptOp_Less,
    NativeScriptOp_LessEqual, NativeScriptOp_Greater, NativeScriptOp_GreaterEqual,
    NativeScriptOp_And, NativeScriptOp_Or,
} NativeScriptIROperation;

enum {
    NativeScriptFlag_Addressable = 1u << 0,
    NativeScriptFlag_Writable = 1u << 1,
    NativeScriptFlag_Callable = 1u << 2,
    NativeScriptFlag_ShortCircuit = 1u << 3,
    NativeScriptFlag_MayInvalidateSource = 1u << 4,
    NativeScriptFlag_Readable = 1u << 5,
    // No engine mutation, RNG consumption, reentry or output-argument writes.
    NativeScriptFlag_Pure = 1u << 6,
    // Resolver-only: publish numeric or closed selector bits as a literal.
    // Parser consumes this flag; no namespace/native read survives in bytecode.
    NativeScriptFlag_Literal = 1u << 7,
    NativeScriptFlag_ReceiverCapture = 1u << 8,
    NativeScriptFlag_ReceiverOwned = 1u << 9,
    // Operation/namespace requires this invocation's own source asset bank.
    NativeScriptFlag_SourceOnly = 1u << 10,
    // A copied invocation record field; never mutable actor or engine state.
    NativeScriptFlag_EventOutput = 1u << 11,
};

#define NATIVE_SCRIPT_NODE_NONE UINT32_MAX
#define NATIVE_SCRIPT_SEMANTIC_NONE UINT32_MAX
enum { NATIVE_SCRIPT_EXPRESSION_DEPTH = 64,
       NATIVE_SCRIPT_EXPRESSION_ARGUMENTS = 32 };

typedef struct NativeScriptIRNode {
    NativeScriptSourceSpan source;
    // Name/Member text and raw string contents reference the immutable source.
    uint32_t text_offset, text_length;
    uint32_t a, b, argument_first, argument_count;
    uint32_t semantic;
    uint64_t bits;
    uint16_t flags;
    uint8_t kind, type, operation;
} NativeScriptIRNode;

typedef struct NativeScriptIRArena {
    const char* source;
    uint32_t source_bytes;
    NativeScriptIRNode* nodes;
    uint32_t node_count, node_capacity;
    uint32_t* arguments;
    uint32_t argument_count, argument_capacity;
} NativeScriptIRArena;

typedef struct NativeScriptIRResolution {
    NativeScriptIRType type;
    uint32_t semantic;
    uint16_t flags;
    uint64_t literal_bits;
} NativeScriptIRResolution;

// Resolve Name, Member, Index and Call using semantic namespaces, declarations
// and binding signatures. Children/arguments already exist in the arena.
// Call validation includes context, argument modes, output destinations and
// selector ownership. A false result must not choose a guessed/default type.
typedef bool (*NativeScriptIRResolver)(void* context,
    const NativeScriptIRArena*, const NativeScriptIRNode*,
    NativeScriptIRResolution*, NativeScriptDiagnostic*);

#endif
