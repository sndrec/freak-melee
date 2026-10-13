#include "grdatfiles.h"

#include "ground.h"
#include "grstage_data.h"
#include "types.h"
#include <melee/it/it_3F14.h>
#include <string.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbarchive.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/archive.h>
#include <sysdolphin/baselib/dat.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/particle.h>
#include <sysdolphin/baselib/psstructs.h>

#ifdef MELEE_NATIVE
typedef struct NativeStageArchiveSpec {
    const char* filename;
    GrStageImportOptions options;
} NativeStageArchiveSpec;

// grLast.c addresses joint 16 in the first animation group of map 4,
// joint 1 in each group's primary animation, and joint 2 in the
// secondary animations of maps 5-8. DAT has no array length here.
static const GrStageAnimationLayout fd_animation_layouts[] = {
    { 4, 17, 2 }, { 5, 3, 3 }, { 6, 3, 3 },
    { 7, 3, 3 },  { 8, 3, 3 },
};
// grKraid indexes joint 18 in animation groups 0 and 1, and joint 13
// in group 2 of scene 3. Those descriptors are contiguous in the DAT.
static const GrStageAnimationLayout brinstar_depths_animation_layouts[] = {
    { 3, 19, 19 },
};
// grInishie2 addresses joint 7 in each of three animation groups of map 2.
static const GrStageAnimationLayout mushroom_kingdom_ii_animation_layouts[] = {
    { 2, 8, 8 },
};
// grYorster addresses joint 26 in map 1 animation group 3.
static const GrStageAnimationLayout yoshis_island_animation_layouts[] = {
    { 1, 27, 27 },
};
// grOldYoshi indexes joints 1, 5, and 9 in each of map 2's three animation
// groups. All three groups contain thirteen contiguous joint descriptors.
static const GrStageAnimationLayout old_yoshi_animation_layouts[] = {
    { 2, 13, 13 },
};
// grOldKongo applies barrel animation states to joint 2 in all three
// animation groups of map 1. The descriptors are contiguous in the DAT.
static const GrStageAnimationLayout old_kongo_animation_layouts[] = {
    { 1, 3, 3 },
};
// Venom's Arwing controllers index joints 7-8 in map 3 and joints 0-2 in map
// 6. Map 5 startup applies several animation states to joints through 15;
// the radio controller on map 8 creates the dialogue overlay on map 1.
static const GrStageAnimationLayout venom_animation_layouts[] = {
    { 1, 10, 10 },
    { 3, 9, 9 },
    { 5, 16, 16 },
    { 6, 16, 16 },
};
// grZebes directly indexes animation descriptor 28 on map 6 and descriptor 1
// on its map-7 lower platform. The source descriptors are contiguous in every
// animation group, but a graph import alone does not preserve array indexing.
static const GrStageAnimationLayout brinstar_animation_layouts[] = {
    { 1, 4, 4 },
    { 6, 29, 29 },
    { 7, 2, 2 },
};
// Mute City's map 29 applies animation group 1 to joint 6 when the moving
// road object is created.
static const GrStageAnimationLayout mute_city_animation_layouts[] = {
    { 29, 7, 7 },
};
// Corneria stores joint-indexed animation descriptors contiguously for its
// Arwing, background Arwing, smash-taunt, and carrier scenes. The generic
// graph importer preserves their hierarchy but not the source arrays needed
// by grAnime's direct joint indexing.
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
static const u16 pokemon_stadium_halfword_offsets[] = {
    0x48, 0x4A, 0x4C, 0x4E, 0x50,
};
static const GrStageImportOptions pokemon_stadium_fragment_options = {
    .yakumono_size = 0x54,
    .stage_fragment = true,
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
static const GrStageParamPointer brinstar_pointers[] = {
    { 0x2C, GrStageParamPointer_EnvironmentHit },
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

static const NativeStageArchiveSpec native_stage_archives[] = {
    { "/GrNBa.dat", { 8, NULL, 0, NULL, 0, false,
                       battlefield_pointers,
                       ARRAY_SIZE(battlefield_pointers) } },
    { "/GrNLa.dat", { 16, fd_animation_layouts,
                       ARRAY_SIZE(fd_animation_layouts), NULL, 0, false,
                       final_destination_pointers,
                       ARRAY_SIZE(final_destination_pointers) } },
    // grOldPupupu's yakumono parameter has four shorts, two ints,
    // and nine floats (0x34 bytes).
    { "/GrOp.dat", { 0x34, NULL, 0, dream_land_halfword_offsets,
                      ARRAY_SIZE(dream_land_halfword_offsets) } },
    // grGreens uses parameters through offset 0x78.
    { "/GrGr.dat", { 0x7C, NULL, 0 } },
    // grGarden's eight four-byte fields occupy 0x20 bytes.
    { "/GrGd.dat", { 0x20, NULL, 0 } },
    // grStory has three floats and six Shy Guy heights.
    { "/GrSt.dat", { 0x24, NULL, 0 } },
    // grFlatzone has sixteen four-byte parameters.
    { "/GrFz.dat", { 0x40, NULL, 0 } },
    { "/GrOy.dat", { 0x1C, old_yoshi_animation_layouts,
                      ARRAY_SIZE(old_yoshi_animation_layouts),
                      old_yoshi_halfword_offsets,
                      ARRAY_SIZE(old_yoshi_halfword_offsets) } },
    { "/GrOk.dat", { 0x70, old_kongo_animation_layouts,
                      ARRAY_SIZE(old_kongo_animation_layouts),
                      old_kongo_halfword_offsets,
                      ARRAY_SIZE(old_kongo_halfword_offsets), false,
                      old_kongo_pointers,
                      ARRAY_SIZE(old_kongo_pointers) } },
    // grIzumi's parameter object ends at offset 0x54 in GrIz.dat.
    { "/GrIz.dat", { 0x54, NULL, 0 } },
    { "/GrRc.dat", { 0x48, NULL, 0 } },
    { "/GrGb.dat", { 0xA4, NULL, 0, great_bay_halfword_offsets,
                      ARRAY_SIZE(great_bay_halfword_offsets) } },
    { "/GrSh.dat", { 4, NULL, 0 } },
    // grOnett's yakumono parameter is 26 contiguous floats.
    { "/GrOt", { 0x68, NULL, 0 } },
    { "/GrPs", { 0x54, NULL, 0, pokemon_stadium_halfword_offsets,
                  ARRAY_SIZE(pokemon_stadium_halfword_offsets) } },
    { "/GrKr.dat", { 0x34, brinstar_depths_animation_layouts,
                      ARRAY_SIZE(brinstar_depths_animation_layouts) } },
    { "/GrFs.dat", { 0x4C, NULL, 0, fourside_halfword_offsets,
                      ARRAY_SIZE(fourside_halfword_offsets) } },
    { "/GrI1.dat", { 0x54, NULL, 0, mushroom_kingdom_halfword_offsets,
                      ARRAY_SIZE(mushroom_kingdom_halfword_offsets) } },
    { "/GrI2.dat", { 0x4C, mushroom_kingdom_ii_animation_layouts,
                      ARRAY_SIZE(mushroom_kingdom_ii_animation_layouts),
                      mushroom_kingdom_ii_halfword_offsets,
                      ARRAY_SIZE(mushroom_kingdom_ii_halfword_offsets) } },
    { "/GrYt.dat", { 0x20, yoshis_island_animation_layouts,
                      ARRAY_SIZE(yoshis_island_animation_layouts) } },
    { "/GrPu.dat", { 4, NULL, 0 } },
    { "/GrBb.dat", { 0x144, NULL, 0 } },
    { "/GrIm.dat", { 0x13C, NULL, 0, NULL, 0, true } },
    { "/GrCs.dat", { 0x144, NULL, 0, NULL, 0, false,
                      castle_pointers,
                      ARRAY_SIZE(castle_pointers) } },
    { "/GrKg.dat", { 0xBC, NULL, 0, NULL, 0, false,
                      kongo_pointers,
                      ARRAY_SIZE(kongo_pointers) } },
    { "/GrCn", { 0x8C, corneria_animation_layouts,
                  ARRAY_SIZE(corneria_animation_layouts), NULL, 0, false,
                  corneria_pointers,
                  ARRAY_SIZE(corneria_pointers) } },
    { "/GrVe", { 0x3C, venom_animation_layouts,
                  ARRAY_SIZE(venom_animation_layouts), NULL, 0, false,
                  venom_pointers,
                  ARRAY_SIZE(venom_pointers) } },
    { "/GrZe.dat", { 0x190, brinstar_animation_layouts,
                       ARRAY_SIZE(brinstar_animation_layouts), NULL, 0, false,
                       brinstar_pointers, ARRAY_SIZE(brinstar_pointers),
                       0xA0, 30 * 4 } },
    { "/GrMc.dat", { 0x50, mute_city_animation_layouts,
                      ARRAY_SIZE(mute_city_animation_layouts), NULL, 0, false,
                      mute_city_pointers, ARRAY_SIZE(mute_city_pointers) } },
};

static GrStageAssets* grDatFiles_ImportNativeAssets(
    const void* bytes, size_t length, const GrStageImportOptions* options,
    void* owned_bytes, const char* label)
{
    GrStageAssets* assets = lbHeap_80015BD0(0, sizeof(*assets));
    enum { STAGE_ARENA_SIZE = 4 * 1024 * 1024 };
    void* arena = lbHeap_80015BD0(0, STAGE_ARENA_SIZE);
    HSD_ASSERT(0, assets != NULL && arena != NULL);
    GrStageImportOptions prepared=*options;
    prepared.stage_item_script_article=it_804D6D38
        ? it_804D6D38[It_PKind_Random-It_Kind_Kuriboh] : NULL;
    if (!grStageAssets_Import(bytes, length, arena, STAGE_ARENA_SIZE, 32768,
                              &prepared, assets))
    {
        OSReport("Native stage import failed: %s status=%d offset=%x\n",
                 label, assets->import.status, assets->import.error_offset);
        HSD_ASSERT(0, 0);
    }
    assets->arena = arena;
    assets->owned_bytes = owned_bytes;
    return assets;
}

static void grDatFiles_RegisterNativeAssets(UnkArchiveStruct* archive,
                                            GrStageAssets* assets)
{
    archive->unk0 = NULL;
    archive->unk4 = &assets->map_head;
    archive->unk8 = 0;
    archive->native_assets = assets;
}
#endif

/* 1C6228 */ static void grDatFiles_801C6228(UnkStageDat*);
/* 1C62B4 */ static UnkArchiveStruct* grDatFiles_801C62B4(void);

/// @todo Merge declaration and definition
/* static */ extern GroundParam grDatFiles_803E0848;

/// @todo Merge declaration and definition
/* static */ extern UnkStageDat grDatFiles_803E0924;

void grDatFiles_801C5FC0(HSD_Archive* archive, void* data, size_t length)
{
#ifdef MELEE_NATIVE
    HSD_Dat dat;
    (void) archive;
    HSD_ASSERTREPORT(0, HSD_DatOpen(&dat, data, length) == HSD_DAT_OK,
                     "Invalid native stage DAT");
#else
    HSD_Archive* map_ptcl;
    HSD_Archive* map_texg;
    lbArchive_InitializeDAT(archive, data, length);
    map_ptcl = HSD_ArchiveGetPublicAddress(archive, "map_ptcl");
    map_texg = HSD_ArchiveGetPublicAddress(archive, "map_texg");

    if (map_ptcl != NULL && map_texg != NULL) {
        psInitDataBankLocate(map_ptcl, map_texg, NULL);
    }
#endif
}

void grDatFiles_801C6038(void* arg0, s32 arg1, s32 arg2)
{
    UnkArchiveStruct* temp_r3 = grDatFiles_801C62B4();
    if (arg0 != NULL) {
#ifdef MELEE_NATIVE
        const char* filename = arg0;
        void* bytes = NULL;
        size_t length = 0;
        bool preloaded = arg2 != 0 &&
            lbDvd_GetPreloadedFile(filename, &bytes, &length);
        const GrStageImportOptions* options = NULL;
        for (u32 i = 0; i < ARRAY_SIZE(native_stage_archives); ++i) {
            if (strcmp(filename, native_stage_archives[i].filename) == 0) {
                options = &native_stage_archives[i].options;
                break;
            }
        }
        HSD_ASSERTREPORT(0, options != NULL,
                         "Native stage import does not support %s", filename);
        if (!preloaded) {
            length = lbFileGetSize(filename);
            bytes = lbHeap_80015BD0(0, OSRoundUp32B(length));
            HSD_ASSERT(0, bytes != NULL);
            lbFile_8001668C(filename, bytes, &length);
        }
        GrStageAssets* assets = grDatFiles_ImportNativeAssets(
            bytes, length, options, preloaded ? NULL : bytes, filename);
        grDatFiles_RegisterNativeAssets(temp_r3, assets);
        if (arg1 == 0) {
            stage_info.coll_data = &assets->collision;
            stage_info.param = &assets->ground_param;
            stage_info.itemdata = assets->itemdata;
            stage_info.ald_yaku_all = assets->ald_yaku_all;
            stage_info.map_ptcl = assets->particle_bank.command_count != 0
                ? &assets->particle_bank : NULL;
            stage_info.map_texg = stage_info.map_ptcl;
            stage_info.yakumono_param = assets->yakumono_param;
            stage_info.map_plit = assets->map_plit;
            stage_info.x6C8 = NULL;
            stage_info.quake_model_set = assets->quake_model_set;
        }
        if (stage_info.map_ptcl != NULL) {
            psInitDataBankNative(0x40, &assets->particle_bank);
        }
        grDatFiles_801C6228(temp_r3->unk4);
#else
        HSD_Archive* sp14;
        s32 phi_r28;
        void* r4 = arg0;
        if (arg2 != 0) {
            phi_r28 =
                lbArchive_800171CC(&sp14, r4, &temp_r3->unk4, "map_head", 0);
        } else {
            sp14 =
                lbArchive_80016DBC(r4, (void**) &temp_r3->unk4, "map_head", 0);
            phi_r28 = 0;
        }
        temp_r3->unk8 = 0;
        if (arg1 == 0) {
            stage_info.coll_data =
                HSD_ArchiveGetPublicAddress(sp14, "coll_data");
            stage_info.param =
                HSD_ArchiveGetPublicAddress(sp14, "grGroundParam");
            stage_info.itemdata =
                HSD_ArchiveGetPublicAddress(sp14, "itemdata");
            stage_info.ald_yaku_all =
                HSD_ArchiveGetPublicAddress(sp14, "ALDYakuAll");
            stage_info.map_ptcl =
                HSD_ArchiveGetPublicAddress(sp14, "map_ptcl");
            stage_info.map_texg =
                HSD_ArchiveGetPublicAddress(sp14, "map_texg");
            stage_info.yakumono_param =
                HSD_ArchiveGetPublicAddress(sp14, "yakumono_param");
            stage_info.map_plit =
                HSD_ArchiveGetPublicAddress(sp14, "map_plit");
            stage_info.quake_model_set =
                HSD_ArchiveGetPublicAddress(sp14, "quake_model_set");
        }
        temp_r3->unk0 = sp14;
        if (stage_info.map_ptcl != NULL && stage_info.map_texg != NULL) {
            if (phi_r28 != 0) {
                psInitDataBankLoad(0x40, stage_info.map_ptcl,
                                   stage_info.map_texg, 0, 0);
            } else {
                psInitDataBank(0x40, stage_info.map_ptcl, stage_info.map_texg,
                               0, 0);
            }
        }
        grDatFiles_801C6228(temp_r3->unk4);
#endif
    } else {
        temp_r3->unk4 = &grDatFiles_803E0924;
        if (arg1 == 0) {
            stage_info.coll_data = NULL;
            stage_info.param = &grDatFiles_803E0848;
            stage_info.itemdata = NULL;
            stage_info.ald_yaku_all = NULL;
            stage_info.map_ptcl = NULL;
            stage_info.map_texg = NULL;
            stage_info.yakumono_param = NULL;
            stage_info.map_plit = NULL;
            stage_info.x6C8 = NULL;
        }
        temp_r3->unk0 = (void*) -1;
    }
}

void grDatFiles_801C6228(UnkStageDat* arg0)
{
    if (arg0 != NULL && arg0->unk28 != NULL && arg0->unk2C != 0) {
        s32 i;
        for (i = 0; i < arg0->unk2C; i++) {
            UnkStageDatInternal* temp_r4 = arg0->unk28[i];
            if (temp_r4 != NULL) {
                temp_r4->unk4 |= 0x4000000;
            }
        }
    }
}

static UnkArchiveStruct grDatFiles_8049EE10[4];

void grDatFiles_801C6288(void)
{
    memzero(&grDatFiles_8049EE10, sizeof(grDatFiles_8049EE10));
}

UnkArchiveStruct* grDatFiles_801C62B4(void)
{
    s32 i;
    for (i = 0; i < 4; i++) {
#ifdef MELEE_NATIVE
        if (grDatFiles_8049EE10[i].unk4 == NULL) {
#else
        if (grDatFiles_8049EE10[i].unk0 == NULL) {
#endif
            return &grDatFiles_8049EE10[i];
        }
    }
    HSD_ASSERT(229, 0);
}

UnkArchiveStruct* grDatFiles_GetArchive(void)
{
    return grDatFiles_8049EE10;
}

UnkArchiveStruct* grDatFiles_801C6330(s32 arg0)
{
    if (arg0 >= 0) {
        s32 i;
        for (i = 0; i < 4; i++) {
#ifdef MELEE_NATIVE
            if (grDatFiles_8049EE10[i].unk4 != NULL) {
#else
            if (grDatFiles_8049EE10[i].unk0 != NULL) {
#endif
                UnkStageDat* temp_r7 = grDatFiles_8049EE10[i].unk4;
                if (temp_r7 != NULL && temp_r7->unkC > arg0 &&
                    temp_r7->unk8[arg0].unk0 != 0)
                {
                    return &grDatFiles_8049EE10[i];
                }
            }
        }
    }
    return NULL;
}

UnkArchiveStruct* grDatFiles_801C6478(void* data, s32 length)
{
    UnkArchiveStruct* arc;

#ifdef MELEE_NATIVE
    GrStageAssets* assets = grDatFiles_ImportNativeAssets(
        data, length, &pokemon_stadium_fragment_options, NULL,
        "Pokemon Stadium transformation");
    arc = grDatFiles_801C62B4();
    HSD_ASSERT(290, arc);
    grDatFiles_RegisterNativeAssets(arc, assets);
#else
    HSD_Archive* archive = lbHeap_80015BD0(0, sizeof(HSD_Archive));
    lbArchive_InitializeDAT(archive, data, length);
    arc = grDatFiles_801C62B4();
    HSD_ASSERT(290, arc);
    arc->unk0 = archive;
    arc->unk4 = HSD_ArchiveGetPublicAddress(archive, "map_head");
    arc->unk8 = 1;

#endif
    grDatFiles_801C6228(arc->unk4);

    return arc;
}

static StageParam grDatFiles_803E07E4 = {
    0, -1, -1, 0, 0, 0, 0, 0, { 0 },
};

GroundParam grDatFiles_803E0848 = {
    1,  0x80, { 0 }, 0x1E, 0,  1,     0x8000, 10,
    0,  0,    1,     1,    1,  { 0 }, 40,     10,
    50, 100,  10,    10,   10, 10,    false,  0,
    0,  0,    30,    10,   0,  0,     { 0 },  &grDatFiles_803E07E4,
    1,
};

UnkStageDat grDatFiles_803E0924 = { 0 };
