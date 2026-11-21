#ifndef MELEE_FT_NATIVE_SOURCE_ASSETS_H
#define MELEE_FT_NATIVE_SOURCE_ASSETS_H

#include "ftcharacter_assets.h"
#include "ftcostume_assets.h"
#include "ftparts.h"
#include "ftdat.h"
#include "native_source_asset_facts.h"
#include "native_fighter_rig.h"
#include "native_fighter_pose.h"

typedef struct NativeSourceAssetInput {
    const void* fighter;size_t fighter_bytes;
    const void* animation;size_t animation_bytes;
    const void* costume;size_t costume_bytes;
} NativeSourceAssetInput;
typedef struct NativeSourceImportedAssets {
    NativeSourceAssetFacts facts;
    NativeFighterRig rig;
    NativeFighterPoseBinding poses[NATIVE_SOURCE_POSE_GRAPHS];
    FtCharacterAssets character;
    FtCostumeAssets costume;
    void* character_arena;
    void* animation_arena;
    MotionState* motions;
    const MotionState* common_motions;
    ItemKind article_kinds[NATIVE_SOURCE_ARTICLES];
    FighterPartsTable parts;
    bool external_hitbox_parts[MAX_FT_PARTS];
    // Donor ordinals that can be instantiated as one joint in both fighter poses.
    uint32_t detachable_joints[FT_CHARACTER_ITEM_CAPACITY][8];
    u8 joint_to_part[MAX_FT_PARTS],part_to_joint[FT_COMMON_PART_COUNT];
    struct Fighter_804D6540_t omissions;
    struct Fighter_804D6540_x0_t omission_rows[NATIVE_SOURCE_OMISSIONS];
    Fighter_DemoStrings demos;
    bool ready;
} NativeSourceImportedAssets;
// Output is a zero-initialized vacant candidate at its final stable address.
// It must never be moved: imported internal pointers target its actual fields.
// Transition allocations only; failure releases this candidate back to vacant.
// Input bytes remain pinned by the candidate owner; accepted assets are separate.
bool nativeSourceAssets_Import(const NativeSourceAssetFacts*,const NativeSourceAssetInput*,
    FighterKind target,NativeSourceImportedAssets*,NativeScriptDiagnostic*);
void nativeSourceAssets_Release(NativeSourceImportedAssets*);

#endif
