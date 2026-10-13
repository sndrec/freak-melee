#include "native_shell_workshop_editor_internal.h"

#ifdef MELEE_NATIVE
namespace WorkshopEditor {

    namespace {
        void refresh_moved_event(uint32_t source, int frame)
        {
            nativeFighterAction_Commands(
                session->project, session->selected_action, session->commands,
                512, &session->command_count);
            nativeFighterAction_Events(
                session->project, session->selected_action, session->events,
                256, &session->event_count);
            session->selected_event = session->selected_command = -1;
            for (size_t i = 0; i < session->event_count; ++i) {
                if (session->events[i].source_offset == source) {
                    session->selected_event = (int) i;
                    break;
                }
            }
            for (size_t i = 0; i < session->command_count; ++i) {
                if (session->commands[i].source_offset == source) {
                    session->selected_command = (int) i;
                    break;
                }
            }
            session->timeline_frame = frame;
            show_action_timeline();
            show_event();
            nativeHome_WorkshopPoseInvalidate();
        }
    }

    bool ActionTimelineEvents::Begin(unsigned event, int frame, bool keyboard)
    {
        if (active || event >= session->event_count) {
            return false;
        }
        stop_action_preview();
        session->selected_event = (int) event;
        session->selected_command = -1;
        original_source = selected_source =
            session->events[event].source_offset;
        auto* ruler =
            document->GetElementById("workshop-event-timeline-ruler");
        if (!ruler || ruler->GetClientWidth() <= 0 ||
            session->event_frames[event] < 0)
        {
            set_text("workshop-editor-status",
                     "This event has no resolved timeline frame.");
            return false;
        }
        // Freeze the screen-to-frame scale for the entire
        // gesture. Resizing the timeline or unrolling loops must not change
        // the drag baseline.
        frames_per_pixel =
            session->timeline_end_frame / (ruler->GetClientWidth() * 0.92f);
        initial_frame = session->timeline_frame;
        occurrence_frame = frame;
        was_dirty = session->action_dirty;
        edit.size = session->project->fighter_size;
        edit.before = (uint8_t*) std::malloc(edit.size);
        if (!edit.before) {
            return false;
        }
        std::memcpy(edit.before, session->project->fighter_dat, edit.size);
        edit.localized = true;
        edit.offset = original_source;
        start_x = last_x;
        preview_delta = 0;
        modal = keyboard;
        active = true;
        session->timeline_frame = frame;
        field("workshop-current-frame")->SetValue(number(frame));
        update_timeline_cursor();
        show_event();
        document->GetElementById("workshop-timeline-graph")->Focus();
        set_text("workshop-editor-status",
                 keyboard ? "Grab event: move the mouse, left click to "
                            "confirm, right click or Esc to cancel."
                          : "Move event: release to confirm, right "
                            "click or Esc to cancel.");
        return true;
    }

    void ActionTimelineEvents::Preview(int x)
    {
        if (!active) {
            return;
        }
        const int delta =
            std::clamp((int) std::lround((x - start_x) * frames_per_pixel),
                       -occurrence_frame, 1000 - occurrence_frame);
        if (delta == preview_delta) {
            return;
        }
        // Always compile from the gesture's immutable starting graph. Repeated
        // mouse moves must neither compound the delta nor grow the saved bank.
        NativeFighterProject candidate = *session->project;
        candidate.fighter_dat = (uint8_t*) std::malloc(edit.size);
        if (!candidate.fighter_dat) {
            return;
        }
        candidate.fighter_size = edit.size;
        std::memcpy(candidate.fighter_dat, edit.before, edit.size);
        uint32_t moved = original_source;
        const bool ok = delta == 0 || nativeFighterAction_MoveEvent(
                                          &candidate, session->selected_action,
                                          original_source, delta, &moved);
        if (!ok) {
            std::free(candidate.fighter_dat);
            set_text("workshop-editor-status",
                     "Keep this event within its loop interval. "
                     "Animation-dependent timing must be edited in the event "
                     "list.");
            return;
        }
        std::free(session->project->fighter_dat);
        session->project->fighter_dat = candidate.fighter_dat;
        session->project->fighter_size = candidate.fighter_size;
        selected_source = edit.offset = moved;
        preview_delta = delta;
        refresh_pending = true;
        field("workshop-current-frame")
            ->SetValue(number(occurrence_frame + delta));
        set_text("workshop-editor-status",
                 "Moving event; loop repetitions follow. Left click/release "
                 "confirms, right click or Esc cancels.");
    }

