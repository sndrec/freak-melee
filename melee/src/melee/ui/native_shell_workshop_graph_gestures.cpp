#include "native_shell_workshop_editor_internal.h"
#ifdef MELEE_NATIVE
namespace WorkshopEditor {

    bool rebuilding_action_inspector;

    bool begin_graph_transform(GraphTool tool, int mouse_x, int mouse_y,
                               bool modal)
    {
        if (!session->project || session->graph_selection.empty() ||
            session->graph_drag_active ||
            session->mode != WorkshopMode::Animation ||
            session->graph_width <= 18)
        {
            return false;
        }
        stop_action_preview();
        session->graph_tool = tool;
        session->graph_tangent = false;
        session->graph_drag_baselines.clear();
        auto selection = session->graph_selection;
        std::sort(selection.begin(), selection.end(), [](const auto& a, const auto& b) {
            return a.track < b.track || (a.track == b.track && a.key < b.key);
        });
        std::vector<NativeFighterAnimationKey> source_keys;
        std::vector<float> frames, values;
        for (size_t first = 0; first < selection.size();) {
            const unsigned track = selection[first].track;
            size_t end = first + 1;
            while (end < selection.size() && selection[end].track == track) ++end;
            if (track < session->track_count && session->graph_visible[track]) {
                source_keys.resize(session->tracks[track].keys);
                size_t count;
                if (nativeFighterAnimation_Keys(session->project, session->animation_clip,
                        track, source_keys.data(), source_keys.size(), &count) &&
                    selection[end - 1].key < count) {
                    frames.clear();
                    for (size_t i = first; i < end; ++i)
                        frames.push_back(float(source_keys[selection[i].key].frame));
                    values.resize(frames.size());
                    const bool sampled = nativeFighterAnimation_SampleFrames(session->project,
                        session->animation_clip, track, frames.data(), values.data(), frames.size());
                    for (size_t i = first; i < end; ++i) {
                        const auto ref = selection[i];
                        const auto& key = source_keys[ref.key];
                        const float displayed = sampled && std::isfinite(values[i - first])
                            ? values[i - first] : key.value;
                        const float x = 8.0f + (key.frame - session->graph_view_start) /
                            (session->graph_view_end - session->graph_view_start) *
                            (session->graph_width - 18.0f);
                        const float units = session->graph_units_per_pixel[track];
                        const float y = 7.0f + (session->graph_high_value[track] - displayed) / units;
                        session->graph_drag_baselines.push_back(
                            {ref, key.frame, key.value, units, x, y,
                             key.tangent, key.tangent_editable});
                    }
                }
            }
            first = end;
        }
        if (session->graph_drag_baselines.empty()) {
            return false;
        }
        float pivot_x = 0, pivot_y = 0;
        if (session->graph_cursor_pivot) {
            pivot_x =
                8 + (session->graph_cursor_frame - session->graph_view_start) *
                        (session->graph_width - 18) /
                        (session->graph_view_end - session->graph_view_start);
            pivot_y =
                7 + (session->graph_value_high - session->graph_cursor_value) *
                        (session->graph_height - 22) /
                        (session->graph_value_high - session->graph_value_low);
        } else {
            float min_x = INFINITY, min_y = INFINITY, max_x = -INFINITY,
                  max_y = -INFINITY;
            for (const auto& baseline : session->graph_drag_baselines) {
                min_x = std::min(min_x, baseline.screen_x);
                max_x = std::max(max_x, baseline.screen_x);
                min_y = std::min(min_y, baseline.screen_y);
                max_y = std::max(max_y, baseline.screen_y);
            }
            pivot_x = (min_x + max_x) * .5f;
            pivot_y = (min_y + max_y) * .5f;
        }
        session->graph_pivot_x = pivot_x;
        session->graph_pivot_y = pivot_y;
        session->graph_rotation = 0;
        const auto origin = document->GetElementById("workshop-animation-graph")
            ->GetAbsoluteOffset();
        const float x = mouse_x - origin.x - pivot_x;
        const float y = mouse_y - origin.y - pivot_y;
        session->graph_previous_angle = std::atan2(y, x);
        session->graph_has_angle = std::hypot(x, y) >= 2.0f;
        session->graph_before.assign(session->project->animation_dat,
                                     session->project->animation_dat +
                                         session->project->animation_size);
        session->graph_fighter_before.assign(session->project->fighter_dat,
            session->project->fighter_dat + session->project->fighter_size);
        session->drag_start_x = mouse_x;
        session->drag_start_y = mouse_y;
        session->graph_constraint = 0;
        session->graph_drag_active = true;
        session->graph_modal = modal;
        session->graph_refresh = true;
        set_text("workshop-editor-status",
                 "Transform keys: move mouse; left click confirms, right "
                 "click cancels. X: time, Y: value.");
        return true;
    }

