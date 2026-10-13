#include "native_article_animation_texture.h"

bool nativeArticleAnimation_TextureIDs(const HSD_MObjDesc* material,
    NativeArticleAnimationTextureID* output, uint32_t capacity, uint32_t* written)
{
    if (!material || !written || capacity > NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT ||
        (capacity && !output))
        return false;
    uint32_t count = 0, bump = UINT32_MAX, toon = UINT32_MAX;
    for (const HSD_TObjDesc* t = material->texdesc; t; t = t->next) {
        if (count == capacity) return false;
        for (uint32_t prior = 0; prior < count; ++prior)
            if (output[prior].descriptor == t) return false;
        output[count] = (NativeArticleAnimationTextureID){ t, GX_TEXMAP_NULL };
        if ((t->blend_flags & TEX_COORD_MASK) == TEX_COORD_TOON) toon = count;
        else if ((t->blend_flags & TEX_BUMP)) bump = count;
        ++count;
    }
    uint32_t limit = 8 - (toon != UINT32_MAX) - 2 * (bump != UINT32_MAX);
    uint32_t next = 0;
    // GX_TEXMAP0..7 are consecutive, as in native HSD_Index2TexMap's switch.
    for (uint32_t i = 0; i < count; ++i) {
        const HSD_TObjDesc* t = output[i].descriptor;
        if ((t->blend_flags & TEX_COORD_MASK) != TEX_COORD_TOON && !(t->blend_flags & TEX_BUMP) && next < limit)
            output[i].id = (GXTexMapID)(GX_TEXMAP0 + next++);
    }
    if (bump != UINT32_MAX) output[bump].id = (GXTexMapID)(GX_TEXMAP0 + next++);
    if (toon != UINT32_MAX) output[toon].id = (GXTexMapID)(GX_TEXMAP0 + next++);
    *written = count;
    return true;
}
