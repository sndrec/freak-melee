#ifndef MELEE_UI_NATIVE_SHELL_WORKSHOP_POSE_H
#define MELEE_UI_NATIVE_SHELL_WORKSHOP_POSE_H
#include <cstddef>
#include <cstdint>
#include "native_shell_workshop_transform.h"

#include <melee/gm/native_home.h>
#include <melee/ft/native_fighter_animation_editor.h>
struct NativeFighterProject;
namespace Rml {
    class ElementDocument;
}
using NativeWorkshopPoseRecord = void (*)(const uint8_t*, size_t,
                                         const uint8_t*, size_t);
using NativeWorkshopPoseHistory = void (*)(const NativeWorkshopTransformChange*, unsigned);
void nativeWorkshopPose_SetHistory(NativeWorkshopPoseHistory recorder);
void nativeWorkshopPose_Restore(const NativeWorkshopTransformChange*, unsigned, bool after);
bool nativeWorkshopPose_InsertChannel(unsigned joint, unsigned channel);
void nativeWorkshopPose_Init(Rml::ElementDocument* document);
void nativeWorkshopPose_Clear();
// Reset temporary poses before evaluating a new clip/frame.
void nativeWorkshopPose_Begin(NativeFighterProject* project, NativeFighterAnimationClip clip,
                              int frame);
void nativeWorkshopPose_Update(const unsigned* selected, unsigned count,
                               unsigned active, bool auto_key,
                               NativeWorkshopPoseRecord record);
bool nativeWorkshopPose_Stage(unsigned joint,
                              const NativeHomeWorkshopBoneLocal& pose);
bool nativeWorkshopPose_InsertKeys();
bool nativeWorkshopPose_ConsumeChanged();
bool nativeWorkshopPose_Local(unsigned joint,
                              NativeHomeWorkshopBoneLocal* pose);
#endif
