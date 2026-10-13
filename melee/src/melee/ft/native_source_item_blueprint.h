#ifndef MELEE_FT_NATIVE_SOURCE_ITEM_BLUEPRINT_H
#define MELEE_FT_NATIVE_SOURCE_ITEM_BLUEPRINT_H
#include <melee/it/itarticle_data.h>
#include "native_script_ir.h"
#include <stddef.h>

// Portable envelope around an original DAT and an explicit Article descriptor
// offset. Runtime never searches a stock item bank or inherits its callbacks.
enum { NATIVE_SOURCE_BLUEPRINT_HEADER=16, NATIVE_SOURCE_BLUEPRINT_ARENA=2*1024*1024 };
typedef struct NativeSourceItemBlueprint {
    const struct NativeSourceItemBlueprint* address;
    const void* bytes;
    size_t size;
    void* arena;
    HSD_Dat dat;
    HSD_DatImport import;
    ItArticleImportStats stats;
    ItCommandData commands;
    Article* article;
} NativeSourceItemBlueprint;
bool nativeSourceItemBlueprint_Write(const void* dat,size_t bytes,uint32_t article,
    void* output,size_t capacity,size_t* written,NativeScriptDiagnostic*);
// Owner is vacant and stays at its final address. Input remains pinned.
bool nativeSourceItemBlueprint_Import(const void*,size_t,NativeSourceItemBlueprint*,NativeScriptDiagnostic*);
void nativeSourceItemBlueprint_Release(NativeSourceItemBlueprint*);
#endif
