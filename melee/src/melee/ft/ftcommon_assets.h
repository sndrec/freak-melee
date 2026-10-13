#ifndef MELEE_FT_FTCOMMON_ASSETS_H
#define MELEE_FT_FTCOMMON_ASSETS_H

#include "fighter.h"
#include "ftcpu_data.h"
#include "ftdat.h"

#include <melee/sfx/crowdsfx.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    FT_COMMON_COLOR_ENTRY_COUNT = 123,
    FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT = 6,
    FT_COMMON_COLOR_COMMAND_CAPACITY = 2048,
    FT_COMMON_PART_TABLE_COUNT = Ft_Kind_None + 1,
};

typedef struct FtCommonAssets {
    void* arena;
    void* owned_bytes;
    HSD_Dat dat;
    HSD_DatImport import;
    ftCommonData common;
    FtCpuData cpu_data;
    float item_throw_attributes[78];
    float swing_multipliers[6][5];
    float stale_move_penalties[9];
    FighterPartsTable* parts[Ft_Kind_RuntimeCapacity];
    FighterPartsTable part_records[FT_COMMON_PART_TABLE_COUNT];
    u8 joint_to_part[FT_COMMON_PART_TABLE_COUNT][256];
    u8 part_to_joint[FT_COMMON_PART_TABLE_COUNT][FT_COMMON_PART_COUNT];
    struct Fighter_804D6540_t* omissions[Ft_Kind_RuntimeCapacity];
    struct Fighter_804D6540_t omission_records[FT_COMMON_PART_TABLE_COUNT];
    struct Fighter_804D6540_x0_t omission_parts[FT_COMMON_PART_TABLE_COUNT][32];
    struct Fighter_804D6518_t zako_modifiers;
    CrowdConfig crowd_config;
    struct Fighter_RespawnPlatform respawn_platform;
    struct Fighter_804D653C_t color_entries[FT_COMMON_COLOR_ENTRY_COUNT];
    struct Fighter_804D653C_t
        special_color_entries[FT_COMMON_SPECIAL_COLOR_ENTRY_COUNT];
    union ColorOverlay_x8_t
        color_commands[FT_COMMON_COLOR_COMMAND_CAPACITY];
    u32 color_command_count;
    struct Fighter_DamageShiftTable damage_shift_tables[3];
    Vec2 damage_shifts[13];
    struct Fighter_ShakeTable_t grab_mash_shake;
    Vec2 grab_mash_offsets[5];
    struct Fighter_ShakeTable_t smash_charge_shake;
    Vec2 smash_charge_offsets[5];
    HSD_Joint* entry_platform;
    u8* x6510_colors;
    u8* x650C_colors;
    u8* x6508_colors;
    struct Fighter_804D651C_t metal_modifiers;
    struct Fighter_804D6520_t bunny_modifiers;
    struct Fighter_804D6524_t scale_modifiers;
    HSD_Joint* metal_model;
} FtCommonAssets;

bool ftCommonAssets_Import(const void* bytes, size_t size, void* arena,
                           size_t arena_size, u32 record_capacity,
                           FtCommonAssets* output);

#endif
