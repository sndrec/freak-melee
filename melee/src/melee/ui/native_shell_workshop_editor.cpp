#include "native_shell_workshop_editor_internal.h"
#include "native_shell_workshop_docs.h"
#include <melee/ft/native_fighter_model_editor.h>

#ifdef MELEE_NATIVE

namespace WorkshopEditor {

Rml::ElementDocument* document;
EditorSession sessions[2];
EditorSession* session = &sessions[0];
unsigned selected_member;
GraphKeyEvents graph_key_events;
GraphTangentEvents graph_tangent_events;
GraphBoxEvents graph_box_events;
GraphNavigationEvents graph_navigation_events;
ActionTimelineEvents action_timeline_events;
TimelineRulerEvents timeline_ruler_events;
WorkshopCameraEvents workshop_camera_events;
TimelineSplitterEvents timeline_splitter_events;

Rml::ElementFormControlInput* field(const char* id)
{
    return static_cast<Rml::ElementFormControlInput*>(
        document->GetElementById(id));
}

void set_text(const char* id, const std::string& value)
{
    document->GetElementById(id)->SetInnerRML(
        Rml::StringUtilities::EncodeRml(value));
}

void update_history_buttons()
{
    for (const auto& button : {
             std::pair<const char*, bool>{ "workshop-undo", session->edit_cursor > 0 },
             { "workshop-redo", session->edit_cursor < session->edit_history.size() } }) {
        Rml::Element* element = document->GetElementById(button.first);
        if (button.second) element->RemoveAttribute("disabled");
        else element->SetAttribute("disabled", "");
    }
}

void record_edit(bool animation, const uint8_t* before)
{
    if (session->project == nullptr || before == nullptr) return;
    const uint8_t* after = animation ? session->project->animation_dat :
                                         session->project->fighter_dat;
    const size_t size = animation ? session->project->animation_size :
                                    session->project->fighter_size;
    EditHistoryEntry edit = { animation, {}, {}, {} };
    for (size_t i = 0; i < size; ++i) {
        if (before[i] == after[i]) continue;
        edit.bytes.push_back({ (uint32_t) i, before[i], after[i] });
    }
    if (edit.bytes.empty()) return;
    session->edit_history.erase(session->edit_history.begin() + session->edit_cursor,
                       session->edit_history.end());
    if (session->edit_history.size() == 32) {
        session->edit_history.erase(session->edit_history.begin());
        --session->edit_cursor;
    }
    session->edit_history.push_back(std::move(edit));
    session->edit_cursor = session->edit_history.size();
    update_history_buttons();
}

void record_structural_edit(bool animation, const uint8_t* before, size_t size,
                            const uint8_t* before_fighter, size_t fighter_size)
{
    if (session->project == nullptr || before == nullptr || size == 0) return;
    const uint8_t* after = animation ? session->project->animation_dat :
                                         session->project->fighter_dat;
    const size_t after_size = animation ? session->project->animation_size :
                                          session->project->fighter_size;
    EditHistoryEntry edit = { animation, {},
        std::vector<uint8_t>(before, before + size),
        std::vector<uint8_t>(after, after + after_size) };
    if (animation && before_fighter != nullptr &&
        fighter_size == session->project->fighter_size &&
        std::memcmp(before_fighter, session->project->fighter_dat, fighter_size) != 0) {
        edit.before_fighter.assign(before_fighter, before_fighter + fighter_size);
        edit.after_fighter.assign(session->project->fighter_dat,
                                 session->project->fighter_dat + fighter_size);
        session->action_dirty = true;
    }
    session->edit_history.erase(session->edit_history.begin() + session->edit_cursor,
                       session->edit_history.end());
    if (session->edit_history.size() == 32) {
        session->edit_history.erase(session->edit_history.begin());
        --session->edit_cursor;
    }
    session->edit_history.push_back(std::move(edit));
    session->edit_cursor = session->edit_history.size();
    if (!animation) session->event_templates_dirty = true;
    update_history_buttons();
}

void record_pose_edit(const NativeWorkshopTransformChange* changes, unsigned count)
{
    if (!count || !session->project) return;
    bool different = false;
    for (unsigned i = 0; i < count; ++i)
        different |= std::memcmp(&changes[i].before, &changes[i].after,
                                 sizeof(changes[i].before)) != 0;
    if (!different) return;
    EditHistoryEntry edit = {};
    edit.poses.assign(changes, changes + count);
    edit.pose_clip = session->animation_clip;
    edit.pose_frame = session->animation_frame;
    session->edit_history.erase(session->edit_history.begin() + session->edit_cursor,
                                session->edit_history.end());
    if (session->edit_history.size() == 32) session->edit_history.erase(session->edit_history.begin());
    session->edit_history.push_back(std::move(edit));
    session->edit_cursor = session->edit_history.size();
    update_history_buttons();
}

std::string bone_name(unsigned joint)
{
    if (joint < 512 && !session->bone_names[joint].empty() &&
        session->bone_names[joint].compare(0, 6, "Joint ") != 0) return session->bone_names[joint];
    const char* role = nativeHome_WorkshopBoneRole(joint);
    if (role) return role;
    return "Joint " + std::to_string(joint);
}

std::string number(unsigned value)
{
    return std::to_string(value);
}

std::string float_text(float value)
{
    char result[32];
    std::snprintf(result, sizeof(result), "%.8g", value);
    return result;
}

std::string graph_tick_text(float value, float step)
{
    const int precision = std::clamp(int(std::ceil(-std::log10(std::max(step,1e-9f)))),0,9);
    if (std::fabs(value) < step * 1e-4f) value = 0;
    char result[32];
    std::snprintf(result, sizeof(result), "%.*f", precision, value);
    return result;
}

bool parse_float(const char* id, float* result)
{
    const std::string text = field(id)->GetValue();
    char* end;
    errno = 0;
    const float value = std::strtof(text.c_str(), &end);
    if (text.empty() || *end != '\0' || errno != 0 ||
        !std::isfinite(value)) return false;
    *result = value;
    return true;
}

bool parse_row(const char* id, const char* prefix, unsigned* index)
{
    const size_t length = std::strlen(prefix);
    if (std::strncmp(id, prefix, length) != 0) return false;
    char* end;
    errno = 0;
    const unsigned long value = std::strtoul(id + length, &end, 10);
    if (errno != 0 || end == id + length || *end != '\0' || value > UINT_MAX)
        return false;
    *index = (unsigned) value;
    return true;
}

bool parse_u16(const char* id, unsigned limit, uint16_t* value)
{
    const std::string text = field(id)->GetValue();
    char* end;
    errno = 0;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
    if (text.empty() || text[0] == '-' || errno != 0 ||
        *end != '\0' || parsed > limit) return false;
    *value = (uint16_t) parsed;
    return true;
}

bool parse_u32(const char* id, uint32_t* value)
{
    const std::string text = field(id)->GetValue();
    char* end;
    errno = 0;
    const unsigned long parsed = std::strtoul(text.c_str(), &end, 10);
    if (text.empty() || text[0] == '-' || errno != 0 ||
        *end != '\0' || parsed > UINT32_MAX) return false;
    *value = (uint32_t) parsed;
    return true;
}

bool parse_i16(const char* id, int16_t* value)
{
    const std::string text = field(id)->GetValue();
    char* end;
    errno = 0;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (text.empty() || errno != 0 || *end != '\0' ||
        parsed < INT16_MIN || parsed > INT16_MAX) return false;
    *value = (int16_t) parsed;
    return true;
}

void stop_action_preview()
{
    if (!session->action_preview_playing) return;
    session->action_preview_playing = false;
    set_text("workshop-frame-play", "Play");
    set_text("workshop-animation-frame-play", "Play");
}

void set_graph_focus(bool focused)
{
    session->graph_focused = focused;
    document->GetElementById("workshop-animation-pane")
        ->SetClass("focused", focused);
    document->GetElementById("workshop-graph-focus")
        ->SetClass("selected", focused);
    set_text("workshop-graph-focus", focused
        ? "Restore layout" : "Expand graph");
    if (session->mode == WorkshopMode::Animation) {
        for (const char* id : { "workshop-editor-browser",
                                "workshop-editor-inspector",
                                "workshop-animation-viewport" })
            document->GetElementById(id)->SetProperty("display",
                focused ? "none" : "block");
    }
    show_animation_graph();
}

void toggle_action_preview()
{
    if (session->action_preview_playing) {
        stop_action_preview();
        return;
    }
    const bool animation_mode = session->mode == WorkshopMode::Animation;
    const NativeFighterAnimationClip clip = animation_mode ? session->animation_clip :
        NativeFighterAnimationClip{ NativeFighterAnimation_Action, (unsigned) session->selected_action };
    const bool available = animation_mode ? clip.index != UINT_MAX :
        session->selected_action >= 0 && session->action_has_clip[session->selected_action];
    if (!available) {
        set_text("workshop-editor-status", "Choose an action with an "
                 "animation clip to preview.");
        return;
    }
    const float requested = (float) (session->mode == WorkshopMode::Action
        ? session->timeline_frame : session->animation_frame);
    const float frame = std::min(requested,
        std::max(0.0f, (animation_mode ? session->clip_frames :
            session->action_clip_frames[session->selected_action]) - 1.0f));
    if (!nativeHome_WorkshopPose(clip, frame)) {
        set_text("workshop-editor-status", "Press Test Character to load this "
                 "fighter in practice before playing it.");
        return;
    }
    session->action_preview_start_frame = (int) frame;
    session->action_preview_started = std::chrono::steady_clock::now();
    session->action_preview_playing = true;
    set_text("workshop-frame-play", "Pause");
    set_text("workshop-animation-frame-play", "Pause");
}

void select_mode(WorkshopMode next)
{
    stop_action_preview();
    if (next != WorkshopMode::Animation && session->graph_focused)
        set_graph_focus(false);
    const bool shared_action = session->animation_clip.bank == NativeFighterAnimation_Action &&
        session->animation_clip.index == (unsigned) session->selected_action;
    if (session->mode == WorkshopMode::Action && next == WorkshopMode::Animation && shared_action)
        session->animation_frame = session->timeline_frame;
    else if (session->mode == WorkshopMode::Animation && next == WorkshopMode::Action && shared_action) {
        session->timeline_frame = std::clamp(session->animation_frame, 0, session->timeline_end_frame);
        field("workshop-current-frame")->SetValue(
            number((unsigned) session->timeline_frame));
        update_timeline_cursor();
    }
    session->mode = next;
    ftDrawCommon_SetWorkshopOverlays(
        session->mode == WorkshopMode::Action && session->show_hitboxes,
        session->mode == WorkshopMode::Action && session->show_hurtboxes);
    Rml::Element* panel = document->GetElementById("workshop-panel");
    panel->SetClass("workshop-mode-action", session->mode == WorkshopMode::Action);
    panel->SetClass("workshop-mode-script", session->mode == WorkshopMode::Script);
    panel->SetClass("workshop-mode-animation", session->mode == WorkshopMode::Animation);
    panel->SetClass("workshop-mode-model", session->mode == WorkshopMode::Model);
    document->GetElementById("workshop-editor-footer")
        ->SetProperty("display", "flex");
    document->GetElementById("workshop-mode-action")
        ->SetClass("selected", session->mode == WorkshopMode::Action);
    document->GetElementById("workshop-mode-script")
        ->SetClass("selected", session->mode == WorkshopMode::Script);
    document->GetElementById("workshop-mode-animation")
        ->SetClass("selected", session->mode == WorkshopMode::Animation);
    document->GetElementById("workshop-mode-model")
        ->SetClass("selected", session->mode == WorkshopMode::Model);
    nativeShellWorkshopModel_Show(session->mode == WorkshopMode::Model);
    document->GetElementById("workshop-editor-browser")
        ->SetProperty("display", session->mode == WorkshopMode::Action ||
            session->mode == WorkshopMode::Animation ? "block" : "none");
    document->GetElementById("workshop-editor-inspector")
        ->SetProperty("display", session->mode == WorkshopMode::Action ||
            session->mode == WorkshopMode::Animation ? "block" : "none");
    document->GetElementById("workshop-editor-timeline")
        ->SetProperty("display", session->mode == WorkshopMode::Action ? "block" : "none");
    document->GetElementById("workshop-action-browser")
        ->SetProperty("display", session->mode == WorkshopMode::Action ? "block" : "none");
    document->GetElementById("workshop-animation-browser")
        ->SetProperty("display", session->mode == WorkshopMode::Animation ? "block" : "none");
    document->GetElementById("workshop-action-viewport")
        ->SetProperty("display", session->mode == WorkshopMode::Action ? "block" : "none");
    document->GetElementById("workshop-animation-viewport")
        ->SetProperty("display", session->mode == WorkshopMode::Animation ? "block" : "none");
    document->GetElementById("workshop-script-inspector")
        ->SetProperty("display", session->mode == WorkshopMode::Script ? "block" : "none");
    document->GetElementById("workshop-script-pane")
        ->SetProperty("display", session->mode == WorkshopMode::Script ? "flex" : "none");
    document->GetElementById("workshop-animation-pane")
        ->SetProperty("display", session->mode == WorkshopMode::Animation ? "block" : "none");
    document->GetElementById("workshop-event-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-value-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-sound-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-effect-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-event-timing")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-delete-event")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-animation-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-bone-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-action-playback")
        ->SetProperty("display", session->mode == WorkshopMode::Action ? "block" : "none");
    document->GetElementById("workshop-frame-stack")
        ->SetProperty("display", session->mode == WorkshopMode::Action ? "block" : "none");
    document->GetElementById("workshop-clip-fields")
        ->SetProperty("display", session->mode == WorkshopMode::Animation ? "block" : "none");
    document->GetElementById("workshop-pose-fields")
        ->SetProperty("display", session->mode == WorkshopMode::Animation ? "block" : "none");
    if (session->mode == WorkshopMode::Action) {
        set_text("workshop-inspector-heading", "EVENT INSPECTOR");
        set_text("workshop-action-title",
                 session->selected_action >= 0
                     ? WorkshopLabels::action(session->actions[session->selected_action].name)
                     : "Select an action");
        set_text("workshop-action-detail", "Action " +
                 number((unsigned) session->selected_action));
        if (session->selected_action >= 0) {
            set_text("workshop-context-action", WorkshopLabels::action(session->actions[session->selected_action].name));
            set_text("workshop-context-clip", session->actions[session->selected_action].name);
        }
        show_event();
    }
    if (session->mode == WorkshopMode::Script) show_script();
    if (session->mode == WorkshopMode::Animation) {
        set_text("workshop-inspector-heading", "ANIMATION INSPECTOR");
        set_text("workshop-action-title",
                 session->animation_clip.index != UINT_MAX
                     ? WorkshopLabels::action(session->animation_name.c_str())
                     : "Select a clip");
        set_text("workshop-context-action", session->animation_clip.bank == NativeFighterAnimation_Demo
            ? "Non-gameplay animation" : WorkshopLabels::action(session->animation_name.c_str()));
        set_text("workshop-context-clip", session->animation_name);
        nativeShellWorkshopEditor_FilterClips();
        show_animation();
        set_text("workshop-action-detail", std::string(session->animation_clip.bank == NativeFighterAnimation_Demo
            ? "Owned demo clip " : "Animation clip for action ") + number(session->animation_clip.index));
    }
    if (session->mode == WorkshopMode::Model) {
        set_text("workshop-context-action", "Model");
        set_text("workshop-context-phase", "Default costume");
        set_text("workshop-context-clip", "Compatible fighter rig");
        document->GetElementById("workshop-context-animation")
            ->SetProperty("display", "none");
    } else {
        document->GetElementById("workshop-context-animation")
            ->SetProperty("display", "block");
    }
}

} // namespace WorkshopEditor

