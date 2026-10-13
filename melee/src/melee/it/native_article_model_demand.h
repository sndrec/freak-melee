#ifndef MELEE_NATIVE_ARTICLE_MODEL_DEMAND_H
#define MELEE_NATIVE_ARTICLE_MODEL_DEMAND_H

#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_hsd_reservation.h>

enum NativeArticleModelUncovered {
    NativeArticleModel_TevCompiler = 1u << 0,
    NativeArticleModel_Shape = 1u << 1,
    NativeArticleModel_Envelope = 1u << 2,
    NativeArticleModel_References = 1u << 3,
    NativeArticleModel_Instance = 1u << 4,
    NativeArticleModel_ParticleOrSpline = 1u << 5,
    NativeArticleModel_CustomClass = 1u << 6,
    // MObjLoad adds RENDER_TOON; actual compile can splice global toon/shadow
    // textures. Descriptor-only census must not invent an empty environment.
    NativeArticleModel_MaterialEnvironment = 1u << 7,
};

typedef struct NativeArticleModelDemand {
    NativeHsdDemand construction[NATIVE_HSD_RESERVATION_BINS];
    uint32_t construction_count;
    // Potential lazy matrix scale allocation, not performed by JObjLoad.
    NativeHsdDemand scale_vectors;
    uint32_t uncovered;
    uint32_t joints, drawables, polygons, materials, textures, references;
    // Conservative total allocation calls, not optimized live-stage counts.
    uint32_t tev_expressions, tev_constants, tev_descriptors;
} NativeArticleModelDemand;

// Explicit transition census over already imported/trusted descriptors. No
// allocation and no native graph construction. False preserves output.
// True reports bounded demand; uncovered!=0 MUST prohibit graph admission.
// Drawable descriptors must satisfy native texture/vertex-list prerequisites.
// This trusts imported pointer extents; DAT geometry/image byte validation is
// still required at import. Zero texture scale and fully culled polygons retain
// their native behavior. This does not prove a renderer/environment is active.
// Animation additions are not included in this construction-only census.
bool nativeArticleModel_Census(const HSD_Joint*, NativeArticleModelDemand*);

// No initialization/allocation or global texture changes. A census alone cannot
// establish the effective loader methods or the material compiler environment.
bool nativeArticleModel_EnvironmentReady(const NativeArticleModelDemand*);

// Explicit transition: compile every material through the real checked native
// compiler in a prebound reservation. No JObj/ID registration or live slot
// mutation. False preserves output. Success closes MaterialEnvironment only;
// descriptor bytes must stay immutable/pinned and EnvironmentReady must be
// rechecked before subsequent native construction. Lazy Vec demand remains
// separate and must be claimed at actual matrix traversal.
bool nativeArticleModel_PrepareStatic(const HSD_Joint*, NativeHsdReservation*,
                                     NativeArticleModelDemand*);

#endif
