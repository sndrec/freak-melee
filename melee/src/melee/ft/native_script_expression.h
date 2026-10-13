#ifndef MELEE_FT_NATIVE_SCRIPT_EXPRESSION_H
#define MELEE_FT_NATIVE_SCRIPT_EXPRESSION_H

#include "native_script_ir.h"

typedef enum NativeScriptTokenKind {
    NativeScriptToken_End, NativeScriptToken_Name, NativeScriptToken_Integer,
    NativeScriptToken_Floating, NativeScriptToken_String, NativeScriptToken_Dot,
    NativeScriptToken_OpenParen, NativeScriptToken_CloseParen,
    NativeScriptToken_OpenIndex, NativeScriptToken_CloseIndex,
    NativeScriptToken_Comma, NativeScriptToken_Operator,
    NativeScriptToken_Assign, NativeScriptToken_Colon,
} NativeScriptTokenKind;

typedef struct NativeScriptExpressionToken {
    NativeScriptSourceSpan source;
    uint32_t text_offset, text_length;
    uint64_t bits;
    uint8_t kind, type, operation;
} NativeScriptExpressionToken;

// Shared lexical inspection for inventories and declaration/statement readers.
// Includes a final End token. It does not assert semantic or grammar validity.
// Failure sets count=0; token capacity is supplied by the load-time caller.
bool nativeScriptExpression_Tokenize(const char* source, uint32_t source_bytes,
    NativeScriptSourceSpan, NativeScriptExpressionToken*, uint32_t capacity,
    uint32_t* count, NativeScriptDiagnostic*);

// Parse exactly one expression in source. Caller owns fixed arena storage and
// immutable source; failure restores both arena counts and returns NODE_NONE.
bool nativeScriptExpression_Parse(NativeScriptIRArena*,
    NativeScriptSourceSpan, NativeScriptIRResolver, void* context,
    uint32_t* expression, NativeScriptDiagnostic*);

bool nativeScriptExpression_IsNumeric(NativeScriptIRType);
bool nativeScriptExpression_IsIntegral(NativeScriptIRType);
bool nativeScriptExpression_Assignable(NativeScriptIRType target,
                                      NativeScriptIRType value);

// Append a deliberate scalar conversion for assignment/return/arguments.
// No implicit float-to-int assignment; int()/uint() bindings can request an
// explicit conversion after checking their own argument contract.
bool nativeScriptExpression_Convert(NativeScriptIRArena*, uint32_t expression,
    NativeScriptIRType target, bool explicit_conversion, uint32_t* converted,
    NativeScriptDiagnostic*);

const char* nativeScriptExpression_TypeName(NativeScriptIRType);

#endif
