#include "native_shell_workshop_editor_internal.h"
#include <melee/ft/native_fighter_model_references.h>
#include <melee/ft/native_action_properties.h>

#ifdef MELEE_NATIVE

namespace WorkshopEditor {

// The runtime command uses this exact scale for radius and bone offsets.
constexpr float HitboxDistanceScale = 0.003906f;

static bool visible_command(const NativeFighterActionCommand& command)
{
    if (command.opcode != 20) return true;
    const uint8_t* p = session->project->fighter_dat + 32 + command.source_offset;
    const uint32_t word = uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | p[3];
    return (word & 0x3ffffffu) <= 1;
}

void show_action_playback()
{
    const ActionInspectorRefresh refresh;
    if (session->project == nullptr || session->selected_action < 0) return;
    NativeFighterActionMetadata metadata = {};
    if (!nativeFighterAction_GetMetadata(session->project,
            (unsigned) session->selected_action, &metadata)) return;
    Rml::Element* toggle = document->GetElementById("workshop-action-loop");
    // The DAT stores PPC-ordered flags; source bit 30 becomes native bit 1.
    if (metadata.animation_flags & 0x40000000U)
        toggle->SetAttribute("checked", "");
    else toggle->RemoveAttribute("checked");
    Rml::Element* root_motion = document->GetElementById("workshop-action-root-motion");
    if (metadata.animation_flags & 0x80000000U)
        root_motion->SetAttribute("checked", "");
    else root_motion->RemoveAttribute("checked");
}

void apply_action_playback()
{
    if (session->project == nullptr || session->selected_action < 0) return;
    NativeFighterActionMetadata metadata = {};
    if (!nativeFighterAction_GetMetadata(session->project,
            (unsigned) session->selected_action, &metadata)) return;
    const bool enabled = document->GetElementById("workshop-action-loop")
        ->HasAttribute("checked");
    uint32_t flags = enabled
        ? metadata.animation_flags | 0x40000000U
        : metadata.animation_flags & ~0x40000000U;
    if (document->GetElementById("workshop-action-root-motion")->HasAttribute("checked"))
        flags |= 0x80000000U;
    else flags &= ~0x80000000U;
    if (flags == metadata.animation_flags) return;
    std::vector<uint8_t> before(session->project->fighter_dat,
                                session->project->fighter_dat + session->project->fighter_size);
    if (!nativeFighterAction_SetAnimationFlags(session->project,
            (unsigned) session->selected_action, flags)) {
        set_text("workshop-editor-status", "Could not edit this action's "
                 "animation playback flag.");
        return;
    }
    record_edit(false, before.data());
    session->action_dirty = true;
    set_text("workshop-editor-status", "Action animation playback changed. "
             "Save the draft, then Test Character to hear and see it.");
}

void update_timeline_cursor()
{
    const double left = session->timeline_end_frame > 0
        ? std::clamp(double(session->timeline_frame) * 92.0 / session->timeline_end_frame,
                     0.0, 92.0) : 0.0;
    document->GetElementById("workshop-timeline-playhead")->SetProperty(
        "left", std::to_string(left) + "%");
}

int hitbox_end_for_instance(size_t instance)
{
    if (instance >= session->timeline_event_count) return session->timeline_end_frame;
    const unsigned id = session->events[session->timeline_events[instance].event].target_id;
    const unsigned duration = session->events[session->timeline_events[instance].event].duration256;
    const int owned_end = duration ? session->timeline_events[instance].frame + (int)((duration + 255) / 256) : session->timeline_end_frame;
    for (size_t later = instance + 1; later < session->timeline_event_count; ++later) {
        const NativeFighterActionEvent& event =
            session->events[session->timeline_events[later].event];
        if (event.kind == NativeFighterActionEvent_ClearAllHitboxes ||
            ((event.kind == NativeFighterActionEvent_ClearHitbox ||
              event.kind == NativeFighterActionEvent_CreateHitbox) &&
             event.target_id == id))
            return std::min(owned_end, session->timeline_events[later].frame);
    }
    return owned_end;
}

int hitbox_end_frame(size_t index)
{
    for (size_t i = 0; i < session->timeline_event_count; ++i)
        if (session->timeline_events[i].event == index)
            return hitbox_end_for_instance(i);
    return session->timeline_end_frame;
}

struct HitboxToggle {
    const char* id;
    uint8_t NativeFighterHitbox::*member;
};

constexpr HitboxToggle hitbox_toggles[] = {
    {"workshop-hitbox-grounded", &NativeFighterHitbox::hit_grounded},
    {"workshop-hitbox-aerial", &NativeFighterHitbox::hit_aerial},
    {"workshop-hitbox-common-bone", &NativeFighterHitbox::use_common_bone_ids},
    {"workshop-hitbox-grabbed", &NativeFighterHitbox::only_hit_grabbed},
    {"workshop-hitbox-items", &NativeFighterHitbox::item_hit_interaction},
    {"workshop-hitbox-ignore-thrown", &NativeFighterHitbox::ignore_thrown_fighters},
    {"workshop-hitbox-ignore-scale", &NativeFighterHitbox::ignore_fighter_scale},
    {"workshop-hitbox-clank", &NativeFighterHitbox::clank},
    {"workshop-hitbox-rebound", &NativeFighterHitbox::rebound},
};

void show_event()
{
    const ActionInspectorRefresh refresh;
    document->GetElementById("workshop-command-fields")->SetProperty("display", "none");
    document->GetElementById("workshop-delete-event")
        ->SetProperty("display", "none");
    Rml::Element* inspector = document->GetElementById("workshop-event-fields");
    inspector->SetProperty("display", "none");
    Rml::Element* value_inspector = document->GetElementById("workshop-value-fields");
    value_inspector->SetProperty("display", "none");
    document->GetElementById("workshop-sound-fields")->SetProperty("display", "none");
    document->GetElementById("workshop-effect-fields")->SetProperty("display", "none");
    document->GetElementById("workshop-action-playback")->SetProperty("display",
        session->selected_event < 0 && session->selected_command < 0 ? "block" : "none");
    if (session->project == nullptr || session->selected_action < 0 || session->selected_event < 0 ||
        (size_t) session->selected_event >= session->event_count) {
        if (session->selected_command < 0) {
            set_text("workshop-action-title", "No event selected");
            set_text("workshop-action-detail", "Select an event to edit it.");
        }
        return;
    }
    const NativeFighterActionEvent& event = session->events[session->selected_event];
    show_command_fields(event);
    const bool airborne_event = event.kind == NativeFighterActionEvent_SetAirborneState;
    document->GetElementById("workshop-air-state")->SetProperty("display",
        airborne_event ? "block" : "none");
    document->GetElementById("workshop-value-number")->SetProperty("display",
        airborne_event ? "none" : "block");
    set_text("workshop-event-value-label",
        event.kind == NativeFighterActionEvent_ThrowHitbox ? "Damage" : "Command value");
    document->GetElementById("workshop-delete-event")
        ->SetProperty("display", "block");
    set_text("workshop-action-title", event_name(event));
    if (event.kind == NativeFighterActionEvent_Command) {
        set_text("workshop-action-title", event_name(event));
        set_text("workshop-action-detail", NativeFighterActionLabels::command_description(event.opcode));
        document->GetElementById("workshop-delete-event")->SetProperty("display", "block");
        return;
    }
    if (event.kind == NativeFighterActionEvent_SoundEffect) {
        NativeFighterSoundEffect sound;
        if (!nativeFighterAction_GetSoundEffect(session->project,
                (unsigned) session->selected_action, event.source_offset, &sound)) return;
        document->GetElementById("workshop-sound-fields")->SetProperty("display", "block");
        set_text("workshop-action-detail", "Original sound command at 0x" +
                 [&] { char hex[16]; std::snprintf(hex, sizeof(hex), "%X",
                           event.source_offset); return std::string(hex); }());
        set_text("workshop-sound-behavior", "Behavior " + number(sound.behavior) +
                 (sound.behavior > 6 ? " · canned sound is read-only" : ""));
        field("workshop-sound-id")->SetValue(number(sound.sfx_id));
        field("workshop-sound-volume")->SetValue(number(sound.volume));
        field("workshop-sound-pan")->SetValue(number(sound.panning));
        for (const char* id : { "workshop-sound-id", "workshop-sound-volume",
                                "workshop-sound-pan" }) {
            if (sound.behavior > 6) field(id)->SetAttribute("disabled", "");
            else field(id)->RemoveAttribute("disabled");
        }
        return;
    }
    if (event.kind == NativeFighterActionEvent_SpawnEffect) {
        NativeFighterSpawnEffect effect;
        if (!nativeFighterAction_GetSpawnEffect(session->project,
                (unsigned) session->selected_action, event.source_offset, &effect)) return;
        document->GetElementById("workshop-effect-fields")->SetProperty("display", "block");
        set_text("workshop-action-detail", "Original visual-effect command");
        field("workshop-effect-id")->SetValue(number(effect.gfx_id));
        field("workshop-effect-bone")->SetValue(number(effect.bone));
        field("workshop-effect-x")->SetValue(std::to_string(effect.offset_x));
        field("workshop-effect-y")->SetValue(std::to_string(effect.offset_y));
        field("workshop-effect-z")->SetValue(std::to_string(effect.offset_z));
        return;
    }
    if (event.kind != NativeFighterActionEvent_CreateHitbox) {
        set_text("workshop-action-detail",
                 std::string(event_name(event)) + " · source 0x" +
                 [&] { char hex[16]; std::snprintf(hex, sizeof(hex), "%X",
                                           event.source_offset); return std::string(hex); }());
        if (event.kind != NativeFighterActionEvent_SetCommandVar &&
            event.kind != NativeFighterActionEvent_AllowInterrupt &&
            event.kind != NativeFighterActionEvent_ReleaseThrow &&
            event.kind != NativeFighterActionEvent_ReverseFacing &&
            event.kind != NativeFighterActionEvent_NeutralCue &&
            event.kind != NativeFighterActionEvent_ClearHitbox &&
            event.kind != NativeFighterActionEvent_ClearAllHitboxes &&
            event.kind != NativeFighterActionEvent_SoundEffect &&
            event.kind != NativeFighterActionEvent_SpawnEffect) {
            value_inspector->SetProperty("display", "block");
            set_text("workshop-value-kind", event_name(event));
            set_text("workshop-value-target", "Target ID " +
                     number(event.target_id) + " · original command value");
            field("workshop-event-value")->SetValue(number(event.value));
            if (airborne_event) {
                auto* state = static_cast<Rml::ElementFormControlSelect*>(
                    document->GetElementById("workshop-air-state"));
                state->SetValue(number(event.value));
                set_text("workshop-value-target", "Change movement state at this event's frame.");
            } else if (event.kind == NativeFighterActionEvent_ThrowHitbox) {
                set_text("workshop-value-target", "Throw record #" + number(event.target_id) +
                    ". Defines damage; the separate Release throw event applies the throw.");
            }
        }
        if (event.kind == NativeFighterActionEvent_ReleaseThrow)
            set_text("workshop-action-detail", "Releases the grabbed fighter using throw record #0. "
                "Change its execution frame with the source timer below.");
        return;
    }
    NativeFighterHitbox hitbox;
    if (!nativeFighterAction_GetHitbox(session->project, (unsigned) session->selected_action,
                                      event.source_offset, &hitbox)) {
        set_text("workshop-editor-status", "Could not decode this hitbox.");
        return;
    }
    inspector->SetProperty("display", "block");
    document->GetElementById("workshop-hitbox-lifetime")->SetProperty("display", hitbox.timed ? "block" : "none");
    document->GetElementById("workshop-hitbox-manual-lifetime")->SetProperty("display", hitbox.timed ? "none" : "block");
    if (hitbox.timed) {
        field("workshop-hitbox-duration")->SetValue(std::to_string(hitbox.duration256 / 256.0f));
        auto* refresh = document->GetElementById("workshop-hitbox-refresh");
        if (hitbox.refresh_hit_eligibility) refresh->SetAttribute("checked", "checked");
        else refresh->RemoveAttribute("checked");
    }
    set_text("workshop-action-title", "Hitbox " + number(hitbox.id));
    set_text("workshop-hitbox-id", "Hitbox " + number(hitbox.id) +
             (hitbox.use_common_bone_ids ? " · common bone " :
                                           " · fighter bone ") +
             number(hitbox.bone));
    field("workshop-hitbox-damage")->SetValue(number(hitbox.damage));
    field("workshop-hitbox-angle")->SetValue(number(hitbox.angle));
    field("workshop-hitbox-radius")->SetValue(std::to_string(hitbox.radius * HitboxDistanceScale));
    field("workshop-hitbox-kbg")->SetValue(number(hitbox.knockback_growth));
    field("workshop-hitbox-bkb")->SetValue(number(hitbox.base_knockback));
    field("workshop-hitbox-bone")->SetValue(number(hitbox.bone));
    field("workshop-hitbox-offset-x")->SetValue(std::to_string(hitbox.offset_x * HitboxDistanceScale));
    field("workshop-hitbox-offset-y")->SetValue(std::to_string(hitbox.offset_y * HitboxDistanceScale));
    field("workshop-hitbox-offset-z")->SetValue(std::to_string(hitbox.offset_z * HitboxDistanceScale));
    static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("workshop-hitbox-element"))->SetValue(number(hitbox.element));
    field("workshop-hitbox-shield")->SetValue(std::to_string(hitbox.shield_damage));
    field("workshop-hitbox-wkb")->SetValue(number(hitbox.weight_set_knockback));
    field("workshop-hitbox-rehit-group")->SetValue(number(hitbox.hit_group));
    field("workshop-hitbox-sfx-level")->SetValue(number(hitbox.hit_sfx_severity));
    field("workshop-hitbox-sfx-kind")->SetValue(number(hitbox.hit_sfx_kind));
    for (const auto& toggle : hitbox_toggles) {
        auto* control = document->GetElementById(toggle.id);
        if (hitbox.*toggle.member) control->SetAttribute("checked", "");
        else control->RemoveAttribute("checked");
    }
    unsigned executions = 0;
    for (size_t i = 0; i < session->timeline_event_count; ++i)
        if (session->timeline_events[i].event == (unsigned) session->selected_event)
            ++executions;
    set_text("workshop-action-detail", session->event_frames[session->selected_event] >= 0
        ? std::string(executions > 1 ? "First active frames " : "Frames ") +
          number((unsigned) session->event_frames[session->selected_event]) + "–" +
          number((unsigned) hitbox_end_frame(session->selected_event)) +
          (executions > 1 ? " · " + number(executions) + " executions" : "")
        : "Timing depends on script control flow");
}

