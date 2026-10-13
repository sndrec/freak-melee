#ifndef MELEE_IT_ARTICLE_MODEL_LAYERS_H
#define MELEE_IT_ARTICLE_MODEL_LAYERS_H

#include "forward.h"
#include <sysdolphin/baselib/dat_import.h>

// The source descriptor is sixteen bytes; this decoded object contains native
// pointers and must never be populated by copying the source record verbatim.
struct ArticleModelLayers {
    u16 normal_count;
    const u8* normal_indices;
    u16 outline_count;
    const u8* outline_indices;
};

enum {
    ARTICLE_MODEL_LAYER_INDEX_LIMIT = 256,
    ARTICLE_MODEL_LAYER_JOINT_LIMIT = 100,
};

ArticleModelLayers* articleModelLayers_Import(HSD_DatImport* import,
    u32 reference_slot, const HSD_Joint* model_root);
// Validate the item's own immutable metadata against its actual live model and
// dynamic ordinal table. This neither enables drawing nor changes visibility.
bool articleModelLayers_ValidateItem(const Item* item);
// Disable clears the binding; enable validates the item's own immutable lists
// against the primary model before publishing them. Failure leaves it disabled.
bool articleModelLayers_Bind(Item_GObj* item_gobj, bool enabled);

#endif
