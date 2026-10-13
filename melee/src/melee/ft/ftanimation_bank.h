#ifndef MELEE_FT_FTANIMATION_BANK_H
#define MELEE_FT_FTANIMATION_BANK_H

#include "types.h"
#include <sysdolphin/baselib/dat.h>

enum {
    FT_ANIMATION_IMPORT_ARENA_SIZE = 2 * 1024 * 1024,
    FT_ANIMATION_SCRATCH_TRACK_CAPACITY = 4096,
};

typedef struct FtAnimationBankTable {
    struct Fighter_WaitAnimData* entries;
    u32 count;
    // Only absent named symbols may be skipped. Present malformed clips fail.
    bool allow_missing;
} FtAnimationBankTable;

typedef struct FtAnimationBankStats {
    u32 clip_count;
    u32 max_tracks;
    size_t arena_used;
    HSD_DatResult status;
    u32 failed_table;
    u32 failed_index;
} FtAnimationBankStats;

bool ftAnimationBank_Import(const void* bytes, size_t size,
                            const FtAnimationBankTable* tables, u32 table_count,
                            void* arena, size_t arena_size,
                            FtAnimationBankStats* stats);

#endif