void show_commands()
{
    std::string markup = "<div class='workshop-command-head'>"
        "<span>FRAME</span><span>ORDER</span><span>COMMAND</span><span>TARGET</span>"
        "</div>";
    int previous_frame = -2;
    unsigned frame_order = 0;
    for (size_t i = 0; i < session->command_count; ++i) {
        const NativeFighterActionCommand& command = session->commands[i];
        if (!visible_command(command)) continue;
        if (session->command_frames[i] != previous_frame) {
            frame_order = 0;
            previous_frame = session->command_frames[i];
        }
        ++frame_order;
        if (session->command_frames[i] >= 0 &&
            (i == 0 || session->command_frames[i - 1] != session->command_frames[i])) {
            size_t grouped = 0;
            for (size_t j = i; j < session->command_count &&
                 session->command_frames[j] == session->command_frames[i]; ++j) ++grouped;
            if (grouped > 1)
                markup += "<div class='workshop-command-frame'>Frame " +
                    number((unsigned) session->command_frames[i]) + " · " +
                    number((unsigned) grouped) +
                    " commands · execute top to bottom</div>";
        }
        int event_index = -1;
        for (size_t j = 0; j < session->event_count; ++j) {
            if (session->events[j].source_offset == command.source_offset) {
                event_index = (int) j;
                break;
            }
        }
        char hex[16];
        std::snprintf(hex, sizeof(hex), "%X", command.source_offset);
        const std::string target = event_index < 0 ? "source 0x" +
            std::string(hex) :
            session->events[event_index].kind == NativeFighterActionEvent_CreateHitbox ||
            session->events[event_index].kind == NativeFighterActionEvent_ClearHitbox ||
            session->events[event_index].kind == NativeFighterActionEvent_SetHitboxDamage ||
            session->events[event_index].kind == NativeFighterActionEvent_SetHitboxScale ||
            session->events[event_index].kind == NativeFighterActionEvent_SetHitboxFlag
                ? "Hitbox #" + number(session->events[event_index].target_id) :
            session->events[event_index].kind == NativeFighterActionEvent_SoundEffect
                ? "Sound" :
            session->events[event_index].kind == NativeFighterActionEvent_SpawnEffect
                ? "Effect" :
            session->events[event_index].kind == NativeFighterActionEvent_AllowInterrupt
                ? "IASA" : "";
        markup += "<button id='workshop-command-" + number((unsigned) i) +
                  "' class='workshop-command-row" +
                  ((int) i == session->selected_command
                       ? " selected" : "") +
                  "' title='Source 0x" + hex + "'><span>" +
                  (session->command_frames[i] >= 0
                       ? number((unsigned) session->command_frames[i]) : "flow") +
                  "</span><span>" + number(frame_order) +
                  "</span><span>" +
                  (event_index >= 0 ? event_name(session->events[event_index])
                                    : NativeFighterActionLabels::command(command.opcode)) +
                  "</span><span>" + target + "</span></button>";
    }
    document->GetElementById("workshop-command-list")->SetInnerRML(
        session->command_count == 0 ? "No reachable commands for this action." : markup);
}