using namespace WorkshopEditor;

void nativeShellWorkshopEditor_Init(Rml::ElementDocument* shell)
{
    document = shell;
    nativeWorkshopDocs_Init(shell);
    nativeWorkshopTransform_Init(shell);
    nativeWorkshopPose_Init(shell);
    nativeWorkshopPose_SetHistory(record_pose_edit);
    for (const char* type : { "mousedown", "mousemove", "mouseup", "click", "keydown" })
        shell->GetContext()->AddEventListener(type, &action_timeline_events, true);
    init_editor_input();
    nativeShellWorkshopBones_Init(shell);
    nativeShellWorkshopHitboxes_Init(shell);
    nativeShellWorkshopModel_Init(shell);
    for (const char* id : { "workshop-action-viewport",
                            "workshop-animation-viewport" }) {
        Rml::Element* viewport = shell->GetElementById(id);
        viewport->AddEventListener("mousedown", &workshop_camera_events);
        viewport->AddEventListener("mousescroll", &workshop_camera_events);
    }
    shell->AddEventListener("mousemove", &workshop_camera_events);
    shell->AddEventListener("mouseup", &workshop_camera_events);
    Rml::Element* graph = shell->GetElementById("workshop-animation-graph");
    graph->AddEventListener("dragstart", &graph_box_events);
    graph->AddEventListener("drag", &graph_box_events);
    graph->AddEventListener("dragend", &graph_box_events);
    graph->AddEventListener("mousescroll", &graph_navigation_events);
    graph->AddEventListener("click", &graph_navigation_events);
    graph->AddEventListener("mousedown", &graph_navigation_events);
    shell->AddEventListener("mousemove", &graph_navigation_events);
    shell->AddEventListener("mouseup", &graph_navigation_events);
    Rml::Element* action_ruler = shell->GetElementById(
        "workshop-event-timeline-ruler");
    for (const char* type : { "mousedown", "mousemove", "mouseup",
                              "dragstart", "drag", "dragend" })
        action_ruler->AddEventListener(type, &timeline_ruler_events);
    for (const char* id : { "workshop-action-splitter",
                            "workshop-animation-splitter" }) {
        Rml::Element* splitter = shell->GetElementById(id);
        splitter->AddEventListener("dragstart", &timeline_splitter_events);
        splitter->AddEventListener("dragend", &timeline_splitter_events);
    }
}

