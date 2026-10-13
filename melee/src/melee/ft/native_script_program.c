#include "native_script_program.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

static bool error(NativeScriptDiagnostic* diagnostic, NativeScriptSourceSpan source,
                   const char* message)
{
    diagnostic->source=source;
    snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);
    return false;
}

static bool same(const NativeScriptSourceTree* tree, NativeScriptSourceSpan a,
                  NativeScriptSourceSpan b)
{
    return a.length==b.length && memcmp(tree->source+a.offset,tree->source+b.offset,a.length)==0;
}

static uint32_t export_key(const char* text,uint32_t length)
{
    uint32_t key=UINT32_C(2166136261);
    for(uint32_t i=0;i<length;++i){key^=(unsigned char)text[i];key*=UINT32_C(16777619);}
    return key;
}

bool nativeScriptProgram_ExportName(NativeScriptProgramIndex* index,NativeScriptSourceSpan name,
    uint32_t* output,NativeScriptDiagnostic* diagnostic)
{
    if(!index||!index->tree||!output||!diagnostic||!index->tree->source||
       name.offset>index->tree->source_bytes||name.length>index->tree->source_bytes-name.offset)
        return diagnostic?error(diagnostic,name,"Export name source span is invalid"):false;
    const char* text=index->tree->source+name.offset;
    if(!name.length||name.length>127)return error(diagnostic,name,"Export name must contain 1 through 127 identifier characters");
    for(uint32_t i=0;i<name.length;++i){unsigned char ch=(unsigned char)text[i];
        if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||ch=='_'||(i&&ch>='0'&&ch<='9')))
            return error(diagnostic,name,"Export name must be an identifier");}
    if(index->export_name_count>NATIVE_SCRIPT_EXPORT_NAMES)return error(diagnostic,name,"Export name table is invalid");
    for(uint32_t i=0;i<index->export_name_count;++i)if(same(index->tree,index->export_names[i].name,name)){*output=i;return true;}
    if(index->export_name_count==NATIVE_SCRIPT_EXPORT_NAMES)return error(diagnostic,name,"Prepared export name capacity exhausted");
    uint32_t at=index->export_name_count;
    index->export_names[at]=(NativeScriptExportName){.key=export_key(text,name.length),.name=name};
    index->export_name_count=at+1;*output=at;return true;
}

static bool index_export(NativeScriptProgramIndex* index,uint32_t function,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptFunctionDeclaration* declaration=&index->functions[function];
    NativeScriptSourceSpan name=declaration->name;
    if(!index->tree->source || !name.length || name.length>127 ||
       name.offset>index->tree->source_bytes || name.length>index->tree->source_bytes-name.offset)
        return error(diagnostic,name,"Export declaration needs a valid 1 through 127 character name");
    if(declaration->owner||declaration->callback||declaration->parameter_count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS-2||
       (declaration->result_type!=NativeScriptType_Void&&declaration->result_type!=NativeScriptType_Bool&&
        declaration->result_type!=NativeScriptType_I32&&declaration->result_type!=NativeScriptType_F32))
        return error(diagnostic,declaration->name,"Export must be a root helper with at most 30 parameters returning Void, Bool, Int or Float");
    if(declaration->parameter_first>index->parameter_count ||
       declaration->parameter_count>index->parameter_count-declaration->parameter_first)
        return error(diagnostic,declaration->name,"Export parameter range is malformed");
    for(unsigned p=0;p<declaration->parameter_count;++p) {
        NativeScriptIRType type=index->parameters[declaration->parameter_first+p].type;
        if(type!=NativeScriptType_Bool && type!=NativeScriptType_I32 && type!=NativeScriptType_U32 && type!=NativeScriptType_F32)
            return error(diagnostic,declaration->name,"Export parameters must be Bool, Int, UInt or Float");
    }
    if(index->export_count>=NATIVE_SCRIPT_EXPORTS)return error(diagnostic,declaration->name,"Prepared export capacity exhausted");
    NativeScriptExport value={.key=export_key(index->tree->source+declaration->name.offset,declaration->name.length),
        .function=function,.name=declaration->name,.result_type=declaration->result_type};
    uint32_t at=0;while(at<index->export_count&&index->exports[at].key<=value.key){
        if(index->exports[at].key==value.key&&same(index->tree,index->exports[at].name,value.name))
            return error(diagnostic,declaration->name,"Duplicate exported function name");
        ++at;
    }
    memmove(index->exports+at+1,index->exports+at,(index->export_count-at)*sizeof(*index->exports));
    index->exports[at]=value;++index->export_count;return true;
}

static NativeScriptIRType type(const NativeScriptSourceTree* tree,
                                NativeScriptSourceSpan source)
{
    if (nativeScriptSource_SpanEquals(tree,source,"bool")) return NativeScriptType_Bool;
    if (nativeScriptSource_SpanEquals(tree,source,"int")) return NativeScriptType_I32;
    if (nativeScriptSource_SpanEquals(tree,source,"uint")) return NativeScriptType_U32;
    if (nativeScriptSource_SpanEquals(tree,source,"float")) return NativeScriptType_F32;
    return NativeScriptType_Invalid;
}