void show_action_timeline()
{
    session->preview_hitbox_cached_frame = -1;
    session->timeline_end_frame = 48;
    NativeFighterAnimationInfo action_clip = {};
    const bool has_clip = session->project != nullptr && session->selected_action >= 0 &&
        nativeFighterAnimation_Info(session->project, NativeFighterAnimationClip{ NativeFighterAnimation_Action, (unsigned) session->selected_action },
                                    &action_clip);
    if (has_clip && std::isfinite(action_clip.frames))
        session->timeline_end_frame = std::max(session->timeline_end_frame,
            std::clamp((int) std::ceil(action_clip.frames), 1, 1000));
    for (size_t i = 0; i < session->event_count; ++i) session->event_frames[i] = -1;
    for (size_t i = 0; i < session->command_count; ++i) session->command_frames[i] = -1;
    session->timeline_event_count = 0;
    session->timeline_complete = true;
    session->timeline_occurrence_count = 0;
    if (session->project != nullptr && session->selected_action >= 0 && session->command_count != 0 &&
        !nativeFighterAction_Timeline(session->project, (unsigned) session->selected_action,
            session->timeline_occurrences, std::size(session->timeline_occurrences),
            &session->timeline_occurrence_count, &session->timeline_complete)) {
        session->timeline_complete = false;
    }
    for (size_t occurrence = 0; occurrence < session->timeline_occurrence_count;
         ++occurrence) {
        const auto& current = session->timeline_occurrences[occurrence];
        const int frame = current.frame;
        session->timeline_end_frame = std::max(session->timeline_end_frame, frame + 3);
        for (size_t e = 0; e < session->event_count; ++e)
            if (session->events[e].source_offset == current.source_offset && session->events[e].duration256)
                session->timeline_end_frame = std::max(session->timeline_end_frame, frame + (int)((session->events[e].duration256 + 255) / 256));
        for (size_t command = 0; command < session->command_count; ++command)
            if (session->commands[command].source_offset == current.source_offset) {
                if (session->command_frames[command] < 0) session->command_frames[command] = frame;
                break;
            }
        for (size_t event = 0; event < session->event_count; ++event) {
            if (session->events[event].source_offset != current.source_offset) continue;
            if (session->event_frames[event] < 0) session->event_frames[event] = frame;
            if (session->timeline_event_count < std::size(session->timeline_events))
                session->timeline_events[session->timeline_event_count++] = {
                    (unsigned) event, frame
                };
            break;
        }
    }
    session->timeline_end_frame = std::min(session->timeline_end_frame, 1000);
    session->timeline_frame = std::clamp(session->timeline_frame, 0, session->timeline_end_frame);
    field("workshop-current-frame")->SetValue(number((unsigned) session->timeline_frame));
    update_timeline_cursor();
    set_text("workshop-frame-total", "/ " + number((unsigned) session->timeline_end_frame));

    const char* lane_names[] = { "Animation", "Hitboxes", "Effects / sound",
                                 "Interrupts", "State", "Other commands" };
    std::string labels = "<div class='workshop-timeline-label ruler'></div>";
    std::string lanes, ruler;
    const int ruler_width = document->GetElementById(
        "workshop-event-timeline-ruler")->GetClientWidth();
    const int drawable_width = std::max(1, (ruler_width > 0 ? ruler_width : 900) * 92 / 100);
    // Every frame gets a boundary when there is room. Longer actions use a
    // readable multiple because subpixel boundaries cannot be distinguished.
    const int tick_step = std::max(1, (session->timeline_end_frame * 7 + drawable_width - 1) /
                                      drawable_width);
    const int label_step = tick_step * std::max(1, 42 / std::max(1,
        drawable_width * tick_step / std::max(1, session->timeline_end_frame)));
    for (int frame = 0; frame <= session->timeline_end_frame; frame += tick_step) {
        const std::string left = std::to_string(
            double(frame) * 92.0 / session->timeline_end_frame);
        ruler += "<span class='workshop-frame-boundary' style='left: " +
                 left + "%'></span>";
        if (frame == 0 || frame % label_step == 0)
            ruler += "<span class='workshop-timeline-tick' style='left: " +
                     left + "%'>" +
                     number((unsigned) frame) + "</span>";
    }
    if (session->timeline_end_frame % tick_step != 0)
        ruler += "<span class='workshop-frame-boundary' style='left: 92%'></span>";
    bool hitbox_ids[8] = {};
    for (size_t i = 0; i < session->event_count; ++i)
        if (session->events[i].kind == NativeFighterActionEvent_CreateHitbox &&
            session->events[i].target_id < 8) hitbox_ids[session->events[i].target_id] = true;
    int hitbox_count = 0;
    for (bool present : hitbox_ids) if (present) ++hitbox_count;
    const int visible_hitbox_lanes = session->hitbox_lanes_expanded ? hitbox_count : 0;
    bool throw_ids[8] = {};
    bool has_throw_release = false;
    for (size_t i = 0; i < session->event_count; ++i) {
        if (session->events[i].kind == NativeFighterActionEvent_ThrowHitbox && session->events[i].target_id < 8)
            throw_ids[session->events[i].target_id] = true;
        if (session->events[i].kind == NativeFighterActionEvent_ReleaseThrow ||
            session->events[i].kind == NativeFighterActionEvent_ReverseFacing)
            has_throw_release = true;
    }
    int throw_count = 0;
    for (bool present : throw_ids) if (present) ++throw_count;
    const int throw_lane_start = 6 + visible_hitbox_lanes;
    const int total_lanes = throw_lane_start + throw_count + (has_throw_release ? 1 : 0);
    for (int lane = 0; lane < total_lanes; ++lane) {
        int throw_id = -1;
        if (lane >= throw_lane_start && lane < throw_lane_start + throw_count) {
            int nth = lane - throw_lane_start;
            for (int id = 0; id < 8; ++id)
                if (throw_ids[id] && nth-- == 0) { throw_id = id; break; }
        }
        const bool release_lane = has_throw_release && lane == total_lanes - 1;
        int hitbox_id = -1;
        if (lane >= 2 && lane < 2 + visible_hitbox_lanes) {
            int remaining = lane - 2;
            for (int id = 0; id < 8; ++id)
                if (hitbox_ids[id] && remaining-- == 0) { hitbox_id = id; break; }
        }
        const int kind = lane < 2 ? lane : lane >= 2 + visible_hitbox_lanes
                                      ? lane - visible_hitbox_lanes + 1 : 2;
        const std::string name = throw_id >= 0 ? "Throw setup #" + number((unsigned) throw_id)
            : release_lane ? "Throw release / facing"
            : hitbox_id >= 0
            ? "#" + number((unsigned) hitbox_id)
            : lane == 1 ? "Hitboxes · " + number((unsigned) hitbox_count)
            : kind == 2 ? "Hitbox IDs" : lane_names[std::min(
                  kind >= 3 ? kind - 1 : kind, 5)];
        if (lane == 1)
            labels += "<button id='workshop-hitbox-group' "
                      "class='workshop-timeline-label hitbox group'>" +
                      std::string(session->hitbox_lanes_expanded ? "⌄ " : "› ") +
                      name + "</button>";
        else
            labels += "<div class='workshop-timeline-label" +
                      std::string(hitbox_id >= 0 ? " hitbox subtrack" : "") +
                      "'>" + name + "</div>";
        lanes += "<div class='workshop-timeline-lane'>";
        if (lane == 0 && has_clip)
            lanes += "<span class='workshop-animation-span'>" +
                     Rml::StringUtilities::EncodeRml(session->actions[session->selected_action].name) +
                     "</span>";
        if (lane == 1 && hitbox_count != 0) {
            int first = session->timeline_end_frame;
            float last = 0;
            for (size_t i = 0; i < session->timeline_event_count; ++i) {
                const auto& occurrence = session->timeline_events[i];
                if (session->events[occurrence.event].kind !=
                    NativeFighterActionEvent_CreateHitbox) continue;
                first = std::min(first, occurrence.frame);
                const auto& event = session->events[occurrence.event];
                const float end = event.duration256
                    ? occurrence.frame + event.duration256 / 256.0f
                    : (float) hitbox_end_for_instance(i);
                last = std::max(last, end);
            }
            if (last > first) {
                const float left = std::clamp(first * 92.0f / session->timeline_end_frame, 0.0f, 92.0f);
                const float width = std::clamp((last - first) * 92.0f /
                    session->timeline_end_frame, 0.0f, 92.0f - left);
                lanes += "<button id='workshop-hitbox-overview' "
                         "class='workshop-timeline-event hitbox-overview' "
                         "style='left: " + std::to_string(left) +
                         "%; width: " + std::to_string(width) + "%'>" +
                         number((unsigned) hitbox_count) + " hitboxes</button>";
            }
        }
        for (size_t instance = 0; instance < session->timeline_event_count; ++instance) {
            const unsigned i = session->timeline_events[instance].event;
            const int frame = session->timeline_events[instance].frame;
            const NativeFighterActionEvent& event = session->events[i];
            bool belongs = false;
            if (throw_id >= 0) belongs = event.kind == NativeFighterActionEvent_ThrowHitbox &&
                                          (int) event.target_id == throw_id;
            else if (release_lane) belongs = event.kind == NativeFighterActionEvent_ReleaseThrow ||
                                             event.kind == NativeFighterActionEvent_ReverseFacing;
            else if (lane == 0) belongs = false;
            else if (lane == 1) belongs = false;
            else if (hitbox_id >= 0)
                belongs = (event.kind == NativeFighterActionEvent_CreateHitbox ||
                           event.kind == NativeFighterActionEvent_ClearHitbox) &&
                          (int) event.target_id == hitbox_id;
            else if (kind == 3) belongs = event.kind == NativeFighterActionEvent_SoundEffect ||
                                          event.kind == NativeFighterActionEvent_SpawnEffect;
            else if (kind == 4) belongs = event.kind == NativeFighterActionEvent_AllowInterrupt;
            else if (kind == 5) belongs = event.kind == NativeFighterActionEvent_SetBodyCollisionState ||
                                          event.kind == NativeFighterActionEvent_SetAirborneState ||
                                          event.kind == NativeFighterActionEvent_SetGlobalHurtState ||
                                          event.kind == NativeFighterActionEvent_SetBoneHurtState;
            else if (kind == 6) belongs =
                event.kind == NativeFighterActionEvent_SetHitboxDamage ||
                event.kind == NativeFighterActionEvent_SetHitboxScale ||
                event.kind == NativeFighterActionEvent_SetHitboxFlag ||
                event.kind == NativeFighterActionEvent_SetCommandVar ||
                event.kind == NativeFighterActionEvent_NeutralCue ||
                event.kind == NativeFighterActionEvent_Command;
            if (!belongs) continue;
            const float left = std::clamp(frame * 92.0f / session->timeline_end_frame, 0.0f, 92.0f);
            int end_frame = frame + 1;
            if (event.kind == NativeFighterActionEvent_CreateHitbox) {
                end_frame = hitbox_end_for_instance(instance);
            }
            const float span = event.duration256 ? event.duration256 / 256.0f : end_frame - frame;
            const float width = std::clamp(span * 92.0f / session->timeline_end_frame,
                event.duration256 ? 0.0f : release_lane ? 5.0f : 2.0f,
                event.duration256 ? 92.0f - left : std::max(5.0f, 92.0f - left));
            lanes += "<button id='workshop-timeline-event-" +
                     number((unsigned) instance) +
                     "' class='workshop-timeline-event" +
                     (event.kind == NativeFighterActionEvent_CreateHitbox
                          ? " hitbox-block" : "") +
                     std::string((int) i == session->selected_event ? " selected" : "") +
                     "' title='" + event_name(event) +
                     " at frame " + number((unsigned) frame) +
                     (event.duration256 ? ". Drag the body to move; drag either edge to resize." : "") +
                     "' style='left: " + std::to_string(left) +
                     "%; width: " + std::to_string(width) + "%'>" +
                     (event.kind == NativeFighterActionEvent_CreateHitbox
                          ? "#" + number(event.target_id) :
                      event.kind == NativeFighterActionEvent_ThrowHitbox
                          ? number(event.value) + "%" :
                      event.kind == NativeFighterActionEvent_ReleaseThrow
                          ? "Release" :
                          Rml::StringUtilities::EncodeRml(event_name(event))) +
                     (event.duration256 ? "<span class='workshop-hitbox-edge left'></span><span class='workshop-hitbox-edge right'></span>" : "") + "</button>";
        }
        lanes += "</div>";
    }
    const Rml::String content_height =
        std::to_string(28 + 27 * total_lanes) + "dp";
    document->GetElementById("workshop-event-timeline-labels")
        ->SetProperty("min-height", content_height);
    document->GetElementById("workshop-event-timeline-tracks")
        ->SetProperty("min-height", content_height);
    std::string grid;
    for (int frame = 0; frame <= session->timeline_end_frame; frame += tick_step) {
        const float left = 92.0f * frame / session->timeline_end_frame;
        grid += "<div style='left:" + float_text(left) + "%'></div>";
    }
    document->GetElementById("workshop-timeline-grid")->SetInnerRML(grid);
    document->GetElementById("workshop-event-timeline-labels")->SetInnerRML(labels);
    document->GetElementById("workshop-event-timeline-ruler")->SetInnerRML(ruler);
    document->GetElementById("workshop-event-timeline-lanes")->SetInnerRML(lanes);
    show_commands();
    set_text("workshop-timeline-selection", session->selected_event >= 0 &&
                 (size_t) session->selected_event < session->event_count
             ? std::string(event_name(session->events[session->selected_event])) +
               (session->event_frames[session->selected_event] >= 0
                    ? " · frame " + number((unsigned) session->event_frames[session->selected_event])
                    : " · dynamic timing; see event list")
             : session->timeline_complete
                 ? "Select an event · repeated commands are unrolled"
                 : "Timeline stops at an animation wait or trace limit · see Event list");
}

