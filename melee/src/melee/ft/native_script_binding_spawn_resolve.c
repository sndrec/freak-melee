#include "native_script_binding_spawn.h"
#include "native_script_constants.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

static int error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* message)
{if(diagnostic){diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);}return -1;}

static bool text(const NativeScriptSourceTree* tree,NativeScriptSourceSpan span,const NativeScriptIRArena* arena,const NativeScriptIRNode* node)
{return span.offset<=tree->source_bytes && span.length<=tree->source_bytes-span.offset && span.length==node->text_length && !memcmp(tree->source+span.offset,arena->source+node->text_offset,span.length);}
static int declaration(const NativeScriptBindingSpawnResolverContext* context,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* name,uint32_t* owner,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptSourceTree* tree=context->program->tree;*owner=0;
    for(uint32_t i=1;i<tree->count;++i) {
        const NativeScriptSourceNode* node=&tree->nodes[i];
        if(node->kind!=NativeScriptSource_Article || !text(tree,node->name,arena,name))continue;
        if(*owner)return error(diagnostic,name->source,"Authored factory Article declaration is ambiguous");
        *owner=i;
    }
    if(!*owner)return 0;
    for(uint32_t i=0;i<context->program->function_count;++i)
        if(!context->program->functions[i].owner && text(tree,context->program->functions[i].name,arena,name))
            return error(diagnostic,name->source,"Article identifier conflicts with a root helper/event declaration");
    for(uint32_t i=0;i<context->program->value_count;++i)
        if(!context->program->values[i].owner && text(tree,context->program->values[i].name,arena,name))
            return error(diagnostic,name->source,"Article identifier conflicts with a root state/parameter declaration");
    for(uint32_t i=1;i<NativeScriptBinding_Count;++i) {
        const NativeScriptBindingDescriptor* d=nativeScriptBindings_Descriptor(i);
        if(d && !d->parent && strlen(d->name)==name->text_length && !memcmp(d->name,arena->source+name->text_offset,name->text_length))
            return error(diagnostic,name->source,"Article identifier conflicts with a native name/namespace");
    }
    NativeScriptIRNode original=*name;original.kind=NativeScriptIR_Name;
    NativeScriptIRResolution constant;int known=nativeScriptConstants_TryResolve(NULL,arena,&original,&constant,diagnostic);
    if(known<0)return -1;
    if(known && (name->text_length<8 || memcmp(arena->source+name->text_offset,"It_Kind_",8)))
        return error(diagnostic,name->source,"Article identifier conflicts with a shared constant");
    return 1;
}

