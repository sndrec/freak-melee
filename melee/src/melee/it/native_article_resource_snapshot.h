#ifndef MELEE_IT_NATIVE_ARTICLE_RESOURCE_SNAPSHOT_H
#define MELEE_IT_NATIVE_ARTICLE_RESOURCE_SNAPSHOT_H
#include "native_article_resource_internal.h"
#include <melee/gm/native_match_arena.h>

typedef struct NativeArticleResourceSnapshotHeader {
    uint64_t scene_identity, model_identity, object_epoch;
    uint32_t generation, collections;
} NativeArticleResourceSnapshotHeader;

// Private scene-local state. Prepared programs/recipes/assets remain leased.
// Stores occupied collection prefixes and created node records, plus generation.
// All peer pools, graph backing, Fighter borrowers and script actors must restore
// before callbacks. Validation checks structure/frozen identities without
// resolving candidate references against currently occupied peer slots.
size_t nativeArticleResource_SnapshotBytes(void);
bool nativeArticleResource_Save(void*,size_t capacity,size_t* written);
bool nativeArticleResource_ValidateSnapshot(const void*,size_t bytes);
bool nativeArticleResource_Load(const void*,size_t bytes);
bool nativeArticleResource_AdmitHistory(NativeMatchArena*);
#endif
