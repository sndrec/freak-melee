#include <melee/ft/native_script_events.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char source[1048576];static NativeScriptSourceNode nodes[16384];
static NativeScriptFunctionDeclaration functions[2048];static NativeScriptParameterDeclaration arguments[8192];
static NativeScriptValueDeclaration values[4096];static NativeScriptIRNode expressions[4096];static uint32_t expression_args[4096];

static void quote(const char* text,uint32_t count)
{
    putchar('"');for(uint32_t i=0;i<count;++i){unsigned char c=(unsigned char)text[i];
        if(c=='"'||c=='\\')printf("\\%c",c);else if(c<32)printf("\\u%04x",c);else putchar(c);}putchar('"');
}
static void span(const NativeScriptSourceTree* tree,NativeScriptSourceSpan value)
{quote(tree->source+value.offset,value.length);}
int main(int argc,char** argv)
{
    if(argc!=2)return 2;FILE* file=fopen(argv[1],"rb");if(!file)return 2;
    uint32_t bytes=(uint32_t)fread(source,1,sizeof(source),file);bool complete=feof(file);fclose(file);if(!complete)return 2;
    NativeScriptSourceTree tree={.source=source,.source_bytes=bytes,.nodes=nodes,.capacity=16384};NativeScriptDiagnostic diagnostic;
    if(!nativeScriptSource_Parse(&tree,&diagnostic)){fprintf(stderr,"parse line %u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    NativeScriptIRArena arena={.source=source,.source_bytes=bytes,.nodes=expressions,.node_capacity=4096,
        .arguments=expression_args,.argument_capacity=4096};
    NativeScriptProgramIndex index={.tree=&tree,.expressions=&arena,.functions=functions,.function_capacity=2048,
        .parameters=arguments,.parameter_capacity=8192,.values=values,.value_capacity=4096,.event_signature=nativeScriptEvents_Signature};
    if(!nativeScriptProgram_Index(&index,&diagnostic)){fprintf(stderr,"index line %u: %s\n",diagnostic.source.line,diagnostic.message);return 1;}
    printf("{\"source_nodes\":%u,\"functions\":%u,\"values\":%u,\"body_acceptance\":false,\"declarations\":[",tree.count,index.function_count,index.value_count);
    bool comma=false;
    for(uint32_t i=1;i<tree.count;++i) {
        NativeScriptSourceNode* node=&nodes[i];if(node->kind>=NativeScriptSource_If||node->kind==NativeScriptSource_Callback)continue;
        if(comma)putchar(',');comma=true;
        printf("{\"node\":%u,\"line\":%u,\"kind\":",i,node->source.line);
        const char* kind=nativeScriptSource_KindName(node->kind);quote(kind,(uint32_t)strlen(kind));
        printf(",\"parent\":%u,\"name\":",node->parent);span(&tree,node->name);printf(",\"value\":");span(&tree,node->value);putchar('}');
    }
    printf("],\"callbacks\":[");comma=false;
    for(uint32_t i=0;i<index.function_count;++i) {
        NativeScriptFunctionDeclaration* function=&functions[i];if(!function->callback)continue;
        NativeScriptSourceNode* callback=&nodes[function->source_node];NativeScriptEventResolution event;
        if(!nativeScriptEvents_Resolve(&tree,callback,&event,&diagnostic))return 1;
        const NativeScriptEventDescriptor* descriptor=event.descriptor;
        if(comma)putchar(',');comma=true;
        printf("{\"node\":%u,\"line\":%u,\"owner\":%u,\"event_id\":%u,\"name\":",function->source_node,callback->source.line,function->owner,event.event_id);
        span(&tree,callback->name);printf(",\"event_name\":");quote(descriptor->name,(uint32_t)strlen(descriptor->name));
        printf(",\"context\":%u,\"event_kind\":%u,\"argument_count\":%u,\"result\":",descriptor->context,descriptor->kind,descriptor->argument_count);
        const char* result=nativeScriptExpression_TypeName(descriptor->result_type);quote(result,(uint32_t)strlen(result));
        printf(",\"native_slot\":");quote(descriptor->native_slot,(uint32_t)strlen(descriptor->native_slot));printf(",\"source_evidence\":");
        quote(descriptor->source_evidence,(uint32_t)strlen(descriptor->source_evidence));putchar('}');
    }
    printf("]}\n");return 0;
}