void select_action(unsigned action)
{
    if (session->project == nullptr || action >= session->action_count) return;
    if ((session->animation_clip.bank != NativeFighterAnimation_Action ||
         session->animation_clip.index != action) &&
        session->mode == WorkshopMode::Animation)
        session->animation_frame = 0;
    stop_action_preview();
    action_timeline_events.Reset();
    session->add_event_menu_open = false;
    document->GetElementById("workshop-add-event-menu")
        ->SetProperty("display", "none");
    session->selected_action = (int) action;
    session->animation_clip = { NativeFighterAnimation_Action, action };
    session->animation_name = session->actions[action].name;
    show_action_playback();
    NativeFighterAnimationInfo selected_clip = {};
    session->action_has_clip[action] = nativeFighterAnimation_Info(session->project, NativeFighterAnimationClip{ NativeFighterAnimation_Action, action },
                                                           &selected_clip);
    session->action_clip_frames[action] = session->action_has_clip[action]
        ? selected_clip.frames : 0.0f;
    session->action_group_open[WorkshopLabels::group(session->actions[action].name)] = true;
    session->displayed_animation_clip.index = UINT_MAX;
    session->selected_event = -1;
    session->selected_command = -1;
    session->event_count = session->command_count = 0;
    set_text("workshop-action-title", WorkshopLabels::action(session->actions[action].name));
    set_text("workshop-context-action", WorkshopLabels::action(session->actions[action].name));
    set_text("workshop-context-clip", session->actions[action].name);
    set_text("workshop-action-detail", "Action " + number(action));
    const char* label = nativeFighterAuthoring_Name(
        session->project, NativeAuthoring_Action, action);
    set_text("workshop-action-title",
             label && *label
                 ? label
                 : WorkshopLabels::action(session->actions[action].name));
    set_text("workshop-context-action",
             label && *label
                 ? label
                 : WorkshopLabels::action(session->actions[action].name));
    const char* stored_group = nativeFighterAuthoring_Name(
        session->project, NativeAuthoring_ActionGroup, action);
    if (stored_group && *stored_group) {
        session->action_group_open[std::min(
            4u, (unsigned) std::atoi(stored_group))] = true;
    }
    field("workshop-action-name")
        ->SetValue(label && *label ? label
                                   : WorkshopLabels::action(
                                         session->actions[action].name));
    set_text("workshop-action-hide",
             nativeFighterAuthoring_Hidden(session->project, action)
                 ? "Show action in editor"
                 : "Hide action in editor");

    set_text("workshop-editor-status", "");
    document->GetElementById("workshop-event-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-value-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-sound-fields")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-effect-fields")
        ->SetProperty("display", "none");
    const bool has_commands = !session->actions[action].has_script ||
        nativeFighterAction_Commands(session->project, action, session->commands, 512,
                                     &session->command_count);
    const bool has_events = !session->actions[action].has_script ||
        nativeFighterAction_Events(session->project, action, session->events, 256, &session->event_count);
    if (!has_commands || !has_events) {
        set_text("workshop-editor-status", "This action has more commands than "
                 "the editor can show, or its source could not be decoded.");
        session->command_count = session->event_count = 0;
    }
    session->timeline_frame = 0;
    show_action_timeline();
    if (session->selected_event >= 0 && session->event_frames[session->selected_event] >= 0) {
        session->timeline_frame = session->event_frames[session->selected_event];
        field("workshop-current-frame")->SetValue(
            number((unsigned) session->timeline_frame));
        update_timeline_cursor();
    }
    show_event();
    nativeShellWorkshopEditor_FilterActions();
    if (session->mode == WorkshopMode::Animation) {
        nativeShellWorkshopEditor_FilterClips();
        show_animation();
    }
}

