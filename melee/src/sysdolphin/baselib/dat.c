#include "dat.h"

#include <string.h>

static u32 read_be32(const u8* bytes)
{
    return ((u32) bytes[0] << 24) | ((u32) bytes[1] << 16) |
           ((u32) bytes[2] << 8) | bytes[3];
}

const u8* HSD_DatBytes(const HSD_Dat* dat, u32 offset, size_t size)
{
    if (offset > dat->data_size || size > dat->data_size - offset) {
        return NULL;
    }
    return dat->data + offset;
}

bool HSD_DatReadU32(const HSD_Dat* dat, u32 offset, u32* value)
{
    const u8* bytes = HSD_DatBytes(dat, offset, 4);
    if (bytes == NULL) {
        return false;
    }
    *value = read_be32(bytes);
    return true;
}

bool HSD_DatReadF32(const HSD_Dat* dat, u32 offset, f32* value)
{
    u32 bits;
    if (!HSD_DatReadU32(dat, offset, &bits)) {
        return false;
    }
    memcpy(value, &bits, sizeof(*value));
    return true;
}

bool HSD_DatReadU16(const HSD_Dat* dat, u32 offset, u16* value)
{
    const u8* bytes = HSD_DatBytes(dat, offset, 2);
    if (bytes == NULL) {
        return false;
    }
    *value = ((u16) bytes[0] << 8) | bytes[1];
    return true;
}

