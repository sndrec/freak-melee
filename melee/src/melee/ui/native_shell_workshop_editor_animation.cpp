#include "native_shell_workshop_editor_internal.h"
#include <melee/lb/native_math_functions.h>

#ifdef MELEE_NATIVE

namespace WorkshopEditor {

void show_animation_keys()
{
    std::string markup;
    for (size_t i = 0; i < session->key_count; ++i) {
        markup += "<button id='workshop-key-" + number((unsigned) i) +
                  "' class='workshop-track-row" +
                  ((int) i == session->selected_key ? " selected" : "") +
                  "'>Frame " + std::to_string(session->keys[i].frame) +
                  " · " + float_text(session->keys[i].value) + "</button>";
    }
    document->GetElementById("workshop-animation-keys")->SetInnerRML(markup);
}

void show_animation_graph();
void show_animation_dopesheet();
void show_animation();

void stop_action_preview();

void show_bone_transform()
{
    // Input/session changes can precede the live clip evaluation this frame.
    // Refresh once again after that evaluation, without overwriting active
    // numeric edits on every UI tick.
    session->bone_pose_refresh = true;
    auto* section = document->GetElementById("workshop-bone-fields");
    const int joint = session->selected_outliner_joint;
    NativeHomeWorkshopBoneLocal pose;
    if (session->project == nullptr || joint < 0 ||
        !nativeWorkshopPose_Local((unsigned) joint, &pose))
    {
        section->SetProperty("display", "none");
        return;
    }
    section->SetProperty("display", "block");
    set_text("workshop-bone-name", bone_name((unsigned) joint));
    unsigned parent = UINT_MAX;
    set_text("workshop-bone-parent",
             nativeHome_WorkshopBoneParent((unsigned) joint, &parent) &&
                     parent != UINT_MAX
                 ? "Parent: joint " + number(parent)
                 : "Root joint");
    set_text("workshop-bone-frame",
             "Local pose at frame " + number(session->animation_frame));
    static const char* ids[] = { "workshop-bone-px", "workshop-bone-py",
                                 "workshop-bone-pz", "workshop-bone-rx",
                                 "workshop-bone-ry", "workshop-bone-rz",
                                 "workshop-bone-sx", "workshop-bone-sy",
                                 "workshop-bone-sz" };
    for (unsigned i = 0; i < 9; ++i) {
        float value = i < 3   ? pose.position[i]
                      : i < 6 ? pose.rotation[i - 3]
                              : pose.scale[i - 6];
        if (document->GetContext()->GetFocusElement() != field(ids[i]))
            field(ids[i])->SetValue(float_text(value));
        field(ids[i])->RemoveAttribute("disabled");
    }
}

void apply_bone_transform()
{
    if (session->selected_outliner_joint < 0) {
        return;
    }
    static const char* ids[] = { "workshop-bone-px", "workshop-bone-py",
                                 "workshop-bone-pz", "workshop-bone-rx",
                                 "workshop-bone-ry", "workshop-bone-rz",
                                 "workshop-bone-sx", "workshop-bone-sy",
                                 "workshop-bone-sz" };
    NativeHomeWorkshopBoneLocal pose = {};
    for (unsigned i = 0; i < 9; ++i) {
        float* value = i < 3   ? pose.position + i
                       : i < 6 ? pose.rotation + i - 3
                               : pose.scale + i - 6;
        if (!parse_float(ids[i], value)) {
            set_text("workshop-editor-status", "Enter finite transform values.");
            return;
        }
    }
    nativeWorkshopPose_Stage((unsigned) session->selected_outliner_joint, pose);
    session->bone_pose_refresh = true;
}

void key_bone_channels()
{
    nativeWorkshopPose_InsertKeys();
}

void record_pose_keys(const uint8_t* before, size_t size,
                      const uint8_t* before_fighter, size_t fighter_size)
{
    record_structural_edit(true, before, size, before_fighter, fighter_size);
    session->animation_dirty = true;
    NativeFighterAnimationTrack previous[1024];
    const int previous_active = session->selected_track;
    const size_t previous_count = session->track_count;
    std::memcpy(previous, session->tracks, previous_count * sizeof(previous[0]));
    nativeFighterAnimation_Tracks(session->project, session->animation_clip, session->tracks,
                                  1024, &session->track_count);
    session->selected_track = -1;
    for (unsigned t = 0; t < session->track_count; ++t) {
        if (!track_selected(t)) continue;
        if (session->selected_track < 0) session->selected_track = (int) t;
        if (previous_active >= 0 && (size_t) previous_active < previous_count &&
            session->tracks[t].joint == previous[previous_active].joint &&
            session->tracks[t].channel == previous[previous_active].channel) {
            session->selected_track = (int) t;
            break;
        }
    }
    if (session->selected_track >= 0) {
        nativeFighterAnimation_Keys(session->project, session->animation_clip,
                                    (unsigned) session->selected_track, session->keys, 1024,
                                    &session->key_count);
    }
    show_animation_keys();
    show_animation_graph();
    show_animation_dopesheet();
    nativeShellWorkshop_RequestPreview(false);
}

class WorkshopCurveGraph final : public Rml::Element {
public:
    explicit WorkshopCurveGraph(const Rml::String& tag) : Rml::Element(tag) {}

