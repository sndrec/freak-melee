#include "native_source_entity_assets.h"
#include "native_source_entity_models.h"
#include "native_source_entity_animations.h"
#include <melee/it/native_item_model_admission.h>
#include <melee/it/types.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,NativeScriptSourceSpan at,const char* text)
{if(d){d->source=at;snprintf(d->message,sizeof(d->message),"%s",text);}return false;}
static bool name(const NativeScriptSourceTree* tree,NativeScriptSourceSpan span,char output[128])
{
    if(!span.length||span.length>=128||span.offset>tree->source_bytes||span.length>tree->source_bytes-span.offset)return false;
    memcpy(output,tree->source+span.offset,span.length);output[span.length]=0;return true;
}
void nativeSourceEntityAssets_Release(NativeSourceEntityAssets* assets)
{
    if(!assets)return;for(unsigned i=0;i<assets->blueprint_count;++i)nativeSourceItemBlueprint_Release(&assets->blueprints[i]);
    memset(assets,0,sizeof(*assets));
}
bool nativeSourceEntityAssets_Prepare(NativeSourceEntityAssets* output,NativeSourceImportedAssets* imported,
    const NativeSourceAssetInput* dats,const NativeSourceResources* resources,const NativeScriptSourceTree* tree,NativeScriptDiagnostic* d)
{
    if(!output||!imported||!imported->ready||!dats||!resources||!tree||output->count||output->blueprint_count)return false;
    const uint8_t* vacant=(const uint8_t*)output;
    for(size_t i=0;i<sizeof(*output);++i)if(vacant[i])return error(d,(NativeScriptSourceSpan){0},"Source entity assets need a vacant stable owner");
    for(unsigned i=0;i<imported->facts.article_count;++i){
        const NativeSourceArticleAsset* row=&imported->facts.articles[i];ItemKind kind=imported->article_kinds[i];
        if(output->count==NATIVE_SCRIPT_ENTITY_LIMIT||kind<0||kind>=It_Kind_NativeHoldable)goto failure;
        unsigned at=output->count++;strcpy(output->names[at],row->declaration);
        output->rows[at]=(NativeScriptItemAsset){.declaration=output->names[at],.kind=kind,
            .article=imported->character.fighter_items[row->slot],.dat_bytes=dats->fighter,.dat_size=dats->fighter_bytes,.owner_node=UINT32_MAX};
        const Article* article=output->rows[at].article;
        if(!article || !article->x10_modelDesc)goto failure;
        if(article->x10_modelDesc->x4_bone_count) {
            HSD_DatImport admission=imported->character.import;u32 populated;
            if(!nativeItemModel_Admit(&admission,article->x10_modelDesc,0,&populated)) {
                error(d,(NativeScriptSourceSpan){0},"Owned Article model exceeds native indexed graph topology/capacity");goto failure;
            }
        }
    }
    for(unsigned node=1;node<tree->count;++node){
        const NativeScriptSourceNode* item=&tree->nodes[node];if(item->kind!=NativeScriptSource_Item)continue;
        if(output->count==NATIVE_SCRIPT_ENTITY_LIMIT)goto failure;
        unsigned at=output->count;
        if(!name(tree,item->name,output->names[at]))goto failure;
        const NativeSourceResourceEntry *blueprint=NULL,*icon=NULL;
        for(unsigned child=item->first_child;child!=UINT32_MAX;child=tree->nodes[child].next){
            const NativeScriptSourceNode* field=&tree->nodes[child];if(field->kind!=NativeScriptSource_Metadata)continue;
            bool is_blueprint=nativeScriptSource_SpanEquals(tree,field->name,"blueprint");
            bool is_icon=nativeScriptSource_SpanEquals(tree,field->name,"icon");
            if(!is_blueprint&&!is_icon)continue;
            char path[128];if(!name(tree,field->value,path))goto failure;
            const NativeSourceResourceEntry* resource=nativeSourceResources_Find(resources,path,is_blueprint?NativeSourceResource_Blob:NativeSourceResource_PNG);
            if(!resource||(is_blueprint?blueprint!=NULL:icon!=NULL)) {
                error(d,field->source,"Item resource is absent, wrong type or declared twice");goto failure;
            }
            if(is_blueprint)blueprint=resource;else icon=resource;
        }
        if(!blueprint){error(d,item->source,"Workshop Item requires an owned blueprint resource");goto failure;}
        unsigned index=0;
        for(;index<output->blueprint_count;++index)if(output->blueprint_entries[index]==blueprint)break;
        if(index==output->blueprint_count){
            if(!nativeSourceItemBlueprint_Import(blueprint->bytes,blueprint->size,&output->blueprints[index],d))goto failure;
            output->blueprint_entries[index]=blueprint;++output->blueprint_count;
        }
        const NativeSourceItemBlueprint* model=&output->blueprints[index];
        if(!model->article || !model->article->x10_modelDesc)goto failure;
        if(model->article->x10_modelDesc->x4_bone_count) {
            HSD_DatImport admission=model->import;u32 populated;
            if(!nativeItemModel_Admit(&admission,model->article->x10_modelDesc,0,&populated)) {
                error(d,item->source,"Owned Item blueprint exceeds native indexed graph topology/capacity");goto failure;
            }
        }
        output->rows[at]=(NativeScriptItemAsset){.declaration=output->names[at],.kind=It_Kind_NativeHoldable,
            .article=model->article,.dat_bytes=model->bytes,.dat_size=model->size,.blueprint_path=blueprint->name,
            .icon_path=icon?icon->name:NULL,.icon_png=icon?icon->bytes:NULL,.icon_bytes=icon?icon->size:0,.owner_node=UINT32_MAX};
        ++output->count;
    }
    if(!nativeSourceEntityModels_Prepare(output,imported,tree,d))goto failure;
    if(!nativeSourceEntityAnimations_Prepare(output,imported,tree,d))goto failure;
    return true;
failure:
    if(d&&!d->message[0])error(d,(NativeScriptSourceSpan){0},"Source entity asset schema or capacity failed");
    nativeSourceEntityAssets_Release(output);return false;
}