void select_event(unsigned index, bool seek_to_first_occurrence)
{
    if (index >= session->event_count) return;
    stop_action_preview();
    session->selected_event = (int) index;
    for (size_t command = 0; command < session->command_count; ++command)
        if (session->commands[command].source_offset == session->events[index].source_offset) {
            session->selected_command = (int) command;
            break;
        }
    if (seek_to_first_occurrence && session->event_frames[index] >= 0) {
        session->timeline_frame = session->event_frames[index];
        field("workshop-current-frame")->SetValue(
            number((unsigned) session->timeline_frame));
        update_timeline_cursor();
    }
    show_event();
    show_action_timeline();
}

void active_hitbox_events(int (&event_indices)[4])
{
    const TimelineEventInstance* active[4] = {};
    for (size_t i = 0; i < session->timeline_event_count; ++i) {
        const TimelineEventInstance& instance = session->timeline_events[i];
        if (instance.frame > session->timeline_frame) continue;
        const NativeFighterActionEvent& event = session->events[instance.event];
        if (event.kind == NativeFighterActionEvent_ClearAllHitboxes) {
            std::fill(std::begin(active), std::end(active), nullptr);
        } else if (event.target_id < std::size(active)) {
            if (event.kind == NativeFighterActionEvent_ClearHitbox)
                active[event.target_id] = nullptr;
            else if (event.kind == NativeFighterActionEvent_CreateHitbox)
                active[event.target_id] = &instance;
        }
    }
    for (size_t slot = 0; slot < std::size(active); ++slot) {
        event_indices[slot] = -1;
        const TimelineEventInstance* instance = active[slot];
        if (instance == nullptr) continue;
        const auto duration = session->events[instance->event].duration256;
        const unsigned elapsed = (session->timeline_frame - instance->frame) * 256u;
        if (duration == 0 || elapsed < duration)
            event_indices[slot] = (int) instance->event;
    }
}

void toggle_add_event_menu()
{
    if (session->project == nullptr || session->selected_action < 0) {
        set_text("workshop-editor-status", "Choose an action, then a frame for the new event.");
        return;
    }
    session->add_event_menu_open = !session->add_event_menu_open;
    Rml::Element* menu = document->GetElementById("workshop-add-event-menu");
    if (!session->add_event_menu_open) {
        menu->SetProperty("display", "none");
        return;
    }
    std::string markup;
    if (session->selected_event >= 0)
        markup = "<button id='workshop-add-copy'>Copy selected event</button>";
    static const unsigned kinds[] = {
        NativeFighterActionEvent_CreateHitbox,
        NativeFighterActionEvent_SpawnEffect,
        NativeFighterActionEvent_SoundEffect,
        NativeFighterActionEvent_ClearAllHitboxes,
        NativeFighterActionEvent_ClearHitbox,
        NativeFighterActionEvent_AllowInterrupt,
        NativeFighterActionEvent_SyncTimer,
        NativeFighterActionEvent_AsyncTimer,
        NativeFighterActionEvent_SetCommandVar,
        NativeFighterActionEvent_SetBodyCollisionState,
        NativeFighterActionEvent_SetGlobalHurtState,
        NativeFighterActionEvent_SetBoneHurtState,
        NativeFighterActionEvent_ThrowHitbox,
        NativeFighterActionEvent_ReleaseThrow,
        NativeFighterActionEvent_ReverseFacing,
        NativeFighterActionEvent_SetAirborneState,
    };
    bool offered[61] = {};
    for (unsigned kind : kinds) {
        if (!session->show_event_list && (kind == NativeFighterActionEvent_SyncTimer || kind == NativeFighterActionEvent_AsyncTimer ||
            kind == NativeFighterActionEvent_ClearHitbox || kind == NativeFighterActionEvent_ClearAllHitboxes)) continue;
        const auto* type = NativeActionProperties::type_for_event(kind);
        if (!type) continue;
        offered[type->opcode] = true;
        markup += "<button id='workshop-add-kind-" + number(kind) +
                  "'>" + NativeFighterActionLabels::event(kind) + "</button>";
    }
    for (const auto& type : NativeActionProperties::types()) {
        if (type.opcode < 9 || offered[type.opcode] || type.opcode == 59) continue;
        markup += "<button id='workshop-add-command-" + number(type.opcode) + "'>" +
                  type.label + "</button>";
    }
    menu->SetInnerRML(markup);
    menu->SetProperty("display", "block");
}

void insert_selected_event(unsigned template_action, uint32_t template_offset)
{
    if (session->project == nullptr || session->selected_action < 0) {
        set_text("workshop-editor-status", "Choose an action and frame for the new event.");
        return;
    }
    if (session->command_count >= 512 || session->event_count >= 256) {
        set_text("workshop-editor-status", "This action exceeds the "
                 "editor's current command capacity.");
        return;
    }
    const unsigned action = (unsigned) session->selected_action;
    const int requested_frame = session->timeline_frame;
    const size_t previous_size = session->project->fighter_size;
    std::vector<uint8_t> previous(session->project->fighter_dat,
                                  session->project->fighter_dat + previous_size);
    uint32_t inserted;
    if (!nativeFighterAction_InsertAtFrame(session->project, action, requested_frame,
                                            template_action, template_offset,
                                            &inserted)) {
        set_text("workshop-editor-status", "Could not add this event. "
                 "The source graph may contain an unsupported reference, "
                 "or a new hitbox may need a free ID.");
        return;
    }
    record_structural_edit(false, previous.data(), previous_size);
    session->action_dirty = true;
    select_action(action);
    for (size_t i = 0; i < session->event_count; ++i) {
        if (session->events[i].source_offset != inserted) continue;
        session->selected_event = (int) i;
        for (size_t command = 0; command < session->command_count; ++command)
            if (session->commands[command].source_offset == inserted) {
                session->selected_command = (int) command;
                break;
            }
        if (session->event_frames[i] >= 0) {
            session->timeline_frame = session->event_frames[i];
            field("workshop-current-frame")->SetValue(
                number((unsigned) session->timeline_frame));
            update_timeline_cursor();
        }
        break;
    }
    show_event();
    show_action_timeline();
    set_text("workshop-editor-status", "Added an action-local event. Edit "
             "its properties, then save the draft.");
}