    static void restore_graph_draft()
    {
        // A retime only appends data. The current allocation always has room
        // for the smaller original bank, even after a partial mouse update.
        std::memcpy(session->project->animation_dat, session->graph_before.data(),
                    session->graph_before.size());
        session->project->animation_size = session->graph_before.size();
        std::memcpy(session->project->fighter_dat, session->graph_fighter_before.data(),
                    session->graph_fighter_before.size());
    }

    bool preview_graph_transform(int mouse_x, int mouse_y)
    {
        if (!session->graph_drag_active) return false;
        restore_graph_draft();
        session->graph_refresh = true;
        nativeHome_WorkshopPoseInvalidate();
        if (session->graph_tangent) {
            const auto& baseline = session->graph_drag_baselines[0];
            const auto origin =
                document->GetElementById("workshop-animation-graph")
                    ->GetAbsoluteOffset();
            const float x = mouse_x - origin.x - baseline.screen_x;
            const float y = mouse_y - origin.y - baseline.screen_y;
            const float frames_per_pixel =
                (session->graph_view_end - session->graph_view_start) /
                (session->graph_width - 18);
            if (x * session->graph_tangent_side < 3) {
                return false;
            }
            const float tangent =
                -y * baseline.units_per_pixel / (x * frames_per_pixel);
            float snapped;
            if (!nativeFighterAnimation_Snap(session->project,
                    session->animation_clip, baseline.ref.track, true,
                    tangent, &snapped) ||
                !nativeFighterAnimation_SetTangent(
                    session->project, session->animation_clip,
                    baseline.ref.track, baseline.ref.key, snapped))
            {
                return false;
            }
            nativeFighterAnimation_Keys(
                session->project, session->animation_clip, baseline.ref.track,
                session->keys, 1024, &session->key_count);
            field("workshop-key-tangent")
                ->SetValue(
                    float_text(session->keys[baseline.ref.key].tangent));
            session->bone_pose_refresh = true;
            return true;
        }
        const int dx = mouse_x - session->drag_start_x;
        const int dy = mouse_y - session->drag_start_y;
        if (dx == 0 && dy == 0 && session->graph_tool != GraphTool::Rotate) {
            session->graph_refresh = true;
            return true;
        }
        const float pivot_x = session->graph_pivot_x;
        const float pivot_y = session->graph_pivot_y;
        if (session->graph_tool == GraphTool::Rotate) {
            const auto origin = document->GetElementById("workshop-animation-graph")
                ->GetAbsoluteOffset();
            const float x = mouse_x - origin.x - pivot_x;
            const float y = mouse_y - origin.y - pivot_y;
            if (std::hypot(x, y) >= 2.0f) {
                const float angle = std::atan2(y, x);
                if (session->graph_has_angle)
                    session->graph_rotation += std::remainder(
                        angle - session->graph_previous_angle, 6.28318530718f);
                session->graph_previous_angle = angle;
                session->graph_has_angle = true;
            }
        }
        const float rotation = session->graph_rotation;
        const float cos_rotation = std::cos(rotation);
        const float sin_rotation = std::sin(rotation);
        struct KeyTarget {
            GraphKeyBaseline baseline;
            int frame;
            float value;
            float tangent;
        };
        std::vector<KeyTarget> targets;
        targets.reserve(session->graph_drag_baselines.size());
        for (const GraphKeyBaseline& baseline : session->graph_drag_baselines)
        {
            float x = baseline.screen_x, y = baseline.screen_y;
            if (session->graph_tool == GraphTool::Move) {
                x += dx;
                y += dy;
            } else if (session->graph_tool == GraphTool::Scale) {
                const float factor =
                    std::clamp(std::exp((dx - dy) * 0.007f), 0.125f, 8.0f);
                x = pivot_x + (x - pivot_x) * factor;
                y = pivot_y + (y - pivot_y) * factor;
            } else {
                const float rx = x - pivot_x, ry = y - pivot_y;
                x = pivot_x + cos_rotation * rx - sin_rotation * ry;
                y = pivot_y + sin_rotation * rx + cos_rotation * ry;
            }
            if (session->graph_constraint == 1) {
                y = baseline.screen_y;
            }
            if (session->graph_constraint == 2) {
                x = baseline.screen_x;
            }
            const float time =
                session->graph_view_start +
                (x - 8.0f) *
                    (session->graph_view_end - session->graph_view_start) /
                    (session->graph_width - 18.0f);
            const float raw_value = baseline.value +
                (baseline.screen_y - y) * baseline.units_per_pixel;
            if (!std::isfinite(time) || !std::isfinite(raw_value) ||
                time < 0 || time > session->clip_frames)
            {
                set_text("workshop-editor-status",
                         "Transform would place a key "
                         "outside the clip. No keys were changed.");
                return false;
            }
            float snapped = baseline.value;
            if (raw_value != baseline.value && !nativeFighterAnimation_Snap(
                    session->project, session->animation_clip,
                    baseline.ref.track, false, raw_value, &snapped))
            {
                set_text("workshop-editor-status",
                         "Transform exceeds a "
                         "track's value precision. No keys were changed.");
                return false;
            }
            float tangent = baseline.tangent;
            if (baseline.tangent_editable &&
                session->graph_tool != GraphTool::Move)
            {
                const float frames_per_pixel =
                    (session->graph_view_end - session->graph_view_start) /
                    (session->graph_width - 18);
                float tx = 1, ty = -baseline.tangent * frames_per_pixel /
                                   baseline.units_per_pixel;
                const float original_y = ty;
                if (session->graph_tool == GraphTool::Scale) {
                    const float factor =
                        std::clamp(std::exp((dx - dy) * .007f), .125f, 8.0f);
                    tx *= factor;
                    ty *= factor;
                } else {
                    tx = cos_rotation - sin_rotation * original_y;
                    ty = sin_rotation + cos_rotation * original_y;
                }
                if (session->graph_constraint == 1) {
                    ty = original_y;
                }
                if (session->graph_constraint == 2) {
                    tx = 1;
                }
                if (std::fabs(tx) < 1e-6f) {
                    set_text("workshop-editor-status",
                             "This rotation makes a vertical tangent. Adjust "
                             "the angle to keep a finite slope.");
                    return false;
                }
                const float slope =
                    -ty / tx * baseline.units_per_pixel / frames_per_pixel;
                if (!nativeFighterAnimation_Snap(session->project,
                        session->animation_clip, baseline.ref.track, true,
                        slope, &tangent)) return false;
            }
            targets.push_back(
                { baseline, (int) std::lround(time), snapped, tangent });
        }
        bool changed = false;
        for (const KeyTarget& target : targets) {
            changed |= target.frame != target.baseline.frame ||
                       target.value != target.baseline.value ||
                       target.tangent != target.baseline.tangent;
        }
        if (!changed) {
            return false;
        }
        std::vector<NativeFighterAnimationKeyTime> times;
        for (const KeyTarget& target : targets)
            if (target.frame != target.baseline.frame)
                times.push_back({target.baseline.ref, target.frame});
        std::sort(times.begin(), times.end(), [](const auto& a, const auto& b) {
            return a.ref.track < b.ref.track ||
                   (a.ref.track == b.ref.track && a.ref.key < b.ref.key);
        });
        bool valid = times.empty() || nativeFighterAnimation_EditKeys(
            session->project, session->animation_clip, times.data(), times.size(), false);
        for (const KeyTarget& target : targets) {
            if (!valid) {
                break;
            }
            if (target.value != target.baseline.value) {
                valid = nativeFighterAnimation_SetKey(
                    session->project, session->animation_clip,
                    target.baseline.ref.track, target.baseline.ref.key,
                    target.value);
            }
            if (valid && target.baseline.tangent_editable &&
                target.tangent != target.baseline.tangent)
            {
                valid = nativeFighterAnimation_SetTangent(
                    session->project, session->animation_clip,
                    target.baseline.ref.track, target.baseline.ref.key,
                    target.tangent);
            }
        }
        if (!valid) {
            restore_graph_draft();
            set_text("workshop-editor-status",
                     "Selected keys cannot move there "
                     "in the source encoding. Key order and track precision "
                     "must remain valid; no keys were changed.");
            return false;
        }
        nativeFighterAnimation_Keys(session->project, session->animation_clip,
                                    (unsigned) session->selected_track,
                                    session->keys, 1024, &session->key_count);
        session->bone_pose_refresh = true;
        return true;
    }

