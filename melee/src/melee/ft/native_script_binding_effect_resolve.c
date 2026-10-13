#include "native_script_binding_effect.h"
#include "native_script_effect_program.h"
#include <stdio.h>
#include <string.h>

static int error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return -1;}
static bool same(const NativeScriptIRArena* a,const NativeScriptIRNode* n,const char* text)
{return n->text_length==strlen(text)&&!memcmp(a->source+n->text_offset,text,n->text_length);}
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
        if(!found)return 0;if(c->owner||c->bindings.context_mask!=NativeScriptBindings_Fighter)return error(d,n->source,"Source effect identifier requires the root Fighter scope");
        if(found>NATIVE_SCRIPT_EFFECT_INDEX_MASK)return error(d,n->source,"Source effect owner exceeds semantic capacity");
        *r=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=UINT32_MAX,.flags=NativeScriptFlag_Literal,.literal_bits=found};return 1;
    }
    if(n->a>=a->node_count)return 0;const NativeScriptIRNode* parent=&a->nodes[n->a];uint32_t base=parent->semantic;
    if(n->kind==NativeScriptIR_Member) {
        if(base==NativeScriptBinding_Effects&&same(a,n,"spawn_root"))return out(r,NativeScriptType_Callable,NativeScriptEffectSpawnCall,NativeScriptFlag_Callable|NativeScriptFlag_MayInvalidateSource);
        if(base==NativeScriptEffectNamespace&&same(a,n,"joint"))return out(r,NativeScriptType_Callable,NativeScriptEffectJointCall,NativeScriptFlag_Callable|NativeScriptFlag_Pure);
        if(base==NativeScriptEffectNamespace&&same(a,n,"cancel"))return out(r,NativeScriptType_Callable,NativeScriptEffectCancelCall,NativeScriptFlag_Callable|NativeScriptFlag_MayInvalidateSource);
        if(base==NativeScriptEffectJointNamespace&&same(a,n,"rotation"))return out(r,NativeScriptType_Namespace,NativeScriptEffectRotationNamespace,0);
        bool visible=base==NativeScriptEffectJointNamespace&&same(a,n,"visible"),rotation=base==NativeScriptEffectRotationNamespace&&(same(a,n,"x")||same(a,n,"y")||same(a,n,"z"));
        if(visible||rotation){uint32_t id=a->node_count;if(id>NATIVE_SCRIPT_EFFECT_INDEX_MASK)return error(d,n->source,"Effect selector exceeds IR semantic capacity");
            return out(r,visible?NativeScriptType_Bool:NativeScriptType_F32,NATIVE_SCRIPT_EFFECT_PROPERTY|id,NativeScriptFlag_Readable|NativeScriptFlag_Addressable|NativeScriptFlag_Writable);}
        if((base&NATIVE_SCRIPT_EFFECT_DOMAIN_MASK)==NATIVE_SCRIPT_EFFECT_DOMAIN)return error(d,n->source,"Unsupported effect joint property");return 0;
    }
    if(n->kind!=NativeScriptIR_Call)return 0;
    if(base==NativeScriptEffectJointCall){NativeScriptEffectSelector selected={0};if(c->bindings.context_mask!=NativeScriptBindings_Effect||!arguments(a,n,&selected))return error(d,n->source,"Effect joint requires a literal bounded child path and optional member ordinal");return out(r,NativeScriptType_Namespace,NativeScriptEffectJointNamespace,NativeScriptFlag_Pure);}
    if(base==NativeScriptEffectCancelCall){if(n->argument_count)return error(d,n->source,"Effect cancel takes no arguments");return out(r,NativeScriptType_Void,NATIVE_SCRIPT_EFFECT_CANCEL,NativeScriptFlag_MayInvalidateSource);}
    bool deferred=base==NativeScriptBinding_EffectsSpawnAsyncPositionOnPart;
    bool on_part=base==NativeScriptBinding_EffectsSpawnOnPart||deferred;
    if(base!=NativeScriptEffectSpawnCall&&!on_part) {
        // A declared effect's literal is an owner index, never a numeric gfx
        // ID. Refuse unsupported call shapes rather than dispatching that
        // index as an unrelated engine effect.
        const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(base);
        if(descriptor&&descriptor->parent==NativeScriptBinding_Effects&&
           n->argument_count&&n->argument_first<a->argument_count) {
            uint32_t first=a->arguments[n->argument_first];
            if(first<a->node_count) {
                const NativeScriptIRNode* argument=&a->nodes[first];
                if(argument->kind==NativeScriptIR_Integer&&argument->type==NativeScriptType_I32&&
                   argument->bits<tree->count&&tree->nodes[argument->bits].kind==NativeScriptSource_Effect&&
                   argument->text_length==tree->nodes[argument->bits].name.length&&
                   !memcmp(a->source+argument->text_offset,tree->source+tree->nodes[argument->bits].name.offset,argument->text_length))
                    return error(d,n->source,"Named model_effect requires spawn_root, spawn_on_part or spawn_async_position_on_part");
            }
        }
        return 0;
    }
    if(on_part) {
        // Numeric engine recipe IDs keep their original dispatch ABI. Only a
        // declared model_effect selects an interpreted update program.
        if(!n->argument_count||n->argument_first>=a->argument_count)return 0;
        uint32_t first=a->arguments[n->argument_first];if(first>=a->node_count)return 0;
        const NativeScriptIRNode* argument=&a->nodes[first];
        if(argument->kind!=NativeScriptIR_Integer||argument->type!=NativeScriptType_I32||
           argument->bits>=tree->count||tree->nodes[argument->bits].kind!=NativeScriptSource_Effect||
           argument->text_length!=tree->nodes[argument->bits].name.length||
           memcmp(a->source+argument->text_offset,tree->source+tree->nodes[argument->bits].name.offset,argument->text_length))return 0;
    }
    if(c->owner||c->bindings.context_mask!=NativeScriptBindings_Fighter||n->argument_count!=(on_part?2u:1u)||n->argument_first>=a->argument_count)return error(d,n->source,"Source effect spawn needs a declared effect and optional raw part in root Fighter scope");
    uint32_t arg=a->arguments[n->argument_first];if(arg>=a->node_count)return error(d,n->source,"Invalid source effect argument");const NativeScriptIRNode* selected=&a->nodes[arg];
    uint32_t selected_owner=(uint32_t)selected->bits;
    if(selected->kind!=NativeScriptIR_Integer||selected->type!=NativeScriptType_I32||!selected_owner||selected_owner>=tree->count||tree->nodes[selected_owner].kind!=NativeScriptSource_Effect||
       selected->text_length!=tree->nodes[selected_owner].name.length||
       memcmp(a->source+selected->text_offset,tree->source+tree->nodes[selected_owner].name.offset,selected->text_length))
        return error(d,n->source,"Source effect spawn requires a declared model_effect identifier");
    return out(r,NativeScriptType_Void,(deferred?NATIVE_SCRIPT_EFFECT_SPAWN_DEFERRED_POSITION:on_part?NATIVE_SCRIPT_EFFECT_SPAWN_ON_PART:NATIVE_SCRIPT_EFFECT_SPAWN)|selected_owner,NativeScriptFlag_MayInvalidateSource);
}
bool nativeScriptBindingEffect_DescribeCall(void* unused,uint32_t semantic,NativeScriptCallLayout* layout,NativeScriptDiagnostic* d)
{
    (void)unused;if(!layout)return false;
    if(semantic==NATIVE_SCRIPT_EFFECT_CANCEL){*layout=(NativeScriptCallLayout){0};return true;}
    if((semantic&NATIVE_SCRIPT_EFFECT_PROPERTY_MASK)==NATIVE_SCRIPT_EFFECT_SPAWN){*layout=(NativeScriptCallLayout){.argument_count=1};layout->argument_types[0]=NativeScriptType_I32;return true;}
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
            selector.channel=field->type==NativeScriptType_Bool?10:same(a,field,"x")?3:same(a,field,"y")?4:5;
            if(parent->semantic==NativeScriptEffectRotationNamespace){if(parent->a>=a->node_count)return false;parent=&a->nodes[parent->a];}
            if(parent->semantic!=NativeScriptEffectJointNamespace||parent->kind!=NativeScriptIR_Call||!arguments(a,parent,&selector)||selector.member>=p->member_count)return false;
            memmove(p->selectors+at+1,p->selectors+at,(p->selector_count-at)*sizeof(*p->selectors));p->selectors[at]=selector;++p->selector_count;
        }
    }
    return true;
}
