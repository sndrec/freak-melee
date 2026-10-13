#ifndef MELEE_FT_FTKIRBY_COPY_ASSETS_H
#define MELEE_FT_FTKIRBY_COPY_ASSETS_H

#include "forward.h"
#include "types.h"

#include <melee/it/itarticle_data.h>
#include <sysdolphin/baselib/dat_import.h>

enum {
    FT_KIRBY_COPY_RESOURCE_COUNT = 11,
    FT_KIRBY_COPY_IMPORT_ARENA_SIZE = 1024 * 1024,
    FT_KIRBY_COPY_IMPORT_RECORD_CAPACITY = 4096,
};

typedef struct FtKirbyCopyAssets {
    void* owned_bytes;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    KirbyCopyData data;
    ItArticleImportStats article_stats;
    ItCommandData item_command_data;
} FtKirbyCopyAssets;

bool ftKirbyCopyAssets_Import(const void* bytes, size_t size,
                              FighterKind copied_kind,
                              const char* root_name, void* arena,
                              size_t arena_size, u32 record_capacity,
                              FtKirbyCopyAssets* output);

#endif