void nativeShellWorkshopEditor_Open(unsigned member, NativeFighterProject* fighter,
                                    const char* path)
{
    if (document == nullptr || member >= 2 || fighter == nullptr || path == nullptr)
        return;
    if (sessions[member].project == fighter &&
        std::strcmp(sessions[member].project_path, path) == 0) {
        nativeShellWorkshopEditor_Select(member);
        return;
    }
    capture_session_widgets();
    cancel_session_gestures();
    sessions[member].~EditorSession();
    new (&sessions[member]) EditorSession{};
    selected_member = member;
    session = &sessions[member];
    session->project = fighter;
    auto* model = (NativeFighterModelEditor*) std::malloc(sizeof(NativeFighterModelEditor));
    if (model && nativeFighterModel_Open(model, fighter))
        for (unsigned joint = 0; joint < model->joint_count; ++joint)
            session->bone_names[joint] = model->joints[joint].name;
    std::free(model);
    session->edit_history.clear();
    session->edit_cursor = 0;
    update_history_buttons();
    std::snprintf(session->project_path, sizeof(session->project_path), "%s", path);
    nativeShellWorkshopParameters_Open(document, session->project);
    session->saved_script.clear();
    session->pose_copied = false;
    set_text("workshop-pose-source", "No pose copied yet.");
    session->animation_dirty = false;
    session->action_dirty = false;
    session->script_loaded = false;
    WorkshopScriptFiles::close(session->script_files);
    session->graph_width = 0;
    session->graph_refresh = false;
    session->graph_focused = false;
    document->GetElementById("workshop-animation-pane")
        ->SetClass("focused", false);
    document->GetElementById("workshop-graph-focus")
        ->SetClass("selected", false);
    set_text("workshop-graph-focus", "Expand graph");
    session->graph_drag_active = false;
    graph_navigation_events.Reset();
    session->graph_box_active = false;
    session->graph_animated_only = true;
    document->GetElementById("workshop-graph-animated-only")
        ->SetClass("selected", true);
    session->animation_frame = 0;
    session->show_animation_curves = true;
    document->GetElementById("workshop-animation-curves")
        ->SetClass("selected", true);
    document->GetElementById("workshop-animation-dope")
        ->SetClass("selected", false);
    session->show_event_list = false;
    session->hitbox_lanes_expanded = true;
    session->action_preview_playing = false;
    set_text("workshop-frame-play", "Play");
    set_text("workshop-animation-frame-play", "Play");
    session->add_event_menu_open = false;
    session->event_templates_dirty = true;
    document->GetElementById("workshop-add-event-menu")
        ->SetProperty("display", "none");
    session->timeline_frame = 0;
    session->animation_clip = { NativeFighterAnimation_Action, UINT_MAX };
    session->animation_name.clear();
    session->displayed_animation_clip.index = UINT_MAX;
    session->mode = WorkshopMode::Action;
    session->show_hitboxes = true;
    session->show_hurtboxes = false;
    session->show_action_bones = false;
    session->show_animation_bones = true;
    nativeShellWorkshopBones_Reset();
    document->GetElementById("workshop-view-hitboxes")
        ->SetClass("selected", true);
    document->GetElementById("workshop-view-hurtboxes")
        ->SetClass("selected", false);
    document->GetElementById("workshop-view-bones")
        ->SetClass("selected", false);
    document->GetElementById("workshop-animation-show-bones")
        ->SetClass("selected", true);
    session->action_count = 0;
    session->selected_action = -1;
    session->skeleton_hierarchy_available = false;
    std::fill_n(session->action_group_open, 5, false);
    field("workshop-action-search")->SetValue("");
    field("workshop-animation-search")->SetValue("");
    const bool actions_available = nativeFighterAction_List(
        session->project, session->actions, 1024, &session->action_count);
    if (!actions_available) session->action_count = 0;
    for (size_t i = 0; i < session->action_count; ++i) {
        NativeFighterActionMetadata metadata = {};
        NativeFighterAnimationInfo info = {};
        session->action_has_clip[i] = nativeFighterAction_GetMetadata(session->project,
            (unsigned) i, &metadata) && metadata.animation_size >= 32 &&
            nativeFighterAnimation_Info(session->project, NativeFighterAnimationClip{ NativeFighterAnimation_Action, (unsigned) i }, &info);
        session->action_clip_frames[i] = session->action_has_clip[i] ? info.frames : 0.0f;
    }
    read_demo_clips();
    const bool model_available = nativeShellWorkshopModel_Open(
        selected_member, session->project, session->actions,
        session->action_has_clip, session->action_clip_frames, session->action_count);
    show_script();
    unsigned first = 0;
    for (size_t i = 0; i < session->action_count; ++i) {
        if (std::strcmp(session->actions[i].name, "AttackAirLw") == 0) {
            first = (unsigned) i;
            break;
        }
    }
    if (session->action_count != 0) select_action(first);
    else {
        nativeShellWorkshopEditor_FilterActions();
        nativeShellWorkshopEditor_FilterClips();
        show_commands();
        show_action_timeline();
        show_event();
    }
    select_mode(session->action_count != 0 ? WorkshopMode::Action : WorkshopMode::Script);
    if (!actions_available)
        set_text("workshop-editor-status", model_available
            ? "This member's actions are unavailable. Script and model editing remain available."
            : "This member's actions and model are unavailable. Script editing remains available.");
    else if (!model_available)
        set_text("workshop-editor-status", "This member's model is unavailable. Script and action editing remain available.");
    capture_session_widgets();
}

