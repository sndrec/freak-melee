#ifndef MELEE_IT_ITCOMMON_DATA_H
#define MELEE_IT_ITCOMMON_DATA_H

#include "it_3F14.h"
#include "itarticle_data.h"

#include <melee/lb/types.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    IT_COMMON_ARTICLE_COUNT = 43,
    IT_CHARACTER_ARTICLE_COUNT = It_PKind_Start - It_Kind_Kuriboh,
    IT_POKEMON_ARTICLE_COUNT = 47,
    IT_COMMON_COLOR_ENTRY_COUNT = 7,
    IT_COMMON_COLOR_COMMAND_CAPACITY = 128,
    IT_COMMON_IMPORT_ARENA_SIZE = 8 * 1024 * 1024,
    IT_COMMON_IMPORT_RECORD_CAPACITY = 32768,
};

typedef struct ItCommonAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    it_804D6D20_t public_data;
    ItemCommonData common_data;
    Article* common_articles[IT_COMMON_ARTICLE_COUNT];
    Article* character_articles[IT_CHARACTER_ARTICLE_COUNT];
    Article* pokemon_articles[IT_POKEMON_ARTICLE_COUNT];
    it_804D6D40_t zako_data;
    Fighter_804D653C_t color_entries[IT_COMMON_COLOR_ENTRY_COUNT];
    union ColorOverlay_x8_t color_commands[IT_COMMON_COLOR_COMMAND_CAPACITY];
    u32 color_command_count;
    ItArticleImportStats article_stats;
    ItCommandData command_data;
} ItCommonAssets;

bool itCommonAssets_Import(const void* bytes, size_t size, void* arena,
                           size_t arena_size, u32 record_capacity,
                           ItCommonAssets* output);

#endif