void delete_selected_event()
{
    if (session->project == nullptr || session->selected_action < 0 || session->selected_event < 0 ||
        (size_t) session->selected_event >= session->event_count) return;
    const unsigned action = (unsigned) session->selected_action;
    const uint32_t source = session->events[session->selected_event].source_offset;
    const size_t previous_size = session->project->fighter_size;
    std::vector<uint8_t> previous(session->project->fighter_dat,
                                  session->project->fighter_dat + previous_size);
    if (!nativeFighterAction_DeleteCommand(session->project, action, source)) {
        set_text("workshop-editor-status", "Could not remove this event. "
                 "A branch may target it, or it may be the final command.");
        return;
    }
    record_structural_edit(false, previous.data(), previous_size);
    session->action_dirty = true;
    select_action(action);
    set_text("workshop-editor-status", "Removed the event from this "
             "action. Save the draft to keep the edit.");
}

void TimelineRulerEvents::ProcessEvent(Rml::Event& event)
{
    if (session->project == nullptr || session->mode != WorkshopMode::Action) return;
    const auto type = event.GetType();
    if (type == "mousedown") seeking = true;
    if (type == "mouseup") { seeking = false; return; }
    if (type == "dragend") {
        nativeShellWorkshopEditor_ScrubTimeline(
            "workshop-event-timeline-ruler",
            event.GetParameter<int>("mouse_x", 0));
        seeking = false;
        return;
    }
    if ((type == "mousemove" && seeking) || type == "mousedown" ||
        type == "dragstart" || type == "drag")
        nativeShellWorkshopEditor_ScrubTimeline(
            "workshop-event-timeline-ruler",
            event.GetParameter<int>("mouse_x", 0));
}

void WorkshopCameraEvents::ProcessEvent(Rml::Event& event)
{
    const auto type = event.GetType();
    if (type == "mouseup") {
        if (event.GetParameter<int>("button", -1) == 2) dragging = false;
        return;
    }
    if (type == "mousemove") {
        if (!dragging) return;
        const int x = event.GetParameter<int>("mouse_x", last_x);
        const int y = event.GetParameter<int>("mouse_y", last_y);
        nativeHome_WorkshopCameraDrag(float(x - last_x), float(y - last_y),
            event.GetParameter<int>("shift_key", 0) != 0);
        last_x = x;
        last_y = y;
        return;
    }
    if (session->project == nullptr || session->action_preview_playing ||
        !nativeHome_WorkshopCameraActive()) return;
    if (type == "mousescroll") {
        const float steps = event.GetParameter<float>("wheel_delta_y", 0.0f);
        if (steps != 0.0f) nativeHome_WorkshopCameraZoom(steps);
        event.StopPropagation();
        return;
    }
    if (type != "mousedown" || event.GetParameter<int>("button", -1) != 2)
        return;
    Rml::Element* viewport = event.GetCurrentElement();
    const int x = event.GetParameter<int>("mouse_x", 0);
    const int y = event.GetParameter<int>("mouse_y", 0);
    if (y < viewport->GetAbsoluteOffset().y + 41) return;
    dragging = true;
    last_x = x;
    last_y = y;
    // RmlUi otherwise interprets middle click as document autoscroll.
    event.StopPropagation();
}

void TimelineSplitterEvents::ProcessEvent(Rml::Event& event)
{
    Rml::Element* target = event.GetCurrentElement();
    if (target == nullptr || session->graph_focused) return;
    const bool animation = target->GetId() ==
        "workshop-animation-splitter";
    if (event.GetType() == "dragstart") {
        active = true;
        start_y = event.GetParameter<int>("mouse_y", 0);
        start_fraction = animation ? session->animation_timeline_fraction :
                                     session->action_timeline_fraction;
        return;
    }
    if (event.GetType() != "dragend" || !active) return;
    active = false;
    const int height = document->GetContext()->GetDimensions().y;
    if (height <= 0) return;
    const int delta = event.GetParameter<int>("mouse_y", start_y) - start_y;
    float& fraction = animation ? session->animation_timeline_fraction :
                                  session->action_timeline_fraction;
    fraction = std::clamp(start_fraction - float(delta) / height,
                          animation ? 0.32f : 0.22f, 0.65f);
    session->graph_refresh = true;
}

