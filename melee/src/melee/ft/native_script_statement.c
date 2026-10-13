#include "native_script_statement.h"

#include <stdio.h>
#include <string.h>

typedef struct StatementScope {
    NativeScriptStatementCompiler* compiler;
    uint32_t current_scope, function;
} StatementScope;

static bool error(NativeScriptDiagnostic* diagnostic, NativeScriptSourceSpan source,
                   const char* message)
{
    diagnostic->source=source;
    snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    return false;
}

static bool visible(const NativeScriptSourceTree* tree,uint32_t scope,uint32_t current)
{
    for (;current!=UINT32_MAX;current=tree->nodes[current].parent)
        if (scope==current) return true;
    return false;
}

static bool resolve(void* opaque,const NativeScriptIRArena* arena,
                     const NativeScriptIRNode* node,NativeScriptIRResolution* result,
                     NativeScriptDiagnostic* diagnostic)
{
    StatementScope* scope=opaque;
    NativeScriptStatementCompiler* compiler=scope->compiler;
    if (node->kind==NativeScriptIR_Name) {
        for (uint32_t i=compiler->local_count;i!=0;--i) {
            const NativeScriptLocal* local=&compiler->locals[i-1];
            if (node->text_length==local->name.length &&
                memcmp(arena->source+node->text_offset,arena->source+local->name.offset,node->text_length)==0 &&
                visible(compiler->tree,local->scope,scope->current_scope)) {
                *result=(NativeScriptIRResolution){local->type,NATIVE_SCRIPT_LOCAL_SEMANTIC|(i-1),
                    NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable,0};
                return true;
            }
        }
    }
    if (!compiler->bindings) return error(diagnostic,node->source,"Unknown semantic name or binding");
    return compiler->bindings(compiler->binding_context,arena,node,result,diagnostic);
}

static bool expression(StatementScope* scope,NativeScriptSourceSpan source,
                        uint32_t* output,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptStatementCompiler* compiler=scope->compiler;
    uint32_t first=compiler->expressions->node_count;
    if (!nativeScriptExpression_Parse(compiler->expressions,source,resolve,scope,output,diagnostic)) return false;
    if (compiler->body_policy&NativeScriptBodyPolicy_ReadOnly)
        for (uint32_t i=first;i<compiler->expressions->node_count;++i) {
            const NativeScriptIRNode* node=&compiler->expressions->nodes[i];
            if (node->kind==NativeScriptIR_Call && !(node->flags&NativeScriptFlag_Pure))
                return error(diagnostic,node->source,"Read-only event requires a source-checked pure call");
        }
    return true;
}

