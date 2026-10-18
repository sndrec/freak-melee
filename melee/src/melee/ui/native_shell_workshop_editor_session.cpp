#include "native_shell_workshop_editor_internal.h"

#ifdef MELEE_NATIVE

namespace WorkshopEditor {
namespace {
// These are editor-owned panels, not the family selector or draft metadata.
const char* const control_roots[] = {
    "workshop-editor-browser", "workshop-editor-inspector",
    "workshop-editor-timeline", "workshop-animation-pane",
    "workshop-script-pane", "workshop-script-inspector"
};
const char* const message_ids[] = {
    "workshop-editor-status", "workshop-script-status",
    "workshop-parameters-status", "workshop-script-problems",
    "workshop-script-watch", "workshop-pose-source",
    "workshop-animation-summary", "workshop-animation-browser-summary",
    "workshop-graph-note"
};
const char* const scroll_ids[] = {
    "workshop-editor-browser", "workshop-editor-inspector",
    "workshop-action-list", "workshop-animation-clips",
    "workshop-animation-tracks", "workshop-animation-keys",
    "workshop-graph-content", "workshop-graph-channels",
    "workshop-dope-content", "workshop-script-files",
    "workshop-script-outline", "workshop-script-variables",
    "workshop-script-inspector"
};

void capture_controls(Rml::Element* root)
{
    if (root == nullptr) return;
    const auto& tag = root->GetTagName();
    if (!root->GetId().empty() &&
        (tag == "input" || tag == "textarea" || tag == "select")) {
        EditorWidgetState saved;
        saved.id = root->GetId();
        saved.value = static_cast<Rml::ElementFormControl*>(root)->GetValue();
        saved.checked = root->HasAttribute("checked");
        saved.scroll_left = root->GetScrollLeft();
        saved.scroll_top = root->GetScrollTop();
        Rml::String selected;
        if (tag == "textarea")
            static_cast<Rml::ElementFormControlTextArea*>(root)->GetSelection(
                &saved.selection_start, &saved.selection_end, &selected);
        else if (tag == "input")
            static_cast<Rml::ElementFormControlInput*>(root)->GetSelection(
                &saved.selection_start, &saved.selection_end, &selected);
        session->widgets.push_back(std::move(saved));
    }
    // Exclude anonymous/internal text-input widget nodes.
    for (int i = 0; i < root->GetNumChildren(); ++i)
        capture_controls(root->GetChild(i));
}
}

void capture_script_view()
{
    if (!session->script_loaded || document == nullptr) return;
    session->script_views.resize(WorkshopScriptFiles::count(session->script_files));
    if (session->script_file_index >= session->script_views.size()) return;
    auto* source = static_cast<Rml::ElementFormControlTextArea*>(
        document->GetElementById("workshop-script-source"));
    auto& view = session->script_views[session->script_file_index];
    Rml::String selected;
    source->GetSelection(&view.selection_start, &view.selection_end, &selected);
    view.left = source->GetScrollLeft();
    view.top = source->GetScrollTop();
}

void restore_script_view()
{
    if (session->script_file_index >= session->script_views.size()) return;
    auto* source = static_cast<Rml::ElementFormControlTextArea*>(
        document->GetElementById("workshop-script-source"));
    const auto& view = session->script_views[session->script_file_index];
    source->SetSelectionRange(view.selection_start, view.selection_end);
    source->SetScrollLeft(view.left);
    source->SetScrollTop(view.top);
    document->GetElementById("workshop-script-lines")->SetScrollTop(view.top);
}

void capture_session_widgets()
{
    if (session->project == nullptr || document == nullptr) return;
    combined_script();
    capture_script_view();
    session->parameter_source = nativeShellWorkshopParameters_Text();
    session->widgets.clear();
    for (const char* id : control_roots)
        capture_controls(document->GetElementById(id));
    session->messages.clear();
    for (const char* id : message_ids) {
        if (auto* element = document->GetElementById(id))
            session->messages.push_back({ id, element->GetInnerRML() });
    }
    session->scrolls.clear();
    for (const char* id : scroll_ids) {
        if (auto* element = document->GetElementById(id))
            session->scrolls.push_back({ id, element->GetScrollLeft(), element->GetScrollTop() });
    }
    auto* focus = document->GetContext()->GetFocusElement();
    session->focus_id = focus != nullptr ? focus->GetId() : "";
}

void restore_session_widgets()
{
    // Restore values after structural rows and selected inspectors were rebuilt.
    // Invalid/unapplied values are intentionally not parsed or written to DAT.
    for (const auto& saved : session->widgets) {
        auto* element = document->GetElementById(saved.id);
        if (element == nullptr) continue;
        auto* control = static_cast<Rml::ElementFormControl*>(element);
        // RmlUi selects emit change even when SetValue selects the same option.
        // Rebuilt parameter rows already reflect this member's saved source;
        // restoring an identical value must not author a parameter override.
        if (control->GetValue() != saved.value) control->SetValue(saved.value);
        if (element->GetTagName() == "input") {
            if (saved.checked) element->SetAttribute("checked", "");
            else element->RemoveAttribute("checked");
            static_cast<Rml::ElementFormControlInput*>(element)->SetSelectionRange(
                saved.selection_start, saved.selection_end);
        } else if (element->GetTagName() == "textarea") {
            static_cast<Rml::ElementFormControlTextArea*>(element)->SetSelectionRange(
                saved.selection_start, saved.selection_end);
        }
        element->SetScrollLeft(saved.scroll_left);
        element->SetScrollTop(saved.scroll_top);
    }
    for (const auto& saved : session->messages)
        if (auto* element = document->GetElementById(saved.id))
            element->SetInnerRML(saved.markup);
    // Layout changed when replacing list/graph nodes. Update before restoring
    // offsets, otherwise RmlUi clamps them against the outgoing content height.
    document->GetContext()->Update();
    if (auto* focus = document->GetElementById(session->focus_id)) focus->Focus();
    // Focusing and relayout can move the caret or clamp a text control's scroll.
    // Restore raw control views last, including the unapplied parameter editor.
    for (const auto& saved : session->widgets) {
        auto* element = document->GetElementById(saved.id);
        if (element == nullptr) continue;
        if (element->GetTagName() == "textarea")
            static_cast<Rml::ElementFormControlTextArea*>(element)->SetSelectionRange(
                saved.selection_start, saved.selection_end);
        else if (element->GetTagName() == "input")
            static_cast<Rml::ElementFormControlInput*>(element)->SetSelectionRange(
                saved.selection_start, saved.selection_end);
        element->SetScrollLeft(saved.scroll_left);
        element->SetScrollTop(saved.scroll_top);
    }
    for (const auto& saved : session->scrolls)
        if (auto* element = document->GetElementById(saved.id)) {
            element->SetScrollLeft(saved.left);
            element->SetScrollTop(saved.top);
        }
    restore_script_view();
}

void cancel_session_gestures()
{
    stop_action_preview();
    finish_graph_transform(false);
    session->graph_drag_active = false;
    session->graph_box_active = false;
    session->graph_drag_baselines.clear();
    session->add_event_menu_open = false;
    graph_navigation_events.Reset();
    action_timeline_events.Reset();
    timeline_ruler_events.Reset();
    timeline_splitter_events.Reset();
    workshop_camera_events.Reset();
    nativeWorkshopPose_Clear();
    nativeWorkshopTransform_Clear();
    nativeHome_WorkshopPoseRelease(true);
    nativeShellWorkshopBones_Reset();
    nativeShellWorkshopHitboxes_Update(false, -1, nullptr, 0);
}
} // namespace WorkshopEditor

