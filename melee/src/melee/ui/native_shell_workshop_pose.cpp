#include "native_shell_workshop_pose.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "native_shell_workshop_transform.h"
#include <melee/ft/native_fighter_animation_editor.h>
#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_fighter_project.h>
#include <RmlUi/Core.h>

namespace {
    constexpr unsigned MaxBones = 512;
    Rml::ElementDocument* document;
    NativeFighterProject* project;
    NativeWorkshopPoseRecord record;
    NativeFighterAnimationClip clip;
    unsigned selected[MaxBones], count;
    int frame;
    bool auto_key, changed, staged[MaxBones];
    NativeHomeWorkshopBoneLocal poses[MaxBones], base[MaxBones];

    void status(const char* text)
    {
        document->GetElementById("workshop-editor-status")->SetInnerRML(text);
    }

    bool key_poses(const NativeWorkshopTransformChange* changes,
                   unsigned change_count, bool changed_only)
    {
        if (!project || !record) {
            return false;
        }
        std::vector<uint8_t> before(project->animation_dat,
                                    project->animation_dat +
                                        project->animation_size);
        std::vector<uint8_t> before_fighter(project->fighter_dat,
                                            project->fighter_dat + project->fighter_size);
        auto* restore = (uint8_t*) std::malloc(before.size());
        if (!restore) {
            return false;
        }
        std::memcpy(restore, before.data(), before.size());
        bool edited = false, valid = true;
        const unsigned channels[] = { 5, 6, 7, 1, 2, 3, 8, 9, 10 };
        // The practice fighter has display transforms (notably root scale and
        // facing) which are not authored animation values. Apply the user's
        // local delta to the source clip rather than keying those display
        // values.
        float source_values[MaxBones][9];
        NativeFighterModelEditor* model =
            (NativeFighterModelEditor*) std::malloc(sizeof(*model));
        if (!model || !nativeFighterModel_Open(model, project)) {
            std::free(model);
            std::free(restore);
            return false;
        }
        NativeFighterAnimationTrack tracks[1024];
        size_t track_count = 0;
        nativeFighterAnimation_Tracks(project, clip, tracks, 1024,
                                      &track_count);
        for (unsigned i = 0; i < change_count; ++i) {
            unsigned joint = changes[i].joint;
            if (joint >= model->joint_count) {
                std::free(model);
                std::free(restore);
                return false;
            }
            const auto& rest = model->joints[joint];
            for (unsigned c = 0; c < 9; ++c) {
                float value = c < 3   ? rest.position[c]
                              : c < 6 ? rest.rotation[c - 3]
                                      : rest.scale[c - 6];
                for (size_t t = 0; t < track_count; ++t) {
                    if (tracks[t].joint == joint &&
                        tracks[t].channel == channels[c])
                    {
                        nativeFighterAnimation_Sample(project, clip,
                                                      (unsigned) t,
                                                      (float) frame, &value);
                        break;
                    }
                }
                source_values[i][c] = value;
            }
        }
        std::free(model);
        for (unsigned i = 0; i < change_count && valid; ++i) {
            const auto& change = changes[i];
            for (unsigned channel = 0; channel < 9; ++channel) {
                const float* old = channel < 3   ? change.before.position
                                   : channel < 6 ? change.before.rotation
                                                 : change.before.scale;
                const float* next = channel < 3   ? change.after.position
                                    : channel < 6 ? change.after.rotation
                                                  : change.after.scale;
                unsigned axis = channel % 3;
                if (changed_only && std::fabs(next[axis] - old[axis]) < 1e-6f)
                {
                    continue;
                }
                const float source = source_values[i][channel];
                const float value =
                    channel >= 6 && std::fabs(old[axis]) > 1e-8f
                        ? source * (next[axis] / old[axis])
                        : source + next[axis] - old[axis];
                if (!nativeFighterAnimation_KeyChannel(
                        project, clip, change.joint, channels[channel],
                        frame, value, source))
                {
                    valid = false;
                    break;
                }
                edited = true;
            }
        }
        if (!valid) {
            std::memcpy(project->fighter_dat, before_fighter.data(), before_fighter.size());
            std::free(project->animation_dat);
            project->animation_dat = restore;
            project->animation_size = before.size();
            status("Could not encode this pose. The temporary pose is "
                   "retained; no keys were changed.");
            return false;
        }
        std::free(restore);
        if (edited) {
            record(before.data(), before.size(), before_fighter.data(), before_fighter.size());
            for (unsigned i = 0; i < change_count; ++i) {
                staged[changes[i].joint] = false;
            }
            nativeHome_WorkshopPoseInvalidate();
            status("Pose keyed at the current frame. Save the draft to keep "
                   "the animation edit.");
        } else {
            status("No editable animation channels are selected.");
        }
        return edited;
    }
    bool commit_pose(const NativeWorkshopTransformChange* edits, unsigned n)
    {
        changed = true;
        for (unsigned i = 0; i < n; ++i) {
            unsigned joint = edits[i].joint;
            if (!staged[joint]) {
                base[joint] = edits[i].before;
            }
            poses[joint] = edits[i].after;
            staged[joint] = true;
        }
        if (auto_key) {
            NativeWorkshopTransformChange combined[MaxBones];
            for (unsigned i = 0; i < n; ++i) {
                combined[i] = edits[i];
                combined[i].before = base[edits[i].joint];
            }
            key_poses(combined, n, true);
        } else {
            status("Temporary pose. Insert Key to keep it; changing frames "
                   "discards unkeyed poses.");
        }
        return true;
    }
}

