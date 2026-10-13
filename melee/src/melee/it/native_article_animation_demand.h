#ifndef MELEE_IT_NATIVE_ARTICLE_ANIMATION_DEMAND_H
#define MELEE_IT_NATIVE_ARTICLE_ANIMATION_DEMAND_H

#include <stdbool.h>
#include <stdint.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/jobj.h>

enum NativeArticleAnimationUncovered {
    NativeArticleAnimation_ObjReferences = 1u << 0,
    NativeArticleAnimation_Shape = 1u << 1,
    NativeArticleAnimation_PaletteTables = 1u << 2,
    NativeArticleAnimation_CallbackChannels = 1u << 3,
    NativeArticleAnimation_EulerQuaternion = 1u << 4,
    NativeArticleAnimation_Path = 1u << 5,
    NativeArticleAnimation_Channel = 1u << 6,
    NativeArticleAnimation_Stream = 1u << 7,
    NativeArticleAnimation_Instance = 1u << 8,
    NativeArticleAnimation_ParticleOrSpline = 1u << 9,
};

typedef struct NativeArticleAnimationDemand {
    uint32_t controllers, tracks, uncovered;
} NativeArticleAnimationDemand;

// Transition-only census over trusted, pinned imported descriptor extents.
// Applies native positional matching and first texture-ID selection; shared
// descriptors count once per application, not once per address. No allocation,
// native constructor, global ID lookup, or controller attachment occurs.
// False preserves output (malformed bounds/cycles/startframe or fixed limits).
// A successful count is NOT pool capacity, native channel-value legality, or
// graph admission: uncovered flags must be resolved before runtime attachment.
// Scalar stream coverage checks operand bounds/grammar without rejecting raw
// NaN/Inf payloads. Texture indexing, color conversions and callback semantics
// remain explicit channel coverage gaps. Sparse bundles can accumulate; this
// is one application, not the maximum retained union of all bundles.
bool nativeArticleAnimation_Census(const HSD_Joint*, const HSD_AnimJoint*,
    const HSD_MatAnimJoint*, const HSD_ShapeAnimJoint*,
    NativeArticleAnimationDemand*);

#endif
