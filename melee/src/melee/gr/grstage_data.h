#ifndef MELEE_GR_GRSTAGE_DATA_H
#define MELEE_GR_GRSTAGE_DATA_H

#include "types.h"

#include <melee/lb/forward.h>
#include <melee/mp/types.h>
#include <melee/sc/types.h>
#include <melee/it/itarticle_data.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/dat_particle.h>

struct GrStageAssets {
    void* arena;
    void* owned_bytes;
    HSD_Dat dat;
    HSD_DatImport import;
    UnkStageDat map_head;
    MapCollData collision;
    GroundParam ground_param;
    HSD_PSDataBank particle_bank;
    struct GroundItemData** itemdata;
    union CmdUnion** ald_yaku_all;
    ItArticleImportStats item_article_stats;
    ItCommandData item_command_data;
    void* yakumono_param;
    LightList** map_plit;
    DynamicModelDesc* quake_model_set;
    struct { u32 offset, count; void** table; } sis_roots[16];
    u32 sis_root_count;
};

typedef struct GrStageAnimationLayout {
    u32 scene_index;
    u32 first_group_joint_count;
    u32 other_group_joint_count;
} GrStageAnimationLayout;

enum {
    GrStageParamPointer_ColorScript,
    GrStageParamPointer_EnvironmentHit,
};

typedef struct GrStageParamPointer {
    u16 offset;
    u16 kind;
} GrStageParamPointer;

typedef struct GrStageImportOptions {
    size_t yakumono_size;
    const GrStageAnimationLayout* animation_layouts;
    u32 animation_layout_count;
    const u16* yakumono_halfword_offsets;
    u32 yakumono_halfword_count;
    bool icicle_mountain_param;
    const GrStageParamPointer* yakumono_pointers;
    u32 yakumono_pointer_count;
    u32 yakumono_halfword_range_offset;
    u32 yakumono_halfword_range_count;
    bool stage_fragment;
    // ald_yaku_all scripts run on the common Random Pokemon Article.
    const Article* stage_item_script_article;
} GrStageImportOptions;

bool grStageAssets_Import(const void* bytes, size_t size, void* arena,
                          size_t arena_size, u32 record_capacity,
                          const GrStageImportOptions* options,
                          GrStageAssets* output);
DynamicsDesc* grStageAssets_ImportDynamics(GrStageAssets* assets,
                                          const char* symbol);
void** grStageAssets_ImportSis(GrStageAssets* assets, const char* symbol,
                               u32* count);

#endif