    void finish_graph_transform(bool commit)
    {
        if (!session->graph_drag_active) {
            return;
        }
        if (commit) {
            if (session->project->animation_size != session->graph_before.size()) {
                record_structural_edit(true, session->graph_before.data(), session->graph_before.size(),
                    session->graph_fighter_before.data(), session->graph_fighter_before.size());
                session->animation_dirty = true;
            } else if (std::memcmp(session->graph_before.data(), session->project->animation_dat,
                                   session->graph_before.size())) {
                record_edit(true, session->graph_before.data());
                session->animation_dirty = true;
            }
        } else restore_graph_draft();
        session->graph_drag_active = false;
        session->graph_tool = GraphTool::Move;
        session->graph_before.clear();
        session->graph_fighter_before.clear();
        nativeHome_WorkshopPoseInvalidate();
        if (session->selected_track >= 0) {
            nativeFighterAnimation_Keys(
                session->project, session->animation_clip,
                (unsigned) session->selected_track, session->keys, 1024,
                &session->key_count);
            if (session->selected_key >= 0)
                select_key((unsigned) session->selected_key, false);
        }
        session->bone_pose_refresh = true;
        session->graph_refresh = true;
        set_text("workshop-editor-status", commit
                                               ? "Key transform committed."
                                               : "Key transform cancelled.");
    }

