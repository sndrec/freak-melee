#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <dolphin/os/OSAlloc.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lbfile.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/native_animation_pool.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static void* read_archive(const char* basename, size_t* size)
{
    DVDFileInfo file;
    char path[64];
    snprintf(path, sizeof(path), "/%s", lbFileGetFullName(basename));
    require(DVDOpen(path, &file), "open fighter archive\n");
    *size = file.length;
    size_t padded = (*size + 31) & ~(size_t) 31;
    void* bytes = _aligned_malloc(padded, 32);
    require(bytes != NULL &&
                DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) *size &&
                DVDClose(&file),
            "read fighter archive\n");
    return bytes;
}

typedef struct AnimationSample {
    bool finite;
} AnimationSample;

static void check_animation_sample(void* context, enum_t type,
                                   HSD_ObjData* value)
{
    AnimationSample* sample = context;
    if (type >= HSD_A_J_ROTX && type <= HSD_A_J_SCAZ &&
        !isfinite(value->fv))
    {
        sample->finite = false;
    }
}

static void check_fighter_animation_tracks(FighterKind kind,
    const struct Fighter_WaitAnimData* table, u32 count)
{
    for (u32 clip = 0; clip < count; ++clip) {
        const FigaTree* tree = table[clip].x14;
        if (tree == NULL) {
            continue;
        }
        u32 track_count = 0;
        for (const s8* node = tree->nodes; *node != -1; ++node) {
            track_count += (u8) *node;
        }
        for (u32 track_index = 0; track_index < track_count; ++track_index) {
            const FigaTrack* track = &tree->tracks[track_index];
            if (track->obj_type < HSD_A_J_ROTX ||
                track->obj_type > HSD_A_J_SCAZ)
            {
                continue;
            }
            HSD_FObj fobj = { 0 };
            fobj.startframe = track->startframe;
            fobj.obj_type = track->obj_type;
            fobj.frac_value = track->frac_value;
            fobj.frac_slope = track->frac_slope;
            fobj.ad_head = track->ad_head;
            fobj.length = track->length;
            HSD_FObjReqAnimAll(&fobj, 0.0F);
            AnimationSample sample = { true };
            float last_frame = 0.0F;
            // Gameplay can evaluate between whole animation frames during
            // speed changes and blends, so exercise the interpolation paths.
            u32 quarter_frames = (u32) (tree->frames * 4.0F);
            for (u32 step = 0; step <= quarter_frames + 1; ++step) {
                float frame = step > quarter_frames ? tree->frames :
                                                       (float) step * 0.25F;
                HSD_FObjInterpretAnim(&fobj, &sample,
                                      check_animation_sample,
                                      frame - last_frame);
                if (fobj.ad < fobj.ad_head ||
                    fobj.ad > fobj.ad_head + fobj.length)
                {
                    fprintf(stderr,
                            "fighter animation stream overrun: kind=%d clip=%lu track=%lu frame=%f consumed=%td length=%lu\n",
                            kind, clip, track_index, frame,
                            fobj.ad - fobj.ad_head, fobj.length);
                    require(false, "fighter animation consumed beyond its stream\n");
                }
                if (!sample.finite) {
                    fprintf(stderr,
                            "nonfinite fighter animation: kind=%d clip=%lu track=%lu type=%u frame=%f\n",
                            kind, clip, track_index, track->obj_type, frame);
                    require(false, "fighter animation sample is nonfinite\n");
                }
                last_frame = frame;
            }
        }
    }
}

