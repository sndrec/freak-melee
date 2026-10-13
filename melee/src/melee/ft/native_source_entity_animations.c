#include "native_source_entity_animations.h"
#include "native_script_model_metadata.h"
#include <melee/it/itarticle_data.h>
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d, NativeScriptSourceSpan at, const char* text)
{ if (d) { d->source = at; snprintf(d->message, sizeof(d->message), "%s", text); } return false; }
static bool equal(const NativeScriptSourceTree* tree, NativeScriptSourceSpan name, const char* text)
{ return text && nativeScriptSource_SpanEquals(tree, name, text); }
bool nativeSourceEntityAnimations_Prepare(NativeSourceEntityAssets* output,
    NativeSourceImportedAssets* imported, const NativeScriptSourceTree* tree, NativeScriptDiagnostic* d)
{
    if (!output || !imported || !imported->ready || !tree || !tree->nodes ||
        !tree->source || output->count > NATIVE_SCRIPT_ENTITY_LIMIT ||
        output->blueprint_count > NATIVE_SCRIPT_ENTITY_LIMIT)
        return error(d, (NativeScriptSourceSpan){0}, "Animation bundles require exact unpublished entity/arena owners");
    for (uint32_t owner = 1; owner < tree->count; ++owner) {
        const NativeScriptSourceNode* node = &tree->nodes[owner];
        if (node->kind != NativeScriptSource_Article && node->kind != NativeScriptSource_Item) continue;
        unsigned row = 0;
        for (; row < output->count; ++row)
            if (equal(tree, node->name, output->rows[row].declaration)) break;
        bool state_seen = false;
        for (uint32_t child = node->first_child; child != UINT32_MAX; child = tree->nodes[child].next) {
            if (child >= tree->count)
                return error(d, node->source, "Animation bundle child index is invalid");
            const NativeScriptSourceNode* field = &tree->nodes[child];
            if (field->kind == NativeScriptSource_ArticleState) state_seen = true;
            if (field->kind != NativeScriptSource_Metadata ||
                !nativeScriptSource_SpanEquals(tree, field->name, "model_animation")) continue;
            if (row == output->count)
                return error(d, field->source, "Animation bundle has no exact owned Article asset");
            NativeScriptItemAsset* asset = &output->rows[row];
            if (asset->owner_node != owner || !asset->article ||
                asset->model_animation_count >= NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT ||
                field->parent != owner || field->opens || state_seen)
                return error(d, field->source, "Animation bundle must precede states and fit its exact owned table");
            NativeScriptModelMetadata metadata;
            if (!nativeScriptModelMetadata_Parse(tree, field->value, true, &metadata))
                return error(d, field->source, "Animation bundle needs an identifier and aligned pointer components");
            NativeScriptSourceSpan name = metadata.name;
            unsigned at = asset->model_animation_count;
            if ((at && asset->model_animations != output->model_animations[row]) ||
                (!at && asset->model_animations))
                return error(d, field->source, "Animation bundle table differs from its unpublished owner");
            for (unsigned previous = 0; previous < at; ++previous)
                if (equal(tree, name, output->model_animation_names[row][previous]))
                    return error(d, field->source, "Duplicate animation bundle name in this owner");
            if (asset->model_root_count > NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT ||
                (asset->model_root_count && !asset->model_roots))
                return error(d, field->source, "Animation owner has an invalid root table");
            for (unsigned root = 0; root < asset->model_root_count; ++root)
                if (equal(tree, name, asset->model_roots[root].name))
                    return error(d, field->source, "Animation bundle name conflicts with an owned model root");
            HSD_DatImport* arena = &imported->character.import;
            if (node->kind == NativeScriptSource_Item) {
                unsigned blueprint = 0;
                for (; blueprint < output->blueprint_count; ++blueprint)
                    if (output->blueprints[blueprint].article == asset->article) break;
                if (blueprint == output->blueprint_count)
                    return error(d, field->source, "Animation bundle has no owning blueprint arena");
                arena = &output->blueprints[blueprint].import;
            }
            ItArticleModelAnimationBundle bundle;
            if (!itArticle_ImportModelAnimation(arena, asset->article, metadata.components, &bundle))
                return error(d, field->source, "Animation components do not resolve owned typed DAT graphs");
            char* copied = output->model_animation_names[row][at];
            memcpy(copied, tree->source + name.offset, name.length); copied[name.length] = 0;
            output->model_animations[row][at] = (NativeScriptItemModelAnimationAsset){
                .name = copied,
                .anim = bundle.anim, .matanim = bundle.matanim, .shapeanim = bundle.shapeanim
            };
            memcpy(output->model_animations[row][at].components, metadata.components, sizeof(metadata.components));
            asset->model_animations = output->model_animations[row];
            ++asset->model_animation_count;
        }
    }
    return true;
}
