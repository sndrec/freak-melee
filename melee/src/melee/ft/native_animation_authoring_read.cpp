#include <string>

#include "native_animation_authoring_internal.h"

namespace NativeAnimationAuthoring {
    const char* channel(unsigned id)
    {
        static const char* names[] = {
            "unknown", "rotation_x", "rotation_y", "rotation_z",
            "path",    "position_x", "position_y", "position_z",
            "scale_x", "scale_y",    "scale_z"
        };
        return id < std::size(names) ? names[id] : "unknown";
    }
    unsigned channel(const char* name)
    {
        for (unsigned i = 1; i <= 10; ++i) {
            if (i != 4 && !std::strcmp(name, channel(i))) {
                return i;
            }
        }
        return 0;
    }
    bool Clip::open(const NativeFighterProject& p, yyjson_val* value, Reply& r)
    {
        if (!yyjson_is_obj(value)) {
            r.fail("missing_clip",
                   "Choose a clip returned by the clips view.");
            return false;
        }
        ref = reference(value);
        if (!nativeFighterAnimation_Info(&p, ref, &info) ||
            info.tracks > 4096 || info.joints > 512)
        {
            r.fail("clip_unavailable",
                   "This clip is absent from the owned animation bank or "
                   "exceeds its supported limits.");
            return false;
        }
        tracks.resize(info.tracks);
        size_t count = 0;
        if (!nativeFighterAnimation_Tracks(&p, ref, tracks.data(),
                                           tracks.size(), &count) ||
            count != tracks.size())
        {
            r.fail("invalid_tracks", "Could not decode the clip's tracks.");
            return false;
        }
        return true;
    }
    bool keys(const NativeFighterProject& p, NativeFighterAnimationClip clip,
              unsigned track, std::vector<NativeFighterAnimationKey>& out,
              Reply& r)
    {
        size_t count = 0;
        if (!nativeFighterAnimation_Keys(&p, clip, track, nullptr, 0,
                                         &count) ||
            count > 65536)
        {
            r.fail("invalid_keys", "Could not decode the selected track "
                                   "within the 65536-key limit.");
            return false;
        }
        out.resize(count);
        if (count && !nativeFighterAnimation_Keys(&p, clip, track, out.data(),
                                                  count, &count))
        {
            r.fail("invalid_keys", "Could not decode the selected track.");
            return false;
        }
        return true;
    }
    bool Rig::open(const NativeFighterProject& p,
                   const NativeFighterAnimationInfo* info, Reply& r)
    {
        model = std::make_unique<NativeFighterModelEditor>();
        if (!nativeFighterModel_Open(model.get(),
                                     const_cast<NativeFighterProject*>(&p)))
        {
            r.fail("rig_unavailable",
                   "Could not read the fighter's costume skeleton.");
            return false;
        }
        bone_to_node.assign(model->joint_count, -1);
        if (!info) {
            return true;
        }
        node_to_bone.assign(info->joints, -1);
        if ((info->binding_flags & 63u) != p.facts.animation_tag) {
            return true;
        }
        unsigned bone = 0, node = 0;
        for (unsigned part = 0; part < p.facts.joints + p.facts.omission_count;
             ++part)
        {
            int omitted = -1;
            for (unsigned i = 0; i < p.facts.omission_count; ++i) {
                if (p.facts.omissions[i].joint == part) {
                    omitted = (int) i;
                }
            }
            const bool included =
                omitted < 0 || ((info->binding_flags >> 9) & (1u << omitted));
            if (included) {
                if (node >= info->joints) {
                    return true;
                }
                if (omitted < 0 && bone < model->joint_count) {
                    node_to_bone[node] = (int) bone;
                    bone_to_node[bone] = (int) node;
                }
                ++node;
            }
            if (omitted < 0) {
                ++bone;
            }
        }
        bound = node == info->joints && bone == model->joint_count;
        if (!bound) {
            std::fill(node_to_bone.begin(), node_to_bone.end(), -1);
            std::fill(bone_to_node.begin(), bone_to_node.end(), -1);
        }
        return true;
    }
    void describe(Reply& r, const Clip& clip)
    {
        reference(r, r.result, "clip", clip.ref);
        add_string(r, r.result, "name", clip.info.name);
        yyjson_mut_obj_add_real(r.document, r.result, "frames",
                                clip.info.frames);
        yyjson_mut_obj_add_uint(r.document, r.result, "animation_joints",
                                clip.info.joints);
        yyjson_mut_obj_add_uint(r.document, r.result, "track_count",
                                clip.info.tracks);
        yyjson_mut_obj_add_uint(r.document, r.result, "binding_flags",
                                clip.info.binding_flags);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_tracks", 4096);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_keys_per_track",
                                65536);
        yyjson_mut_obj_add_uint(r.document, r.result, "maximum_selected_keys",
                                32768);
        yyjson_mut_obj_add_uint(r.document, r.result,
                                "maximum_pose_copy_joints",
                                NATIVE_FIGHTER_POSE_JOINTS);
    }
    static bool matches(const std::string& name, const char* query)
    {
        std::string a = name, b = query;
        for (char& c : a) {
            if (c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
        }
        for (char& c : b) {
            if (c >= 'A' && c <= 'Z') {
                c += 'a' - 'A';
            }
        }
        return a.find(b) != std::string::npos;
    }
    static void vector(Reply& r, yyjson_mut_val* row, const char* name,
                       const float* values)
    {
        auto* array = yyjson_mut_arr(r.document);
        yyjson_mut_obj_add_val(r.document, row, name, array);
        for (unsigned i = 0; i < 3; ++i) {
            yyjson_mut_arr_add_real(r.document, array, values[i]);
        }
    }
    void read(const NativeFighterProject& p, yyjson_val* args, Reply& r)
    {
        const char* view = text(args, "view");
        unsigned offset = number(args, "offset"),
                 limit = has(args, "limit") ? number(args, "limit") : 50;
        if (!std::strcmp(view, "clips")) {
            std::vector<NativeFighterActionInfo> actions(p.action_count);
            size_t count = 0;
            if (!nativeFighterAction_List(&p, actions.data(), actions.size(),
                                          &count))
            {
                r.fail("invalid_actions",
                       "Could not read the clip action table.");
                return;
            }
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result, "clips", rows);
            size_t total = 0;
            for (unsigned bank = 0; bank < 2; ++bank) {
                for (unsigned i = 0;
                     i < (bank ? p.demo_count : p.action_count); ++i)
                {
                    NativeFighterAnimationClip ref{
                        bank ? NativeFighterAnimation_Demo
                             : NativeFighterAnimation_Action,
                        i
                    };
                    NativeFighterAnimationInfo info{};
                    const bool available =
                        nativeFighterAnimation_Info(&p, ref, &info);
                    std::string name = available ? info.name
                                       : bank    ? "Demo " + std::to_string(i)
                                                 : actions[i].name;
                    const char* label =
                        !bank ? nativeFighterAuthoring_Name(
                                    &p, NativeAuthoring_Action, i)
                              : nullptr;
                    if (!matches(name, text(args, "query")) &&
                        !(label && matches(label, text(args, "query"))))
                    {
                        continue;
                    }
                    if (total++ < offset || yyjson_mut_arr_size(rows) >= limit)
                    {
                        continue;
                    }
                    auto* row = yyjson_mut_obj(r.document);
                    yyjson_mut_arr_append(rows, row);
                    reference(r, row, "clip", ref);
                    add_string(r, row, "name", name);
                    if (label) {
                        add_string(r, row, "label", label);
                    }
                    yyjson_mut_obj_add_bool(r.document, row, "available",
                                            available);
                    if (available) {
                        yyjson_mut_obj_add_real(r.document, row, "frames",
                                                info.frames);
                        yyjson_mut_obj_add_uint(r.document, row, "tracks",
                                                info.tracks);
                    }
                }
            }
            page(r, total, offset, limit);
            return;
        }
        Clip clip;
        const bool bones = !std::strcmp(view, "bones");
        if ((!bones || has(args, "clip")) &&
            !clip.open(p, yyjson_obj_get(args, "clip"), r))
        {
            return;
        }
        if (!bones || has(args, "clip")) {
            describe(r, clip);
        }
        if (!std::strcmp(view, "info")) {
            return;
        }
        Rig rig;
        if ((bones || !std::strcmp(view, "tracks") ||
             !std::strcmp(view, "pose")) &&
            !rig.open(p, !bones || has(args, "clip") ? &clip.info : nullptr,
                      r))
        {
            return;
        }
        if (bones || !std::strcmp(view, "pose")) {
            const bool pose = !bones;
            if (pose && !rig.bound) {
                r.fail("foreign_clip_binding",
                       "This clip uses a foreign or incompatible rig. Inspect "
                       "its track samples; bone-local posing requires a clip "
                       "bound to this fighter's rig.");
                return;
            }
            const double frame = numeric(args, "frame");
            if (pose && frame > clip.info.frames) {
                r.fail("invalid_frame", "Choose a frame inside this clip.");
                return;
            }
            struct Pose {
                float position[3], rotation[3], scale[3];
            };
            std::vector<Pose> poses(rig.model->joint_count);
            for (unsigned i = 0; i < rig.model->joint_count; ++i) {
                const auto& rest = rig.model->joints[i];
                std::copy_n(rest.position, 3, poses[i].position);
                std::copy_n(rest.rotation, 3, poses[i].rotation);
                std::copy_n(rest.scale, 3, poses[i].scale);
            }
            if (pose) {
                for (size_t i = 0; i < clip.tracks.size(); ++i) {
                    const auto& track = clip.tracks[i];
                    if (track.joint >= rig.node_to_bone.size() ||
                        rig.node_to_bone[track.joint] < 0 ||
                        track.channel < 1 || track.channel > 10 ||
                        track.channel == 4)
                    {
                        continue;
                    }
                    float value;
                    if (!nativeFighterAnimation_Sample(
                            &p, clip.ref, (unsigned) i, (float) frame, &value))
                    {
                        continue;
                    }
                    auto& target = poses[rig.node_to_bone[track.joint]];
                    (track.channel <= 3 ? target.rotation
                     : track.channel <= 7
                         ? target.position
                         : target.scale)[track.channel <= 3 ? track.channel - 1
                                         : track.channel <= 7
                                             ? track.channel - 5
                                             : track.channel - 8] = value;
                }
            }
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result,
                                   pose ? "pose" : "bones", rows);
            size_t total = 0;
            for (unsigned i = 0; i < rig.model->joint_count; ++i) {
                const auto& bone = rig.model->joints[i];
                if (!matches(bone.name, text(args, "query"))) {
                    continue;
                }
                if (total++ < offset || yyjson_mut_arr_size(rows) >= limit) {
                    continue;
                }
                auto* row = yyjson_mut_obj(r.document);
                yyjson_mut_arr_append(rows, row);
                yyjson_mut_obj_add_uint(r.document, row, "id", i);
                add_string(r, row, "name", bone.name);
                if (bone.parent < 0) {
                    yyjson_mut_obj_add_null(r.document, row, "parent");
                } else {
                    yyjson_mut_obj_add_int(r.document, row, "parent",
                                           bone.parent);
                }
                yyjson_mut_obj_add_uint(r.document, row, "depth", bone.depth);
                const char* role = nativeFighterAuthoring_BoneRole(&p, i);
                if (role) {
                    add_string(r, row, "role", role);
                }
                if (rig.bound && rig.bone_to_node[i] >= 0) {
                    yyjson_mut_obj_add_int(r.document, row, "animation_joint",
                                           rig.bone_to_node[i]);
                } else {
                    yyjson_mut_obj_add_null(r.document, row,
                                            "animation_joint");
                }
                vector(r, row, "position", poses[i].position);
                vector(r, row, "rotation", poses[i].rotation);
                vector(r, row, "scale", poses[i].scale);
            }
            yyjson_mut_obj_add_bool(r.document, r.result, "rig_bound",
                                    rig.bound);
            if (pose) {
                yyjson_mut_obj_add_real(r.document, r.result, "frame", frame);
                add_string(r, r.result, "space", "bone_local");
            }
            page(r, total, offset, limit);
            return;
        }
        if (!std::strcmp(view, "tracks")) {
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result, "tracks", rows);
            size_t total = 0;
            for (size_t i = 0; i < clip.tracks.size(); ++i) {
                const auto& track = clip.tracks[i];
                if (!matches(channel(track.channel), text(args, "query"))) {
                    continue;
                }
                if (total++ < offset || yyjson_mut_arr_size(rows) >= limit) {
                    continue;
                }
                auto* row = yyjson_mut_obj(r.document);
                yyjson_mut_arr_append(rows, row);
                yyjson_mut_obj_add_uint(r.document, row, "id", i);
                yyjson_mut_obj_add_uint(r.document, row, "joint", track.joint);
                add_string(r, row, "channel", channel(track.channel));
                add_string(r, row, "unit", unit(track.channel));
                yyjson_mut_obj_add_uint(r.document, row, "keys", track.keys);
                yyjson_mut_obj_add_int(r.document, row, "start_frame",
                                       track.start_frame);
                if (rig.bound && track.joint < rig.node_to_bone.size() &&
                    rig.node_to_bone[track.joint] >= 0)
                {
                    const unsigned bone = rig.node_to_bone[track.joint];
                    yyjson_mut_obj_add_uint(r.document, row, "bone", bone);
                    add_string(r, row, "bone_name",
                               rig.model->joints[bone].name);
                } else {
                    yyjson_mut_obj_add_null(r.document, row, "bone");
                }
            }
            page(r, total, offset, limit);
            return;
        }
        const unsigned track = number(args, "track");
        if (!has(args, "track") || track >= clip.tracks.size()) {
            r.fail("unknown_track",
                   "Choose a track ID returned by the tracks view.");
            return;
        }
        if (!std::strcmp(view, "samples")) {
            std::vector<float> frames, values;
            size_t i, n;
            yyjson_val* value;
            yyjson_arr_foreach(yyjson_obj_get(args, "frames"), i, n, value)
                frames.push_back((float) yyjson_get_num(value));
            if (frames.empty()) {
                r.fail("missing_frames", "Supply sorted frames to sample.");
                return;
            }
            values.resize(frames.size());
            if (!nativeFighterAnimation_SampleFrames(
                    &p, clip.ref, track, frames.data(), values.data(),
                    frames.size()))
            {
                r.fail("sample_failed", "Use sorted frames inside this clip "
                                        "on a transform track.");
                return;
            }
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result, "samples", rows);
            for (i = 0; i < frames.size(); ++i) {
                auto* row = yyjson_mut_obj(r.document);
                yyjson_mut_arr_append(rows, row);
                yyjson_mut_obj_add_real(r.document, row, "frame", frames[i]);
                if (std::isfinite(values[i])) {
                    yyjson_mut_obj_add_real(r.document, row, "value",
                                            values[i]);
                } else {
                    yyjson_mut_obj_add_null(r.document, row, "value");
                }
            }
            return;
        }
        std::vector<NativeFighterAnimationKey> data;
        if (!keys(p, clip.ref, track, data, r)) {
            return;
        }
        static const char* interpolation[] = { "unknown", "constant",
                                               "linear",  "spline_zero",
                                               "spline",  "slope",
                                               "key" };
        auto* rows = yyjson_mut_arr(r.document);
        yyjson_mut_obj_add_val(r.document, r.result, "keys", rows);
        for (size_t i = offset;
             i < data.size() && yyjson_mut_arr_size(rows) < limit; ++i)
        {
            const auto& key = data[i];
            auto* row = yyjson_mut_obj(r.document);
            yyjson_mut_arr_append(rows, row);
            yyjson_mut_obj_add_uint(r.document, row, "id", i);
            yyjson_mut_obj_add_int(r.document, row, "frame", key.frame);
            yyjson_mut_obj_add_real(r.document, row, "value", key.value);
            yyjson_mut_obj_add_real(r.document, row, "tangent", key.tangent);
            add_string(r, row, "interpolation",
                       key.interpolation < std::size(interpolation)
                           ? interpolation[key.interpolation]
                           : "unknown");
            yyjson_mut_obj_add_bool(r.document, row, "value_editable",
                                    key.editable);
            yyjson_mut_obj_add_bool(r.document, row, "tangent_editable",
                                    key.tangent_editable);
        }
        page(r, data.size(), offset, limit);
    }
}
