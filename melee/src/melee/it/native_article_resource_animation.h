#ifndef MELEE_IT_NATIVE_ARTICLE_RESOURCE_ANIMATION_H
#define MELEE_IT_NATIVE_ARTICLE_RESOURCE_ANIMATION_H

#include <melee/ft/forward.h>
#include <melee/it/forward.h>
#include <stdbool.h>
#include <stdint.h>

enum { NATIVE_ARTICLE_RESOURCE_ANIMATION_OWNERS = 64,
       NATIVE_ARTICLE_RESOURCE_ANIMATION_APPLICATIONS = 128,
       NATIVE_ARTICLE_RESOURCE_ANIMATION_PATH = 12 };

// Prepare declared source calls after resource model ownership, before
// history/actors. Names/paths are never resolved on ticks. Retirement
// follows graph teardown and retained-history drain, before package release.
bool nativeArticleResourceAnimation_Prepare(void);
bool nativeArticleResourceAnimation_Retire(void);

bool nativeArticleResource_AddAnimation(Item_GObj*, unsigned index,
    const uint8_t* path, unsigned depth, uint32_t bundle_ordinal);
bool nativeArticleResource_RequestAnimation(Item_GObj*, unsigned index,
    const uint8_t* path, unsigned depth, float frame);
bool nativeArticleResource_AdvanceAnimation(Item_GObj*, unsigned index,
    const uint8_t* path, unsigned depth);
bool nativeArticleResource_RemoveAnimation(Item_GObj*, unsigned index,
    const uint8_t* path, unsigned depth);
bool nativeArticleResource_SetAnimationRate(Item_GObj*, unsigned index,
    const uint8_t* path, unsigned depth, float rate);

#endif