void nativeShellWorkshopEditor_Close()
{
    cancel_session_gestures();
    nativeWorkshopPose_Clear();
    nativeShellWorkshopModel_Close();
    workshop_camera_events.Reset();
    stop_action_preview();
    nativeHome_WorkshopPoseRelease();
    nativeShellWorkshopBones_Reset();
    nativeShellWorkshopHitboxes_Update(false, -1, nullptr, 0);
    document->GetElementById("workshop-editor-footer")
        ->SetProperty("display", "none");
    ftDrawCommon_SetWorkshopOverlays(false, false);
    session->project = nullptr;
    session->edit_history.clear();
    session->edit_cursor = 0;
    update_history_buttons();
    session->action_count = session->event_count = session->command_count = 0;
    session->selected_action = session->selected_event = session->selected_command = -1;
    session->animation_dirty = false;
    session->action_dirty = false;
    session->script_loaded = false;
    WorkshopScriptFiles::close(session->script_files);
    session->saved_script.clear();
    session->pose_copied = false;
    session->add_event_menu_open = false;
    document->GetElementById("workshop-add-event-menu")
        ->SetProperty("display", "none");
    session->project_path[0] = '\0';
    session->displayed_animation_clip.index = UINT_MAX;
    for (auto& closed : sessions) {
        closed.~EditorSession();
        new (&closed) EditorSession{};
    }
    selected_member = 0;
    session = &sessions[0];
}

void nativeShellWorkshopEditor_FilterActions()
{
    if (document == nullptr || session->project == nullptr) return;
    const std::string search = field("workshop-action-search")->GetValue();
    static const char* group_names[] = {
        "Ground attacks", "Aerial attacks", "Specials",
        "Movement &amp; defense", "Other states"
    };
    std::string markup;
    for (unsigned group_index = 0; group_index < 5; ++group_index) {
      std::string rows;
      unsigned matches = 0;
      for (size_t i = 0; i < session->action_count; ++i) {
        const std::string name = session->actions[i].name;
        const unsigned group = WorkshopLabels::group(name.c_str());
        if (group != group_index) continue;
        std::string lower_name = name;
        std::string lower_label = WorkshopLabels::action(name.c_str());
        std::string lower_search = search;
        for (char& ch : lower_name) ch = (char) std::tolower((unsigned char) ch);
        for (char& ch : lower_label) ch = (char) std::tolower((unsigned char) ch);
        for (char& ch : lower_search) ch = (char) std::tolower((unsigned char) ch);
        if (!lower_search.empty() && lower_name.find(lower_search) ==
                                         std::string::npos &&
            lower_label.find(lower_search) == std::string::npos &&
            number((unsigned) i).find(lower_search) == std::string::npos)
            continue;
        ++matches;
        const bool active = (int) i == session->selected_action;
        unsigned hitboxes = 0;
        if (active) {
            bool ids[8] = {};
            for (size_t event = 0; event < session->event_count; ++event)
                if (session->events[event].kind == NativeFighterActionEvent_CreateHitbox &&
                    session->events[event].target_id < 8)
                    ids[session->events[event].target_id] = true;
            for (bool used : ids) hitboxes += used;
        }
        rows += "<button id='workshop-action-" + number((unsigned) i) +
                  "' class='workshop-action-row" +
                  (active ? " selected" : "") +
                  "'><span>" +
                  Rml::StringUtilities::EncodeRml(WorkshopLabels::action(name.c_str())) +
                  "</span>" +
                  (active && hitboxes != 0
                      ? "<small class='workshop-action-count'>" +
                        number(hitboxes) +
                        (hitboxes == 1 ? " hitbox" : " hitboxes") + "</small>"
                      : "") + "</button>";
      }
      if (matches == 0) continue;
      const bool expanded = !search.empty() || session->action_group_open[group_index];
      markup += "<button id='workshop-group-" + number(group_index) +
                "' class='workshop-action-group-button'><span>" +
                (expanded ? "⌄ " : "› ") + group_names[group_index] +
                "</span><small>" + number(matches) + "</small></button>";
      if (expanded) markup += rows;
    }
    document->GetElementById("workshop-action-list")->SetInnerRML(markup);
}

bool nativeShellWorkshopEditor_ScrubTimeline(const char* id, int mouse_x)
{
    const bool action = std::strcmp(id, "workshop-event-timeline-ruler") == 0;
    const bool animation = std::strcmp(id, "workshop-dope-ruler") == 0;
    if ((!action && !animation) || session->project == nullptr) return false;
    Rml::Element* ruler = document->GetElementById(id);
    const int width = ruler->GetClientWidth();
    if (width <= 0) return true;
    const int x = mouse_x - (int) ruler->GetAbsoluteOffset().x;
    stop_action_preview();
    if (action) {
        // Timeline event bars and the playhead use the first 92% of the
        // track, leaving room for the last frame's label and event handle.
        const int next_frame = std::clamp(
            int(std::lround(double(x) * session->timeline_end_frame /
                            (width * 0.92))),
            0, session->timeline_end_frame);
        if (next_frame == session->timeline_frame) return true;
        session->timeline_frame = next_frame;
        field("workshop-current-frame")->SetValue(
            number((unsigned) session->timeline_frame));
        update_timeline_cursor();
    } else {
        session->animation_frame = std::clamp(
            int(std::lround(double(x) * session->clip_frames / width)),
            0, (int) std::ceil(session->clip_frames));
        field("workshop-animation-current-frame")->SetValue(
            number((unsigned) session->animation_frame));
        show_bone_transform();
        show_animation_graph();
        show_animation_dopesheet();
    }
    return true;
}

