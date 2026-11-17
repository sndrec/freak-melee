#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_TRANSFORM_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_TRANSFORM_H

#include <cstdint>

#include <melee/gm/native_home.h>

namespace Rml {
    class ElementDocument;
}

struct NativeWorkshopTransformChange {
    unsigned joint;
    NativeHomeWorkshopBoneLocal before, after;
};
using NativeWorkshopTransformCommit =
    bool (*)(const NativeWorkshopTransformChange*, unsigned);

void nativeWorkshopTransform_Init(Rml::ElementDocument* document);
// Call after evaluating the base pose. Selection order must be stable.
void nativeWorkshopTransform_Update(const char* viewport,
                                    const unsigned* joints, unsigned count,
                                    unsigned active, uint64_t revision,
                                    NativeWorkshopTransformCommit commit);
void nativeWorkshopTransform_Clear();
bool nativeWorkshopTransform_IsActive();

#endif
