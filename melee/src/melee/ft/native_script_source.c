#include "native_script_source.h"

#include <ctype.h>
#include <string.h>

enum { SourceDepth = 64 };

static bool fail(NativeScriptDiagnostic* diagnostic, NativeScriptSourceSpan source,
                  const char* message)
{
    if (diagnostic) {
        diagnostic->source = source;
        size_t bytes = strlen(message);
        if (bytes >= sizeof(diagnostic->message)) bytes = sizeof(diagnostic->message)-1;
        memcpy(diagnostic->message, message, bytes);
        diagnostic->message[bytes] = 0;
    }
    return false;
}

bool nativeScriptSource_SpanEquals(const NativeScriptSourceTree* tree,
                                   NativeScriptSourceSpan span, const char* text)
{
    return tree && text && span.offset <= tree->source_bytes &&
        span.length <= tree->source_bytes-span.offset && strlen(text)==span.length &&
        memcmp(tree->source+span.offset,text,span.length)==0;
}

static NativeScriptSourceSpan slice(NativeScriptSourceSpan source,
                                    uint32_t start, uint32_t end)
{
    source.offset += start; source.column += start; source.length = end-start;
    return source;
}

static NativeScriptSourceSpan trimmed(const NativeScriptSourceTree* tree,
                                      NativeScriptSourceSpan source)
{
    uint32_t a=0,b=source.length;
    while (a<b && isspace((unsigned char)tree->source[source.offset+a])) ++a;
    while (b>a && isspace((unsigned char)tree->source[source.offset+b-1])) --b;
    return slice(source,a,b);
}

static NativeScriptSourceSpan word(const NativeScriptSourceTree* tree,
                                   NativeScriptSourceSpan source, uint32_t* at)
{
    const char* text=tree->source+source.offset;
    while (*at<source.length && isspace((unsigned char)text[*at])) ++*at;
    uint32_t first=*at;
    while (*at<source.length && (isalnum((unsigned char)text[*at]) || text[*at]=='_')) ++*at;
    return slice(source,first,*at);
}

static bool identifier(const NativeScriptSourceTree* tree, NativeScriptSourceSpan name)
{
    if (!name.length) return false;
    unsigned char first=(unsigned char)tree->source[name.offset];
    return isalpha(first) || first=='_';
}

static NativeScriptIRType scalar(const NativeScriptSourceTree* tree,
                                  NativeScriptSourceSpan name)
{
    if (nativeScriptSource_SpanEquals(tree,name,"bool")) return NativeScriptType_Bool;
    if (nativeScriptSource_SpanEquals(tree,name,"int")) return NativeScriptType_I32;
    if (nativeScriptSource_SpanEquals(tree,name,"uint")) return NativeScriptType_U32;
    if (nativeScriptSource_SpanEquals(tree,name,"float")) return NativeScriptType_F32;
    return NativeScriptType_Invalid;
}

// Scan quotes, grouping and comments together. '=' is an assignment only at
// nesting zero and outside ==, !=, <=, >=. A colon inside a string is text.
static bool delimiters(const NativeScriptSourceTree* tree, NativeScriptSourceSpan* source,
                        uint32_t* assignment, uint32_t* colon, bool* opens,
                        NativeScriptDiagnostic* diagnostic)
{
    const char* text=tree->source+source->offset;
    char groups[SourceDepth], quote=0;
    unsigned depth=0; bool escaped=false;
    *assignment=UINT32_MAX; *colon=UINT32_MAX; *opens=false;
    for (uint32_t i=0;i<source->length;++i) {
        char c=text[i];
        if (quote) {
            if (escaped) escaped=false;
            else if (c=='\\') escaped=true;
            else if (c==quote) quote=0;
            continue;
        }
        if (c=='\"' || c=='\'') { quote=c; continue; }
        if (c=='#') { source->length=i; break; }
        if (c==':' && !depth && *colon==UINT32_MAX) *colon=i;
        if (c=='(' || c=='[') {
            if (depth==SourceDepth) return fail(diagnostic,*source,"Grouping depth exceeds fixed capacity");
            groups[depth++]=c; continue;
        }
        if (c==')' || c==']') {
            if (!depth || groups[--depth]!=(c==')'?'(':'['))
                return fail(diagnostic,slice(*source,i,i+1),"Unmatched closing delimiter");
            continue;
        }
        if (c=='=' && !depth && (i==0 || (text[i-1]!='=' && text[i-1]!='!' && text[i-1]!='<' && text[i-1]!='>')) &&
            (i+1==source->length || text[i+1]!='=')) {
            if (*assignment!=UINT32_MAX) return fail(diagnostic,*source,"Multiple statement assignments are not supported");
            *assignment=i;
        }
    }
    if (quote || depth) return fail(diagnostic,*source,"Unterminated string or grouping");
    *source=trimmed(tree,*source);
    *opens=source->length && tree->source[source->offset+source->length-1]==':';
    return true;
}

