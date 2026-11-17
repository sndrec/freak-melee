#ifndef MELEE_FT_NATIVE_SOURCE_ITEM_BLUEPRINT_H
#define MELEE_FT_NATIVE_SOURCE_ITEM_BLUEPRINT_H
#include <melee/it/itarticle_data.h>
#include "native_source_item_blueprint_io.h"
#include <stddef.h>

// Portable envelope around an original DAT and an explicit Article descriptor
// offset. Runtime never searches a stock item bank or inherits its callbacks.
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
#ifdef __cplusplus
extern "C" {
#endif
// Owner is vacant and stays at its final address. Input remains pinned.
bool nativeSourceItemBlueprint_Import(const void*,size_t,NativeSourceItemBlueprint*,NativeScriptDiagnostic*);
void nativeSourceItemBlueprint_Release(NativeSourceItemBlueprint*);
#ifdef __cplusplus
}
#endif
#endif
