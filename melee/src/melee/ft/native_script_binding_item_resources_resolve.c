#include "native_script_binding_item_resources_resolve.h"
#include "native_script_binding_item_model_resolve.h"
#include "native_script_compile.h"
#include "native_script_item_program.h"
#include "native_script_expression.h"
#include <melee/it/native_article_resource.h>
#include <stdio.h>
#include <string.h>

#define R NativeScriptFlag_Readable
#define W (NativeScriptFlag_Writable|NativeScriptFlag_Addressable)
#define P NativeScriptFlag_Pure
#define S NativeScriptFlag_SourceOnly
#define A NativeScriptBindings_Article
#define ID(n) NativeScriptItemResource_##n
static int error(NativeScriptDiagnostic* d,NativeScriptSourceSpan at,const char* text)
{if(d){d->source=at;snprintf(d->message,sizeof(d->message),"%s",text);}return -1;}
static bool word(const NativeScriptIRArena* a,const NativeScriptIRNode* n,const char* text)
{return n->text_offset<=a->source_bytes&&n->text_length<=a->source_bytes-n->text_offset&&n->text_length==strlen(text)&&!memcmp(a->source+n->text_offset,text,n->text_length);}
static int resolved(NativeScriptIRResolution* r,NativeScriptIRType type,uint32_t semantic,uint16_t flags)
{*r=(NativeScriptIRResolution){.type=type,.semantic=semantic,.flags=flags};return 1;}
static bool owned(const NativeScriptBindingSpawnResolverContext* c,const NativeScriptIRArena* a)
{return c&&c->program&&c->program->tree&&c->program->expressions==a&&c->bindings.context_mask==A&&c->owner&&c->owner<c->program->tree->count&&
    (c->program->tree->nodes[c->owner].kind==NativeScriptSource_Article||c->program->tree->nodes[c->owner].kind==NativeScriptSource_Item);}
static const NativeScriptIRNode* argument(const NativeScriptIRArena* a,const NativeScriptIRNode* n,unsigned i)
{if(i>=n->argument_count||n->argument_first>a->argument_count||n->argument_count>a->argument_count-n->argument_first)return NULL;
 uint32_t id=a->arguments[n->argument_first+i];return id<a->node_count?&a->nodes[id]:NULL;}
