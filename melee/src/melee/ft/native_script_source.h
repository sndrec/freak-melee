#ifndef MELEE_FT_NATIVE_SCRIPT_SOURCE_H
#define MELEE_FT_NATIVE_SCRIPT_SOURCE_H

#include "native_script_ir.h"

typedef enum NativeScriptSourceKind {
    NativeScriptSource_Root, NativeScriptSource_Fighter,
    NativeScriptSource_Move, NativeScriptSource_Action,
    NativeScriptSource_Article, NativeScriptSource_Item,
    NativeScriptSource_Effect, NativeScriptSource_Function,
    NativeScriptSource_Callback, NativeScriptSource_ArticleState,
    NativeScriptSource_State, NativeScriptSource_Parameter,
    NativeScriptSource_Attribute, NativeScriptSource_Table, NativeScriptSource_Metadata,
    NativeScriptSource_If, NativeScriptSource_ElseIf,
    NativeScriptSource_Else, NativeScriptSource_While,
    NativeScriptSource_Local, NativeScriptSource_Assign,
    NativeScriptSource_Expression, NativeScriptSource_Return,
    NativeScriptSource_Break, NativeScriptSource_Continue,
    NativeScriptSource_Pass,
} NativeScriptSourceKind;

typedef struct NativeScriptSourceNode {
    NativeScriptSourceSpan source;
    NativeScriptSourceSpan name, value, target, inline_body;
    uint32_t parent, first_child, last_child, next;
    uint32_t expression, target_expression;
    uint16_t indent, child_indent;
    uint8_t kind, value_type;
    bool opens, exported;
} NativeScriptSourceNode;

// Source hierarchy, declarations and statements use fixed caller-owned storage.
// This syntax pass does not claim that metadata or engine bindings are valid.
// Semantic compilation must resolve every declaration/expression before loading.
typedef struct NativeScriptSourceTree {
    const char* source;
    uint32_t source_bytes;
    NativeScriptSourceNode* nodes;
    uint32_t count, capacity;
} NativeScriptSourceTree;

bool nativeScriptSource_Parse(NativeScriptSourceTree*, NativeScriptDiagnostic*);
bool nativeScriptSource_SpanEquals(const NativeScriptSourceTree*,
                                   NativeScriptSourceSpan, const char*);
const char* nativeScriptSource_KindName(NativeScriptSourceKind);

#endif
