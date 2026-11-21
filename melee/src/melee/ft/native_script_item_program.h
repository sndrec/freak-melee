#ifndef MELEE_FT_NATIVE_SCRIPT_ITEM_PROGRAM_H
#define MELEE_FT_NATIVE_SCRIPT_ITEM_PROGRAM_H
#include <melee/it/native_script_item_actor.h>
#include "native_fighter_runtime.h"
#include "native_script_metadata.h"
#include "native_script_binding_item_resources.h"
#include "native_script_binding_item_animation.h"
#include <melee/it/native_article_animation_live.h>
enum { NATIVE_SCRIPT_ENTITY_LIMIT=64, NATIVE_SCRIPT_ENTITY_STATE_LIMIT=2048,
       NATIVE_SCRIPT_ENTITY_SIGNAL_LIMIT=256, NATIVE_SCRIPT_ENTITY_MODEL_SELECTORS=2048,
       NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS=4096,
       NATIVE_SCRIPT_ENTITY_ANIMATION_DESCRIPTORS=2048,
       NATIVE_SCRIPT_ENTITY_RESOURCE_SELECTORS=2048 };
typedef struct NativeScriptItemScalar {
    float value;
    uint32_t parameter_semantic; // UINT32_MAX denotes a finite literal.
} NativeScriptItemScalar;
typedef struct NativeScriptItemProjectile {
    int32_t appearance_state;
    uint32_t item_state;
    NativeScriptItemScalar speed, angle, lifetime;
    bool auto_lifetime, initial_velocity, initial_update;
} NativeScriptItemProjectile;
typedef struct NativeScriptItemAttachment {
    bool origin_at_owner, origin_at_center, previous_at_bone;
    bool flatten_z, initial_collision, register_as_held;
} NativeScriptItemAttachment;
// Prepared immutable assets, not native behavior definitions. The owning
// package pins every pointer and original DAT until its programs/actors drain.
typedef struct NativeScriptItemModelRootAsset {
    const char* name; // Exact immutable bytes, scoped to owner_node below.
    uint32_t attribute_offset;
    const HSD_Joint* root;
} NativeScriptItemModelRootAsset;
typedef struct NativeScriptItemModelAnimationAsset {
    const char* name; // Exact immutable bytes, scoped to the asset owner node.
    NativeFighterArticleAnimationComponent components[3]; // Exact source pointer/index declarations.
    const HSD_AnimJoint* anim;
    const HSD_MatAnimJoint* matanim;
    const HSD_ShapeAnimJoint* shapeanim;
} NativeScriptItemModelAnimationAsset;
typedef struct NativeScriptItemAsset {
    const char* declaration;
    ItemKind kind;
    const Article* article;
    const char* blueprint_path;
    const char* icon_path;
    const uint8_t* icon_png;
    uint32_t icon_bytes;
    const struct HSD_Dat* dat;
    uint32_t attribute_bytes;
    uint32_t owner_node; // UINT32_MAX if this asset has no source declaration.
    const NativeScriptItemModelRootAsset* model_roots;
    uint32_t model_root_count;
    const NativeScriptItemModelAnimationAsset* model_animations;
    uint32_t model_animation_count;
} NativeScriptItemAsset;
typedef struct NativeScriptItemMetadataContext {
    const NativeScriptMoveLayout* motions;
    const NativeScriptItemAsset* assets;
    uint32_t asset_count;
    const struct NativeSourceAssetFacts* source_facts;
    const struct NativeSourceImportedAssets* source_assets;
} NativeScriptItemMetadataContext;
typedef struct NativeScriptItemPrograms {
    NativeScriptItemProgram programs[NATIVE_SCRIPT_ENTITY_LIMIT];
    NativeItemDefinition recipes[NATIVE_SCRIPT_ENTITY_LIMIT];
    NativeScriptItemProjectile projectiles[NATIVE_SCRIPT_ENTITY_LIMIT];
    NativeScriptItemAttachment attachments[NATIVE_SCRIPT_ENTITY_LIMIT];
    char recipe_names[NATIVE_SCRIPT_ENTITY_LIMIT][128];
    NativeScriptItemState states[NATIVE_SCRIPT_ENTITY_STATE_LIMIT];
    NativeScriptItemSignal signals[NATIVE_SCRIPT_ENTITY_SIGNAL_LIMIT];
    NativeScriptItemModelSelector model_selectors[NATIVE_SCRIPT_ENTITY_MODEL_SELECTORS];
    uint32_t model_selector_count;
    NativeScriptItemResourceSelector resource_selectors[NATIVE_SCRIPT_ENTITY_RESOURCE_SELECTORS];
    uint32_t resource_selector_count;
    NativeArticleAnimationOrigin animation_origins[NATIVE_SCRIPT_ENTITY_ANIMATION_ORIGINS];
    uint32_t animation_origin_count;
    NativeScriptItemAnimationDescriptor animation_descriptors[NATIVE_SCRIPT_ENTITY_ANIMATION_DESCRIPTORS];
    uint32_t animation_descriptor_count;
    uint32_t count,state_count,signal_count,recipe_count;
} NativeScriptItemPrograms;
bool nativeScriptItemMetadata_Validate(void*,const NativeScriptSourceTree*,uint32_t,NativeScriptDiagnostic*);
bool nativeScriptItemPrograms_Prepare(const NativeScriptCompileBank*,FighterKind,
    uint64_t program_identity,const NativeScriptItemMetadataContext*,NativeScriptItemPrograms*,NativeScriptDiagnostic*);
// Rebase only internal immutable pointers after copying a prepared package.
// No parsing, allocation, callback or late validation occurs during publication.
void nativeScriptItemPrograms_Rebind(NativeScriptItemPrograms* destination,const NativeScriptItemPrograms* source);
bool nativeScriptItemMetadata_ImportParameters(const NativeScriptProgramIndex*,
    const NativeScriptItemMetadataContext*,NativeScriptParameterSupply*,uint32_t,uint32_t*,NativeScriptDiagnostic*);
#endif
