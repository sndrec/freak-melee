#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftkirby_copy_assets.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>

#include <stdio.h>
#include <stdlib.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static bool uses_merged_model(FighterKind kind)
{
    return kind == Ft_Kind_Donkey || kind == Ft_Kind_Purin ||
           kind == Ft_Kind_Mewtwo || kind == Ft_Kind_Falco ||
           kind == Ft_Kind_GameWatch;
}

int main(int argc, char** argv)
{
    char path[64];
    u32 archive_count = 0;
    u32 article_count = 0;
    u32 script_count = 0;
    u32 instruction_count = 0;
    require(argc == 2, "usage: native_kirby_copy_assets_test <iso>\n");
    require(aurora_dvd_open(argv[1]), "open ISO\n");

    for (FighterKind kind = Ft_Kind_Mario; kind < Ft_Kind_StockCount; ++kind) {
        const ftKirby_CopyName* names = &ftKb_Init_803CA9D0[kind];
        if (names->filename == NULL) {
            continue;
        }
        DVDFileInfo file;
        snprintf(path, sizeof(path), "/%s", names->filename);
        require(DVDOpen(path, &file), "open Kirby copy archive\n");
        size_t size = file.length;
        size_t padded = (size + 31) & ~(size_t) 31;
        void* bytes = _aligned_malloc(padded, 32);
        void* arena = _aligned_malloc(FT_KIRBY_COPY_IMPORT_ARENA_SIZE,
                                      sizeof(void*));
        require(bytes != NULL && arena != NULL &&
                    DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                    DVDClose(&file),
                "read Kirby copy archive\n");

        FtKirbyCopyAssets assets;
        if (!ftKirbyCopyAssets_Import(
                bytes, size, kind, names->name, arena,
                FT_KIRBY_COPY_IMPORT_ARENA_SIZE,
                FT_KIRBY_COPY_IMPORT_RECORD_CAPACITY, &assets))
        {
            fprintf(stderr,
                    "%s (%s): status=%d offset=%lx records=%lu external=%s\n",
                    names->filename, names->name, assets.import.status,
                    assets.import.error_offset, assets.import.record_count,
                    assets.import.external_symbol != NULL
                        ? assets.import.external_symbol
                        : "-");
            require(false, "import Kirby copy archive\n");
        }
        require(assets.data.parts.model_num > 0 &&
                    assets.data.parts.model_num <= 11 &&
                    assets.data.parts.vis_table != NULL &&
                    (uses_merged_model(kind)
                         ? assets.data.part_animation.xC != NULL
                         : assets.data.hat_joint != NULL) &&
                    assets.article_stats.script_count ==
                        assets.item_command_data.entry_count,
                "Kirby copy archive dimensions\n");
        if (kind == Ft_Kind_GameWatch) {
            FtPartsVisLookup* lookup = assets.data.resources[3].ptr;
            KirbyCopyColorData* colors = assets.data.resources[4].colors;
            require(lookup != NULL && lookup[0].x0 == 1 &&
                        lookup[0].x4 != NULL && lookup[0].x4[0].x0 == 7 &&
                        lookup[0].x4[0].x4 != NULL &&
                        lookup[1].x0 == 0 && lookup[1].x4 == NULL,
                    "Game & Watch copy visibility has a native body sentinel\n");
            for (u32 i = 0; i < 7; ++i) {
                require(lookup[0].x4[0].x4[i] == i + 3,
                        "Game & Watch copy visibility draw indices\n");
            }
            require(colors != NULL && colors->material_color == 0x3C23D70A &&
                        colors->item_color == 0x000000FF &&
                        colors->outline_color == 0xFFFFFF80,
                    "Game & Watch copy colors match the retail archive\n");
        }
        ++archive_count;
        article_count += assets.article_stats.article_count;
        script_count += assets.article_stats.script_count;
        instruction_count += assets.item_command_data.instruction_count;
        printf("%-12s records=%lu arena=%zu articles=%lu scripts=%lu\n",
               names->filename, assets.import.record_count,
               assets.import.arena_used, assets.article_stats.article_count,
               assets.article_stats.script_count);
        _aligned_free(arena);
        _aligned_free(bytes);
    }
    printf("TOTAL: %lu archives, %lu articles, %lu scripts, "
           "%lu instructions\n",
           archive_count, article_count, script_count, instruction_count);
    require(archive_count == 25, "all Kirby copy archives imported\n");
    require(article_count == 28 && script_count == 49 &&
                instruction_count == 113,
            "Kirby copy archive corpus totals\n");
    return 0;
}
