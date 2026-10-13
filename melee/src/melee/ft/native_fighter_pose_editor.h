#ifndef MELEE_FT_NATIVE_FIGHTER_POSE_EDITOR_H
#define MELEE_FT_NATIVE_FIGHTER_POSE_EDITOR_H

#ifdef MELEE_NATIVE

#include "native_fighter_animation_editor.h"

#ifdef __cplusplus
extern "C" {
#endif

enum { NATIVE_FIGHTER_POSE_JOINTS = 128, NATIVE_FIGHTER_POSE_CHANNELS = 11 };

typedef struct NativeFighterPose {
    NativeFighterAnimationClip source_clip;
    float source_frame;
    unsigned root_joint;
    unsigned sampled_channels;
    unsigned char present[NATIVE_FIGHTER_POSE_JOINTS]
                         [NATIVE_FIGHTER_POSE_CHANNELS];
    float values[NATIVE_FIGHTER_POSE_JOINTS]
                [NATIVE_FIGHTER_POSE_CHANNELS];
} NativeFighterPose;

// Capture evaluated local transform channels from a clip. The pose is a
// snapshot, not a link to the source clip or its original key positions.
bool nativeFighterPose_Capture(const NativeFighterProject* project,
                               NativeFighterAnimationClip clip, float frame,
                               NativeFighterPose* pose);

// Apply matching channels to existing destination tracks as one transaction.
// A missing destination channel is not invented here; skipped_channels tells
// the editor how much of the source pose could not be applied. The caller owns
// the project and must not retain pointers into its animation bank on success.
bool nativeFighterPose_Paste(NativeFighterProject* project,
                             const NativeFighterPose* pose, NativeFighterAnimationClip clip,
                             int frame, const unsigned char* selected_joints,
                             bool selected_only, bool include_root_translation,
                             unsigned* written_channels,
                             unsigned* skipped_channels,
                             unsigned* failed_track);

#ifdef __cplusplus
}
#endif

#endif
#endif