void nativeWorkshopPose_Init(Rml::ElementDocument* doc)
{
    document = doc;
}
void nativeWorkshopPose_Clear()
{
    if (!project) {
        return;
    }
    nativeWorkshopTransform_Clear();
    if (project) {
        nativeHome_WorkshopPoseInvalidate();
    }
    project = nullptr;
    count = 0;
    std::memset(staged, 0, sizeof(staged));
}
void nativeWorkshopPose_Begin(NativeFighterProject* next, NativeFighterAnimationClip next_clip,
                              int next_frame)
{
    if (project != next || clip.bank != next_clip.bank ||
        clip.index != next_clip.index || frame != next_frame) {
        nativeWorkshopPose_Clear();
        project = next;
        clip = next_clip;
        frame = next_frame;
    }
}
void nativeWorkshopPose_Update(const unsigned* joints, unsigned n,
                               unsigned active, bool automatic,
                               NativeWorkshopPoseRecord recorder)
{
    if (!project) {
        return;
    }
    count = std::min(n, MaxBones);
    std::memcpy(selected, joints, count * sizeof(unsigned));
    auto_key = automatic;
    record = recorder;
    for (unsigned i = 0; i < MaxBones; ++i) {
        if (staged[i]) {
            nativeHome_WorkshopSetBoneLocal(i, &poses[i]);
        }
    }
    nativeWorkshopTransform_Update(
        "workshop-animation-viewport", selected, count, active,
        ((uint64_t) clip.bank << 63) | ((uint64_t) clip.index << 32) |
            (unsigned) frame, commit_pose);
}
bool nativeWorkshopPose_Stage(unsigned joint,
                              const NativeHomeWorkshopBoneLocal& pose)
{
    if (!project || joint >= MaxBones) {
        return false;
    }
    NativeWorkshopTransformChange change = {};
    change.joint = joint;
    change.after = pose;
    if (!nativeWorkshopPose_Local(joint, &change.before)) {
        return false;
    }
    nativeHome_WorkshopSetBoneLocal(joint, &pose);
    return commit_pose(&change, 1);
}
bool nativeWorkshopPose_Local(unsigned joint,
                              NativeHomeWorkshopBoneLocal* pose)
{
    if (joint >= MaxBones) {
        return false;
    }
    if (staged[joint]) {
        *pose = poses[joint];
        return true;
    }
    return nativeHome_WorkshopBoneLocal(joint, pose);
}
bool nativeWorkshopPose_ConsumeChanged()
{
    bool result = changed;
    changed = false;
    return result;
}
bool nativeWorkshopPose_InsertKeys()
{
    if (!project || !count) {
        return false;
    }
    NativeWorkshopTransformChange changes[MaxBones];
    unsigned n = 0;
    // Sample the underlying animation for the before pose so missing channels
    // are distinguishable from channels whose temporary value actually
    // changed.
    for (unsigned i = 0; i < count; ++i) {
        auto& change = changes[n];
        change.joint = selected[i];
        if (!nativeWorkshopPose_Local(selected[i], &change.after)) {
            continue;
        }
        change.before = staged[selected[i]] ? base[selected[i]] : change.after;
        ++n;
    }
    return key_poses(changes, n, false);
}