static uint32_t owner(const NativeScriptSourceTree* tree,uint32_t node)
{
    while (node!=UINT32_MAX) {
        unsigned kind=tree->nodes[node].kind;
        if (kind==NativeScriptSource_Article || kind==NativeScriptSource_Item || kind==NativeScriptSource_Effect)
            return node;
        node=tree->nodes[node].parent;
    }
    return 0;
}

static bool tokenize(const NativeScriptProgramIndex* index,NativeScriptSourceSpan source,
                      NativeScriptExpressionToken* tokens,uint32_t capacity,
                      uint32_t* count,NativeScriptDiagnostic* diagnostic)
{
    return nativeScriptExpression_Tokenize(index->tree->source,index->tree->source_bytes,
        source,tokens,capacity,count,diagnostic);
}

static bool signature(NativeScriptProgramIndex* index,NativeScriptFunctionDeclaration* function,
                       NativeScriptDiagnostic* diagnostic)
{
    NativeScriptSourceNode* node=&index->tree->nodes[function->source_node];
    function->parameter_first=index->parameter_count;
    function->result_type=NativeScriptType_Void;
    if (function->callback) {
        if (!index->event_signature) return true;
        uint32_t count=0;
        uint32_t available=index->parameter_capacity-index->parameter_count;
        if (!index->event_signature(index->event_context,index->tree,node,function->owner,
                &function->result_type,&function->default_return,&function->body_policy,
                index->parameters+index->parameter_count,available,&count,diagnostic)) return false;
        if (count>available || count>NATIVE_SCRIPT_EXPRESSION_ARGUMENTS)
            return error(diagnostic,node->source,"Event parameter capacity exceeded");
        if (function->result_type==NativeScriptType_Invalid ||
            function->default_return.kind>NativeScriptDefaultReturn_Parameter ||
            (function->body_policy&~NativeScriptBodyPolicy_All) ||
            ((function->body_policy&NativeScriptBodyPolicy_StrictReturn) &&
             (function->body_policy&NativeScriptBodyPolicy_TruthReturn)) ||
            ((function->body_policy&NativeScriptBodyPolicy_TruthReturn) && function->result_type!=NativeScriptType_Bool))
            return error(diagnostic,node->source,"Invalid event return contract");
        for (uint32_t i=0;i<count;++i)
            if (index->parameters[index->parameter_count+i].type==NativeScriptType_Invalid ||
                index->parameters[index->parameter_count+i].type==NativeScriptType_Void)
                return error(diagnostic,node->source,"Invalid event parameter type");
        if (function->default_return.kind!=NativeScriptDefaultReturn_Reject) {
            if (function->default_return.type!=function->result_type || function->result_type==NativeScriptType_Void)
                return error(diagnostic,node->source,"Event default return type differs from its signature");
            if (function->default_return.kind==NativeScriptDefaultReturn_Parameter &&
                (function->default_return.argument>=count ||
                 !nativeScriptExpression_Assignable(function->result_type,
                    index->parameters[index->parameter_count+function->default_return.argument].type)))
                return error(diagnostic,node->source,"Event default parameter return is invalid");
        }
        function->parameter_count=count;index->parameter_count+=count;
        function->signature_resolved=true;
        return true;
    }
    NativeScriptExpressionToken tokens[128];uint32_t count=0;
    if (!tokenize(index,node->value,tokens,128,&count,diagnostic)) return false;
    uint32_t at=0;
    if (tokens[at++].kind!=NativeScriptToken_OpenParen)
        return error(diagnostic,node->value,"Helper signature needs '('");
    while (at<count && tokens[at].kind!=NativeScriptToken_CloseParen) {
        if (function->parameter_count==NATIVE_SCRIPT_EXPRESSION_ARGUMENTS || index->parameter_count==index->parameter_capacity)
            return error(diagnostic,node->source,"Helper parameter capacity exceeded");
        NativeScriptIRType value_type=type(index->tree,tokens[at++].source);
        if (value_type==NativeScriptType_Invalid || at>=count || tokens[at].kind!=NativeScriptToken_Name)
            return error(diagnostic,node->value,"Helper parameters need a scalar type and name");
        NativeScriptSourceSpan name=tokens[at++].source;
        for (uint32_t i=function->parameter_first;i<index->parameter_count;++i)
            if (same(index->tree,index->parameters[i].name,name))
                return error(diagnostic,name,"Duplicate helper parameter");
        index->parameters[index->parameter_count++]=(NativeScriptParameterDeclaration){name,value_type};
        ++function->parameter_count;
        if (tokens[at].kind==NativeScriptToken_CloseParen) break;
        if (tokens[at++].kind!=NativeScriptToken_Comma || tokens[at].kind==NativeScriptToken_CloseParen)
            return error(diagnostic,node->value,"Expected next typed helper parameter");
    }
    if (at>=count || tokens[at++].kind!=NativeScriptToken_CloseParen)
        return error(diagnostic,node->value,"Unterminated helper parameter list");
    if (tokens[at].kind!=NativeScriptToken_End) {
        if (at+3>=count || tokens[at].operation!=NativeScriptOp_Subtract ||
            tokens[at+1].operation!=NativeScriptOp_Greater)
            return error(diagnostic,tokens[at].source,"Expected helper return arrow");
        function->result_type=type(index->tree,tokens[at+2].source);at+=3;
        if (function->result_type==NativeScriptType_Invalid)
            return error(diagnostic,node->source,"Unknown helper return type");
    }
    if (tokens[at].kind!=NativeScriptToken_End) return error(diagnostic,tokens[at].source,"Unexpected signature suffix");
    function->signature_resolved=true;
    return true;
}

