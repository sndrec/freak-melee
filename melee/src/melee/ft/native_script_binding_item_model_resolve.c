#include "native_script_binding_item_model_resolve.h"
#include "native_script_compile.h"
#include "native_script_item_program.h"
#include "native_fighter_runtime.h"
#include "native_script_constants.h"
#include "native_script_model_metadata.h"
#include <melee/it/native_article_animation_demand.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <string.h>
static int error(NativeScriptDiagnostic* d,NativeScriptSourceSpan source,const char* message)
{if(d){d->source=source;snprintf(d->message,sizeof(d->message),"%s",message);}return -1;}
static bool word(const NativeScriptIRArena* a,const NativeScriptIRNode* n,const char* text)
{return n->text_length==strlen(text)&&!memcmp(a->source+n->text_offset,text,n->text_length);}
static int out(NativeScriptIRResolution* r,unsigned type,uint32_t semantic,uint16_t flags)
{*r=(NativeScriptIRResolution){.type=(NativeScriptIRType)type,.semantic=semantic,.flags=flags};return 1;}
static bool equal(const NativeScriptSourceTree* tree,NativeScriptSourceSpan x,NativeScriptSourceSpan y)
{return x.length==y.length&&!memcmp(tree->source+x.offset,tree->source+y.offset,x.length);}
bool nativeScriptBindingItemModel_Declaration(const NativeScriptSourceTree* tree,
    uint32_t owner,uint32_t node,NativeScriptModelMetadata* metadata,uint32_t* ordinal,NativeScriptDiagnostic* d)
{
    if(!tree||!tree->source||!tree->nodes||!metadata||!ordinal||!owner||owner>=tree->count||node>=tree->count||
       (tree->nodes[owner].kind!=NativeScriptSource_Article&&tree->nodes[owner].kind!=NativeScriptSource_Item))
        return error(d,(NativeScriptSourceSpan){0},"Model declaration requires an exact Item/Article owner")>0;
    const NativeScriptSourceNode* selected_node=&tree->nodes[node];
    bool selected_animation=nativeScriptSource_SpanEquals(tree,selected_node->name,"model_animation");
    if(selected_node->kind!=NativeScriptSource_Metadata||selected_node->parent!=owner||
       (!selected_animation&&!nativeScriptSource_SpanEquals(tree,selected_node->name,"model_root")))
        return error(d,selected_node->source,"Model declaration is outside its direct owner")>0;
    NativeScriptSourceSpan names[NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT+NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT];
    NativeScriptModelMetadata selected={0};
    uint32_t count=0,counts[2]={0},selected_ordinal=UINT32_MAX;
    bool state_seen=false,callback_seen=false;
    for(uint32_t child=tree->nodes[owner].first_child,bound=0;child!=UINT32_MAX;child=tree->nodes[child].next){
        if(child>=tree->count||bound++==tree->count)return error(d,tree->nodes[owner].source,"Model declaration child index/cycle is invalid")>0;
        const NativeScriptSourceNode* field=&tree->nodes[child];
        if(field->kind==NativeScriptSource_ArticleState)state_seen=true;
        if(field->kind==NativeScriptSource_Callback)callback_seen=true;
        if(field->kind!=NativeScriptSource_Metadata)continue;
        bool animation=nativeScriptSource_SpanEquals(tree,field->name,"model_animation");
        if(!animation&&!nativeScriptSource_SpanEquals(tree,field->name,"model_root"))continue;
        NativeScriptModelMetadata parsed;
        uint32_t limit=animation?NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT:NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT;
        if(field->parent!=owner||field->opens||state_seen||(animation&&callback_seen)||counts[animation]==limit||
           !nativeScriptModelMetadata_Parse(tree,field->value,animation,&parsed))
            return error(d,field->source,"Model declaration needs bounded aligned pointers before states (bundles before callbacks)")>0;
        for(uint32_t previous=0;previous<count;++previous)if(equal(tree,parsed.name,names[previous]))
            return error(d,field->source,"Duplicate/conflicting root or animation name in this owner")>0;
        if(child==node){selected=parsed;selected_ordinal=counts[animation];}
        names[count++]=parsed.name;++counts[animation];
    }
    if(selected_ordinal==UINT32_MAX)return error(d,selected_node->source,"Model declaration is outside its direct owner")>0;
    for(uint32_t i=1;i<tree->count;++i){const NativeScriptSourceNode* other=&tree->nodes[i];
        bool symbol=(other->kind==NativeScriptSource_Function||other->kind==NativeScriptSource_State||
            other->kind==NativeScriptSource_Parameter||other->kind==NativeScriptSource_Attribute)&&(!other->parent||other->parent==owner);
        bool entity=other->kind==NativeScriptSource_Article||other->kind==NativeScriptSource_Item||other->kind==NativeScriptSource_Effect;
        if((symbol||entity)&&equal(tree,selected.name,other->name))return error(d,selected_node->source,"Model name conflicts with a source declaration")>0;
    }
    const char* const reserved[]={"item","state","params","attrs","creator","owner","current_owner","attachment_owner","holder","event","program","types","true","false","None","and","or","not"};
    for(unsigned i=0;i<sizeof(reserved)/sizeof(*reserved);++i)if(nativeScriptSource_SpanEquals(tree,selected.name,reserved[i]))
        return error(d,selected_node->source,"Model name conflicts with a source/native namespace")>0;
    for(uint32_t i=1;i<NativeScriptBinding_Count;++i){const NativeScriptBindingDescriptor* descriptor=nativeScriptBindings_Descriptor(i);
        if(descriptor&&!descriptor->parent&&nativeScriptSource_SpanEquals(tree,selected.name,descriptor->name))
            return error(d,selected_node->source,"Model name conflicts with a native name")>0;}
    NativeScriptIRArena source={.source=tree->source,.source_bytes=tree->source_bytes};
    NativeScriptIRNode identifier={.kind=NativeScriptIR_Name,.source=selected.name,.text_offset=selected.name.offset,.text_length=selected.name.length};
    NativeScriptIRResolution constant;int known=nativeScriptConstants_TryResolve(NULL,&source,&identifier,&constant,d);
    if(known<0)return false;if(known)return error(d,selected_node->source,"Model name conflicts with a shared constant")>0;
    *metadata=selected;*ordinal=selected_ordinal;return true;
}
static bool matches(const NativeScriptSourceTree* tree,NativeScriptSourceSpan name,
    const NativeScriptIRArena* a,const NativeScriptIRNode* n)
{return name.length==n->text_length&&!memcmp(tree->source+name.offset,a->source+n->text_offset,name.length);}
static int model_symbol(const NativeScriptBindingSpawnResolverContext* c,const NativeScriptIRArena* a,
    const NativeScriptIRNode* n,uint32_t* selected,bool* animation,NativeScriptDiagnostic* d)
{
    if(!c||!c->program||!c->program->tree||!c->owner||c->owner>=c->program->tree->count)return 0;
    const NativeScriptSourceTree* tree=c->program->tree;
    for(uint32_t child=tree->nodes[c->owner].first_child,bound=0;child!=UINT32_MAX;child=tree->nodes[child].next){
        if(child>=tree->count||bound++==tree->count)return error(d,n->source,"Model declaration child index/cycle is invalid");
        const NativeScriptSourceNode* field=&tree->nodes[child];
        if(field->kind!=NativeScriptSource_Metadata)continue;
        bool bundle=nativeScriptSource_SpanEquals(tree,field->name,"model_animation");
        if(!bundle&&!nativeScriptSource_SpanEquals(tree,field->name,"model_root"))continue;
        NativeScriptModelMetadata metadata;uint32_t ordinal;
        if(!nativeScriptBindingItemModel_Declaration(tree,c->owner,child,&metadata,&ordinal,d))return -1;
        if(!matches(tree,metadata.name,a,n))continue;
        if(c->bindings.context_mask!=NativeScriptBindings_Article||c->program->expressions!=a||tree->source!=a->source||tree->source_bytes!=a->source_bytes)
            return error(d,n->source,"Model literal requires its exact indexed Item body");
        *selected=ordinal;*animation=bundle;return 1;
    }
    return 0;
}
static bool path_argument(const NativeScriptIRArena* a,const NativeScriptIRNode* n,unsigned path,NativeScriptItemModelSelector* s)
{
    if(n->kind!=NativeScriptIR_Call||path>=n->argument_count||
       n->argument_first>a->argument_count||n->argument_count>a->argument_count-n->argument_first)return false;
    uint32_t id=a->arguments[n->argument_first+path];if(id>=a->node_count)return false;
    const NativeScriptIRNode* literal=&a->nodes[id];if(literal->kind!=NativeScriptIR_String||literal->type!=NativeScriptType_Text||
       literal->text_offset>a->source_bytes||literal->text_length>a->source_bytes-literal->text_offset)return false;
    s->path_offset=literal->text_offset;s->path_length=literal->text_length;s->depth=0;
    const char* text=a->source+literal->text_offset;uint32_t at=0;
    while(at<literal->text_length){if(s->depth==12)return false;unsigned ordinal=0,digits=0;
        while(at<literal->text_length&&text[at]>='0'&&text[at]<='9'){if(++digits>3)return false;ordinal=ordinal*10+(unsigned)(text[at++]-'0');}
        if(!digits||ordinal>255)return false;s->path[s->depth++]=(uint8_t)ordinal;
        if(at<literal->text_length&&(text[at++]!='/'||at==literal->text_length))return false;
    }
    return true;
}
static bool arguments(const NativeScriptIRArena* a,const NativeScriptIRNode* n,unsigned count,NativeScriptItemModelSelector* s)
{
    if((count!=1&&count!=2)||!path_argument(a,n,count-1,s))return false;
    if(count==2){uint32_t id=a->arguments[n->argument_first];if(id>=a->node_count)return false;
        const NativeScriptIRNode* index=&a->nodes[id];if(index->kind!=NativeScriptIR_Integer||index->type!=NativeScriptType_I32||index->bits>3)return false;s->model=(uint8_t)index->bits;}
    else s->model=0;
    s->argument_count=(uint8_t)count;return true;
}
static bool async_arguments(const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptItemModelSelector* s)
{
    if((n->argument_count!=3&&n->argument_count!=4)||!path_argument(a,n,1,s))return false;
    uint32_t effect=a->arguments[n->argument_first];
    if(effect>=a->node_count||a->nodes[effect].type!=NativeScriptType_I32)return false;
    uint32_t id=a->arguments[n->argument_first+2];if(id>=a->node_count)return false;
    const NativeScriptIRNode* mode=&a->nodes[id];
    if(mode->kind!=NativeScriptIR_Integer||mode->type!=NativeScriptType_I32||
       (n->argument_count==3?(mode->bits!=0&&mode->bits!=1&&mode->bits!=3):mode->bits!=4))return false;
    s->argument_count=(uint8_t)n->argument_count;s->model=0;s->channel=(uint8_t)mode->bits;return true;
}
static bool sync_arguments(const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptItemModelSelector* s)
{
    if(n->argument_count!=2||!path_argument(a,n,1,s))return false;
    uint32_t effect=a->arguments[n->argument_first];
    if(effect>=a->node_count||a->nodes[effect].type!=NativeScriptType_I32)return false;
    s->argument_count=2;s->model=0;s->channel=0;return true;
}
static bool dynamic_arguments(const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptItemModelSelector* s)
{
    if(n->kind!=NativeScriptIR_Call||n->argument_count!=1||n->argument_first>=a->argument_count)return false;
    uint32_t id=a->arguments[n->argument_first];if(id>=a->node_count)return false;
    const NativeScriptIRNode* index=&a->nodes[id];
    if(index->kind!=NativeScriptIR_Integer||index->type!=NativeScriptType_I32||index->bits>=100)return false;
    s->dynamic_bone=(uint8_t)index->bits;s->argument_count=1;return true;
}
static uint32_t animation_domain(uint32_t base,unsigned count)
{
    if(base==NativeScriptBinding_ItemAddModelAnimation)return NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD;
    if(base==NativeScriptBinding_ItemRequestModelAnimation)return NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REQUEST;
    if(base==NativeScriptBinding_ItemRemoveModelAnimation)return NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REMOVE;
    if(base==NativeScriptBinding_ItemAdvanceModelAnimation&&count)return NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADVANCE;
    if(base==NativeScriptBinding_ItemSetModelAnimationRate&&count>1)return NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE;
    return 0;
}
static bool animation_arguments(const NativeScriptBindingSpawnResolverContext* c,const NativeScriptIRArena* a,
    const NativeScriptIRNode* n,uint32_t domain,NativeScriptItemModelSelector* s,NativeScriptDiagnostic* d)
{
    if(!n||n->kind!=NativeScriptIR_Call||n->a>=a->node_count||
       n->argument_first>a->argument_count||n->argument_count>a->argument_count-n->argument_first||
       animation_domain(a->nodes[n->a].semantic,n->argument_count)!=domain)
        return error(d,n?n->source:(NativeScriptSourceSpan){0},"Animation call identity/argument range is invalid")>0;
    bool value=domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD||domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REQUEST||domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE;
    bool path=n->argument_count==(value?3:2);
    if(!path&&n->argument_count!=(value?2:1))
        return error(d,n->source,"Animation call needs literal model0..3, optional path and its typed value")>0;
    for(unsigned i=0;i<n->argument_count;++i)if(a->arguments[n->argument_first+i]>=a->node_count)
        return error(d,n->source,"Animation argument IR index is invalid")>0;
    const NativeScriptIRNode* model=&a->nodes[a->arguments[n->argument_first]];
    if(model->kind!=NativeScriptIR_Integer||model->type!=NativeScriptType_I32||model->bits>3)
        return error(d,model->source,"Animation model must be a literal0..3")>0;
    s->animation=UINT32_MAX;
    if(path){if(!arguments(a,n,2,s))return error(d,n->source,"Animation path must be a bounded literal child path")>0;}
    else{s->model=(uint8_t)model->bits;s->depth=0;s->path_offset=0;s->path_length=0;}
    s->argument_count=(uint8_t)n->argument_count;
    if(!value)return true;
    const NativeScriptIRNode* input=&a->nodes[a->arguments[n->argument_first+n->argument_count-1]];
    if(domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD){
        if(input->kind!=NativeScriptIR_Integer||input->type!=NativeScriptType_I32||!input->text_length||
           input->text_offset>a->source_bytes||input->text_length>a->source_bytes-input->text_offset)
            return error(d,input->source,"Animation Add needs its declared bundle identifier")>0;
        uint32_t ordinal;bool bundle;int found=model_symbol(c,a,input,&ordinal,&bundle,d);
        if(found<0)return false;
        if(!found||!bundle||input->bits!=ordinal)
            return error(d,input->source,"Animation Add bundle is not its exact same-owner declaration")>0;
        s->animation=ordinal;
    }else if(!nativeScriptExpression_Assignable(NativeScriptType_F32,input->type))
        return error(d,input->source,"Animation frame/rate must be numeric Float input")>0;
    return true;
}
bool nativeScriptBindingItemModel_DescribeCall(void* opaque,uint32_t semantic,
    NativeScriptCallLayout* output,NativeScriptDiagnostic* d)
{
    const NativeScriptBindingSpawnResolverContext* c=opaque;
    uint32_t domain=semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
    if(!c||!c->program||!c->program->expressions||!output||c->bindings.context_mask!=NativeScriptBindings_Article||
       domain<NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD||
       (domain>NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT&&domain!=NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT))
        return error(d,(NativeScriptSourceSpan){0},"Animation layout requires its exact indexed source context")>0;
    const NativeScriptIRArena* a=c->program->expressions;uint32_t id=semantic&NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK;
    if(id>=a->node_count||a->nodes[id].semantic!=semantic)
        return error(d,(NativeScriptSourceSpan){0},"Animation layout IR semantic differs")>0;
    const NativeScriptIRNode* n=&a->nodes[id];NativeScriptItemModelSelector selector={0};
    if(domain==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT){
        if(!sync_arguments(a,n,&selector))return error(d,n->source,"Synchronous Item effect needs an Int effect and immutable primary path")>0;
        *output=(NativeScriptCallLayout){.argument_count=2,.argument_types={NativeScriptType_I32,NativeScriptType_Text}};return true;
    }
    if(domain==NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT){
        if(!async_arguments(a,n,&selector))return error(d,n->source,"Async Item effect needs its immutable path and mode0/1/3, or mode4 with a scalar")>0;
        *output=(NativeScriptCallLayout){.argument_count=selector.argument_count,.argument_types={NativeScriptType_I32,NativeScriptType_Text,NativeScriptType_I32,NativeScriptType_F32}};return true;
    }
    if(!animation_arguments(c,a,n,domain,&selector,d))return false;
    NativeScriptCallLayout layout={0};layout.argument_count=n->argument_count;
    layout.argument_types[0]=NativeScriptType_I32;
    bool value=domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD||domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_REQUEST||domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE;
    bool path=n->argument_count==(value?3:2);
    if(path)layout.argument_types[1]=NativeScriptType_Text;
    if(value)layout.argument_types[n->argument_count-1]=domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD?NativeScriptType_I32:NativeScriptType_F32;
    *output=layout;return true;
}
static bool animation_targets(const NativeScriptItemProgram* p,uint32_t domain,
    NativeScriptItemModelSelector* s,NativeScriptDiagnostic* d,NativeScriptSourceSpan at)
{
    bool add=domain==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD;
    if(p->model_root_count>NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT||(p->model_root_count&&!p->model_roots)||
       p->model_animation_count>NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT||
       (p->model_animation_count&&!p->model_animations)||(add&&s->animation>=p->model_animation_count))
        return error(d,at,"Animation selector resource table is invalid")>0;
    const NativeScriptItemModelAnimationAsset* bundle=add?&p->model_animations[s->animation]:NULL;
    unsigned first=s->model?1:0,last=s->model?p->model_root_count:0;
    for(unsigned index=first;index<=last;++index){
        const HSD_Joint* root=index?p->model_roots[index-1].root:p->article->x10_modelDesc->x0_joint;
        if(!root||(!index&&(root->flags&(JOBJ_PTCL|JOBJ_SPLINE))))
            return error(d,at,"Animation selectors do not infer synthetic/particle primary topology")>0;
        const HSD_Joint* selected=root;
        for(unsigned step=0;step<s->depth&&selected;++step){
            if(selected->flags&(JOBJ_INSTANCE|JOBJ_PTCL|JOBJ_SPLINE))
                return error(d,at,"Animation path crosses an unsupported instance/particle graph")>0;
            selected=selected->child;
            for(unsigned sibling=0;sibling<s->path[step]&&selected;++sibling)selected=selected->next;
        }
        NativeScriptItemModelAnimationTarget target={.descriptor=selected,.valid=true};
        if(selected){
            NativeArticleAnimationDemand demand;
            if(!nativeArticleAnimation_Census(selected,bundle?bundle->anim:NULL,bundle?bundle->matanim:NULL,
                bundle?bundle->shapeanim:NULL,&demand)||(add&&demand.uncovered))
                return error(d,at,"Animation bundle/target has malformed or uncovered native channels")>0;
            target.controllers=demand.controllers;target.tracks=demand.tracks;target.uncovered=demand.uncovered;
        }
        s->animation_targets[index]=target;
    }
    return true;
}
int nativeScriptBindingItemModel_TryResolve(void* opaque,const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptIRResolution* r,NativeScriptDiagnostic* d)
{
    if(!a||!n||!a->nodes)return 0;
    if(!a->source||n->text_offset>a->source_bytes||n->text_length>a->source_bytes-n->text_offset)
        return error(d,n->source,"Item model source span is invalid");
    const NativeScriptBindingSpawnResolverContext* c=opaque;
    if(n->kind==NativeScriptIR_Name){uint32_t ordinal;bool animation;int found=model_symbol(c,a,n,&ordinal,&animation,d);if(found<=0)return found;
        if(!r)return error(d,n->source,"Model declaration literal output is absent");
        *r=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,.flags=NativeScriptFlag_Literal,.literal_bits=ordinal};return 1;}
    if(n->a>=a->node_count)return 0;
    const NativeScriptIRNode* parent=&a->nodes[n->a];uint32_t base=parent->semantic;
    uint32_t animation=n->kind==NativeScriptIR_Call?animation_domain(base,n->argument_count):0;
    bool transform=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ItemSetJointTransform;
    bool async=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ItemSpawnAsyncJointEffect;
    bool sync=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ItemSpawnJointEffect;
    bool load=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ItemLoadModel;
    bool secondary=n->kind==NativeScriptIR_Call&&(load||base==NativeScriptBinding_ItemDestroyModel||base==NativeScriptBinding_ItemSetModelDrawEnabled);
    bool joint=n->kind==NativeScriptIR_Member&&base==NATIVE_SCRIPT_ITEM_NAMESPACE&&word(a,n,"joint");
    bool world=n->kind==NativeScriptIR_Member&&base==NATIVE_SCRIPT_ITEM_NAMESPACE&&word(a,n,"joint_world_position");
    bool dynamic=n->kind==NativeScriptIR_Member&&base==NATIVE_SCRIPT_ITEM_NAMESPACE&&word(a,n,"dynamic_joint");
    bool matrix=n->kind==NativeScriptIR_Call&&base==NativeScriptBinding_ItemReadJointMatrixRotation;
    if(!animation&&!joint&&!world&&!dynamic&&!matrix&&!transform&&!async&&!sync&&!secondary&&(base&NATIVE_SCRIPT_ITEM_MODEL_DOMAIN_MASK)!=NATIVE_SCRIPT_ITEM_MODEL_DOMAIN)return 0;
    if(!r||!c||!c->program||c->program->expressions!=a||c->bindings.context_mask!=NativeScriptBindings_Article||!c->owner||
       c->owner>=c->program->tree->count||
       (c->program->tree->nodes[c->owner].kind!=NativeScriptSource_Article&&c->program->tree->nodes[c->owner].kind!=NativeScriptSource_Item))
        return error(d,n->source,"Item model requires its exact indexed Item/Article source owner");
    if(sync){NativeScriptItemModelSelector selector={0};
        if(!sync_arguments(a,n,&selector))return error(d,n->source,"Synchronous Item effect needs an Int effect and literal primary path");
        if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,base,r,d))return -1;
        if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Synchronous Item effect exceeds semantic capacity");
        return out(r,NativeScriptType_Void,NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT|a->node_count,NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource);
    }
    if(async){NativeScriptItemModelSelector selector={0};
        if(!async_arguments(a,n,&selector))return error(d,n->source,"Async Item effect needs an Int effect, literal primary path and mode0/1/3, or mode4 with a scalar");
        if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,base,r,d))return -1;
        if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Async Item effect exceeds semantic capacity");
        return out(r,NativeScriptType_Void,NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT|a->node_count,NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource);
    }
    if(animation){NativeScriptItemModelSelector selector={0};
        if(!animation_arguments(c,a,n,animation,&selector,d))return -1;
        if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Animation call exceeds semantic capacity");
        return out(r,animation==NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD?NativeScriptType_Bool:NativeScriptType_Void,
            animation|a->node_count,NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource);
    }
    if(secondary){
        if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,base,r,d))return -1;
        const NativeScriptIRNode* slot=&a->nodes[a->arguments[n->argument_first]];
        if(slot->kind!=NativeScriptIR_Integer||slot->type!=NativeScriptType_I32||slot->bits<1||slot->bits>3)
            return error(d,slot->source,"Secondary model needs a literal slot in 1..3");
        if(load){const NativeScriptIRNode* name=&a->nodes[a->arguments[n->argument_first+1]];uint32_t ordinal;
            if(name->kind!=NativeScriptIR_Integer||name->type!=NativeScriptType_I32||!name->text_length||
               name->text_offset>a->source_bytes||name->text_length>a->source_bytes-name->text_offset)
                return error(d,name->source,"Model load needs its declared root identifier");
            bool animation;int found=model_symbol(c,a,name,&ordinal,&animation,d);if(found<0)return -1;
            if(!found||animation||name->bits!=ordinal)return error(d,name->source,"Model load root is not its exact same-owner declaration");}
        return 1;
    }
    if(transform){NativeScriptItemModelSelector selector={0};
        if(n->argument_count!=13||!arguments(a,n,2,&selector))return error(d,n->source,"Joint transform needs literal model0..3, path, ten floats and a Bool");
        if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,base,r,d))return -1;
        if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Joint transform exceeds semantic capacity");
        return out(r,NativeScriptType_Void,NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM|a->node_count,NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource);
    }
    if(matrix){NativeScriptItemModelSelector selector={0};
        if(n->argument_count!=5||!arguments(a,n,2,&selector))return error(d,n->source,"Matrix rotation needs literal model0..3, a bounded literal path and three Float outputs");
        if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,base,r,d))return -1;
        if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Matrix rotation exceeds semantic capacity");
        return out(r,NativeScriptType_Void,NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION|a->node_count,NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly);
    }
    if(dynamic)return out(r,NativeScriptType_Callable,NativeScriptItemModel_DynamicJointCall,NativeScriptFlag_Readable|NativeScriptFlag_Callable|NativeScriptFlag_SourceOnly);
    if(joint)return out(r,NativeScriptType_Callable,NativeScriptItemModel_JointCall,NativeScriptFlag_Readable|NativeScriptFlag_Callable|NativeScriptFlag_SourceOnly);
    if(world)return out(r,NativeScriptType_Callable,NativeScriptItemModel_WorldPositionCall,NativeScriptFlag_Readable|NativeScriptFlag_Callable|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource);
    if(n->kind==NativeScriptIR_Call&&base==NativeScriptItemModel_WorldPositionCall){NativeScriptItemModelSelector s={0};
        if(n->argument_count!=5||!arguments(a,n,2,&s))return error(d,n->source,"Item world position needs literal model0..3, a bounded ordinal path and three float outputs");
        if(!nativeScriptBindings_ResolveCall((void*)&c->bindings,a,n,NativeScriptBinding_ItemJointWorldPosition,r,d))return -1;
        if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Item world position exceeds semantic capacity");
        return out(r,NativeScriptType_Void,NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION|a->node_count,NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly|NativeScriptFlag_MayInvalidateSource);}
    if(n->kind==NativeScriptIR_Call&&base==NativeScriptItemModel_JointCall){NativeScriptItemModelSelector s={0};
        if(!arguments(a,n,n->argument_count,&s))return error(d,n->source,"Item joint needs literal model0..3 and a bounded literal ordinal path");
        return out(r,NativeScriptType_Namespace,NativeScriptItemModel_JointNamespace,NativeScriptFlag_SourceOnly);}
    if(n->kind==NativeScriptIR_Call&&base==NativeScriptItemModel_DynamicJointCall){NativeScriptItemModelSelector s={0};
        if(!dynamic_arguments(a,n,&s))return error(d,n->source,"Dynamic joint needs one literal Int index0..99");
        return out(r,NativeScriptType_Namespace,NativeScriptItemModel_DynamicJointNamespace,NativeScriptFlag_SourceOnly);}
    if(n->kind==NativeScriptIR_Member) {
        bool joint_namespace=base==NativeScriptItemModel_JointNamespace||base==NativeScriptItemModel_DynamicJointNamespace;
        if(joint_namespace&&word(a,n,"quaternion"))return out(r,NativeScriptType_Namespace,NativeScriptItemModel_QuaternionNamespace,NativeScriptFlag_SourceOnly);
        if(joint_namespace&&word(a,n,"rotation"))return out(r,NativeScriptType_Namespace,NativeScriptItemModel_RotationNamespace,NativeScriptFlag_SourceOnly);
        if(joint_namespace&&word(a,n,"translation"))return out(r,NativeScriptType_Namespace,NativeScriptItemModel_TranslationNamespace,NativeScriptFlag_SourceOnly);
        if(joint_namespace&&word(a,n,"scale"))return out(r,NativeScriptType_Namespace,NativeScriptItemModel_ScaleNamespace,NativeScriptFlag_SourceOnly);
        bool visible=joint_namespace&&(word(a,n,"visible")||word(a,n,"local_visible"));bool exists=joint_namespace&&word(a,n,"exists");
        bool axis=(base==NativeScriptItemModel_RotationNamespace||base==NativeScriptItemModel_ScaleNamespace||base==NativeScriptItemModel_TranslationNamespace)&&n->text_length==1&&strchr("xyz",a->source[n->text_offset]);
        bool quaternion=base==NativeScriptItemModel_QuaternionNamespace&&n->text_length==1&&strchr("xyzw",a->source[n->text_offset]);
        if(visible||exists||axis||quaternion){if(a->node_count>NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK)return error(d,n->source,"Item model property exceeds semantic capacity");
            uint16_t flags=NativeScriptFlag_Readable|NativeScriptFlag_SourceOnly;if(!exists)flags|=NativeScriptFlag_Writable|NativeScriptFlag_Addressable;
            const NativeScriptIRNode* owner=parent;
            if(!joint_namespace){if(parent->a>=a->node_count)return error(d,n->source,"Item joint projection parent differs");owner=&a->nodes[parent->a];}
            uint32_t domain=owner->semantic==NativeScriptItemModel_DynamicJointNamespace?NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY:NATIVE_SCRIPT_ITEM_MODEL_PROPERTY;
            return out(r,visible||exists?NativeScriptType_Bool:NativeScriptType_F32,domain|a->node_count,flags);}
    }
    return error(d,n->source,"Unsupported Item model projection");
}
bool nativeScriptBindingItemModel_PrepareSelectors(const NativeScriptCompileBank* bank,NativeScriptItemProgram* p,
    NativeScriptItemModelSelector* destination,uint32_t capacity,NativeScriptDiagnostic* d)
{
    if(!bank||!bank->valid||!p||p->bank!=bank||!p->article||!p->article->x10_modelDesc||
       bank->code.function_count!=bank->declarations.function_count||bank->code.functions!=bank->function_code||
       !p->owner||p->owner>=bank->tree.count)
        return error(d,(NativeScriptSourceSpan){0},"Item model preparation needs its exact owned Article descriptor and indexed code array")>0;
    NativeScriptItemModelSelector selected[NATIVE_SCRIPT_ITEM_MODEL_SELECTORS];unsigned count=0;
    p->clones_primary_model=false;
    const NativeScriptIRArena* a=&bank->expressions;
    for(uint32_t f=0;f<bank->code.function_count;++f)if(bank->declarations.functions[f].owner==p->owner) {
        const NativeScriptFunctionCode* fn=&bank->code.functions[f];
        for(uint32_t i=0;i<fn->instruction_count;++i){uint32_t semantic=bank->code.instructions[fn->first_instruction+i].semantic;
            if(semantic==NativeScriptBinding_ItemCloneModel)p->clones_primary_model=true;
            uint32_t domain=semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
            bool animation=domain>=NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD&&domain<=NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE;
            bool async=domain==NATIVE_SCRIPT_ITEM_MODEL_ASYNC_EFFECT;
            bool sync=domain==NATIVE_SCRIPT_ITEM_MODEL_SYNC_EFFECT;
            if(!animation&&!async&&!sync&&domain!=NATIVE_SCRIPT_ITEM_MODEL_PROPERTY&&domain!=NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY&&domain!=NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION&&domain!=NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION&&domain!=NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM)continue;
            unsigned at=0;while(at<count&&selected[at].semantic<semantic)++at;if(at<count&&selected[at].semantic==semantic)continue;
            if(count==NATIVE_SCRIPT_ITEM_MODEL_SELECTORS)return error(d,fn->source,"Item model selector capacity exhausted")>0;
            uint32_t id=semantic&NATIVE_SCRIPT_ITEM_MODEL_INDEX_MASK;if(id>=a->node_count)return error(d,fn->source,"Item model property IR identity is invalid")>0;
            const NativeScriptIRNode* property=&a->nodes[id];if(property->semantic!=semantic||property->a>=a->node_count)return error(d,property->source,"Item model selector IR semantic/parent differs")>0;const NativeScriptIRNode* parent=&a->nodes[property->a];NativeScriptItemModelSelector s={.semantic=semantic,.animation=UINT32_MAX};
            if(sync){
                if(parent->semantic!=NativeScriptBinding_ItemSpawnJointEffect||!sync_arguments(a,property,&s))
                    return error(d,property->source,"Synchronous Item effect selector ancestry/literal is invalid")>0;
            }else if(async){
                if(parent->semantic!=NativeScriptBinding_ItemSpawnAsyncJointEffect||!async_arguments(a,property,&s))
                    return error(d,property->source,"Async Item effect selector ancestry/literal is invalid")>0;
            }else if(animation){
                NativeScriptBindingSpawnResolverContext c={.program=&bank->declarations,.owner=p->owner,.bindings={NativeScriptBindings_Article}};
                if(!animation_arguments(&c,a,property,domain,&s,d)||!animation_targets(p,domain,&s,d,property->source))return false;
            }else if(domain==NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION||domain==NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM||domain==NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION){s.channel=11;
                uint32_t expected=domain==NATIVE_SCRIPT_ITEM_MODEL_WORLD_POSITION?NativeScriptItemModel_WorldPositionCall:domain==NATIVE_SCRIPT_ITEM_MODEL_MATRIX_ROTATION?NativeScriptBinding_ItemReadJointMatrixRotation:NativeScriptBinding_ItemSetJointTransform;
                uint32_t arity=domain==NATIVE_SCRIPT_ITEM_MODEL_TRANSFORM?13:5;
                if(parent->semantic!=expected||property->argument_count!=arity||!arguments(a,property,2,&s))
                    return error(d,property->source,"Item model call selector ancestry/literal is invalid")>0;}
            else {
            if(parent->semantic==NativeScriptItemModel_QuaternionNamespace){const char axes[]="xyzw";const char* axis=strchr(axes,a->source[property->text_offset]);if(!axis)return false;s.channel=(uint8_t)(11+(axis-axes));if(parent->a>=a->node_count)return false;parent=&a->nodes[parent->a];}
             else if(parent->semantic==NativeScriptItemModel_RotationNamespace||parent->semantic==NativeScriptItemModel_ScaleNamespace||parent->semantic==NativeScriptItemModel_TranslationNamespace){s.channel=(uint8_t)((parent->semantic==NativeScriptItemModel_TranslationNamespace?0:parent->semantic==NativeScriptItemModel_RotationNamespace?3:6)+(a->source[property->text_offset]-'x'));if(parent->a>=a->node_count)return false;parent=&a->nodes[parent->a];}
            else s.channel=word(a,property,"visible")?10:word(a,property,"local_visible")?15:9;
            if(domain==NATIVE_SCRIPT_ITEM_MODEL_DYNAMIC_PROPERTY){
                if(parent->semantic!=NativeScriptItemModel_DynamicJointNamespace||!dynamic_arguments(a,parent,&s))return error(d,property->source,"Dynamic joint selector ancestry/literal is invalid")>0;
            }else if(parent->semantic!=NativeScriptItemModel_JointNamespace||!arguments(a,parent,parent->argument_count,&s))return error(d,property->source,"Item model selector ancestry/literal is invalid")>0;
            }
            // A bounded path can legitimately be absent, including particle
            // Articles with a synthetic root and instance-transformed topology.
            // Preserve native missing-joint defaults/no-op writes; descriptors
            // are imported/validated by their owner, not treated as live JObjs.
            memmove(selected+at+1,selected+at,(count-at)*sizeof(*selected));selected[at]=s;++count;
        }
    }
    if(count>capacity||(count&&!destination))return error(d,(NativeScriptSourceSpan){0},"Item model aggregate selector capacity exhausted")>0;
    if(count)memcpy(destination,selected,count*sizeof(*selected));p->model_selectors=destination;p->model_selector_count=count;return true;
}
