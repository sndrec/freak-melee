#include "native_source_entity_models.h"
#include "native_script_model_metadata.h"
#include <stdio.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d, NativeScriptSourceSpan at, const char* text)
{ if (d) { d->source = at; snprintf(d->message, sizeof(d->message), "%s", text); } return false; }
bool nativeSourceEntityModels_Prepare(NativeSourceEntityAssets* output,
    NativeSourceImportedAssets* imported, const NativeScriptSourceTree* tree, NativeScriptDiagnostic* d)
{
    for (unsigned node = 1; node < tree->count; ++node) {
        const NativeScriptSourceNode* owner = &tree->nodes[node];
        if (owner->kind != NativeScriptSource_Article && owner->kind != NativeScriptSource_Item) continue;
        unsigned row = 0;
        for (; row < output->count; ++row)
            if (nativeScriptSource_SpanEquals(tree, owner->name, output->rows[row].declaration)) break;
        bool has_roots = false;
        for (unsigned child = owner->first_child; child != UINT32_MAX; child = tree->nodes[child].next) {
            const NativeScriptSourceNode* field = &tree->nodes[child];
            if (field->kind != NativeScriptSource_Metadata) continue;
            if (nativeScriptSource_SpanEquals(tree, field->name, "model_root")) has_roots = true;
        }
        if (row == output->count) {
            if (has_roots) return error(d, owner->source, "Secondary model owner has no exact owned Article asset");
            continue;
        }
        NativeScriptItemAsset* asset = &output->rows[row];
        if (asset->owner_node != UINT32_MAX)
            return error(d, owner->source, "Duplicate source entity asset declaration");
        asset->owner_node = node;
        HSD_DatImport* import = &imported->character.import;
        if (owner->kind == NativeScriptSource_Item) {
            unsigned index = 0;
            for (; index < output->blueprint_count; ++index)
                if (output->blueprints[index].article == asset->article) break;
            if (index == output->blueprint_count) return error(d, owner->source, "Item model roots have no owning blueprint import");
            import = &output->blueprints[index].import;
        }
        bool state_seen = false;
        for (unsigned child = owner->first_child; child != UINT32_MAX; child = tree->nodes[child].next) {
            const NativeScriptSourceNode* field = &tree->nodes[child];
            if (field->kind == NativeScriptSource_ArticleState) state_seen = true;
            if (field->kind != NativeScriptSource_Metadata ||
                !nativeScriptSource_SpanEquals(tree, field->name, "model_root")) continue;
            unsigned at = asset->model_root_count;
            if (field->opens || state_seen || at == NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT)
                return error(d, field->source, "Static model root must precede states and fit the owned root table");
            NativeScriptItemModelRootAsset* record = &output->model_roots[row][at];
            char* name = output->model_root_names[row][at];
            NativeScriptModelMetadata parsed;
            if (!nativeScriptModelMetadata_Parse(tree, field->value, false, &parsed))
                return error(d, field->source, "Model root needs an identifier and aligned native32 attribute offset");
            record->attribute_offset = parsed.root_offset;
            memcpy(name, tree->source + parsed.name.offset, parsed.name.length);
            name[parsed.name.length] = 0;
            for (unsigned previous = 0; previous < at; ++previous)
                if (!strcmp(name, output->model_root_names[row][previous]))
                    return error(d, field->source, "Duplicate model root name in this source owner");
            HSD_Joint* root = NULL;
            if (!itArticle_ImportModelRoot(import, asset->article, record->attribute_offset, &root))
                return error(d, field->source, "Model root offset does not resolve an owned DAT joint graph");
            if (!root) return error(d, field->source, "Declared secondary model root is NULL");
            record->name = name; record->root = root;
            asset->model_roots = output->model_roots[row]; ++asset->model_root_count;
        }
    }
    return true;
}
