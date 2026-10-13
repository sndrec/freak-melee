#include "native_fighter_pose_editor.h"

#ifdef MELEE_NATIVE

#include "native_fighter_animation_editor.h"
#include <melee/gm/native_content_store.h>
#include <sysdolphin/baselib/jobj.h>

#include <math.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static bool transform_channel(unsigned channel)
{
    return channel >= HSD_A_J_ROTX && channel <= HSD_A_J_SCAZ &&
           channel != HSD_A_J_PATH;
}

bool nativeFighterPose_Capture(const NativeFighterProject* project,
                               NativeFighterAnimationClip clip, float frame,
                               NativeFighterPose* pose)
{
    NativeFighterAnimationInfo info;
    if (pose == NULL || !isfinite(frame) || frame < 0 ||
        !nativeFighterAnimation_Info(project, clip, &info) ||
        frame > info.frames || info.joints > NATIVE_FIGHTER_POSE_JOINTS ||
        info.tracks == 0 || info.tracks > 4096) return false;
    NativeFighterAnimationTrack* tracks = malloc(info.tracks * sizeof(*tracks));
    if (tracks == NULL) return false;
    size_t count;
    bool ok = nativeFighterAnimation_Tracks(project, clip, tracks,
                                            info.tracks, &count) &&
              count == info.tracks;
    NativeFighterPose captured = { 0 };
    captured.source_clip = clip;
    captured.source_frame = frame;
    captured.root_joint = NATIVE_FIGHTER_POSE_JOINTS;
    for (size_t i = 0; ok && i < count; ++i) {
        const unsigned joint = tracks[i].joint;
        const unsigned channel = tracks[i].channel;
        if (!transform_channel(channel) ||
            channel >= NATIVE_FIGHTER_POSE_CHANNELS ||
            joint >= NATIVE_FIGHTER_POSE_JOINTS) continue;
        float value;
        ok = nativeFighterAnimation_Sample(project, clip, (unsigned) i,
                                            frame, &value);
        if (!ok) break;
        captured.present[joint][channel] = 1;
        captured.values[joint][channel] = value;
        ++captured.sampled_channels;
        if (channel >= HSD_A_J_TRAX && channel <= HSD_A_J_TRAZ &&
            joint < captured.root_joint) captured.root_joint = joint;
    }
    free(tracks);
    if (ok && captured.sampled_channels != 0) *pose = captured;
    return ok && captured.sampled_channels != 0;
}

bool nativeFighterPose_Paste(NativeFighterProject* project,
                             const NativeFighterPose* pose, NativeFighterAnimationClip clip,
                             int frame, const unsigned char* selected_joints,
                             bool selected_only, bool include_root_translation,
                             unsigned* written_channels,
                             unsigned* skipped_channels,
                             unsigned* failed_track)
{
    NativeFighterAnimationInfo info;
    if (project == NULL || pose == NULL || written_channels == NULL ||
        skipped_channels == NULL || failed_track == NULL ||
        (selected_only && selected_joints == NULL) ||
        frame < 0 || !nativeFighterAnimation_Info(project, clip, &info) ||
        frame > info.frames || info.joints > NATIVE_FIGHTER_POSE_JOINTS ||
        info.tracks == 0 || info.tracks > 4096) return false;
    NativeFighterAnimationTrack* tracks = malloc(info.tracks * sizeof(*tracks));
    if (tracks == NULL) return false;
    size_t count;
    bool ok = nativeFighterAnimation_Tracks(project, clip, tracks,
                                            info.tracks, &count) &&
              count == info.tracks;
    if (!ok) { free(tracks); return false; }
    NativeFighterProject working = *project;
    working.animation_dat = malloc(project->animation_size);
    working.fighter_dat = malloc(project->fighter_size);
    if (working.animation_dat == NULL || working.fighter_dat == NULL) {
        free(working.animation_dat);
        free(working.fighter_dat);
        free(tracks);
        return false;
    }
    memcpy(working.animation_dat, project->animation_dat,
           project->animation_size);
    memcpy(working.fighter_dat, project->fighter_dat, project->fighter_size);
    unsigned written = 0, skipped = 0;
    *failed_track = UINT_MAX;
    unsigned char matched[NATIVE_FIGHTER_POSE_JOINTS]
                         [NATIVE_FIGHTER_POSE_CHANNELS] = { 0 };
    for (size_t i = 0; ok && i < count; ++i) {
        unsigned joint = tracks[i].joint;
        unsigned channel = tracks[i].channel;
        if (joint >= NATIVE_FIGHTER_POSE_JOINTS ||
            channel >= NATIVE_FIGHTER_POSE_CHANNELS ||
            !pose->present[joint][channel] ||
            (selected_only && !selected_joints[joint]) ||
            (!include_root_translation && joint == pose->root_joint &&
             channel >= HSD_A_J_TRAX && channel <= HSD_A_J_TRAZ)) continue;
        float snapped;
        ok = nativeFighterAnimation_SnapValue(&working, clip, (unsigned) i,
            pose->values[joint][channel], &snapped) &&
             nativeFighterAnimation_InsertKey(&working, clip, (unsigned) i,
                                               frame, snapped);
        if (ok) {
            matched[joint][channel] = 1;
            ++written;
        } else *failed_track = (unsigned) i;
    }
    for (unsigned joint = 0; joint < NATIVE_FIGHTER_POSE_JOINTS; ++joint) {
        if (selected_only && !selected_joints[joint]) continue;
        for (unsigned channel = 0; channel < NATIVE_FIGHTER_POSE_CHANNELS;
             ++channel) {
            if (pose->present[joint][channel] && !matched[joint][channel] &&
                (include_root_translation || joint != pose->root_joint ||
                 channel < HSD_A_J_TRAX || channel > HSD_A_J_TRAZ))
                ++skipped;
        }
    }
    free(tracks);
    if (ok && written != 0 &&
        working.animation_size <= NATIVE_CONTENT_MAX_PACKAGE_BYTES) {
        memcpy(project->fighter_dat, working.fighter_dat, project->fighter_size);
        free(working.fighter_dat);
        free(project->animation_dat);
        project->animation_dat = working.animation_dat;
        project->animation_size = working.animation_size;
        *written_channels = written;
        *skipped_channels = skipped;
        return true;
    }
    free(working.animation_dat);
    free(working.fighter_dat);
    return false;
}

#endif
