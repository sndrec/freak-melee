#include "native_fighter_script_relations.h"
#include <string.h>

static bool fighter_block(const ScriptCodeBlock* block)
{
    return block != NULL && !block->item_event && !block->model_effect;
}

const char* nativeScriptRelations_Property(const char* name)
{
    static const struct { const char* name; const char* code; } fields[] = {
        { "hitlag.linked_id", "((int32_t)engine->linked_hitlag_id(gobj))" },
        { "hitlag.active", "fighter->x2219_b5" },
        { "fighter.incapacitated", "fighter->x221F_b3" },
        { "spawn.horizontal_offset", "fighter->x40" },
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(*fields); ++i)
        if (!strcmp(name, fields[i].name)) return fields[i].code;
    return NULL;
}

bool nativeScriptRelations_Expression(const ScriptCodeBlock* block, const char* name,
    const char** source, char* output, size_t capacity)
{
    if (!fighter_block(block)) return false;
    if (!strcmp(name, "targets.read_bone_position")) {
        char args[5][192], values[2][256], stores[3][160]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 5, &after)) return false;
        for (unsigned i = 0; i < 2; ++i)
            if (nativeScriptCode_ExpressionType(block, args[i]) != ScriptType_Int ||
                !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return false;
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < i; ++j) if (!strcmp(args[i+2], args[j+2])) return false;
            int written;
            if (!strncmp(args[i+2], "state.", 6)) {
                const ScriptStateField* field = nativeFighterScript_StateField(block->state, args[i+2]+6);
                if (!field || field->array || field->type != ScriptState_Float) return false;
                written = snprintf(stores[i], sizeof(stores[i]),
                    "script_state_set_float(fighter, script_state_base + %u, point.%c);", field->offset, 'x'+i);
            } else {
                bool found = false;
                for (unsigned j = 0; j < block->local_count; ++j)
                    if (!strcmp(block->locals[j], args[i+2]) && block->local_types[j] == ScriptType_Float) found = true;
                if (!found) return false;
                written = snprintf(stores[i], sizeof(stores[i]), "local_%s = point.%c;", args[i+2], 'x'+i);
            }
            if (written < 0 || (size_t)written >= sizeof(stores[i])) return false;
        }
        // Publish all three outputs only after both target and caller survive.
        // The source guard also protects evaluation nested in a larger expression.
        int written = snprintf(output, capacity,
            "({ uint32_t source_generation = (uint32_t)fighter->x8_spawnNum; "
            "uint32_t target = (uint32_t)(%s); int32_t part = (%s); Vec3 point; "
            "bool found = part >= 0 && engine->target_bone_position(target, (unsigned)part, &point); "
            "if (found && engine->callback_source_alive(gobj, source_generation, NativeCallbackSource_Fighter) "
            "&& gobj->user_data == fighter) { %s %s %s } found; })",
            values[0], values[1], stores[0], stores[1], stores[2]);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after; return true;
    }
    static const struct { const char* name; const char* host; bool boolean; } methods[] = {
        { "family.member_id", "family_member_id", false },
        { "hitlag.link", "link_hitlag", true },
        { "collision.copy_from", "copy_collision_from", true },
    };
    for (unsigned i = 0; i < sizeof(methods) / sizeof(*methods); ++i) {
        if (strcmp(name, methods[i].name)) continue;
        char args[1][192], value[256]; const char* after;
        if (!nativeScriptCode_ParseArguments(*source, args, 1, &after) ||
            nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[0], value, sizeof(value))) return false;
        int written = snprintf(output, capacity, "%sengine->%s(gobj, (uint32_t)(%s))",
            methods[i].boolean ? "" : "(int32_t)", methods[i].host, value);
        if (written < 0 || (size_t)written >= capacity) return false;
        *source = after; return true;
    }
    return false;
}

bool nativeScriptRelations_Action(const ScriptCodeBlock* block, const char* source,
    char* output, size_t capacity)
{
    if (!fighter_block(block)) return false;
    static const char* expressions[] = { "family.member_id", "hitlag.link", "collision.copy_from", "targets.read_bone_position" };
    for (unsigned i = 0; i < sizeof(expressions) / sizeof(*expressions); ++i) {
        size_t n = strlen(expressions[i]);
        if (strncmp(source, expressions[i], n) || source[n] != '(') continue;
        char value[2048]; const char* after = source + n;
        if (!nativeScriptRelations_Expression(block, expressions[i], &after, value, sizeof(value)) || *after) return false;
        int written = snprintf(output, capacity, "%s;", value);
        return written >= 0 && (size_t)written < capacity;
    }
    if (!strcmp(source, "hitlag.unlink()"))
        return snprintf(output, capacity, "engine->link_hitlag(gobj, 0);") < capacity;
    if (!strcmp(source, "hitlag.request_release()"))
        return snprintf(output, capacity, "engine->request_hitlag_release(gobj);") < capacity;
    static const struct { const char* name; const char* field; ScriptExpressionType type; } fields[] = {
        { "airborne", "ground_or_air", ScriptType_Bool },
        { "spawn.horizontal_offset", "x40", ScriptType_Float },
    };
    for (unsigned i = 0; i < sizeof(fields) / sizeof(*fields); ++i) {
        size_t n = strlen(fields[i].name);
        if (strncmp(source, fields[i].name, n) || strncmp(source+n, " = ", 3)) continue;
        char value[512];
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, source+n+3);
        if ((fields[i].type == ScriptType_Bool && type != ScriptType_Bool) ||
            (fields[i].type == ScriptType_Float && type != ScriptType_Float && type != ScriptType_Int) ||
            !nativeScriptCode_Expression(block, source+n+3, value, sizeof(value))) return false;
        // Publish the classification alone. Full ground/air transition helpers
        // intentionally remain separate: paired entry already did that work.
        int written = snprintf(output, capacity, i == 0 ?
            "fighter->ground_or_air = (%s) ? GA_Air : GA_Ground;" : "fighter->x40 = (%s);", value);
        return written >= 0 && (size_t)written < capacity;
    }
    return false;
}
