#ifndef SYSDOLPHIN_BASELIB_DAT_H
#define SYSDOLPHIN_BASELIB_DAT_H

#include <Runtime/platform.h>

#ifdef __cplusplus
extern "C" {
#endif

// Immutable on-disc view for native conversion. None of these bytes are a
// native HSD descriptor: pointer fields remain big-endian 32-bit offsets.
// The caller owns the byte buffer and keeps it alive while using this view.
typedef struct HSD_Dat {
    const u8* data;
    const u8* relocations;
    const u8* roots;
    const u8* externs;
    const char* strings;
    size_t strings_size;
    u32 data_size;
    u32 relocation_count;
    u32 root_count;
    u32 extern_count;
    bool relocations_sorted;
} HSD_Dat;

typedef enum HSD_DatResult {
    HSD_DAT_OK,
    HSD_DAT_HEADER,
    HSD_DAT_TABLE,
    HSD_DAT_RELOCATION,
    HSD_DAT_SYMBOL,
    HSD_DAT_EXTERNAL,
    HSD_DAT_DESCRIPTOR,
    HSD_DAT_CAPACITY,
} HSD_DatResult;

typedef enum HSD_DatReferenceKind {
    HSD_DAT_REF_INVALID,
    HSD_DAT_REF_NULL,
    HSD_DAT_REF_LOCAL,
    HSD_DAT_REF_EXTERNAL,
} HSD_DatReferenceKind;

HSD_DatResult HSD_DatOpen(HSD_Dat* dat, const void* bytes, size_t size);
// Accessors require a successfully opened view and non-NULL output arguments.
const u8* HSD_DatBytes(const HSD_Dat* dat, u32 offset, size_t size);
bool HSD_DatReadU32(const HSD_Dat* dat, u32 offset, u32* value);
bool HSD_DatReadU16(const HSD_Dat* dat, u32 offset, u16* value);
bool HSD_DatReadF32(const HSD_Dat* dat, u32 offset, f32* value);
bool HSD_DatRelocation(const HSD_Dat* dat, u32 index, u32* offset);
u32 HSD_DatNextTarget(const HSD_Dat* dat, u32 offset);
bool HSD_DatSymbol(const HSD_Dat* dat, bool external, u32 index,
                   u32* offset, const char** name);
bool HSD_DatFindRoot(const HSD_Dat* dat, const char* name, u32* offset);
HSD_DatReferenceKind HSD_DatResolve(const HSD_Dat* dat, u32 slot,
                                   u32* target, const char** external);

#ifdef __cplusplus
}
#endif
#endif
