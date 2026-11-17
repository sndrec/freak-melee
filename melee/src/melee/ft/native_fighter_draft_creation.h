#pragma once
#include "native_fighter_family_project.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeFighterStockLayout {
    NativeFighterFamilyMode mode;
    FighterKind primary, partner;
} NativeFighterStockLayout;
bool nativeFighterDraft_StockLayout(FighterKind, NativeFighterStockLayout*);
// Construct a complete editable draft in a private sibling directory, then
// publish it without replacing an existing destination. Caller opens the disc.
// Stock drafts include owned scripts/parameters, split sources and UI images.
bool nativeFighterDraft_CreateStock(const char* path, const char* title,
    FighterKind, char* error, size_t error_size);
bool nativeFighterDraft_CreateFromPackage(const char* path, const char* title,
    const void* package, size_t package_size, char* error, size_t error_size);
#ifdef __cplusplus
}
#endif