using namespace WorkshopEditor;

bool nativeShellWorkshopEditor_Select(unsigned member)
{
    if (document == nullptr || member >= 2 || sessions[member].project == nullptr)
        return false;
    if (member == selected_member) return true;
    capture_session_widgets();
    cancel_session_gestures();
    const bool model_available = nativeShellWorkshopModel_Select(member);
    selected_member = member;
    session = &sessions[member];
    field("workshop-action-search")->SetValue("");
    field("workshop-animation-search")->SetValue("");
    // Filters are needed before generating rows, not only after restoration.
    for (const auto& saved : session->widgets)
        if (saved.id == "workshop-action-search" || saved.id == "workshop-animation-search")
            field(saved.id.c_str())->SetValue(saved.value);
    nativeShellWorkshopParameters_SetText(document, session->parameter_source);
    static_cast<Rml::ElementFormControlTextArea*>(
        document->GetElementById("workshop-script-source"))->SetValue(
            WorkshopScriptFiles::source(session->script_files, session->script_file_index));
    show_script_files();
    nativeShellWorkshopEditor_RefreshScriptOutline();
    nativeShellWorkshopEditor_FilterActions();
    nativeShellWorkshopEditor_FilterClips();
    show_action_playback();
    show_commands();
    show_action_timeline();
    select_mode(session->mode);
    if (session->mode == WorkshopMode::Animation) {
        show_track_browser();
        show_animation_keys();
        if (session->selected_key >= 0) select_key((unsigned) session->selected_key);
    }
    set_graph_focus(session->graph_focused);
    document->GetElementById("workshop-event-timeline")->SetProperty(
        "display", session->show_event_list ? "none" : "flex");
    document->GetElementById("workshop-command-list")->SetProperty(
        "display", session->show_event_list ? "block" : "none");
    document->GetElementById("workshop-timeline-graph")->SetClass("selected", !session->show_event_list);
    document->GetElementById("workshop-timeline-list")->SetClass("selected", session->show_event_list);
    for (const auto& flag : {
        std::pair<const char*, bool>{ "workshop-graph-animated-only", session->graph_animated_only },
        { "workshop-animation-curves", session->show_animation_curves },
        { "workshop-animation-dope", !session->show_animation_curves },
        { "workshop-animation-autokey", session->auto_key },
        { "workshop-view-hitboxes", session->show_hitboxes },
        { "workshop-view-hurtboxes", session->show_hurtboxes },
        { "workshop-view-bones", session->show_action_bones },
        { "workshop-animation-show-bones", session->show_animation_bones } })
        document->GetElementById(flag.first)->SetClass("selected", flag.second);
    field("workshop-graph-normalize")->SetAttribute("checked", session->normalize_values);
    if (!session->normalize_values) field("workshop-graph-normalize")->RemoveAttribute("checked");
    field("workshop-graph-animated-only")->SetAttribute("checked", session->graph_animated_only);
    if (!session->graph_animated_only) field("workshop-graph-animated-only")->RemoveAttribute("checked");
    static_cast<Rml::ElementFormControlSelect*>(document->GetElementById("workshop-graph-pivot"))->SetValue(
        session->graph_cursor_pivot ? "cursor" : "selection");
    update_history_buttons();
    restore_session_widgets();
    if (!model_available)
        set_text("workshop-editor-status", "This member's model is unavailable. Script and action editing remain available.");
    return true;
}

