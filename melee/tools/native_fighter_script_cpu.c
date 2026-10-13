#include "native_fighter_script_code.h"

#include <stdio.h>
#include <string.h>

const char* nativeScriptCpu_Property(const char* name)
{
    static const struct { const char* name; const char* code; } properties[] = {
        { "cpu.mode", "fighter->cpu.xC" },
        { "cpu.follow_distance", "fighter->cpu.x3C" },
        { "cpu.combat_candidates_enabled", "fighter->cpu.xF9_b2" },
        { "cpu.ranged_gate_enabled", "fighter->cpu.xF9_b4" },
        { "motion.allows_controller_sync", "fighter->x2225_b3" },
        { "cpu.output.left_x", "fighter->cpu.lstick.x" },
        { "cpu.output.left_y", "fighter->cpu.lstick.y" },
        { "cpu.output.buttons", "fighter->cpu.buttons" },
        { "cpu.output.left_trigger", "fighter->cpu.ltrigger" },
        { "cpu.output.right_trigger", "fighter->cpu.rtrigger" },
        { "cpu.level", "fighter->cpu.level" },
        { "cpu.behavior", "fighter->cpu.x18" },
        { "cpu.selected_command", "fighter->cpu.xA4" },
        { "cpu.previous_behavior", "fighter->cpu.x1C" },
        { "cpu.elapsed_frames", "fighter->cpu.x7C" },
        { "cpu.travel_frames", "fighter->cpu.x90" },
        { "cpu.pending_action", "fighter->cpu.xF8_b5" },
        { "cpu.target_id", "(fighter->cpu.x44 != NULL ? (uint32_t)fighter->cpu.x44->x8_spawnNum : 0)" },
        { "cpu.grab_attempts", "fighter->cpu.x94" },
        { "cpu.approach_spacing", "fighter->cpu.x56C" },
        { "cpu.floor_in_bounds", "fighter->cpu.xFA_b5" },
        { "cpu.body_half_width", "fighter->cpu.x564" },
        { "cpu.destination.x", "fighter->cpu.x54.x" },
        { "cpu.destination.y", "fighter->cpu.x54.y" },
    };
    for (unsigned i = 0; i < sizeof(properties) / sizeof(properties[0]); ++i)
        if (strcmp(name, properties[i].name) == 0) return properties[i].code;
    return NULL;
}