bool nativeShellWorkshopEditor_Click(const char* id, bool shift)
{
    if (session->mode == WorkshopMode::Model &&
        nativeShellWorkshopModel_Click(id, shift))
    {
        return true;
    }
    unsigned index;
    unsigned joint;
    if (nativeShellWorkshopBones_Click(id, &joint)) {
        if (session->mode == WorkshopMode::Action && session->selected_event >= 0 &&
            session->events[session->selected_event].kind ==
                NativeFighterActionEvent_CreateHitbox) {
            field("workshop-hitbox-bone")->SetValue(number(joint));
            set_text("workshop-editor-status", "Attachment set to joint " +
                     number(joint) + ". Apply the hitbox edit to keep it.");
            return true;
        }
        if (session->mode == WorkshopMode::Action)
            select_mode(WorkshopMode::Animation);
        for (unsigned track = 0; track < session->track_count; ++track)
            if (session->tracks[track].joint == joint) {
                select_track(track);
                break;
            }
        return true;
    }
    if (std::strcmp(id, "workshop-view-bones") == 0 ||
        std::strcmp(id, "workshop-animation-show-bones") == 0) {
        bool& shown = std::strcmp(id, "workshop-view-bones") == 0
            ? session->show_action_bones : session->show_animation_bones;
        shown = !shown;
        document->GetElementById(id)->SetClass("selected", shown);
        return true;
    }
    if (std::strcmp(id, "workshop-viewport-reset") == 0) {
        stop_action_preview();
        session->timeline_frame = 0;
        field("workshop-current-frame")->SetValue("0");
        update_timeline_cursor();
        return true;
    }
    if (std::strcmp(id, "workshop-frame-play") == 0 ||
        std::strcmp(id, "workshop-animation-frame-play") == 0) {
        toggle_action_preview();
        return true;
    }
    if (std::strcmp(id, "workshop-view-hitboxes") == 0 ||
        std::strcmp(id, "workshop-view-hurtboxes") == 0) {
        if (std::strcmp(id, "workshop-view-hitboxes") == 0)
            session->show_hitboxes = !session->show_hitboxes;
        else session->show_hurtboxes = !session->show_hurtboxes;
        document->GetElementById("workshop-view-hitboxes")
            ->SetClass("selected", session->show_hitboxes);
        document->GetElementById("workshop-view-hurtboxes")
            ->SetClass("selected", session->show_hurtboxes);
        ftDrawCommon_SetWorkshopOverlays(session->show_hitboxes, session->show_hurtboxes);
        return true;
    }
    if (std::strcmp(id, "workshop-undo") == 0 ||
        std::strcmp(id, "workshop-redo") == 0) {
        const bool redo = std::strcmp(id, "workshop-redo") == 0;
        if (session->mode == WorkshopMode::Model) nativeShellWorkshopModel_Undo(redo);
        else apply_history(redo);
        return true;
    }
    if (std::strcmp(id, "workshop-hitbox-group") == 0) {
        session->hitbox_lanes_expanded = !session->hitbox_lanes_expanded;
        show_action_timeline();
        return true;
    }
    if (std::strcmp(id, "workshop-hitbox-overview") == 0) {
        session->hitbox_lanes_expanded = !session->hitbox_lanes_expanded;
        show_action_timeline();
        return true;
    }
    if (parse_row(id, "workshop-timeline-event-", &index)) {
        if (index < session->timeline_event_count) {
            const TimelineEventInstance occurrence = session->timeline_events[index];
            select_event(occurrence.event);
            session->timeline_frame = occurrence.frame;
            field("workshop-current-frame")->SetValue(
                number((unsigned) session->timeline_frame));
            update_timeline_cursor();
        }
        return true;
    }
    if (parse_row(id, "workshop-event-", &index)) {
        select_event(index);
        return true;
    }
    if (parse_row(id, "workshop-viewport-hitbox-", &index)) {
        const int chosen = active_hitbox_event(index);
        if (chosen >= 0) select_event((unsigned) chosen, false);
        return true;
    }
    if (std::strcmp(id, "workshop-timeline-graph") == 0 ||
        std::strcmp(id, "workshop-timeline-list") == 0) {
        session->show_event_list = std::strcmp(id, "workshop-timeline-list") == 0;
        document->GetElementById("workshop-event-timeline")->SetProperty(
            "display", session->show_event_list ? "none" : "flex");
        document->GetElementById("workshop-command-list")->SetProperty(
            "display", session->show_event_list ? "block" : "none");
        document->GetElementById("workshop-timeline-graph")->SetClass(
            "selected", !session->show_event_list);
        document->GetElementById("workshop-timeline-list")->SetClass(
            "selected", session->show_event_list);
        if (!session->show_event_list && session->selected_event >= 0 &&
            (session->events[session->selected_event].kind == NativeFighterActionEvent_SyncTimer ||
             session->events[session->selected_event].kind == NativeFighterActionEvent_AsyncTimer)) {
            session->selected_event = session->selected_command = -1;
            show_event();
        }
        show_frame_stack();
        return true;
    }
    if (std::strcmp(id, "workshop-frame-start") == 0 ||
        std::strcmp(id, "workshop-frame-back") == 0 ||
        std::strcmp(id, "workshop-frame-next") == 0) {
        stop_action_preview();
        session->timeline_frame = std::strcmp(id, "workshop-frame-start") == 0 ? 0 :
            std::strcmp(id, "workshop-frame-back") == 0
                ? std::max(0, session->timeline_frame - 1)
                : std::min(session->timeline_frame + 1, session->timeline_end_frame);
        field("workshop-current-frame")->SetValue(number((unsigned) session->timeline_frame));
        update_timeline_cursor();
        return true;
    }
    if (std::strcmp(id, "workshop-animation-frame-start") == 0 ||
        std::strcmp(id, "workshop-animation-frame-back") == 0 ||
        std::strcmp(id, "workshop-animation-frame-next") == 0) {
        stop_action_preview();
        const int delta = std::strcmp(id, "workshop-animation-frame-start") == 0
            ? -session->animation_frame :
              std::strcmp(id, "workshop-animation-frame-back") == 0 ? -1 : 1;
        session->animation_frame = std::clamp(session->animation_frame + delta, 0,
                                    (int) std::ceil(session->clip_frames));
        field("workshop-animation-current-frame")->SetValue(
            number((unsigned) session->animation_frame));
        show_bone_transform();
        show_animation_graph();
        show_animation_dopesheet();
        return true;
    }
    if (std::strcmp(id, "workshop-context-animation") == 0) {
        select_mode(WorkshopMode::Animation);
        return true;
    }
    if (parse_row(id, "workshop-script-file-", &index)) {
        select_script_file(index);
        return true;
    }
    if (std::strcmp(id, "workshop-script-variable-add") == 0) {
        const std::string name = field("workshop-script-variable-name")
            ->GetValue();
        const std::string type = static_cast<Rml::ElementFormControlSelect*>(
            document->GetElementById("workshop-script-variable-type"))
                ->GetValue();
        bool valid = !name.empty() &&
            ((name[0] >= 'A' && name[0] <= 'Z') ||
             (name[0] >= 'a' && name[0] <= 'z') || name[0] == '_');
        for (char ch : name)
            valid = valid && ((ch >= 'A' && ch <= 'Z') ||
                              (ch >= 'a' && ch <= 'z') ||
                              (ch >= '0' && ch <= '9') || ch == '_');
        std::string state = WorkshopScriptFiles::source(session->script_files, 0);
        for (size_t at = 0; valid && at < state.size();) {
            const size_t end = state.find('\n', at);
            std::string line = state.substr(at,
                (end == std::string::npos ? state.size() : end) - at);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line == "state bool " + name ||
                line == "state float " + name) valid = false;
            if (end == std::string::npos) break;
            at = end + 1;
        }
        if (!valid) {
            set_text("workshop-script-status", "Enter a unique variable name "
                     "using letters, digits, and underscores.");
            return true;
        }
        if (!state.empty() && state.back() != '\n') state += '\n';
        state += "state " + type + " " + name + "\n";
        WorkshopScriptFiles::set_source(session->script_files, 0, state);
        if (session->script_file_index == 0)
            static_cast<Rml::ElementFormControlTextArea*>(
                document->GetElementById("workshop-script-source"))
                ->SetValue(state);
        field("workshop-script-variable-name")->SetValue("");
        nativeShellWorkshopEditor_RefreshScriptOutline();
        return true;
    }
    if (parse_row(id, "workshop-script-variable-remove-", &index)) {
        combined_script();
        std::string state = WorkshopScriptFiles::source(session->script_files, 0);
        size_t begin = 0;
        for (unsigned line = 1; line < index && begin < state.size(); ++line) {
            const size_t next = state.find('\n', begin);
            if (next == std::string::npos) return true;
            begin = next + 1;
        }
        if (begin >= state.size()) return true;
        const size_t end = state.find('\n', begin);
        if (state.compare(begin, 6, "state ") != 0) return true;
        state.erase(begin, end == std::string::npos ?
            std::string::npos : end - begin + 1);
        WorkshopScriptFiles::set_source(session->script_files, 0, state);
        if (session->script_file_index == 0)
            static_cast<Rml::ElementFormControlTextArea*>(
                document->GetElementById("workshop-script-source"))
                ->SetValue(state);
        nativeShellWorkshopEditor_RefreshScriptOutline();
        return true;
    }
    if (parse_row(id, "workshop-script-variable-", &index)) {
        select_script_file(0);
        jump_to_script_line(index);
        return true;
    }
    if (parse_row(id, "workshop-script-line-", &index) ||
        parse_row(id, "workshop-script-problem-line-", &index)) {
        jump_to_script_line(index);
        return true;
    }
    if (std::strncmp(id, "workshop-graph-key-", 19) == 0) {
        unsigned track, key;
        char tail;
        if (std::sscanf(id + 19, "%u-%u%c", &track, &key, &tail) == 2 &&
            track < session->track_count) {
            if (!shift) session->graph_selection.clear();
            auto found = std::find_if(session->graph_selection.begin(),
                session->graph_selection.end(), [=](const GraphKeyRef& ref) {
                    return ref.track == track && ref.key == key;
                });
            if (found == session->graph_selection.end())
                session->graph_selection.push_back({ track, key });
            else if (shift && session->graph_selection.size() > 1)
                session->graph_selection.erase(found);
            session->selected_track = (int) track;
            if (nativeFighterAnimation_Keys(session->project, session->animation_clip, track, session->keys, 1024,
                    &session->key_count)) {
                show_animation_keys();
                select_key(key);
            }
            show_animation_graph();
        }
        return true;
    }
    if (std::strcmp(id, "workshop-mode-action") == 0) {
        select_mode(WorkshopMode::Action);
        return true;
    }
    if (std::strcmp(id, "workshop-mode-script") == 0) {
        select_mode(WorkshopMode::Script);
        return true;
    }
    if (std::strcmp(id, "workshop-mode-animation") == 0) {
        select_mode(WorkshopMode::Animation);
        return true;
    }
    if (std::strcmp(id, "workshop-mode-model") == 0) {
        select_mode(WorkshopMode::Model);
        return true;
    }
    if (std::strcmp(id, "workshop-model-edit-animation") == 0) {
        const int clip = nativeShellWorkshopModel_SelectedClip();
        if (clip >= 0) select_action((unsigned) clip);
        select_mode(WorkshopMode::Animation);
        return true;
    }
    if (std::strcmp(id, "workshop-jump-action") == 0) {
        select_mode(WorkshopMode::Action);
        return true;
    }
    if (std::strcmp(id, "workshop-jump-animation") == 0) {
        select_mode(WorkshopMode::Animation);
        return true;
    }
    if (std::strcmp(id, "workshop-jump-script") == 0) {
        select_mode(WorkshopMode::Script);
        return true;
    }
    if (parse_row(id, "workshop-action-", &index)) {
        select_action(index);
        return true;
    }
    if (parse_row(id, "workshop-group-", &index)) {
        if (index < 5) {
            session->action_group_open[index] = !session->action_group_open[index];
            nativeShellWorkshopEditor_FilterActions();
        }
        return true;
    }
    if (parse_row(id, "workshop-demo-", &index)) {
        select_demo_clip(index);
        return true;
    }
    if (parse_row(id, "workshop-clip-", &index)) {
        if (index < session->action_count && session->action_has_clip[index]) select_action(index);
        return true;
    }
    if (parse_row(id, "workshop-command-", &index)) {
        select_command(index);
        return true;
    }
    if (parse_row(id, "workshop-stack-command-", &index)) {
        select_command(index);
        return true;
    }
    if (std::strcmp(id, "workshop-event-timeline-lanes") == 0) {
            session->selected_event = -1;
        session->selected_command = -1;
        show_event();
        show_action_timeline();
        return true;
    }
    if (parse_row(id, "workshop-track-", &index)) {
        select_track(index, shift);
        return true;
    }
    if (parse_row(id, "workshop-graph-channel-", &index)) {
        select_track(index, true);
        return true;
    }
    if (parse_row(id, "workshop-joint-", &index)) {
        select_joint(index, shift);
        return true;
    }
    if (std::strcmp(id, "workshop-animation-dope") == 0 ||
        std::strcmp(id, "workshop-animation-curves") == 0) {
        session->show_animation_curves = std::strcmp(id,
            "workshop-animation-curves") == 0;
        document->GetElementById("workshop-animation-dope")
            ->SetClass("selected", !session->show_animation_curves);
        document->GetElementById("workshop-animation-curves")
            ->SetClass("selected", session->show_animation_curves);
        show_animation_dopesheet();
        if (session->show_animation_curves) show_animation_graph();
        return true;
    }
    if (parse_row(id, "workshop-dope-track-", &index)) {
        select_track(index, shift);
        return true;
    }
    if (parse_row(id, "workshop-dope-frame-", &index)) {
        stop_action_preview();
        session->animation_frame = (int) index;
        field("workshop-animation-current-frame")->SetValue(number(index));
        show_bone_transform();
        show_animation_dopesheet();
        show_animation_graph();
        return true;
    }
    if (std::strncmp(id, "workshop-dope-key-", 18) == 0) {
        unsigned track, key;
        char tail;
        if (std::sscanf(id + 18, "%u-%u%c", &track, &key, &tail) != 2 ||
            track >= session->track_count) return true;
        if (session->selected_track != (int) track) select_track(track);
        if (!nativeFighterAnimation_Keys(session->project, session->animation_clip,
                track, session->keys, 1024, &session->key_count) || key >= session->key_count)
            return true;
        if (!shift) session->graph_selection.clear();
        const auto existing = std::find_if(session->graph_selection.begin(),
            session->graph_selection.end(), [track, key](const GraphKeyRef& ref) {
                return ref.track == track && ref.key == key;
            });
        if (existing == session->graph_selection.end())
            session->graph_selection.push_back({ track, key });
        session->selected_track = (int) track;
        select_key(key);
        show_animation_graph();
        return true;
    }
    if (parse_row(id, "workshop-key-", &index)) {
        session->graph_selection.clear();
        session->graph_selection.push_back({ (unsigned) session->selected_track, index });
        select_key(index);
        show_animation_graph();
        return true;
    }
    if (std::strcmp(id, "workshop-key-apply") == 0) {
        apply_key();
        return true;
    }
    if (std::strcmp(id, "workshop-add-event") == 0) {
        toggle_add_event_menu();
        return true;
    }
    if (std::strcmp(id, "workshop-delete-event") == 0) {
        delete_selected_event();
        return true;
    }
    if (std::strcmp(id, "workshop-event-frame-apply") == 0) {
        apply_event_frame();
        return true;
    }
    if (parse_row(id, "workshop-add-command-", &index) && index < 59) {
        session->add_event_menu_open = false;
        document->GetElementById("workshop-add-event-menu")->SetProperty("display", "none");
        if (session->command_templates[index].found)
            insert_selected_event(session->command_templates[index].action, session->command_templates[index].offset);
        return true;
    }
    if (std::strcmp(id, "workshop-add-copy") == 0 ||
        parse_row(id, "workshop-add-kind-", &index)) {
        session->add_event_menu_open = false;
        document->GetElementById("workshop-add-event-menu")
            ->SetProperty("display", "none");
        if (std::strcmp(id, "workshop-add-copy") == 0) {
            if (session->selected_event >= 0 && (size_t) session->selected_event < session->event_count)
                insert_selected_event((unsigned) session->selected_action,
                                      session->events[session->selected_event].source_offset);
        } else if (index <= NativeFighterActionEvent_SetAirborneState &&
                   session->event_templates[index].found) {
            insert_selected_event(session->event_templates[index].action,
                                  session->event_templates[index].offset);
        }
        return true;
    }
    if (std::strcmp(id, "workshop-key-insert") == 0) {
        insert_key_at_frame();
        return true;
    }
    if (std::strcmp(id, "workshop-key-delete") == 0) {
        delete_selected_key();
        return true;
    }
    if (std::strcmp(id, "workshop-pose-copy") == 0) {
        copy_pose_at_frame();
        return true;
    }
    if (std::strcmp(id, "workshop-pose-paste-current") == 0 ||
        std::strcmp(id, "workshop-pose-paste-start") == 0 ||
        std::strcmp(id, "workshop-pose-paste-end") == 0) {
        paste_copied_pose(id + std::strlen("workshop-pose-paste-"));
        return true;
    }
    if (parse_row(id, "workshop-bone-key-", &index) && index < 9) {
        nativeWorkshopPose_InsertChannel((unsigned) session->selected_outliner_joint, index);
        return true;
    }
    if (std::strcmp(id, "workshop-bone-key") == 0) {
        key_bone_channels();
        return true;
    }
    if (std::strcmp(id, "workshop-action-loop-apply") == 0) {
        apply_action_playback();
        return true;
    }
    if (std::strcmp(id, "workshop-animation-autokey") == 0) {
        session->auto_key = !session->auto_key;
        document->GetElementById(id)->SetClass("selected", session->auto_key);
        show_bone_transform();
        return true;
    }
    if (std::strcmp(id, "workshop-script-validate") == 0) {
        validate_script();
        return true;
    }
    if (std::strcmp(id, "workshop-parameters-save") == 0) {
        nativeShellWorkshopParameters_Save(document, session->project, session->project_path);
        return true;
    }
    if (std::strcmp(id, "workshop-graph-focus") == 0) {
        if (!session->show_animation_curves) {
            session->show_animation_curves = true;
            document->GetElementById("workshop-animation-dope")
                ->SetClass("selected", false);
            document->GetElementById("workshop-animation-curves")
                ->SetClass("selected", true);
            show_animation_dopesheet();
        }
        set_graph_focus(!session->graph_focused);
        return true;
    }
    return false;
}

