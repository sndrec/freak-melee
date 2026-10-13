#ifndef MELEE_FT_FTACCESSORY_ASSETS_H
#define MELEE_FT_FTACCESSORY_ASSETS_H

#include <sysdolphin/baselib/dat_import.h>

enum { FT_ACCESSORY_ANIMATION_VARIANTS = 16 };

typedef struct FtAccessoryAnimation {
    HSD_AnimJoint* joint;
    HSD_MatAnimJoint* material;
    HSD_ShapeAnimJoint* shape;
} FtAccessoryAnimation;

// Immutable imported data, independent of the serialized archive's layout.
// Each animation selection can replace any of the three HSD channels.
typedef struct FtAccessoryAsset {
    HSD_Joint* root;
    FtAccessoryAnimation base;
    FtAccessoryAnimation variants[FT_ACCESSORY_ANIMATION_VARIANTS];
    u32 variant_count;
} FtAccessoryAsset;

// Absolute serialized POINTER SLOT offsets, not host structure offsets.
// UINT32_MAX omits a channel; an explicit NULL animation slot is also absent.
typedef struct FtAccessoryAnimationSource {
    u32 joint, material, shape;
} FtAccessoryAnimationSource;

typedef struct FtAccessoryAssetSource {
    u32 root;
    FtAccessoryAnimationSource base;
    FtAccessoryAnimationSource variants[FT_ACCESSORY_ANIMATION_VARIANTS];
    u32 variant_count;
} FtAccessoryAssetSource;

// Transition-only normalization through the actual HSD graph importer.
// On failure the import context is invalid; no partial recipe is returned.
FtAccessoryAsset* ftAccessoryAssets_Import(HSD_DatImport*,
                                          const FtAccessoryAssetSource*);
// Verify recipe extent and typed graph provenance in its exact import arena.
bool ftAccessoryAssets_Validate(const HSD_DatImport*, const FtAccessoryAsset*);

#endif
