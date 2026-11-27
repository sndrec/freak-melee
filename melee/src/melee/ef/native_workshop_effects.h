#ifndef MELEE_EF_NATIVE_WORKSHOP_EFFECTS_H
#define MELEE_EF_NATIVE_WORKSHOP_EFFECTS_H

#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_fighter_animation_editor.h>

enum {
    NativeWorkshopEffect_LinkFirst = 3,
    NativeWorkshopEffect_LinkCount = 3,
    NativeWorkshopEffect_Links = 7 << NativeWorkshopEffect_LinkFirst,
    NativeWorkshopEffect_Mask = NativeWorkshopEffect_Links << 16,
};

// Set only while advancing the isolated preview. A child using an already
// remapped link keeps that link.
extern bool nativeWorkshopEffects_Advancing;
bool nativeWorkshopEffects_Holds(const Fighter_GObj* owner);
void nativeWorkshopEffects_Clear(void);
bool nativeWorkshopEffects_Seek(Fighter_GObj* actor,
    const NativeFighterProject* draft, NativeFighterAnimationClip clip,
    float pose, unsigned frame, const NativeFighterActionOccurrence* events,
    size_t count);

#endif
