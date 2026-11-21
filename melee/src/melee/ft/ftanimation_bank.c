#include "ftanimation_bank.h"
#include "ftparts.h"

#include <melee/lb/lbanim_dat.h>

#include <string.h>

typedef struct FtAnimationArena {
    u8* base;
    size_t size;
    size_t used;
} FtAnimationArena;

typedef struct FtAnimationClip {
    const char* name;
    size_t offset;
    u32 size;
    u32 root;
} FtAnimationClip;

enum { FT_ANIMATION_BANK_MAX_SYMBOLS = 1024 };

bool ftAnimationBank_FitsRig(const struct Fighter_WaitAnimData* clip,
                             unsigned parts, unsigned insertions)
{
    const FigaTree* tree = clip->x14;
    if (!tree || parts > MAX_FT_PARTS || insertions > parts || insertions > 32)
        return false;
    unsigned expected = parts - insertions;
    unsigned mask = ((u32)clip->x10_animCurrFlags >> 11) & 0x1fff;
    for (unsigned i = 0; i < insertions; ++i)
        expected += (mask >> i) & 1;
    unsigned nodes = 0;
    while (nodes < expected && tree->nodes[nodes] != -1) ++nodes;
    return nodes == expected && tree->nodes[nodes] == -1;
}

static u32 read_be32(const u8* bytes)
{
    return ((u32) bytes[0] << 24) | ((u32) bytes[1] << 16) |
           ((u32) bytes[2] << 8) | bytes[3];
}

static HSD_DatResult index_clips(const u8* bytes, size_t size,
                                 FtAnimationClip* clips, u32* clip_count)
{
    size_t cursor = 0;
    *clip_count = 0;
    while (cursor < size) {
        if (bytes[cursor] == 0xFF) {
            ++cursor;
            continue;
        }
        if (size - cursor < 32) {
            return HSD_DAT_HEADER;
        }
        u32 length = read_be32(bytes + cursor);
        if (length < 32 || length > size - cursor) {
            return HSD_DAT_HEADER;
        }
        HSD_Dat dat;
        HSD_DatResult status = HSD_DatOpen(&dat, bytes + cursor, length);
        if (status != HSD_DAT_OK) {
            return status;
        }
        for (u32 root_index = 0; root_index < dat.root_count; ++root_index) {
            u32 root;
            const char* name;
            if (*clip_count == FT_ANIMATION_BANK_MAX_SYMBOLS) {
                return HSD_DAT_CAPACITY;
            }
            if (!HSD_DatSymbol(&dat, false, root_index, &root, &name)) {
                return HSD_DAT_SYMBOL;
            }
            clips[(*clip_count)++] = (FtAnimationClip){
                name, cursor, length, root
            };
        }
        cursor += length;
    }
    return HSD_DAT_OK;
}

static void* arena_alloc(FtAnimationArena* arena, size_t size)
{
    size_t padding = (-(uintptr_t) (arena->base + arena->used)) &
                     (sizeof(void*) - 1);
    if (padding > arena->size - arena->used ||
        size > arena->size - arena->used - padding)
    {
        return NULL;
    }
    void* result = arena->base + arena->used + padding;
    arena->used += padding + size;
    return result;
}

