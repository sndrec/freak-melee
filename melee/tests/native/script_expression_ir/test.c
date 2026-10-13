#include <melee/ft/native_script_expression.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#undef NDEBUG
static unsigned checks, programs;
#define CHECK(value) do { ++checks; if (!(value)) { fprintf(stderr,"Expression check failed %d: %s\n",__LINE__,#value); return false; } } while (0)

enum { Namespace = 1, FloatArray, BoolArray, ResourceSelector, Resource,
       Position, ReadPosition, CallInt, CallFloat, CallBool, IntCast, UIntCast,
       Measure, Absolute, Callback, Cleanup, JointSelector, Joint };

static bool path(const NativeScriptIRArena* arena, const NativeScriptIRNode* node,
                 char* output, size_t capacity)
{
    if (node->kind == NativeScriptIR_Name) {
        if (node->text_length >= capacity) return false;
        memcpy(output, arena->source + node->text_offset, node->text_length);
        output[node->text_length] = 0; return true;
    }
    if (node->kind == NativeScriptIR_Call) return path(arena,&arena->nodes[node->a],output,capacity);
    if (node->kind != NativeScriptIR_Member) return false;
    char prefix[160];
    if (!path(arena,&arena->nodes[node->a],prefix,sizeof(prefix))) return false;
    int size=snprintf(output,capacity,"%s.%.*s",prefix,(int)node->text_length,arena->source+node->text_offset);
    return size>=0&&(size_t)size<capacity;
}

static bool semantic_error(NativeScriptDiagnostic* error, const NativeScriptIRNode* node, const char* message)
{
    error->source=node->source;snprintf(error->message,sizeof(error->message),"%s",message);return false;
}