    static bool discard_click;

    void GraphKeyEvents::ProcessEvent(Rml::Event& event)
    {
        if (event.GetType() != "mousedown" ||
            event.GetParameter<int>("button", -1) != 0 ||
            session->graph_tool == GraphTool::Box ||
            session->graph_drag_active)
        {
            return;
        }
        unsigned track, key;
        char tail;
        const auto& id = event.GetTargetElement()->GetId();
        if (id.compare(0, 19, "workshop-graph-key-") != 0 ||
            std::sscanf(id.c_str() + 19, "%u-%u%c", &track, &key, &tail) != 2)
        {
            return;
        }
        const auto found = std::find_if(
            session->graph_selection.begin(), session->graph_selection.end(),
            [=](const GraphKeyRef& ref) {
                return ref.track == track && ref.key == key;
            });
        if (found != session->graph_selection.end() &&
            event.GetParameter<int>("shift_key", 0))
        {
            session->graph_selection.erase(found);
            session->graph_refresh = true;
            discard_click = true;
            event.StopPropagation();
            return;
        }
        if (found == session->graph_selection.end()) {
            if (!event.GetParameter<int>("shift_key", 0)) {
                session->graph_selection.clear();
            }
            session->graph_selection.push_back({ track, key });
        }
        session->selected_track = (int) track;
        if (nativeFighterAnimation_Keys(
                session->project, session->animation_clip, track,
                session->keys, 1024, &session->key_count))
        {
            select_key(key, false);
        }
        begin_graph_transform(session->graph_tool,
                              event.GetParameter<int>("mouse_x", 0),
                              event.GetParameter<int>("mouse_y", 0), false);
        event.StopPropagation();
    }

