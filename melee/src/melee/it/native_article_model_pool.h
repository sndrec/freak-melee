#ifndef MELEE_NATIVE_ARTICLE_MODEL_POOL_H
#define MELEE_NATIVE_ARTICLE_MODEL_POOL_H

#include "native_article_model_demand.h"

enum { NATIVE_ARTICLE_MODEL_POOL_ROOTS = 256 };

typedef struct NativeArticleModelPoolEntry {
    const HSD_Joint* descriptor;
    NativeArticleModelDemand demand;
} NativeArticleModelPoolEntry;

// One scene owner, not one reservation registration per Item. The caller pins
// immutable descriptors and supplies captured, stable metadata AND backing.
// Retained history must drain before either is retired or rebound.
typedef struct NativeArticleModelPool {
    NativeHsdReservation reservation;
    NativeArticleModelPoolEntry* entries;
    void* backing;
    size_t bytes;
    uint64_t epoch;
    uint32_t count;
} NativeArticleModelPool;

// Explicit transition only, after HSD allocator/class initialization. Capacity
// includes a full replacement graph and lazy matrix scale storage. This is a
// conservative allocation-call bound, not a precreated graph population.
// Zero/failure preserves bytes/alignment outputs.
bool nativeArticleModelPool_Size(const HSD_Joint* const* roots, uint32_t count,
    uint32_t live_graph_limit, size_t* bytes, size_t* alignment);
// The destination must be vacant. A failed admission returns it to vacant;
// supplied backing is scratch on failure. No live graph/ID is constructed here.
bool nativeArticleModelPool_Bind(NativeArticleModelPool*, void* backing,
    size_t bytes, const HSD_Joint* const* roots, uint32_t count,
    uint32_t live_graph_limit, uint64_t epoch);
// Complete capacity/environment admission precedes real HSD graph creation.
// A failed claim does not register IDs or disturb existing graphs.
HSD_JObj* nativeArticleModelPool_Construct(NativeArticleModelPool*,
    const HSD_Joint* descriptor);
// Begin at an actual matrix/draw boundary; caller ends reservation after native
// traversal. Only a root allocated by this exact owner is accepted. No fallback.
bool nativeArticleModelPool_BeginMatrices(NativeArticleModelPool*, HSD_JObj*);
// Does not run graph destructors: every graph must already have been removed.
bool nativeArticleModelPool_Release(NativeArticleModelPool*);

#endif