bool nativeShellWorkshopEditor_HasUnsavedEdits(unsigned member)
{
    if (member >= 2 || sessions[member].project == nullptr) return false;
    auto& target = sessions[member];
    if (member == selected_member) {
        combined_script();
        target.parameter_source = nativeShellWorkshopParameters_Text();
    }
    const auto* fighter = target.project;
    const bool parameter_dirty = target.parameter_source.size() != fighter->parameter_size ||
        (!target.parameter_source.empty() &&
         (fighter->parameter_text == nullptr || std::memcmp(target.parameter_source.data(),
             fighter->parameter_text, target.parameter_source.size()) != 0));
    return target.action_dirty || target.animation_dirty || parameter_dirty ||
        nativeShellWorkshopModel_HasUnsavedEdits(member) ||
        (target.script_loaded && WorkshopScriptFiles::dirty(target.script_files));
}

bool nativeShellWorkshopEditor_ScriptChanged(unsigned member)
{
    if (member >= 2 || sessions[member].project == nullptr || !sessions[member].script_loaded)
        return false;
    auto& target = sessions[member];
    if (member == selected_member) combined_script();
    const std::string text = WorkshopScriptFiles::compose(target.script_files);
    const uint8_t* compiled = target.project->script_text;
    if (!compiled) return !text.empty();
    // The text widget uses LF. Compare source text with the same line endings
    // so opening a CRLF-authored package keeps its validated behavior status.
    size_t edited = 0, accepted = 0;
    for (;;) {
        while (edited < text.size() && text[edited] == '\r') ++edited;
        while (accepted < target.project->script_size && compiled[accepted] == '\r') ++accepted;
        if (edited == text.size() || accepted == target.project->script_size)
            return edited != text.size() || accepted != target.project->script_size;
        if ((uint8_t)text[edited++] != compiled[accepted++]) return true;
    }
}

#endif
