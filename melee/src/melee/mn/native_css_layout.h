#ifndef MELEE_MN_NATIVE_CSS_LAYOUT_H
#define MELEE_MN_NATIVE_CSS_LAYOUT_H

#ifdef MELEE_NATIVE

#include <melee/gm/native_content_store.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    NATIVE_CSS_LAYOUT_MAX_TILES = 256,
    NATIVE_CSS_LAYOUT_EXTENT = 1000,
    NATIVE_CSS_LAYOUT_MIN_TILE_SIZE = 48,
};

typedef struct NativeCssTile {
    uint64_t stock_character_id;
    NativeContentId fighter_package_id;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} NativeCssTile;

typedef struct NativeCssLayout {
    uint16_t count;
    NativeCssTile tiles[NATIVE_CSS_LAYOUT_MAX_TILES];
} NativeCssLayout;

void nativeCssLayout_Default(NativeCssLayout* layout);
bool nativeCssLayout_Validate(const NativeCssLayout* layout);
bool nativeCssLayout_Add(NativeCssLayout* layout, const NativeCssTile* tile);
bool nativeCssLayout_Update(NativeCssLayout* layout, unsigned index,
                            const NativeCssTile* tile);
bool nativeCssLayout_Remove(NativeCssLayout* layout, unsigned index);
bool nativeCssLayout_ReplaceFighter(NativeCssLayout* layout,
                                    const NativeContentId* previous,
                                    const NativeContentId* next);

// The file belongs to the installation, not a profile. A missing file is
// reported separately so the caller can use the stock default. A malformed
// file is an error and must not replace the caller's current layout.
bool nativeCssLayout_LoadFile(NativeCssLayout* layout, const char* path,
                              bool* missing);
bool nativeCssLayout_SaveFile(const NativeCssLayout* layout, const char* path);
bool nativeCssLayout_DefaultPath(char* path, unsigned capacity);

#ifdef __cplusplus
}
#endif

#endif
#endif
