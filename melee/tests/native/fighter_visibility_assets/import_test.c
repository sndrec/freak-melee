// Actual full fighter/costume DAT import. No HSD objects or display are created.
#include <stdint.h>
#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/dobjlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
static void* read_archive(const char* filename, size_t* size)
{
    char path[128]; DVDFileInfo file;
    snprintf(path, sizeof(path), "/%s", filename);
    CHECK(DVDOpen(path, &file)); *size = file.length;
    size_t padded = (*size + 31) & ~(size_t)31;
    void* bytes = _aligned_malloc(padded, 32);
    CHECK(bytes != NULL);
    CHECK(DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32)*size);
    CHECK(DVDClose(&file)); return bytes;
}

static bool import_character(const void* bytes, size_t size, void* arena,
                              FtCharacterAssets* assets)
{
    return ftCharacterAssets_Import(
        bytes, size, &ftCharacterStockLayouts[Ft_Kind_GameWatch],
        ftData_803C1F40[Ft_Kind_GameWatch].root_name,
        ftData_Table_Unk0[Ft_Kind_GameWatch].count,
        ftData_UnkIntPairs[Ft_Kind_GameWatch].count,
        CostumeListsForeachCharacter[Ft_Kind_GameWatch].numCostumes,
        arena, FT_CHARACTER_IMPORT_ARENA_SIZE,
        FT_CHARACTER_IMPORT_RECORD_CAPACITY, assets);
}

int main(int argc, char** argv)
{
    CHECK(argc == 2); CHECK(aurora_dvd_open(argv[1]));
    const FtCharacterAssetLayout* layout = &ftCharacterStockLayouts[Ft_Kind_GameWatch];
    CHECK(layout->article_mask == 0x3ff && layout->visibility_mask == 0x400);
    size_t size;
    unsigned char* bytes = read_archive(ftData_803C1F40[Ft_Kind_GameWatch].filename, &size);
    void* arena = _aligned_malloc(FT_CHARACTER_IMPORT_ARENA_SIZE, sizeof(void*));
    CHECK(arena != NULL);
    FtCharacterAssets* assets = malloc(sizeof(*assets)); CHECK(assets != NULL);
    CHECK(import_character(bytes, size, arena, assets));
    CHECK(assets->visibility_assets[10] != NULL);
    CHECK(assets->visibility_assets[10] == assets->fighter_items[10]);
    CHECK(assets->model_data.x0.model_num == 11);
    for (unsigned slot = 0; slot < FT_CHARACTER_ITEM_CAPACITY; ++slot)
        CHECK((assets->visibility_assets[slot] != NULL) == (slot == 10));
    const unsigned costumes = CostumeListsForeachCharacter[Ft_Kind_GameWatch].numCostumes;
    CHECK(costumes == 4);
    for (unsigned costume = 0; costume < costumes; ++costume) {
        const Fighter_CostumeStrings* names = &ftData_803C2360[Ft_Kind_GameWatch][costume];
        size_t costume_size;
        void* costume_bytes = read_archive(names->dat_filename, &costume_size);
        void* costume_arena = _aligned_malloc(FT_COSTUME_IMPORT_ARENA_SIZE, sizeof(void*));
        CHECK(costume_arena != NULL);
        FtCostumeAssets graph;
        CHECK(ftCostumeAssets_Import(costume_bytes, costume_size, names->joint_name,
                                     names->matanim_joint_name, costume_arena,
                                     FT_COSTUME_IMPORT_ARENA_SIZE,
                                     FT_COSTUME_IMPORT_RECORD_CAPACITY, &graph));
        CHECK(ftCharacterAssets_ValidateVisibility(assets, 0x400, graph.joint));
        CHECK(!ftCharacterAssets_ValidateVisibility(assets, 1, graph.joint));
        // Change a real imported ordinal, not the graph pointer. Every actual
        // costume must reject a reference beyond its DObj descriptor list.
        bool exercised = false;
        for (unsigned group = 0; group < 11 && !exercised; ++group) {
            FtPartsVisLookup* lookup = &assets->visibility_assets[10][group];
            for (int variant = 0; variant < lookup->x0 && !exercised; ++variant) {
                TempS* selection = &lookup->x4[variant];
                if (selection->x0 == 0) continue;
                unsigned char old = selection->x4[0]; selection->x4[0] = 255;
                CHECK(!ftCharacterAssets_ValidateVisibility(assets, 0x400, graph.joint));
                selection->x4[0] = old;
                CHECK(ftCharacterAssets_ValidateVisibility(assets, 0x400, graph.joint));
                exercised = true;
            }
        }
        CHECK(exercised);
        _aligned_free(costume_arena); _aligned_free(costume_bytes);
    }
    free(assets); _aligned_free(arena); _aligned_free(bytes);
    printf("fighter visibility full DAT import: %u checks / %u costumes\n", checks, costumes);
    return 0;
}
