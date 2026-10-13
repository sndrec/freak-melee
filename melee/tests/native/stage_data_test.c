#include <stdint.h>

#include <aurora/dvd.h>

#include <dolphin/dvd.h>
#include <melee/gr/grstage_data.h>
#include <melee/gr/gricemt_data.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/itcommon_data.h>
#include <melee/lb/forward.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dat.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void* HSD_AudioMalloc(size_t size)
{
    return calloc(1, size);
}

void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%lu: %s\n", file, line, expression);
    abort();
}

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static const GrStageAnimationLayout fd_animation_layouts[] = {
    { 4, 17, 2 }, { 5, 3, 3 }, { 6, 3, 3 },
    { 7, 3, 3 },  { 8, 3, 3 },
};
static const GrStageAnimationLayout mushroom_kingdom_ii_animation_layouts[] = {
    { 2, 8, 8 },
};
static const GrStageAnimationLayout yoshis_island_animation_layouts[] = {
    { 1, 27, 27 },
};
static const GrStageAnimationLayout old_yoshi_animation_layouts[] = {
    { 2, 13, 13 },
};
static const GrStageAnimationLayout old_kongo_animation_layouts[] = {
    { 1, 3, 3 },
};
static const GrStageAnimationLayout venom_animation_layouts[] = {
    { 1, 10, 10 },
    { 3, 9, 9 },
    { 5, 16, 16 },
    { 6, 16, 16 },
};
static const GrStageAnimationLayout brinstar_animation_layouts[] = {
    { 1, 4, 4 },
    { 6, 29, 29 },
    { 7, 2, 2 },
};
static const GrStageParamPointer brinstar_pointers[] = {
    { 0x2C, GrStageParamPointer_EnvironmentHit },
};
static const GrStageAnimationLayout mute_city_animation_layouts[] = {
    { 29, 7, 7 },
};
static const GrStageAnimationLayout corneria_animation_layouts[] = {
    { 1, 5, 5 },
    { 2, 9, 9 },
    { 5, 10, 10 },
    { 10, 16, 16 },
};

