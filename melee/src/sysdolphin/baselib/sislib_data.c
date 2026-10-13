#include "sislib_data.h"
#include "sislib.h"
#include "dat_sis.h"
#include "debug.h"
#include "memory.h"
#include <melee/lb/lbfile.h>
#include <string.h>

static struct {
    void* bytes;
    void** table;
    HSD_Dat dat;
} fonts[5];

void HSD_SisLib_ResetData(void)
{
    // Called after the original scene heap transition has released its data.
    memset(fonts, 0, sizeof(fonts));
    memset(HSD_SisLib_804D1124, 0, sizeof(HSD_SisLib_804D1124));
}

void HSD_SisLib_FreeData(s32 font_idx)
{
    HSD_ASSERT(0, font_idx >= 0 && font_idx < 5);
    if (fonts[font_idx].bytes) {
        HSD_Free(fonts[font_idx].table);
        HSD_Free(fonts[font_idx].bytes);
    }
    memset(&fonts[font_idx], 0, sizeof(fonts[font_idx]));
    HSD_SisLib_804D1124[font_idx] = NULL;
}

void HSD_SisLib_803A62A0(s32 font_idx, char* archive_name, char* symbol_name)
{
    HSD_ASSERT(0, font_idx >= 0 && font_idx < 5);
    HSD_SisLib_FreeData(font_idx);
    size_t size = lbFileGetSize(archive_name);
    void* bytes = HSD_MemAlloc((size + 31) & ~(size_t) 31);
    HSD_ASSERT(0, bytes);
    lbFile_8001668C(archive_name, bytes, &size);
    HSD_Dat* dat = &fonts[font_idx].dat;
    HSD_ASSERT(0, HSD_DatOpen(dat, bytes, size) == HSD_DAT_OK);
    u32 root, count;
    HSD_ASSERT(0, HSD_DatFindRoot(dat, symbol_name, &root));
    HSD_ASSERT(0, HSD_DatSisLayout(dat, root, &count));
    void** table = HSD_MemAlloc(count * sizeof(*table));
    HSD_ASSERT(0, table && HSD_DatImportSis(dat, root, table, count));
    fonts[font_idx].bytes = bytes;
    fonts[font_idx].table = table;
    HSD_SisLib_804D1124[font_idx] = (SIS*) table;
}

const u8* HSD_SisLib_Jump(s32 font_idx, const u8* operand)
{
    HSD_ASSERT(0, font_idx >= 0 && font_idx < 5 && fonts[font_idx].bytes);
    // Copies of encoded strings retain file-relative operands as well.
    u32 offset = (u32) operand[0] << 24 | (u32) operand[1] << 16 |
                 (u32) operand[2] << 8 | operand[3];
    const u8* target = HSD_DatBytes(&fonts[font_idx].dat, offset, 1);
    HSD_ASSERT(0, target);
    return target;
}