void apply_history(bool redo)
{
    if (session->project == nullptr || (redo ? session->edit_cursor >= session->edit_history.size()
                                    : session->edit_cursor == 0)) return;
    if (auto* focus = document->GetContext()->GetFocusElement()) {
        if (focus->GetId().compare(0,14,"workshop-bone-") == 0) focus->Blur();
    }
    const EditHistoryEntry& edit = session->edit_history[redo ? session->edit_cursor :
                                                     session->edit_cursor - 1];
    if (edit.source_edit) {
        if (apply_source_history(edit, redo)) {
            if (redo) ++session->edit_cursor; else --session->edit_cursor;
            update_history_buttons();
        }
        return;
    }
    if(edit.rig_revision != session->project->rig_edit_revision) {
        set_text("workshop-editor-status", "Undo the bone edits in Model before restoring this edit.");
        return;
    }
    if (edit.label_edit) {
        const std::string& value = redo ? edit.after_name : edit.before_name;
        bool hidden = redo ? edit.after_hidden : edit.before_hidden;
        if (!nativeFighterAuthoring_SetName(
                session->project, NativeAuthoring_Action, edit.label_action,
                value.c_str()) ||
            !nativeFighterAuthoring_SetHidden(session->project,
                                              edit.label_action, hidden))
        {
            return;
        }
        if (redo) {
            ++session->edit_cursor;
        } else {
            --session->edit_cursor;
        }
        update_history_buttons();
        nativeShellWorkshopEditor_FilterActions();
        nativeShellWorkshopEditor_FilterClips();
        if (session->selected_action == (int) edit.label_action) {
            field("workshop-action-name")
                ->SetValue(value.empty()
                               ? WorkshopLabels::action(
                                     session->actions[edit.label_action].name)
                               : value);
            set_text("workshop-action-hide", hidden ? "Show action in editor"
                                                    : "Hide action in editor");
            set_text("workshop-context-action", value.empty() ? WorkshopLabels::action(session->actions[edit.label_action].name) : value);
        }
        return;
    }
    if (!edit.poses.empty()) {
        stop_action_preview();
        session->animation_clip = edit.pose_clip;
        session->animation_frame = edit.pose_frame;
        nativeWorkshopPose_Begin(session->project, edit.pose_clip, edit.pose_frame);
        nativeWorkshopPose_Restore(edit.poses.data(), (unsigned) edit.poses.size(), redo);
        field("workshop-animation-current-frame")->SetValue(number(edit.pose_frame));
        if (redo) ++session->edit_cursor; else --session->edit_cursor;
        update_history_buttons();
        show_bone_transform();
        session->graph_refresh = true;
        return;
    }
    nativeWorkshopPose_Clear();
    nativeHome_WorkshopPoseInvalidate();
    const auto& fighter = redo ? edit.after_fighter : edit.before_fighter;
    uint8_t* replacement_fighter = nullptr;
    if (!fighter.empty()) {
        replacement_fighter = (uint8_t*) std::malloc(fighter.size());
        if (!replacement_fighter) {
            return;
        }
        std::memcpy(replacement_fighter, fighter.data(), fighter.size());
        NativeFighterProject candidate=*session->project;
        candidate.fighter_dat=replacement_fighter;candidate.fighter_size=fighter.size();
        if(!nativeFighterModel_RestoreSelectors(&candidate,session->project->fighter_dat,session->project->fighter_size)) {
            std::free(replacement_fighter);return;
        }
    }
    if (!edit.before_bank.empty()) {
        const std::vector<uint8_t>& replacement =
            redo ? edit.after_bank : edit.before_bank;
        uint8_t* bank = (uint8_t*) std::malloc(replacement.size());
        if (bank == nullptr) {
            std::free(replacement_fighter);
            return;
        }
        std::memcpy(bank, replacement.data(), replacement.size());
        if(!edit.animation) {
            NativeFighterProject candidate=*session->project;
            candidate.fighter_dat=bank;candidate.fighter_size=replacement.size();
            if(!nativeFighterModel_RestoreSelectors(&candidate,session->project->fighter_dat,session->project->fighter_size)) {
                std::free(bank);std::free(replacement_fighter);return;
            }
        }
        if (edit.animation) {
            std::free(session->project->animation_dat);
            session->project->animation_dat = bank;
            session->project->animation_size = replacement.size();
        } else {
            std::free(session->project->fighter_dat);
            session->project->fighter_dat = bank;
            session->project->fighter_size = replacement.size();
        }
    } else {
        uint8_t* bank = edit.animation ? session->project->animation_dat :
                                         session->project->fighter_dat;
        const size_t size = edit.animation ? session->project->animation_size :
                                           session->project->fighter_size;
        for (const EditedByte& byte : edit.bytes) {
            if (byte.offset >= size) {
                std::free(replacement_fighter);
                return;
            }
            bank[byte.offset] = redo ? byte.after : byte.before;
        }
    }
    if (!fighter.empty()) {
        std::free(session->project->fighter_dat);
        session->project->fighter_dat = replacement_fighter;
        session->project->fighter_size = fighter.size();
        session->action_dirty = true;
    }
    if (redo) ++session->edit_cursor;
    else --session->edit_cursor;
    update_history_buttons();
    if (edit.animation) session->animation_dirty = true;
    else {
        session->action_dirty = true;
    }
    const int action = session->selected_action;
    const NativeFighterAnimationClip clip = session->animation_clip;
    nativeFighterAction_List(session->project, session->actions, 1024,
                             &session->action_count);
    const int animation_frame = session->animation_frame;
    const int track = session->selected_track;
    const int key = session->selected_key;
    const uint32_t source = session->selected_event >= 0 &&
        (size_t) session->selected_event < session->event_count
        ? session->events[session->selected_event].source_offset : UINT_MAX;
    if (action >= 0) select_action((unsigned) action);
    if (session->mode == WorkshopMode::Animation && clip.bank == NativeFighterAnimation_Demo) {
        read_demo_clips();
        select_demo_clip(clip.index);
        session->animation_frame = animation_frame;
        show_animation();
    }
    if (session->mode == WorkshopMode::Action && source != UINT_MAX) {
        for (size_t i = 0; i < session->event_count; ++i) {
            if (session->events[i].source_offset != source) continue;
            session->selected_event = (int) i;
            for (size_t command = 0; command < session->command_count; ++command)
                if (session->commands[command].source_offset == source) {
                    session->selected_command = (int) command;
                    break;
                }
            show_event();
            show_commands();
            show_action_timeline();
            break;
        }
    } else if (session->mode == WorkshopMode::Animation && track >= 0 &&
               (size_t) track < session->track_count) {
        select_track((unsigned) track);
        if (key >= 0 && (size_t) key < session->key_count) select_key((unsigned) key);
    }
    session->animation_frame = std::clamp(animation_frame, 0, int(std::ceil(session->clip_frames)));
    field("workshop-animation-current-frame")->SetValue(number(session->animation_frame));
    nativeHome_WorkshopPoseInvalidate();
    session->bone_pose_refresh = true;
    session->graph_refresh = true;
    if (!fighter.empty()) nativeShellWorkshop_RequestPreview(false);
    set_text("workshop-editor-status", redo ? "Reapplied workshop edit."
                                            : "Undid workshop edit.");
}

void select_command(unsigned index)
{
    if (index >= session->command_count) return;
    stop_action_preview();
    session->selected_command = (int) index;
    session->selected_event = -1;
    for (size_t i = 0; i < session->event_count; ++i) {
        if (session->events[i].source_offset == session->commands[index].source_offset) {
            session->selected_event = (int) i;
            break;
        }
    }
    if (session->selected_event < 0) {
        document->GetElementById("workshop-action-playback")->SetProperty("display", "none");
        document->GetElementById("workshop-command-fields")->SetProperty("display", "none");
        document->GetElementById("workshop-delete-event")
            ->SetProperty("display", "none");
        set_text("workshop-action-detail", "Original opcode " +
                 number(session->commands[index].opcode) + " at source offset " +
                 number(session->commands[index].source_offset) +
                 ". This command has no inspector yet.");
        document->GetElementById("workshop-event-fields")
            ->SetProperty("display", "none");
        document->GetElementById("workshop-value-fields")
            ->SetProperty("display", "none");
        document->GetElementById("workshop-sound-fields")
            ->SetProperty("display", "none");
        document->GetElementById("workshop-effect-fields")
            ->SetProperty("display", "none");
    } else show_event();
    show_commands();
    show_action_timeline();
}


bool begin_fighter_event_edit(uint32_t offset, FighterEventEdit* edit)
{
    unsigned users;
    if (!nativeFighterAction_Users(session->project, offset, &users) || users == 0)
        return false;
    edit->size = session->project->fighter_size;
    edit->before = (uint8_t*) std::malloc(edit->size);
    if (edit->before == nullptr) return false;
    std::memcpy(edit->before, session->project->fighter_dat, edit->size);
    edit->offset = offset;
    edit->localized = users > 1;
    if (edit->localized && !nativeFighterAction_LocalizeCommand(
            session->project, (unsigned) session->selected_action, offset, &edit->offset)) {
        std::free(edit->before);
        edit->before = nullptr;
        return false;
    }
    return true;
}

void cancel_fighter_event_edit(FighterEventEdit* edit)
{
    if (edit->before == nullptr) return;
    if (edit->localized) {
        std::free(session->project->fighter_dat);
        session->project->fighter_dat = edit->before;
        session->project->fighter_size = edit->size;
    } else {
        std::memcpy(session->project->fighter_dat, edit->before, edit->size);
        std::free(edit->before);
    }
    edit->before = nullptr;
}

void commit_fighter_event_edit(FighterEventEdit* edit)
{
    if (edit->localized)
        record_structural_edit(false, edit->before, edit->size);
    else record_edit(false, edit->before);
    std::free(edit->before);
    edit->before = nullptr;
    session->action_dirty = true;
    if (edit->localized) {
        const unsigned action = (unsigned) session->selected_action;
        select_action(action);
        for (size_t i = 0; i < session->event_count; ++i) {
            if (session->events[i].source_offset != edit->offset) continue;
            session->selected_event = (int) i;
            for (size_t command = 0; command < session->command_count; ++command)
                if (session->commands[command].source_offset == edit->offset) {
                    session->selected_command = (int) command;
                    break;
                }
            break;
        }
    } else {
        nativeFighterAction_Events(session->project, (unsigned) session->selected_action,
                                   session->events, 256, &session->event_count);
    }
    show_action_timeline();
    show_event();
}