static bool constant(NativeScriptIRArena* arena,uint32_t id,NativeScriptValue* result,
                     NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptIRNode* node=&arena->nodes[id];
    *result=(NativeScriptValue){.type=node->type};
    switch (node->kind) {
    case NativeScriptIR_Integer: case NativeScriptIR_Boolean:
        result->as.u64=node->bits; return true;
    case NativeScriptIR_Floating:
        if (node->type==NativeScriptType_F32) {uint32_t bits=(uint32_t)node->bits;memcpy(&result->as.f32,&bits,4);}
        else memcpy(&result->as.f64,&node->bits,8);
        return true;
    case NativeScriptIR_Convert: case NativeScriptIR_Unary: {
        NativeScriptValue value;
        if (!constant(arena,node->a,&value,diagnostic)) return false;
        return node->kind==NativeScriptIR_Convert?
            nativeScriptValue_Convert(&value,(NativeScriptIRType)node->type,result,diagnostic):
            nativeScriptValue_Unary((NativeScriptIROperation)node->operation,&value,result,diagnostic);
    }
    case NativeScriptIR_Binary: {
        NativeScriptValue a,b;
        if (!constant(arena,node->a,&a,diagnostic)) return false;
        if (node->operation==NativeScriptOp_And || node->operation==NativeScriptOp_Or) {
            bool truth;
            if (!nativeScriptValue_Truth(&a,&truth)) return error(diagnostic,node->source,"Invalid constant condition");
            if ((node->operation==NativeScriptOp_And && !truth) || (node->operation==NativeScriptOp_Or && truth)) {
                *result=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=truth}; return true;
            }
        }
        if (!constant(arena,node->b,&b,diagnostic)) return false;
        return nativeScriptValue_Binary((NativeScriptIROperation)node->operation,&a,&b,result,diagnostic);
    }
    default: return error(diagnostic,node->source,"Parameter default must be a constant scalar expression");
    }
}

static bool resolve_constant(void* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptProgramIndex* index=context;
    if (node->kind!=NativeScriptIR_Name && node->kind!=NativeScriptIR_Member)
        return error(diagnostic,node->source,"Parameter default cannot call or index a semantic value");
    if (node->kind==NativeScriptIR_Member &&
        (node->a>=arena->node_count || arena->nodes[node->a].type!=NativeScriptType_Namespace))
        return error(diagnostic,node->source,"Parameter default member needs a constant namespace");
    if (!index->constant_resolver)
        return error(diagnostic,node->source,"Parameter default has no constant identifier resolver");
    NativeScriptIRResolution resolved={.semantic=NATIVE_SCRIPT_SEMANTIC_NONE};
    if (!index->constant_resolver(index->constant_context,arena,node,&resolved,diagnostic)) {
        if (!diagnostic->message[0]) return error(diagnostic,node->source,"Unknown parameter default constant");
        if (!diagnostic->source.length) diagnostic->source=node->source;
        return false;
    }
    if (resolved.type==NativeScriptType_Namespace && resolved.flags==0 &&
        resolved.semantic!=NATIVE_SCRIPT_SEMANTIC_NONE) {
        *result=resolved;return true;
    }
    if ((nativeScriptExpression_IsNumeric(resolved.type) || resolved.type==NativeScriptType_Bool) &&
        resolved.flags==NativeScriptFlag_Literal && resolved.semantic==NATIVE_SCRIPT_SEMANTIC_NONE) {
        *result=resolved;return true;
    }
    return error(diagnostic,node->source,"Parameter default resolver must supply a scalar literal or constant namespace");
}

