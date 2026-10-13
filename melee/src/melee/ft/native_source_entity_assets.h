#ifndef MELEE_FT_NATIVE_SOURCE_ENTITY_ASSETS_H
#define MELEE_FT_NATIVE_SOURCE_ENTITY_ASSETS_H
#include "native_source_item_blueprint.h"
#include "native_source_resources.h"
#include "native_source_assets.h"
#include "native_script_item_program.h"

typedef struct NativeSourceEntityAssets {
    NativeScriptItemAsset rows[NATIVE_SCRIPT_ENTITY_LIMIT];
    char names[NATIVE_SCRIPT_ENTITY_LIMIT][NATIVE_SOURCE_RESOURCE_NAME_BYTES];
    NativeSourceItemBlueprint blueprints[NATIVE_SCRIPT_ENTITY_LIMIT];
    const NativeSourceResourceEntry* blueprint_entries[NATIVE_SCRIPT_ENTITY_LIMIT];
    NativeScriptItemModelRootAsset model_roots[NATIVE_SCRIPT_ENTITY_LIMIT][NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT];
    char model_root_names[NATIVE_SCRIPT_ENTITY_LIMIT][NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT][NATIVE_SOURCE_NAME_BYTES];
    NativeScriptItemModelAnimationAsset model_animations[NATIVE_SCRIPT_ENTITY_LIMIT][NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT];
    char model_animation_names[NATIVE_SCRIPT_ENTITY_LIMIT][NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT][NATIVE_SOURCE_NAME_BYTES];
    uint32_t count,blueprint_count;
} NativeSourceEntityAssets;
// The syntax tree is inspected only at preparation; runtime sees indexed owned
// assets. All DAT descriptors and strings retain their final owner addresses.
// imported is a mutable, unpublished candidate: exact owning import cursors
// advance when adding secondary descriptors. Failure discards that candidate;
// it does not rewind import arenas or mutate a separately accepted package.
bool nativeSourceEntityAssets_Prepare(NativeSourceEntityAssets*,NativeSourceImportedAssets*,
    const NativeSourceAssetInput*,const NativeSourceResources*,const NativeScriptSourceTree*,NativeScriptDiagnostic*);
void nativeSourceEntityAssets_Release(NativeSourceEntityAssets*);
#endif
