#include "dat_sis.h"

bool HSD_DatSisLayout(const HSD_Dat* dat, u32 root, u32* count)
{
    u32 end = dat->data_size, slot = root;
    if (root % 4 || root >= end) return false;
    while (slot < end) {
        u32 target;
        const char* external;
        HSD_DatReferenceKind kind = HSD_DatResolve(dat, slot, &target, &external);
        if (kind == HSD_DAT_REF_LOCAL) {
            if (target <= slot) return false;
            if (target < end) end = target;
        } else if (kind != HSD_DAT_REF_NULL) {
            return false;
        }
        if (end - slot < 4) return false;
        slot += 4;
    }
    *count = (end - root) / 4;
    return *count >= 2;
}

bool HSD_DatImportSis(const HSD_Dat* dat, u32 root, void** table,
                      size_t capacity)
{
    u32 count;
    if (!HSD_DatSisLayout(dat, root, &count) || capacity < count) return false;
    for (u32 i = 0; i < count; ++i) {
        u32 target;
        const char* external;
        HSD_DatReferenceKind kind = HSD_DatResolve(dat, root + i * 4,
                                                  &target, &external);
        // Layout validated all entries. Byte data remains in the owned DAT;
        // only its pointer table expands to native width.
        table[i] = kind == HSD_DAT_REF_NULL ? NULL : (void*) (dat->data + target);
    }
    return true;
}