static const u16 dream_land_halfword_offsets[] = { 0, 2, 4, 6 };
static const u16 old_yoshi_halfword_offsets[] = {
    0, 2, 0x10, 0x12, 0x14, 0x16, 0x18,
};
static const u16 old_kongo_halfword_offsets[] = {
    0, 2, 0x30, 0x32, 0x34, 0x36, 0x38,
    0x3A, 0x3C, 0x3E,
};
static const u16 great_bay_halfword_offsets[] = {
    0, 2, 0x44, 0x46, 0x48, 0x4A, 0x70, 0x72, 0x74, 0x76,
    0x7C, 0x7E, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8A,
    0x8C, 0x8E, 0x90, 0x92, 0x94, 0x96, 0x98, 0x9A,
    0x9C, 0x9E, 0xA0, 0xA2,
};
static const u16 fourside_halfword_offsets[] = {
    0x44, 0x46, 0x48,
};
static const u16 mushroom_kingdom_halfword_offsets[] = {
    0x14, 0x16, 0x18, 0x1A, 0x1C, 0x1E,
};
static const u16 mushroom_kingdom_ii_halfword_offsets[] = {
    0, 2, 4, 6, 8, 0xA, 0xC, 0xE, 0x10, 0x12, 0x48, 0x4A,
};
static const u16 icicle_mountain_halfword_offsets[] = {
    0, 2, 0x34, 0x36, 0x38, 0x3A, 0x98, 0x9A,
    0xA4, 0xA6, 0xA8, 0xAA,
};
static const GrStageParamPointer battlefield_pointers[] = {
    { 0, GrStageParamPointer_ColorScript },
    { 4, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer final_destination_pointers[] = {
    { 0, GrStageParamPointer_ColorScript },
    { 4, GrStageParamPointer_ColorScript },
    { 8, GrStageParamPointer_ColorScript },
    { 0xC, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer old_kongo_pointers[] = {
    { 0x6C, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer castle_pointers[] = {
    { 0x114, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer kongo_pointers[] = {
    { 0x84, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer corneria_pointers[] = {
    { 0x84, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer venom_pointers[] = {
    { 0x38, GrStageParamPointer_ColorScript },
};
static const GrStageParamPointer mute_city_pointers[] = {
    { 0, GrStageParamPointer_ColorScript },
    { 4, GrStageParamPointer_ColorScript },
    { 8, GrStageParamPointer_EnvironmentHit },
    { 0xC, GrStageParamPointer_EnvironmentHit },
};

static void require_halfwords_match(const GrStageAssets* assets,
                                    const u16* offsets, size_t count,
                                    const char* message)
{
    u32 root;
    require(HSD_DatFindRoot(&assets->dat, "yakumono_param", &root), message);
    for (size_t i = 0; i < count; ++i) {
        u16 source;
        u16 native;
        require(HSD_DatReadU16(&assets->dat, root + offsets[i], &source),
                message);
        memcpy(&native, (u8*) assets->yakumono_param + offsets[i],
               sizeof(native));
        require(native == source, message);
    }
}

static void require_spawn_joint_maps(const GrStageAssets* assets,
                                     const char* message)
{
    bool found[4] = { false, false, false, false };
    GroundJointMapSet* maps = assets->map_head.unk0;
    require(maps != NULL && assets->map_head.unk4 > 0, message);
    for (int i = 0; i < assets->map_head.unk4; ++i) {
        bool scene_matches = false;
        for (int j = 0; j < assets->map_head.unkC; ++j) {
            if (maps[i].joint == assets->map_head.unk8[j].unk0) {
                scene_matches = true;
                break;
            }
        }
        require(scene_matches, message);
        for (int j = 0; j < maps[i].count; ++j) {
            require(maps[i].entries[j].a >= 0 &&
                        maps[i].entries[j].b >= 0 &&
                        maps[i].entries[j].b < GROUND_JOINT_POINT_COUNT,
                    message);
            int point = maps[i].entries[j].b;
            if (point >= 0 && point < 4) {
                found[point] = true;
            }
        }
    }
    require(found[0] && found[1] && found[2] && found[3], message);
}

static void require_stage_import(const char* filename, const char* stage_name,
                                 const GrStageImportOptions* options,
                                 void* arena, size_t arena_size,
                                 GrStageAssets* assets)
{
    DVDFileInfo file;
    require(DVDOpen((char*) filename, &file), stage_name);
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            stage_name);
    if (!grStageAssets_Import(bytes, size, arena, arena_size, 32768, options,
                              assets))
    {
        fprintf(stderr, "%s import status=%d offset=%lx records=%lu\n",
                stage_name, assets->import.status, assets->import.error_offset,
                assets->import.record_count);
        require(false, stage_name);
    }
    if (!(assets->map_head.unkC > 0 && assets->collision.vert_count > 0 &&
          assets->collision.line_count > 0 &&
          assets->ground_param.stage_param_count > 0 &&
          assets->yakumono_param != NULL))
    {
        fprintf(stderr,
                "%s dimensions scenes=%ld vertices=%d lines=%d params=%ld\n",
                stage_name, assets->map_head.unkC,
                assets->collision.vert_count, assets->collision.line_count,
                assets->ground_param.stage_param_count);
        require(false, stage_name);
    }
    _aligned_free(bytes);
}

static void require_pokemon_stadium_transform(const char* filename,
                                              void* arena,
                                              size_t arena_size)
{
    DVDFileInfo file;
    require(DVDOpen((char*) filename, &file),
            "open Pokemon Stadium transformation archive\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Pokemon Stadium transformation archive\n");

    HSD_Dat dat;
    u32 param_root;
    require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
                HSD_DatFindRoot(&dat, "yakumono_param", &param_root),
            "inspect Pokemon Stadium transformation archive\n");
    u32 param_end = HSD_DatNextTarget(&dat, param_root);
    require(param_end == param_root + 0x54,
            "Pokemon Stadium transformation parameter span\n");
    GrStageImportOptions options = {
        .yakumono_size = 0x54,
        .stage_fragment = true,
    };
    GrStageAssets assets;
    if (!grStageAssets_Import(bytes, size, arena, arena_size, 32768,
                              &options, &assets))
    {
        fprintf(stderr,
                "%s import status=%d offset=%lx records=%lu external=%s param=%lx\n",
                filename, assets.import.status, assets.import.error_offset,
                assets.import.record_count,
                assets.import.external_symbol != NULL
                    ? assets.import.external_symbol
                    : "none",
                (unsigned long) options.yakumono_size);
        require(false, "import Pokemon Stadium transformation archive\n");
    }
    require(assets.map_head.unkC > 0 && assets.collision.vert_count > 0 &&
                assets.collision.line_count > 0,
            "Pokemon Stadium transformation geometry\n");
    _aligned_free(bytes);
}

int main(int argc, char** argv)
{
    DVDFileInfo file;
    require(argc == 2, "expected ISO path\n");
    require(aurora_dvd_open(argv[1]), "open test ISO\n");
    // Green Greens' ald_yaku_all commands run on the real common Random
    // Article. Geometry admission must use that imported model, not guessed
    // bounds or an ownerless stage-script exemption.
    DVDFileInfo common_file;
    require(DVDOpen("/ItCo.usd",&common_file),"open common Item archive\n");
    size_t common_size=common_file.length;
    void* common_bytes=_aligned_malloc((common_size+31)&~(size_t)31,32);
    void* common_arena=_aligned_malloc(IT_COMMON_IMPORT_ARENA_SIZE,sizeof(void*));
    ItCommonAssets* common=calloc(1,sizeof(*common));
    require(common_bytes&&common_arena&&common&&
        DVDReadPrio(&common_file,common_bytes,(common_size+31)&~(size_t)31,0,2)>=common_size&&
        DVDClose(&common_file)&&itCommonAssets_Import(common_bytes,common_size,common_arena,
            IT_COMMON_IMPORT_ARENA_SIZE,IT_COMMON_IMPORT_RECORD_CAPACITY,common),
        "import common Item geometry context\n");
    require(DVDOpen("/GrNBa.dat", &file), "open Battlefield archive\n");
    size_t size = file.length;
    size_t padded = (size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    enum { ARENA_SIZE = 4 * 1024 * 1024 };
    void* arena = _aligned_malloc(ARENA_SIZE, sizeof(void*));
    require(bytes != NULL && arena != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Battlefield archive\n");

    GrStageAssets assets;
    GrStageImportOptions battlefield_options = {
        8, NULL, 0, NULL, 0, false, battlefield_pointers,
        ARRAY_SIZE(battlefield_pointers),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &battlefield_options,
                              &assets)) {
        fprintf(stderr, "stage import status=%d offset=%lx records=%lu\n",
                assets.import.status, assets.import.error_offset,
                assets.import.record_count);
        require(false, "import Battlefield stage data\n");
    }
    require(assets.map_head.unkC == 7 && assets.map_head.unk8 != NULL &&
                assets.map_head.unk8[0].unk0 != NULL &&
                assets.map_head.unk8[1].unk4 != NULL &&
                assets.collision.vert_count == 26 &&
                assets.collision.line_count == 23 &&
                assets.collision.joint_count == 1 &&
                assets.ground_param.stage_param_count > 0 &&
                assets.particle_bank.command_count > 0 &&
                assets.particle_bank.texture_group_count > 0 &&
                assets.map_plit != NULL && assets.map_plit[0] != NULL &&
                assets.quake_model_set != NULL &&
                assets.quake_model_set->joint != NULL &&
                assets.itemdata != NULL && assets.itemdata[0] == NULL &&
                assets.ald_yaku_all != NULL &&
                assets.ald_yaku_all[0] == NULL &&
                assets.ald_yaku_all[1] != NULL &&
                assets.ald_yaku_all[2] == NULL &&
                assets.item_article_stats.article_count == 0 &&
                assets.item_command_data.entry_count == 1 &&
                assets.item_command_data.instruction_count == 2 &&
                assets.ald_yaku_all[1] ==
                    &assets.item_command_data.commands[
                        (0x340C8 - assets.item_command_data.source_begin) / 4] &&
                assets.ald_yaku_all[1]->it_create_hitbox_0.opcode == 11 &&
                assets.ald_yaku_all[1]->it_create_hitbox_0.damage == 6,
            "Battlefield stage dimensions\n");
    require_spawn_joint_maps(&assets, "Battlefield spawn joint maps\n");

    GrStageAssets truncated;
    require(!grStageAssets_Import(bytes, size - 1, arena, ARENA_SIZE, 32768,
                                  &battlefield_options, &truncated),
            "Battlefield truncation check\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrGr.dat", &file), "open Green Greens archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Green Greens archive\n");
    GrStageAssets green_greens;
    GrStageImportOptions green_greens_options = { 0x7C, NULL, 0 };
    green_greens_options.stage_item_script_article=common->character_articles[It_PKind_Random-It_Kind_Kuriboh];
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &green_greens_options,
                              &green_greens)) {
        fprintf(stderr, "Green Greens stage import status=%d offset=%lx records=%lu\n",
                green_greens.import.status, green_greens.import.error_offset,
                green_greens.import.record_count);
        require(false, "import Green Greens stage data\n");
    }
    require(green_greens.item_article_stats.article_count == 2 &&
                green_greens.item_article_stats.script_count == 6 &&
                green_greens.itemdata[0] != NULL &&
                green_greens.itemdata[0]->unk0 == 0xE1 &&
                green_greens.itemdata[0]->unk4 != NULL &&
                green_greens.itemdata[1] != NULL &&
                green_greens.itemdata[1]->unk0 == 0xE2 &&
                green_greens.itemdata[1]->unk4 != NULL &&
                green_greens.itemdata[2] == NULL &&
                green_greens.ald_yaku_all[1] != NULL &&
                green_greens.ald_yaku_all[2] == NULL &&
                green_greens.item_command_data.entry_count == 7 &&
                green_greens.item_command_data.instruction_count == 14,
            "Green Greens stage item graph\n");
    itWhispyAppleAttributes* whispy =
        green_greens.itemdata[0]->unk4->x4_specialAttributes;
    require(whispy != NULL && whispy->common != NULL &&
                whispy->common->damage_threshold == 10 &&
                whispy->common->movement_speed == 4.0f &&
                whispy->x4 == 8 && whispy->x8 == 540 &&
                whispy->x14 == 3.0f && whispy->x18 == 0.5f &&
                green_greens.itemdata[1]->unk4->x4_specialAttributes != NULL,
            "Green Greens Whispy apple attributes\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrGd.dat", &file), "open Jungle Japes archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Jungle Japes archive\n");
    GrStageAssets jungle_japes;
    GrStageImportOptions jungle_japes_options = { 0x20, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &jungle_japes_options, &jungle_japes)) {
        fprintf(stderr, "Jungle Japes import status=%d offset=%lx records=%lu\n",
                jungle_japes.import.status,
                jungle_japes.import.error_offset,
                jungle_japes.import.record_count);
        require(false, "import Jungle Japes stage data\n");
    }
    require(jungle_japes.map_head.unkC > 0 &&
                jungle_japes.collision.vert_count > 0 &&
                jungle_japes.collision.line_count > 0 &&
                jungle_japes.yakumono_param != NULL,
            "Jungle Japes stage dimensions\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrSt.dat", &file), "open Yoshi's Story archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Yoshi's Story archive\n");
    GrStageAssets yoshis_story;
    GrStageImportOptions yoshis_story_options = { 0x24, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &yoshis_story_options, &yoshis_story)) {
        fprintf(stderr, "Yoshi's Story import status=%d offset=%lx records=%lu\n",
                yoshis_story.import.status, yoshis_story.import.error_offset,
                yoshis_story.import.record_count);
        require(false, "import Yoshi's Story stage data\n");
    }
    require(yoshis_story.map_head.unkC > 0 &&
                yoshis_story.collision.vert_count > 0 &&
                yoshis_story.collision.line_count > 0 &&
                yoshis_story.yakumono_param != NULL,
            "Yoshi's Story stage dimensions\n");
    require(yoshis_story.itemdata != NULL &&
                yoshis_story.itemdata[0] != NULL &&
                yoshis_story.itemdata[0]->unk0 == It_Kind_Heiho &&
                yoshis_story.itemdata[0]->unk4 != NULL,
            "Yoshi's Story Shy Guy article\n");
    itHeihoAttributes* heiho =
        yoshis_story.itemdata[0]->unk4->x4_specialAttributes;
    require(heiho != NULL && heiho->common != NULL &&
                heiho->common->damage_threshold == 15 &&
                heiho->common->movement_speed == 0.5f &&
                heiho->movement_speeds[0] == 0.3f &&
                heiho->movement_speeds[3] == 1.5f &&
                heiho->horizontal_speed_variation == 2.0f,
            "Yoshi's Story Shy Guy attributes\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrFz.dat", &file), "open Flat Zone archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Flat Zone archive\n");
    GrStageAssets flat_zone;
    GrStageImportOptions flat_zone_options = { 0x40, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &flat_zone_options, &flat_zone)) {
        fprintf(stderr, "Flat Zone import status=%d offset=%lx records=%lu\n",
                flat_zone.import.status, flat_zone.import.error_offset,
                flat_zone.import.record_count);
        require(false, "import Flat Zone stage data\n");
    }
    require(flat_zone.map_head.unkC > 0 &&
                flat_zone.collision.vert_count > 0 &&
                flat_zone.collision.line_count > 0 &&
                flat_zone.yakumono_param != NULL,
            "Flat Zone stage dimensions\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrOy.dat", &file), "open Yoshi's Island (64) archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Yoshi's Island (64) archive\n");
    GrStageAssets old_yoshi;
    GrStageImportOptions old_yoshi_options = {
        0x1C, old_yoshi_animation_layouts,
        sizeof(old_yoshi_animation_layouts) /
            sizeof(old_yoshi_animation_layouts[0]),
        old_yoshi_halfword_offsets,
        sizeof(old_yoshi_halfword_offsets) /
            sizeof(old_yoshi_halfword_offsets[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &old_yoshi_options, &old_yoshi)) {
        fprintf(stderr, "Yoshi's Island (64) import status=%d offset=%lx records=%lu\n",
                old_yoshi.import.status, old_yoshi.import.error_offset,
                old_yoshi.import.record_count);
        require(false, "import Yoshi's Island (64) stage data\n");
    }
    require(old_yoshi.map_head.unkC > 2 &&
                old_yoshi.collision.vert_count > 0 &&
                old_yoshi.collision.line_count > 0 &&
                old_yoshi.yakumono_param != NULL &&
                old_yoshi.map_head.unk8[2].unk4 != NULL &&
                old_yoshi.map_head.unk8[2].unk4[1] != NULL &&
                old_yoshi.map_head.unk8[2].unk4[1][12].aobjdesc != NULL &&
                old_yoshi.map_head.unk8[2].unk8 != NULL &&
                old_yoshi.map_head.unk8[2].unk8[1] != NULL &&
                old_yoshi.map_head.unk8[2].unk8[1][12].matanim != NULL,
            "Yoshi's Island (64) stage dimensions\n");
    require_halfwords_match(&old_yoshi, old_yoshi_halfword_offsets,
                            sizeof(old_yoshi_halfword_offsets) /
                                sizeof(old_yoshi_halfword_offsets[0]),
                            "Yoshi's Island (64) yakumono halfword endian\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrOk.dat", &file), "open Kongo Jungle (64) archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Kongo Jungle (64) archive\n");
    GrStageAssets old_kongo;
    GrStageImportOptions old_kongo_options = {
        0x70, old_kongo_animation_layouts,
        ARRAY_SIZE(old_kongo_animation_layouts), old_kongo_halfword_offsets,
        sizeof(old_kongo_halfword_offsets) /
            sizeof(old_kongo_halfword_offsets[0]),
        false, old_kongo_pointers,
        ARRAY_SIZE(old_kongo_pointers),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &old_kongo_options, &old_kongo)) {
        fprintf(stderr, "Kongo Jungle (64) import status=%d offset=%lx records=%lu\n",
                old_kongo.import.status, old_kongo.import.error_offset,
                old_kongo.import.record_count);
        require(false, "import Kongo Jungle (64) stage data\n");
    }
    require(old_kongo.map_head.unkC > 1 &&
                old_kongo.collision.vert_count > 0 &&
                old_kongo.collision.line_count > 0 &&
                old_kongo.yakumono_param != NULL &&
                old_kongo.map_head.unk8[1].unk4 != NULL &&
                old_kongo.map_head.unk8[1].unk4[0] != NULL &&
                old_kongo.map_head.unk8[1].unk4[1] != NULL &&
                old_kongo.map_head.unk8[1].unk4[2] != NULL &&
                old_kongo.map_head.unk8[1].unk4[0][2].child == NULL &&
                old_kongo.map_head.unk8[1].unk4[0][2].aobjdesc != NULL &&
                old_kongo.map_head.unk8[1].unk4[1][2].child == NULL &&
                old_kongo.map_head.unk8[1].unk4[1][2].aobjdesc != NULL &&
                old_kongo.map_head.unk8[1].unk4[2][2].child == NULL &&
                old_kongo.map_head.unk8[1].unk4[2][2].aobjdesc != NULL,
            "Kongo Jungle (64) stage dimensions\n");
    require_halfwords_match(&old_kongo, old_kongo_halfword_offsets,
                            sizeof(old_kongo_halfword_offsets) /
                                sizeof(old_kongo_halfword_offsets[0]),
                            "Kongo Jungle (64) yakumono halfword endian\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrIz.dat", &file),
            "open Fountain of Dreams archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Fountain of Dreams archive\n");
    GrStageAssets fountain;
    GrStageImportOptions fountain_options = { 0x54, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &fountain_options, &fountain)) {
        fprintf(stderr, "Fountain of Dreams import status=%d offset=%lx records=%lu\n",
                fountain.import.status, fountain.import.error_offset,
                fountain.import.record_count);
        require(false, "import Fountain of Dreams stage data\n");
    }
    require(fountain.map_head.unkC > 0 &&
                fountain.collision.vert_count > 0 &&
                fountain.collision.line_count > 0 &&
                fountain.yakumono_param != NULL,
            "Fountain of Dreams stage dimensions\n");
    u32 fountain_root;
    require(HSD_DatFindRoot(
                &fountain.dat,
                "GrdIzumi_cd_wt_GrdIzumiDummy1_1_image_desc",
                &fountain_root) &&
                HSD_DatImportGraph(&fountain.import, fountain_root,
                                   DAT_IMAGE) != NULL,
            "Fountain of Dreams reflection image\n");
    require(HSD_DatFindRoot(&fountain.dat, "GrdIzumiStar_TopN_joint",
                            &fountain_root) &&
                HSD_DatImportGraph(&fountain.import, fountain_root,
                                   DAT_JOINT) != NULL,
            "Fountain of Dreams star joint\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrRc.dat", &file), "open Rainbow Cruise archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Rainbow Cruise archive\n");
    GrStageAssets rainbow;
    GrStageImportOptions rainbow_options = { 0x48, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &rainbow_options, &rainbow)) {
        fprintf(stderr, "Rainbow Cruise import status=%d offset=%lx records=%lu\n",
                rainbow.import.status, rainbow.import.error_offset,
                rainbow.import.record_count);
        require(false, "import Rainbow Cruise stage data\n");
    }
    DynamicsDesc* ship_flag = grStageAssets_ImportDynamics(
        &rainbow, "dynamicsdata_shipflag");
    require(ship_flag != NULL && ship_flag->data != NULL &&
                ship_flag->count == 6 && rainbow.map_head.unkC > 0 &&
                rainbow.collision.vert_count > 0 &&
                rainbow.collision.line_count > 0,
            "Rainbow Cruise ship-flag dynamics\n");
    u32 ship_flag_root, ship_flag_data;
    const char* external;
    require(HSD_DatFindRoot(&rainbow.dat, "dynamicsdata_shipflag",
                            &ship_flag_root) &&
                HSD_DatResolve(&rainbow.dat, ship_flag_root, &ship_flag_data,
                               &external) == HSD_DAT_REF_LOCAL,
            "Rainbow Cruise dynamics source\n");
    for (u32 i = 0; i < ship_flag->count *
                            sizeof(struct lb_00F9_UnkDesc1Inner); i += 4) {
        u32 source_word, native_word;
        require(HSD_DatReadU32(&rainbow.dat, ship_flag_data + i,
                               &source_word),
                "Rainbow Cruise dynamics word\n");
        memcpy(&native_word, (u8*) ship_flag->data + i, sizeof(native_word));
        require(native_word == source_word,
                "Rainbow Cruise dynamics endian\n");
    }
    _aligned_free(bytes);

    require(DVDOpen("/GrGb.dat", &file), "open Great Bay archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Great Bay archive\n");
    GrStageAssets great_bay;
    GrStageImportOptions great_bay_options = {
        0xA4, NULL, 0, great_bay_halfword_offsets,
        sizeof(great_bay_halfword_offsets) /
            sizeof(great_bay_halfword_offsets[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &great_bay_options, &great_bay)) {
        fprintf(stderr, "Great Bay import status=%d offset=%lx records=%lu\n",
                great_bay.import.status, great_bay.import.error_offset,
                great_bay.import.record_count);
        require(false, "import Great Bay stage data\n");
    }
    require(great_bay.map_head.unkC > 9 &&
                great_bay.collision.vert_count > 0 &&
                great_bay.collision.line_count > 0,
            "Great Bay stage geometry\n");
    HSD_AnimJoint* moon_anim = great_bay.map_head.unk8[9].unk4[0];
    const u8* moon_scene_flags = great_bay.map_head.unk8[9].x28;
    require(moon_anim != NULL && moon_anim->aobjdesc != NULL &&
                moon_anim->aobjdesc->flags == 0 &&
                moon_anim->aobjdesc->end_frame == 4690.0f &&
                moon_scene_flags != NULL && moon_scene_flags[0] == 0,
            "Great Bay moon animation flags\n");
    require_halfwords_match(&great_bay, great_bay_halfword_offsets,
                            sizeof(great_bay_halfword_offsets) /
                                sizeof(great_bay_halfword_offsets[0]),
                            "Great Bay yakumono halfword endian\n");
    require(great_bay.itemdata != NULL && great_bay.itemdata[0] != NULL &&
                great_bay.itemdata[0]->unk0 == It_Kind_Tincle &&
                great_bay.itemdata[0]->unk4 != NULL,
            "Great Bay Tingle article\n");
    itTincleAttributes* tincle =
        great_bay.itemdata[0]->unk4->x4_specialAttributes;
    require(tincle != NULL && tincle->common != NULL &&
                tincle->common->damage_threshold == 10 &&
                tincle->common->movement_speed == 0.3f &&
                tincle->x4 == 600 && tincle->x8 == 2400 &&
                tincle->x54 == 3 && tincle->x55 == 6,
            "Great Bay Tingle attributes\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrSh.dat", &file), "open Hyrule Temple archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Hyrule Temple archive\n");
    GrStageAssets hyrule_temple;
    GrStageImportOptions hyrule_temple_options = { 4, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &hyrule_temple_options, &hyrule_temple)) {
        fprintf(stderr, "Hyrule Temple import status=%d offset=%lx records=%lu\n",
                hyrule_temple.import.status,
                hyrule_temple.import.error_offset,
                hyrule_temple.import.record_count);
        require(false, "import Hyrule Temple stage data\n");
    }
    u32 hyrule_param_root;
    require(HSD_DatFindRoot(&hyrule_temple.dat, "yakumono_param",
                            &hyrule_param_root) &&
                HSD_DatNextTarget(&hyrule_temple.dat, hyrule_param_root) ==
                    hyrule_param_root + 4 &&
                hyrule_temple.map_head.unkC > 0 &&
                hyrule_temple.collision.vert_count > 0 &&
                hyrule_temple.collision.line_count > 0,
            "Hyrule Temple stage geometry and parameter span\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrOt.dat", &file), "open Onett archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Onett archive\n");
    GrStageAssets onett;
    GrStageImportOptions onett_options = { 0x68, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &onett_options, &onett)) {
        fprintf(stderr, "Onett import status=%d offset=%lx records=%lu\n",
                onett.import.status, onett.import.error_offset,
                onett.import.record_count);
        require(false, "import Onett stage data\n");
    }
    u32 onett_param_root;
    require(HSD_DatFindRoot(&onett.dat, "yakumono_param",
                            &onett_param_root) &&
                HSD_DatNextTarget(&onett.dat, onett_param_root) ==
                    onett_param_root + 0x68 &&
                onett.map_head.unkC > 1 &&
                onett.map_head.unk8[1].unk0 != NULL &&
                onett.map_head.unk8[1].unk4 != NULL &&
                onett.map_head.unk8[1].unk4[0] != NULL &&
                onett.collision.vert_count > 0 &&
                onett.collision.line_count > 0,
            "Onett stage geometry and parameter span\n");
    require_spawn_joint_maps(&onett, "Onett spawn joint maps\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrPs.dat", &file), "open Pokemon Stadium archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Pokemon Stadium archive\n");
    static const u16 pokemon_stadium_halfwords[] = {
        0x48, 0x4A, 0x4C, 0x4E, 0x50,
    };
    GrStageAssets pokemon_stadium;
    GrStageImportOptions pokemon_stadium_options = {
        0x54, NULL, 0, pokemon_stadium_halfwords,
        ARRAY_SIZE(pokemon_stadium_halfwords),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &pokemon_stadium_options, &pokemon_stadium)) {
        fprintf(stderr,
                "Pokemon Stadium import status=%d offset=%lx records=%lu external=%s\n",
                pokemon_stadium.import.status,
                pokemon_stadium.import.error_offset,
                pokemon_stadium.import.record_count,
                pokemon_stadium.import.external_symbol != NULL
                    ? pokemon_stadium.import.external_symbol
                    : "none");
        require(false, "import Pokemon Stadium stage data\n");
    }
    u32 pokemon_stadium_param_root;
    require(HSD_DatFindRoot(&pokemon_stadium.dat, "yakumono_param",
                            &pokemon_stadium_param_root) &&
                HSD_DatNextTarget(&pokemon_stadium.dat,
                                  pokemon_stadium_param_root) ==
                    pokemon_stadium_param_root + 0x54 &&
                pokemon_stadium.collision.vert_count > 0 &&
                pokemon_stadium.collision.line_count > 0,
            "Pokemon Stadium stage geometry and parameter span\n");
    require_spawn_joint_maps(&pokemon_stadium,
                             "Pokemon Stadium spawn joint maps\n");
    u32 pokemon_stadium_sis_count;
    size_t imported_bytes = pokemon_stadium.import.arena_used;
    void** sis = grStageAssets_ImportSis(&pokemon_stadium,
                    "SIS_GrPStadiumData", &pokemon_stadium_sis_count);
    require(sis != NULL && pokemon_stadium_sis_count == 22 &&
                sis == grStageAssets_ImportSis(&pokemon_stadium,
                    "SIS_GrPStadiumData", NULL) &&
                imported_bytes == pokemon_stadium.import.arena_used,
            "Pokemon Stadium SIS data imported once during stage loading\n");
    _aligned_free(bytes);
    require_pokemon_stadium_transform("/GrPs1.dat", arena, ARENA_SIZE);
    require_pokemon_stadium_transform("/GrPs2.dat", arena, ARENA_SIZE);
    require_pokemon_stadium_transform("/GrPs3.dat", arena, ARENA_SIZE);
    require_pokemon_stadium_transform("/GrPs4.dat", arena, ARENA_SIZE);

    require(DVDOpen("/GrBb.dat", &file), "open Big Blue archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Big Blue archive\n");
    GrStageAssets big_blue;
    GrStageImportOptions big_blue_options = { 0x144, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &big_blue_options, &big_blue)) {
        fprintf(stderr,
                "Big Blue import status=%d offset=%lx records=%lu external=%s\n",
                big_blue.import.status, big_blue.import.error_offset,
                big_blue.import.record_count,
                big_blue.import.external_symbol != NULL
                    ? big_blue.import.external_symbol
                    : "none");
        require(false, "import Big Blue stage data\n");
    }
    u32 big_blue_param_root;
    require(HSD_DatFindRoot(&big_blue.dat, "yakumono_param",
                            &big_blue_param_root) &&
                HSD_DatNextTarget(&big_blue.dat, big_blue_param_root) ==
                    big_blue_param_root + 0x144 &&
                big_blue.map_head.unkC > 2 &&
                big_blue.collision.vert_count > 0 &&
                big_blue.collision.line_count > 0,
            "Big Blue stage geometry and parameter span\n");
    require_spawn_joint_maps(&big_blue, "Big Blue spawn joint maps\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrKr.dat", &file), "open Brinstar Depths archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Brinstar Depths archive\n");
    GrStageAssets brinstar_depths;
    const GrStageAnimationLayout brinstar_depths_animation_layouts[] = {
        { 3, 19, 19 },
    };
    GrStageImportOptions brinstar_depths_options = {
        0x34, brinstar_depths_animation_layouts,
        sizeof(brinstar_depths_animation_layouts) /
            sizeof(brinstar_depths_animation_layouts[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &brinstar_depths_options, &brinstar_depths)) {
        fprintf(stderr, "Brinstar Depths import status=%d offset=%lx records=%lu\n",
                brinstar_depths.import.status,
                brinstar_depths.import.error_offset,
                brinstar_depths.import.record_count);
        require(false, "import Brinstar Depths stage data\n");
    }
    u32 brinstar_param_root;
    require(HSD_DatFindRoot(&brinstar_depths.dat, "yakumono_param",
                            &brinstar_param_root) &&
                HSD_DatNextTarget(&brinstar_depths.dat, brinstar_param_root) ==
                    brinstar_param_root + 0x34 &&
                brinstar_depths.map_head.unkC > 0 &&
                brinstar_depths.collision.vert_count > 0 &&
                brinstar_depths.collision.line_count > 0,
            "Brinstar Depths stage geometry and parameter span\n");
    require_spawn_joint_maps(&brinstar_depths,
                             "Brinstar Depths spawn joint maps\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrFs.dat", &file), "open Fourside archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Fourside archive\n");
    GrStageAssets fourside;
    GrStageImportOptions fourside_options = {
        0x4C, NULL, 0, fourside_halfword_offsets,
        sizeof(fourside_halfword_offsets) /
            sizeof(fourside_halfword_offsets[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &fourside_options, &fourside)) {
        fprintf(stderr, "Fourside import status=%d offset=%lx records=%lu\n",
                fourside.import.status, fourside.import.error_offset,
                fourside.import.record_count);
        require(false, "import Fourside stage data\n");
    }
    u32 fourside_param_root;
    require(HSD_DatFindRoot(&fourside.dat, "yakumono_param",
                            &fourside_param_root) &&
                HSD_DatNextTarget(&fourside.dat, fourside_param_root) ==
                    fourside_param_root + 0x4C &&
                fourside.map_head.unkC > 0 &&
                fourside.collision.vert_count > 0 &&
                fourside.collision.line_count > 0,
            "Fourside stage geometry and parameter span\n");
    require_halfwords_match(&fourside, fourside_halfword_offsets,
                            sizeof(fourside_halfword_offsets) /
                                sizeof(fourside_halfword_offsets[0]),
                            "Fourside yakumono halfword endian\n");
    require_spawn_joint_maps(&fourside, "Fourside spawn joint maps\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrI1.dat", &file), "open Mushroom Kingdom archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Mushroom Kingdom archive\n");
    GrStageAssets mushroom_kingdom;
    GrStageImportOptions mushroom_kingdom_options = {
        0x54, NULL, 0, mushroom_kingdom_halfword_offsets,
        sizeof(mushroom_kingdom_halfword_offsets) /
            sizeof(mushroom_kingdom_halfword_offsets[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &mushroom_kingdom_options, &mushroom_kingdom)) {
        fprintf(stderr, "Mushroom Kingdom import status=%d offset=%lx records=%lu\n",
                mushroom_kingdom.import.status,
                mushroom_kingdom.import.error_offset,
                mushroom_kingdom.import.record_count);
        require(false, "import Mushroom Kingdom stage data\n");
    }
    u32 mushroom_kingdom_param_root;
    require(HSD_DatFindRoot(&mushroom_kingdom.dat, "yakumono_param",
                            &mushroom_kingdom_param_root) &&
                HSD_DatNextTarget(&mushroom_kingdom.dat,
                                  mushroom_kingdom_param_root) ==
                    mushroom_kingdom_param_root + 0x54 &&
                mushroom_kingdom.map_head.unkC > 0 &&
                mushroom_kingdom.collision.vert_count > 0 &&
                mushroom_kingdom.collision.line_count > 0,
            "Mushroom Kingdom stage geometry and parameter span\n");
    require_halfwords_match(&mushroom_kingdom,
                            mushroom_kingdom_halfword_offsets,
                            sizeof(mushroom_kingdom_halfword_offsets) /
                                sizeof(mushroom_kingdom_halfword_offsets[0]),
                            "Mushroom Kingdom yakumono halfword endian\n");
    require_spawn_joint_maps(&mushroom_kingdom,
                             "Mushroom Kingdom spawn joint maps\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrI2.dat", &file), "open Mushroom Kingdom II archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Mushroom Kingdom II archive\n");
    GrStageAssets mushroom_kingdom_ii;
    GrStageImportOptions mushroom_kingdom_ii_options = {
        0x4C, mushroom_kingdom_ii_animation_layouts,
        sizeof(mushroom_kingdom_ii_animation_layouts) /
            sizeof(mushroom_kingdom_ii_animation_layouts[0]),
        mushroom_kingdom_ii_halfword_offsets,
        sizeof(mushroom_kingdom_ii_halfword_offsets) /
            sizeof(mushroom_kingdom_ii_halfword_offsets[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &mushroom_kingdom_ii_options,
                              &mushroom_kingdom_ii)) {
        fprintf(stderr, "Mushroom Kingdom II import status=%d offset=%lx "
                        "records=%lu\n", mushroom_kingdom_ii.import.status,
                mushroom_kingdom_ii.import.error_offset,
                mushroom_kingdom_ii.import.record_count);
        require(false, "import Mushroom Kingdom II stage data\n");
    }
    u32 mushroom_kingdom_ii_param_root;
    require(HSD_DatFindRoot(&mushroom_kingdom_ii.dat, "yakumono_param",
                            &mushroom_kingdom_ii_param_root) &&
                HSD_DatNextTarget(&mushroom_kingdom_ii.dat,
                                  mushroom_kingdom_ii_param_root) ==
                    mushroom_kingdom_ii_param_root + 0x4C &&
                mushroom_kingdom_ii.map_head.unkC > 0 &&
                mushroom_kingdom_ii.collision.vert_count > 0 &&
                mushroom_kingdom_ii.collision.line_count > 0,
            "Mushroom Kingdom II stage geometry and parameter span\n");
    require_halfwords_match(&mushroom_kingdom_ii,
                            mushroom_kingdom_ii_halfword_offsets,
                            sizeof(mushroom_kingdom_ii_halfword_offsets) /
                                sizeof(mushroom_kingdom_ii_halfword_offsets[0]),
                            "Mushroom Kingdom II yakumono halfword endian\n");
    require_spawn_joint_maps(&mushroom_kingdom_ii,
                             "Mushroom Kingdom II spawn joint maps\n");
    require(mushroom_kingdom_ii.map_head.unkC > 2 &&
                mushroom_kingdom_ii.map_head.unk8[2].unk8 != NULL &&
                mushroom_kingdom_ii.map_head.unk8[2].unk8[0] != NULL &&
                mushroom_kingdom_ii.map_head.unk8[2].unk8[1] != NULL &&
                mushroom_kingdom_ii.map_head.unk8[2].unk8[2] != NULL &&
                mushroom_kingdom_ii.map_head.unk8[2].unk8[1][6].matanim != NULL,
            "Mushroom Kingdom II indexed stage animations\n");
    require(mushroom_kingdom_ii.itemdata != NULL &&
                mushroom_kingdom_ii.itemdata[0] != NULL &&
                mushroom_kingdom_ii.itemdata[0]->unk0 == It_Kind_Kyasarin &&
                mushroom_kingdom_ii.itemdata[0]->unk4 != NULL &&
                mushroom_kingdom_ii.itemdata[1] != NULL &&
                mushroom_kingdom_ii.itemdata[1]->unk0 ==
                    It_Kind_Kyasarin_Egg &&
                mushroom_kingdom_ii.itemdata[1]->unk4 != NULL,
            "Mushroom Kingdom II Birdo articles\n");
    itKyasarinAttributes* kyasarin =
        mushroom_kingdom_ii.itemdata[0]->unk4->x4_specialAttributes;
    itKyasarinEggAttributes* kyasarin_egg =
        mushroom_kingdom_ii.itemdata[1]->unk4->x4_specialAttributes;
    require(kyasarin != NULL && kyasarin->common != NULL &&
                kyasarin->common->damage_threshold == 6 &&
                kyasarin->common->movement_speed == 0.5f &&
                kyasarin->x4 == 0.4f && kyasarin->x8 == 20.0f &&
                kyasarin->x40 == 10 && kyasarin->x44 == 20 &&
                kyasarin->x48 == 3 && kyasarin_egg != NULL &&
                kyasarin_egg->common != NULL &&
                kyasarin_egg->common->damage_threshold == 6 &&
                kyasarin_egg->x4 == 1.4f && kyasarin_egg->x8 == 3.0f &&
                kyasarin_egg->xC == 1 && kyasarin_egg->x10 == 8,
            "Mushroom Kingdom II Birdo attributes\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrYt.dat", &file), "open Yoshi's Island archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Yoshi's Island archive\n");
    GrStageAssets yoshis_island;
    GrStageImportOptions yoshis_island_options = {
        0x20, yoshis_island_animation_layouts,
        sizeof(yoshis_island_animation_layouts) /
            sizeof(yoshis_island_animation_layouts[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &yoshis_island_options, &yoshis_island)) {
        fprintf(stderr, "Yoshi's Island import status=%d offset=%lx "
                        "records=%lu\n", yoshis_island.import.status,
                yoshis_island.import.error_offset,
                yoshis_island.import.record_count);
        require(false, "import Yoshi's Island stage data\n");
    }
    u32 yoshis_island_param_root;
    u32 yoshis_island_tail;
    require(HSD_DatFindRoot(&yoshis_island.dat, "yakumono_param",
                            &yoshis_island_param_root) &&
                HSD_DatNextTarget(&yoshis_island.dat,
                                  yoshis_island_param_root) ==
                    yoshis_island_param_root + 0x24 &&
                HSD_DatReadU32(&yoshis_island.dat,
                               yoshis_island_param_root + 0x20,
                               &yoshis_island_tail) &&
                yoshis_island_tail == 0 &&
                yoshis_island.map_head.unkC > 1 &&
                yoshis_island.collision.vert_count > 0 &&
                yoshis_island.collision.line_count > 0,
            "Yoshi's Island stage geometry and parameter span\n");
    require_spawn_joint_maps(&yoshis_island,
                             "Yoshi's Island spawn joint maps\n");
    require(yoshis_island.map_head.unk8[1].unk8 != NULL &&
                yoshis_island.map_head.unk8[1].unk8[3] != NULL &&
                yoshis_island.map_head.unk8[1].unk8[3][26].matanim != NULL,
            "Yoshi's Island indexed stage animations\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrPu.dat", &file), "open Poke Floats archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Poke Floats archive\n");
    GrStageAssets poke_floats;
    GrStageImportOptions poke_floats_options = { 4, NULL, 0 };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &poke_floats_options, &poke_floats)) {
        fprintf(stderr, "Poke Floats import status=%d offset=%lx records=%lu\n",
                poke_floats.import.status,
                poke_floats.import.error_offset,
                poke_floats.import.record_count);
        require(false, "import Poke Floats stage data\n");
    }
    u32 poke_floats_param_root;
    require(HSD_DatFindRoot(&poke_floats.dat, "yakumono_param",
                            &poke_floats_param_root) &&
                HSD_DatNextTarget(&poke_floats.dat, poke_floats_param_root) ==
                    poke_floats_param_root + 4 &&
                poke_floats.map_head.unkC > 0 &&
                poke_floats.collision.vert_count > 0 &&
                poke_floats.collision.line_count > 0,
            "Poke Floats stage geometry and parameter span\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrIm.dat", &file), "open Icicle Mountain archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Icicle Mountain archive\n");
    GrStageAssets icicle_mountain;
    GrStageImportOptions icicle_mountain_options = {
        0x13C, NULL, 0, NULL, 0, true,
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &icicle_mountain_options, &icicle_mountain)) {
        fprintf(stderr, "Icicle Mountain import status=%d offset=%lx records=%lu\n",
                icicle_mountain.import.status,
                icicle_mountain.import.error_offset,
                icicle_mountain.import.record_count);
        require(false, "import Icicle Mountain stage data\n");
    }
    u32 icicle_param_root;
    require(HSD_DatFindRoot(&icicle_mountain.dat, "yakumono_param",
                            &icicle_param_root) &&
                HSD_DatNextTarget(&icicle_mountain.dat, icicle_param_root) ==
                    icicle_param_root + 0x13C &&
                icicle_mountain.map_head.unkC > 0 &&
                icicle_mountain.collision.vert_count > 0 &&
                icicle_mountain.collision.line_count > 0,
            "Icicle Mountain geometry and parameter span\n");
    require_halfwords_match(&icicle_mountain,
                            icicle_mountain_halfword_offsets,
                            sizeof(icicle_mountain_halfword_offsets) /
                                sizeof(icicle_mountain_halfword_offsets[0]),
                            "Icicle Mountain scalar halfwords\n");
    const GrIceMtYakumonoParam* icicle_param =
        icicle_mountain.yakumono_param;
    s16* const icicle_arrays[] = {
        icicle_param->field_ixs, icicle_param->xB0, icicle_param->xB4,
    };
    const u32 icicle_counts[] = { 16, 12, 12 };
    const char* icicle_external;
    for (u32 i = 0; i < 3; ++i) {
        u32 source;
        require(HSD_DatResolve(&icicle_mountain.dat,
                               icicle_param_root + 0xAC + i * 4,
                               &source, &icicle_external) ==
                    HSD_DAT_REF_LOCAL && icicle_arrays[i] != NULL,
                "Icicle Mountain parameter array reference\n");
        for (u32 j = 0; j < icicle_counts[i]; ++j) {
            u16 value;
            require(HSD_DatReadU16(&icicle_mountain.dat, source + j * 2,
                                   &value) &&
                        icicle_arrays[i][j] == (s16) value,
                    "Icicle Mountain parameter array values\n");
        }
    }
    for (u32 i = 0; i < 32; ++i) {
        u32 at = icicle_param_root + 0xBC + i * 4;
        u16 kind;
        const u8* record = HSD_DatBytes(&icicle_mountain.dat, at, 4);
        require(record != NULL &&
                    HSD_DatReadU16(&icicle_mountain.dat, at, &kind) &&
                    icicle_param->xBC[i].kind == kind &&
                    icicle_param->xBC[i].x2 == record[2] &&
                    icicle_param->xBC[i].respawn == record[3],
                    "Icicle Mountain spawn description values\n");
    }
    require(icicle_mountain.itemdata != NULL &&
                icicle_mountain.itemdata[0] != NULL &&
                icicle_mountain.itemdata[0]->unk0 == It_Kind_Whitebea &&
                icicle_mountain.itemdata[0]->unk4 != NULL,
            "Icicle Mountain Polar Bear article\n");
    itWhiteBeaAttributes* whitebea =
        icicle_mountain.itemdata[0]->unk4->x4_specialAttributes;
    require(whitebea != NULL && whitebea->common != NULL &&
                whitebea->common->damage_threshold == 25 &&
                whitebea->common->movement_speed == 0.3f &&
                whitebea->x4 == 0.5f && whitebea->x8 == 120 &&
                whitebea->xA == 320 && whitebea->xC == 240 &&
                whitebea->xE == 480 && whitebea->x10 == 1.8f &&
                whitebea->x14 == 20,
            "Icicle Mountain Polar Bear attributes\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrOp.dat", &file), "open Dream Land archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Dream Land archive\n");
    GrStageAssets dream_land;
    GrStageImportOptions dream_land_options = {
        0x34, NULL, 0, dream_land_halfword_offsets,
        sizeof(dream_land_halfword_offsets) /
            sizeof(dream_land_halfword_offsets[0]),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &dream_land_options, &dream_land)) {
        fprintf(stderr, "Dream Land import status=%d offset=%lx records=%lu\n",
                dream_land.import.status, dream_land.import.error_offset,
                dream_land.import.record_count);
        require(false, "import Dream Land stage data\n");
    }
    require(dream_land.map_head.unkC == 8 &&
                dream_land.collision.vert_count > 0 &&
                dream_land.collision.line_count > 0 &&
                dream_land.ground_param.stage_param_count > 0 &&
                dream_land.yakumono_param != NULL,
            "Dream Land stage dimensions\n");
    require_halfwords_match(&dream_land, dream_land_halfword_offsets,
                            sizeof(dream_land_halfword_offsets) /
                                sizeof(dream_land_halfword_offsets[0]),
                            "Dream Land yakumono halfword endian\n");
    _aligned_free(bytes);

    require(DVDOpen("/GrNLa.dat", &file), "open Final Destination archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Final Destination archive\n");
    GrStageAssets final_destination;
    GrStageImportOptions final_destination_options = {
        16, fd_animation_layouts,
        sizeof(fd_animation_layouts) / sizeof(fd_animation_layouts[0]),
        NULL, 0, false, final_destination_pointers,
        ARRAY_SIZE(final_destination_pointers),
    };
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &final_destination_options,
                              &final_destination)) {
        fprintf(stderr, "Final Destination import status=%d offset=%lx "
                        "records=%lu\n", final_destination.import.status,
                final_destination.import.error_offset,
                final_destination.import.record_count);
        require(false, "import Final Destination stage data\n");
    }
    require(final_destination.map_head.unkC > 0 &&
                final_destination.collision.vert_count > 0 &&
                final_destination.collision.line_count > 0 &&
                final_destination.ground_param.stage_param_count > 0 &&
                final_destination.yakumono_param != NULL &&
                final_destination.map_head.unkC > 4 &&
                final_destination.map_head.unk8[4].unk4 != NULL &&
                final_destination.map_head.unk8[4].unk8 != NULL &&
                final_destination.map_head.unk8[4].unk4[0] != NULL &&
                final_destination.map_head.unk8[4].unk8[0] != NULL &&
                final_destination.map_head.unk8[4].unk4[0][16].aobjdesc != NULL &&
                final_destination.map_head.unk8[4].unk4[0][16].child == NULL &&
                final_destination.map_head.unk8[4].unk8[0][16].child == NULL,
            "Final Destination stage dimensions\n");
    for (int m = 4; m <= 8; ++m) {
        struct UnkStageDat_x8_t* scene = &final_destination.map_head.unk8[m];
        require(scene->unk4 != NULL && scene->unk4[1] != NULL &&
                    scene->unk4[1][1].aobjdesc != NULL,
                "Final Destination background animation descriptors\n");
    }
    for (int m = 5; m <= 8; ++m) {
        struct UnkStageDat_x8_t* scene = &final_destination.map_head.unk8[m];
        int secondary_group = 4 + (m - 5) * 2;
        require(scene->unk4[secondary_group] != NULL &&
                    scene->unk8[secondary_group] != NULL &&
                    (scene->unk4[secondary_group][2].aobjdesc != NULL ||
                     scene->unk8[secondary_group][2].matanim != NULL),
                "Final Destination secondary background animation descriptors\n");
    }
    _aligned_free(bytes);

    const GrStageImportOptions castle_options = {
        0x144, NULL, 0, NULL, 0, false, castle_pointers,
        ARRAY_SIZE(castle_pointers),
    };
    const GrStageImportOptions kongo_options = {
        0xBC, NULL, 0, NULL, 0, false, kongo_pointers,
        ARRAY_SIZE(kongo_pointers),
    };
    const GrStageImportOptions corneria_options = {
        0x8C, corneria_animation_layouts,
        ARRAY_SIZE(corneria_animation_layouts), NULL, 0, false,
        corneria_pointers,
        ARRAY_SIZE(corneria_pointers),
    };
    const GrStageImportOptions venom_options = {
        0x3C, venom_animation_layouts, ARRAY_SIZE(venom_animation_layouts),
        NULL, 0, false, venom_pointers,
        ARRAY_SIZE(venom_pointers),
    };
    const GrStageImportOptions brinstar_options = {
        0x190, brinstar_animation_layouts,
        ARRAY_SIZE(brinstar_animation_layouts), NULL, 0, false,
        brinstar_pointers, ARRAY_SIZE(brinstar_pointers),
        0xA0, 30 * 4,
    };
    const GrStageImportOptions mute_city_options = {
        0x50, mute_city_animation_layouts,
        ARRAY_SIZE(mute_city_animation_layouts), NULL, 0, false,
        mute_city_pointers,
        ARRAY_SIZE(mute_city_pointers),
    };
    GrStageAssets castle;
    require_stage_import("/GrCs.dat", "Princess Peach's Castle", &castle_options,
                         arena, ARENA_SIZE, &castle);
    GrStageAssets kongo;
    require_stage_import("/GrKg.dat", "Kongo Jungle", &kongo_options, arena,
                         ARENA_SIZE, &kongo);
    require(kongo.itemdata != NULL && kongo.itemdata[0] != NULL &&
                kongo.itemdata[0]->unk0 == It_Kind_Klap &&
                kongo.itemdata[0]->unk4 != NULL,
            "Kongo Jungle Klaptrap article\n");
    itKlapAttributes* klap =
        kongo.itemdata[0]->unk4->x4_specialAttributes;
    require(klap != NULL && klap->common != NULL &&
                klap->common->damage_threshold == 10 &&
                klap->common->movement_speed == 0.5f,
            "Kongo Jungle Klaptrap attributes\n");
    require(DVDOpen("/GrCn.dat", &file), "open Corneria archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Corneria archive\n");
    GrStageAssets corneria;
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &corneria_options, &corneria))
    {
        fprintf(stderr, "Corneria import status=%d offset=%lx records=%lu\n",
                corneria.import.status, corneria.import.error_offset,
                corneria.import.record_count);
        require(false, "import Corneria stage data\n");
    }
    require(corneria.map_head.unkC > 10 &&
                corneria.map_head.unk8[1].unk4 != NULL &&
                corneria.map_head.unk8[1].unk4[0] != NULL &&
                corneria.map_head.unk8[2].unk4 != NULL &&
                corneria.map_head.unk8[2].unk4[0] != NULL &&
                corneria.map_head.unk8[2].unk4[0][8].aobjdesc != NULL &&
                corneria.map_head.unk8[5].unk4 != NULL &&
                corneria.map_head.unk8[5].unk4[0] != NULL &&
                corneria.map_head.unk8[10].unk4 != NULL &&
                corneria.map_head.unk8[10].unk4[0] != NULL &&
                corneria.itemdata != NULL && corneria.itemdata[0] != NULL &&
                corneria.itemdata[0]->unk4 != NULL &&
                corneria.itemdata[0]->unk0 == It_Kind_Arwing_Laser,
            "Corneria indexed animations and Arwing laser article\n");
    itArwingLaser_Attrs* arwing_laser =
        corneria.itemdata[0]->unk4->x4_specialAttributes;
    require(arwing_laser != NULL && arwing_laser->common_attrs != NULL &&
                arwing_laser->common_attrs->x4_throw_speed_mul > 0.0f &&
                arwing_laser->common_attrs->x4_throw_speed_mul < 1000.0f &&
                arwing_laser->aimed_speed_scale > 0.0f &&
                arwing_laser->beam_thickness_scale > 0.0f,
            "Corneria indexed animations and Arwing laser attributes\n");
    _aligned_free(bytes);
    require(DVDOpen("/GrVe.dat", &file), "open Venom archive\n");
    size = file.length;
    padded = (size + 31) & ~(size_t) 31;
    bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= size &&
                DVDClose(&file),
            "read Venom archive\n");
    GrStageAssets venom;
    if (!grStageAssets_Import(bytes, size, arena, ARENA_SIZE, 32768,
                              &venom_options, &venom))
    {
        fprintf(stderr, "Venom import status=%d offset=%lx records=%lu\n",
                venom.import.status, venom.import.error_offset,
                venom.import.record_count);
        require(false, "import Venom stage data\n");
    }
    require(venom.map_head.unkC > 6 &&
                venom.map_head.unk8[1].unk4 != NULL &&
                venom.map_head.unk8[1].unk4[0] != NULL &&
                venom.map_head.unk8[1].unk4[0][7].aobjdesc != NULL &&
                venom.map_head.unk8[1].unk8 != NULL &&
                venom.map_head.unk8[1].unk8[0] != NULL &&
                venom.map_head.unk8[1].unk8[0][7].matanim != NULL,
            "Venom indexed radio dialogue animation descriptors\n");
    u32 venom_sis_count;
    size_t venom_sis_used = venom.import.arena_used;
    void** venom_sis = grStageAssets_ImportSis(
        &venom, "SIS_GrCorneriaData", &venom_sis_count);
    require(venom_sis != NULL && venom_sis_count > 0 &&
                grStageAssets_ImportSis(&venom, "SIS_GrCorneriaData", NULL) ==
                    venom_sis && venom.import.arena_used == venom_sis_used,
            "Venom preloaded radio text reused without runtime allocation\n");
    require(venom.map_head.unk8[3].unk4 != NULL &&
                venom.map_head.unk8[3].unk4[0] != NULL &&
                venom.map_head.unk8[3].unk4[0][7].aobjdesc == NULL &&
                venom.map_head.unk8[3].unk8 != NULL &&
                venom.map_head.unk8[3].unk8[0] != NULL &&
                venom.map_head.unk8[3].unk8[0][8].matanim != NULL &&
                venom.map_head.unk8[6].unk4 != NULL &&
                venom.map_head.unk8[6].unk4[3] != NULL &&
                venom.map_head.unk8[6].unk4[3][2].aobjdesc == NULL &&
                venom.map_head.unk8[6].unk8 != NULL &&
                venom.map_head.unk8[6].unk8[3] != NULL &&
                venom.map_head.unk8[6].unk8[3][2].matanim != NULL,
            "Venom indexed Arwing animation descriptors\n");
    _aligned_free(bytes);
    GrStageAssets brinstar;
    require_stage_import("/GrZe.dat", "Brinstar", &brinstar_options, arena,
                         ARENA_SIZE, &brinstar);
    lbColl_80008D30_arg1* brinstar_acid_hit;
    memcpy(&brinstar_acid_hit, (u8*) brinstar.yakumono_param + 0x30,
           sizeof(brinstar_acid_hit));
    require(brinstar_acid_hit != NULL && brinstar_acid_hit->damage == 14 &&
                brinstar_acid_hit->kb_angle == 90,
            "Brinstar acid collision descriptor relocation\n");
    DVDFileInfo brinstar_file;
    require(DVDOpen("/GrZe.dat", &brinstar_file),
            "open Brinstar for missing-pointer check\n");
    size_t brinstar_size = brinstar_file.length;
    size_t brinstar_padded = (brinstar_size + 31) & ~(size_t) 31;
    void* brinstar_bytes = _aligned_malloc(brinstar_padded, 32);
    require(brinstar_bytes != NULL &&
                DVDReadPrio(&brinstar_file, brinstar_bytes, brinstar_padded,
                            0, 2) >= brinstar_size &&
                DVDClose(&brinstar_file),
            "read Brinstar for missing-pointer check\n");
    GrStageImportOptions missing_acid_pointer = brinstar_options;
    missing_acid_pointer.yakumono_pointers = NULL;
    missing_acid_pointer.yakumono_pointer_count = 0;
    GrStageAssets incomplete_brinstar;
    require(!grStageAssets_Import(brinstar_bytes, brinstar_size, arena,
                                  ARENA_SIZE, 32768, &missing_acid_pointer,
                                  &incomplete_brinstar) &&
                incomplete_brinstar.import.status == HSD_DAT_RELOCATION,
            "reject an unhandled Brinstar parameter relocation\n");
    _aligned_free(brinstar_bytes);
    GrStageAssets mute_city;
    require_stage_import("/GrMc.dat", "Mute City", &mute_city_options, arena,
                         ARENA_SIZE, &mute_city);
    _aligned_free(arena);
    free(common);
    _aligned_free(common_arena);
    _aligned_free(common_bytes);
    puts("Battlefield native stage archive imported");
    return 0;
}