void apply_hitbox()
{
    if (session->project == nullptr || session->selected_action < 0 || session->selected_event < 0 ||
        (size_t) session->selected_event >= session->event_count ||
        session->events[session->selected_event].kind != NativeFighterActionEvent_CreateHitbox)
        return;
    const uint32_t offset = session->events[session->selected_event].source_offset;
    NativeFighterHitbox hitbox;
    if (!nativeFighterAction_GetHitbox(session->project, (unsigned) session->selected_action,
                                      offset, &hitbox)) return;
    if (hitbox.timed) {
        const std::string text = field("workshop-hitbox-duration")->GetValue();
        char* end; const double frames = std::strtod(text.c_str(), &end);
        if (text.empty() || *end || !std::isfinite(frames) || frames < 1.0/256.0 || frames > 1000) {
            set_text("workshop-editor-status", "Duration must be between 1/256 and 1000 animation frames.");
            return;
        }
        hitbox.duration256 = (uint32_t) std::lround(frames * 256);
        hitbox.refresh_hit_eligibility = document->GetElementById("workshop-hitbox-refresh")->HasAttribute("checked");
    }
    uint16_t bone;
    const char* spatial_ids[] = {"workshop-hitbox-radius", "workshop-hitbox-offset-x",
                                 "workshop-hitbox-offset-y", "workshop-hitbox-offset-z"};
    long spatial[4];
    for (unsigned axis = 0; axis < 4; ++axis) {
        float value;
        if (!parse_float(spatial_ids[axis], &value) ||
            value < (axis ? -32768 : 0) * HitboxDistanceScale ||
            value > (axis ? 32767 : 65535) * HitboxDistanceScale) {
            set_text("workshop-editor-status", "Enter a radius from 0 to 255.98 and offsets from -127.99 to 127.99 game units.");
            return;
        }
        spatial[axis] = std::lround(value / HitboxDistanceScale);
    }
    hitbox.radius = (uint16_t) spatial[0];
    hitbox.offset_x = (int16_t) spatial[1];
    hitbox.offset_y = (int16_t) spatial[2];
    hitbox.offset_z = (int16_t) spatial[3];
    if (!parse_u16("workshop-hitbox-damage", 1023, &hitbox.damage) ||
        !parse_u16("workshop-hitbox-angle", 511, &hitbox.angle) ||
        !parse_u16("workshop-hitbox-kbg", 511, &hitbox.knockback_growth) ||
        !parse_u16("workshop-hitbox-bkb", 511, &hitbox.base_knockback) ||
        !parse_u16("workshop-hitbox-bone", 255, &bone)) {
        set_text("workshop-editor-status", "Enter whole values within the "
                 "original command's range.");
        return;
    }
    hitbox.bone = (uint8_t) bone;
    uint16_t group, sound_level, sound_kind;
    const std::string shield_text = field("workshop-hitbox-shield")->GetValue();
    char* shield_end;
    const long shield = std::strtol(shield_text.c_str(), &shield_end, 10);
    if (shield_end == shield_text.c_str() || *shield_end ||
        shield < -128 || shield > 127 ||
        !parse_u16("workshop-hitbox-wkb", 511, &hitbox.weight_set_knockback) ||
        !parse_u16("workshop-hitbox-rehit-group", 7, &group) ||
        !parse_u16("workshop-hitbox-sfx-level", 7, &sound_level) ||
        !parse_u16("workshop-hitbox-sfx-kind", 31, &sound_kind)) {
        set_text("workshop-editor-status", "Enter valid hitbox values. Shield damage ranges from -128 to 127.");
        return;
    }
    hitbox.shield_damage = (int8_t) shield;
    hitbox.hit_group = (uint8_t) group;
    hitbox.hit_sfx_severity = (uint8_t) sound_level;
    hitbox.hit_sfx_kind = (uint8_t) sound_kind;
    const std::string element = static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("workshop-hitbox-element"))->GetValue();
    if (element.empty()) return;
    hitbox.element = (uint8_t) std::strtoul(element.c_str(), nullptr, 10);
    for (const auto& toggle : hitbox_toggles)
        hitbox.*toggle.member = document->GetElementById(toggle.id)->HasAttribute("checked");
    FighterEventEdit edit = {};
    if (!begin_fighter_event_edit(offset, &edit)) {
        set_text("workshop-editor-status", "Could not prepare this "
                 "action's source for editing.");
        return;
    }
    if (!nativeFighterAction_SetHitbox(session->project, (unsigned) session->selected_action,
                                       edit.offset, &hitbox)) {
        cancel_fighter_event_edit(&edit);
        set_text("workshop-editor-status", "Could not edit this hitbox "
                 "within its source encoding.");
        return;
    }
    commit_fighter_event_edit(&edit);
    set_text("workshop-editor-status", "Hitbox changed in the draft. Save "
             "before leaving this fighter.");
}

void apply_event_value()
{
    if (session->project == nullptr || session->selected_action < 0 || session->selected_event < 0 ||
        (size_t) session->selected_event >= session->event_count) return;
    const std::string entered = session->events[session->selected_event].kind == NativeFighterActionEvent_SetAirborneState
        ? static_cast<Rml::ElementFormControlSelect*>(document->GetElementById(
              "workshop-air-state"))->GetValue()
        : field("workshop-event-value")->GetValue();
    char* end = nullptr;
    errno = 0;
    const unsigned long parsed = std::strtoul(entered.c_str(), &end, 10);
    if (entered.empty() || entered[0] == '-' || *end != '\0' ||
        errno != 0 || parsed > UINT_MAX) {
        set_text("workshop-editor-status", "Enter a nonnegative whole command value.");
        return;
    }
    const uint32_t offset = session->events[session->selected_event].source_offset;
    FighterEventEdit edit = {};
    if (!begin_fighter_event_edit(offset, &edit)) {
        set_text("workshop-editor-status", "Could not prepare this "
                 "action's source for editing.");
        return;
    }
    if (!nativeFighterAction_SetValue(session->project, (unsigned) session->selected_action,
                                      edit.offset, (uint32_t) parsed)) {
        cancel_fighter_event_edit(&edit);
        set_text("workshop-editor-status", "This value is outside the "
                 "source command's range.");
        return;
    }
    commit_fighter_event_edit(&edit);
    set_text("workshop-editor-status", "Event changed in the draft. Save to keep it.");
}

void apply_sound()
{
    if (session->project == nullptr || session->selected_action < 0 || session->selected_event < 0 ||
        session->events[session->selected_event].kind != NativeFighterActionEvent_SoundEffect)
        return;
    const uint32_t offset = session->events[session->selected_event].source_offset;
    NativeFighterSoundEffect sound;
    uint16_t volume, pan;
    if (!nativeFighterAction_GetSoundEffect(session->project, (unsigned) session->selected_action,
                                            offset, &sound) ||
        !parse_u32("workshop-sound-id", &sound.sfx_id) ||
        !parse_u16("workshop-sound-volume", 255, &volume) ||
        !parse_u16("workshop-sound-pan", 255, &pan)) {
        set_text("workshop-editor-status", "Enter a valid sound ID, volume and panning.");
        return;
    }
    sound.volume = (uint8_t) volume;
    sound.panning = (uint8_t) pan;
    FighterEventEdit edit = {};
    if (!begin_fighter_event_edit(offset, &edit)) {
        set_text("workshop-editor-status", "Could not prepare this "
                 "action's source for editing.");
        return;
    }
    if (!nativeFighterAction_SetSoundEffect(session->project, (unsigned) session->selected_action,
                                            edit.offset, &sound)) {
        cancel_fighter_event_edit(&edit);
        set_text("workshop-editor-status", "This sound command cannot be edited "
                 "in this encoding; canned sounds are read-only.");
        return;
    }
    commit_fighter_event_edit(&edit);
    set_text("workshop-editor-status", "Sound changed in the draft. Save to keep it.");
}

void apply_effect()
{
    if (session->project == nullptr || session->selected_action < 0 || session->selected_event < 0 ||
        session->events[session->selected_event].kind != NativeFighterActionEvent_SpawnEffect)
        return;
    const uint32_t offset = session->events[session->selected_event].source_offset;
    NativeFighterSpawnEffect effect;
    uint16_t id, bone;
    if (!nativeFighterAction_GetSpawnEffect(session->project, (unsigned) session->selected_action,
                                             offset, &effect) ||
        !parse_u16("workshop-effect-id", 65535, &id) ||
        !parse_u16("workshop-effect-bone", 255, &bone) ||
        !parse_i16("workshop-effect-x", &effect.offset_x) ||
        !parse_i16("workshop-effect-y", &effect.offset_y) ||
        !parse_i16("workshop-effect-z", &effect.offset_z)) {
        set_text("workshop-editor-status", "Enter valid effect, bone and offset values.");
        return;
    }
    effect.gfx_id = id;
    effect.bone = (uint8_t) bone;
    FighterEventEdit edit = {};
    if (!begin_fighter_event_edit(offset, &edit)) {
        set_text("workshop-editor-status", "Could not prepare this "
                 "action's source for editing.");
        return;
    }
    if (!nativeFighterAction_SetSpawnEffect(session->project, (unsigned) session->selected_action,
                                            edit.offset, &effect)) {
        cancel_fighter_event_edit(&edit);
        set_text("workshop-editor-status", "This effect command cannot be edited "
                 "in this source encoding; verify its bone and offsets.");
        return;
    }
    commit_fighter_event_edit(&edit);
    set_text("workshop-editor-status", "Effect changed in the draft. Save to keep it.");
}


} // namespace WorkshopEditor

#endif
