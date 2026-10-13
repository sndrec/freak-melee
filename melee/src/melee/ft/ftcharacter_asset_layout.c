#include "ftcharacter_assets.h"
#include <stddef.h>

bool ftCharacterAssetLayout_IsValid(const FtCharacterAssetLayout* layout)
{
    if (layout == NULL || layout->item_format >= FtCharacterItems_FormatCount ||
        layout->byte_range_count > FT_CHARACTER_BYTE_RANGE_LIMIT ||
        (layout->article_mask & layout->joint_graph_mask) != 0 ||
        (layout->visibility_mask &
         (layout->article_mask | layout->joint_graph_mask)) != 0)
        return false;
    for (unsigned i = 0; i < layout->byte_range_count; ++i) {
        const FtCharacterByteRange* range = &layout->byte_ranges[i];
        if (range->size == 0 || range->offset > FT_CHARACTER_EXT_ATTR_CAPACITY ||
            range->size > FT_CHARACTER_EXT_ATTR_CAPACITY - range->offset)
            return false;
        for (unsigned j = 0; j < i; ++j) {
            const FtCharacterByteRange* other = &layout->byte_ranges[j];
            if (range->offset < other->offset + other->size &&
                other->offset < range->offset + range->size)
                return false;
        }
    }
    const u16 required[] = { 0, 4, 14, 8, 0x400, 8, 0, 0x3FF };
    const u16 reserved[] = { 0, 0, 0, 0, 0, 0x10, 0x02, 0x400 };
    if (((layout->article_mask | layout->joint_graph_mask) &
         reserved[layout->item_format]) != 0)
        return false;
    // Paper's slot10 is no longer a special decoder: its typed visibility
    // recipe owns the slot. Other format-owned non-article slots stay reserved.
    if (layout->item_format != FtCharacterItems_PaperArticles &&
        (layout->visibility_mask & reserved[layout->item_format]) != 0)
        return false;
    return (layout->article_mask & required[layout->item_format]) ==
           required[layout->item_format];
}