int nativeScriptBindingSpawn_TryResolve(void* opaque,const NativeScriptIRArena* arena,
    const NativeScriptIRNode* node,NativeScriptIRResolution* result,NativeScriptDiagnostic* diagnostic)
{
    if(!arena || !node)return 0;
    NativeScriptBindingSpawnResolverContext* context=opaque;
    const NativeScriptSourceTree* tree=context && context->program?context->program->tree:NULL;
    if(node->kind==NativeScriptIR_Name) {
        if(!tree)return 0;
        if(node->text_offset>arena->source_bytes || node->text_length>arena->source_bytes-node->text_offset)return error(diagnostic,node->source,"Article identifier source span is invalid");
        uint32_t owner;int found=declaration(context,arena,node,&owner,diagnostic);if(found<=0)return found;
        if(!result || context->program->expressions!=arena || !context->program->function_count ||
           tree->source!=arena->source || tree->source_bytes!=arena->source_bytes)
            return error(diagnostic,node->source,"Article identity literal requires the exact indexed body program/arena");
        if(owner>NATIVE_SCRIPT_SPAWN_OWNER_MASK)return error(diagnostic,node->source,"Article identity exceeds semantic capacity");
        *result=(NativeScriptIRResolution){.type=NativeScriptType_I32,.semantic=NATIVE_SCRIPT_SEMANTIC_NONE,.flags=NativeScriptFlag_Literal,.literal_bits=owner};
        return 1;
    }
    if(node->kind!=NativeScriptIR_Call || node->a>=arena->node_count)return 0;
    uint32_t semantic=arena->nodes[node->a].semantic,operation;
    switch(semantic) {
    case NativeScriptBinding_SpawnAuthoredProjectile:operation=NativeScriptSpawn_Projectile;break;
    case NativeScriptBinding_ArticlesLaunchAtPosition:operation=NativeScriptSpawn_LaunchAtPosition;break;
    case NativeScriptBinding_ArticlesLaunchProjectile:operation=NativeScriptSpawn_LaunchProjectile;break;
    case NativeScriptBinding_SpawnBoneAttached:operation=NativeScriptSpawn_Attached;break;
    case NativeScriptBinding_SpawnAttachedPresentation:operation=NativeScriptSpawn_AttachedPresentation;break;
    case NativeScriptBinding_ArticlesCreateAtPosition:
        operation=node->argument_count==8?NativeScriptSpawn_AtPositionData:NativeScriptSpawn_AtPosition;break;
    case NativeScriptBinding_ArticlesCreateWithVelocity:operation=NativeScriptSpawn_WithVelocity;break;
    case NativeScriptBinding_ArticlesCreateUnownedAtPosition:operation=NativeScriptSpawn_UnownedAtPosition;break;
    case NativeScriptBinding_ItemCreateAtPosition:operation=NativeScriptSpawn_ItemAtPosition;break;
    case NativeScriptBinding_ItemCreateChild:operation=NativeScriptSpawn_Child;break;
    case NativeScriptBinding_ItemCreateLinked:operation=NativeScriptSpawn_Linked;break;
    case NativeScriptBinding_ArticlesCreateHeldAtCenter:operation=NativeScriptSpawn_HeldAtCenter;break;
    default:return 0;
    }
    bool item=operation==NativeScriptSpawn_ItemAtPosition || operation==NativeScriptSpawn_Child || operation==NativeScriptSpawn_Linked;
    if(!result || !tree || context->program->expressions!=arena ||
       tree->source!=arena->source || tree->source_bytes!=arena->source_bytes ||
       (item ? (!context->owner || context->owner>=tree->count ||
           (tree->nodes[context->owner].kind!=NativeScriptSource_Article && tree->nodes[context->owner].kind!=NativeScriptSource_Item) ||
           context->bindings.context_mask!=NativeScriptBindings_Article) :
           (context->owner || context->bindings.context_mask!=NativeScriptBindings_Fighter)))
        return error(diagnostic,node->source,item?"Item factory requires its exact indexed Item source scope":"Authored factory requires the exact root Fighter source scope");
    if(!node->argument_count || node->argument_first>=arena->argument_count ||
       node->argument_count>arena->argument_count-node->argument_first)
        return error(diagnostic,node->source,"Authored factory argument range is invalid");
    uint32_t argument=arena->arguments[node->argument_first];
    if(argument>=arena->node_count)return error(diagnostic,node->source,"Authored factory kind argument is invalid");
    const NativeScriptIRNode* kind=&arena->nodes[argument];
    uint32_t owner=0;
    if(kind->text_length && kind->text_offset<=arena->source_bytes && kind->text_length<=arena->source_bytes-kind->text_offset) {
        int found=declaration(context,arena,kind,&owner,diagnostic);if(found<0)return -1;
    }
    if(kind->kind!=NativeScriptIR_Integer || kind->type!=NativeScriptType_I32 || !kind->text_length ||
       kind->text_offset>arena->source_bytes || kind->text_length>arena->source_bytes-kind->text_offset ||
       !(isalpha((unsigned char)arena->source[kind->text_offset]) || arena->source[kind->text_offset]=='_'))
        return error(diagnostic,kind->source,"Authored factory needs an identifier constant naming its Article declaration");
    if(!owner || owner>NATIVE_SCRIPT_SPAWN_OWNER_MASK)
        return error(diagnostic,kind->source,"Authored factory Article declaration is absent or exceeds semantic capacity");
    uint32_t selected=NATIVE_SCRIPT_SPAWN_DOMAIN|(operation<<20)|owner;
    if(operation==NativeScriptSpawn_AtPositionData || operation==NativeScriptSpawn_LaunchAtPosition) {
        if(node->argument_count!=(operation==NativeScriptSpawn_AtPositionData?8:10))
            return error(diagnostic,node->source,"Positional Article argument count does not match its signature");
        uint32_t group=arena->arguments[node->argument_first+(operation==NativeScriptSpawn_AtPositionData?6:8)];
        if(group>=arena->node_count || !nativeScriptExpression_IsIntegral(arena->nodes[group].type))
            return error(diagnostic,node->source,"Positional Article collision group requires an integral value");
    }
    if(!nativeScriptBindings_ResolveCall(&context->bindings,arena,node,selected,result,diagnostic))return -1;
    return 1;
}
