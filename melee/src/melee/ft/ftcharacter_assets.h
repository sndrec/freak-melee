#ifndef MELEE_FT_FTCHARACTER_ASSETS_H
#define MELEE_FT_FTCHARACTER_ASSETS_H

#include "fighter.h"
#include "ftaction_data.h"
#include "ftwaitanim.h"
#include "ftcharacter_asset_layout.h"
#include "ftaccessory_assets.h"

#include <melee/it/itarticle_data.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    FT_CHARACTER_EXT_ATTR_CAPACITY = 0x424,
    FT_CHARACTER_ITEM_CAPACITY = 16,
    FT_CHARACTER_IMPORT_ARENA_SIZE = 2 * 1024 * 1024,
    FT_CHARACTER_IMPORT_RECORD_CAPACITY = 4096,
    FT_CHARACTER_PART_ANIMATION_VARIANTS = 8,
};

typedef struct FtCharacterAssets {
    void* arena;
    void* owned_bytes;
    HSD_Dat dat;
    HSD_DatImport import;
    ftData data;
    ftCo_DatAttrs common_attributes;
    u8 extended_attributes[FT_CHARACTER_EXT_ATTR_CAPACITY];
    struct ftData_x8 model_data;
    struct ftData_x1C* part_animation_groups[5];
    struct ftData_x1C part_animation_group_data[5];
    struct ftData_x20 shield_model;
    HSD_Joint* shield_joints[3];
    struct ftDynamics dynamics;
    struct ArticleDynamicBones dynamic_bones;
    struct ftData_x30 hurtboxes;
    struct ftData_x34 scale;
    struct ftData_x38 collision_bubbles[2];
    void* fighter_items[FT_CHARACTER_ITEM_CAPACITY];
    HSD_Joint* joint_graphs[FT_CHARACTER_ITEM_CAPACITY];
    FtAccessoryAsset* accessory_assets[FT_CHARACTER_ITEM_CAPACITY];
    // Each declared lookup has model_data.x0.model_num group rows. Legacy
    // x48_items aliases the same allocation; owned runtime code uses this type.
    FtPartsVisLookup* visibility_assets[FT_CHARACTER_ITEM_CAPACITY];
    ItArticleImportStats article_stats;
    ItCommandData item_command_data;
    FtActionDataStats action_stats;
    struct UnkFloat6_Camera camera;
    struct itPickup pickup;
    ftData_x44_t collision;
    FtSFX sfx;
    FtSFXArr smash_sfx;
    FtSFXArr light_damage_sfx;
    FtSFXArr damage_sfx;
    Vec2 x50;
    int x54[5];
    struct ftData_x58_t inverse_kinematics;
    HSD_Joint* metal_model;
} FtCharacterAssets;

bool ftCharacterAssets_Import(const void* bytes, size_t size,
                              const FtCharacterAssetLayout* layout,
                              const char* root_name,
                              u32 wait_count, u32 demo_count,
                              u32 costume_count, void* arena,
                              size_t arena_size, u32 record_capacity,
                              FtCharacterAssets* output);

// Check every declared fighter visibility ordinal against an actual imported
// costume descriptor graph, before that package can be registered.
bool ftCharacterAssets_ValidateVisibility(const FtCharacterAssets*,
                                         uint16_t visibility_mask,
                                         const HSD_Joint* costume);

#endif
