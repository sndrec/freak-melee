#ifndef MELEE_IT_NATIVE_ARTICLE_MODEL_SCENE_H
#define MELEE_IT_NATIVE_ARTICLE_MODEL_SCENE_H

#include <melee/it/forward.h>
#include <sysdolphin/baselib/forward.h>
#include "native_item_lifetime.h"

// One invocation's exact secondary slot. A vacant slot has root == NULL;
// occupied slots retain the allocation generation rather than a bare address.
typedef struct NativeArticleModelSceneSlot {
    NativeItemLifetime owner;
    NativeMatchPoolReference graph;
    HSD_JObj* root;
    const HSD_Joint* descriptor;
    uint64_t epoch;
    uint32_t slot, root_index;
} NativeArticleModelSceneSlot;
struct NativeSourcePackage;
// Primary descriptor construction uses the same independent, registered pool
// path as named roots. An occupied slot still owns a separate live graph.
#define NATIVE_ARTICLE_MODEL_PRIMARY_ROOT UINT32_MAX
enum { NATIVE_ARTICLE_MODEL_SCENE_CAPACITY=128 };
// Resolve the retained Item's exact published program in this scene's pinned
// package set. Also used by primary animation operations; no graph is aliased.
bool nativeArticleModelScene_ResolveOwner(const NativeItemLifetime*,
                                         const struct NativeSourcePackage**);

// Real scene transition boundaries, before actors/history and after history
// drains. Packages with secondary roots are leased until every graph is removed.
bool nativeArticleModelScene_Prepare(void);
bool nativeArticleModelScene_Retire(void);
// Exact immutable ItemProgram root ordinal, or PRIMARY_ROOT when the prepared
// program uses clone_model. Secondary slots are 1..3.
// Constructs replacement separately. Failure leaves the selected slot intact.
bool nativeArticleModelScene_Load(Item_GObj*, unsigned slot, unsigned root);
bool nativeArticleModelScene_CaptureSlot(Item_GObj*, unsigned slot,
                                        NativeArticleModelSceneSlot*);
bool nativeArticleModelScene_SlotCurrent(const NativeArticleModelSceneSlot*);
// Retained owner/slot admission plus bounded lazy scale allocation claim.
// A successful Begin must be balanced even if native traversal invalidates its
// caller. End verifies retained identities before publishing its status.
bool nativeArticleModelScene_BeginMatrices(Item_GObj*, unsigned slot);
bool nativeArticleModelScene_EndMatrices(void);
// Called by the actual Destroy path before graph removal. An owned root may
// never be freed through a borrowed Item/slot. Unregistered ordinary graphs are
// unaffected. This does not run the destructor itself.
bool nativeArticleModelScene_Revoke(Item_GObj*, unsigned slot, HSD_JObj*);

#include <melee/gm/native_match_arena.h>
bool nativeArticleModelScene_AdmitHistory(NativeMatchArena*);
#endif
