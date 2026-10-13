#ifndef MELEE_FT_NATIVE_COSTUME_PACKAGE_H
#define MELEE_FT_NATIVE_COSTUME_PACKAGE_H

#ifdef MELEE_NATIVE

#include <melee/gm/native_content_store.h>

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeCostumeValidation {
    unsigned import_status;
    size_t error_offset;
    size_t imported_records;
    size_t arena_used;
} NativeCostumeValidation;

// Checks the original costume DAT against the selected stock costume's required
// symbols and typed model/material graph before it enters the collection.
bool nativeCostumePackage_Validate(const NativeCostumePackage* costume,
                                   const void* dat, size_t dat_size,
                                   const char* root,
                                   NativeCostumeValidation* result);

#ifdef __cplusplus
}
#endif

#endif
#endif