    void GraphTangentEvents::ProcessEvent(Rml::Event& event)
    {
        if (event.GetType() != "mousedown" ||
            event.GetParameter<int>("button", -1) != 0 ||
            session->graph_drag_active)
        {
            return;
        }
        unsigned track, key, side;
        char tail;
        const auto& id = event.GetTargetElement()->GetId();
        const char* prefix = "workshop-graph-tangent-";
        if (id.compare(0, std::strlen(prefix), prefix) ||
            std::sscanf(id.c_str() + std::strlen(prefix), "%u-%u-%u%c", &track,
                        &key, &side, &tail) != 3 ||
            track >= session->track_count || side > 1)
        {
            return;
        }
        NativeFighterAnimationKey keys[1024];
        size_t count;
        if (!nativeFighterAnimation_Keys(session->project,
                                         session->animation_clip, track, keys,
                                         1024, &count) ||
            key >= count || !keys[key].tangent_editable)
        {
            return;
        }
        session->graph_selection.clear();
        session->graph_selection.push_back({ track, key });
        session->selected_track = (int) track;
        nativeFighterAnimation_Keys(session->project, session->animation_clip,
                                    track, session->keys, 1024,
                                    &session->key_count);
        select_key(key, false);
        if (begin_graph_transform(
                GraphTool::Move, event.GetParameter<int>("mouse_x", 0),
                event.GetParameter<int>("mouse_y", 0), false))
        {
            session->graph_tangent = true;
            session->graph_tangent_side = side ? 1 : -1;
            set_text("workshop-editor-status",
                     "Adjust tangent slope; release to confirm, right click "
                     "or Esc to cancel.");
        }
        event.StopPropagation();
    }

    enum class Property {
        None, Hitbox, Sound, Effect, Command, EventValue, Playback,
        ActionName, Key, Bone, ClipEnd, Model, Attribute, Parameter,
    };

    static Property inspector_property(const Rml::String& id)
    {
        auto prefix = [&](const char* start) { return id.rfind(start, 0) == 0; };
        if (prefix("workshop-attribute-")) return Property::Attribute;
        switch (session->mode) {
        case WorkshopMode::Action:
            if (prefix("workshop-hitbox-")) return Property::Hitbox;
            if (prefix("workshop-sound-")) return Property::Sound;
            if (prefix("workshop-effect-")) return Property::Effect;
            if (prefix("workshop-command-field-")) return Property::Command;
            if (id == "workshop-event-value" || id == "workshop-air-state")
                return Property::EventValue;
            if (id == "workshop-action-loop" || id == "workshop-action-root-motion")
                return Property::Playback;
            if (id == "workshop-action-name") return Property::ActionName;
            break;
        case WorkshopMode::Animation:
            if (prefix("workshop-key-")) return Property::Key;
            if (prefix("workshop-bone-")) return Property::Bone;
            if (id == "workshop-clip-end") return Property::ClipEnd;
            break;
        case WorkshopMode::Model:
            if (prefix("workshop-model-") || prefix("workshop-material-") ||
                prefix("workshop-texture-")) return Property::Model;
            break;
        case WorkshopMode::Script:
            if (prefix("workshop-param-value-")) return Property::Parameter;
            break;
        }
        return Property::None;
    }

    static bool commit_property(Property property, const Rml::String& id)
    {
        switch (property) {
        case Property::Hitbox: apply_hitbox(); show_event(); return true;
        case Property::Sound: apply_sound(); show_event(); return true;
        case Property::Effect: apply_effect(); show_event(); return true;
        case Property::Command: apply_command_fields(); show_event(); return true;
        case Property::EventValue: apply_event_value(); show_event(); return true;
        case Property::Playback: apply_action_playback(); show_action_playback(); return true;
        case Property::ActionName: return edit_action_label(true);
        case Property::Key:
            apply_key();
            if (session->selected_key >= 0) select_key((unsigned) session->selected_key, false);
            return true;
        case Property::Bone: apply_bone_transform(); return true;
        case Property::ClipEnd:
            apply_clip_frames();
            field("workshop-clip-end")->SetValue(float_text(session->clip_frames));
            return true;
        case Property::Model: return nativeShellWorkshopModel_CommitProperty(id.c_str());
        case Property::Attribute:
        case Property::Parameter:
        case Property::None: return false;
        }
        return false;
    }

    enum class DeleteTarget { None, Keys, Event, Bone };
    static DeleteTarget delete_target;
    static Rml::ObserverPtr<Rml::Element> delete_return_focus;
    static int pointer_x, pointer_y;