bool nativeScriptProgram_EvaluateConstant(const NativeScriptProgramIndex* index,NativeScriptSourceSpan span,
                          NativeScriptIRType target,NativeScriptValue* value,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptSourceTree* tree=index->tree;
    NativeScriptIRNode nodes[64];uint32_t arguments[64],expression,converted;
    NativeScriptIRArena arena={tree->source,tree->source_bytes,nodes,0,64,arguments,0,64};
    if (!nativeScriptExpression_Parse(&arena,span,resolve_constant,(void*)index,&expression,diagnostic)) return false;
    // Even unevaluated short-circuit operands must belong to the scalar constant
    // language. Arithmetic errors inside a skipped operand remain unevaluated.
    for (uint32_t i=0;i<arena.node_count;++i) {
        const NativeScriptIRNode* part=&arena.nodes[i];
        if (part->kind==NativeScriptIR_String || part->kind==NativeScriptIR_Null)
            return error(diagnostic,part->source,"Parameter default permits only numeric/Bool constants");
    }
    if (
        !nativeScriptExpression_Convert(&arena,expression,target,false,&converted,diagnostic) ||
        !constant(&arena,converted,value,diagnostic)) {
        if (!diagnostic->source.length) diagnostic->source=span;
        return false;
    }
    return true;
}

static bool value_declaration(NativeScriptProgramIndex* index,uint32_t source_node,
                              NativeScriptDiagnostic* diagnostic)
{
    NativeScriptSourceNode* node=&index->tree->nodes[source_node];
    if (index->value_count==index->value_capacity)
        return error(diagnostic,node->source,"Owned value declaration capacity exceeded");
    NativeScriptValueDeclaration value={.name=node->name,.source_node=source_node,
        .owner=owner(index->tree,source_node),.count=1,.type=(NativeScriptIRType)node->value_type,
        .parameter=node->kind!=NativeScriptSource_State,.imported=node->kind==NativeScriptSource_Attribute,
        .default_value={.type=node->value_type}};
    for (uint32_t i=0;i<index->value_count;++i)
        if (index->values[i].owner==value.owner && index->values[i].parameter==value.parameter &&
            same(index->tree,index->values[i].name,value.name))
            return error(diagnostic,node->name,"Duplicate owned value in this scope");
    if (value.imported) {
        NativeScriptExpressionToken tokens[4];uint32_t count=0;
        if (!tokenize(index,node->value,tokens,4,&count,diagnostic)) return false;
        if (count!=2 || tokens[0].kind!=NativeScriptToken_Integer || tokens[0].bits>UINT32_MAX)
            return error(diagnostic,node->value,"Imported attribute needs a nonnegative byte offset");
        value.import_offset=(uint32_t)tokens[0].bits;
    } else if (value.parameter) {
        NativeScriptSourceSpan span=node->value;
        if(!span.length || index->tree->source[span.offset]!='=')
            return error(diagnostic,node->source,"Parameter needs '= constant expression'");
        ++span.offset;--span.length;++span.column;
        if (!nativeScriptProgram_EvaluateConstant(index,span,value.type,&value.default_value,diagnostic)) return false;
    } else if (node->value.length) {
        NativeScriptExpressionToken tokens[8];uint32_t count=0;
        if (!tokenize(index,node->value,tokens,8,&count,diagnostic)) return false;
        if (count!=4 || tokens[0].kind!=NativeScriptToken_OpenIndex || tokens[1].kind!=NativeScriptToken_Integer ||
            tokens[1].bits==0 || tokens[1].bits>UINT16_MAX || tokens[2].kind!=NativeScriptToken_CloseIndex)
            return error(diagnostic,node->value,"State array needs a positive constant count");
        value.count=(uint32_t)tokens[1].bits;
        value.array=true;
    }
    if (!value.parameter) {
        uint32_t bytes=value.type==NativeScriptType_Bool?1:4;
        for (uint32_t i=0;i<index->value_count;++i) {
            const NativeScriptValueDeclaration* previous=&index->values[i];
            if (!previous->parameter && previous->owner==value.owner) {
                uint32_t end=previous->byte_offset+previous->count*(previous->type==NativeScriptType_Bool?1:4);
                if (end>value.byte_offset) value.byte_offset=end;
            }
        }
        value.byte_offset=(value.byte_offset+bytes-1)&~(bytes-1);
    }
    index->values[index->value_count++]=value;
    return true;
}

