#include "native_shell_workshop_editor_internal.h"
#ifdef MELEE_NATIVE
namespace WorkshopEditor {
    std::string event_name(const NativeFighterActionEvent& event)
    {
        return event.kind == NativeFighterActionEvent_Command
                   ? NativeFighterActionLabels::command(event.opcode)
                   : NativeFighterActionLabels::event(event.kind);
    }
    void show_command_fields(const NativeFighterActionEvent& event)
    {
        if (event.kind != NativeFighterActionEvent_Command &&
            event.kind != NativeFighterActionEvent_SetCommandVar)
        {
            return;
        }
        NativeFighterActionField fields[48];
        size_t count;
        if (!nativeFighterAction_Fields(
                session->project, (unsigned) session->selected_action,
                event.source_offset, fields, 48, &count))
        {
            return;
        }
        std::string markup;
        for (unsigned index = 0; index < count; ++index) {
            const char* name = NativeFighterActionLabels::field(event.opcode, index);
            if (name && std::strcmp(name, "Reserved") == 0) {
                continue;
            }
            const std::string label =
                name ? name : "Parameter " + number(index + 1);
            const std::string id = "workshop-command-field-" + number(index);
            markup +=
                "<div class='workshop-inspector-row'><label for='" + id +
                "' class='field-label' title='" +
                Rml::StringUtilities::EncodeRml(label) +
                (event.opcode == 31 && index == 1 ? " Â· -1 hides the group"
                                                  : "") +
                "'>" + Rml::StringUtilities::EncodeRml(label) +
                "</label><input id='" + id +
                "' class='room-input' type='text' value='" +
                (event.opcode == 31 && index == 1 &&
                         (fields[index].value & (1u << 18))
                     ? std::to_string((int32_t) fields[index].value -
                                      (1 << 19))
                     : number(fields[index].value)) +
                "'/></div>";
        }
        document->GetElementById("workshop-command-fields-inputs")
            ->SetInnerRML(markup);
        document->GetElementById("workshop-command-fields")
            ->SetProperty("display", "block");
        set_text("workshop-command-fields-title", event_name(event));
        const bool aerial =
            event.opcode == 19 && event.target_id == 0 &&
            std::strncmp(session->actions[session->selected_action].name,
                         "AttackAir", 9) == 0;
        set_text("workshop-command-fields-help",
                 aerial ? "Variable 0 controls landing: 0 enables autocancel; "
                          "nonzero uses this aerial's landing lag."
                 : event.opcode == 19
                     ? "Sets one of the four action command variables. Move "
                       "behavior reads these values."
                 : event.opcode == 31
                     ? "Choose a visibility group and its mesh variant. "
                       "Variant -1 hides the group."
                     : NativeFighterActionLabels::command_description(event.opcode));
    }
    void apply_command_fields()
    {
        if (!session->project || session->selected_action < 0 ||
            session->selected_event < 0)
        {
            return;
        }
        const auto event = session->events[session->selected_event];
        NativeFighterActionField fields[48];
        size_t count;
        if (!nativeFighterAction_Fields(
                session->project, (unsigned) session->selected_action,
                event.source_offset, fields, 48, &count))
        {
            return;
        }
        uint32_t values[48];
        for (unsigned i = 0; i < count; ++i) {
            const char* name = NativeFighterActionLabels::field(event.opcode, i);
            if (name && std::strcmp(name, "Reserved") == 0) {
                values[i] = fields[i].value;
                continue;
            }
            if (event.opcode == 31 && i == 1) {
                const std::string text =
                    field(("workshop-command-field-" + number(i)).c_str())
                        ->GetValue();
                char* end = nullptr;
                errno = 0;
                const long value = std::strtol(text.c_str(), &end, 10);
                if (text.empty() || *end || errno || value < -1 ||
                    value > 262143)
                {
                    set_text(
                        "workshop-editor-status",
                        "Choose a visible variant, or -1 to hide the group.");
                    return;
                }
                values[i] = (uint32_t) value & 0x7ffffu;
                continue;
            }
            if (!parse_u32(("workshop-command-field-" + number(i)).c_str(),
                           &values[i]) ||
                (fields[i].width != 32 &&
                 values[i] >= (1u << fields[i].width)))
            {
                set_text("workshop-editor-status",
                         "Enter whole values within each parameter's range.");
                return;
            }
        }
        FighterEventEdit edit = {};
        if (!begin_fighter_event_edit(event.source_offset, &edit)) {
            return;
        }
        for (unsigned i = 0; i < count; ++i) {
            if (!nativeFighterAction_SetField(
                    session->project, (unsigned) session->selected_action,
                    edit.offset, i, values[i]))
            {
                cancel_fighter_event_edit(&edit);
                return;
            }
        }
        commit_fighter_event_edit(&edit);
        nativeHome_WorkshopPoseInvalidate();
        set_text("workshop-editor-status", "Command properties updated.");
    }
}
#endif