int main(int argc, char** argv)
{
    s8 rig_nodes[] = {0, 0, -1};
    FigaTree rig_tree = { .nodes = rig_nodes };
    struct Fighter_WaitAnimData rig_clip = { .x14 = &rig_tree };
    require(ftAnimationBank_FitsRig(&rig_clip, 3, 1) &&
            !ftAnimationBank_FitsRig(&rig_clip, 1, 0),
            "Clip must fit its available bones\n");
    rig_clip.x10_animCurrFlags = 1u << 11;
    require(!ftAnimationBank_FitsRig(&rig_clip, 3, 1) &&
            ftAnimationBank_FitsRig(&rig_clip, 2, 1),
            "Enabled insertion must consume a clip node\n");
    require(argc == 2, "usage: native_fighter_animation_bank_test <iso>\n");
    require(aurora_dvd_open(argv[1]), "open ISO\n");
    void* character_arena = _aligned_malloc(
        FT_CHARACTER_IMPORT_ARENA_SIZE, sizeof(void*));
    void* animation_arena = _aligned_malloc(
        FT_ANIMATION_IMPORT_ARENA_SIZE, sizeof(void*));
    require(character_arena != NULL && animation_arena != NULL,
            "allocate fighter import arenas\n");
    u32 total_clips = 0;
    u32 max_tracks = 0;
    size_t max_arena = 0;
    for (FighterKind kind = Ft_Kind_Mario; kind < Ft_Kind_StockCount; ++kind) {
        const FtDataArchiveName* archive = &ftData_803C1F40[kind];
        size_t character_size;
        void* character_bytes = read_archive(archive->filename,
                                              &character_size);
        FtCharacterAssets character;
        require(ftCharacterAssets_Import(
                    character_bytes, character_size, &ftCharacterStockLayouts[kind],
                    archive->root_name,
                    (u32) ftData_Table_Unk0[kind].count,
                    (u32) ftData_UnkIntPairs[kind].count,
                    CostumeListsForeachCharacter[kind].numCostumes,
                    character_arena, FT_CHARACTER_IMPORT_ARENA_SIZE,
                    FT_CHARACTER_IMPORT_RECORD_CAPACITY, &character),
                "import fighter character data\n");
        s32 expected_relinked_offset = -1;
        u32 relinked_index = 0;
        if (kind == Ft_Kind_Mario) {
            for (u32 i = 0; i < (u32) ftData_Table_Unk0[kind].count; ++i) {
                struct Fighter_WaitAnimData* entry = &character.data.xC[i];
                if (entry->x0 != NULL && entry->x4 >= 24 && entry->x8 > 0) {
                    expected_relinked_offset = entry->x4;
                    relinked_index = i;
                    entry->x4 -= 24;
                    break;
                }
            }
            require(expected_relinked_offset >= 0,
                    "find a real Mario animation to relink\n");
        }
        size_t animation_size;
        void* animation_bytes = read_archive(ftData_803C23E4[kind],
                                              &animation_size);
        FtAnimationBankStats stats;
        if (!ftAnimationBank_Import(animation_bytes, animation_size,
                (const FtAnimationBankTable[]){{ character.data.xC, (u32) ftData_Table_Unk0[kind].count, false }}, 1,
                animation_arena, FT_ANIMATION_IMPORT_ARENA_SIZE, &stats))
        {
            fprintf(stderr,
                    "%s animation import failed: index=%lu status=%d "
                    "offset=%ld size=%ld name=%s clips=%lu\n",
                    ftData_803C23E4[kind], stats.failed_index, stats.status,
                    character.data.xC[stats.failed_index].x4,
                    character.data.xC[stats.failed_index].x8,
                    character.data.xC[stats.failed_index].x0 != NULL
                        ? character.data.xC[stats.failed_index].x0 : "none",
                    stats.clip_count);
            return 1;
        }
        if (kind == Ft_Kind_Mario) {
            require(character.data.xC[relinked_index].x4 ==
                        expected_relinked_offset &&
                        character.data.xC[relinked_index].x14 != NULL,
                    "relink a stale animation offset by DAT symbol\n");
        }
        check_fighter_animation_tracks(kind, character.data.xC,
                                       (u32) ftData_Table_Unk0[kind].count);
        total_clips += stats.clip_count;
        if (kind == Ft_Kind_Falco) {
            Fighter flags = { 0 };
            flags.x594_s32 = character.data.xC[10].x10_animCurrFlags;
            require(!flags.x594_b1_loop &&
                        flags.x597_bits == Ft_Kind_Falco &&
                        flags.x594_bits == 0 && flags.x596_bone == 3,
                    "Falco Turn finite animation and bone index\n");
            flags.x594_s32 = character.data.xC[7].x10_animCurrFlags;
            require(flags.x594_b1_loop &&
                        flags.x597_bits == Ft_Kind_Falco,
                    "Falco walking animation flags\n");
        }
        if (stats.max_tracks > max_tracks) {
            max_tracks = stats.max_tracks;
        }
        if (stats.arena_used > max_arena) {
            max_arena = stats.arena_used;
        }
        _aligned_free(animation_bytes);
        _aligned_free(character_bytes);
    }
    _aligned_free(animation_arena);
    _aligned_free(character_arena);
    aurora_dvd_close();
    require(total_clips == 8011, "all retail fighter clips covered\n");
    static u8 hsd_arena[64 * 1024] ATTRIBUTE_ALIGN(64);
    void* heap_start = OSInitAlloc(hsd_arena,
                                   hsd_arena + sizeof(hsd_arena), 1);
    OSHeapHandle heap = OSCreateHeap(heap_start,
                                     hsd_arena + sizeof(hsd_arena));
    require(heap >= 0, "initialize animation object heap\n");
    HSD_SetHeap(heap);
    OSSetCurrentHeap(heap);
    require(nativeAnimationPool_BeginHeap(), "initialize native animation ownership\n");
    FigaTrack mixed[] = {
        { .obj_type = 5 },
        { .startframe = 17, .obj_type = 1 },
        { .obj_type = 6 },
    };
    HSD_FObj* selected = fn_8001E60C(mixed, 3);
    require(selected != NULL && selected->startframe == 17 &&
                selected->obj_type == 1 && selected->next == NULL,
            "filtered fighter tracks use the correct source\n");
    HSD_FObjRemoveAll(selected);
    require(fn_8001E60C(mixed, 1) == NULL &&
                nativeFObjPool.used == 0,
            "node with only filtered tracks stays empty\n");
    printf("clips=%lu max_tracks=%lu max_arena=%zu\n",
           total_clips, max_tracks, max_arena);
    return 0;
}
