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

HSD_DatReferenceKind HSD_DatResolve(const HSD_Dat* dat, u32 slot,
                                   u32* target, const char** external)
{
    u32 i;
    *external = NULL;
    if ((slot & 3) || !HSD_DatReadU32(dat, slot, target)) {
        return HSD_DAT_REF_INVALID;
    }
    if (dat->relocations_sorted) {
        u32 lo = 0;
        u32 hi = dat->relocation_count;
        while (lo < hi) {
            u32 mid = lo + (hi - lo) / 2;
            u32 candidate = read_be32(dat->relocations + (size_t) mid * 4);
            if (candidate < slot) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        if (lo < dat->relocation_count &&
            read_be32(dat->relocations + (size_t) lo * 4) == slot)
        {
            return HSD_DAT_REF_LOCAL;
        }
    } else {
        for (i = 0; i < dat->relocation_count; ++i) {
            if (read_be32(dat->relocations + (size_t) i * 4) == slot) {
                return HSD_DAT_REF_LOCAL;
            }
        }
    }
    for (i = 0; i < dat->extern_count; ++i) {
        const u8* entry = dat->externs + (size_t) i * 8;
        u32 link = read_be32(entry);
        while (link != U32_MAX) {
            if (link == slot) {
                *external = dat->strings + read_be32(entry + 4);
                return HSD_DAT_REF_EXTERNAL;
            }
            // Open already validated the chain and its termination.
            HSD_DatReadU32(dat, link, &link);
        }
    }
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

    view.relocations_sorted = true;
    u32 previous_slot = 0;
    for (i = 0; i < view.relocation_count; ++i) {
        u32 slot = read_be32(view.relocations + (size_t) i * 4);
        u32 target;
        if (i != 0 && slot < previous_slot) {
            view.relocations_sorted = false;
        }
        previous_slot = slot;
        if ((slot & 3) || !HSD_DatReadU32(&view, slot, &target) ||
            target > view.data_size) {
            return HSD_DAT_RELOCATION;
        }
    }
    for (i = 0; i < view.root_count + view.extern_count; ++i) {
        // Tables are adjacent; their total count is bounded by the file size.
        const u8* entry = view.roots + (size_t) i * 8;
        u32 offset = read_be32(entry);
        u32 symbol = read_be32(entry + 4);
        if (symbol >= view.strings_size ||
            memchr(view.strings + symbol, 0, view.strings_size - symbol) == NULL) {
            return HSD_DAT_SYMBOL;
        }
        if (i < view.root_count) {
            if (offset >= view.data_size) {
                return HSD_DAT_SYMBOL;
            }
        } else {
            // External references form lists of pointer slots terminated by -1.
            // Bound the walk even if a bad list cycles back to an earlier slot.
            u32 remaining = view.data_size / 4;
            while (offset != U32_MAX) {
                if (remaining-- == 0 || (offset & 3) ||
                    !HSD_DatReadU32(&view, offset, &offset)) {
                    return HSD_DAT_EXTERNAL;
                }
            }
        }
    }
    *dat = view;
    return HSD_DAT_OK;
}