bool nativeShellWorkshopEditor_Shortcut(int key_identifier)
{
    if (session->project == nullptr) return false;
    if (session->mode != WorkshopMode::Animation ||
        !session->show_animation_curves) return false;
    if (key_identifier == Rml::Input::KI_A) {
        if (!session->graph_selection.empty()) session->graph_selection.clear();
        else {
        for (unsigned track = 0; track < session->track_count; ++track) {
            if (!session->graph_visible[track]) continue;
            NativeFighterAnimationKey listed[1024];
            size_t count;
            if (!nativeFighterAnimation_Keys(session->project, session->animation_clip, track, listed, 1024,
                    &count)) continue;
            for (unsigned key = 0; key < count &&
                 session->graph_selection.size() < 2048; ++key)
                if (listed[key].frame >= session->graph_view_start &&
                    listed[key].frame <= session->graph_view_end)
                    session->graph_selection.push_back({ track, key });
        }
        }
        show_animation_graph();
        return true;
    }
    switch (key_identifier) {
    case Rml::Input::KI_B: set_graph_tool(GraphTool::Box); return true;
    case Rml::Input::KI_G: set_graph_tool(GraphTool::Move); return true;
    case Rml::Input::KI_R: set_graph_tool(GraphTool::Rotate); return true;
    case Rml::Input::KI_S: set_graph_tool(GraphTool::Scale); return true;
    default: return false;
    }
}

