#include "itcommon_data.h"

#include <stddef.h>
#include <string.h>

#include <melee/lb/lbcolanim_data.h>

static bool resolve_local(const HSD_Dat* dat, u32 slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(dat, slot, target, &external) == HSD_DAT_REF_LOCAL;
}

static bool import_words(const HSD_Dat* dat, u32 source, void* destination,
                         size_t size)
{
    u8* output = destination;
    u32 value;
    if ((size & 3) != 0 || HSD_DatBytes(dat, source, size) == NULL) {
        return false;
    }
    for (size_t offset = 0; offset < size; offset += sizeof(value)) {
        if (!HSD_DatReadU32(dat, source + (u32) offset, &value)) {
            return false;
        }
        memcpy(output + offset, &value, sizeof(value));
    }
    return true;
}

bool itCommonAssets_Import(const void* bytes, size_t size, void* arena,
                           size_t arena_size, u32 record_capacity,
                           ItCommonAssets* output)
{
    u32 common_offset;
    u32 common_table;
    u32 character_table;
    u32 pokemon_table;
    u32 root;
    u32 zako_offset;
    u32 color_table;
    ItScriptEntry script_entries[256];
    ItScriptCollect scripts = { script_entries, 0, ARRAY_SIZE(script_entries) };

    if (output == NULL) {
        return false;
    }
    memset(output, 0, sizeof(*output));
    if (HSD_DatOpen(&output->dat, bytes, size) != HSD_DAT_OK ||
        HSD_DatImportInit(&output->import, &output->dat, arena, arena_size,
                          record_capacity) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&output->dat, "itPublicData", &root) ||
        !resolve_local(&output->dat, root, &common_offset) ||
        !resolve_local(&output->dat, root + 4, &common_table) ||
        !resolve_local(&output->dat, root + 8, &character_table) ||
        !resolve_local(&output->dat, root + 12, &pokemon_table) ||
        !resolve_local(&output->dat, root + 16, &zako_offset) ||
        !resolve_local(&output->dat, root + 20, &color_table) ||
        HSD_DatNextTarget(&output->dat, color_table) - color_table !=
            IT_COMMON_COLOR_ENTRY_COUNT * 8 ||
        !import_words(&output->dat, common_offset, &output->common_data,
                      sizeof(output->common_data)) ||
        !import_words(&output->dat, zako_offset, &output->zako_data,
                      sizeof(output->zako_data)) ||
        !itArticle_ImportTable(&output->import, common_table,
                               output->common_articles,
                               IT_COMMON_ARTICLE_COUNT,
                               &output->article_stats, &scripts, -1,
                               It_Common_Start) ||
        !itArticle_ImportTable(&output->import, character_table,
                               output->character_articles,
                               IT_CHARACTER_ARTICLE_COUNT,
                               &output->article_stats, &scripts,
                               It_PKind_Random - It_Kind_Kuriboh,
                               It_Kind_Kuriboh) ||
        !itArticle_ImportTable(&output->import, pokemon_table,
                               output->pokemon_articles,
                               IT_POKEMON_ARTICLE_COUNT,
                               &output->article_stats, &scripts, -1,
                               It_PKind_Start) ||
        !itCommandData_Import(&output->import, &scripts,
                              &output->command_data))
    {
        return false;
    }

    // Byte-sized flags occupy the word at 0x48 and retain their disc order.
    memcpy((u8*) &output->common_data + offsetof(ItemCommonData, x48_byte),
           HSD_DatBytes(&output->dat, common_offset + 0x48, 4), 4);

    LbColorScriptTable color_scripts = {
        color_table, IT_COMMON_COLOR_ENTRY_COUNT, output->color_entries,
    };
    if (!lbColorScript_Import(
            &output->dat, &color_scripts, 1, color_table,
            output->color_commands, IT_COMMON_COLOR_COMMAND_CAPACITY,
            &output->color_command_count, &output->import.status,
            &output->import.error_offset))
    {
        return false;
    }

    output->public_data.x0 = &output->common_data;
    output->public_data.x4 = output->common_articles;
    output->public_data.x8 = output->character_articles;
    output->public_data.xC = output->pokemon_articles;
    output->public_data.x10 = &output->zako_data;
    output->public_data.x14 = output->color_entries;
    return true;
}
