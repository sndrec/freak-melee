#include "dat_rumble.h"

bool HSD_DatRumbleLayout(const HSD_Dat* dat, u32* patterns, u32* words)
{
    u32 root;
    if (dat->root_count != 1 || dat->extern_count ||
        !HSD_DatFindRoot(dat, "lbRumbleData", &root) || root % 2 ||
        root >= dat->data_size || (dat->data_size - root) % 8) return false;
    *patterns = (dat->data_size - root) / 8;
    *words = root / 2;
    return true;
}

bool HSD_DatImportRumble(const HSD_Dat* dat, HSD_RumblePattern* patterns,
                         size_t pattern_capacity, u16* words, size_t word_capacity)
{
    u32 count, word_count;
    if (!HSD_DatRumbleLayout(dat, &count, &word_count) ||
        count > pattern_capacity || word_count > word_capacity) return false;
    u32 root = word_count * 2;
    const char* external;
    for (u32 i = 0; i < count; ++i) {
        u32 target;
        if (HSD_DatResolve(dat, root + i * 8, &target, &external) != HSD_DAT_REF_LOCAL ||
            target % 2 || target >= root) return false;
    }
    for (u32 i = 0; i < word_count; ++i) {
        if (!HSD_DatReadU16(dat, i * 2, &words[i])) return false;
    }
    for (u32 i = 0; i < count; ++i) {
        u32 target;
        HSD_DatResolve(dat, root + i * 8, &target, &external);
        patterns[i].commands = words + target / 2;
        patterns[i].priority = dat->data[root + i * 8 + 4];
    }
    return true;
}