bool nativeScriptCpu_Action(const ScriptCodeBlock* block, const char* source,
                            char* output, size_t capacity)
{
    if (block == NULL || block->item_event || block->model_effect) return false;
    static const struct { const char* name; const char* field; ScriptExpressionType type; } fields[] = {
        { "cpu.mode", "xC", ScriptType_Int },
        { "cpu.level", "level", ScriptType_Int },
        { "cpu.ranged_gate_enabled", "xF9_b4", ScriptType_Bool },
        { "cpu.combat_candidates_enabled", "xF9_b2", ScriptType_Bool },
        { "cpu.travel_frames", "x90", ScriptType_Int },
        { "cpu.pending_action", "xF8_b5", ScriptType_Bool },
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(*fields); ++i) {
        size_t n = strlen(fields[i].name);
        if (strncmp(source, fields[i].name, n) || strncmp(source + n, " = ", 3)) continue;
        char value[512];
        if (nativeScriptCode_ExpressionType(block, source + n + 3) != fields[i].type ||
            !nativeScriptCode_Expression(block, source + n + 3, value, sizeof(value))) return false;
        int written = snprintf(output, capacity, fields[i].type == ScriptType_Bool ?
            "fighter->cpu.%s = (%s) != 0;" : "fighter->cpu.%s = (%s);", fields[i].field, value);
        return written >= 0 && (size_t)written < capacity;
    }
    static const struct { const char* name; const char* field; int minimum, maximum; } outputs[] = {
        { "cpu.output.left_x", "lstick.x", -128, 127 },
        { "cpu.output.left_y", "lstick.y", -128, 127 },
        { "cpu.output.left_trigger", "ltrigger", 0, 255 },
        { "cpu.output.right_trigger", "rtrigger", 0, 255 },
    };
    for (unsigned i = 0; i < sizeof(outputs) / sizeof(*outputs); ++i) {
        size_t n = strlen(outputs[i].name);
        if (strncmp(source, outputs[i].name, n) || strncmp(source + n, " = ", 3)) continue;
        char value[512];
        if (nativeScriptCode_ExpressionType(block, source + n + 3) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, source + n + 3, value, sizeof(value))) return false;
        int written = snprintf(output, capacity,
            "{ int32_t cpu_output = (%s); if (cpu_output >= %d && cpu_output <= %d) "
            "fighter->cpu.%s = cpu_output; }", value, outputs[i].minimum, outputs[i].maximum, outputs[i].field);
        return written >= 0 && (size_t)written < capacity;
    }
    static const char buttons[] = "cpu.output.buttons = ";
    if (!strncmp(source, buttons, sizeof(buttons) - 1)) {
        char value[512]; const char* expression = source + sizeof(buttons) - 1;
        if (nativeScriptCode_ExpressionType(block, expression) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, expression, value, sizeof(value))) return false;
        int written = snprintf(output, capacity, "fighter->cpu.buttons = (uint32_t)(%s);", value);
        return written >= 0 && (size_t)written < capacity;
    }
    const struct { const char* name; const char* host; } common[] = {
        { "cpu.prepare_combat_plan", "cpu_prepare_combat_plan" },
        { "cpu.update_recovery_destination", "cpu_update_recovery_destination" },
        { "cpu.plan_without_target", "cpu_plan_without_target" },
    };
    for (unsigned i = 0; i < sizeof(common) / sizeof(*common); ++i) {
        size_t n = strlen(common[i].name); const char* after; char empty[1][192];
        if (strncmp(source, common[i].name, n) || source[n] != '(') continue;
        if (!nativeScriptCode_ParseArguments(source + n, empty, 0, &after) || *after) return false;
        int written = snprintf(output, capacity, "engine->%s(fighter);", common[i].host);
        return written >= 0 && (size_t)written < capacity;
    }
    static const char destination[] = "cpu.set_destination(";
    if (!strncmp(source, destination, sizeof(destination) - 1)) {
        char args[3][192], values[3][512]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(destination) - 2, args, 3, &after) || *after) return false;
        for (unsigned i = 0; i < 3; ++i) {
            ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
            if ((type != ScriptType_Int && type != ScriptType_Float) ||
                !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        }
        int written = snprintf(output, capacity,
            "{ float cpu_x = (%s); float cpu_y = (%s); float cpu_spacing = (%s); "
            "engine->cpu_set_destination(fighter, cpu_x, cpu_y, cpu_spacing); }",
            values[0], values[1], values[2]);
        return written >= 0 && (size_t)written < capacity;
    }
    static const char distance[] = "cpu.follow_distance = ";
    if (!strncmp(source, distance, sizeof(distance) - 1)) {
        const char* expression = source + sizeof(distance) - 1;
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, expression);
        char value[512];
        if ((type != ScriptType_Float && type != ScriptType_Int) ||
            !nativeScriptCode_Expression(block, expression, value, sizeof(value))) return false;
        int written = snprintf(output, capacity,
            "{ float follow_distance = (%s); if (isfinite(follow_distance) && follow_distance >= 0.0f) fighter->cpu.x3C = follow_distance; }", value);
        return written >= 0 && (size_t) written < capacity;
    }
    const char* method_end = strchr(source, '(');
    if (method_end != NULL && (size_t)(method_end - source) < 96) {
        char name[96], expression[1536];
        memcpy(name, source, method_end - source); name[method_end - source] = 0;
        const char* after = method_end;
        int found = nativeScriptCpu_Expression(block, name, &after, expression, sizeof(expression));
        if (found < 0) return false;
        if (found > 0) {
            if (*after) return false;
            int written = snprintf(output, capacity, "%s;", expression);
            return written >= 0 && (size_t)written < capacity;
        }
    }
    static const char candidate[] = "cpu.add_candidate(";
    const char* after;
    static const char run_script[] = "cpu.run_script(";
    if (strncmp(source, run_script, sizeof(run_script) - 1) == 0) {
        char args[1][192], value[256];
        if (!nativeScriptCode_ParseArguments(source + sizeof(run_script) - 2,
                                             args, 1, &after) || *after ||
            !nativeScriptTypes_Assignable(ScriptType_Int,
                nativeScriptCode_ExpressionType(block, args[0])) ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value)))
            return false;
        // Evaluate once; an invalid index cannot read outside the common DAT.
        int length = snprintf(output, capacity,
            "{ int script_index = %s; if (script_index > 0 && "
            "script_index < FT_CPU_SCRIPT_COUNT) "
            "engine->cpu_run_script(fighter, script_index); }", value);
        return length >= 0 && (size_t) length < capacity;
    }
    if (strcmp(source, "cpu.clear_target()") == 0)
        return snprintf(output, capacity, "fighter->cpu.x44 = NULL;") < capacity;
    static const char selected[] = "cpu.selected_command = ";
    if (strncmp(source, selected, sizeof(selected) - 1) == 0) {
        const char* expression = source + sizeof(selected) - 1;
        char value[512];
        if (!nativeScriptTypes_Assignable(ScriptType_Int, nativeScriptCode_ExpressionType(block, expression)) ||
            !nativeScriptCode_Expression(block, expression, value, sizeof(value))) return false;
        int length = snprintf(output, capacity,
            "{ int32_t command = (%s); if (command >= 0 && command <= 61) fighter->cpu.xA4 = command; }", value);
        return length >= 0 && (size_t)length < capacity;
    }
    static const char behavior[] = "cpu.behavior = ";
    if (strncmp(source, behavior, sizeof(behavior) - 1) == 0) {
        char value[512];
        if (!nativeScriptCode_Expression(block, source + sizeof(behavior) - 1,
                                         value, sizeof(value))) return false;
        return snprintf(output, capacity, "fighter->cpu.x18 = (int) (%s);", value) < capacity;
    }
    if (strncmp(source, candidate, sizeof(candidate) - 1) == 0) {
        char args[1][192], value[128];
        if (!nativeScriptCode_ParseArguments(source + sizeof(candidate) - 2,
                                             args, 1, &after) || *after ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value)))
            return false;
        int length = snprintf(output, capacity,
            "engine->cpu_add_candidate(fighter, (int) (%s));", value);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char spacing[] = "cpu.set_approach_spacing(";
    if (strncmp(source, spacing, sizeof(spacing) - 1) == 0) {
        char args[1][192], value[128];
        if (!nativeScriptCode_ParseArguments(source + sizeof(spacing) - 2,
                                             args, 1, &after) || *after ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value)))
            return false;
        int length = snprintf(output, capacity, "fighter->cpu.x56C = %s;", value);
        return length >= 0 && (size_t) length < capacity;
    }
    static const char queue[] = "cpu.queue(";
    if (strncmp(source, queue, sizeof(queue) - 1) != 0) return false;
    static const struct {
        const char* name;
        const char* command;
        unsigned arguments;
    } commands[] = {
        { "Cpu.press_a", "CpuCmd_PressA", 0 },
        { "Cpu.release_a", "CpuCmd_ReleaseA", 0 },
        { "Cpu.press_b", "CpuCmd_PressB", 0 },
        { "Cpu.release_b", "CpuCmd_ReleaseB", 0 },
        { "Cpu.press_x", "CpuCmd_PressX", 0 },
        { "Cpu.release_x", "CpuCmd_ReleaseX", 0 },
        { "Cpu.press_y", "CpuCmd_PressY", 0 },
        { "Cpu.release_y", "CpuCmd_ReleaseY", 0 },
        { "Cpu.press_r", "CpuCmd_PressR", 0 },
        { "Cpu.release_r", "CpuCmd_ReleaseR", 0 },
        { "Cpu.press_l", "CpuCmd_PressL", 0 },
        { "Cpu.release_l", "CpuCmd_ReleaseL", 0 },
        { "Cpu.press_z", "CpuCmd_PressZ", 0 },
        { "Cpu.release_z", "CpuCmd_ReleaseZ", 0 },
        { "Cpu.press_up", "CpuCmd_PressUp", 0 },
        { "Cpu.release_up", "CpuCmd_ReleaseUp", 0 },
        { "Cpu.press_down", "CpuCmd_PressDown", 0 },
        { "Cpu.release_down", "CpuCmd_ReleaseDown", 0 },
        { "Cpu.press_right", "CpuCmd_PressRight", 0 },
        { "Cpu.release_right", "CpuCmd_ReleaseRight", 0 },
        { "Cpu.press_left", "CpuCmd_PressLeft", 0 },
        { "Cpu.release_left", "CpuCmd_ReleaseLeft", 0 },
        { "Cpu.press_start", "CpuCmd_PressStart", 0 },
        { "Cpu.release_start", "CpuCmd_ReleaseStart", 0 },
        { "Cpu.release_all", "CpuCmd_ReleaseAll", 0 },
        { "Cpu.done", "CpuCmd_Done", 0 },
        { "Cpu.stick_x", "CpuCmd_SetLstickX", 1 },
        { "Cpu.stick_y", "CpuCmd_SetLstickY", 1 },
        { "Cpu.cstick_x", "CpuCmd_SetCstickX", 1 },
        { "Cpu.cstick_y", "CpuCmd_SetCstickY", 1 },
        { "Cpu.trigger_r", "CpuCmd_SetRtrigger", 1 },
        { "Cpu.trigger_l", "CpuCmd_SetLtrigger", 1 },
        { "Cpu.press_a_for", "CpuCmd_PressAFor", 1 },
        { "Cpu.release_a_for", "CpuCmd_ReleaseAFor", 1 },
        { "Cpu.press_b_for", "CpuCmd_PressBFor", 1 },
        { "Cpu.release_b_for", "CpuCmd_ReleaseBFor", 1 },
        { "Cpu.press_x_for", "CpuCmd_PressXFor", 1 },
        { "Cpu.release_x_for", "CpuCmd_ReleaseXFor", 1 },
        { "Cpu.press_y_for", "CpuCmd_PressYFor", 1 },
        { "Cpu.release_y_for", "CpuCmd_ReleaseYFor", 1 },
        { "Cpu.wait", "CpuCmd_WaitFor", 1 },
        { "Cpu.toward_destination", "CpuCmd_LstickTowardDestination", 1 },
        { "Cpu.x_toward_destination", "CpuCmd_LstickXTowardDestination", 1 },
        { "Cpu.forward", "CpuCmd_LstickXForward", 1 },
        { "Cpu.wait_if_motion", "CpuCmd_WaitIfMotionId", 1 },
        { "Cpu.set_behavior", "CpuCmd_Unk0x93", 1 },
        { "Cpu.toward_target", "CpuCmd_LstickTowardFighter", 1 },
        { "Cpu.x_toward_target", "CpuCmd_LstickXTowardFighter", 1 },
        { "Cpu.toward_destination_clamped", "CpuCmd_LstickTowardDestinationClamped", 2 },
        { "Cpu.x_toward_destination_clamped", "CpuCmd_LstickXTowardDestinationClamped", 2 },
        { "Cpu.forward_clamped", "CpuCmd_LstickForwardClamped", 2 },
    };
    for (unsigned i = 0; i < sizeof(commands) / sizeof(commands[0]); ++i) {
        char args[3][192], values[2][128];
        if (!nativeScriptCode_ParseArguments(source + sizeof(queue) - 2,
                    args, commands[i].arguments + 1, &after) || *after ||
            strcmp(args[0], commands[i].name) != 0) continue;
        for (unsigned j = 0; j < commands[i].arguments; ++j)
            if (!nativeScriptCode_Expression(block, args[j + 1], values[j],
                                             sizeof(values[j]))) return false;
        int length;
        if (commands[i].arguments == 0)
            length = snprintf(output, capacity, "engine->cpu_command(fighter, %s);",
                              commands[i].command);
        else if (commands[i].arguments == 1)
            // Locals are floats. Convert through int before the byte encoding so
            // negative stick values use defined integer modulo conversion.
            length = snprintf(output, capacity, "engine->cpu_command_arg(fighter, %s, (u8) (int) (%s));",
                              commands[i].command, values[0]);
        else
            length = snprintf(output, capacity, "engine->cpu_command_pair(fighter, %s, (u8) (int) (%s), (u8) (int) (%s));",
                              commands[i].command, values[0], values[1]);
        return length >= 0 && (size_t) length < capacity;
    }
    return false;
}