static bool block(StatementScope* scope,uint32_t parent,unsigned loop_depth,
                   NativeScriptDiagnostic* diagnostic)
{
    NativeScriptStatementCompiler* compiler=scope->compiler;
    NativeScriptSourceTree* tree=compiler->tree;
    scope->current_scope=parent;
    for (uint32_t at=tree->nodes[parent].first_child;at!=UINT32_MAX;at=tree->nodes[at].next) {
        NativeScriptSourceNode* node=&tree->nodes[at];
        switch (node->kind) {
        case NativeScriptSource_Local: {
            if (!expression(scope,node->value,&node->expression,diagnostic)) return false;
            NativeScriptIRType type=(NativeScriptIRType)node->value_type;
            if (type==NativeScriptType_Invalid) type=(NativeScriptIRType)compiler->expressions->nodes[node->expression].type;
            if (type!=NativeScriptType_Bool && !nativeScriptExpression_IsNumeric(type))
                return error(diagnostic,node->source,"Local values must have a supported scalar type");
            if (!nativeScriptExpression_Convert(compiler->expressions,node->expression,type,false,&node->expression,diagnostic)) return false;
            for (uint32_t i=0;i<compiler->local_count;++i)
                if (compiler->locals[i].scope==parent &&
                    compiler->locals[i].name.length==node->name.length &&
                    memcmp(tree->source+compiler->locals[i].name.offset,tree->source+node->name.offset,node->name.length)==0)
                    return error(diagnostic,node->name,"Duplicate local in this scope");
            if (compiler->local_count==compiler->local_capacity)
                return error(diagnostic,node->source,"Local declaration capacity exceeded");
            compiler->locals[compiler->local_count++]=(NativeScriptLocal){node->name,parent,type};
            node->value_type=(uint8_t)type;
            break;
        }
        case NativeScriptSource_Assign: {
            if (!expression(scope,node->target,&node->target_expression,diagnostic) ||
                !expression(scope,node->value,&node->expression,diagnostic)) return false;
            const NativeScriptIRNode* target=&compiler->expressions->nodes[node->target_expression];
            if ((compiler->body_policy&NativeScriptBodyPolicy_ReadOnly) &&
                (target->semantic&UINT32_C(0xf0000000))!=NATIVE_SCRIPT_LOCAL_SEMANTIC &&
                !(target->flags&NativeScriptFlag_EventOutput))
                return error(diagnostic,node->target,"Read-only event cannot write owned or engine state");
            if (!(target->flags&NativeScriptFlag_Writable))
                return error(diagnostic,node->target,"Assignment target is not writable");
            if (!nativeScriptExpression_Convert(compiler->expressions,node->expression,
                    (NativeScriptIRType)target->type,false,&node->expression,diagnostic)) return false;
            break;
        }
        case NativeScriptSource_Expression:
            if (!expression(scope,node->value,&node->expression,diagnostic)) return false;
            if (compiler->expressions->nodes[node->expression].kind!=NativeScriptIR_Call)
                return error(diagnostic,node->source,"Expression statement must call a function");
            if (compiler->expressions->nodes[node->expression].type==NativeScriptType_Namespace)
                return error(diagnostic,node->source,"Namespace selector requires an executable scalar projection");
            break;
        case NativeScriptSource_Return:
            if (!node->value.length) {
                if (compiler->return_type!=NativeScriptType_Void)
                    return error(diagnostic,node->source,"Typed helper needs a return value");
            } else {
                if (compiler->return_type==NativeScriptType_Void)
                    return error(diagnostic,node->source,"Void callback cannot return a value");
                if (!expression(scope,node->value,&node->expression,diagnostic)) return false;
                if ((compiler->body_policy&NativeScriptBodyPolicy_StrictReturn) &&
                    compiler->expressions->nodes[node->expression].type!=compiler->return_type)
                    return error(diagnostic,node->value,"Event requires an exact return expression type");
                if (!nativeScriptExpression_Convert(compiler->expressions,node->expression,
                        compiler->return_type,(compiler->body_policy&NativeScriptBodyPolicy_TruthReturn)!=0 ||
                            (compiler->return_type==NativeScriptType_Bool &&
                             tree->nodes[scope->function].kind==NativeScriptSource_Function),
                        &node->expression,diagnostic)) return false;
            }
            break;
        case NativeScriptSource_If: case NativeScriptSource_ElseIf: case NativeScriptSource_While:
            if ((compiler->body_policy&NativeScriptBodyPolicy_ReadOnly) && node->kind==NativeScriptSource_While)
                return error(diagnostic,node->source,"Read-only event does not allow while loops");
            if (!expression(scope,node->value,&node->expression,diagnostic) ||
                !nativeScriptExpression_Convert(compiler->expressions,node->expression,
                    NativeScriptType_Bool,true,&node->expression,diagnostic)) return false;
            if (!block(scope,at,loop_depth+(node->kind==NativeScriptSource_While),diagnostic)) return false;
            scope->current_scope=parent;
            break;
        case NativeScriptSource_Else:
            if (!block(scope,at,loop_depth,diagnostic)) return false;
            scope->current_scope=parent;
            break;
        case NativeScriptSource_Break: case NativeScriptSource_Continue:
            if (!loop_depth) return error(diagnostic,node->source,"Loop control requires an enclosing while");
            break;
        case NativeScriptSource_Pass: break;
        default: return error(diagnostic,node->source,"Declaration is not executable callback behavior");
        }
    }
    return true;
}

bool nativeScriptStatement_Compile(NativeScriptStatementCompiler* compiler,
    uint32_t function,NativeScriptDiagnostic* diagnostic)
{
    if (!compiler || !compiler->tree || !compiler->expressions || !compiler->locals || !diagnostic ||
        function>=compiler->tree->count || compiler->tree->source!=compiler->expressions->source ||
        compiler->tree->source_bytes!=compiler->expressions->source_bytes) return false;
    unsigned kind=compiler->tree->nodes[function].kind;
    if ((compiler->body_policy&~NativeScriptBodyPolicy_All) ||
        ((compiler->body_policy&NativeScriptBodyPolicy_StrictReturn) &&
         (compiler->body_policy&NativeScriptBodyPolicy_TruthReturn)) ||
        ((compiler->body_policy&NativeScriptBodyPolicy_TruthReturn) && compiler->return_type!=NativeScriptType_Bool))
        return error(diagnostic,compiler->tree->nodes[function].source,"Unknown body policy");
    if (kind!=NativeScriptSource_Function && kind!=NativeScriptSource_Callback)
        return error(diagnostic,compiler->tree->nodes[function].source,"Expected function or callback body");
    uint32_t nodes=compiler->expressions->node_count,args=compiler->expressions->argument_count;
    uint32_t locals=compiler->local_count;
    StatementScope scope={compiler,function,function};
    memset(diagnostic,0,sizeof(*diagnostic));
    if (block(&scope,function,0,diagnostic)) return true;
    compiler->expressions->node_count=nodes;compiler->expressions->argument_count=args;
    compiler->local_count=locals;
    // Clear rejected body annotations; callers cannot execute a partial body.
    for (uint32_t i=function+1;i<compiler->tree->count;++i)
        if (visible(compiler->tree,function,i)) {
            compiler->tree->nodes[i].expression=UINT32_MAX;
            compiler->tree->nodes[i].target_expression=UINT32_MAX;
        }
    return false;
}