static bool table_declaration(NativeScriptProgramIndex* index,uint32_t source_node,
                              NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptSourceTree* tree=index->tree;
    const NativeScriptSourceNode* node=&tree->nodes[source_node];
    uint32_t scope=owner(tree,source_node);
    if(node->parent!=scope || (node->value_type!=NativeScriptType_Bool &&
       node->value_type!=NativeScriptType_I32 && node->value_type!=NativeScriptType_U32 &&
       node->value_type!=NativeScriptType_F32))
        return error(diagnostic,node->source,"Table requires a root/entity scope and Bool, Int, Uint or Float elements");
    if(index->table_count==NATIVE_SCRIPT_TABLES)
        return error(diagnostic,node->source,"Constant table declaration capacity exhausted");
    for(uint32_t i=0;i<index->table_count;++i)
        if(index->tables[i].owner==scope && same(tree,index->tables[i].name,node->name))
            return error(diagnostic,node->name,"Duplicate constant table in this scope");
    NativeScriptSourceSpan span=node->value;
    if(span.offset>tree->source_bytes || span.length>tree->source_bytes-span.offset)
        return error(diagnostic,node->source,"Table source span is invalid");
    const char* text=tree->source+span.offset;uint32_t at=0;
    if(span.length<4 || text[at++]!='=')
        return error(diagnostic,span,"Table needs '= [constant expressions]'");
    while(at<span.length && (text[at]==' '||text[at]=='\t'))++at;
    if(at==span.length || text[at++]!='[' || text[span.length-1]!=']')
        return error(diagnostic,span,"Table needs a bracketed constant list");
    NativeScriptTableDeclaration table={node->name,source_node,scope,index->table_element_count,0,node->value_type};
    uint32_t end=span.length-1;
    while(at<end) {
        uint32_t start=at;unsigned parentheses=0;
        for(;at<end;++at) {
            if(text[at]=='(')++parentheses;
            else if(text[at]==')') {if(!parentheses)return error(diagnostic,span,"Unbalanced table expression");--parentheses;}
            else if(text[at]==','&&!parentheses)break;
        }
        if(parentheses)return error(diagnostic,span,"Unbalanced table expression");
        NativeScriptSourceSpan element={span.offset+start,at-start,span.line,span.column+start};
        if(table.first+table.count==NATIVE_SCRIPT_TABLE_ELEMENTS)
            return error(diagnostic,element,"Constant table element capacity exhausted");
        NativeScriptValue value;
        if(!nativeScriptProgram_EvaluateConstant(index,element,table.type,&value,diagnostic))return false;
        uint32_t bits;
        if(table.type==NativeScriptType_F32) {
            if(!isfinite(value.as.f32))return error(diagnostic,element,"Table Float must be finite");
            memcpy(&bits,&value.as.f32,4);
        } else bits=(uint32_t)value.as.u64;
        index->table_bits[table.first+table.count++]=bits;
        if(at<end && ++at==end)return error(diagnostic,span,"Table cannot end with an empty element");
    }
    if(!table.count)return error(diagnostic,span,"Table needs at least one element");
    index->tables[index->table_count++]=table;index->table_element_count+=table.count;
    return true;
}

bool nativeScriptProgram_ReadTable(const NativeScriptProgramIndex* index,uint32_t scope,
    uint32_t semantic,const NativeScriptValue* selectors,uint32_t count,
    NativeScriptValue* output,NativeScriptDiagnostic* diagnostic)
{
    uint32_t id=semantic&NATIVE_SCRIPT_TABLE_INDEX_MASK;
    if(!index || !output || (semantic&NATIVE_SCRIPT_TABLE_DOMAIN_MASK)!=NATIVE_SCRIPT_TABLE_SEMANTIC ||
       index->table_count>NATIVE_SCRIPT_TABLES || id>=index->table_count)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Unknown immutable table declaration");
    const NativeScriptTableDeclaration* table=&index->tables[id];
    if(table->owner && table->owner!=scope)return error(diagnostic,table->name,"Table belongs to a different entity scope");
    if(count!=1 || !selectors || !nativeScriptExpression_IsIntegral(selectors[0].type))
        return error(diagnostic,table->name,"Table needs one integral index");
    NativeScriptValue value=selectors[0];uint64_t element;
    if(value.type==NativeScriptType_I32 || value.type==NativeScriptType_I64) {
        if(value.as.i64<0)return error(diagnostic,table->name,"Table index is negative");
        element=(uint64_t)value.as.i64;
    } else element=value.type==NativeScriptType_Bool?value.as.u64!=0:value.as.u64;
    if(!table->count || element>=table->count || index->table_element_count>NATIVE_SCRIPT_TABLE_ELEMENTS ||
       table->first>index->table_element_count || table->count>index->table_element_count-table->first)
        return error(diagnostic,table->name,"Table index or element range is outside its declaration");
    uint32_t bits=index->table_bits[table->first+(uint32_t)element];value=(NativeScriptValue){.type=table->type};
    if(table->type==NativeScriptType_F32)memcpy(&value.as.f32,&bits,4);
    else if(table->type==NativeScriptType_I32)value.as.i64=bits<=INT32_MAX?(int64_t)bits:-1-(int64_t)(UINT32_MAX-bits);
    else if(table->type==NativeScriptType_U32)value.as.u64=bits;
    else if(table->type==NativeScriptType_Bool)value.as.u64=bits!=0;
    else return error(diagnostic,table->name,"Invalid immutable table element type");
    *output=value;return true;
}