int nativeScriptCpu_Expression(const ScriptCodeBlock* block, const char* name,
    const char** source, char* output, size_t capacity)
{
    const struct { const char* name; const char* host; unsigned count; bool fighter; } calls[] = {
        { "cpu.record_input", "cpu_record_input", 1, true },
        { "cpu.apply_recorded_input", "cpu_apply_recorded_input", 0, true },
        { "cpu.prepare_destination_update", "cpu_prepare_destination_update", 0, true },
        { "cpu.plan_attack", "cpu_plan_attack", 0, true },
        { "stage.same_line_island", "stage_same_line_island", 2, false },
        { "stage.cpu_floor_excluded", "stage_cpu_floor_excluded", 1, false },
        { "stage.cpu_island_blocked", "stage_cpu_island_blocked", 1, false },
    };
    for (unsigned c = 0; c < sizeof(calls) / sizeof(*calls); ++c) {
        if (strcmp(name, calls[c].name)) continue;
        if (block == NULL || block->model_effect || block->item_event) return -1;
        char args[2][192], values[2][512]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, calls[c].count, &after)) return -1;
        for (unsigned i = 0; i < calls[c].count; ++i)
            if (nativeScriptCode_ExpressionType(block, args[i]) != ScriptType_Int ||
                !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return -1;
        const char* receiver = !strcmp(name, "cpu.record_input") || !strcmp(name, "cpu.apply_recorded_input") ? "gobj" : "fighter";
        int written;
        if (calls[c].fighter) written = snprintf(output, capacity, calls[c].count ?
            "engine->%s(%s, (uint32_t)(%s))" : "engine->%s(%s)",
            calls[c].host, receiver, calls[c].count ? values[0] : "");
        else if (calls[c].count == 1) written = snprintf(output, capacity,
            "engine->%s(%s)", calls[c].host, values[0]);
        else written = snprintf(output, capacity,
            "({ int32_t island_first = (%s); int32_t island_second = (%s); engine->%s(island_first, island_second); })",
            values[0], values[1], calls[c].host);
        if (written < 0 || (size_t)written >= capacity) return -1;
        *source = after; return 1;
    }
    return 0;
}