    void ActionTimelineEvents::Finish(bool commit)
    {
        if (!active) {
            return;
        }
        active = false;
        refresh_pending = false;
        const bool changed = commit && preview_delta != 0;
        if (changed) {
            commit_fighter_event_edit(&edit);
        } else {
            cancel_fighter_event_edit(&edit);
            session->action_dirty = was_dirty;
        }
        refresh_moved_event(changed ? selected_source : original_source,
                            commit ? occurrence_frame + preview_delta
                                   : initial_frame);
        document->GetElementById("workshop-timeline-graph")->Focus();
        set_text("workshop-editor-status",
                 changed  ? "Moved the event; other event times are unchanged."
                 : commit ? "Event selected."
                          : "Move cancelled; original timing restored.");
    }

    void ActionTimelineEvents::Reset()
    {
        Finish(false);
        discard_click = false;
    }

    void ActionTimelineEvents::Update()
    {
        if (!active || !refresh_pending) {
            return;
        }
        refresh_pending = false;
        // Rebuild outside input dispatch so the mouse-down target survives its
        // event. Refresh hover afterward so mouse-up has a live target, even
        // when the pointer stops moving over the rebuilt event bars.
        refresh_moved_event(selected_source, occurrence_frame + preview_delta);
        document->GetElementById("workshop-timeline-graph")->Focus();
        auto* context = document->GetContext();
        context->Update();
        context->ProcessMouseMove(last_x, last_y, 0);
    }

    void ActionTimelineEvents::ProcessEvent(Rml::Event& event)
    {
        if (!session->project || !nativeShellWorkshopEditor_IsOpen()) {
            return;
        }
        const auto type = event.GetType();
        auto* target = event.GetTargetElement();
        if (!target) {
            return;
        }
        if (type == "mousemove") {
            last_x = event.GetParameter<int>("mouse_x", last_x);
            last_y = event.GetParameter<int>("mouse_y", last_y);
            if (active) {
                Preview(last_x);
                event.StopPropagation();
            }
            return;
        }
        if (type == "click" && discard_click) {
            discard_click = false;
            event.StopPropagation();
            return;
        }
        if (type == "mousedown") {
            last_x = event.GetParameter<int>("mouse_x", last_x);
            last_y = event.GetParameter<int>("mouse_y", last_y);
            discard_click = false;
            const int button = event.GetParameter<int>("button", -1);
            if (active) {
                if (button == 1 || (button == 0 && modal)) {
                    if (button == 0) {
                        Preview(last_x);
                    }
                    Finish(button == 0);
                    discard_click = true;
                }
                event.StopPropagation();
                return;
            }
            if (button != 0 || session->mode != WorkshopMode::Action) {
                return;
            }
            unsigned index;
            if (parse_row(target->GetId().c_str(), "workshop-timeline-event-",
                          &index) &&
                index < session->timeline_event_count)
            {
                const auto occurrence = session->timeline_events[index];
                if (Begin(occurrence.event, occurrence.frame, false)) {
                    event.StopPropagation();
                }
            }
        } else if (type == "mouseup" && active && !modal &&
                   event.GetParameter<int>("button", -1) == 0)
        {
            Preview(event.GetParameter<int>("mouse_x", last_x));
            Finish(true);
            discard_click = true;
            event.StopPropagation();
        } else if (type == "keydown") {
            const int key = event.GetParameter<int>("key_identifier", 0);
            if (active) {
                if (key == Rml::Input::KI_ESCAPE ||
                    key == Rml::Input::KI_RETURN)
                {
                    Finish(key == Rml::Input::KI_RETURN);
                    discard_click = true;
                }
                event.StopPropagation();
                return;
            }
            if (session->mode != WorkshopMode::Action ||
                key != Rml::Input::KI_G ||
                event.GetParameter<int>("ctrl_key", 0) ||
                event.GetParameter<int>("alt_key", 0) ||
                target->GetTagName() == "input" ||
                target->GetTagName() == "textarea" ||
                target->GetTagName() == "select" ||
                session->selected_event < 0)
            {
                return;
            }
            // Use the selected occurrence's playhead, including repetitions in
            // loops.
            const unsigned selected_event = session->selected_event;
            int frame = session->event_frames[selected_event];
            int closest = INT_MAX;
            for (size_t i = 0; i < session->timeline_event_count; ++i) {
                const auto& occurrence = session->timeline_events[i];
                const int distance =
                    std::abs(occurrence.frame - session->timeline_frame);
                if (occurrence.event == selected_event && distance < closest) {
                    closest = distance;
                    frame = occurrence.frame;
                }
            }
            Begin(selected_event, frame, true);
            event.StopPropagation();
        }
    }
}
#endif