bool nativeScriptProgram_Index(NativeScriptProgramIndex* index,NativeScriptDiagnostic* diagnostic)
{
    if (!index || !index->tree || !index->functions || !index->parameters || !index->values || !diagnostic) return false;
    index->function_count=index->parameter_count=index->value_count=index->export_count=index->export_name_count=0;
    index->table_count=index->table_element_count=0;
    memset(diagnostic,0,sizeof(*diagnostic));
    for (uint32_t i=1;i<index->tree->count;++i) {
        NativeScriptSourceNode* node=&index->tree->nodes[i];
        if (node->kind==NativeScriptSource_Function || node->kind==NativeScriptSource_Callback) {
            if (index->function_count==index->function_capacity) {error(diagnostic,node->source,"Function index capacity exceeded");goto failure;}
            NativeScriptFunctionDeclaration function={.name=node->name,.source_node=i,.owner=owner(index->tree,i),
                .callback=node->kind==NativeScriptSource_Callback,.exported=node->exported};
            for (uint32_t p=0;p<index->function_count;++p)
                if (!function.callback && !index->functions[p].callback && index->functions[p].owner==function.owner &&
                    same(index->tree,index->functions[p].name,function.name)) {error(diagnostic,node->name,"Duplicate helper in this scope");goto failure;}
            if (!signature(index,&function,diagnostic)) goto failure;
            index->functions[index->function_count++]=function;
            if(function.exported&&!index_export(index,index->function_count-1,diagnostic))goto failure;
        } else if (node->kind==NativeScriptSource_Table) {
            if (!table_declaration(index,i,diagnostic)) goto failure;
        } else if (node->kind==NativeScriptSource_State || node->kind==NativeScriptSource_Parameter || node->kind==NativeScriptSource_Attribute) {
            if (!value_declaration(index,i,diagnostic)) goto failure;
        }
    }
    return true;
failure:
    index->function_count=index->parameter_count=index->value_count=index->export_count=index->export_name_count=0;
    index->table_count=index->table_element_count=0;
    return false;
}

static bool name_matches(const NativeScriptProgramIndex* index,const NativeScriptIRNode* node,NativeScriptSourceSpan name)
{
    return name.length==node->text_length && memcmp(index->tree->source+name.offset,index->tree->source+node->text_offset,name.length)==0;
}

