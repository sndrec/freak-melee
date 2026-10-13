#include "native_script_binding_article_reference_resolve.h"
#include "native_script_expression.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
static int error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return -1;}
static bool word(const NativeScriptIRArena* a,const NativeScriptIRNode* n,const char* text)
{return n->text_length==strlen(text)&&!memcmp(a->source+n->text_offset,text,n->text_length);}
static bool scalar(unsigned type)
{return type==NativeScriptType_Bool||type==NativeScriptType_I32||type==NativeScriptType_F32;}
static int out(NativeScriptIRResolution* r,unsigned type,uint32_t semantic,uint16_t flags)
{*r=(NativeScriptIRResolution){.type=(NativeScriptIRType)type,.semantic=semantic,.flags=flags};return 1;}
int nativeScriptBindingArticleReference_TryResolve(void* opaque,const NativeScriptIRArena* a,
    const NativeScriptIRNode* n,NativeScriptIRResolution* r,NativeScriptDiagnostic* d)
{
    if(!a||!n||n->a>=a->node_count)return 0;
    const NativeScriptIRNode* parent=&a->nodes[n->a];uint32_t base=parent->semantic;
    uint32_t category=base&NATIVE_SCRIPT_RECEIVER_CLASS_MASK;
    bool root=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ArticlesRef;
    bool translation=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ArticlesReadPrimaryTranslation;
    bool bone=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ArticlesReadBonePosition;
    bool state_namespace=category==NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE_NAMESPACE;
    bool position_namespace=category==NATIVE_SCRIPT_RECEIVER_ARTICLE_POSITION_NAMESPACE;
    bool normal_namespace=category==NATIVE_SCRIPT_RECEIVER_ARTICLE_NORMAL_NAMESPACE;
    if(!root&&!translation&&!bone&&!nativeScriptBindingArticleReference_Namespace(base)&&!nativeScriptBindingArticleReference_Handles(base))return 0;
    const NativeScriptBindingSpawnResolverContext* c=opaque;
    const NativeScriptProgramIndex* p=c?c->program:NULL;const NativeScriptSourceTree* tree=p?p->tree:NULL;
    if(!r||!tree||!a->source||p->expressions!=a||tree->source!=a->source||tree->source_bytes!=a->source_bytes||
       (c->bindings.context_mask!=NativeScriptBindings_Fighter&&c->bindings.context_mask!=NativeScriptBindings_Article)||
       n->text_offset>a->source_bytes||n->text_length>a->source_bytes-n->text_offset)
        return error(d,n->source,"Article reference needs its exact indexed source program and Fighter/Item scope");
    if(root||translation||bone) {
        if(n->argument_count!=(root?2u:bone?6u:5u)||n->argument_first>a->argument_count||n->argument_count>a->argument_count-n->argument_first)
            return error(d,n->source,"Article reference argument count/range is invalid");
        for(uint32_t i=0;i<n->argument_count;++i)if(a->arguments[n->argument_first+i]>=a->node_count)
            return error(d,n->source,"Article reference argument node is out of range");
        const NativeScriptIRNode* declaration=&a->nodes[a->arguments[n->argument_first+1]];
        if(declaration->kind!=NativeScriptIR_Integer||declaration->type!=NativeScriptType_I32||!declaration->text_length||
           declaration->text_offset>a->source_bytes||declaration->text_length>a->source_bytes-declaration->text_offset||
           (!isalpha((unsigned char)a->source[declaration->text_offset])&&a->source[declaration->text_offset]!='_'))
            return error(d,declaration->source,"Article reference requires a declared Article identifier literal");
        NativeScriptIRNode name=*declaration;name.kind=NativeScriptIR_Name;NativeScriptIRResolution selected;
        int found=nativeScriptBindingSpawn_TryResolve((void*)c,a,&name,&selected,d);if(found<0)return -1;
        if(!found||selected.type!=NativeScriptType_I32||selected.flags!=NativeScriptFlag_Literal||!selected.literal_bits||
           selected.literal_bits>=tree->count||selected.literal_bits>NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK)
            return error(d,declaration->source,"Article reference declaration is absent or exceeds semantic capacity");
        if(bone&&!nativeScriptExpression_IsIntegral(a->nodes[a->arguments[n->argument_first+2]].type))
            return error(d,n->source,"Article bone index must be integral");
        uint32_t semantic=(root?NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT:bone?NATIVE_SCRIPT_RECEIVER_ARTICLE_BONE_POSITION:NATIVE_SCRIPT_RECEIVER_ARTICLE_TRANSLATION)|(uint32_t)selected.literal_bits;
        NativeScriptIRResolution resolved;if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,semantic,&resolved,d))return -1;
        resolved.flags|=NativeScriptFlag_SourceOnly;
        if(root)resolved.flags|=NativeScriptFlag_ReceiverCapture;
        *r=resolved;return 1;
    }
    uint16_t flags=NativeScriptFlag_Readable|NativeScriptFlag_Pure|NativeScriptFlag_SourceOnly;
    uint32_t owner=base&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;
    if((category==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT||nativeScriptBindingArticleReference_Namespace(base))&&
       (!owner||owner>=tree->count||tree->nodes[owner].kind!=NativeScriptSource_Article))
        return error(d,n->source,"Article reference projection has an invalid declaration owner");
    if(n->kind==NativeScriptIR_Member&&category==NATIVE_SCRIPT_RECEIVER_ARTICLE_ROOT) {
        if(word(a,n,"exists"))return out(r,NativeScriptType_Bool,NATIVE_SCRIPT_RECEIVER_ARTICLE_EXISTS|owner,flags);
        if(word(a,n,"current_owner"))return out(r,NativeScriptType_U32,NATIVE_SCRIPT_RECEIVER_ARTICLE_OWNER|owner,flags);
        if(word(a,n,"state"))return out(r,NativeScriptType_Namespace,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE_NAMESPACE|owner,NativeScriptFlag_SourceOnly);
        if(word(a,n,"position")||word(a,n,"surface_normal")) {
            if(owner>(NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK>>3))return error(d,n->source,"Article physical projection exceeds declaration capacity");
            return out(r,NativeScriptType_Namespace,
                (word(a,n,"position")?NATIVE_SCRIPT_RECEIVER_ARTICLE_POSITION_NAMESPACE:NATIVE_SCRIPT_RECEIVER_ARTICLE_NORMAL_NAMESPACE)|owner,
                NativeScriptFlag_SourceOnly);
        }
    }
    if(n->kind==NativeScriptIR_Member&&(position_namespace||normal_namespace)) {
        uint32_t component=word(a,n,"x")?0:word(a,n,"y")?1:word(a,n,"z")?2:UINT32_MAX;
        if(component==UINT32_MAX||owner>(NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK>>3))
            return error(d,n->source,"Article physical vector requires x, y or z");
        return out(r,NativeScriptType_F32,NATIVE_SCRIPT_RECEIVER_ARTICLE_PHYSICAL|(owner<<3)|((normal_namespace?3:0)+component),flags);
    }
    if(n->kind==NativeScriptIR_Member&&state_namespace) {
        for(uint32_t v=0;v<p->value_count;++v) {
            const NativeScriptValueDeclaration* field=&p->values[v];
            if(field->owner!=owner||field->parameter||field->name.length!=n->text_length||
               memcmp(tree->source+field->name.offset,a->source+n->text_offset,n->text_length))continue;
            if(v>NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK||!scalar(field->type))
                return error(d,n->source,"Article reference state type/identity is unsupported");
            return out(r,field->array?NativeScriptType_Namespace:field->type,NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE|v,
                field->array?NativeScriptFlag_SourceOnly:flags);
        }
        return error(d,n->source,"State field is not declared by the selected Article");
    }
    if(n->kind==NativeScriptIR_Index&&category==NATIVE_SCRIPT_RECEIVER_ARTICLE_STATE) {
        uint32_t id=base&NATIVE_SCRIPT_RECEIVER_PAYLOAD_MASK;
        if(id>=p->value_count||!p->values[id].array||p->values[id].parameter||!scalar(p->values[id].type)||
           !p->values[id].owner||p->values[id].owner>=tree->count||tree->nodes[p->values[id].owner].kind!=NativeScriptSource_Article||
           n->b>=a->node_count||!nativeScriptExpression_IsIntegral(a->nodes[n->b].type))
            return error(d,n->source,"Article state array needs an integral index and a supported owned scalar declaration");
        return out(r,p->values[id].type,base,flags);
    }
    return error(d,n->source,"Unknown or unsupported read-only Article reference projection");
}
