#include "native_script_binding_signal_resolve.h"
#include "native_script_events.h"
#include "native_script_expression.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return -1;}
static bool bounded(const NativeScriptIRArena* a,const NativeScriptIRNode* n)
{return n->text_offset<=a->source_bytes&&n->text_length<=a->source_bytes-n->text_offset;}
static bool identifier(const NativeScriptIRArena* a,const NativeScriptIRNode* n)
{
    if(!bounded(a,n)||!n->text_length)return false;
    const char* s=a->source+n->text_offset;
    if(!isalpha((unsigned char)s[0])&&s[0]!='_')return false;
    for(uint32_t i=1;i<n->text_length;++i)if(!isalnum((unsigned char)s[i])&&s[i]!='_')return false;
    return true;
}
int nativeScriptBindingSignal_TryResolve(void* opaque,const NativeScriptIRArena* a,
    const NativeScriptIRNode* n,NativeScriptIRResolution* output,NativeScriptDiagnostic* d)
{
    if(!a||!n||n->kind!=NativeScriptIR_Call||n->a>=a->node_count)return 0;
    uint32_t base=a->nodes[n->a].semantic;
    bool self=base==NativeScriptBinding_ItemSignal;
    bool owned=base==NativeScriptBinding_ArticlesSignalOwned;
    if(!self&&!owned&&base!=NativeScriptBinding_ArticlesSignal)return 0;
    const NativeScriptBindingSpawnResolverContext* c=opaque;
    const NativeScriptProgramIndex* p=c?c->program:NULL;
    const NativeScriptSourceTree* tree=p?p->tree:NULL;
    if(!output||!tree||!a->source||p->expressions!=a||tree->source!=a->source||tree->source_bytes!=a->source_bytes)
        return error(d,n->source,"Signal call needs its exact indexed body program and source arena");
    if((self&&(c->bindings.context_mask!=NativeScriptBindings_Article||!c->owner||c->owner>=tree->count||tree->nodes[c->owner].kind!=NativeScriptSource_Article))||
       (!self&&!((c->bindings.context_mask==NativeScriptBindings_Fighter&&!c->owner)||
            (!owned&&c->bindings.context_mask==NativeScriptBindings_Article&&c->owner&&c->owner<tree->count&&tree->nodes[c->owner].kind==NativeScriptSource_Article))))
        return error(d,n->source,"Signal call has unsupported source ownership");
    if(n->argument_count!=(self?2u:4u)||n->argument_first>a->argument_count||n->argument_count>a->argument_count-n->argument_first)
        return error(d,n->source,"Signal call argument count/range is invalid");
    for(uint32_t i=0;i<n->argument_count;++i)if(a->arguments[n->argument_first+i]>=a->node_count)
        return error(d,n->source,"Signal argument node is out of range");
    uint32_t owner=c->owner;
    if(!self) {
        const NativeScriptIRNode* declared=&a->nodes[a->arguments[n->argument_first+1]];
        if(declared->kind!=NativeScriptIR_Integer||declared->type!=NativeScriptType_I32||!identifier(a,declared))
            return error(d,declared->source,"Target signal requires a declared Article identifier literal");
        NativeScriptIRNode name=*declared;name.kind=NativeScriptIR_Name;
        NativeScriptIRResolution selected;
        int found=nativeScriptBindingSpawn_TryResolve((void*)c,a,&name,&selected,d);
        if(found<0)return -1;
        if(!found||selected.type!=NativeScriptType_I32||selected.flags!=NativeScriptFlag_Literal||
           selected.literal_bits>=tree->count||!selected.literal_bits)
            return error(d,declared->source,"Target signal Article declaration is absent");
        owner=(uint32_t)selected.literal_bits;
    }
    const NativeScriptIRNode* name=&a->nodes[a->arguments[n->argument_first+(self?0:2)]];
    if(name->kind!=NativeScriptIR_String||name->type!=NativeScriptType_Text||!identifier(a,name))
        return error(d,name->source,"Signal name must be a literal identifier without escapes");
    uint32_t found=UINT32_MAX;
    for(uint32_t f=0;f<p->function_count;++f) {
        const NativeScriptFunctionDeclaration* fn=&p->functions[f];
        if(!fn->callback||fn->owner!=owner)continue;
        if(fn->source_node>=tree->count)return error(d,fn->name,"Signal callback source node is invalid");
        NativeScriptEventResolution event;
        if(!nativeScriptEvents_Resolve(tree,&tree->nodes[fn->source_node],&event,d))return -1;
        if(event.descriptor->kind!=NativeScriptEvent_ArticleSignal||event.owner_node!=owner||event.signal_name.length!=name->text_length||
           memcmp(tree->source+event.signal_name.offset,a->source+name->text_offset,name->text_length))continue;
        if(found!=UINT32_MAX)return error(d,name->source,"Signal name is ambiguous within the declared Article");
        if(!fn->signature_resolved||fn->result_type!=NativeScriptType_Void||fn->parameter_count!=1||
           fn->parameter_first>=p->parameter_count||p->parameters[fn->parameter_first].type!=NativeScriptType_F32||
           !event.descriptor->signature_supported||event.descriptor->result_type!=NativeScriptType_Void||
           event.descriptor->argument_count!=1||event.descriptor->arguments[0].type!=NativeScriptType_F32)
            return error(d,fn->name,"Signal handler must use the audited Void/one-F32 event signature");
        found=f;
    }
    if(found==UINT32_MAX)return error(d,name->source,"Signal is not declared by the selected Article");
    if(found>NATIVE_SCRIPT_SIGNAL_FUNCTION_MASK)return error(d,name->source,"Signal function identity exceeds its semantic capacity");
    uint32_t semantic=NATIVE_SCRIPT_SIGNAL_DOMAIN|((self?NativeScriptSignal_Self:owned?NativeScriptSignal_Owned:NativeScriptSignal_Target)<<20)|found;
    NativeScriptIRResolution resolved;
    if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,semantic,&resolved,d))return -1;
    *output=resolved;return 1;
}