static bool path(const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptItemResourceSelector* s)
{
    if(!n||n->kind!=NativeScriptIR_String||n->type!=NativeScriptType_Text||n->text_length>40||n->text_offset>a->source_bytes||n->text_length>a->source_bytes-n->text_offset)return false;
    s->path_offset=n->text_offset;s->path_length=n->text_length;
    unsigned at=0;s->depth=0;while(at<n->text_length){unsigned value=0,start=at;
        while(at<n->text_length&&a->source[n->text_offset+at]>='0'&&a->source[n->text_offset+at]<='9'){
            value=value*10+(unsigned)(a->source[n->text_offset+at++]-'0');if(value>255)return false;}
        if(start==at||s->depth==NATIVE_SCRIPT_ITEM_RESOURCE_PATH_DEPTH)return false;s->path[s->depth++]=(uint8_t)value;
        if(at<n->text_length&&(a->source[n->text_offset+at++]!='/'||at==n->text_length))return false;
    }return true;
}
static bool matrix_decl(const NativeScriptBindingSpawnResolverContext* c,const NativeScriptIRNode* n,uint32_t* id)
{
    if(!n||n->kind!=NativeScriptIR_Member||n->type!=NativeScriptType_Object||(n->semantic&NATIVE_SCRIPT_STORAGE_DOMAIN_MASK)!=NATIVE_SCRIPT_STATE_SEMANTIC)return false;
    *id=n->semantic&NATIVE_SCRIPT_STORAGE_INDEX_MASK;
    if(*id>=c->program->value_count)return false;
    const NativeScriptValueDeclaration* v=&c->program->values[*id];return v->owner==c->owner&&!v->parameter&&v->array&&v->count==12&&v->type==NativeScriptType_F32;
}
static bool model_literal(const NativeScriptBindingSpawnResolverContext* c,const NativeScriptIRArena* a,const NativeScriptIRNode* n,bool animation,NativeScriptDiagnostic* d)
{
    if(!n||n->kind!=NativeScriptIR_Integer||n->type!=NativeScriptType_I32||!n->text_length||n->text_offset>a->source_bytes||n->text_length>a->source_bytes-n->text_offset)return false;
    const NativeScriptSourceTree* t=c->program->tree;uint32_t node=t->nodes[c->owner].first_child;
    unsigned bound=0;while(node!=UINT32_MAX){if(node>=t->count||bound++==t->count)return false;NativeScriptModelMetadata metadata;uint32_t ordinal;
        if(t->nodes[node].kind==NativeScriptSource_Metadata&&nativeScriptSource_SpanEquals(t,t->nodes[node].name,animation?"model_animation":"model_root")&&
           nativeScriptBindingItemModel_Declaration(t,c->owner,node,&metadata,&ordinal,d)&&metadata.name.length==n->text_length&&
           !memcmp(t->source+metadata.name.offset,a->source+n->text_offset,metadata.name.length))return n->bits==ordinal;
        node=t->nodes[node].next;}
    return false;
}
static bool same(const NativeScriptIRArena* a,uint32_t x,uint32_t y,unsigned depth)
{
    if(x==y)return true;if(depth==NATIVE_SCRIPT_EXPRESSION_DEPTH||x>=a->node_count||y>=a->node_count)return false;
    const NativeScriptIRNode* l=&a->nodes[x],*r=&a->nodes[y];if(l->kind!=r->kind||l->type!=r->type||l->semantic!=r->semantic||l->bits!=r->bits||l->operation!=r->operation||l->argument_count!=r->argument_count)return false;
    if((l->a==UINT32_MAX)!=(r->a==UINT32_MAX)||(l->b==UINT32_MAX)!=(r->b==UINT32_MAX))return false;
    if(l->a!=UINT32_MAX&&!same(a,l->a,r->a,depth+1))return false;if(l->b!=UINT32_MAX&&!same(a,l->b,r->b,depth+1))return false;
    for(unsigned i=0;i<l->argument_count;++i)if(!same(a,a->arguments[l->argument_first+i],a->arguments[r->argument_first+i],depth+1))return false;return true;
}
static bool call(const NativeScriptBindingSpawnResolverContext* c,const NativeScriptIRArena* a,const NativeScriptIRNode* n,uint32_t id,NativeScriptIRResolution* r,NativeScriptDiagnostic* d)
{
    if(id==ID(Create)&&n->argument_count==6)id=ID(CreateAt);
    const NativeScriptBindingDescriptor* spec=nativeScriptBindingItemResources_Descriptor(id);
    if(!spec||spec->kind!=NativeScriptBinding_Intrinsic||n->argument_count!=spec->argument_count)return error(d,n->source,"Resource call arity differs from its typed contract")>0;
    bool matrix=id>=ID(AttachmentMatrix)&&id<=ID(SetWorldMatrix);uint32_t declaration;
    if(matrix){if(!matrix_decl(c,argument(a,n,0),&declaration))return error(d,n->source,"Resource matrix needs its exact owned twelve-Float state array")>0;}
    else for(unsigned i=0;i<n->argument_count;++i){const NativeScriptIRNode* v=argument(a,n,i);const NativeScriptBindingArgument* p=&spec->arguments[i];
        if(!v)return false;
        if(p->numeric_any){if(!nativeScriptExpression_IsIntegral(v->type))return error(d,v->source,"Resource indices/parts/categories/effect IDs require integral arguments")>0;}
        else if(p->mode==NativeScriptArgument_Output){if(v->type!=NativeScriptType_F32||(v->flags&(R|W))!=(R|W))return error(d,v->source,"Resource output needs writable Float storage")>0;
            for(unsigned j=0;j<i;++j)if(spec->arguments[j].mode==NativeScriptArgument_Output&&same(a,a->arguments[n->argument_first+i],a->arguments[n->argument_first+j],0))return error(d,v->source,"Resource outputs must be distinct destinations")>0;}
        else if(!nativeScriptExpression_Assignable((NativeScriptIRType)p->type,(NativeScriptIRType)v->type))return error(d,v->source,"Resource argument type differs")>0;
    }
    if(id==ID(Joint)){NativeScriptItemResourceSelector s={0};if(!path(a,argument(a,n,0),&s))return error(d,n->source,"Resource joint needs a bounded literal ordinal path")>0;}
    if(id==ID(LoadModel)&&!model_literal(c,a,argument(a,n,0),false,d))return error(d,n->source,"Resource model needs an exact same-owner declared root identifier")>0;
    bool animation=id>=ID(AddAnimation)&&id<=ID(SetAnimationRate);
    if(id==ID(AddAnimation)&&!model_literal(c,a,argument(a,n,0),true,d))return error(d,n->source,"Resource animation needs an exact same-owner declared bundle identifier")>0;
    if((matrix||animation)&&a->node_count>NATIVE_SCRIPT_ITEM_RESOURCES_INDEX_MASK)return error(d,n->source,"Resource call semantic capacity exhausted")>0;
    uint32_t semantic=matrix?(NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX|a->node_count):animation?(NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION|a->node_count):id;
    NativeScriptIRType type=(NativeScriptIRType)spec->result_type;
    if(id==ID(Resource)){semantic=ID(Node);type=NativeScriptType_Namespace;}
    if(id==ID(Joint)){semantic=ID(JointNode);type=NativeScriptType_Namespace;}
    *r=(NativeScriptIRResolution){.type=type,.semantic=semantic,.flags=spec->flags};return true;
}
int nativeScriptBindingItemResources_TryResolve(void* opaque,const NativeScriptIRArena* a,const NativeScriptIRNode* n,NativeScriptIRResolution* r,NativeScriptDiagnostic* d)
{
    if(!a||!n||!a->source||n->a>=a->node_count)return 0;
    uint32_t base=a->nodes[n->a].semantic;bool root=n->kind==NativeScriptIR_Member&&base==NATIVE_SCRIPT_ITEM_NAMESPACE&&(word(a,n,"resources")||word(a,n,"resource"));
    if(!root&&(base&NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN_MASK)!=NATIVE_SCRIPT_ITEM_RESOURCES_DOMAIN)return 0;
    const NativeScriptBindingSpawnResolverContext* c=opaque;if(!r||!owned(c,a))return error(d,n->source,"Resource expressions require the exact indexed Item/Article owner");
    if(n->kind==NativeScriptIR_Call)return call(c,a,n,base,r,d)?1:-1;
    if(n->kind!=NativeScriptIR_Member)return error(d,n->source,"Unsupported resource selector projection");
    if(base==ID(JointNode)||base==ID(Translation)||base==ID(Rotation)||base==ID(Scale)||base==ID(Quaternion)){
        unsigned channel=UINT32_MAX;bool boolean=false;
        if(base==ID(JointNode)){
            for(uint32_t id=ID(AddAnimation);id<=ID(SetAnimationRate);++id){const NativeScriptBindingDescriptor* spec=nativeScriptBindingItemResources_Descriptor(id);if(word(a,n,spec->name))return resolved(r,NativeScriptType_Callable,id,spec->flags);}
            if(word(a,n,"translation"))return resolved(r,NativeScriptType_Namespace,ID(Translation),S);
            if(word(a,n,"rotation"))return resolved(r,NativeScriptType_Namespace,ID(Rotation),S);
            if(word(a,n,"scale"))return resolved(r,NativeScriptType_Namespace,ID(Scale),S);
            if(word(a,n,"quaternion"))return resolved(r,NativeScriptType_Namespace,ID(Quaternion),S);
            if(word(a,n,"exists")){channel=9;boolean=true;}else if(word(a,n,"visible")){channel=10;boolean=true;}else if(word(a,n,"self_visible")){channel=15;boolean=true;}
        }else if(n->text_length==1){const char* axes=base==ID(Quaternion)?"xyzw":"xyz";const char* axis=strchr(axes,a->source[n->text_offset]);if(axis)channel=(unsigned)(axis-axes)+(base==ID(Translation)?0:base==ID(Rotation)?3:base==ID(Scale)?6:11);}
        if(channel!=UINT32_MAX){if(a->node_count>NATIVE_SCRIPT_ITEM_RESOURCES_INDEX_MASK)return error(d,n->source,"Resource joint semantic capacity exhausted");return resolved(r,boolean?NativeScriptType_Bool:NativeScriptType_F32,NATIVE_SCRIPT_ITEM_RESOURCES_PROPERTY|a->node_count,R|P|S|(channel==9?0:W));}
        return error(d,n->source,"Unknown resource joint channel");
    }
    for(uint32_t id=ID(Resources);id<ID(StaticEnd);++id){const NativeScriptBindingDescriptor* spec=nativeScriptBindingItemResources_Descriptor(id);if(spec->parent==base&&word(a,n,spec->name))return resolved(r,(spec->kind==NativeScriptBinding_Intrinsic?NativeScriptType_Callable:(NativeScriptIRType)spec->result_type),id,spec->flags);}
    for(uint32_t id=ID(FloatFirst);id<ID(FloatFirst)+NativeArticleResource_FloatCount;++id){const NativeScriptBindingDescriptor* spec=nativeScriptBindingItemResources_Descriptor(id);if(spec->parent==base&&word(a,n,spec->name))return resolved(r,(NativeScriptIRType)spec->result_type,id,spec->flags);}
    return error(d,n->source,"Unknown resource operation/property");
}
bool nativeScriptBindingItemResources_DescribeCall(void* opaque,uint32_t semantic,NativeScriptCallLayout* out,NativeScriptDiagnostic* d)
{
    const NativeScriptBindingSpawnResolverContext* c=opaque;if(!c||!c->program||!out||!owned(c,c->program->expressions))return error(d,(NativeScriptSourceSpan){0},"Resource call layout needs its exact indexed source owner")>0;
    NativeScriptCallLayout layout={0};uint32_t id=semantic;
    uint32_t domain=semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK;
    if(domain==NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX||domain==NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION){const NativeScriptIRArena* a=c->program->expressions;uint32_t index=semantic&NATIVE_SCRIPT_ITEM_RESOURCES_INDEX_MASK,declaration;
        if(domain==NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION){
            if(index>=a->node_count||a->nodes[index].semantic!=semantic||a->nodes[index].a>=a->node_count)return error(d,(NativeScriptSourceSpan){0},"Resource animation layout differs from its immutable call")>0;
            id=a->nodes[a->nodes[index].a].semantic;
            if(id<ID(AddAnimation)||id>ID(SetAnimationRate))return error(d,a->nodes[index].source,"Resource animation operation differs")>0;
        }else{
        if(index>=a->node_count||a->nodes[index].semantic!=semantic||a->nodes[index].a>=a->node_count||!matrix_decl(c,argument(a,&a->nodes[index],0),&declaration))return error(d,(NativeScriptSourceSpan){0},"Resource matrix layout differs from its immutable owner declaration")>0;
        id=a->nodes[a->nodes[index].a].semantic;layout.constant_argument_mask=1;layout.constant_arguments[0]=declaration;
        }
    }
    const NativeScriptBindingDescriptor* spec=nativeScriptBindingItemResources_Descriptor(id);if(!spec||spec->kind!=NativeScriptBinding_Intrinsic)return error(d,(NativeScriptSourceSpan){0},"Unknown resource call layout")>0;
    layout.argument_count=spec->argument_count;layout.publication=spec->publication;
    for(unsigned i=0;i<spec->argument_count;++i){layout.argument_types[i]=spec->arguments[i].numeric_any?NativeScriptType_Invalid:spec->arguments[i].type;if(spec->arguments[i].mode==NativeScriptArgument_Output)layout.output_mask|=UINT32_C(1)<<i;}
    *out=layout;return true;
}
bool nativeScriptBindingItemResources_PrepareSelectors(const NativeScriptCompileBank* bank,NativeScriptItemProgram* p,NativeScriptItemResourceSelector* destination,uint32_t capacity,NativeScriptDiagnostic* d)
{
    if(!bank||!bank->valid||!p||p->bank!=bank||bank->code.function_count!=bank->declarations.function_count||!p->owner||p->owner>=bank->tree.count)return error(d,(NativeScriptSourceSpan){0},"Resource recipes need the exact owned compiled bank")>0;
    NativeScriptItemResourceSelector recipes[NATIVE_SCRIPT_ITEM_RESOURCE_SELECTORS];unsigned count=0;const NativeScriptIRArena* a=&bank->expressions;
    NativeScriptBindingSpawnResolverContext context={.program=&bank->declarations,.owner=p->owner,.bindings={A}};
    for(unsigned f=0;f<bank->code.function_count;++f)if(bank->declarations.functions[f].owner==p->owner){const NativeScriptFunctionCode* fn=&bank->code.functions[f];
        for(unsigned i=0;i<fn->instruction_count;++i){uint32_t semantic=bank->code.instructions[fn->first_instruction+i].semantic,domain=semantic&NATIVE_SCRIPT_ITEM_RESOURCES_OPERATION_MASK;
            if(domain!=NATIVE_SCRIPT_ITEM_RESOURCES_PROPERTY&&domain!=NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX&&domain!=NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION)continue;
            unsigned at=0;while(at<count&&recipes[at].semantic<semantic)++at;if(at<count&&recipes[at].semantic==semantic)continue;if(count==NATIVE_SCRIPT_ITEM_RESOURCE_SELECTORS)return error(d,fn->source,"Resource selector capacity exhausted")>0;
            uint32_t id=semantic&NATIVE_SCRIPT_ITEM_RESOURCES_INDEX_MASK;if(id>=a->node_count||a->nodes[id].semantic!=semantic)return error(d,fn->source,"Resource selector IR identity differs")>0;
            const NativeScriptIRNode* n=&a->nodes[id];NativeScriptItemResourceSelector s={.semantic=semantic,.declaration=UINT32_MAX,.animation=UINT32_MAX};
            if(domain==NATIVE_SCRIPT_ITEM_RESOURCES_MATRIX){if(n->a>=a->node_count||!matrix_decl(&context,argument(a,n,0),&s.declaration))return error(d,n->source,"Resource matrix declaration differs")>0;
                uint32_t op=a->nodes[n->a].semantic;if(op<ID(AttachmentMatrix)||op>ID(SetWorldMatrix))return error(d,n->source,"Resource matrix operation differs")>0;s.operation=(uint8_t)(op-ID(Resources));
            }else{if(n->a>=a->node_count)return false;const NativeScriptIRNode* parent=&a->nodes[n->a];
                if(domain==NATIVE_SCRIPT_ITEM_RESOURCES_ANIMATION){
                    uint32_t op=parent->semantic;if(op<ID(AddAnimation)||op>ID(SetAnimationRate)||parent->a>=a->node_count)return error(d,n->source,"Resource animation ancestry differs")>0;
                    s.operation=(uint8_t)(op-ID(Resources));
                    if(op==ID(AddAnimation)){const NativeScriptIRNode* bundle=argument(a,n,0);if(!model_literal(&context,a,bundle,true,d)||bundle->bits>=p->model_animation_count)return error(d,n->source,"Resource animation bundle differs from its owned asset table")>0;s.animation=(uint32_t)bundle->bits;}
                    parent=&a->nodes[parent->a];
                }else{
                if(parent->semantic==ID(Translation)||parent->semantic==ID(Rotation)||parent->semantic==ID(Scale)||parent->semantic==ID(Quaternion)){
                    const char* axes=parent->semantic==ID(Quaternion)?"xyzw":"xyz";const char* axis=strchr(axes,a->source[n->text_offset]);if(!axis)return false;s.channel=(uint8_t)(axis-axes+(parent->semantic==ID(Translation)?0:parent->semantic==ID(Rotation)?3:parent->semantic==ID(Scale)?6:11));
                    if(parent->a>=a->node_count)return false;parent=&a->nodes[parent->a];
                }else s.channel=word(a,n,"exists")?9:word(a,n,"visible")?10:15;
                }
                if(parent->semantic!=ID(JointNode)||!path(a,argument(a,parent,0),&s))return error(d,n->source,"Resource joint literal ancestry differs")>0;
            }
            memmove(recipes+at+1,recipes+at,(count-at)*sizeof(*recipes));recipes[at]=s;++count;
        }
    }
    if(count>capacity||(count&&!destination))return error(d,(NativeScriptSourceSpan){0},"Resource aggregate recipe capacity exhausted")>0;
    if(count)memcpy(destination,recipes,count*sizeof(*recipes));p->resource_selectors=destination;p->resource_selector_count=count;return true;
}