void nativeShellWorkshopEditor_Update()
{
    if (session->project == nullptr) return;
    action_timeline_events.Update();
    if (session->mode == WorkshopMode::Model) {
        nativeWorkshopPose_Clear();
        nativeShellWorkshopModel_Update();
        return;
    }
    if (session->mode == WorkshopMode::Animation && session->clip_scroll_delay > 0 &&
        --session->clip_scroll_delay == 0) {
        const std::string id = std::string(session->animation_clip.bank == NativeFighterAnimation_Demo
            ? "workshop-demo-" : "workshop-clip-") + number(session->animation_clip.index);
        if (Rml::Element* selected = document->GetElementById(id.c_str()))
            selected->ScrollIntoView(false);
    }
    if (session->mode == WorkshopMode::Animation && !session->action_preview_playing &&
        !session->graph_focused)
    {
        nativeWorkshopPose_Begin(session->project, session->animation_clip,
                                 session->animation_frame);
    } else {
        nativeWorkshopPose_Clear();
        nativeWorkshopTransform_Clear();
    }
    const int selected_hitbox = session->selected_event >= 0 &&
        (size_t) session->selected_event < session->event_count &&
        session->events[session->selected_event].kind == NativeFighterActionEvent_CreateHitbox
        ? (int) session->events[session->selected_event].target_id : -1;
    if (session->mode == WorkshopMode::Animation && session->selected_outliner_joint >= 0) {
        unsigned parent;
        const bool available = nativeHome_WorkshopBoneParent(
            (unsigned) session->selected_outliner_joint, &parent);
        if (available != session->skeleton_hierarchy_available) {
            session->skeleton_hierarchy_available = available;
            show_track_browser();
            show_bone_transform();
        }
    }
    nativeShellWorkshopBones_Update(session->project,
        session->mode == WorkshopMode::Animation ? session->animation_clip :
            NativeFighterAnimationClip{ NativeFighterAnimation_Action, (unsigned) session->selected_action },
        session->mode == WorkshopMode::Animation ? session->selected_outliner_joint : -1,
        session->mode == WorkshopMode::Action && session->show_action_bones,
        session->mode == WorkshopMode::Animation && session->show_animation_bones &&
            !session->graph_focused);
    const bool animation_mode = session->mode == WorkshopMode::Animation;
    const NativeFighterAnimationClip preview_clip = animation_mode ? session->animation_clip :
        NativeFighterAnimationClip{ NativeFighterAnimation_Action, (unsigned) session->selected_action };
    const bool preview_available = animation_mode ? preview_clip.index != UINT_MAX :
        session->selected_action >= 0 && session->action_has_clip[session->selected_action];
    const float preview_frames = animation_mode ? session->clip_frames :
        (preview_available ? session->action_clip_frames[session->selected_action] : 0.0f);
    const char* preview_name = animation_mode ? session->animation_name.c_str() :
        (preview_available ? session->actions[session->selected_action].name : "");
    if (session->action_preview_playing) {
        const auto elapsed = std::chrono::steady_clock::now() -
            session->action_preview_started;
        const int frame = session->action_preview_start_frame + (int) std::floor(
            std::chrono::duration<float>(elapsed).count() * 60.0f);
        if (preview_available && frame < (int) std::ceil(preview_frames)) {
            if (session->mode == WorkshopMode::Action) {
                session->timeline_frame = std::min(frame, session->timeline_end_frame);
                field("workshop-current-frame")->SetValue(
                    number((unsigned) session->timeline_frame));
                update_timeline_cursor();
            } else if (session->mode == WorkshopMode::Animation) {
                session->animation_frame = std::min(frame,
                    (int) std::ceil(session->clip_frames));
                field("workshop-animation-current-frame")->SetValue(
                    number((unsigned) session->animation_frame));
                const float left = 100.0f * session->animation_frame /
                    std::max(1.0f, session->clip_frames);
                char style[48];
                std::snprintf(style, sizeof(style), "left:%.2f%%", left);
                document->GetElementById("workshop-dope-playhead")
                    ->SetAttribute("style", style);
                Rml::Element* graph_cursor = document->GetElementById(
                    "workshop-graph-playhead");
                if (graph_cursor != nullptr && session->graph_width > 18) {
                    const float x = 8.0f +
                        (session->animation_frame - session->graph_view_start) /
                        (session->graph_view_end - session->graph_view_start) *
                        (session->graph_width - 18.0f);
                    graph_cursor->SetProperty("left",
                        number((unsigned) std::max(0.0f, x)) + "px");
                }
            }
            set_text("workshop-preview-state",
                std::string(preview_name) + " · frame " +
                number((unsigned) frame) + " · playing");
            // Keep posing below: editor playback advances animation only.
        } else {
            stop_action_preview();
        }
    }
    bool pose_ready = false;
    if (session->mode == WorkshopMode::Action || session->mode == WorkshopMode::Animation ||
        session->mode == WorkshopMode::Script) {
        const int frame = session->mode == WorkshopMode::Animation ? session->animation_frame :
                                                          session->timeline_frame;
        if (preview_available && nativeHome_WorkshopPose(preview_clip,
                std::min((float) frame,
                         std::max(0.0f,
                             preview_frames - 1.0f)))) {
            pose_ready = true;
            if (session->mode == WorkshopMode::Action &&
                (session->preview_hitbox_cached_frame != session->timeline_frame ||
                 session->preview_hitbox_cached_action != session->selected_action)) {
                session->preview_hitbox_mask = 0;
                for (size_t i = 0; i < session->timeline_event_count; ++i) {
                    const auto& occurrence = session->timeline_events[i];
                    if (occurrence.frame > session->timeline_frame) break;
                    const NativeFighterActionEvent& event =
                        session->events[occurrence.event];
                    if (event.kind == NativeFighterActionEvent_ClearAllHitboxes)
                        session->preview_hitbox_mask = 0;
                    else if (event.kind == NativeFighterActionEvent_ClearHitbox &&
                             event.target_id < 4)
                        session->preview_hitbox_mask &= ~(1u << event.target_id);
                    else if (event.kind == NativeFighterActionEvent_CreateHitbox &&
                             event.target_id < 4 &&
                             nativeFighterAction_GetHitbox(session->project,
                                 (unsigned) session->selected_action,
                                 event.source_offset,
                                 &session->preview_hitboxes[event.target_id]))
                        session->preview_hitbox_mask |= 1u << event.target_id;
                }
                session->preview_hitbox_cached_frame = session->timeline_frame;
                session->preview_hitbox_cached_action = session->selected_action;
            }
            nativeHome_WorkshopPreviewHitboxes(session->preview_hitboxes,
                session->mode == WorkshopMode::Action ? session->preview_hitbox_mask : 0);
            set_text("workshop-preview-state",
                std::string(preview_name) + " · frame " +
                number((unsigned) frame) + (session->action_preview_playing ? " · playing" : " · paused preview"));
        }
        else nativeHome_WorkshopPoseRelease();
    } else {
        nativeHome_WorkshopPoseRelease();
    }
    if (session->mode == WorkshopMode::Animation && !session->action_preview_playing &&
        !session->graph_focused)
    {
        bool selected_bones[512] = {};
        for (size_t t = 0; t < session->track_count; ++t) {
            if (session->selected_tracks[t] && session->tracks[t].joint < 512) {
                selected_bones[session->tracks[t].joint] = true;
            }
        }
        if (session->selected_outliner_joint >= 0 && session->selected_outliner_joint < 512) {
            selected_bones[session->selected_outliner_joint] = true;
        }
        unsigned joints[512], count = 0;
        for (unsigned i = 0; i < 512; ++i) {
            if (selected_bones[i]) {
                joints[count++] = i;
            }
        }
        nativeWorkshopPose_Update(joints, count,
                                  (unsigned) session->selected_outliner_joint, session->auto_key,
                                  record_pose_keys);
        if (nativeWorkshopPose_ConsumeChanged()) {
            session->bone_pose_refresh = true;
        }
        if (pose_ready && session->bone_pose_refresh) {
            show_bone_transform();
            session->bone_pose_refresh = false;
        }
    }
    nativeShellWorkshopHitboxes_Update(session->mode == WorkshopMode::Action &&
        session->show_hitboxes, selected_hitbox, session->preview_hitboxes, session->preview_hitbox_mask);
    if (session->mode == WorkshopMode::Action) {
        const std::string entered = field("workshop-current-frame")->GetValue();
        char* end = nullptr;
        const long frame = std::strtol(entered.c_str(), &end, 10);
        if (!entered.empty() && *end == '\0' && frame >= 0 &&
            frame <= session->timeline_end_frame && session->timeline_frame != frame) {
            session->timeline_frame = (int) frame;
            update_timeline_cursor();
        }
        return;
    }
    if (session->mode == WorkshopMode::Script) {
        Rml::Element* source = document->GetElementById("workshop-script-source");
        document->GetElementById("workshop-script-lines")
            ->SetScrollTop(source->GetScrollTop());
        static unsigned watch_tick;
        if (++watch_tick % 6 != 0) return;
        if (!nativeHome_IsPractice()) {
            set_text("workshop-script-watch", "Enter practice to inspect P1.");
            return;
        }
        NativeHomePracticeWatch watch = {};
        if (!nativeHome_PracticeWatch(0, &watch)) {
            set_text("workshop-script-watch", "P1 has no active fighter.");
            return;
        }
        std::string markup =
            "<div class='workshop-watch-row'><span>P1 action</span><b>" +
            number(watch.motion_id) + "</b></div>" +
            "<div class='workshop-watch-row'><span>Animation frame</span><b>" +
            float_text(watch.animation_frame) + "</b></div>" +
            "<div class='workshop-watch-row'><span>Velocity X</span><b>" +
            float_text(watch.velocity_x) + "</b></div>" +
            "<div class='workshop-watch-row'><span>Velocity Y</span><b>" +
            float_text(watch.velocity_y) + "</b></div>" +
            "<div class='workshop-watch-row'><span>Grounded</span><b>" +
            std::string(watch.grounded ? "yes" : "no") +
            "</b></div>";
        document->GetElementById("workshop-script-watch")->SetInnerRML(markup);
        return;
    }
    if (session->mode != WorkshopMode::Animation || session->track_count == 0) return;
    const std::string entered = field("workshop-animation-current-frame")->GetValue();
    char* end = nullptr;
    const long requested = std::strtol(entered.c_str(), &end, 10);
    if (!entered.empty() && *end == '\0' && requested >= 0 &&
        requested <= (long) std::ceil(session->clip_frames) &&
        session->animation_frame != requested) {
        session->animation_frame = (int) requested;
        show_bone_transform();
        show_animation_graph();
        show_animation_dopesheet();
    }
    if (session->graph_refresh) {
        session->graph_refresh = false;
        show_animation_graph();
        return;
    }
    const int width = document->GetElementById("workshop-animation-graph")
        ->GetClientWidth();
    const int height = document->GetElementById("workshop-animation-graph")
        ->GetClientHeight();
    if (width >= 100 && (width != session->graph_width ||
        std::max(height, 140) != session->graph_height)) show_animation_graph();
}