bool nativeScriptProgram_Resolve(void* context,const NativeScriptIRArena* arena,const NativeScriptIRNode* node,
    NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptProgramCompiler* compiler=context;NativeScriptProgramIndex* index=compiler->index;
    uint32_t current_owner=index->functions[compiler->active_function].owner;
    if (node->kind==NativeScriptIR_Name) {
        if (node->text_length==5 && !memcmp(arena->source+node->text_offset,"table",5)) {
            *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_TABLE_NAMESPACE,0,0};return true;
        }
        if (node->text_length==7 && memcmp(arena->source+node->text_offset,"program",7)==0) {
            *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_PROGRAM_NAMESPACE,0,0};return true;
        }
        if (current_owner && (index->tree->nodes[current_owner].kind==NativeScriptSource_Article ||
                              index->tree->nodes[current_owner].kind==NativeScriptSource_Item) &&
            node->text_length==4 && memcmp(arena->source+node->text_offset,"item",4)==0) {
            *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_ITEM_NAMESPACE,0,0};return true;
        }
        if (node->text_length==5 && memcmp(arena->source+node->text_offset,"state",5)==0) {
            *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_STATE_NAMESPACE,0,0};return true;
        }
        if (node->text_length==5 && memcmp(arena->source+node->text_offset,"attrs",5)==0) {
            *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_ATTRIBUTE_NAMESPACE,0,0};return true;
        }
        for (unsigned pass=0;pass<2;++pass) {
            uint32_t scope=pass?0:current_owner;
            for (uint32_t i=0;i<index->function_count;++i) {
                const NativeScriptFunctionDeclaration* function=&index->functions[i];
                if (!function->callback && function->owner==scope && name_matches(index,node,function->name)) {
                    *result=(NativeScriptIRResolution){NativeScriptType_Callable,NATIVE_SCRIPT_FUNCTION_SEMANTIC|i,
                        NativeScriptFlag_Callable|NativeScriptFlag_Readable,0};return true;
                }
            }
        }
    } else if (node->kind==NativeScriptIR_Call && (arena->nodes[node->a].semantic&UINT32_C(0xc0000000))==NATIVE_SCRIPT_FUNCTION_SEMANTIC) {
        uint32_t id=arena->nodes[node->a].semantic&NATIVE_SCRIPT_SEMANTIC_INDEX;
        if (id>=index->function_count) return error(diagnostic,node->source,"Invalid helper identity");
        const NativeScriptFunctionDeclaration* function=&index->functions[id];
        if (!function->signature_resolved || function->parameter_count!=node->argument_count)
            return error(diagnostic,node->source,"Helper argument count or signature mismatch");
        for (uint32_t i=0;i<node->argument_count;++i) {
            uint32_t value=arena->arguments[node->argument_first+i];
            if (!nativeScriptExpression_Assignable(index->parameters[function->parameter_first+i].type,(NativeScriptIRType)arena->nodes[value].type))
                return error(diagnostic,arena->nodes[value].source,"Helper argument type mismatch");
        }
        *result=(NativeScriptIRResolution){function->result_type,NATIVE_SCRIPT_FUNCTION_SEMANTIC|id,NativeScriptFlag_Readable,0};return true;
    } else if (node->kind==NativeScriptIR_Member) {
        uint32_t base=arena->nodes[node->a].semantic;
        if (base==NATIVE_SCRIPT_PROGRAM_NAMESPACE) {
            if (node->text_length==5 && !memcmp(arena->source+node->text_offset,"table",5)) {
                *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_PROGRAM_TABLE_NAMESPACE,0,0};return true;
            }
            if (node->text_length==5 && !memcmp(arena->source+node->text_offset,"attrs",5)) {
                *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_PROGRAM_ATTRIBUTE_NAMESPACE,0,0};return true;
            }
            if (node->text_length==13 && !memcmp(arena->source+node->text_offset,"entity_params",13)) {
                *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_PROGRAM_ENTITY_PARAMETERS_NAMESPACE,0,0};return true;
            }
            return error(diagnostic,node->source,"Program exposes immutable root attributes, entity parameters and tables");
        }
        if(base==NATIVE_SCRIPT_PROGRAM_ENTITY_PARAMETERS_NAMESPACE) {
            uint32_t selected=0;
            for(uint32_t i=1;i<index->tree->count;++i) {
                const NativeScriptSourceNode* entity=&index->tree->nodes[i];
                if((entity->kind==NativeScriptSource_Article||entity->kind==NativeScriptSource_Item||entity->kind==NativeScriptSource_Effect)&&
                   name_matches(index,node,entity->name)) {
                    if(selected)return error(diagnostic,node->source,"Immutable entity parameter name is ambiguous");
                    selected=i;
                }
            }
            if(!selected||selected>NATIVE_SCRIPT_ENTITY_PARAMETERS_OWNER_MASK)
                return error(diagnostic,node->source,"Unknown declared entity in immutable package parameters");
            *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_ENTITY_PARAMETERS_NAMESPACE|selected,0,0};return true;
        }
        if(base==NATIVE_SCRIPT_TABLE_NAMESPACE || base==NATIVE_SCRIPT_PROGRAM_TABLE_NAMESPACE) {
            for(unsigned pass=0;pass<2;++pass) {
                uint32_t scope=base==NATIVE_SCRIPT_PROGRAM_TABLE_NAMESPACE || pass ? 0:current_owner;
                for(uint32_t i=0;i<index->table_count;++i) {
                    const NativeScriptTableDeclaration* table=&index->tables[i];
                    if(table->owner==scope && name_matches(index,node,table->name)) {
                        *result=(NativeScriptIRResolution){NativeScriptType_Object,NATIVE_SCRIPT_TABLE_SEMANTIC|i,NativeScriptFlag_Readable,0};return true;
                    }
                }
            }
            return error(diagnostic,node->source,"Unknown immutable table in this scope");
        }
        if (base==NATIVE_SCRIPT_ITEM_NAMESPACE) {
            if (node->text_length==5 && !memcmp(arena->source+node->text_offset,"state",5)) {
                *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_STATE_NAMESPACE,0,0};return true;
            }
            if ((node->text_length==6 && !memcmp(arena->source+node->text_offset,"params",6)) ||
                (node->text_length==4 && !memcmp(arena->source+node->text_offset,"attr",4))) {
                *result=(NativeScriptIRResolution){NativeScriptType_Namespace,NATIVE_SCRIPT_ATTRIBUTE_NAMESPACE,0,0};return true;
            }
        }
        bool entity_parameters=(base&NATIVE_SCRIPT_ENTITY_PARAMETERS_DOMAIN_MASK)==NATIVE_SCRIPT_ENTITY_PARAMETERS_NAMESPACE;
        if (base==NATIVE_SCRIPT_STATE_NAMESPACE || base==NATIVE_SCRIPT_ATTRIBUTE_NAMESPACE ||
            base==NATIVE_SCRIPT_PROGRAM_ATTRIBUTE_NAMESPACE || entity_parameters) {
            bool program_parameter=base==NATIVE_SCRIPT_PROGRAM_ATTRIBUTE_NAMESPACE || entity_parameters;
            bool parameter=base!=NATIVE_SCRIPT_STATE_NAMESPACE;
            uint32_t value_owner=entity_parameters?base&NATIVE_SCRIPT_ENTITY_PARAMETERS_OWNER_MASK:program_parameter?0:current_owner;
            for (uint32_t i=0;i<index->value_count;++i) {
                const NativeScriptValueDeclaration* value=&index->values[i];
                if (value->owner==value_owner && value->parameter==parameter && name_matches(index,node,value->name)) {
                    uint16_t flags=NativeScriptFlag_Readable;
                    if (!parameter) flags|=NativeScriptFlag_Addressable|(!value->array?NativeScriptFlag_Writable:0);
                    *result=(NativeScriptIRResolution){!value->array?value->type:NativeScriptType_Object,
                        (program_parameter?NATIVE_SCRIPT_PROGRAM_PARAMETER_SEMANTIC:
                            parameter?NATIVE_SCRIPT_PARAMETER_SEMANTIC:NATIVE_SCRIPT_STATE_SEMANTIC)|i,flags,0};return true;
                }
            }
            if (program_parameter)return error(diagnostic,node->source,"Unknown immutable parameter in the selected program scope");
        }
    } else if(node->kind==NativeScriptIR_Index && (arena->nodes[node->a].semantic&NATIVE_SCRIPT_TABLE_DOMAIN_MASK)==NATIVE_SCRIPT_TABLE_SEMANTIC) {
        uint32_t id=arena->nodes[node->a].semantic&NATIVE_SCRIPT_TABLE_INDEX_MASK;
        if(id>=index->table_count || arena->nodes[node->a].type!=NativeScriptType_Object ||
            !nativeScriptExpression_IsIntegral((NativeScriptIRType)arena->nodes[node->b].type))
            return error(diagnostic,node->source,"Invalid immutable table index");
        *result=(NativeScriptIRResolution){index->tables[id].type,NATIVE_SCRIPT_TABLE_SEMANTIC|id,NativeScriptFlag_Readable,0};return true;
    } else if (node->kind==NativeScriptIR_Index && (arena->nodes[node->a].semantic&UINT32_C(0xf0000000))==NATIVE_SCRIPT_STATE_SEMANTIC) {
        uint32_t id=arena->nodes[node->a].semantic&UINT32_C(0x0fffffff);
        if (id>=index->value_count || !index->values[id].array || !nativeScriptExpression_IsIntegral((NativeScriptIRType)arena->nodes[node->b].type))
            return error(diagnostic,node->source,"Invalid owned state array index");
        *result=(NativeScriptIRResolution){index->values[id].type,NATIVE_SCRIPT_STATE_SEMANTIC|id,
            NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable,0};return true;
    }
    if (compiler->native_bindings) return compiler->native_bindings(compiler->native_binding_context,arena,node,result,diagnostic);
    return error(diagnostic,node->source,"Unknown program declaration or native binding");
}

