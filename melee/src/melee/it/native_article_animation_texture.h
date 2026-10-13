#ifndef MELEE_IT_NATIVE_ARTICLE_ANIMATION_TEXTURE_H
#define MELEE_IT_NATIVE_ARTICLE_ANIMATION_TEXTURE_H
#include <sysdolphin/baselib/mobj.h>
#include <stdbool.h>
#include <stdint.h>

enum { NATIVE_ARTICLE_ANIMATION_TEXTURE_LIMIT = 512 };
typedef struct NativeArticleAnimationTextureID {
    const HSD_TObjDesc* descriptor;
    GXTexMapID id;
} NativeArticleAnimationTextureID;

// Pure bounded plan for HSD_MObjCompileTev's HSD_TObjAssignResources call.
// Descriptor IDs are replaced, with the last bump/toon textures assigned last.
// Models the owned descriptor chain without external shadow/toon textures;
// callers reject active external material context before attachment. No native
// construction, class dispatch, GPU work, global texture lookup or allocation.
bool nativeArticleAnimation_TextureIDs(const HSD_MObjDesc*,
    NativeArticleAnimationTextureID*, uint32_t capacity, uint32_t* written);
#endif
