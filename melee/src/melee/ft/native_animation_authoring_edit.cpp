#include <cstdlib>

#include "native_animation_authoring_internal.h"
#include "native_animation_edits.h"

namespace NativeAnimationAuthoring {
    namespace {
        struct Candidate {
            NativeFighterProject p;
            Candidate(const NativeFighterProject& source) : p(source)
            {
                p.fighter_dat = (uint8_t*) std::malloc(source.fighter_size);
                p.animation_dat =
                    (uint8_t*) std::malloc(source.animation_size);
                if (p.fighter_dat) {
                    std::memcpy(p.fighter_dat, source.fighter_dat,
                                source.fighter_size);
                }
                if (p.animation_dat) {
                    std::memcpy(p.animation_dat, source.animation_dat,
                                source.animation_size);
                }
            }
            ~Candidate()
            {
                std::free(p.fighter_dat);
                std::free(p.animation_dat);
            }
        };
        bool selected(yyjson_val* array, unsigned value)
        {
            size_t i, n;
            yyjson_val* v;
            yyjson_arr_foreach(array, i, n,
                               v) if (yyjson_get_uint(v) == value) return true;
            return false;
        }
        bool gather(const NativeFighterProject& p, const Clip& clip,
                    yyjson_val* args,
                    std::vector<NativeAnimationEdits::Change>& changes,
                    Reply& r)
        {
            auto* list = yyjson_obj_get(args, "keys");
            auto* selection = yyjson_obj_get(args, "selection");
            if (list && selection) {
                r.fail("ambiguous_selection", "Use inspected key references "
                                              "or a bone/channel selection.");
                return false;
            }
            if (list) {
                size_t i, n;
                yyjson_val* v;
                yyjson_arr_foreach(list, i, n, v) changes.push_back(
                    { { number(v, "track"), number(v, "key") },
                      (int) number(v, "frame"),
                      (float) numeric(v, "value"),
                      (float) numeric(v, "tangent"),
                      (has(v, "frame") ? NativeAnimationEdits::Time : 0u) |
                          (has(v, "value") ? NativeAnimationEdits::Value
                                           : 0u) |
                          (has(v, "tangent") ? NativeAnimationEdits::Tangent
                                             : 0u) });
            } else if (selection) {
                Rig rig;
                if (!rig.open(p, &clip.info, r)) {
                    return false;
                }
                if (has(selection, "bones") && !rig.bound) {
                    r.fail("foreign_clip_binding",
                           "Bone selection requires a clip bound to this "
                           "fighter; use inspected track/key pairs for a "
                           "foreign clip.");
                    return false;
                }
                if (numeric(selection, "start_frame") >
                    numeric(selection, "end_frame", 65535))
                {
                    r.fail("invalid_selection",
                           "Selection start must precede its end.");
                    return false;
                }
                for (size_t t = 0; t < clip.tracks.size(); ++t) {
                    const auto& track = clip.tracks[t];
                    if (has(selection, "bones") &&
                        (track.joint >= rig.node_to_bone.size() ||
                         rig.node_to_bone[track.joint] < 0 ||
                         !selected(yyjson_obj_get(selection, "bones"),
                                   rig.node_to_bone[track.joint])))
                    {
                        continue;
                    }
                    if (auto* channels = yyjson_obj_get(selection, "channels"))
                    {
                        bool found = false;
                        size_t i, n;
                        yyjson_val* v;
                        yyjson_arr_foreach(
                            channels, i, n,
                            v) if (!std::strcmp(yyjson_get_str(v),
                                                channel(track.channel)))
                            found = true;
                        if (!found) {
                            continue;
                        }
                    }
                    std::vector<NativeFighterAnimationKey> data;
                    if (!keys(p, clip.ref, (unsigned) t, data, r)) {
                        return false;
                    }
                    for (size_t k = 0; k < data.size(); ++k) {
                        if (data[k].frame >=
                                numeric(selection, "start_frame") &&
                            data[k].frame <=
                                numeric(selection, "end_frame", 65535))
                        {
                            if (changes.size() == 32768) {
                                r.fail(
                                    "selection_limit",
                                    "Select at most 32768 keys in one edit.");
                                return false;
                            }
                            changes.push_back({ { (unsigned) t, (unsigned) k },
                                                data[k].frame,
                                                data[k].value,
                                                data[k].tangent,
                                                0 });
                        }
                    }
                }
            } else {
                r.fail("missing_keys",
                       "Supply keys or a bone/channel selection.");
                return false;
            }
            std::sort(changes.begin(), changes.end(),
                      [](const auto& a, const auto& b) {
                          return a.ref.track < b.ref.track ||
                                 (a.ref.track == b.ref.track &&
                                  a.ref.key < b.ref.key);
                      });
            for (size_t i = 0; i < changes.size(); ++i) {
                if (changes[i].ref.track >= clip.tracks.size() ||
                    (i && changes[i].ref.track == changes[i - 1].ref.track &&
                     changes[i].ref.key == changes[i - 1].ref.key))
                {
                    r.fail("invalid_selection",
                           "Choose distinct inspected track/key pairs.");
                    return false;
                }
            }
            if (changes.empty()) {
                r.fail("empty_selection", "No keys match this selection.");
                return false;
            }
            return true;
        }
    }
    bool edit(NativeFighterProject& project, yyjson_val* args, Reply& r)
    {
        const char* op = text(args, "operation");
        Candidate candidate(project);
        auto& p = candidate.p;
        if (!p.fighter_dat || !p.animation_dat) {
            r.fail("allocation_failed",
                   "Could not stage this animation edit.");
            return false;
        }
        Clip clip;
        if (std::strcmp(op, "create") &&
            !clip.open(p, yyjson_obj_get(args, "clip"), r))
        {
            return false;
        }
        bool ok = false;
        std::string error;
        std::vector<NativeAnimationEdits::Change> applied;
        if (!std::strcmp(op, "create") || !std::strcmp(op, "duplicate")) {
            if (!has(args, "target_action") || !*text(args, "name") ||
                number(args, "target_action") >= p.action_count)
            {
                r.fail(
                    "missing_target",
                    "Choose a target action and an alphanumeric clip name.");
                return false;
            }
            if (!std::strcmp(op, "create")) {
                double frames = numeric(args, "frames");
                if (frames < 1 || frames > 1000 ||
                    frames != std::floor(frames))
                {
                    r.fail("invalid_duration",
                           "Create a clip with 1 to 1000 whole frames.");
                    return false;
                }
                ok = nativeFighterAnimation_CreateRest(
                    &p, number(args, "target_action"), text(args, "name"),
                    (unsigned) frames);
            } else {
                ok = nativeFighterAnimation_Duplicate(
                    &p, clip.ref, number(args, "target_action"),
                    text(args, "name"));
            }
            clip.ref = { NativeFighterAnimation_Action,
                         number(args, "target_action") };
        } else if (!std::strcmp(op, "duration")) {
            if (!has(args, "frames")) {
                r.fail("missing_duration", "Supply the clip's end frame.");
                return false;
            }
            ok = nativeFighterAnimation_SetFrames(
                &p, clip.ref, (float) numeric(args, "frames"));
        } else if (!std::strcmp(op, "insert")) {
            Rig rig;
            if (!rig.open(p, &clip.info, r)) {
                return false;
            }
            if (!rig.bound) {
                r.fail("foreign_clip_binding", "Insert bone keys into a clip "
                                               "bound to this fighter's rig.");
                return false;
            }
            auto* rows = yyjson_obj_get(args, "insertions");
            if (!yyjson_arr_size(rows)) {
                r.fail("missing_keys", "Supply bone/channel insertions.");
                return false;
            }
            size_t i, n;
            yyjson_val* v;
            ok = true;
            for (size_t a = 0; a < yyjson_arr_size(rows); ++a) {
                for (size_t b = 0; b < a; ++b) {
                    auto* x = yyjson_arr_get(rows, a);
                    auto* y = yyjson_arr_get(rows, b);
                    if (number(x, "bone") == number(y, "bone") &&
                        number(x, "frame") == number(y, "frame") &&
                        !std::strcmp(text(x, "channel"), text(y, "channel")))
                    {
                        r.fail(
                            "duplicate_key",
                            "Use one insertion for each bone/channel/frame.");
                        return false;
                    }
                }
            }
            yyjson_arr_foreach(rows, i, n, v)
            {
                const unsigned bone = number(v, "bone"),
                               c = channel(text(v, "channel"));
                if (bone >= rig.model->joint_count ||
                    rig.bone_to_node[bone] < 0 || !c)
                {
                    ok = false;
                    break;
                }
                const auto& rest = rig.model->joints[bone];
                const float base = c <= 3   ? rest.rotation[c - 1]
                                   : c <= 7 ? rest.position[c - 5]
                                            : rest.scale[c - 8];
                if (!nativeFighterAnimation_KeyChannel(
                        &p, clip.ref, (unsigned) rig.bone_to_node[bone], c,
                        (int) number(v, "frame"), (float) numeric(v, "value"),
                        base))
                {
                    ok = false;
                    break;
                }
            }
            yyjson_mut_obj_add_uint(r.document, r.result, "inserted_keys", i);
        } else if (!std::strcmp(op, "copy_pose")) {
            Clip source;
            if (!source.open(p, yyjson_obj_get(args, "source_clip"), r)) {
                return false;
            }
            if (!has(args, "frame") || !has(args, "source_frame")) {
                r.fail("missing_frame",
                       "Supply source_frame and destination frame.");
                return false;
            }
            // The existing Workshop pose buffer addresses animation nodes.
            // Matching rig tags/masks keep those node identities equal between
            // the clips.
            constexpr uint32_t binding_mask = 63u | (0x1fffu << 9);
            if ((source.info.binding_flags & binding_mask) !=
                (clip.info.binding_flags & binding_mask))
            {
                r.fail("pose_binding_mismatch",
                       "Pose copying needs matching source/destination rig "
                       "bindings.");
                return false;
            }
            NativeFighterPose pose{};
            unsigned written = 0, skipped = 0, failed = UINT32_MAX;
            unsigned char selection[NATIVE_FIGHTER_POSE_JOINTS]{};
            const bool selected_only = has(args, "bones");
            if (selected_only) {
                Rig rig;
                if (!rig.open(p, &clip.info, r)) {
                    return false;
                }
                if (!rig.bound) {
                    r.fail("foreign_clip_binding",
                           "Selected-bone pose copying needs a clip bound to "
                           "this fighter.");
                    return false;
                }
                size_t i, n;
                yyjson_val* v;
                yyjson_arr_foreach(yyjson_obj_get(args, "bones"), i, n, v)
                {
                    unsigned bone = (unsigned) yyjson_get_uint(v);
                    if (bone >= rig.bone_to_node.size() ||
                        rig.bone_to_node[bone] < 0 ||
                        rig.bone_to_node[bone] >= NATIVE_FIGHTER_POSE_JOINTS)
                    {
                        r.fail("unknown_bone",
                               "Choose bones inside the pose-copy capacity.");
                        return false;
                    }
                    selection[rig.bone_to_node[bone]] = 1;
                }
            }
            ok = nativeFighterPose_Capture(
                     &p, source.ref, (float) numeric(args, "source_frame"),
                     &pose) &&
                 nativeFighterPose_Paste(
                     &p, &pose, clip.ref, (int) number(args, "frame"),
                     selection, selected_only,
                     yyjson_get_bool(
                         yyjson_obj_get(args, "include_root_translation")),
                     &written, &skipped, &failed);
            yyjson_mut_obj_add_uint(r.document, r.result, "written_channels",
                                    written);
            yyjson_mut_obj_add_uint(r.document, r.result, "skipped_channels",
                                    skipped);
        } else {
            std::vector<NativeAnimationEdits::Change> changes;
            if (!gather(p, clip, args, changes, r)) {
                return false;
            }
            if (std::strcmp(op, "change")) {
                for (const auto& change : changes) {
                    if (change.fields) {
                        r.fail("unexpected_key_fields",
                               "Use only track/key references for deletion or "
                               "transformation.");
                        return false;
                    }
                }
            }
            if (!std::strcmp(op, "change")) {
                if (has(args, "selection")) {
                    r.fail("missing_changes", "Use explicit key references "
                                              "and their changed fields.");
                    return false;
                }
                ok = NativeAnimationEdits::apply(p, clip.ref, changes, applied,
                                                 error);
            } else if (!std::strcmp(op, "delete")) {
                std::vector<NativeFighterAnimationKeyTime> times;
                for (const auto& c : changes) {
                    times.push_back({ c.ref, 0 });
                }
                ok = nativeFighterAnimation_EditKeys(
                    &p, clip.ref, times.data(), times.size(), true);
                yyjson_mut_obj_add_uint(r.document, r.result, "deleted_keys",
                                        changes.size());
            } else if (!std::strcmp(op, "transform")) {
                auto* t = yyjson_obj_get(args, "transform");
                if (!yyjson_is_obj(t)) {
                    r.fail("missing_transform", "Supply the key transform.");
                    return false;
                }
                const double px = numeric(t, "pivot_frame"),
                             py = numeric(t, "pivot_value"),
                             sx = numeric(t, "time_scale", 1),
                             sy = numeric(t, "value_scale", 1);
                const double angle = numeric(t, "rotation"),
                             cs = std::cos(angle), sn = std::sin(angle);
                std::vector<NativeFighterAnimationKey> data;
                std::vector<float> displayed;
                unsigned previous = UINT32_MAX;
                for (auto& c : changes) {
                    if (previous != c.ref.track) {
                        if (!keys(p, clip.ref, c.ref.track, data, r)) {
                            return false;
                        }
                        displayed.assign(data.size(), NAN);
                        std::vector<float> frames, values;
                        std::vector<size_t> indices;
                        for (size_t i = 0; i < data.size(); ++i) {
                            if (data[i].frame >= 0 &&
                                data[i].frame <= clip.info.frames)
                            {
                                frames.push_back((float) data[i].frame);
                                indices.push_back(i);
                            }
                        }
                        values.resize(frames.size());
                        if (!frames.empty() &&
                            nativeFighterAnimation_SampleFrames(
                                &p, clip.ref, c.ref.track, frames.data(),
                                values.data(), frames.size()))
                        {
                            for (size_t i = 0; i < indices.size(); ++i) {
                                displayed[indices[i]] = values[i];
                            }
                        }
                    }
                    previous = c.ref.track;
                    if (c.ref.key >= data.size()) {
                        r.fail("unknown_key",
                               "Choose a key returned by inspection.");
                        return false;
                    }
                    const auto& key = data[c.ref.key];
                    const double ordinate = std::isfinite(displayed[c.ref.key])
                                                ? displayed[c.ref.key]
                                                : key.value;
                    const double x = (key.frame - px) * sx,
                                 y = (ordinate - py) * sy;
                    const double time =
                        px + cs * x - sn * y + numeric(t, "frame_offset");
                    const double value =
                        py + sn * x + cs * y + numeric(t, "value_offset");
                    if (!std::isfinite(time) || time < 0 ||
                        time > clip.info.frames || !std::isfinite(value))
                    {
                        r.fail("transform_range",
                               "The transform places a key outside its clip "
                               "or numeric range.");
                        return false;
                    }
                    c.frame = (int) std::lround(time);
                    c.value = (float) (key.value + value - ordinate);
                    c.tangent = key.tangent;
                    c.fields =
                        NativeAnimationEdits::Time |
                        (key.editable ? NativeAnimationEdits::Value : 0u);
                    if (key.tangent_editable) {
                        const double dx = cs * sx - sn * sy * key.tangent,
                                     dy = sn * sx + cs * sy * key.tangent;
                        if (!std::isfinite(dx) || std::abs(dx) < 1e-12 ||
                            !std::isfinite(dy / dx))
                        {
                            r.fail(
                                "vertical_tangent",
                                "This transform creates a vertical tangent, "
                                "which Melee's curves cannot represent.");
                            return false;
                        }
                        c.tangent = (float) (dy / dx);
                        c.fields |= NativeAnimationEdits::Tangent;
                    }
                }
                ok = NativeAnimationEdits::apply(p, clip.ref, changes, applied,
                                                 error);
            }
        }
        if (!ok) {
            r.fail(
                "animation_edit_rejected",
                error.empty()
                    ? "The edit exceeds a clip, key-order, binding or "
                      "source-encoding limit. The original draft is preserved."
                    : error);
            return false;
        }
        NativeFighterAnimationInfo info;
        if (!nativeFighterAnimation_Info(&p, clip.ref, &info)) {
            r.fail("invalid_result", "The edited clip could not be decoded; "
                                     "the original draft is preserved.");
            return false;
        }
        const bool changed =
            p.fighter_size != project.fighter_size ||
            p.animation_size != project.animation_size ||
            std::memcmp(p.fighter_dat, project.fighter_dat,
                        project.fighter_size) ||
            std::memcmp(p.animation_dat, project.animation_dat,
                        project.animation_size);
        if (changed) {
            std::free(project.fighter_dat);
            std::free(project.animation_dat);
            project.fighter_dat = p.fighter_dat;
            project.fighter_size = p.fighter_size;
            project.animation_dat = p.animation_dat;
            project.animation_size = p.animation_size;
            p.fighter_dat = nullptr;
            p.animation_dat = nullptr;
        }
        reference(r, r.result, "clip", clip.ref);
        yyjson_mut_obj_add_real(r.document, r.result, "frames", info.frames);
        yyjson_mut_obj_add_bool(r.document, r.result, "changed", changed);
        yyjson_mut_obj_add_bool(r.document, r.result, "saved", false);
        yyjson_mut_obj_add_bool(r.document, r.result, "installed", false);
        add_string(r, r.result, "preview_status", "refresh_required");
        yyjson_mut_obj_add_uint(r.document, r.result, "selected_keys",
                                applied.size());
        yyjson_mut_obj_add_uint(
            r.document, r.result, "changed_keys",
            std::count_if(applied.begin(), applied.end(),
                          [](const auto& key) { return key.fields != 0; }));
        if (auto* transform = yyjson_obj_get(args, "transform")) {
            yyjson_mut_obj_add_val(r.document, r.result, "requested_transform",
                                   yyjson_val_mut_copy(r.document, transform));
        }
        if (auto* requested = yyjson_obj_get(args, "keys")) {
            auto* rows = yyjson_mut_arr(r.document);
            yyjson_mut_obj_add_val(r.document, r.result, "requested_keys",
                                   rows);
            for (size_t i = 0; i < yyjson_arr_size(requested) && i < 200; ++i)
            {
                yyjson_mut_arr_append(
                    rows, yyjson_val_mut_copy(r.document,
                                              yyjson_arr_get(requested, i)));
            }
        }
        auto* rows = yyjson_mut_arr(r.document);
        yyjson_mut_obj_add_val(r.document, r.result, "applied_keys", rows);
        for (size_t i = 0; i < applied.size() && i < 200; ++i) {
            const auto& key = applied[i];
            auto* row = yyjson_mut_obj(r.document);
            yyjson_mut_arr_append(rows, row);
            yyjson_mut_obj_add_uint(r.document, row, "track", key.ref.track);
            yyjson_mut_obj_add_uint(r.document, row, "key", key.ref.key);
            yyjson_mut_obj_add_int(r.document, row, "frame", key.frame);
            yyjson_mut_obj_add_real(r.document, row, "value", key.value);
            yyjson_mut_obj_add_real(r.document, row, "tangent", key.tangent);
        }
        yyjson_mut_obj_add_bool(r.document, r.result, "applied_keys_truncated",
                                applied.size() > 200);
        return true;
    }
}
