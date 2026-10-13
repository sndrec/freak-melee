#include "native_script_item_program.h"
#include "native_script_pose_metadata.h"
#include "native_script_resource_metadata.h"
#include "native_source_declarations.h"
#include "native_source_motion_policy.h"
#include "native_script_binding_item_model_resolve.h"
#include <melee/it/types.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Words {const NativeScriptSourceTree* tree;NativeScriptSourceSpan span;uint32_t at;} Words;
static bool error(NativeScriptDiagnostic* diagnostic,NativeScriptSourceSpan source,const char* message)
{if(diagnostic){diagnostic->source=source;snprintf(diagnostic->message,sizeof(diagnostic->message),"%s",message);}return false;}
static NativeScriptSourceSpan word(Words* cursor)
{
    const char* text=cursor->tree->source+cursor->span.offset;
    while(cursor->at<cursor->span.length && text[cursor->at]==' ')++cursor->at;
    uint32_t begin=cursor->at;while(cursor->at<cursor->span.length && text[cursor->at]!=' ')++cursor->at;
    NativeScriptSourceSpan out=cursor->span;out.offset+=begin;out.column+=begin;out.length=cursor->at-begin;return out;
}
static bool same(const NativeScriptSourceTree* tree,NativeScriptSourceSpan name,const char* text)
{return text && strlen(text)==name.length && !memcmp(tree->source+name.offset,text,name.length);}
static bool number(Words* cursor,int32_t minimum,int32_t maximum,int32_t* value)
{
    NativeScriptSourceSpan token=word(cursor);if(!token.length || token.length>=32)return false;
    char text[32];memcpy(text,cursor->tree->source+token.offset,token.length);text[token.length]=0;
    char* end;long long parsed=strtoll(text,&end,0);if(*end || parsed<minimum || parsed>maximum)return false;
    *value=(int32_t)parsed;return true;
}
static bool real(Words* cursor,float* value)
{
    NativeScriptSourceSpan token=word(cursor);if(!token.length || token.length>=64)return false;
    char text[64];memcpy(text,cursor->tree->source+token.offset,token.length);text[token.length]=0;
    char* end;float parsed=strtof(text,&end);if(*end || !isfinite(parsed))return false;*value=parsed;return true;
}
static const NativeScriptItemAsset* asset(const NativeScriptItemMetadataContext* context,
    const NativeScriptSourceTree* tree,uint32_t owner)
{
    if(!context || context->asset_count>NATIVE_SCRIPT_ENTITY_LIMIT ||
        (context->asset_count && !context->assets) || owner>=tree->count)return NULL;
    const NativeScriptItemAsset* found=NULL;
    for(uint32_t i=0;i<context->asset_count;++i)if(same(tree,tree->nodes[owner].name,context->assets[i].declaration)) {
        if(found)return NULL;found=&context->assets[i];
    }
    return found;
}
static bool staticModelRoots(const NativeScriptSourceTree* tree,uint32_t owner,
    const NativeScriptItemAsset* binding,NativeScriptDiagnostic* diagnostic)
{
    if(binding->model_root_count>NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT||
       (binding->model_root_count&&!binding->model_roots)||(!binding->model_root_count&&binding->model_roots))
        return error(diagnostic,tree->nodes[owner].source,"Static root asset table is invalid");
    uint32_t count=0;
    for(uint32_t child=tree->nodes[owner].first_child;child!=UINT32_MAX;child=tree->nodes[child].next){
        if(child>=tree->count)return error(diagnostic,tree->nodes[owner].source,"Static root source child is invalid");
        const NativeScriptSourceNode* field=&tree->nodes[child];
        if(field->kind!=NativeScriptSource_Metadata||!same(tree,field->name,"model_root"))continue;
        NativeScriptModelMetadata metadata;uint32_t ordinal;
        if(!nativeScriptBindingItemModel_Declaration(tree,owner,child,&metadata,&ordinal,diagnostic))return false;
        if(binding->owner_node!=owner||ordinal!=count||ordinal>=binding->model_root_count)
            return error(diagnostic,field->source,"Static root does not match its exact prepared owner/ordinal");
        const NativeScriptItemModelRootAsset* record=&binding->model_roots[ordinal];
        if(!record->root||!same(tree,metadata.name,record->name)||record->attribute_offset!=metadata.root_offset)
            return error(diagnostic,field->source,"Static root name/offset/graph differs from its prepared asset");
        ++count;
    }
    if(count!=binding->model_root_count)return error(diagnostic,tree->nodes[owner].source,"Static root asset/source table count differs");
    return true;
}
static bool staticModelAnimations(const NativeScriptSourceTree* tree,uint32_t owner,
    const NativeScriptItemAsset* binding,NativeScriptDiagnostic* diagnostic)
{
    if(binding->model_animation_count>NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT||
       (binding->model_animation_count&&!binding->model_animations)||
       (!binding->model_animation_count&&binding->model_animations))
        return error(diagnostic,tree->nodes[owner].source,"Animation asset table is invalid");
    uint32_t count=0;
    for(uint32_t child=tree->nodes[owner].first_child,bound=0;child!=UINT32_MAX;child=tree->nodes[child].next){
        if(child>=tree->count||bound++==tree->count)return error(diagnostic,tree->nodes[owner].source,"Animation source child index/cycle is invalid");
        const NativeScriptSourceNode* field=&tree->nodes[child];
        if(field->kind!=NativeScriptSource_Metadata||!same(tree,field->name,"model_animation"))continue;
        NativeScriptModelMetadata metadata;uint32_t ordinal;
        if(!nativeScriptBindingItemModel_Declaration(tree,owner,child,&metadata,&ordinal,diagnostic))return false;
        if(binding->owner_node!=owner||ordinal!=count||ordinal>=binding->model_animation_count)
            return error(diagnostic,field->source,"Animation does not match its exact prepared owner/ordinal");
        const NativeScriptItemModelAnimationAsset* record=&binding->model_animations[ordinal];
        if(!same(tree,metadata.name,record->name)||memcmp(record->components,metadata.components,sizeof(metadata.components)))
            return error(diagnostic,field->source,"Animation name/components differ from its prepared asset");
        // Imported NULL Joint/material/shape channels are legitimate omissions.
        ++count;
    }
    if(count!=binding->model_animation_count)return error(diagnostic,tree->nodes[owner].source,"Animation asset/source table count differs");
    return true;
}
static bool animationOrigins(NativeScriptItemPrograms* candidate,
    NativeScriptItemProgram* program,NativeScriptDiagnostic* diagnostic)
{
    bool primary=program->animation_descriptors_prepared && program->article->x10_modelDesc &&
        program->article->x10_modelDesc->x0_joint,secondary=false;
    for(uint32_t i=0;i<program->model_selector_count;++i){
        const NativeScriptItemModelSelector* selector=&program->model_selectors[i];
        uint32_t domain=selector->semantic&NATIVE_SCRIPT_ITEM_MODEL_PROPERTY_MASK;
        if(domain<NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_ADD||domain>NATIVE_SCRIPT_ITEM_MODEL_ANIMATION_RATE)continue;
        if(selector->model)secondary=true;else primary=true;
    }
    uint32_t begin=candidate->animation_origin_count;
    unsigned first=primary?0:1,last=secondary?program->model_root_count:0;
    for(unsigned i=first;i<=last;++i){
        const HSD_Joint* descriptor=i?program->model_roots[i-1].root:program->article->x10_modelDesc->x0_joint;
        bool duplicate=false;
        for(unsigned previous=first;previous<i;++previous){
            const HSD_Joint* prior=previous?program->model_roots[previous-1].root:program->article->x10_modelDesc->x0_joint;
            if(prior==descriptor){duplicate=true;break;}
        }
        if(duplicate)continue;
        uint32_t written=0;
        if(!nativeArticleAnimation_PrepareOrigins(descriptor,program,
            candidate->animation_origins+candidate->animation_origin_count,
            NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS-candidate->animation_origin_count,&written))
            return error(diagnostic,program->bank->tree.nodes[program->owner].source,"Animation controller origin preparation/capacity failed");
        candidate->animation_origin_count+=written;
    }
    program->animation_origin_count=candidate->animation_origin_count-begin;
    if(program->animation_origin_count)program->animation_origins=candidate->animation_origins+begin;
    return true;
}
static bool header(const NativeScriptSourceTree* tree,uint32_t owner,uint32_t* states,NativeItemDefinition* recipe)
{
    const NativeScriptSourceNode* node=&tree->nodes[owner];Words words={tree,node->value,0};int32_t count;
    if(node->kind==NativeScriptSource_Article) {
        if(!same(tree,word(&words),"states") || !number(&words,1,NATIVE_SCRIPT_ITEM_STATES,&count) || word(&words).length)return false;
        *states=(uint32_t)count;return true;
    }
    if(node->kind!=NativeScriptSource_Item)return false;
    *states=4;unsigned seen=0;
    *recipe=(NativeItemDefinition){.billboard_size=8,.hitbox_size=4,.knockback_growth=70,.base_knockback=25};
    for(NativeScriptSourceSpan key=word(&words);key.length;key=word(&words)) {
        unsigned bit=same(tree,key,"damage")?1:same(tree,key,"angle")?2:same(tree,key,"lifetime")?4:0;
        if(!bit || (seen&bit) || !number(&words,0,UINT16_MAX,&count))return false;
        seen|=bit;if(bit==1)recipe->damage=(uint16_t)count;else if(bit==2)recipe->angle=(uint16_t)count;else recipe->lifetime_frames=(uint16_t)count;
    }
    return seen==7;
}
static bool articleState(const NativeScriptSourceTree* tree,const NativeScriptSourceNode* node,
    uint32_t count,uint32_t* state,int32_t* animation,uint32_t* alias)
{
    Words words={tree,node->value,0};int32_t id;
    if(!number(&words,0,(int32_t)count-1,&id))return false;*state=(uint32_t)id;*animation=-1;*alias=UINT32_MAX;
    NativeScriptSourceSpan key=word(&words);
    if(!same(tree,key,"anim") || !number(&words,-1,INT32_MAX,animation))return false;
    key=word(&words);
    if(key.length && same(tree,key,"use")) {
        if(!number(&words,0,(int32_t)count-1,&id))return false;*alias=(uint32_t)id;key=word(&words);
    }
    return !key.length;
}
static bool recipeField(const NativeScriptSourceTree* tree,const NativeScriptSourceNode* node,
    const NativeScriptItemAsset* binding,NativeItemDefinition* recipe)
{
    Words words={tree,node->value,0};int32_t integer;float value;
    if(same(tree,node->name,"blueprint"))
        return binding->blueprint_path&&same(tree,word(&words),binding->blueprint_path)&&!word(&words).length;
    if(same(tree,node->name,"icon")) {
        if(!binding->icon_path || !binding->icon_png || !binding->icon_bytes || !same(tree,word(&words),binding->icon_path) || word(&words).length)return false;
        recipe->billboard_png=binding->icon_png;recipe->billboard_png_size=binding->icon_bytes;return true;
    }
    if(same(tree,node->name,"billboard_size") || same(tree,node->name,"hitbox_size")) {
        if(!real(&words,&value) || value<=0 || word(&words).length)return false;
        if(same(tree,node->name,"billboard_size"))recipe->billboard_size=value;else recipe->hitbox_size=value;return true;
    }
    if(same(tree,node->name,"knockback_growth") || same(tree,node->name,"base_knockback")) {
        if(!number(&words,0,UINT16_MAX,&integer) || word(&words).length)return false;
        if(same(tree,node->name,"knockback_growth"))recipe->knockback_growth=(uint16_t)integer;else recipe->base_knockback=(uint16_t)integer;return true;
    }
    return false;
}
static bool switchValue(Words* words,bool* value)
{
    NativeScriptSourceSpan token=word(words);
    if(same(words->tree,token,"on")){*value=true;return true;}
    if(same(words->tree,token,"off")){*value=false;return true;}return false;
}
static bool scalar(Words* words,const NativeScriptCompileBank* bank,uint32_t owner,NativeScriptItemScalar* result)
{
    NativeScriptSourceSpan token=word(words);if(!token.length || token.length>=64)return false;
    char text[64];memcpy(text,words->tree->source+token.offset,token.length);text[token.length]=0;
    char* end;float value=strtof(text,&end);
    if(end!=text && !*end){if(!isfinite(value))return false;*result=(NativeScriptItemScalar){value,UINT32_MAX};return true;}
    if(!(isalpha((unsigned char)text[0])||text[0]=='_'))return false;
    for(unsigned i=1;text[i];++i)if(!(isalnum((unsigned char)text[i])||text[i]=='_'))return false;
    if(!bank){*result=(NativeScriptItemScalar){0,0};return true;}
    for(unsigned i=0;i<bank->declarations.value_count;++i){const NativeScriptValueDeclaration* field=&bank->declarations.values[i];
        if(field->owner!=owner || !field->parameter || field->array || field->count!=1 || field->type!=NativeScriptType_F32 || !same(words->tree,field->name,text))continue;
        if(i>=bank->storage_parameters.count || !bank->storage_parameters.slots[i].resolved ||
           bank->storage_parameters.slots[i].value.type!=NativeScriptType_F32 || !isfinite(bank->storage_parameters.slots[i].value.as.f32))return false;
        *result=(NativeScriptItemScalar){bank->storage_parameters.slots[i].value.as.f32,NATIVE_SCRIPT_PARAMETER_SEMANTIC|i};return true;
    }
    return false;
}
static bool projectileField(const NativeScriptSourceTree* tree,const NativeScriptSourceNode* node,
    const NativeScriptItemAsset* binding,uint32_t states,const NativeScriptCompileBank* bank,uint32_t owner,
    NativeScriptItemProjectile* output)
{
    Words words={tree,node->value,0};NativeScriptItemProjectile recipe={.auto_lifetime=true,.initial_velocity=true,.initial_update=true};
    uint32_t seen=0;int32_t value;
    for(NativeScriptSourceSpan key=word(&words);key.length;key=word(&words)){
        uint32_t bit=same(tree,key,"appearance")?1:same(tree,key,"state")?2:same(tree,key,"speed")?4:
            same(tree,key,"angle")?8:same(tree,key,"lifetime")?16:same(tree,key,"auto_lifetime")?32:
            same(tree,key,"initial_velocity")?64:same(tree,key,"initial_update")?128:0;
        if(!bit || (seen&bit))return false;seen|=bit;
        if(bit==1){if(!number(&words,-1,INT32_MAX,&recipe.appearance_state))return false;}
        else if(bit==2){if(!number(&words,0,(int32_t)states-1,&value))return false;recipe.item_state=(uint32_t)value;}
        else if(bit==4){if(!scalar(&words,bank,owner,&recipe.speed))return false;}
        else if(bit==8){if(!scalar(&words,bank,owner,&recipe.angle))return false;}
        else if(bit==16){if(!scalar(&words,bank,owner,&recipe.lifetime))return false;}
        else if(!switchValue(&words,bit==32?&recipe.auto_lifetime:bit==64?&recipe.initial_velocity:&recipe.initial_update))return false;
    }
    if((seen&31)!=31 || (recipe.appearance_state>=0 && (!binding->article->xC_itemStates ||
        (uint32_t)recipe.appearance_state>=binding->article->xC_itemStates->count)))return false;
    if((bank || recipe.lifetime.parameter_semantic==UINT32_MAX) && recipe.lifetime.value<=0)return false;
    *output=recipe;return true;
}
static bool attachmentField(const NativeScriptSourceTree* tree,const NativeScriptSourceNode* node,
    NativeScriptItemAttachment* output)
{
    Words words={tree,node->value,0};NativeScriptItemAttachment recipe={.initial_collision=true,.register_as_held=true};uint32_t seen=0;
    for(NativeScriptSourceSpan key=word(&words);key.length;key=word(&words)){
        uint32_t bit=same(tree,key,"origin")?1:same(tree,key,"previous")?2:same(tree,key,"plane")?4:
            same(tree,key,"initial_collision")?8:same(tree,key,"held")?16:0;
        if(!bit || (seen&bit))return false;seen|=bit;
        if(bit==1){NativeScriptSourceSpan value=word(&words);
            if(same(tree,value,"owner"))recipe.origin_at_owner=true;
            else if(same(tree,value,"center"))recipe.origin_at_center=true;
            else if(!same(tree,value,"bone"))return false;
        }else if(bit==2){NativeScriptSourceSpan value=word(&words);
            if(same(tree,value,"bone"))recipe.previous_at_bone=true;else if(!same(tree,value,"origin"))return false;
        }else if(!switchValue(&words,bit==4?&recipe.flatten_z:bit==8?&recipe.initial_collision:&recipe.register_as_held))return false;
    }
    if(!(seen&1))return false;*output=recipe;return true;
}
static uint32_t enclosing(const NativeScriptSourceTree* tree,uint32_t node)
{
    for(unsigned depth=0;node && node<tree->count && depth<NATIVE_SCRIPT_EXPRESSION_DEPTH;++depth,node=tree->nodes[node].parent)
        if(tree->nodes[node].kind==NativeScriptSource_Article || tree->nodes[node].kind==NativeScriptSource_Item)return node;
    return 0;
}
bool nativeScriptItemMetadata_Validate(void* opaque,const NativeScriptSourceTree* tree,uint32_t at,NativeScriptDiagnostic* diagnostic)
{
    const NativeScriptItemMetadataContext* context=opaque;
    if(!tree || at>=tree->count)return false;
    if(nativeSourceMotionPolicy_Owns(tree,at))
        return nativeSourceMotionPolicy_Validate(tree,at,context?context->source_assets:NULL,diagnostic);
    if(nativeSourceDeclarations_Owns(tree,at))
        return nativeSourceDeclarations_Validate(tree,at,context?context->source_facts:NULL,diagnostic);
    const NativeScriptSourceNode* node=&tree->nodes[at];uint32_t owner=enclosing(tree,at);
    if(!owner)return nativeScriptMoves_Metadata((void*)(context?context->motions:NULL),tree,at,diagnostic);
    const NativeScriptItemAsset* binding=asset(context,tree,owner);uint32_t count;NativeItemDefinition recipe;
    if(!binding)return error(diagnostic,tree->nodes[owner].source,"Entity declaration has no matching owned asset binding");
    if(!binding->article)return error(diagnostic,tree->nodes[owner].source,"Entity binding has no imported Article descriptor");
    if(!binding->article->x0_common_attr)return error(diagnostic,tree->nodes[owner].source,"Entity Article has no imported common attributes");
    if(!binding->article->x10_modelDesc)return error(diagnostic,tree->nodes[owner].source,"Entity Article has no imported model descriptor");
    // A descriptor with no mesh joint is intentional (for example, a particle
    // article). Item_802680CC creates its runtime root anchor. The descriptor
    // itself remains required by the common constructor and dynamics setup.
    if(!staticModelRoots(tree,owner,binding,diagnostic)||!staticModelAnimations(tree,owner,binding,diagnostic))return false;
    NativeScriptResourceRecipe resources;
    if(!nativeScriptResourceMetadata_Prepare(tree,owner,context?context->source_facts:NULL,&resources,diagnostic))return false;
    if(!header(tree,owner,&count,&recipe))return error(diagnostic,tree->nodes[owner].source,"Entity declaration header exceeds the supported numeric schema");
    bool workshop=tree->nodes[owner].kind==NativeScriptSource_Item;
    if((workshop && binding->kind!=It_Kind_NativeHoldable) || (!workshop && binding->kind==It_Kind_NativeHoldable))
        return error(diagnostic,node->source,"Item/Article declaration differs from its physical asset schema");
    if(at==owner)return node->parent==0;
    if(node->kind==NativeScriptSource_Metadata&&node->parent==owner&&same(tree,node->name,"resources"))return true;
    if(node->kind==NativeScriptSource_Metadata&&node->parent==owner&&same(tree,node->name,"model_root"))return true;
    if(node->kind==NativeScriptSource_Metadata&&node->parent==owner&&same(tree,node->name,"model_animation"))return true;
    if(node->kind==NativeScriptSource_State || node->kind==NativeScriptSource_Parameter || node->kind==NativeScriptSource_Attribute || node->kind==NativeScriptSource_Table || node->kind==NativeScriptSource_Function)
        return node->parent==owner;
    if(node->kind==NativeScriptSource_ArticleState) {
        uint32_t state,alias;int32_t animation;
        if(workshop || node->parent!=owner || !articleState(tree,node,count,&state,&animation,&alias))
            return error(diagnostic,node->source,"Invalid authored article state/alias");
        if(animation>=0 && (!binding->article->xC_itemStates || (uint32_t)animation>=binding->article->xC_itemStates->count))
            return error(diagnostic,node->source,"Article animation index exceeds the actual imported asset array");
        return true;
    }
    if(node->kind==NativeScriptSource_Callback) {
        NativeScriptEventResolution resolved;
        return nativeScriptEvents_Resolve(tree,node,&resolved,diagnostic) && resolved.owner_node==owner;
    }
    if(!workshop && node->kind==NativeScriptSource_Metadata && node->parent==owner && !node->opens){
        NativeScriptItemProjectile projectile;NativeScriptItemAttachment attachment;
        if(same(tree,node->name,"render")) {
            if(!same(tree,node->value,"standard"))return error(diagnostic,node->source,"Article rendering requires the owned standard model policy");
            for(uint32_t other=tree->nodes[owner].first_child;other!=UINT32_MAX;other=tree->nodes[other].next)
                if(other!=at && tree->nodes[other].kind==NativeScriptSource_Metadata && same(tree,tree->nodes[other].name,"render"))
                    return error(diagnostic,node->source,"Duplicate Article rendering policy");
            return true;
        }
        if(same(tree,node->name,"projectile") && projectileField(tree,node,binding,count,NULL,owner,&projectile))return true;
        if(same(tree,node->name,"attachment") && attachmentField(tree,node,&attachment))return true;
    }
    if(workshop && node->kind==NativeScriptSource_Metadata && node->parent==owner && !node->opens && recipeField(tree,node,binding,&recipe))return true;
    return error(diagnostic,node->source,"Unsupported entity metadata: prepare its data/lifetime schema before activation");
}
static int hook(const NativeScriptEventDescriptor* descriptor)
{
    static const char* const names[NativeScriptItemHook_Count]={"spawned","destroyed","picked_up","dropped","thrown",
        "damage_dealt","damage_received","entered_air","reflected","clanked","absorbed","shield_bounced","hit_shield","interaction",
        "created","out_of_bounds","accessory","entered_hitlag","exited_hitlag","touched","jumped_on","grab_dealt","grabbed_for_victim",
        "launched","stage_contact","stage_landed","stage_left","held_status","held_request","holder_neutral","thrown","tick","hit","ground_collision"};
    unsigned first=descriptor->context==NativeScriptEvent_WorkshopItem?NativeScriptItemHook_WorkshopThrown:0;
    unsigned last=descriptor->context==NativeScriptEvent_WorkshopItem?NativeScriptItemHook_Count:NativeScriptItemHook_WorkshopThrown;
    for(unsigned i=first;i<last;++i)if(!strcmp(descriptor->name,names[i]))return (int)i;
    return -1;
}
static uint32_t nameHash(const NativeScriptSourceTree* tree,NativeScriptSourceSpan name)
{
    uint32_t hash=UINT32_C(2166136261);for(uint32_t i=0;i<name.length;++i){hash^=(uint8_t)tree->source[name.offset+i];hash*=UINT32_C(16777619);}return hash;
}
bool nativeScriptItemPrograms_Prepare(const NativeScriptCompileBank* bank,FighterKind author,
    uint64_t identity,const NativeScriptItemMetadataContext* context,NativeScriptItemPrograms* output,NativeScriptDiagnostic* diagnostic)
{
    if(!bank || !bank->valid || !output || !identity)return error(diagnostic,(NativeScriptSourceSpan){0},"Missing immutable entity program inputs");
    NativeScriptItemMetadataContext checked=context?*context:(NativeScriptItemMetadataContext){0};
    checked.motions=bank->has_motion_layout?&bank->motion_layout:NULL;
    for(uint32_t node=0;node<bank->tree.count;++node){
        bool effect=false;
        for(uint32_t at=node,bound=0;at<bank->tree.count&&bound<bank->tree.count;++bound) {
            if(bank->tree.nodes[at].kind==NativeScriptSource_Effect){effect=true;break;}
            if(!at)break;at=bank->tree.nodes[at].parent;
        }
        if(effect)continue; // The direct Effect owner validates its own subtree.
        if(nativeScriptPoseMetadata_Owns(&bank->tree,node)) {
            if(!nativeScriptPoseMetadata_Validate(bank,&bank->tree,node,diagnostic))return false;
            continue;
        }
        if(nativeScriptHitboxMetadata_Owns(&bank->tree,node)) {
            if(!nativeScriptHitboxMetadata_Validate(bank,&bank->tree,node,diagnostic))return false;
            continue;
        }
        unsigned kind=bank->tree.nodes[node].kind;
        if(kind>=NativeScriptSource_If && kind<=NativeScriptSource_Pass)continue;
        if(!nativeScriptItemMetadata_Validate(&checked,&bank->tree,node,diagnostic))return false;
    }
    context=&checked;
    NativeScriptItemPrograms* candidate=calloc(1,sizeof(*candidate));
    if(!candidate)return error(diagnostic,(NativeScriptSourceSpan){0},"Entity program transition scratch allocation failed");
    const NativeScriptSourceTree* tree=&bank->tree;uint32_t recipe_index=0;
    for(uint32_t node=1;node<tree->count;++node) {
        bool workshop=tree->nodes[node].kind==NativeScriptSource_Item;
        if(!workshop && tree->nodes[node].kind!=NativeScriptSource_Article)continue;
        if(candidate->count==NATIVE_SCRIPT_ENTITY_LIMIT){error(diagnostic,tree->nodes[node].source,"Too many source entities");goto failure;}
        const NativeScriptItemAsset* binding=asset(context,tree,node);uint32_t states;
        NativeItemDefinition recipe;const NativeScriptStorageOwner* storage=nativeScriptStorage_Owner(&bank->storage_layout,node);
        if(!binding || !header(tree,node,&states,&recipe) || !storage || storage->state_bytes>NATIVE_SCRIPT_ITEM_STATE_BYTES ||
            states>NATIVE_SCRIPT_ENTITY_STATE_LIMIT-candidate->state_count)
            {error(diagnostic,tree->nodes[node].source,"Entity asset/state storage preparation failed");goto failure;}
        NativeScriptResourceRecipe resources;
        if(!nativeScriptResourceMetadata_Prepare(tree,node,context->source_facts,&resources,diagnostic))goto failure;
        unsigned entry=candidate->count++;NativeScriptItemProgram* program=&candidate->programs[entry];
        *program=(NativeScriptItemProgram){.bank=bank,.identity=identity^(UINT64_C(0x9e3779b97f4a7c15)*node),.author_kind=author,
            .kind=binding->kind,.recipe_index=workshop?recipe_index++:UINT32_MAX,.owner=node,.article=binding->article,
            .resource_capacity=resources.capacity,.resource_reserved_part=resources.reserved_part,
            .model_roots=binding->model_roots,.model_root_count=binding->model_root_count,
            .model_animations=binding->model_animations,.model_animation_count=binding->model_animation_count,
            .drawing=workshop?NativeScriptItemDrawing_Billboard:NativeScriptItemDrawing_Standard,
            .states=&candidate->states[candidate->state_count],.state_count=states};
        if(!program->identity)program->identity=1;
        for(unsigned i=0;i<NativeScriptItemHook_Count;++i)program->hooks[i]=UINT32_MAX;
        uint32_t aliases[NATIVE_SCRIPT_ITEM_STATES],seen=0;
        for(unsigned i=0;i<states;++i){aliases[i]=UINT32_MAX;NativeScriptItemState* state=&candidate->states[candidate->state_count+i];
            state->animation_id=-1;for(unsigned phase=0;phase<NativeScriptItemPhase_Count;++phase)state->functions[phase]=UINT32_MAX;}
        if(!workshop)for(uint32_t child=tree->nodes[node].first_child;child!=UINT32_MAX;child=tree->nodes[child].next){
            const NativeScriptSourceNode* declaration=&tree->nodes[child];if(declaration->kind!=NativeScriptSource_Metadata)continue;
            if(same(tree,declaration->name,"projectile")){
                if(program->projectile || !projectileField(tree,declaration,binding,states,bank,node,&candidate->projectiles[entry]))
                    {error(diagnostic,declaration->source,"Invalid/duplicate data-only projectile recipe or owned Float parameter");goto failure;}
                program->projectile=&candidate->projectiles[entry];
            }else if(same(tree,declaration->name,"attachment")){
                if(program->attachment || !attachmentField(tree,declaration,&candidate->attachments[entry]))
                    {error(diagnostic,declaration->source,"Invalid/duplicate data-only attachment recipe");goto failure;}
                program->attachment=&candidate->attachments[entry];
            }
        }
        if(workshop) {
            NativeScriptSourceSpan name=tree->nodes[node].name;
            if(name.length>=sizeof(candidate->recipe_names[program->recipe_index])){error(diagnostic,name,"Item name is too long");goto failure;}
            memcpy(candidate->recipe_names[program->recipe_index],tree->source+name.offset,name.length);
            unsigned recipe_seen=0;
            for(uint32_t child=tree->nodes[node].first_child;child!=UINT32_MAX;child=tree->nodes[child].next) {
                const NativeScriptSourceNode* field=&tree->nodes[child];
                if(field->kind!=NativeScriptSource_Metadata)continue;
                if(same(tree,field->name,"resources"))continue; // Prepared owner recipe, not visual metadata.
                unsigned bit=same(tree,field->name,"icon")?1:same(tree,field->name,"billboard_size")?2:
                    same(tree,field->name,"hitbox_size")?4:same(tree,field->name,"knockback_growth")?8:
                    same(tree,field->name,"base_knockback")?16:same(tree,field->name,"blueprint")?32:0;
                if(!bit || (recipe_seen&bit) || !recipeField(tree,field,binding,&recipe))
                    {error(diagnostic,field->source,"Invalid/duplicate item recipe metadata");goto failure;}
                recipe_seen|=bit;
            }
            candidate->recipes[program->recipe_index]=recipe;candidate->recipes[program->recipe_index].name=candidate->recipe_names[program->recipe_index];program->recipe=&candidate->recipes[program->recipe_index];
        } else for(uint32_t child=tree->nodes[node].first_child;child!=UINT32_MAX;child=tree->nodes[child].next) {
            if(tree->nodes[child].kind!=NativeScriptSource_ArticleState)continue;uint32_t state,alias;int32_t animation;
            if(!articleState(tree,&tree->nodes[child],states,&state,&animation,&alias) || (seen&(UINT32_C(1)<<state)))
                {error(diagnostic,tree->nodes[child].source,"Duplicate/invalid article state");goto failure;}
            seen|=UINT32_C(1)<<state;candidate->states[candidate->state_count+state].animation_id=animation;aliases[state]=alias;
        }
        if(!workshop && seen!=(states==32?UINT32_MAX:(UINT32_C(1)<<states)-1))
            {error(diagnostic,tree->nodes[node].source,"Every article state needs explicit animation metadata");goto failure;}
        for(uint32_t f=0;f<bank->declarations.function_count;++f) {
            const NativeScriptFunctionDeclaration* function=&bank->declarations.functions[f];if(function->owner!=node || !function->callback)continue;
            NativeScriptEventResolution event;if(!nativeScriptEvents_Resolve(tree,&tree->nodes[function->source_node],&event,diagnostic))goto failure;
            if(event.descriptor->kind==NativeScriptEvent_ArticleState) {
                unsigned phase=!strcmp(event.descriptor->name,"animation")?NativeScriptItemPhase_Animation:
                    !strcmp(event.descriptor->name,"physics")?NativeScriptItemPhase_Physics:NativeScriptItemPhase_Collision;
                if(event.motion>=states || aliases[event.motion]!=UINT32_MAX){error(diagnostic,function->name,"Invalid source article phase target");goto failure;}
                uint32_t* slot=&candidate->states[candidate->state_count+event.motion].functions[phase];
                if(*slot!=UINT32_MAX){error(diagnostic,function->name,"Duplicate article state callback");goto failure;}*slot=f;
            } else if(event.descriptor->kind==NativeScriptEvent_ArticleSignal) {
                if(candidate->signal_count==NATIVE_SCRIPT_ENTITY_SIGNAL_LIMIT){error(diagnostic,function->name,"Too many authored signals");goto failure;}
                if(!program->signals)program->signals=&candidate->signals[candidate->signal_count];
                uint32_t id=nameHash(tree,event.signal_name);unsigned begin=candidate->signal_count-program->signal_count,at=begin;
                while(at<candidate->signal_count && candidate->signals[at].signal_id<id)++at;
                if(at<candidate->signal_count && candidate->signals[at].signal_id==id){error(diagnostic,function->name,"Duplicate signal identity");goto failure;}
                memmove(candidate->signals+at+1,candidate->signals+at,(candidate->signal_count-at)*sizeof(*candidate->signals));
                candidate->signals[at]=(NativeScriptItemSignal){id,f};++program->signal_count;++candidate->signal_count;
            } else {
                int slot=hook(event.descriptor);if(slot<0 || program->hooks[slot]!=UINT32_MAX){error(diagnostic,function->name,"Unsupported/duplicate source item hook");goto failure;}
                program->hooks[slot]=f;
            }
        }
        for(unsigned state=0;state<states;++state)if(aliases[state]!=UINT32_MAX) {
            unsigned target=state,depth=0;while(aliases[target]!=UINT32_MAX){if(++depth>states){error(diagnostic,tree->nodes[node].source,"Article alias cycle");goto failure;}target=aliases[target];}
            memcpy(candidate->states[candidate->state_count+state].functions,candidate->states[candidate->state_count+target].functions,sizeof(candidate->states[0].functions));
        }
        candidate->state_count+=states;candidate->recipe_count=recipe_index;
        if(!nativeScriptBindingItemModel_PrepareSelectors(bank,program,
            candidate->model_selectors+candidate->model_selector_count,
            NATIVE_SCRIPT_ENTITY_MODEL_SELECTORS-candidate->model_selector_count,diagnostic))goto failure;
        candidate->model_selector_count+=program->model_selector_count;
        if(!nativeScriptBindingItemResources_PrepareSelectors(bank,program,
            candidate->resource_selectors+candidate->resource_selector_count,
            NATIVE_SCRIPT_ENTITY_RESOURCE_SELECTORS-candidate->resource_selector_count,diagnostic))goto failure;
        candidate->resource_selector_count+=program->resource_selector_count;
        if(!nativeScriptBindingItemAnimation_PrepareDescriptors(bank,program,
            candidate->animation_descriptors+candidate->animation_descriptor_count,
            NATIVE_SCRIPT_ENTITY_ANIMATION_DESCRIPTORS-candidate->animation_descriptor_count,diagnostic))goto failure;
        candidate->animation_descriptor_count+=program->animation_descriptor_count;
        if(!animationOrigins(candidate,program,diagnostic))goto failure;
    }
    *output=*candidate;nativeScriptItemPrograms_Rebind(output,candidate);free(candidate);return true;
failure:
    free(candidate);return false;
}
void nativeScriptItemPrograms_Rebind(NativeScriptItemPrograms* destination,const NativeScriptItemPrograms* source)
{
    for(unsigned i=0;i<destination->count;++i) {
        NativeScriptItemProgram* program=&destination->programs[i];const NativeScriptItemProgram* old=&source->programs[i];
        program->states=destination->states+(old->states-source->states);
        if(old->signals)program->signals=destination->signals+(old->signals-source->signals);
        if(old->model_selectors)program->model_selectors=destination->model_selectors+(old->model_selectors-source->model_selectors);
        if(old->resource_selectors)program->resource_selectors=destination->resource_selectors+(old->resource_selectors-source->resource_selectors);
        if(old->animation_origins)program->animation_origins=destination->animation_origins+(old->animation_origins-source->animation_origins);
        if(old->animation_descriptors)program->animation_descriptors=destination->animation_descriptors+(old->animation_descriptors-source->animation_descriptors);
        if(old->recipe){program->recipe=&destination->recipes[program->recipe_index];destination->recipes[program->recipe_index].name=destination->recipe_names[program->recipe_index];}
        if(old->projectile)program->projectile=&destination->projectiles[i];
        if(old->attachment)program->attachment=&destination->attachments[i];
    }
}
bool nativeScriptItemMetadata_ImportParameters(const NativeScriptProgramIndex* index,const NativeScriptItemMetadataContext* context,
    NativeScriptParameterSupply* supplies,uint32_t capacity,uint32_t* count,NativeScriptDiagnostic* diagnostic)
{
    if(!index || !index->tree || !count)return false;uint32_t required=0;
    for(uint32_t i=0;i<index->value_count;++i)if(index->values[i].owner && index->values[i].imported)++required;
    if(required>capacity || (required && !supplies))return error(diagnostic,(NativeScriptSourceSpan){0},"Entity parameter supply capacity exhausted");
    for(unsigned pass=0;pass<2;++pass) {
        uint32_t written=0;
        for(uint32_t i=0;i<index->value_count;++i) {
            const NativeScriptValueDeclaration* field=&index->values[i];if(!field->owner || !field->imported)continue;
            const NativeScriptItemAsset* binding=asset(context,index->tree,field->owner);HSD_Dat dat;NativeScriptValue value;
            if(!binding || !binding->article || !binding->dat_bytes || !binding->dat_size ||
                field->array || field->count!=1 || !field->parameter ||
                HSD_DatOpen(&dat,binding->dat_bytes,binding->dat_size)!=HSD_DAT_OK)
                return error(diagnostic,field->name,"Entity parameter lacks its owned original DAT source");
            uint32_t begin=binding->article->native_special_attribute_source,end=HSD_DatNextTarget(&dat,begin);
            if(!begin || end<begin || !HSD_DatBytes(&dat,begin,end-begin) ||
                !nativeScriptMetadata_ReadAttribute(&dat,begin,end-begin,field,&value,diagnostic))return false;
            if(pass)supplies[written]=(NativeScriptParameterSupply){NATIVE_SCRIPT_PARAMETER_SEMANTIC|i,field->owner,value};++written;
        }
        if(pass)*count=written;
    }
    return true;
}
