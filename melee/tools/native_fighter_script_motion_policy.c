#include "native_fighter_script_motion_policy.h"
#include "native_fighter_script_code.h"
#include <melee/ft/kinds/ftCommon/forward.h>

#include <stdio.h>
#include <string.h>

bool nativeScriptMotionPolicy_Parse(char* source, size_t size,
                                    ScriptMotionPolicies* policies,
                                    NativeFighterScriptError* error)
{
    *policies = (ScriptMotionPolicies){ 0 };
    bool code_seen = false;
    unsigned line = 0;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - start;
        ++line;
        if (length && source[start + length - 1] == '\r') --length;
        while (length && (source[start + length - 1] == ' ' ||
                          source[start + length - 1] == '\t')) --length;
        const char* text = source + start;
        if (length >= 14 && memcmp(text, "motion_policy ", 14) == 0) {
            unsigned motion, common;
            int consumed = 0;
            char declaration[128];
            if (length >= sizeof(declaration))
                return nativeScriptCode_Fail(error, line,
                    "Motion policy declaration is too long.");
            memcpy(declaration, text, length); declaration[length] = 0;
            if (code_seen || sscanf(declaration, "motion_policy %u common %u%n",
                    &motion, &common, &consumed) != 2 ||
                declaration[consumed] || motion > UINT16_MAX ||
                common >= ftCo_MS_Count || policies->count == 1024)
                return nativeScriptCode_Fail(error, line,
                    "Expected `motion_policy OWNED_MOTION common COMMON_MOTION` before code blocks.");
            for (unsigned i = 0; i < policies->count; ++i)
                if (policies->bindings[i].motion == motion)
                    return nativeScriptCode_Fail(error, line,
                        "Motion policy target is already declared.");
            policies->bindings[policies->count++] =
                (ScriptMotionPolicy){ (uint16_t) motion, (uint16_t) common };
            memset(source + start, ' ', end - start);
        } else {
            code_seen |= (length >= 5 && memcmp(text, "move ", 5) == 0) ||
                (length >= 5 && memcmp(text, "func ", 5) == 0) ||
                (length >= 12 && memcmp(text, "export func ", 12) == 0) ||
                (length >= 7 && memcmp(text, "action ", 7) == 0) ||
                (length >= 8 && memcmp(text, "article ", 8) == 0) ||
                (length >= 5 && memcmp(text, "item ", 5) == 0);
        }
        start = end < size ? end + 1 : end;
    }
    return true;
}

void nativeScriptMotionPolicy_EmitChecks(const ScriptMotionPolicies* policies,
                                        FILE* output)
{
    if (!policies->count) return;
    fputs(" || host->common_motion_states == 0", output);
    for (unsigned i = 0; i < policies->count; ++i)
        fprintf(output, " || %u < base->motion_state_first || "
            "%u - base->motion_state_first >= base->motion_state_count",
            policies->bindings[i].motion, policies->bindings[i].motion);
}

void nativeScriptMotionPolicy_EmitBindings(const ScriptMotionPolicies* policies,
                                           FILE* output, bool independent)
{
    static const char* fields[] = { "anim_cb", "input_cb", "phys_cb", "coll_cb" };
    for (unsigned i = 0; i < policies->count; ++i) {
        const ScriptMotionPolicy* policy = &policies->bindings[i];
        for (unsigned field = 0; field < 4; ++field) {
            fprintf(output, "    authored_states[%u - base->motion_state_first].%s = "
                "host->common_motion_states[%u].%s;\n", policy->motion,
                fields[field], policy->common_motion, fields[field]);
            if (independent)
                fprintf(output, "    if (host->common_motion_states[%u].%s != 0) "
                    "authored_motion_fields[%u - base->motion_state_first] |= %uu;\n",
                    policy->common_motion, fields[field], policy->motion, 1u << field);
        }
    }
}