bool nativeShellWorkshopEditor_SaveAnimation(const char* path)
{
    if (!session->animation_dirty) return true;
    if (session->project == nullptr ||
        !nativeFighterProject_SaveAnimation(path, session->project))
        return false;
    session->animation_dirty = false;
    session->action_dirty = false;
    return true;
}

void nativeShellWorkshopEditor_MarkFighterSaved()
{
    session->action_dirty = false;
}

bool nativeShellWorkshopEditor_SaveScriptDraft()
{
    if (!session->script_loaded) return true;
    combined_script();
    return WorkshopScriptFiles::save(session->script_files);
}

bool nativeShellWorkshopEditor_IsScriptMode()
{
    return session->project != nullptr && session->mode == WorkshopMode::Script;
}

bool nativeShellWorkshopEditor_IsAnimationMode()
{
    return session->project != nullptr && session->mode == WorkshopMode::Animation;
}

bool nativeShellWorkshopEditor_IsModelMode()
{
    return session->project != nullptr && session->mode == WorkshopMode::Model;
}

bool nativeShellWorkshopEditor_IsGraphFocused()
{
    return session->project != nullptr && session->graph_focused;
}

float nativeShellWorkshopEditor_TimelineFraction()
{
    return session->mode == WorkshopMode::Animation ? session->animation_timeline_fraction :
                                            session->action_timeline_fraction;
}

bool nativeShellWorkshopEditor_IsOpen()
{
    return session->project != nullptr;
}

void nativeShellWorkshopEditor_SetTesting(bool testing)
{
    if (testing) {
        workshop_camera_events.Reset();
        stop_action_preview();
        for (const char* id : {
                 "workshop-editor-header", "workshop-editor-context",
                 "workshop-editor-browser", "workshop-editor-inspector",
                 "workshop-editor-timeline", "workshop-action-viewport",
                 "workshop-animation-viewport", "workshop-animation-pane",
                 "workshop-script-pane", "workshop-script-inspector",
                 "workshop-editor-footer", "workshop-model-pane" })
            document->GetElementById(id)->SetProperty("display", "none");
        ftDrawCommon_SetWorkshopOverlays(false, false);
    } else if (session->project != nullptr) {
        document->GetElementById("workshop-editor-header")
            ->SetProperty("display", "flex");
        document->GetElementById("workshop-editor-context")
            ->SetProperty("display", "flex");
        select_mode(session->mode);
        if (session->graph_focused && session->mode == WorkshopMode::Animation)
            set_graph_focus(true);
    }
}


#endif