static bool executable(unsigned kind)
{
    return kind==NativeScriptSource_Function || kind==NativeScriptSource_Callback ||
        kind==NativeScriptSource_If || kind==NativeScriptSource_ElseIf ||
        kind==NativeScriptSource_Else || kind==NativeScriptSource_While;
}

static bool parse_line(const NativeScriptSourceTree* tree, unsigned parent_kind,
                        NativeScriptSourceNode* node, NativeScriptDiagnostic* diagnostic)
{
    uint32_t assign, colon, at=0; bool opens;
    if (!delimiters(tree,&node->source,&assign,&colon,&opens,diagnostic)) return false;
    NativeScriptSourceSpan span=node->source;
    node->opens=opens;
    NativeScriptSourceSpan first=word(tree,span,&at);
    if (!first.length) return fail(diagnostic,span,"Expected declaration or statement");
    bool code=executable(parent_kind);
#define IS(name) nativeScriptSource_SpanEquals(tree,first,name)
    if (code) {
        if (IS("if") || IS("elif") || IS("while")) {
            if (colon==UINT32_MAX) return fail(diagnostic,span,"Control-flow header needs ':'");
            node->opens=true;
            node->kind=IS("if")?NativeScriptSource_If:IS("elif")?NativeScriptSource_ElseIf:NativeScriptSource_While;
            node->value=trimmed(tree,slice(span,at,colon));
            node->inline_body=trimmed(tree,slice(span,colon+1,span.length));
            if (!node->value.length) return fail(diagnostic,span,"Missing control-flow condition");
        } else if (IS("else")) {
            if (colon!=4) return fail(diagnostic,span,"Expected else:");
            node->opens=true;
            node->kind=NativeScriptSource_Else;
            node->inline_body=trimmed(tree,slice(span,colon+1,span.length));
        } else if (IS("let")) {
            node->kind=NativeScriptSource_Local;
            NativeScriptSourceSpan type=word(tree,span,&at);
            node->value_type=(uint8_t)scalar(tree,type);
            node->name=node->value_type?word(tree,span,&at):type;
            if (!identifier(tree,node->name) || assign==UINT32_MAX || assign<at)
                return fail(diagnostic,span,"Local needs a type, name and initializer");
            while (at<assign && isspace((unsigned char)tree->source[span.offset+at])) ++at;
            if (at!=assign) return fail(diagnostic,span,"Unexpected tokens in local declaration");
            node->value=trimmed(tree,slice(span,assign+1,span.length));
        } else if (IS("return")) {
            node->kind=NativeScriptSource_Return;
            node->value=trimmed(tree,slice(span,at,span.length));
        } else if (IS("break") || IS("continue") || IS("pass")) {
            if (at!=span.length) return fail(diagnostic,span,"Unexpected text after control statement");
            node->kind=IS("break")?NativeScriptSource_Break:IS("continue")?NativeScriptSource_Continue:NativeScriptSource_Pass;
        } else if (assign!=UINT32_MAX) {
            node->kind=NativeScriptSource_Assign;
            node->target=trimmed(tree,slice(span,0,assign));
            node->value=trimmed(tree,slice(span,assign+1,span.length));
        } else {
            if (opens) return fail(diagnostic,span,"Unknown executable block header");
            node->kind=NativeScriptSource_Expression; node->value=span;
        }
        if ((node->kind==NativeScriptSource_Local || node->kind==NativeScriptSource_Assign) && !node->value.length)
            return fail(diagnostic,span,"Missing assignment value");
    } else if (IS("export") || IS("func")) {
        node->exported=IS("export");
        if (node->exported && !nativeScriptSource_SpanEquals(tree,word(tree,span,&at),"func"))
            return fail(diagnostic,span,"Expected exported function declaration");
        node->kind=NativeScriptSource_Function;node->name=word(tree,span,&at);
        if (!opens || !identifier(tree,node->name)) return fail(diagnostic,span,"Function needs name, signature and ':'");
        while (at<span.length && isspace((unsigned char)tree->source[span.offset+at])) ++at;
        if (at>=span.length || tree->source[span.offset+at]!='(') return fail(diagnostic,span,"Missing function signature");
        node->value=slice(span,at,span.length-1);
    } else if (IS("on")) {
        if (!opens) return fail(diagnostic,span,"Callback needs ':'");
        node->kind=NativeScriptSource_Callback;
        node->name=trimmed(tree,slice(span,at,span.length-1));
        if (!node->name.length) return fail(diagnostic,span,"Missing callback name");
    } else if (IS("fighter") || IS("move") || IS("action") || IS("article") || IS("item") || IS("model_effect")) {
        if (!opens) return fail(diagnostic,span,"Definition needs ':'");
        node->kind=IS("fighter")?NativeScriptSource_Fighter:IS("move")?NativeScriptSource_Move:
            IS("action")?NativeScriptSource_Action:IS("article")?NativeScriptSource_Article:
            IS("item")?NativeScriptSource_Item:NativeScriptSource_Effect;
        if (node->kind!=NativeScriptSource_Fighter) node->name=word(tree,span,&at);
        if (node->kind!=NativeScriptSource_Fighter && !identifier(tree,node->name)) return fail(diagnostic,span,"Missing definition name");
        node->value=trimmed(tree,slice(span,at,span.length-1));
    } else if (IS("state") || IS("parameter") || IS("attribute") || IS("table")) {
        NativeScriptSourceSpan type=word(tree,span,&at);
        node->value_type=(uint8_t)scalar(tree,type);
        if (node->value_type==NativeScriptType_U32 && (IS("state") || IS("attribute")))
            return fail(diagnostic,type,IS("state") ? "Unsigned owned state is not supported by the storage contract" :
                "Imported uint attributes need an explicit word-width contract; attribute byte remains one byte");
        if (IS("state") && !node->value_type) {
            if (!opens || parent_kind!=NativeScriptSource_Article) return fail(diagnostic,span,"Invalid article state declaration");
            node->kind=NativeScriptSource_ArticleState; node->value=trimmed(tree,slice(span,first.length,span.length-1));
            // A state alias supplies metadata and intentionally has no body.
            const char* value=tree->source+node->value.offset;
            for (uint32_t i=0;i+5<=node->value.length;++i)
                if (memcmp(value+i," use ",5)==0) node->opens=false;
        } else {
            if (nativeScriptSource_SpanEquals(tree,type,"byte") && IS("attribute")) node->value_type=NativeScriptType_U32;
            if (!node->value_type) return fail(diagnostic,type,"Unknown declared value type");
            node->kind=IS("state")?NativeScriptSource_State:IS("parameter")?NativeScriptSource_Parameter:
                IS("table")?NativeScriptSource_Table:NativeScriptSource_Attribute;
            node->name=word(tree,span,&at);
            if (!identifier(tree,node->name)) return fail(diagnostic,span,"Missing value name");
            node->value=trimmed(tree,slice(span,at,span.length));
            if (opens) return fail(diagnostic,span,"Value declaration cannot open a block");
        }
    } else {
        // Metadata is preserved for the separate typed declaration/binding
        // compiler. It must not be interpreted as executable unknown syntax.
        node->kind=NativeScriptSource_Metadata; node->name=first;
        node->value=trimmed(tree,slice(span,at,span.length-(opens?1:0)));
    }
#undef IS
    return true;
}