static bool has_local_slot(const HSD_Dat* dat, u32 first, u32 last)
{
    u32 begin = 0;
    for (u32 run = 0; run < dat->relocation_run_count; ++run) {
        u32 end = dat->relocation_run_ends[run];
        bool descending = (dat->descending_runs & (1u << run)) != 0;
        u32 lo = begin;
        u32 hi = end;
        while (lo < hi) {
            u32 mid = lo + (hi - lo) / 2;
            u32 candidate = read_be32(dat->relocations + (size_t) mid * 4);
            if (candidate >= first && candidate <= last) {
                return true;
            }
            if (descending ? candidate > last : candidate < first) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        begin = end;
    }
    if (dat->relocation_run_count == 0) {
        for (u32 i = 0; i < dat->relocation_count; ++i) {
            u32 slot = read_be32(dat->relocations + (size_t) i * 4);
            if (slot >= first && slot <= last) return true;
        }
    }
    return false;
}

static const char* external_slot_name(const HSD_Dat* dat, u32 first, u32 last)
{
    if (last < dat->first_external_slot || first > dat->last_external_slot) {
        return NULL;
    }
    for (u32 i = 0; i < dat->extern_count; ++i) {
        const u8* entry = dat->externs + (size_t) i * 8;
        u32 link = read_be32(entry);
        while (link != U32_MAX) {
            if (link >= first && link <= last) {
                return dat->strings + read_be32(entry + 4);
            }
            // Open already validated the chain and its termination.
            HSD_DatReadU32(dat, link, &link);
        }
    }
    return NULL;
}

bool HSD_DatContainsReference(const HSD_Dat* dat, u32 begin, u32 end)
{
    if (end > dat->data_size || begin > end || end - begin < 4) return false;
    return has_local_slot(dat, begin, end - 4) || external_slot_name(dat, begin, end - 4) != NULL;
}

HSD_DatReferenceKind HSD_DatResolve(const HSD_Dat* dat, u32 slot,
                                   u32* target, const char** external)
{
    *external = NULL;
    if ((slot & 3) || !HSD_DatReadU32(dat, slot, target)) return HSD_DAT_REF_INVALID;
    if (has_local_slot(dat, slot, slot)) return HSD_DAT_REF_LOCAL;
    *external = external_slot_name(dat, slot, slot);
    if (*external) return HSD_DAT_REF_EXTERNAL;
    return *target == 0 ? HSD_DAT_REF_NULL : HSD_DAT_REF_INVALID;
}

bool HSD_DatRelocation(const HSD_Dat* dat, u32 index, u32* offset)
{
    if (index >= dat->relocation_count) {
        return false;
    }
    *offset = read_be32(dat->relocations + (size_t) index * 4);
    return true;
}

u32 HSD_DatNextTarget(const HSD_Dat* dat, u32 offset)
{
    u32 result = dat->data_size;
    for (u32 i = 0; i < dat->relocation_count; ++i) {
        u32 slot;
        u32 target;
        if (HSD_DatRelocation(dat, i, &slot) &&
            HSD_DatReadU32(dat, slot, &target) && target > offset &&
            target < result)
        {
            result = target;
        }
    }
    return result;
}

bool HSD_DatSymbol(const HSD_Dat* dat, bool external, u32 index,
                   u32* offset, const char** name)
{
    const u8* table = external ? dat->externs : dat->roots;
    u32 count = external ? dat->extern_count : dat->root_count;
    if (index >= count) {
        return false;
    }
    table += (size_t) index * 8;
    *offset = read_be32(table);
    *name = dat->strings + read_be32(table + 4);
    return true;
}

bool HSD_DatFindRoot(const HSD_Dat* dat, const char* name, u32* offset)
{
    u32 i;
    for (i = 0; i < dat->root_count; ++i) {
        const u8* entry = dat->roots + (size_t) i * 8;
        if (strcmp(dat->strings + read_be32(entry + 4), name) == 0) {
            *offset = read_be32(entry);
            return true;
        }
    }
    return false;
}

HSD_DatResult HSD_DatOpen(HSD_Dat* dat, const void* bytes, size_t size)
{
    HSD_Dat view = { 0 };
    const u8* src = bytes;
    size_t cursor;
    u32 i;
    memset(dat, 0, sizeof(*dat));
    if (src == NULL || size < 32 || size > U32_MAX || read_be32(src) != size) {
        return HSD_DAT_HEADER;
    }
    view.data_size = read_be32(src + 4);
    view.relocation_count = read_be32(src + 8);
    view.root_count = read_be32(src + 12);
    view.extern_count = read_be32(src + 16);
    if (view.data_size > size - 32) {
        return HSD_DAT_TABLE;
    }
    view.data = src + 32;
    cursor = 32 + (size_t) view.data_size;
    if (view.relocation_count > (size - cursor) / 4) {
        return HSD_DAT_TABLE;
    }
    view.relocations = src + cursor;
    cursor += (size_t) view.relocation_count * 4;
    if (view.root_count > (size - cursor) / 8) {
        return HSD_DAT_TABLE;
    }
    view.roots = src + cursor;
    cursor += (size_t) view.root_count * 8;
    if (view.extern_count > (size - cursor) / 8) {
        return HSD_DAT_TABLE;
    }
    view.externs = src + cursor;
    cursor += (size_t) view.extern_count * 8;
    view.strings = (const char*) src + cursor;
    view.strings_size = size - cursor;

    if (view.extern_count > HSD_DAT_EXTERNAL_BUDGET ||
        view.root_count > HSD_DAT_MAX_ROOTS ||
        view.relocation_count > HSD_DAT_MAX_RELOCATIONS) {
        return HSD_DAT_RESOURCE_LIMIT;
    }
    // Index monotonic runs without changing the borrowed file or its ordinals.
    // Small, completely unordered tables also have a bounded linear lookup.
    u32 run = 0;
    int direction = 0;
    u32 previous_slot = 0;
    for (i = 0; i < view.relocation_count; ++i) {
        u32 slot = read_be32(view.relocations + (size_t) i * 4);
        u32 target;
        if (i && slot != previous_slot && run < HSD_DAT_MAX_RELOCATION_RUNS) {
            int step = slot > previous_slot ? 1 : -1;
            if (!direction) {
                direction = step;
            } else if (direction != step) {
                view.relocation_run_ends[run] = i;
                if (direction < 0) {
                    view.descending_runs |= 1u << run;
                }
                ++run;
                if (run == HSD_DAT_MAX_RELOCATION_RUNS &&
                    view.relocation_count > HSD_DAT_LINEAR_RELOCATIONS) {
                    return HSD_DAT_RESOURCE_LIMIT;
                }
                direction = 0;
            }
        }
        previous_slot = slot;
        if ((slot & 3) || !HSD_DatReadU32(&view, slot, &target) ||
            target > view.data_size) {
            return HSD_DAT_RELOCATION;
        }
    }
    if (view.relocation_count && run < HSD_DAT_MAX_RELOCATION_RUNS) {
        view.relocation_run_ends[run] = view.relocation_count;
        if (direction < 0) {
            view.descending_runs |= 1u << run;
        }
        view.relocation_run_count = run + 1;
    }
    // Each external pointer slot names one symbol. Bound the work over the
    // whole archive even when several external lists reference the same slots.
    u32 remaining_links = view.data_size / 4;
    u32 external_budget = HSD_DAT_EXTERNAL_BUDGET - view.extern_count;
    view.first_external_slot = U32_MAX;
    for (i = 0; i < view.root_count + view.extern_count; ++i) {
        // Tables are adjacent; their total count is bounded by the file size.
        const u8* entry = view.roots + (size_t) i * 8;
        u32 offset = read_be32(entry);
        u32 symbol = read_be32(entry + 4);
        if (symbol >= view.strings_size) {
            return HSD_DAT_SYMBOL;
        }
        size_t available = view.strings_size - symbol;
        size_t limit = HSD_DAT_MAX_SYMBOL_BYTES + 1;
        if (!memchr(view.strings + symbol, '\0', available < limit ? available : limit)) {
            return available > limit ? HSD_DAT_RESOURCE_LIMIT : HSD_DAT_SYMBOL;
        }
        if (i < view.root_count) {
            if (offset >= view.data_size) {
                return HSD_DAT_SYMBOL;
            }
        } else {
            // External references form lists of pointer slots terminated by -1.
            // Bound the walk even if a bad list cycles back to an earlier slot.
            while (offset != U32_MAX) {
                if (!external_budget--) {
                    return HSD_DAT_RESOURCE_LIMIT;
                }
                if (offset < view.first_external_slot) view.first_external_slot = offset;
                if (offset > view.last_external_slot) view.last_external_slot = offset;
                if (remaining_links-- == 0 || (offset & 3) ||
                    !HSD_DatReadU32(&view, offset, &offset)) {
                    return HSD_DAT_EXTERNAL;
                }
            }
        }
    }
    *dat = view;
    return HSD_DAT_OK;
}