static bool resolve(void* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result,
    NativeScriptDiagnostic* error)
{
    (void)context;
    const unsigned rw=NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable;
    if (node->kind == NativeScriptIR_Index) {
        unsigned semantic=arena->nodes[node->a].semantic;
        if(semantic!=FloatArray&&semantic!=BoolArray)return semantic_error(error,node,"Not an array.");
        *result=(NativeScriptIRResolution){semantic==FloatArray?NativeScriptType_F32:NativeScriptType_Bool,semantic,rw,0};return true;
    }
    if(node->kind==NativeScriptIR_Call) {
        const NativeScriptIRNode* callee=&arena->nodes[node->a];
        const uint32_t* arguments=arena->arguments+node->argument_first;
        switch(callee->semantic) {
        case ResourceSelector:
            if(node->argument_count!=1||!nativeScriptExpression_IsIntegral(arena->nodes[arguments[0]].type))return semantic_error(error,node,"Resource index must be one Int.");
            *result=(NativeScriptIRResolution){NativeScriptType_Object,Resource,NativeScriptFlag_Readable,0};return true;
        case JointSelector:
            if(node->argument_count!=1||arena->nodes[arguments[0]].type!=NativeScriptType_Text)return semantic_error(error,node,"Joint selector needs a named path.");
            *result=(NativeScriptIRResolution){NativeScriptType_Object,Joint,NativeScriptFlag_Readable,0};return true;
        case ReadPosition:
            if(node->argument_count!=4||!nativeScriptExpression_IsIntegral(arena->nodes[arguments[0]].type))return semantic_error(error,node,"Read-position signature mismatch.");
            for(unsigned i=1;i<4;++i) {
                const NativeScriptIRNode* arg=&arena->nodes[arguments[i]];
                if(arg->type!=NativeScriptType_F32||(arg->flags&rw)!=rw)return semantic_error(error,arg,"Position output must be writable Float.");
                for(unsigned j=1;j<i;++j)if(arg->semantic==arena->nodes[arguments[j]].semantic)return semantic_error(error,arg,"Position outputs overlap.");
            }
            *result=(NativeScriptIRResolution){NativeScriptType_Bool,ReadPosition,NativeScriptFlag_Readable,0};return true;
        case CallInt:case CallFloat:case CallBool:
            if(node->argument_count!=2||!nativeScriptExpression_IsIntegral(arena->nodes[arguments[0]].type)||arena->nodes[arguments[1]].type!=NativeScriptType_Text)return semantic_error(error,node,"Recipient call requires Int ID and Text name.");
            *result=(NativeScriptIRResolution){callee->semantic==CallInt?NativeScriptType_I32:callee->semantic==CallFloat?NativeScriptType_F32:NativeScriptType_Bool,callee->semantic,NativeScriptFlag_Readable|NativeScriptFlag_MayInvalidateSource,0};return true;
        case IntCast:case UIntCast:case Absolute:
            if(node->argument_count!=1||!nativeScriptExpression_IsNumeric(arena->nodes[arguments[0]].type))return semantic_error(error,node,"Scalar call signature mismatch.");
            *result=(NativeScriptIRResolution){callee->semantic==IntCast?NativeScriptType_I32:callee->semantic==UIntCast?NativeScriptType_U32:NativeScriptType_F32,callee->semantic,NativeScriptFlag_Readable,0};return true;
        case Measure:
            if(node->argument_count!=6)return semantic_error(error,node,"Measure needs six coordinates.");
            for(unsigned i=0;i<6;++i)if(!nativeScriptExpression_Assignable(NativeScriptType_F32,arena->nodes[arguments[i]].type))return semantic_error(error,node,"Measure coordinate must be numeric.");
            *result=(NativeScriptIRResolution){NativeScriptType_F32,Measure,NativeScriptFlag_Readable,0};return true;
        case Callback:
            if(node->argument_count!=1||(arena->nodes[arguments[0]].type!=NativeScriptType_Null&&arena->nodes[arguments[0]].type!=NativeScriptType_Callable))return semantic_error(error,node,"Callback must be a helper or None.");
            *result=(NativeScriptIRResolution){NativeScriptType_Void,Callback,0,0};return true;
        default:return semantic_error(error,node,"Unknown call target.");
        }
    }
    char name[192];if(!path(arena,node,name,sizeof(name)))return semantic_error(error,node,"Unrecognized selector.");
    static const char* spaces[]={"state","item","item.state","input","attrs","facing","targets","position","callbacks","bones"};
    for(unsigned i=0;i<sizeof(spaces)/sizeof(*spaces);++i)if(!strcmp(name,spaces[i])){
        *result=(NativeScriptIRResolution){NativeScriptType_Namespace,Namespace,0,0};return true;
    }
    static const struct {const char* name;NativeScriptIRType type;unsigned semantic;} scalars[]={
        {"node",NativeScriptType_I32,100},{"target",NativeScriptType_I32,101},
        {"state.handle",NativeScriptType_I32,102},{"item.creator",NativeScriptType_I32,103},
        {"state.x",NativeScriptType_F32,104},{"state.y",NativeScriptType_F32,105},{"state.z",NativeScriptType_F32,106},
        {"state.copy_controls",NativeScriptType_Bool,107},{"input.z_held",NativeScriptType_Bool,108},
        {"input.x",NativeScriptType_F32,109},{"position.x",NativeScriptType_F32,110},
        {"position.y",NativeScriptType_F32,111},{"facing.direction",NativeScriptType_F32,112},
        {"attrs.maximum_length",NativeScriptType_F32,113},{"attrs.threshold",NativeScriptType_F32,114},
        {"item.state.tail",NativeScriptType_I32,115},
        {"item.resource.position",NativeScriptType_Object,Position},
        {"item.resource.position.x",NativeScriptType_F32,116},
        {"item.resource.position.y",NativeScriptType_F32,117},
        {"item.resource.position.z",NativeScriptType_F32,118},
        {"item.state.matrix",NativeScriptType_Object,FloatArray},
        {"item.state.active",NativeScriptType_Object,BoolArray},
        {"bones.joint.rotation",NativeScriptType_Object,Position},
        {"bones.joint.rotation.x",NativeScriptType_F32,119},
    };
    for(unsigned i=0;i<sizeof(scalars)/sizeof(*scalars);++i)if(!strcmp(name,scalars[i].name)){
        *result=(NativeScriptIRResolution){scalars[i].type,scalars[i].semantic,rw,0};return true;
    }
    static const struct {const char* name;unsigned semantic;} calls[]={
        {"item.resource",ResourceSelector},{"targets.read_position",ReadPosition},
        {"targets.call_int",CallInt},{"targets.call_float",CallFloat},{"targets.call_bool",CallBool},
        {"int",IntCast},{"uint",UIntCast},{"abs",Absolute},{"Measure",Measure},
        {"callbacks.damage_secondary",Callback},{"Cleanup",Cleanup},{"bones.joint",JointSelector},
    };
    for(unsigned i=0;i<sizeof(calls)/sizeof(*calls);++i)if(!strcmp(name,calls[i].name)){
        *result=(NativeScriptIRResolution){NativeScriptType_Callable,calls[i].semantic,NativeScriptFlag_Callable|NativeScriptFlag_Readable,0};return true;
    }
    return semantic_error(error,node,"Unknown semantic name.");
}

