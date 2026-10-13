#ifndef MELEE_FT_CHARACTER_ASSET_LAYOUT_H
#define MELEE_FT_CHARACTER_ASSET_LAYOUT_H

#include <melee/ft/forward.h>
#include <stdbool.h>
#include <stdint.h>

// Serialized DAT schemas describe data only. They never select fighter code.
typedef enum FtCharacterItemFormat {
    FtCharacterItems_Plain,
    FtCharacterItems_ClimberString,
    FtCharacterItems_LinkWeapons,
    FtCharacterItems_Chain,
    FtCharacterItems_Yoyo,
    FtCharacterItems_Grapple,
    FtCharacterItems_ModelParts,
    FtCharacterItems_PaperArticles,
    FtCharacterItems_FormatCount,
} FtCharacterItemFormat;

enum { FT_CHARACTER_BYTE_RANGE_LIMIT = 8 };
typedef struct FtCharacterByteRange {
    uint16_t offset;
    uint16_t size;
} FtCharacterByteRange;

typedef struct FtCharacterAssetLayout {
    uint16_t article_mask;
    uint16_t joint_graph_mask;
    // Typed fighter DObj-variant lookups, independently decoded per asset slot.
    uint16_t visibility_mask;
    uint8_t item_format;
    uint8_t byte_range_count;
    FtCharacterByteRange byte_ranges[FT_CHARACTER_BYTE_RANGE_LIMIT];
} FtCharacterAssetLayout;

// Used when initially decoding stock archives. Native definitions own a copy
// of these facts, independently of any stock callback inheritance.
extern const FtCharacterAssetLayout
    ftCharacterStockLayouts[Ft_Kind_StockCount];
bool ftCharacterAssetLayout_IsValid(const FtCharacterAssetLayout* layout);

#endif
