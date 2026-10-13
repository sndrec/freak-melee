#ifndef MELEE_NATIVE_ARTICLE_REFS_H
#define MELEE_NATIVE_ARTICLE_REFS_H

#include <melee/it/forward.h>
#include <sysdolphin/baselib/forward.h>
#include <stdbool.h>
#include <stdint.h>
#include <dolphin/mtx.h>

// Read-only references resolve full creation IDs within the caller's immutable
// package. Field indices refer to compiled declarations, never raw engine data.
bool nativeArticleRef_Exists(HSD_GObj*, uint32_t, ItemKind);
uint32_t nativeArticleRef_CurrentOwner(HSD_GObj*, uint32_t, ItemKind);
// Primary model root's local translation, not item position or world position.
// Missing, retired or changed source/recipient leaves the output untouched.
bool nativeArticleRef_PrimaryModelTranslation(HSD_GObj*, uint32_t, ItemKind,
                                               Vec3*);
float nativeArticleRef_Float(HSD_GObj*, uint32_t, ItemKind, unsigned, int32_t);
int32_t nativeArticleRef_Int(HSD_GObj*, uint32_t, ItemKind, unsigned, int32_t);
bool nativeArticleRef_Bool(HSD_GObj*, uint32_t, ItemKind, unsigned, int32_t);

#endif