bool ftAnimationBank_Import(const void* bytes, size_t size,
                            const FtAnimationBankTable* tables, u32 table_count,
                            void* arena, size_t arena_size,
                            FtAnimationBankStats* stats)
{
    if (bytes == NULL || tables == NULL || arena == NULL || stats == NULL) {
        return false;
    }
    memset(stats, 0, sizeof(*stats));
    for (u32 t = 0; t < table_count; ++t) {
        if (tables[t].count != 0 && tables[t].entries == NULL) {
            stats->status = HSD_DAT_DESCRIPTOR;
            stats->failed_table = t;
            return false;
        }
        for (u32 i = 0; i < tables[t].count; ++i)
            tables[t].entries[i].x14 = NULL;
    }
    FtAnimationArena storage = { arena, arena_size, 0 };
    FtAnimationClip clips[FT_ANIMATION_BANK_MAX_SYMBOLS];
    u32 clip_count = 0;
    bool indexed = false;
    size_t stream_bytes = 0;
    FigaTrack* scratch = arena_alloc(
        &storage, FT_ANIMATION_SCRATCH_TRACK_CAPACITY * sizeof(*scratch));
    if (scratch == NULL) {
        stats->status = HSD_DAT_CAPACITY;
        return false;
    }
    for (u32 t = 0; t < table_count; ++t) {
        for (u32 i = 0; i < tables[t].count; ++i) {
            struct Fighter_WaitAnimData* entry = &tables[t].entries[i];
            entry->x14 = NULL;
            if (entry->x8 == 0) {
                continue;
            }
            stats->failed_table = t;
            stats->failed_index = i;
            if (entry->x0 == NULL) {
                stats->status = HSD_DAT_DESCRIPTOR;
                return false;
            }
            HSD_Dat dat;
            u32 root = 0;
            bool direct = entry->x4 >= 0 && entry->x8 >= 32 &&
                          (size_t) entry->x4 <= size &&
                          (size_t) entry->x8 <= size - (size_t) entry->x4 &&
                          HSD_DatOpen(&dat, (const u8*) bytes + entry->x4,
                                      entry->x8) == HSD_DAT_OK &&
                          HSD_DatFindRoot(&dat, entry->x0, &root);
            if (!direct) {
                if (!indexed) {
                    stats->status = index_clips(bytes, size, clips, &clip_count);
                    if (stats->status != HSD_DAT_OK) {
                        return false;
                    }
                    indexed = true;
                }
                const FtAnimationClip* match = NULL;
                for (u32 clip = 0; clip < clip_count; ++clip) {
                    if (strcmp(clips[clip].name, entry->x0) == 0) {
                        if (match != NULL) {
                            stats->status = HSD_DAT_SYMBOL;
                            return false;
                        }
                        match = &clips[clip];
                    }
                }
                if (match == NULL) {
                    if (tables[t].allow_missing) continue;
                    stats->status = HSD_DAT_SYMBOL;
                    return false;
                }
                stats->status = HSD_DatOpen(
                    &dat, (const u8*) bytes + match->offset, match->size);
                if (stats->status != HSD_DAT_OK) {
                    return false;
                }
                root = match->root;
                entry->x4 = (s32) match->offset;
                entry->x8 = (s32) match->size;
            }
            FigaTree decoded;
            u32 node_count;
            HSD_DatResult status = lbAnim_DecodeFigaTree(
                &dat, root, &decoded, scratch,
                FT_ANIMATION_SCRATCH_TRACK_CAPACITY, &node_count);
            if (status != HSD_DAT_OK) {
                stats->status = status;
                return false;
            }
            u32 track_count = 0;
            for (u32 node = 0; node < node_count; ++node) {
                track_count += (u8) decoded.nodes[node];
            }
            for (u32 track = 0; track < track_count; ++track)
                stream_bytes += scratch[track].length;
            if (stream_bytes > 64 * 1024 * 1024) {
                stats->status = HSD_DAT_CAPACITY;
                return false;
            }
            FigaTree* tree = arena_alloc(&storage, sizeof(*tree));
            FigaTrack* tracks = track_count != 0
                                    ? arena_alloc(&storage,
                                                  (size_t) track_count *
                                                      sizeof(*tracks))
                                    : NULL;
            if (tree == NULL || (track_count != 0 && tracks == NULL)) {
                stats->status = HSD_DAT_CAPACITY;
                return false;
            }
            if (track_count != 0) {
                memcpy(tracks, scratch, (size_t) track_count * sizeof(*tracks));
            }
            *tree = decoded;
            tree->tracks = tracks;
            entry->x14 = tree;
            ++stats->clip_count;
            if (track_count > stats->max_tracks) {
                stats->max_tracks = track_count;
            }
        }
    }
    stats->arena_used = storage.used;
    return true;
}