bool nativeScriptProgram_DescribeHelper(void* context,uint32_t semantic,NativeScriptCallLayout* layout,NativeScriptDiagnostic* diagnostic)
{
    NativeScriptProgramIndex* index=context;
    uint32_t id=semantic&NATIVE_SCRIPT_SEMANTIC_INDEX;
    if ((semantic&UINT32_C(0xc0000000))!=NATIVE_SCRIPT_FUNCTION_SEMANTIC || id>=index->function_count)
        return error(diagnostic,(NativeScriptSourceSpan){0},"Unknown helper semantic ID");
    const NativeScriptFunctionDeclaration* function=&index->functions[id];
    if (!function->signature_resolved) return error(diagnostic,function->name,"Unresolved helper signature");
    memset(layout,0,sizeof(*layout));layout->argument_count=(uint8_t)function->parameter_count;
    for (uint32_t i=0;i<function->parameter_count;++i) layout->argument_types[i]=(uint8_t)index->parameters[function->parameter_first+i].type;
    return true;
}

bool nativeScriptProgram_CompileBody(NativeScriptProgramCompiler* compiler,uint32_t function,
    NativeScriptStatementCompiler* body,NativeScriptDiagnostic* diagnostic)
{
    if (!compiler || !compiler->index || !body || !diagnostic || function>=compiler->index->function_count) return false;
    NativeScriptProgramIndex* index=compiler->index;
    const NativeScriptFunctionDeclaration* declaration=&index->functions[function];
    if (!declaration->signature_resolved) return error(diagnostic,declaration->name,"Missing event signature contract");
    if (declaration->parameter_count>compiler->local_capacity) return error(diagnostic,declaration->name,"Parameter-local capacity exceeded");
    compiler->active_function=function;
    for (uint32_t i=0;i<declaration->parameter_count;++i) {
        const NativeScriptParameterDeclaration* parameter=&index->parameters[declaration->parameter_first+i];
        compiler->locals[i]=(NativeScriptLocal){parameter->name,declaration->source_node,parameter->type};
    }
    *body=(NativeScriptStatementCompiler){.tree=index->tree,.expressions=index->expressions,
        .bindings=nativeScriptProgram_Resolve,.binding_context=compiler,.locals=compiler->locals,
        .local_count=declaration->parameter_count,.local_capacity=compiler->local_capacity,
        .return_type=declaration->result_type,.default_return=declaration->default_return,
        .body_policy=declaration->body_policy};
    return nativeScriptStatement_Compile(body,declaration->source_node,diagnostic);
}