    void open_delete_confirmation()
    {
        if (!session->project) return;
        Rml::String label;
        if (session->mode == WorkshopMode::Animation &&
            !session->graph_selection.empty()) {
            delete_target = DeleteTarget::Keys;
            const unsigned count = (unsigned) session->graph_selection.size();
            label = "Delete " + number(count) + (count == 1 ? " key?" : " keys?");
        } else if (session->mode == WorkshopMode::Action && session->selected_event >= 0) {
            delete_target = DeleteTarget::Event;
            label = "Delete selected command?";
        } else if (session->mode == WorkshopMode::Model) {
            auto* remove = document->GetElementById("workshop-model-bone-remove");
            if (!remove || !remove->IsVisible()) return;
            delete_target = DeleteTarget::Bone;
            label = "Remove selected bone?";
        } else return;
        auto* popup = document->GetElementById("workshop-delete-confirmation");
        auto* panel = document->GetElementById("workshop-panel");
        const auto origin = panel->GetAbsoluteOffset();
        const auto dimensions = document->GetContext()->GetDimensions();
        const float density = document->GetContext()->GetDensityIndependentPixelRatio();
        const int x = std::clamp(pointer_x - int(origin.x), 0,
            std::max(0, dimensions.x - int(origin.x) - int(210 * density)));
        const int y = std::clamp(pointer_y - int(origin.y), 0,
            std::max(0, dimensions.y - int(origin.y) - int(88 * density)));
        popup->SetProperty("left", number((unsigned) x) + "px");
        popup->SetProperty("top", number((unsigned) y) + "px");
        set_text("workshop-delete-title", label);
        if (auto* focus = document->GetContext()->GetFocusElement())
            delete_return_focus = focus->GetObserverPtr();
        popup->SetProperty("display", "block");
        document->GetElementById("workshop-delete-confirm")->Focus();
    }

    void close_delete_confirmation()
    {
        if (delete_target == DeleteTarget::None) return;
        delete_target = DeleteTarget::None;
        document->GetElementById("workshop-delete-confirmation")->SetProperty("display", "none");
        if (auto* focus = delete_return_focus.get(); focus && focus->IsVisible())
            focus->Focus();
        delete_return_focus = {};
    }

    void confirm_delete()
    {
        const DeleteTarget target = delete_target;
        close_delete_confirmation();
        switch (target) {
        case DeleteTarget::Keys: delete_selected_keys(); break;
        case DeleteTarget::Event: delete_selected_event(); break;
        case DeleteTarget::Bone: nativeShellWorkshopModel_Click("workshop-model-bone-remove", false); break;
        case DeleteTarget::None: break;
        }
    }

    class EditorInputEvents final : public Rml::EventListener {
        const char* ruler = nullptr;
        Rml::ObserverPtr<Rml::Element> editing;
        Rml::String original;
        bool committing = false;
        Rml::ObserverPtr<Rml::Element> pending;

    public:
        void CommitDiscrete()
        {
            auto selected = std::move(pending);
            pending = {};
            if (auto* control = selected.get(); control && control->IsVisible()) {
                const Rml::String id = control->GetId();
                committing = true;
                commit_property(inspector_property(id), id);
                committing = false;
            }
        }