bool nativeScriptSource_Parse(NativeScriptSourceTree* tree, NativeScriptDiagnostic* diagnostic)
{
    if (diagnostic) memset(diagnostic,0,sizeof(*diagnostic));
    if (!tree || !tree->source || !tree->nodes || tree->capacity==0)
        return fail(diagnostic,(NativeScriptSourceSpan){0},"Missing fixed source storage");
    tree->count=1;
    tree->nodes[0]=(NativeScriptSourceNode){.parent=UINT32_MAX,.first_child=UINT32_MAX,
        .last_child=UINT32_MAX,.next=UINT32_MAX,.kind=NativeScriptSource_Root,
        .opens=true,.child_indent=UINT16_MAX,.source={0,tree->source_bytes,1,1}};
    uint32_t parents[SourceDepth]={0}; unsigned depth=1;
    uint32_t offset=0,line=1,last=UINT32_MAX;
    while (offset<tree->source_bytes) {
        uint32_t end=offset;
        while (end<tree->source_bytes && tree->source[end]!='\n') ++end;
        uint32_t first=offset;
        while (first<end && tree->source[first]==' ') ++first;
        uint32_t content_end=end;
        if (first<end && tree->source[first]=='\t') goto indentation_failure;
        if (content_end>first && tree->source[content_end-1]=='\r') --content_end;
        if (first==content_end || tree->source[first]=='#') goto next_line;
        uint32_t indent=first-offset;
        if (indent>UINT16_MAX || indent%4) goto indentation_failure;
        NativeScriptSourceSpan span={first,content_end-first,line,indent+1};
        if (last!=UINT32_MAX && indent>tree->nodes[last].indent && tree->nodes[last].opens) {
            if (depth==SourceDepth) { tree->count=0; return fail(diagnostic,span,"Source block depth exceeds fixed capacity"); }
            parents[depth++]=last;
        }
        while (depth>1 && indent<=tree->nodes[parents[depth-1]].indent) --depth;
        NativeScriptSourceNode* parent=&tree->nodes[parents[depth-1]];
        if ((depth==1 && indent!=0) || (parent->child_indent!=UINT16_MAX && parent->child_indent!=indent))
            goto indentation_failure;
        if (last!=UINT32_MAX && tree->nodes[last].opens && tree->nodes[last].kind!=NativeScriptSource_ArticleState &&
            tree->nodes[last].first_child==UINT32_MAX && indent<=tree->nodes[last].indent) {
            tree->count=0; return fail(diagnostic,tree->nodes[last].source,"Empty block needs an explicit pass");
        }
        if (tree->count==tree->capacity) { tree->count=0; return fail(diagnostic,span,"Source node capacity exceeded"); }
        NativeScriptSourceNode node={.source=span,.parent=parents[depth-1],
            .first_child=UINT32_MAX,.last_child=UINT32_MAX,.next=UINT32_MAX,
            .expression=UINT32_MAX,.target_expression=UINT32_MAX,
            .indent=(uint16_t)indent,.child_indent=UINT16_MAX};
        if (!parse_line(tree,parent->kind,&node,diagnostic)) {tree->count=0;return false;}
        if ((node.kind==NativeScriptSource_Else || node.kind==NativeScriptSource_ElseIf) &&
            (parent->last_child==UINT32_MAX || (tree->nodes[parent->last_child].kind!=NativeScriptSource_If &&
             tree->nodes[parent->last_child].kind!=NativeScriptSource_ElseIf))) {
            tree->count=0;return fail(diagnostic,span,"Else must follow an if or elif at the same indentation");
        }
        if (parent->first_child==UINT32_MAX) parent->first_child=tree->count;
        else tree->nodes[parent->last_child].next=tree->count;
        parent->last_child=tree->count;parent->child_indent=(uint16_t)indent;
        last=tree->count;tree->nodes[tree->count++]=node;
        if (node.inline_body.length) {
            if (tree->count==tree->capacity) { tree->count=0; return fail(diagnostic,span,"Source node capacity exceeded"); }
            NativeScriptSourceNode child={.source=node.inline_body,.parent=last,
                .first_child=UINT32_MAX,.last_child=UINT32_MAX,.next=UINT32_MAX,
                .expression=UINT32_MAX,.target_expression=UINT32_MAX,
                .indent=(uint16_t)(indent+4),.child_indent=UINT16_MAX};
            if (!parse_line(tree,node.kind,&child,diagnostic)) {tree->count=0;return false;}
            if (child.opens) {tree->count=0;return fail(diagnostic,child.source,"Nested inline blocks need explicit lines");}
            tree->nodes[last].first_child=tree->nodes[last].last_child=tree->count;
            tree->nodes[last].child_indent=(uint16_t)(indent+4);
            last=tree->count;tree->nodes[tree->count++]=child;
        }
next_line:
        offset=end+(end<tree->source_bytes);++line;continue;
indentation_failure:
        tree->count=0; return fail(diagnostic,(NativeScriptSourceSpan){first,content_end-first,line,first-offset+1},"Invalid or inconsistent indentation");
    }
    if (last!=UINT32_MAX && tree->nodes[last].opens && tree->nodes[last].kind!=NativeScriptSource_ArticleState &&
        tree->nodes[last].first_child==UINT32_MAX) {
        tree->count=0; return fail(diagnostic,tree->nodes[last].source,"Empty block needs an explicit pass");
    }
    return true;
}

const char* nativeScriptSource_KindName(NativeScriptSourceKind kind)
{
    static const char* names[]={"root","fighter","move","action","article","item","effect",
        "function","callback","article-state","state","parameter","attribute","table","metadata",
        "if","elif","else","while","local","assignment","expression","return","break","continue","pass"};
    return (unsigned)kind<sizeof(names)/sizeof(*names)?names[kind]:"invalid";
}
