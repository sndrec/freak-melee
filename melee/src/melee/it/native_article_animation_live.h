#ifndef MELEE_IT_NATIVE_ARTICLE_ANIMATION_LIVE_H
#define MELEE_IT_NATIVE_ARTICLE_ANIMATION_LIVE_H

#include <stdbool.h>
#include <stdint.h>
#include <sysdolphin/baselib/forward.h>

struct NativeScriptItemProgram;
struct _HSD_TexAnim;
enum NativeArticleAnimationTargetKind {
    NativeArticleAnimationTarget_Joint,
    NativeArticleAnimationTarget_Material,
    NativeArticleAnimationTarget_Texture
};
typedef struct NativeArticleAnimationOrigin {
    const HSD_Joint* graph;
    const void* target;
    const HSD_AObjDesc* controller;
    uint32_t kind;
    const struct _HSD_TexAnim* texture; // Selected native borrowed image table metadata.
} NativeArticleAnimationOrigin;

typedef struct NativeArticleAnimationApplication {
    const HSD_Joint* target; // Exact subtree within the whole graph.
    const HSD_AnimJoint* anim;
    const HSD_MatAnimJoint* matanim;
    const HSD_ShapeAnimJoint* shapeanim;
} NativeArticleAnimationApplication;

// Pure transition preparation for explicit applications, including resource
// graphs. No graph x subtree x bundle expansion: callers provide exact recipes.
// Output is one sorted contiguous graph block; buffer is scratch on failure.
bool nativeArticleAnimation_PrepareApplications(const HSD_Joint*,
    const NativeArticleAnimationApplication*, uint32_t application_count,
    NativeArticleAnimationOrigin*, uint32_t capacity, uint32_t* written);

// Transition preparation over pinned descriptors. Entries describe the whole
// model graph and the actual subtree targets of declared Add operations; native
// Article states apply only to that Article's primary descriptor. Unsupported
// state triplets are not allowed origins. Destination is scratch on failure;
// written is published only on success. Append complete per-graph results.
bool nativeArticleAnimation_PrepareOrigins(const HSD_Joint*,
    const struct NativeScriptItemProgram*, NativeArticleAnimationOrigin*,
    uint32_t capacity, uint32_t* written);

// Admission of an entire selected graph, including sparse retained controllers.
// No allocation, compressed-stream interpretation, callback or ID-table lookup.
// Caller retains physical graph/Item/program ownership across the native call.
// Optional attachment owner admits only unanimated native JObj constraints
// targeting that live Fighter's actual bones, not arbitrary expressions.
bool nativeArticleAnimation_Live(const HSD_Joint*, HSD_JObj*,
    const struct NativeScriptItemProgram*, const NativeArticleAnimationOrigin*,
    uint32_t origin_count, HSD_GObj* attachment_owner);

// Conservative preflight for wholly new allocations; no credit for controllers
// that a subsequent Add removes. Checks actual next free slots and generations,
// without walking unrelated occupied pool slots. Does not reserve those slots.
bool nativeArticleAnimation_CanAllocate(uint32_t controllers, uint32_t tracks);

#endif
