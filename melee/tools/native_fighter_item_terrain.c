#include "native_fighter_item_terrain.h"
#include "native_fighter_item_program.h"

#include <stdio.h>
#include <string.h>

const char* nativeItemTerrain_Constant(const char* name)
{
    if (strcmp(name, "Collision.floor_mask") == 0) return "Collide_FloorMask";
    if (strcmp(name, "Collision.wall_mask") == 0) return "Collide_WallMask";
    if (strcmp(name, "Collision.left_wall_mask") == 0) return "Collide_LeftWallMask";
    if (strcmp(name, "Collision.right_wall_mask") == 0) return "Collide_RightWallMask";
    return NULL;
}

int nativeItemTerrain_Expression(const char* source,
    const ArticleDeclaration* article, const NativeFighterParameters* parameters,
    char* output, size_t capacity, const char** after, ScriptExpressionType* type)
{
    static const struct {
        const char* name;
        const char* call;
        ScriptExpressionType type;
    } contacts[] = {
        { "item.wall_contact_flags", "engine->item_wall_contact_flags(gobj)", ScriptType_Int },
        { "item.ceiling_contact_flags", "engine->item_ceiling_contact_flags(gobj)", ScriptType_Int },
        { "item.reflect_from_wall", "engine->item_reflect_from_wall(gobj)", ScriptType_Bool },
    };
    for (unsigned i = 0; i < sizeof(contacts) / sizeof(contacts[0]); ++i) {
        size_t length = strlen(contacts[i].name);
        if (strncmp(source, contacts[i].name, length) != 0) continue;
        if (article == NULL || strncmp(source + length, "()", 2) != 0) return -1;
        *after = source + length + 2;
        *type = contacts[i].type;
        int written = snprintf(output, capacity, "%s", contacts[i].call);
        return written >= 0 && (size_t)written < capacity ? 1 : -1;
    }
    bool segment = strncmp(source, "stage.segment_blocked", 21) == 0 && source[21] == '(';
    bool audio = strncmp(source, "audio.play_global", 17) == 0 && source[17] == '(';
    if (segment || audio) {
        if (article == NULL) return -1;
        unsigned count = segment ? 4 : 3;
        char args[4][192], values[4][256];
        if (!nativeScriptCode_ParseArguments(source + (segment ? 21 : 17), args, count, after)) return -1;
        for (unsigned i = 0; i < count; ++i) {
            ScriptExpressionType argument = nativeFighterItemScript_ExpressionType(args[i], article, parameters);
            if (audio ? argument != ScriptType_Int : !nativeScriptTypes_Assignable(ScriptType_Float, argument)) return -1;
            if (!nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return -1;
        }
        *type = segment ? ScriptType_Bool : ScriptType_Int;
        int written = segment ? snprintf(output, capacity,
            "engine->stage_segment_intersection(NULL, NULL, NULL, NULL, -1, -1, %s, %s, %s, %s)",
            values[0], values[1], values[2], values[3]) : snprintf(output, capacity,
            "engine->play_global_sound((enum_t)(%s), (int)(%s), (int)(%s))", values[0], values[1], values[2]);
        return written >= 0 && (size_t)written < capacity ? 1 : -1;
    }
    static const char name[] = "stage.read_line_island_bounds";
    if (strncmp(source, name, sizeof(name) - 1) != 0) return 0;
    if (article == NULL || source[sizeof(name) - 1] != '(') return -1;
    *type = ScriptType_Bool;
    char arguments[7][192], line[512];
    const ScriptStateField* fields[6];
    if (!nativeScriptCode_ParseArguments(source + sizeof(name) - 1,
            arguments, 7, after) ||
        nativeFighterItemScript_ExpressionType(arguments[0], article,
            parameters) != ScriptType_Int ||
        !nativeFighterItemScript_Expression(arguments[0], line, sizeof(line),
            article, parameters)) return -1;
    for (unsigned i = 0; i < 6; ++i) {
        if (strncmp(arguments[i + 1], "item.state.", 11) != 0) return -1;
        fields[i] = nativeFighterScript_StateField(&article->local_state,
                                                  arguments[i + 1] + 11);
        if (fields[i] == NULL || fields[i]->array ||
            fields[i]->type != ScriptState_Float) return -1;
        for (unsigned j = 0; j < i; ++j)
            if (fields[i] == fields[j] || fields[i]->offset == fields[j]->offset)
                return -1;
    }
    // Capture the input before any output writes, including when it reads one
    // of those fields through an explicit integer conversion. No pointer from
    // the engine's island list escapes this expression.
    int written = snprintf(output, capacity,
        "({ int32_t terrain_line = (int32_t)(%s); Vec3 terrain_first = {0}, terrain_last = {0}; "
        "bool terrain_found = engine->stage_line_island_bounds(terrain_line, &terrain_first, &terrain_last); "
        "scripted_article_set_state_float(item, %u, terrain_first.x); "
        "scripted_article_set_state_float(item, %u, terrain_first.y); "
        "scripted_article_set_state_float(item, %u, terrain_first.z); "
        "scripted_article_set_state_float(item, %u, terrain_last.x); "
        "scripted_article_set_state_float(item, %u, terrain_last.y); "
        "scripted_article_set_state_float(item, %u, terrain_last.z); terrain_found; })",
        line, fields[0]->offset, fields[1]->offset, fields[2]->offset,
        fields[3]->offset, fields[4]->offset, fields[5]->offset);
    return written >= 0 && (size_t)written < capacity ? 1 : -1;
}