static NativeScriptIRNode nodes[4096];static uint32_t arguments[2048];
static NativeScriptIRArena arena;
static uint32_t expression;
static NativeScriptDiagnostic diagnostic;
static bool parse(const char* text,bool expected)
{
    ++programs;memset(nodes,0,sizeof(nodes));memset(arguments,0,sizeof(arguments));
    arena=(NativeScriptIRArena){text,(uint32_t)strlen(text),nodes,0,4096,arguments,0,2048};
    bool ok=nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,arena.source_bytes,71,5},resolve,NULL,&expression,&diagnostic);
    if(ok!=expected)fprintf(stderr,"Unexpected expression result '%s': %s\n",text,diagnostic.message);
    CHECK(ok==expected);
    if(!ok){CHECK(expression==NATIVE_SCRIPT_NODE_NONE&&arena.node_count==0&&arena.argument_count==0&&diagnostic.message[0]);}
    return true;
}

static bool suite(void)
{
    CHECK(parse("0.95 * position.x + 0.05 * state.x",true));
    CHECK(nodes[expression].type==NativeScriptType_F64&&nodes[expression].operation==NativeScriptOp_Add);
    uint32_t converted;CHECK(nativeScriptExpression_Convert(&arena,expression,NativeScriptType_F32,false,&converted,&diagnostic));
    CHECK(nodes[converted].kind==NativeScriptIR_Convert&&nodes[converted].a==expression);
    CHECK(parse("input.x * attrs.maximum_length",true));CHECK(nodes[expression].type==NativeScriptType_F32);
    CHECK(parse("0x80000001",true));CHECK(nodes[expression].type==NativeScriptType_U32&&nodes[expression].bits==UINT32_C(0x80000001));
    CHECK(parse("0xFFFFFFFC & uint(state.handle)",true));CHECK(nodes[expression].type==NativeScriptType_U32);
    CHECK(parse("2147483648 + uint(state.handle)",true));CHECK(nodes[expression].type==NativeScriptType_I64);
    CHECK(parse("0xffffffffffffffff",true));CHECK(nodes[expression].type==NativeScriptType_U64);
    CHECK(parse("1 + 2 * 3 << 1 | 8",true));CHECK(nodes[expression].operation==NativeScriptOp_BitOr);
    CHECK(nodes[nodes[expression].a].operation==NativeScriptOp_ShiftLeft);
    CHECK(parse("not state.copy_controls or input.z_held and node < item.state.tail",true));
    CHECK(nodes[expression].type==NativeScriptType_Bool&&(nodes[expression].flags&NativeScriptFlag_ShortCircuit));
    CHECK(nodes[nodes[expression].b].operation==NativeScriptOp_And);
    CHECK(parse("item.resource(node + 1).position.x - item.resource(node).position.y",true));
    CHECK(nodes[expression].type==NativeScriptType_F32&&arena.argument_count==2);
    CHECK(parse("item.state.active[node - 1] and node <= item.state.tail",true));
    CHECK(parse("item.state.matrix[4]",true));CHECK(nodes[expression].type==NativeScriptType_F32&&(nodes[expression].flags&NativeScriptFlag_Writable));
    CHECK(parse("Measure(item.resource(node).position.x, state.y, state.z, 0, 0, 0)",true));
    CHECK(nodes[expression].argument_count==6&&arena.argument_count==7);
    CHECK(parse("targets.call_int(item.creator, \"linked_select\")",true));
    CHECK(nodes[expression].type==NativeScriptType_I32&&(nodes[expression].flags&NativeScriptFlag_MayInvalidateSource));
    CHECK(parse("not targets.call_bool(target, \"released\") and input.z_held",true));
    CHECK((nodes[expression].flags&(NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_ShortCircuit))==(NativeScriptFlag_MayInvalidateSource|NativeScriptFlag_ShortCircuit));
    CHECK(nodes[nodes[expression].a].kind==NativeScriptIR_Unary&&(nodes[nodes[expression].a].flags&NativeScriptFlag_MayInvalidateSource));
    CHECK(parse("targets.call_float(target, \"linked_x\") + 0.05 * state.x",true));
    CHECK(nodes[expression].type==NativeScriptType_F64&&(nodes[expression].flags&NativeScriptFlag_MayInvalidateSource));
    CHECK(nativeScriptExpression_Convert(&arena,expression,NativeScriptType_F32,false,&converted,&diagnostic));
    CHECK(nodes[converted].flags&NativeScriptFlag_MayInvalidateSource);
    CHECK(parse("item.resource(targets.call_int(target, \"node\")).position.x",true));
    CHECK(nodes[expression].kind==NativeScriptIR_Member&&(nodes[expression].flags&NativeScriptFlag_MayInvalidateSource));
    CHECK(parse("item.state.matrix[targets.call_int(target, \"node\")]",true));
    CHECK(nodes[expression].kind==NativeScriptIR_Index&&(nodes[expression].flags&NativeScriptFlag_MayInvalidateSource));
    CHECK(parse("Measure(targets.call_float(target, \"linked_x\"), state.y, state.z, 0, 0, 0)",true));
    CHECK(nodes[expression].kind==NativeScriptIR_Call&&(nodes[expression].flags&NativeScriptFlag_MayInvalidateSource));
    CHECK(parse("targets.read_position(target, state.x, state.y, state.z)",true));
    CHECK(parse("callbacks.damage_secondary(None)",true));CHECK(nodes[expression].type==NativeScriptType_Void);
    CHECK(parse("callbacks.damage_secondary(Cleanup)",true));
    CHECK(parse("bones.joint(\"root/hand\").rotation.x",true));
    CHECK(parse("(state.x + state.y) * -state.z # authored trailing comment",true));
    CHECK(parse("0x1.fp-2 + .5e+1",true));double expected=5.0,actual;memcpy(&actual,&nodes[nodes[expression].b].bits,8);CHECK(actual==expected);
    const char* invalid[]={"", "state.", "unknown", "state.x +", "(state.x", "state.x)", "item.resource()", "item.resource(1,2)",
        "item.resource(1.25)", "item.state.matrix[0.5]", "item.state.matrix[]", "Measure(1,2)",
        "targets.call_int(1.0,\"Name\")", "targets.read_position(1, state.x, state.x, state.z)",
        "targets.read_position(1, 2.0, state.y, state.z)","callbacks.damage_secondary(1)",
        "state.x & 1", "state.x % 2", "not \"text\"", "state.x + None", "state.x = 1", "true ? 1 : 2",
        "0x", "08", "1e", "1e9999", "1.0f", "18446744073709551616", "0x10000000000000000",
        "\"unterminated", "\"unknown\\q\"", "state.x;", "item.resource(1,)", "item.resource(,1)", "and true"};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)CHECK(parse(invalid[i],false));
    CHECK(!nativeScriptExpression_Assignable(NativeScriptType_Bool,NativeScriptType_I32));
    CHECK(!nativeScriptExpression_Assignable(NativeScriptType_I32,NativeScriptType_F32));
    CHECK(parse("state.x",true));CHECK(!nativeScriptExpression_Convert(&arena,expression,NativeScriptType_I32,false,&converted,&diagnostic));
    CHECK(nativeScriptExpression_Convert(&arena,expression,NativeScriptType_I32,true,&converted,&diagnostic));
    CHECK(!(nodes[converted].flags&NativeScriptFlag_Addressable));
    arena.node_capacity=arena.node_count;uint32_t before=arena.node_count;
    CHECK(!nativeScriptExpression_Parse(&arena,(NativeScriptSourceSpan){0,7,1,1},resolve,NULL,&converted,&diagnostic));CHECK(arena.node_count==before);
    char nested[160];memset(nested,'(',70);nested[70]='1';memset(nested+71,')',70);nested[141]=0;CHECK(parse(nested,false));
    NativeScriptExpressionToken tokens[32];uint32_t count;
    const char* lexical="target = item.resource(2).position.x # comment";
    CHECK(nativeScriptExpression_Tokenize(lexical,(uint32_t)strlen(lexical),(NativeScriptSourceSpan){0,(uint32_t)strlen(lexical),12,9},tokens,32,&count,&diagnostic));
    CHECK(tokens[1].kind==NativeScriptToken_Assign&&tokens[count-1].kind==NativeScriptToken_End);
    CHECK(tokens[0].source.line==12&&tokens[0].source.column==9);
    CHECK(!nativeScriptExpression_Tokenize(lexical,(uint32_t)strlen(lexical),(NativeScriptSourceSpan){0,(uint32_t)strlen(lexical),1,1},tokens,2,&count,&diagnostic)&&count==0);
    return true;
}

int main(void)
{
    if(!suite())return 1;
    printf("{\"checks\":%u,\"expressions\":%u,\"status\":\"PASS direct typed semantic expression IR\"}\n",checks,programs);
    return 0;
}
