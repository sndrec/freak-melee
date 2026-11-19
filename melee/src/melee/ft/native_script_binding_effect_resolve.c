#include "native_script_binding_effect.h"
#include "native_script_effect_program.h"
#include <stdio.h>
#include <string.h>

static int error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return -1;}
static bool same(const NativeScriptIRArena* a,const NativeScriptIRNode* n,const char* text)
{return n->text_length==strlen(text)&&!memcmp(a->source+n->text_offset,text,n->text_length);}
static bool named_effect(const NativeScriptSourceTree* tree, const NativeScriptIRArena* a,
                         const NativeScriptIRNode* n)
{
    if (n->kind != NativeScriptIR_Integer || n->type != NativeScriptType_I32 ||
        n->bits >= tree->count || tree->nodes[n->bits].kind != NativeScriptSource_Effect)
        return false;
    const NativeScriptSourceSpan name = tree->nodes[n->bits].name;
    return n->text_length == name.length &&
        !memcmp(a->source + n->text_offset, tree->source + name.offset, name.length);
}
static bool baked_position_effect(const NativeScriptSourceTree* tree, uint32_t at)
{
    const NativeScriptSourceSpan value = tree->nodes[at].value;
    // Metadata validation checks the path and lifetime. A positioned resource
    // starts with a quoted path and ends with its lifetime, with no follow flag.
    if (!value.length || tree->source[value.offset] != '"') return false;
    const char end = tree->source[value.offset + value.length - 1];
    if (end < '0' || end > '9') return false;
    for (uint32_t child = at + 1; child < tree->count && tree->nodes[child].parent; ++child) {
        if (tree->nodes[child].parent == at && tree->nodes[child].kind == NativeScriptSource_Callback)
            return false;
    }
    return true;
}
static int out(NativeScriptIRResolution* r,unsigned type,uint32_t semantic,unsigned flags)
{*r=(NativeScriptIRResolution){.type=type,.semantic=semantic,.flags=flags};return 1;}
static bool path(const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptEffectSelector* selector)
{
    if(n->kind!=NativeScriptIR_String||n->text_offset>a->source_bytes||n->text_length>a->source_bytes-n->text_offset)return false;
    selector->path_offset=n->text_offset;selector->path_length=n->text_length;
    const char* text=a->source+n->text_offset;unsigned at=0;
    while(at<n->text_length) {
        unsigned value=0,digits=0;
        while(at<n->text_length&&text[at]>='0'&&text[at]<='9'){if(++digits>3)return false;value=value*10+(unsigned)(text[at++]-'0');}
        if(!digits||value>255||selector->depth==12)return false;selector->path[selector->depth++]=(uint8_t)value;
        if(at==n->text_length)break;if(text[at++]!='/'||at==n->text_length)return false;
    }
    return true;
}
static bool arguments(const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptEffectSelector* selector)
{
    if(n->argument_count<1||n->argument_count>2||n->argument_first>a->argument_count||n->argument_count>a->argument_count-n->argument_first)return false;
    uint32_t first=a->arguments[n->argument_first];if(first>=a->node_count||!path(a,&a->nodes[first],selector))return false;
    selector->argument_count=(uint8_t)n->argument_count;
    if(n->argument_count==2){uint32_t second=a->arguments[n->argument_first+1];if(second>=a->node_count)return false;
        const NativeScriptIRNode* member=&a->nodes[second];if(member->kind!=NativeScriptIR_Integer||!nativeScriptExpression_IsIntegral(member->type)||member->bits>=EF_MODEL_GROUP_MAX)return false;selector->member=(uint8_t)member->bits;}
    return true;
}
int nativeScriptBindingEffect_TryResolve(void* opaque,const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptIRResolution* r,NativeScriptDiagnostic* d)
{
    NativeScriptBindingSpawnResolverContext* c=opaque;if(!c||!c->program||!a||!n||!r)return 0;
    const NativeScriptSourceTree* tree=c->program->tree;
    if(n->kind==NativeScriptIR_Name) {
        if(same(a,n,"effect"))return c->bindings.context_mask==NativeScriptBindings_Effect?out(r,NativeScriptType_Namespace,NativeScriptEffectNamespace,0):error(d,n->source,"Effect view requires an effect source owner");
        uint32_t found=0;
        for(uint32_t node=1;node<tree->count;++node)if(tree->nodes[node].kind==NativeScriptSource_Effect) {
            NativeScriptSourceSpan name=tree->nodes[node].name;
            if(name.length==n->text_length&&!memcmp(tree->source+name.offset,a->source+n->text_offset,name.length)){if(found)return error(d,n->source,"Ambiguous source effect name");found=node;}
        }
        if(!found)return 0;
        if ((c->owner || c->bindings.context_mask != NativeScriptBindings_Fighter) &&
            c->bindings.context_mask != NativeScriptBindings_Article)
            return error(d,n->source,"Source effect identifier requires a Fighter or Article scope");
        if(found>NATIVE_SCRIPT_EFFECT_INDEX_MASK)return error(d,n->source,"Source effect owner exceeds semantic capacity");
        *r=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=UINT32_MAX,.flags=NativeScriptFlag_Literal,.literal_bits=found};return 1;
    }
    if(n->a>=a->node_count)return 0;const NativeScriptIRNode* parent=&a->nodes[n->a];uint32_t base=parent->semantic;
    if(n->kind==NativeScriptIR_Member) {
        if(base==NativeScriptBinding_Effects&&same(a,n,"spawn_root"))return out(r,NativeScriptType_Callable,NativeScriptEffectSpawnCall,NativeScriptFlag_Callable|NativeScriptFlag_MayInvalidateSource);
        if(base==NativeScriptEffectNamespace&&same(a,n,"joint"))return out(r,NativeScriptType_Callable,NativeScriptEffectJointCall,NativeScriptFlag_Callable|NativeScriptFlag_Pure);
        if(base==NativeScriptEffectNamespace&&same(a,n,"cancel"))return out(r,NativeScriptType_Callable,NativeScriptEffectCancelCall,NativeScriptFlag_Callable|NativeScriptFlag_MayInvalidateSource);
        if (base == NativeScriptEffectJointNamespace) {
            if (same(a, n, "translation")) return out(r, NativeScriptType_Namespace, NativeScriptEffectTranslationNamespace, 0);
            if (same(a, n, "rotation")) return out(r, NativeScriptType_Namespace, NativeScriptEffectRotationNamespace, 0);
            if (same(a, n, "scale")) return out(r, NativeScriptType_Namespace, NativeScriptEffectScaleNamespace, 0);
        }
        bool visible = base == NativeScriptEffectJointNamespace && same(a, n, "visible");
        bool transform = (base == NativeScriptEffectTranslationNamespace || base == NativeScriptEffectRotationNamespace ||
                          base == NativeScriptEffectScaleNamespace) &&
                         (same(a, n, "x") || same(a, n, "y") || same(a, n, "z"));
        if(visible||transform){uint32_t id=a->node_count;if(id>NATIVE_SCRIPT_EFFECT_INDEX_MASK)return error(d,n->source,"Effect selector exceeds IR semantic capacity");
            return out(r,visible?NativeScriptType_Bool:NativeScriptType_F32,NATIVE_SCRIPT_EFFECT_PROPERTY|id,NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable);}
        if((base&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)return error(d,n->source,"Unsupported effect joint property");return 0;
    }
    if(n->kind!=NativeScriptIR_Call)return 0;
    if(base==NativeScriptEffectJointCall){NativeScriptEffectSelector selected={0};if(c->bindings.context_mask!=NativeScriptBindings_Effect||!arguments(a,n,&selected))return error(d,n->source,"Effect joint requires a literal bounded child path and optional member ordinal");return out(r,NativeScriptType_Namespace,NativeScriptEffectJointNamespace,NativeScriptFlag_Pure);}
    if(base==NativeScriptEffectCancelCall){if(n->argument_count)return error(d,n->source,"Effect cancel takes no arguments");return out(r,NativeScriptType_Void,NATIVE_SCRIPT_EFFECT_CANCEL,NativeScriptFlag_MayInvalidateSource);}
    const NativeScriptIRNode* first_arg = NULL;
    if (n->argument_count && n->argument_first < a->argument_count) {
        uint32_t first = a->arguments[n->argument_first];
        if (first < a->node_count) first_arg = &a->nodes[first];
    }
    const bool named = first_arg && named_effect(tree, a, first_arg);
    if (base == NativeScriptBinding_ItemSpawnEffectAtPosition && named) {
        if (c->bindings.context_mask != NativeScriptBindings_Article || n->argument_count != 5 ||
            !baked_position_effect(tree, (uint32_t)first_arg->bits))
            return error(d, n->source, "Article model effects need a packaged effect with pass, XYZ and roll");
        return out(r, NativeScriptType_Void,
                   NATIVE_SCRIPT_EFFECT_SPAWN_ITEM_POSITION | (uint32_t)first_arg->bits,
                   NativeScriptFlag_MayInvalidateSource);
    }
    bool deferred=base==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart;
    bool on_part=base==NativeScriptBinding_EffectsSpawnOnPart||deferred;
    if(base!=NativeScriptEffectSpawnCall&&!on_part) {
        // A declared effect's literal is an owner index, never a numeric gfx
        // ID. Refuse unsupported call shapes rather than dispatching that
        // index as an unrelated engine effect.
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(base);
        if (named && descriptor && (descriptor->parent == NativeScriptBinding_Effects ||
            c->bindings.context_mask == NativeScriptBindings_Article))
            return error(d,n->source,"Named model_effect requires a named effect spawn operation");
        return 0;
    }
    if (on_part && !named) return 0;
    if(c->owner||c->bindings.context_mask!=NativeScriptBindings_Fighter||n->argument_count!=(on_part?2u:1u))
        return error(d,n->source,"Source effect spawn needs a declared effect and optional raw part in root Fighter scope");
    if (!named)
        return error(d,n->source,"Source effect spawn requires a declared model_effect identifier");
    return out(r,NativeScriptType_Void,(deferred?NATIVE_SCRIPT_EFFECT_SPAWN_DEFERRED_POSITION:on_part?NATIVE_SCRIPT_EFFECT_SPAWN_ON_PART:NATIVE_SCRIPT_EFFECT_SPAWN)|(uint32_t)first_arg->bits,NativeScriptFlag_MayInvalidateSource);
}
bool nativeScriptBindingEffect_DescribeCall(void* unused,uint32_t semantic,NativeScriptCallLayout* layout,NativeScriptDiagnostic* d)
{
    (void)unused;if(!layout)return false;
    if(semantic==NATIVE_SCRIPT_EFFECT_CANCEL){*layout=(NativeScriptCallLayout){0};return true;}
    if((semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN){*layout=(NativeScriptCallLayout){.argument_count=1};layout->argument_types[0]=NativeScriptType_I32;return true;}
    if ((semantic & NATIVE_SCRIPT_EFFECT_PROPERTY_MASK) == NATIVE_SCRIPT_EFFECT_SPAWN_ITEM_POSITION) {
        *layout = (NativeScriptCallLayout){.argument_count = 5,
            .argument_types = {NativeScriptType_I32, NativeScriptType_F32, NativeScriptType_F32,
                               NativeScriptType_F32, NativeScriptType_F32}};
        return true;
    }
    if((semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN_ON_PART||
       (semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN_DEFERRED_POSITION){*layout=(NativeScriptCallLayout){.argument_count=2};layout->argument_types[0]=layout->argument_types[1]=NativeScriptType_I32;return true;}
    error(d,(NativeScriptSourceSpan){0},"Effect namespace/selector is not an executable call");return false;
}
bool nativeScriptBindingEffect_PrepareSelectors(const NativeScriptCompileBank* bank,NativeScriptEffectProgram* p,NativeScriptDiagnostic* d)
{
    const NativeScriptIRArena* a=&bank->expressions;
    for(uint32_t f=0;f<bank->code.function_count;++f)if(bank->declarations.functions[f].owner==p->owner) {
        const NativeScriptFunctionCode* function=&bank->code.functions[f];
        for(uint32_t i=0;i<function->instruction_count;++i){uint32_t semantic=bank->code.instructions[function->first_instruction+i].semantic;
            if((semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)!=NATIVE_SCRIPT_EFFECT_PROPERTY)continue;
            unsigned at=0;while(at<p->selector_count&&p->selectors[at].semantic<semantic)++at;if(at<p->selector_count&&p->selectors[at].semantic==semantic)continue;
            if(p->selector_count==NATIVE_SCRIPT_EFFECT_SELECTORS){error(d,function->source,"Effect selector capacity exceeded");return false;}
            uint32_t id=semantic&NATIVE_SCRIPT_EFFECT_INDEX_MASK;if(id>=a->node_count)return false;const NativeScriptIRNode* field=&a->nodes[id];
            if(field->a>=a->node_count)return false;const NativeScriptIRNode* parent=&a->nodes[field->a];NativeScriptEffectSelector selector={.semantic=semantic};
            if (field->type == NativeScriptType_Bool) {
                selector.channel = 10;
            } else {
                unsigned channel;
                switch (parent->semantic) {
                case NativeScriptEffectTranslationNamespace: channel = 0; break;
                case NativeScriptEffectRotationNamespace: channel = 3; break;
                case NativeScriptEffectScaleNamespace: channel = 6; break;
                default: return false;
                }
                if (parent->a >= a->node_count) return false;
                selector.channel = channel + (same(a, field, "x") ? 0 : same(a, field, "y") ? 1 : 2);
                parent = &a->nodes[parent->a];
            }
            if(parent->semantic!=NativeScriptEffectJointNamespace||parent->kind!=NativeScriptIR_Call||!arguments(a,parent,&selector)||selector.member>=p->member_count)return false;
            memmove(p->selectors+at+1,p->selectors+at,(p->selector_count-at)*sizeof(*p->selectors));p->selectors[at]=selector;++p->selector_count;
        }
    }
    return true;
}