        void ProcessEvent(Rml::Event& event) override
        {
            if (!session->project || !nativeShellWorkshopEditor_IsOpen()) {
                return;
            }
            const auto type = event.GetType();
            auto* target = event.GetTargetElement();
            const auto& id = target->GetId();
            const bool checkbox = target->GetAttribute<Rml::String>("type", "") == "checkbox";
            const bool text_field = target->GetTagName() == "input" && !checkbox;
            if (type == "focus" && text_field &&
                inspector_property(id) != Property::None &&
                (session->mode != WorkshopMode::Model ||
                 nativeShellWorkshopModel_IsProperty(id.c_str()))) {
                editing = target->GetObserverPtr();
                original = field(id.c_str())->GetValue();
            } else if (type == "blur" && target == editing.get()) {
                editing = {};
                if (!committing) field(id.c_str())->SetValue(original);
            } else if (type == "mousemove") {
                pointer_x = event.GetParameter<int>("mouse_x", pointer_x);
                pointer_y = event.GetParameter<int>("mouse_y", pointer_y);
                if (session->graph_drag_active) {
                    preview_graph_transform(pointer_x, pointer_y);
                } else if (ruler) {
                    nativeShellWorkshopEditor_ScrubTimeline(ruler, pointer_x);
                }
            } else if (type == "mousedown") {
                discard_click = false;
                pointer_x = event.GetParameter<int>("mouse_x", pointer_x);
                pointer_y = event.GetParameter<int>("mouse_y", pointer_y);
                const int button = event.GetParameter<int>("button", -1);
                if (delete_target != DeleteTarget::None) {
                    auto* popup = document->GetElementById("workshop-delete-confirmation");
                    bool inside = false;
                    for (auto* p = target; p; p = p->GetParentNode()) inside |= p == popup;
                    if (button == 1 || !inside) {
                        close_delete_confirmation();
                        discard_click = true;
                        event.StopPropagation();
                        return;
                    }
                }
                if (session->graph_drag_active &&
                    (button == 1 || (button == 0 && session->graph_modal)))
                {
                    finish_graph_transform(button == 0);
                    discard_click = true;
                    event.StopPropagation();
                    return;
                }
                if (button == 1 && editing.get()) {
                    editing.get()->Blur();
                    discard_click = true;
                    event.StopPropagation();
                    return;
                }
                if (button == 0 && session->mode == WorkshopMode::Animation) {
                    for (auto* p = target; p; p = p->GetParentNode()) {
                        if (p->GetId() == "workshop-dope-ruler") {
                            ruler = "workshop-dope-ruler";
                            break;
                        }
                    }
                    if (ruler) {
                        nativeShellWorkshopEditor_ScrubTimeline(ruler, pointer_x);
                    }
                }
            } else if (type == "mouseup") {
                ruler = nullptr;
                if (session->graph_drag_active && !session->graph_modal &&
                    event.GetParameter<int>("button", -1) == 0)
                {
                    finish_graph_transform(true);
                    discard_click = true;
                }
            } else if (type == "click" && discard_click) {
                discard_click = false;
                event.StopPropagation();
            } else if (type == "change") {
                if (!committing && !rebuilding_action_inspector &&
                    (checkbox || target->GetTagName() == "select") &&
                    inspector_property(id) != Property::None &&
                    inspector_property(id) != Property::Parameter &&
                    (session->mode != WorkshopMode::Model ||
                     nativeShellWorkshopModel_IsProperty(id.c_str()))) {
                    // Rml's dropdown continues using its options after dispatching
                    // change. Apply on Update, after the widget has finished.
                    pending = target->GetObserverPtr();
                    return;
                }
                if (id == "workshop-graph-pivot") {
                    session->graph_cursor_pivot =
                        static_cast<Rml::ElementFormControlSelect*>(target)
                            ->GetValue() == "cursor";
                } else if (id == "workshop-graph-animated-only" ||
                           id == "workshop-graph-normalize")
                {
                    const bool checked = target->HasAttribute("checked");
                    if (id == "workshop-graph-normalize") {
                        session->normalize_values = checked;
                    } else {
                        session->graph_animated_only = checked;
                    }
                    session->graph_value_range_valid = false;
                    session->graph_refresh = true;
                }
            } else if (type == "keydown") {
                const int key = event.GetParameter<int>("key_identifier", 0);
                if (delete_target != DeleteTarget::None) {
                    if (key == Rml::Input::KI_ESCAPE) close_delete_confirmation();
                    if (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER)
                        confirm_delete();
                    event.StopPropagation();
                    return;
                }
                if (session->graph_drag_active) {
                    if (key == Rml::Input::KI_ESCAPE) {
                        finish_graph_transform(false);
                    } else if (key == Rml::Input::KI_RETURN) {
                        finish_graph_transform(true);
                    } else if (key == Rml::Input::KI_X ||
                               key == Rml::Input::KI_Y)
                    {
                        session->graph_constraint =
                            key == Rml::Input::KI_X ? 1 : 2;
                        if (event.GetParameter<int>("shift_key", 0)) {
                            session->graph_constraint =
                                3 - session->graph_constraint;
                        }
                        preview_graph_transform(pointer_x, pointer_y);
                    } else {
                        return;
                    }
                    event.StopPropagation();
                    return;
                }
                if (key == Rml::Input::KI_ESCAPE && target == editing.get()) {
                    target->Blur();
                    event.StopPropagation();
                    return;
                }
                const bool ctrl = event.GetParameter<int>("ctrl_key", 0) != 0;
                if (!ctrl && !text_field && !checkbox &&
                    target->GetTagName() != "textarea" && target->GetTagName() != "select" &&
                    key == Rml::Input::KI_DELETE) {
                    open_delete_confirmation();
                    if (delete_target != DeleteTarget::None) event.StopPropagation();
                    return;
                }
                if (ctrl &&
                    (key == Rml::Input::KI_Z || key == Rml::Input::KI_Y) &&
                    (session->mode == WorkshopMode::Animation || session->mode == WorkshopMode::Model))
                {
                    const bool redo = key == Rml::Input::KI_Y ||
                                      event.GetParameter<int>("shift_key", 0);
                    if (session->mode == WorkshopMode::Model) {
                        nativeShellWorkshopModel_Undo(redo);
                    } else {
                        apply_history(redo);
                    }
                    event.StopPropagation();
                    return;
                }
                if (text_field && (key == Rml::Input::KI_RETURN || key == Rml::Input::KI_NUMPADENTER) &&
                    inspector_property(id) != Property::None) {
                    // Inspector refresh can replace the focused element.
                    const Rml::String field_id = id;
                    committing = true;
                    const bool handled = commit_property(inspector_property(id), field_id);
                    if (handled) {
                        if (auto* control = document->GetElementById(field_id);
                            control && control == document->GetContext()->GetFocusElement())
                            control->Blur();
                        event.StopPropagation();
                    }
                    committing = false;
                    if (handled) return;
                }
                if (key == Rml::Input::KI_I &&
                    session->mode == WorkshopMode::Animation)
                {
                    auto* hovered = document->GetContext()->GetHoverElement();
                    static const char* ids[] = { "px", "py", "pz", "rx", "ry",
                                                 "rz", "sx", "sy", "sz" };
                    for (auto* p = hovered; p; p = p->GetParentNode()) {
                        for (unsigned channel = 0; channel < 9; ++channel) {
                            if (p->GetId() ==
                                std::string("workshop-bone-") + ids[channel])
                            {
                                nativeWorkshopPose_InsertChannel(
                                    (unsigned)
                                        session->selected_outliner_joint,
                                    channel);
                                event.StopPropagation();
                                return;
                            }
                        }
                    }
                }
                if (ctrl || event.GetParameter<int>("alt_key", 0) ||
                    target->GetTagName() == "input" ||
                    target->GetTagName() == "textarea" ||
                    session->mode != WorkshopMode::Animation)
                {
                    return;
                }
                if (key == Rml::Input::KI_G || key == Rml::Input::KI_R ||
                    key == Rml::Input::KI_S)
                {
                    auto* hover = document->GetContext()->GetHoverElement();
                    bool on_graph = false;
                    for (auto* p = hover; p; p = p->GetParentNode()) {
                        on_graph |= p->GetId() == "workshop-animation-graph";
                    }
                    if (on_graph &&
                        begin_graph_transform(
                            key == Rml::Input::KI_G   ? GraphTool::Move
                            : key == Rml::Input::KI_R ? GraphTool::Rotate
                                                      : GraphTool::Scale,
                            pointer_x, pointer_y, true))
                    {
                        event.StopPropagation();
                    }
                }
            }
        }
    };
    static EditorInputEvents editor_input_events;
    void commit_discrete_property() { editor_input_events.CommitDiscrete(); }

    void init_editor_input()
    {
        for (const char* type : { "mousemove", "mousedown", "mouseup", "click",
                                  "keydown", "change", "focus", "blur" })
        {
            document->GetElementById("workshop-panel")
                ->AddEventListener(type, &editor_input_events, true);
        }
    }
}
#endif