    void SetCurves(Rml::Mesh mesh)
    {
        curves = GetRenderManager()->MakeGeometry(std::move(mesh));
    }

protected:
    void OnRender() override
    {
        Rml::ElementUtilities::SetClippingRegion(this);
        curves.Render(GetAbsoluteOffset(Rml::BoxArea::Content));
    }

private:
    Rml::Geometry curves;
};

void show_animation_graph()
{
    if (!session->show_animation_curves || session->project == nullptr ||
        session->animation_clip.index == UINT_MAX || session->clip_frames <= 0)
        return;
    auto* graph = static_cast<WorkshopCurveGraph*>(
        document->GetElementById("workshop-animation-graph"));
    int width = graph->GetClientWidth();
    if (width < 100) {
        const Rml::Context* context = document->GetContext();
        const float density = context->GetDensityIndependentPixelRatio();
        width = context->GetDimensions().x - int(700 * density);
    }
    width = std::max(width, 120);
    session->graph_width = width;
    const int client_height = graph->GetClientHeight();
    const int height = client_height > 22 ? client_height : 140;
    session->graph_height = height;
    const float plot_height = float(height - 22);
    constexpr int SAMPLES = 33;
    struct Curve {
        unsigned track;
        std::vector<float> frames, values;
        std::vector<NativeFighterAnimationKey> keys;
        float low = INFINITY;
        float high = -INFINITY;
    };
    std::vector<Curve> curves;
    curves.reserve(session->track_count);
    float global_low = INFINITY, global_high = -INFINITY;
    for (unsigned track = 0; track < session->track_count; ++track) {
        if (!track_selected(track) ||
            (session->graph_animated_only && session->tracks[track].keys <= 1)) continue;
        Curve curve = {};
        curve.track = track;
        curve.keys.resize(session->tracks[track].keys);
        size_t key_count;
        if (!nativeFighterAnimation_Keys(session->project, session->animation_clip,
                track, curve.keys.data(), curve.keys.size(), &key_count)) continue;
        curve.keys.resize(key_count);
        curve.frames.reserve(SAMPLES + key_count);
        for (int i = 0; i < SAMPLES; ++i) {
            const float fraction = float(i) / (SAMPLES - 1);
            curve.frames.push_back(std::lerp(session->graph_view_start,
                                             session->graph_view_end, fraction));
        }
        for (const auto& key : curve.keys)
            if (key.frame >= session->graph_view_start && key.frame <= session->graph_view_end)
                curve.frames.push_back(float(key.frame));
        std::sort(curve.frames.begin(), curve.frames.end());
        curve.frames.erase(std::unique(curve.frames.begin(), curve.frames.end()), curve.frames.end());
        curve.values.resize(curve.frames.size());
        if (!nativeFighterAnimation_SampleFrames(session->project, session->animation_clip,
                track, curve.frames.data(), curve.values.data(), curve.frames.size())) continue;
        for (float value : curve.values) {
            if (!std::isfinite(value)) continue;
            curve.low = std::min(curve.low, value);
            curve.high = std::max(curve.high, value);
        }
        if (curve.low > curve.high) continue;
        global_low = std::min(global_low, curve.low);
        global_high = std::max(global_high, curve.high);
        curves.push_back(std::move(curve));
    }
    const unsigned count = (unsigned) curves.size();
    static const char* colors[] = {
        "#f59e82", "#92b9ed", "#9ccfc0", "#b69bdb",
        "#f2d27e", "#ec8497", "#a7c9fa", "#dfbba4"
    };
    static const Rml::ColourbPremultiplied line_colors[] = {
        {245,158,130,255}, {146,185,237,255}, {156,207,192,255}, {182,155,219,255},
        {242,210,126,255}, {236,132,151,255}, {167,201,250,255}, {223,187,164,255}
    };
    Rml::Mesh mesh;
    auto selection = session->graph_selection;
    auto key_order = [](const GraphKeyRef& a, const GraphKeyRef& b) {
        return a.track < b.track || (a.track == b.track && a.key < b.key);
    };
    std::sort(selection.begin(), selection.end(), key_order);
    std::string markup;
    if (!session->graph_value_range_valid) {
        session->graph_value_low = count ? global_low : -1;
        session->graph_value_high = count ? global_high : 1;
        const float range = session->graph_value_high - session->graph_value_low;
        const float margin = range < 1e-6f ? .5f : range * .12f;
        session->graph_value_low -= margin;
        session->graph_value_high += margin;
        session->graph_value_range_valid = true;
    }
    global_low = session->graph_value_low;
    global_high = session->graph_value_high;
    auto grid_step = [](float span, float pixels) {
        const float desired = span * 80.0f / std::max(pixels, 1.0f);
        const float base = cr_powf(10.0f, std::floor(cr_log10f(std::max(desired, 1e-9f))));
        const float unit = desired / base;
        return base * (unit <= 1 ? 1 : unit <= 2 ? 2 : unit <= 5 ? 5 : 10);
    };
    const float time_span = session->graph_view_end - session->graph_view_start;
    const float time_step = std::max(1.0f, grid_step(time_span, float(width - 18)));
    for (float frame = std::ceil(session->graph_view_start / time_step) * time_step;
         frame <= session->graph_view_end; frame += time_step) {
        const float x = 8 + (frame - session->graph_view_start) * (width - 18) / time_span;
        markup += "<div class='workshop-graph-gridline' style='left:" + float_text(x) +
                  "px;top:0;height:100%;width:1px'></div><div class='workshop-graph-gridlabel' style='left:" +
                  float_text(x + 2) + "px;bottom:1px'>" + graph_tick_text(frame, time_step) + "</div>";
    }
    const float low = session->normalize_values ? 0 : global_low;
    const float high = session->normalize_values ? 1 : global_high;
    const float value_step = grid_step(high - low, plot_height);
    for (float value = std::ceil(low / value_step) * value_step; value <= high; value += value_step) {
        const float y = 7 + (high - value) * plot_height / (high - low);
        markup += "<div class='workshop-graph-gridline' style='left:0;top:" + float_text(y) +
                  "px;width:100%;height:1px'></div><div class='workshop-graph-gridlabel' style='left:3px;top:" +
                  float_text(y + 2) + "px'>" + graph_tick_text(value, value_step) + "</div>";
    }
    const int cursor_x = session->clip_frames > 0.0f
        ? int(8.0f + std::clamp((float(session->animation_frame) - session->graph_view_start) /
                                (session->graph_view_end - session->graph_view_start),
                                0.0f, 1.0f) * (width - 18.0f)) : 8;
    markup += "<div id='workshop-graph-playhead' "
              "class='workshop-graph-playhead' style='left:" +
              number((unsigned) std::max(cursor_x, 0)) + "px'></div>";
    std::fill(session->graph_visible, session->graph_visible + 1024, false);
    for (unsigned c = 0; c < count; ++c) {
        const Curve& curve = curves[c];
        float low = session->normalize_values ? curve.low : global_low;
        float high = session->normalize_values ? curve.high : global_high;
        if (high - low < 1e-6f) { low -= 0.5f; high += 0.5f; }
        if (session->graph_drag_active) {
            high = session->graph_high_value[curve.track];
            low = high - session->graph_units_per_pixel[curve.track] * plot_height;
        }
        session->graph_visible[curve.track] = true;
        session->graph_units_per_pixel[curve.track] = (high - low) / plot_height;
        session->graph_high_value[curve.track] = high;
        auto x = [width](float frame) {
            return 8.0f + (frame - session->graph_view_start) /
                   (session->graph_view_end - session->graph_view_start) * (width - 18.0f);
        };
        auto y = [low, high, plot_height](float value) {
            return 7.0f + (1.0f - (value - low) / (high - low)) * plot_height;
        };
        const auto& graph_keys = curve.keys;
        const size_t graph_key_count = graph_keys.size();
        const bool overflow = session->graph_drag_active &&
            session->selected_track == (int) curve.track;
        auto in_graph = [&](float px, float py, float margin) {
            return std::isfinite(px) && std::isfinite(py) &&
                (overflow || (px >= margin && px <= width - margin &&
                              py >= margin && py <= height - margin));
        };
        auto add_segment = [&](float x0, float y0, float x1, float y1,
                               bool tangent = false) {
            const float dx = x1 - x0, dy = y1 - y0;
            if (!std::isfinite(dx) || !std::isfinite(dy)) return;
            if (!overflow) {
                float enter = 0, leave = 1;
                auto clip_axis = [&](float at, float delta, float limit) {
                    if (delta == 0) return at >= 2 && at <= limit - 2;
                    const float first = (2 - at) / delta;
                    const float last = (limit - 2 - at) / delta;
                    enter = std::max(enter, std::min(first, last));
                    leave = std::min(leave, std::max(first, last));
                    return enter < leave;
                };
                if (!clip_axis(x0, dx, width) || !clip_axis(y0, dy, height)) return;
                x1 = x0 + dx * leave;
                y1 = y0 + dy * leave;
                x0 += dx * enter;
                y0 += dy * enter;
            }
            const float length = cr_hypotf(x1 - x0, y1 - y0);
            if (length < 0.1f) return;
            const float half = tangent ? 0.5f : 0.8f;
            const float offset_x = (y0 - y1) * half / length;
            const float offset_y = (x1 - x0) * half / length;
            const int vertex = (int) mesh.vertices.size();
            const auto color = line_colors[c % std::size(line_colors)];
            mesh.vertices.insert(mesh.vertices.end(), {
                {{x0 + offset_x, y0 + offset_y}, color, {}},
                {{x0 - offset_x, y0 - offset_y}, color, {}},
                {{x1 - offset_x, y1 - offset_y}, color, {}},
                {{x1 + offset_x, y1 + offset_y}, color, {}},
            });
            mesh.indices.insert(mesh.indices.end(), {
                vertex, vertex + 1, vertex + 2, vertex, vertex + 2, vertex + 3
            });
        };
        for (size_t point = 1; point < curve.frames.size(); ++point) {
            if (!std::isfinite(curve.values[point - 1]) ||
                !std::isfinite(curve.values[point])) continue;
            add_segment(x(curve.frames[point - 1]), y(curve.values[point - 1]),
                        x(curve.frames[point]), y(curve.values[point]));
        }
        for (size_t key = 0; key < graph_key_count; ++key) {
            if (graph_keys[key].frame < session->graph_view_start ||
                graph_keys[key].frame > session->graph_view_end) continue;
            const float frame = float(graph_keys[key].frame);
            const size_t point = std::lower_bound(curve.frames.begin(), curve.frames.end(),
                                                   frame) - curve.frames.begin();
            if (point >= curve.values.size() || !std::isfinite(curve.values[point])) continue;
            const float key_x = x(frame);
            const float key_y = y(curve.values[point]);
            const bool chosen = std::binary_search(selection.begin(), selection.end(),
                GraphKeyRef{curve.track, (unsigned) key}, key_order);
            if (chosen && (int) curve.track == session->selected_track &&
                (int) key == session->selected_key && graph_keys[key].tangent_editable) {
                const float frames_per_pixel =
                    (session->graph_view_end - session->graph_view_start) / (width - 18.0f);
                const float pixels_per_pixel = graph_keys[key].tangent *
                    frames_per_pixel / session->graph_units_per_pixel[curve.track];
                const float reach = std::clamp(28.0f /
                    std::max(1.0f, std::fabs(pixels_per_pixel)), 4.0f, 23.0f);
                for (int side : { -1, 1 }) {
                    const float dx = side * reach;
                    const float dy = -dx * pixels_per_pixel;
                    add_segment(key_x, key_y, key_x + dx, key_y + dy, true);
                    if (!in_graph(key_x + dx, key_y + dy, 5)) continue;
                    char handle_style[160];
                    std::snprintf(handle_style, sizeof(handle_style),
                        "left:%.2fpx;top:%.2fpx;border-color:%s",
                        key_x + dx, key_y + dy, colors[c % 8]);
                    markup += "<button id='workshop-graph-tangent-" +
                        number(curve.track) + "-" + number((unsigned) key) +
                        "-" + number((unsigned) (side > 0)) +
                        "' class='workshop-graph-tangent-handle' style='" +
                        handle_style + "'></button>";
                }
            }
            if (!in_graph(key_x, key_y, 5)) continue;
            char style[180];
            std::snprintf(style, sizeof(style), "left:%.2fpx;top:%.2fpx;"
                "background-color:%s", key_x, key_y, colors[c % 8]);
            markup += "<button id='workshop-graph-key-" +
                      number(curve.track) + "-" + number((unsigned) key) +
                      "' class='workshop-graph-key" +
                      (chosen ? " selected" : "") + "' style='" + style +
                      "'></button>";
        }
    }
    markup += "<div id='workshop-graph-selection-box' "
              "class='workshop-graph-selection-box' "
              "style='display:none'></div>";
    document->GetElementById("workshop-animation-pane")->SetClass(
        "transforming", session->graph_drag_active);
    graph->SetCurves(std::move(mesh));
    graph->SetInnerRML(markup);
    bool selected_joints[1024] = {};
    for (unsigned track = 0; track < session->track_count; ++track)
        if (track_selected(track) && session->tracks[track].joint < 1024 &&
            (!session->graph_animated_only || session->tracks[track].keys > 1))
            selected_joints[session->tracks[track].joint] = true;
    std::string channel_rows;
    int previous_joint = -1;
    for (unsigned track = 0; track < session->track_count; ++track) {
        const unsigned joint = session->tracks[track].joint;
        if (joint >= 1024 || !selected_joints[joint] ||
            (session->graph_animated_only && session->tracks[track].keys <= 1)) continue;
        if ((int) joint != previous_joint) {
            previous_joint = (int) joint;
            channel_rows += "<div class='workshop-action-group'>" +
                            Rml::StringUtilities::EncodeRml(bone_name(joint)) + "</div>";
        }
        unsigned color = track % 8;
        for (unsigned curve = 0; curve < count; ++curve)
            if (curves[curve].track == track) {
                color = curve % 8;
                break;
            }
        channel_rows += "<button id='workshop-graph-channel-" +
            number(track) + "' class='workshop-graph-channel" +
            (track == (unsigned) session->selected_track ? " selected" : "") +
            "'><span class='workshop-graph-swatch' style='border-color:" +
            colors[color] + "'></span><span>" +
            WorkshopLabels::channel(session->tracks[track].channel) +
            "</span></button>";
    }
    if (channel_rows.empty())
        channel_rows = "<div class='workshop-stack-empty'>No animated "
                       "channels selected.</div>";
    document->GetElementById("workshop-graph-channels")
        ->SetInnerRML(channel_rows);
    set_text("workshop-graph-selection-count",
        session->graph_selection.empty()
            ? "No keys selected"
            : number((unsigned) session->graph_selection.size()) +
              (session->graph_selection.size() == 1 ? " key selected" : " keys selected"));
    set_text("workshop-graph-active-channel",
        session->selected_track >= 0 && (size_t) session->selected_track < session->track_count
            ? bone_name(session->tracks[session->selected_track].joint) + " · " +
              WorkshopLabels::channel(session->tracks[session->selected_track].channel)
            : "");
    set_text("workshop-graph-note", number(count) + " channels");

}

void set_graph_tool(GraphTool tool)
{
    session->graph_tool = tool;
    static const char* names[] = { "Move", "Box select", "Rotate", "Scale" };
    set_text("workshop-editor-status", std::string(names[(int) tool]) +
        " keys: drag selected keys in the Curves graph.");
}

void show_animation_dopesheet()
{
    if (document == nullptr) return;
    Rml::Element* sheet = document->GetElementById("workshop-animation-dopesheet");
    document->GetElementById("workshop-graph-content")
        ->SetProperty("display", session->show_animation_curves ? "flex" : "none");
    document->GetElementById("workshop-graph-toolbar")
        ->SetProperty("display", session->show_animation_curves ? "flex" : "none");
    document->GetElementById("workshop-graph-readout")
        ->SetProperty("display", session->show_animation_curves ? "flex" : "none");
    document->GetElementById("workshop-graph-help")
        ->SetProperty("display", session->show_animation_curves ? "block" : "none");
    if (session->project == nullptr || session->animation_clip.index == UINT_MAX || session->clip_frames <= 0) {
        sheet->SetProperty("display", "none");
        return;
    }
    sheet->SetProperty("display", session->show_animation_curves ? "none" : "flex");
    if (session->show_animation_curves) return;
    std::string labels = "<div class='workshop-dope-heading'>CHANNELS</div>";
    std::string ruler;
    std::string rows;
    const int end = (int) std::ceil(session->clip_frames);
    const int step = std::max(1, (end + 7) / 8);
    for (int frame = 0; frame <= end; frame += step) {
        const float percent = 100.0f * frame / std::max(1, end);
        char style[48];
        std::snprintf(style, sizeof(style), "left:%.2f%%", percent);
        ruler += "<button id='workshop-dope-frame-" + number(frame) +
                 "' class='workshop-dope-tick' style='" + style +
                 "'>" + number(frame) + "</button>";
    }
    unsigned visible = 0;
    for (unsigned track = 0; track < session->track_count; ++track) {
        if (!track_selected(track)) continue;
        NativeFighterAnimationKey listed[1024];
        size_t count = 0;
        if (!nativeFighterAnimation_Keys(session->project, session->animation_clip,
                track, listed, 1024, &count)) continue;
        labels += "<button id='workshop-dope-track-" + number(track) +
                  "' class='workshop-dope-label" +
                  (track == (unsigned) session->selected_track ? " selected" : "") +
                  "'>Joint " + number(session->tracks[track].joint) + " · " +
                  WorkshopLabels::channel(session->tracks[track].channel) + "</button>";
        rows += "<div class='workshop-dope-row'>";
        for (unsigned key = 0; key < count; ++key) {
            const float percent = 100.0f * listed[key].frame /
                                  std::max(1, end);
            char style[48];
            std::snprintf(style, sizeof(style), "left:%.2f%%", percent);
            const bool chosen = std::any_of(session->graph_selection.begin(),
                session->graph_selection.end(), [track, key](const GraphKeyRef& ref) {
                    return ref.track == track && ref.key == key;
                });
            rows += "<button id='workshop-dope-key-" + number(track) +
                    "-" + number(key) + "' class='workshop-dope-key" +
                    (chosen ? " selected" : "") + "' style='" + style +
                    "' title='Frame " + number(listed[key].frame) +
                    "'></button>";
        }
        rows += "</div>";
        ++visible;
    }
    if (visible == 0) {
        labels += "<div class='workshop-dope-label'>Select a bone channel</div>";
        rows = "<div class='workshop-dope-row'></div>";
    }
    document->GetElementById("workshop-dope-labels")->SetInnerRML(labels);
    document->GetElementById("workshop-dope-ruler")->SetInnerRML(ruler);
    rows += "<div id='workshop-dope-selection-box' class='workshop-graph-selection-box' style='display:none'/>";
    document->GetElementById("workshop-dope-rows")->SetInnerRML(rows);
    const float percent = 100.0f * session->animation_frame / std::max(1, end);
    char style[48];
    std::snprintf(style, sizeof(style), "left:%.2f%%", percent);
    document->GetElementById("workshop-dope-playhead")
        ->SetAttribute("style", style);
}

void select_key(unsigned key, bool seek)
{
    if (key >= session->key_count) return;
    stop_action_preview();
    session->selected_key = (int) key;
    const NativeFighterAnimationKey& item = session->keys[key];
    if (seek) {
        session->animation_frame = item.frame;
        field("workshop-animation-current-frame")->SetValue(number((unsigned) session->animation_frame));
    }
    document->GetElementById("workshop-animation-fields")
        ->SetProperty("display", "block");
    set_text("workshop-key-description", bone_name(session->tracks[session->selected_track].joint) + " · " +
             WorkshopLabels::channel(session->tracks[session->selected_track].channel) + " · frame " +
             std::to_string(item.frame) +
             (item.editable ? "" : " · original encoding is read-only"));
    field("workshop-key-frame")->SetValue(std::to_string(item.frame));
    field("workshop-key-value")->SetValue(float_text(item.value));
    field("workshop-key-tangent")->SetValue(float_text(item.tangent));
    show_animation_keys();
    show_bone_transform();
    show_animation_dopesheet();
}

void GraphBoxEvents::ProcessEvent(Rml::Event& event)
{
    if (session->graph_tool != GraphTool::Box ||
        session->mode != WorkshopMode::Animation || session->animation_clip.index == UINT_MAX) return;
    const bool curves = session->show_animation_curves;
    Rml::Element* surface = document->GetElementById(
        curves ? "workshop-animation-graph" : "workshop-dope-rows");
    if (event.GetTargetElement() != surface) return;
    const Rml::Vector2f origin = surface->GetAbsoluteOffset();
    const int x = event.GetParameter<int>("mouse_x", 0) - (int) origin.x;
    const int y = event.GetParameter<int>("mouse_y", 0) - (int) origin.y;
    if (event.GetType() == "dragstart") {
        session->graph_box_start_x = x;
        session->graph_box_start_y = y;
        session->graph_box_active = true;
    }
    if (!session->graph_box_active) return;
    if (event.GetType() == "dragstart" || event.GetType() == "drag") {
        Rml::Element* box = document->GetElementById(
            curves ? "workshop-graph-selection-box" : "workshop-dope-selection-box");
        if (box == nullptr) return;
        box->SetProperty("display", "block");
        box->SetProperty("left", number((unsigned) std::max(0,
            std::min(x, session->graph_box_start_x))) + "px");
        box->SetProperty("top", number((unsigned) std::max(0,
            std::min(y, session->graph_box_start_y))) + "px");
        box->SetProperty("width", number((unsigned) std::abs(x -
            session->graph_box_start_x)) + "px");
        box->SetProperty("height", number((unsigned) std::abs(y -
            session->graph_box_start_y)) + "px");
        return;
    }
    if (event.GetType() != "dragend") return;
    session->graph_box_active = false;
    const float min_x = float(std::min(x, session->graph_box_start_x));
    const float max_x = float(std::max(x, session->graph_box_start_x));
    const float min_y = float(std::min(y, session->graph_box_start_y));
    const float max_y = float(std::max(y, session->graph_box_start_y));
    if (!event.GetParameter<int>("shift_key", 0)) session->graph_selection.clear();
    GraphKeyRef first = { UINT_MAX, UINT_MAX };
    const char* prefix = curves ? "workshop-graph-key-" : "workshop-dope-key-";
    for (unsigned track = 0; track < session->track_count; ++track) {
        if (!(curves ? session->graph_visible[track] : track_selected(track))) continue;
        for (unsigned key = 0; key < session->tracks[track].keys; ++key) {
            auto* marker = document->GetElementById(prefix + number(track) + "-" + number(key));
            if (!marker) continue;
            const auto point = marker->GetAbsoluteOffset(Rml::BoxArea::Border) - origin +
                               marker->GetBox().GetSize(Rml::BoxArea::Border) * 0.5f;
            if (point.x < min_x || point.x > max_x || point.y < min_y || point.y > max_y)
                continue;
            const auto existing = std::find_if(session->graph_selection.begin(),
                session->graph_selection.end(), [track, key](const GraphKeyRef& ref) {
                    return ref.track == track && ref.key == key;
                });
            if (existing != session->graph_selection.end()) continue;
            session->graph_selection.push_back({ track, key });
            if (first.track == UINT_MAX) first = { track, key };
        }
    }
    if (first.track != UINT_MAX) {
        session->selected_track = (int) first.track;
        if (nativeFighterAnimation_Keys(session->project, session->animation_clip, first.track, session->keys, 1024,
                &session->key_count)) select_key(first.key);
    }
    show_animation_graph();
    show_animation_dopesheet();
    set_text("workshop-editor-status", number((unsigned) session->graph_selection.size()) +
             " graph keys selected.");
}

static void seek_graph(Rml::Event& event, Rml::Element* graph)
{
    const float x = float(event.GetParameter<int>("mouse_x", 0)) - graph->GetAbsoluteOffset().x;
    const float y = float(event.GetParameter<int>("mouse_y", 0)) - graph->GetAbsoluteOffset().y;
    const float ratio = std::clamp((x - 8) / std::max(1, session->graph_width - 18),0.0f,1.0f);
    session->graph_cursor_frame = session->graph_view_start + ratio * (session->graph_view_end - session->graph_view_start);
    session->graph_cursor_value = session->graph_value_high - (y - 7) *
        (session->graph_value_high - session->graph_value_low) / std::max(1,session->graph_height - 22);
    session->graph_cursor_x = x; session->graph_cursor_y = y;
    stop_action_preview();
    session->animation_frame = std::clamp((int) std::lround(session->graph_cursor_frame),0,(int) std::ceil(session->clip_frames));
    field("workshop-animation-current-frame")->SetValue(number(session->animation_frame));
    session->bone_pose_refresh = true;
    session->graph_refresh = true;
}

void GraphNavigationEvents::ProcessEvent(Rml::Event& event)
{
    if (session->mode != WorkshopMode::Animation || !session->show_animation_curves ||
        session->animation_clip.index == UINT_MAX || session->clip_frames <= 0) return;
    Rml::Element* graph = document->GetElementById("workshop-animation-graph");
    const int width = graph->GetClientWidth();
    if (width <= 18) return;
    const auto type = event.GetType();
    if (type == "mouseup") {
        if (event.GetParameter<int>("button", -1) == 0) seeking = false;
        if (event.GetParameter<int>("button", -1) == 2) dragging = false;
        return;
    }
    if (type == "mousemove") {
        if (seeking) { seek_graph(event,graph); event.StopPropagation(); return; }
        if (!dragging) return;
        const int x = event.GetParameter<int>("mouse_x", last_x);
        const int y = event.GetParameter<int>("mouse_y", last_y);
        const float span = session->graph_view_end - session->graph_view_start;
        if (zooming) {
            const float next_span = std::clamp(span *
                cr_expf(float(y - last_y) * 0.012f),
                std::min(session->clip_frames, std::max(1.0f, session->clip_frames / 120.0f)),
                session->clip_frames);
            const float center = session->graph_view_start + span * 0.5f;
            session->graph_view_start = std::clamp(center - next_span * 0.5f,
                0.0f, session->clip_frames - next_span);
            session->graph_view_end = session->graph_view_start + next_span;
            const float value_center = (session->graph_value_low + session->graph_value_high) * .5f;
            const float half_range = (session->graph_value_high - session->graph_value_low) * .5f *
                                     cr_expf(float(y - last_y) * .012f);
            session->graph_value_low = value_center - half_range;
            session->graph_value_high = value_center + half_range;
        } else {
            session->graph_view_start = std::clamp(session->graph_view_start -
                float(x - last_x) * span / (width - 18.0f),
                0.0f, session->clip_frames - span);
            session->graph_view_end = session->graph_view_start + span;
        }
        if (!zooming && !session->normalize_values) {
            const float delta = float(y - last_y) * (session->graph_value_high - session->graph_value_low) /
                                std::max(1, session->graph_height - 22);
            session->graph_value_low += delta; session->graph_value_high += delta;
        }
        last_x = x;
        last_y = y;
        session->graph_refresh = true;
        event.StopPropagation();
        return;
    }
    if (type == "mousedown" && event.GetParameter<int>("button", -1) == 0 &&
        event.GetTargetElement() == graph && session->graph_tool != GraphTool::Box) {
        seeking = true; seek_graph(event,graph); event.StopPropagation(); return;
    }
    if (type == "mousedown" &&
        event.GetParameter<int>("button", -1) == 2) {
        dragging = true;
        zooming = event.GetParameter<int>("ctrl_key", 0) != 0;
        last_x = event.GetParameter<int>("mouse_x", 0);
        last_y = event.GetParameter<int>("mouse_y", 0);
        event.StopPropagation();
        return;
    }
    const float vertical = event.GetParameter<float>("wheel_delta_y", 0.0f);
    const float horizontal = event.GetParameter<float>("wheel_delta_x", 0.0f);
    if (vertical == 0.0f && horizontal == 0.0f) return;
    const float span = session->graph_view_end - session->graph_view_start;
    if (span <= 0) return;
    if (event.GetParameter<int>("shift_key", 0) || horizontal != 0.0f) {
        const float steps = horizontal +
            (event.GetParameter<int>("shift_key", 0) ? vertical : 0.0f);
        session->graph_view_start = std::clamp(session->graph_view_start + steps * span * 0.15f,
                                       0.0f, session->clip_frames - span);
        session->graph_view_end = session->graph_view_start + span;
    } else {
        const float min_span = std::min(session->clip_frames,
            std::max(1.0f, session->clip_frames / 120.0f));
        const float next_span = std::clamp(span * cr_powf(1.2f, vertical),
                                           min_span, session->clip_frames);
        const float x = float(event.GetParameter<int>("mouse_x", 0)) -
                        graph->GetAbsoluteOffset().x;
        const float ratio = std::clamp((x - 8.0f) / (width - 18.0f),
                                       0.0f, 1.0f);
        const float pointer_frame = session->graph_view_start + ratio * span;
        session->graph_view_start = std::clamp(pointer_frame - ratio * next_span,
                                       0.0f, session->clip_frames - next_span);
        session->graph_view_end = session->graph_view_start + next_span;
    }
    session->graph_refresh = true;
    event.StopPropagation();
}

void show_track_browser()
{
    std::string rows;
    const unsigned runtime_count = nativeHome_WorkshopBoneCount();
    const unsigned count = std::min(runtime_count != 0 ? runtime_count :
        session->animation_joint_count, 1024u);
    for (unsigned joint = 0; joint < count; ++joint) {
        unsigned depth = 0, parent = UINT_MAX;
        if (runtime_count && nativeHome_WorkshopBoneParent(joint, &parent)) {
            while (parent != UINT_MAX && depth < 8) {
                ++depth;
                if (!nativeHome_WorkshopBoneParent(parent, &parent)) break;
            }
        }
        rows += "<button id='workshop-joint-" + number(joint) +
                "' class='workshop-joint-row" +
                (joint < std::size(session->selected_bones) && session->selected_bones[joint]
                     ? " selected" : "") +
                ((int) joint == session->selected_outliner_joint ? " active" : "") +
                "' title='Shift-click to select multiple bones'" +
                " style='padding-left:" + number(7 + depth * 12) +
                "px'>" + (depth ? "&nbsp;&nbsp;" : "") +
                Rml::StringUtilities::EncodeRml(bone_name(joint)) + "</button>";
    }
    document->GetElementById("workshop-animation-tracks")->SetInnerRML(rows);
}

void refresh_track_selection()
{
    session->selected_key = -1;
    session->key_count = 0;
    document->GetElementById("workshop-animation-fields")
        ->SetProperty("display", "none");
    if (session->selected_track >= 0 && !nativeFighterAnimation_Keys(session->project, session->animation_clip,
                                     (unsigned) session->selected_track, session->keys, 1024,
                                     &session->key_count)) {
        set_text("workshop-editor-status", "This track has too many keys "
                 "or could not be decoded.");
        return;
    }
    show_track_browser();
    show_animation_keys();
    show_bone_transform();
    show_animation_graph();
    show_animation_dopesheet();
}

bool track_selected(unsigned track)
{
    if (track >= session->track_count) return false;
    const unsigned bone = session->tracks[track].joint;
    return bone < std::size(session->selected_bones) && session->selected_bones[bone];
}

void select_track(unsigned track)
{
    if (track >= session->track_count) return;
    const unsigned bone = session->tracks[track].joint;
    if (bone >= std::size(session->selected_bones)) return;
    session->selected_bones[bone] = true;
    session->selected_track = (int) track;
    session->selected_outliner_joint = (int) bone;
    refresh_track_selection();
}

void select_joint(unsigned joint, bool toggle)
{
    if (joint >= std::size(session->selected_bones)) return;
    auto& bones = session->selected_bones;
    if (!toggle) {
        std::fill(std::begin(bones), std::end(bones), false);
        session->graph_selection.clear();
    }
    bones[joint] = !toggle || !bones[joint];
    session->selected_outliner_joint = bones[joint] ? (int) joint : -1;
    session->selected_track = -1;
    std::erase_if(session->graph_selection,
                  [](const GraphKeyRef& key) { return !track_selected(key.track); });
    if (session->selected_outliner_joint < 0) {
        const auto* first = std::find(std::begin(bones), std::end(bones), true);
        if (first != std::end(bones))
            session->selected_outliner_joint = (int) (first - std::begin(bones));
    }
    for (unsigned track = 0; track < session->track_count; ++track) {
        if (!track_selected(track)) continue;
        if (session->selected_track < 0) session->selected_track = (int) track;
        if ((int) session->tracks[track].joint == session->selected_outliner_joint) {
            session->selected_track = (int) track;
            break;
        }
    }
    session->graph_value_range_valid = false;
    refresh_track_selection();
}

void select_all_bones(bool selected)
{
    auto& bones = session->selected_bones;
    std::fill(std::begin(bones), std::end(bones), false);
    if (selected) {
        const unsigned count = std::min<unsigned>(std::size(bones),
            std::max(session->animation_joint_count, nativeHome_WorkshopBoneCount()));
        std::fill_n(bones, count, true);
    }
    session->selected_track = selected && session->track_count ? 0 : -1;
    session->selected_outliner_joint = selected
        ? (session->selected_track < 0 ? 0 : (int) session->tracks[0].joint) : -1;
    session->graph_selection.clear();
    session->graph_value_range_valid = false;
    refresh_track_selection();
}

void show_animation()
{
    if (session->displayed_animation_clip.bank == session->animation_clip.bank &&
        session->displayed_animation_clip.index == session->animation_clip.index && session->track_count != 0) {
        session->animation_frame = std::clamp(session->animation_frame, 0,
                                     (int) std::ceil(session->clip_frames));
        field("workshop-animation-current-frame")->SetValue(
            number((unsigned) session->animation_frame));
        show_bone_transform();
        show_animation_dopesheet();
        show_animation_graph();
        return;
    }
    session->selected_track = session->selected_key = -1;
    session->track_count = session->key_count = 0;
    session->clip_frames = 0;
    session->animation_joint_count = 0;
    document->GetElementById("workshop-animation-tracks")->SetInnerRML("");
    document->GetElementById("workshop-animation-keys")->SetInnerRML("");
    auto* graph = static_cast<WorkshopCurveGraph*>(document->GetElementById("workshop-animation-graph"));
    graph->SetCurves({});
    graph->SetInnerRML("");
    document->GetElementById("workshop-dope-labels")->SetInnerRML("");
    document->GetElementById("workshop-dope-ruler")->SetInnerRML("");
    document->GetElementById("workshop-dope-rows")->SetInnerRML("");
    document->GetElementById("workshop-animation-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-bone-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-clip-fields")
        ->SetProperty("display", session->mode == WorkshopMode::Animation ? "block" : "none");
    if (session->project == nullptr || session->animation_clip.index == UINT_MAX) return;
    NativeFighterAnimationInfo clip;
    if (!nativeFighterAnimation_Info(session->project, session->animation_clip,
                                     &clip) ||
        !nativeFighterAnimation_Tracks(session->project, session->animation_clip,
                                       session->tracks, 1024, &session->track_count)) {
        set_text("workshop-animation-summary", "This clip has no editable "
                 "figatree clip in the draft.");
        return;
    }
    set_text("workshop-animation-summary", float_text(clip.frames) +
             " frames · " + number(clip.joints) + " joints · " +
             number(clip.tracks) + " tracks");
    set_text("workshop-animation-browser-summary", session->animation_name +
             std::string(" · ") + number(clip.joints) + " joints");
    session->clip_frames = clip.frames;
    session->animation_joint_count = clip.joints;
    field("workshop-clip-end")->SetValue(float_text(clip.frames));
    session->graph_view_start = 0.0f;
    session->graph_view_end = clip.frames;
    session->graph_value_range_valid = false;
    session->graph_selection.clear();
    session->animation_frame = std::clamp(session->animation_frame, 0,
                                 (int) std::ceil(session->clip_frames));
    field("workshop-animation-current-frame")->SetValue(
        number((unsigned) session->animation_frame));
    set_text("workshop-animation-frame-total", "/ " +
             number((unsigned) std::ceil(session->clip_frames)));
    session->displayed_animation_clip = session->animation_clip;
    if (session->selected_outliner_joint < 0 && session->track_count) {
        select_track(0);
    } else {
        for (unsigned t = 0; t < session->track_count; ++t) {
            if (track_selected(t)) { session->selected_track = (int) t; break; }
        }
        refresh_track_selection();
    }
}

void apply_clip_frames()
{
    if (session->project == nullptr || session->animation_clip.index == UINT_MAX || session->clip_frames < 0) return;
    float requested;
    if (!parse_float("workshop-clip-end", &requested) || requested < 0 ||
        requested > 1000) {
        set_text("workshop-editor-status", "Enter an end frame from 0 to 1000.");
        return;
    }
    if (requested == session->clip_frames) return;
    std::vector<uint8_t> before(session->project->animation_dat,
                                session->project->animation_dat + session->project->animation_size);
    if (!nativeFighterAnimation_SetFrames(session->project, session->animation_clip,
                                          requested)) {
        set_text("workshop-editor-status", "Could not change clip length. "
                 "The new end frame may precede an existing key.");
        return;
    }
    record_edit(true, before.data());
    session->animation_dirty = true;
    if (session->animation_clip.bank == NativeFighterAnimation_Action)
        session->action_clip_frames[session->animation_clip.index] = requested;
    else session->demo_clip_frames[session->animation_clip.index] = requested;
    session->displayed_animation_clip.index = UINT_MAX;
    stop_action_preview();
    show_animation();
    show_action_timeline();
    nativeShellWorkshopEditor_FilterClips();
    set_text("workshop-editor-status", "Clip end frame changed to " +
             float_text(requested) + ". Save the draft to keep the edit.");
}

void apply_key()
{
    if (session->project == nullptr || session->animation_clip.index == UINT_MAX || session->selected_track < 0 ||
        session->selected_key < 0 || (size_t) session->selected_key >= session->key_count) return;
    NativeFighterAnimationKey original = session->keys[session->selected_key];
    float value, tangent;
    const std::string frame_text = field("workshop-key-frame")->GetValue();
    char* frame_end = nullptr;
    const long frame = std::strtol(frame_text.c_str(), &frame_end, 10);
    if (frame_text.empty() || *frame_end != '\0' ||
        frame < 0 || frame > 65535) {
        set_text("workshop-editor-status", "Enter a frame from 0 to 65535.");
        return;
    }
    if (!parse_float("workshop-key-value", &value) ||
        !parse_float("workshop-key-tangent", &tangent)) {
        set_text("workshop-editor-status", "Enter finite numeric values.");
        return;
    }
    if ((!original.editable && value != original.value) ||
        (!original.tangent_editable && tangent != original.tangent)) {
        set_text("workshop-editor-status", "This key field is not editable "
                 "in its original encoding.");
        return;
    }
    const unsigned track = (unsigned) session->selected_track;
    const unsigned key = (unsigned) session->selected_key;
    if ((value != original.value && !nativeFighterAnimation_Snap(
            session->project, session->animation_clip, track, false, value, &value)) ||
        (tangent != original.tangent && !nativeFighterAnimation_Snap(
            session->project, session->animation_clip, track, true, tangent, &tangent)))
        return;
    if (frame == original.frame && value == original.value &&
        tangent == original.tangent) return;
    const size_t size = session->project->animation_size;
    std::vector<uint8_t> before(session->project->animation_dat,
                                session->project->animation_dat + size);
    std::vector<uint8_t> fighter;
    const NativeFighterAnimationKeyTime timing = {{track, key}, (int) frame};
    if (frame != original.frame)
        fighter.assign(session->project->fighter_dat,
                       session->project->fighter_dat + session->project->fighter_size);
    const bool valid = (fighter.empty() ||
        nativeFighterAnimation_EditKeys(session->project, session->animation_clip,
                                        &timing, 1, false)) &&
        (value == original.value ||
         nativeFighterAnimation_SetKey(session->project, session->animation_clip, track, key, value)) &&
        (tangent == original.tangent ||
         nativeFighterAnimation_SetTangent(session->project, session->animation_clip, track, key, tangent));
    if (!valid) {
        std::memcpy(session->project->animation_dat, before.data(), size);
        session->project->animation_size = size;
        if (!fighter.empty())
            std::memcpy(session->project->fighter_dat, fighter.data(), fighter.size());
        set_text("workshop-editor-status", "Keep keyframes in order and inside the clip.");
    } else {
        if (fighter.empty()) record_edit(true, before.data());
        else record_structural_edit(true, before.data(), size, fighter.data(), fighter.size());
        session->animation_dirty = true;
        set_text("workshop-editor-status", "Animation key updated.");
    }
    if (valid && nativeFighterAnimation_Keys(session->project, session->animation_clip, track,
                                             session->keys, 1024, &session->key_count)) {
        select_key(key);
        show_animation_graph();
    }
}

void insert_key_at_frame()
{
    if (session->project == nullptr || session->animation_clip.index == UINT_MAX || session->selected_track < 0 ||
        !std::isfinite(session->clip_frames) || session->animation_frame < 0 ||
        session->animation_frame > (int) session->clip_frames) return;
    const unsigned track = (unsigned) session->selected_track;
    float sampled;
    if (!nativeFighterAnimation_Sample(session->project, session->animation_clip, track,
                                       (float) session->animation_frame, &sampled) ||
        !nativeFighterAnimation_Snap(session->project, session->animation_clip, track,
                                     false, sampled, &sampled)) {
        set_text("workshop-editor-status", "Could not sample this transform "
                 "channel at the current frame.");
        return;
    }
    const size_t original_size = session->project->animation_size;
    std::vector<uint8_t> before(session->project->animation_dat,
                                session->project->animation_dat + original_size);
    std::vector<uint8_t> before_fighter(session->project->fighter_dat,
                                        session->project->fighter_dat + session->project->fighter_size);
    if (!nativeFighterAnimation_InsertKey(session->project, session->animation_clip, track,
                                          session->animation_frame, sampled)) {
        set_text("workshop-editor-status", "Could not insert a key here. "
                 "Choose a frame between existing keys in this track.");
        return;
    }
    if (session->project->animation_size == original_size)
        record_edit(true, before.data());
    else record_structural_edit(true, before.data(), original_size, before_fighter.data(), before_fighter.size());
    session->animation_dirty = true;
    const int inserted_frame = session->animation_frame;
    session->displayed_animation_clip.index = UINT_MAX;
    show_animation();
    if (track < session->track_count) {
        select_track(track);
        for (size_t i = 0; i < session->key_count; ++i) {
            if (session->keys[i].frame == inserted_frame && session->keys[i].editable) {
                select_key((unsigned) i);
                break;
            }
        }
    }
    set_text("workshop-editor-status", "Key inserted into the draft at frame " +
             number((unsigned) inserted_frame) + ". Save to keep this edit.");
}

void delete_selected_keys()
{
    if (!session->project || session->graph_selection.empty()) return;
    auto keys = session->graph_selection;
    std::sort(keys.begin(), keys.end(), [](const GraphKeyRef& a, const GraphKeyRef& b) {
        return a.track < b.track || (a.track == b.track && a.key < b.key);
    });
    const size_t size = session->project->animation_size;
    std::vector<uint8_t> before(session->project->animation_dat,
                                session->project->animation_dat + size);
    std::vector<uint8_t> fighter(session->project->fighter_dat,
                                 session->project->fighter_dat + session->project->fighter_size);
    std::vector<NativeFighterAnimationKeyTime> edits;
    edits.reserve(keys.size());
    for (const auto& key : keys) edits.push_back({key, 0});
    if (!nativeFighterAnimation_EditKeys(session->project, session->animation_clip,
                                         edits.data(), edits.size(), true)) {
        set_text("workshop-editor-status", "Could not delete the selected animation keys.");
        return;
    }
    record_structural_edit(true, before.data(), size, fighter.data(), fighter.size());
    session->animation_dirty = true;
    const float start = session->graph_view_start, end = session->graph_view_end;
    session->displayed_animation_clip.index = UINT_MAX;
    show_animation();
    session->graph_view_start = start;
    session->graph_view_end = end;
    session->graph_refresh = true;
    set_text("workshop-editor-status", "Deleted " + number((unsigned) keys.size()) +
        (keys.size() == 1 ? " key." : " keys."));
}

void copy_pose_at_frame()
{
    if (session->project == nullptr || session->animation_clip.index == UINT_MAX || session->track_count == 0) return;
    NativeFighterPose captured;
    if (!nativeFighterPose_Capture(session->project, session->animation_clip,
                                   (float) session->animation_frame, &captured)) {
        set_text("workshop-editor-status", "Could not evaluate this clip's "
                 "local bone transforms at the current frame.");
        return;
    }
    session->copied_pose = captured;
    session->pose_copied = true;
    set_text("workshop-pose-source",
             std::string(WorkshopLabels::action(session->animation_name.c_str())) +
             " · frame " + number((unsigned) session->animation_frame) + " · " +
             number(session->copied_pose.sampled_channels) + " channels");
    set_text("workshop-editor-status", "Pose copied. Choose a destination "
             "clip and paste at the current frame, start or end.");
}

void paste_copied_pose(const char* destination)
{
    if (session->project == nullptr || !session->pose_copied || session->animation_clip.index == UINT_MAX ||
        session->track_count == 0) {
        set_text("workshop-editor-status", "Copy a pose before pasting.");
        return;
    }
    int frame = session->animation_frame;
    if (std::strcmp(destination, "start") == 0) frame = 0;
    if (std::strcmp(destination, "end") == 0)
        frame = (int) std::floor(session->clip_frames);
    const bool selected_only = document->GetElementById(
        "workshop-pose-selected-only")->HasAttribute("checked");
    const bool include_root = document->GetElementById(
        "workshop-pose-root-translation")->HasAttribute("checked");
    unsigned char joints[NATIVE_FIGHTER_POSE_JOINTS] = {};
    for (size_t i = 0; i < session->track_count; ++i)
        if (track_selected((unsigned) i) && session->tracks[i].joint <
            NATIVE_FIGHTER_POSE_JOINTS) joints[session->tracks[i].joint] = 1;
    const size_t original_size = session->project->animation_size;
    std::vector<uint8_t> before(session->project->animation_dat,
                                session->project->animation_dat + original_size);
    std::vector<uint8_t> before_fighter(session->project->fighter_dat,
                                        session->project->fighter_dat + session->project->fighter_size);
    unsigned written = 0, skipped = 0, failed_track = UINT_MAX;
    if (!nativeFighterPose_Paste(session->project, &session->copied_pose,
                                session->animation_clip, frame, joints,
                                selected_only, include_root,
                                &written, &skipped, &failed_track)) {
        set_text("workshop-editor-status", "Pose paste could not be encoded "
                 "in the destination clip" +
                 (failed_track == UINT_MAX ? std::string() :
                    " (track " + number(failed_track) + ")") +
                 ". No channels were changed.");
        return;
    }
    if (session->project->animation_size == original_size)
        record_edit(true, before.data());
    else record_structural_edit(true, before.data(), original_size, before_fighter.data(), before_fighter.size());
    session->animation_dirty = true;
    session->displayed_animation_clip.index = UINT_MAX;
    show_animation();
    session->animation_frame = frame;
    field("workshop-animation-current-frame")->SetValue(
        number((unsigned) frame));
    show_bone_transform();
    show_animation_graph();
    show_animation_dopesheet();
    set_text("workshop-editor-status", "Pasted " + number(written) +
             " channels at frame " + number((unsigned) frame) +
             (skipped ? "; " + number(skipped) +
                            " channels had no matching destination track." :
                        ".") + " Save to keep this edit.");
}


} // namespace WorkshopEditor

void nativeShellWorkshopEditor_RegisterGraph()
{
    static Rml::ElementInstancerGeneric<WorkshopEditor::WorkshopCurveGraph> instancer;
    Rml::Factory::RegisterElementInstancer("workshop-curve-graph", &instancer);
}

#endif
