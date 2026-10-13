#include "native_fighter_script_queries.h"
#include "native_fighter_item_program.h"
#include <stdio.h>
#include <string.h>

static const char nearest[] = "cpu.nearest_enemy_at";
static const char translation[] = "articles.read_primary_translation";
static const char position[] = "targets.read_position";
static const char delta[] = "targets.read_position_delta";
static const char history[] = "cpu.read_history_pose";
static const char floor_read[] = "stage.read_floor";
static const char island_read[] = "stage.read_line_island_bounds";

static bool name_is(const char* source, const char* name)
{
    size_t n = strlen(name);
    return !strncmp(source, name, n) && (source[n] == '(' || !source[n]);
}

static bool store(const ScriptStateTable* state, const char* prefix,
    const char locals[][32], const ScriptExpressionType* types, unsigned count,
    const char* target, unsigned component, bool item, char* output, size_t capacity)
{
    int n;
    size_t prefix_size = strlen(prefix);
    if (!strncmp(target, prefix, prefix_size)) {
        const ScriptStateField* field = nativeFighterScript_StateField(state, target + prefix_size);
        if (!field || field->array || field->type != ScriptState_Float) return false;
        n = snprintf(output, capacity, item ?
            "scripted_article_%s_state_float(item, %u, query_point.%c);" :
            "script_state_%s_float(fighter, script_state_base + %u, query_point.%c);",
            "set", field->offset, 'x' + component);
    } else {
        unsigned i;
        for (i = 0; i < count; ++i)
            if (!strcmp(locals[i], target) && types[i] == ScriptType_Float) break;
        if (i == count) return false;
        n = snprintf(output, capacity, "local_%s = query_point.%c;", target, 'x' + component);
    }
    return n >= 0 && (size_t)n < capacity;
}

static int emit_point_query(const char* handle, const char* kind, const char* host,
    char stores[3][160], char* output, size_t capacity)
{
    // Sequential declarations preserve written evaluation order. The shared
    // lifetime emitter guards this exact Host call before publishing any output.
    char call[160];
    int call_size = kind ? snprintf(call, sizeof(call),
        "engine->article_ref_primary_translation(gobj, query_handle, %s, &query_point)", kind) :
        snprintf(call, sizeof(call), "engine->%s(query_handle, &query_point)", host);
    if (call_size < 0 || (size_t)call_size >= sizeof(call)) return -1;
    int n = snprintf(output, capacity,
        "({ uint32_t query_handle = (uint32_t)(%s); Vec3 query_point; "
        "bool query_found = %s; if (query_found) { %s %s %s } query_found; })",
        handle, call, stores[0], stores[1], stores[2]);
    return n >= 0 && (size_t)n < capacity ? 1 : -1;
}


static bool scalar_store(const ScriptCodeBlock* block, const char* target,
    ScriptExpressionType type, const char* value, char* output, size_t capacity)
{
    int written;
    if (!strncmp(target, "state.", 6)) {
        const ScriptStateField* field = nativeFighterScript_StateField(block->state, target + 6);
        if (!field || field->array || nativeScriptTypes_State(field->type) != type) return false;
        written = snprintf(output, capacity, "script_state_set_%s(fighter, script_state_base + %u, %s);",
            type == ScriptType_Int ? "int" : "float", field->offset, value);
    } else {
        unsigned index;
        for (index = 0; index < block->local_count; ++index)
            if (!strcmp(block->locals[index], target) && block->local_types[index] == type) break;
        if (index == block->local_count) return false;
        written = snprintf(output, capacity, "local_%s = %s;", target, value);
    }
    return written >= 0 && (size_t)written < capacity;
}

static int nativeScriptQueries_FighterGeometry(const ScriptCodeBlock* block,
    const char* name, const char** source, char* output, size_t capacity)
{
    if (!block || block->item_event || block->model_effect) return -1;
    bool is_history = !strcmp(name, history);
    bool is_floor = !strcmp(name, floor_read);
    unsigned inputs = is_history ? 0 : is_floor ? 4 : 1;
    unsigned outputs = is_floor ? 3 : 4;
    char args[7][192], values[4][512], stores[4][192]; const char* after;
    if (!nativeScriptCode_ParseArguments(*source, args, inputs + outputs, &after)) return -1;
    for (unsigned i = 0; i < inputs; ++i) {
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
        if ((is_floor ? type != ScriptType_Int && type != ScriptType_Float : type != ScriptType_Int) ||
            !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return -1;
    }
    const char* history_values[] = { "query_point.x", "query_point.y", "query_point.z", "query_facing" };
    const char* floor_values[] = { "query_point.x", "query_point.y", "query_line" };
    const char* island_values[] = { "query_left.x", "query_left.y", "query_right.x", "query_right.y" };
    for (unsigned i = 0; i < outputs; ++i) {
        for (unsigned j = 0; j < i; ++j)
            if (!strcmp(args[inputs + i], args[inputs + j])) return -1;
        if (!scalar_store(block, args[inputs + i], is_floor && i == 2 ? ScriptType_Int : ScriptType_Float,
            is_history ? history_values[i] : is_floor ? floor_values[i] : island_values[i],
            stores[i], sizeof(stores[i]))) return -1;
    }
    int written;
    if (is_history) written = snprintf(output, capacity,
        "({ Vec3 query_point; float query_facing; bool query_found = "
        "engine->cpu_read_history_pose(gobj, &query_point, &query_facing); "
        "if (query_found) { %s %s %s %s } query_found; })", stores[0], stores[1], stores[2], stores[3]);
    else if (is_floor) written = snprintf(output, capacity,
        "({ float query_x0 = (%s); float query_y0 = (%s); float query_x1 = (%s); float query_y1 = (%s); "
        "Vec3 query_point, query_normal; int query_line; u32 query_flags; "
        "bool query_found = isfinite(query_x0) && isfinite(query_y0) && isfinite(query_x1) && isfinite(query_y1) && "
        "engine->stage_read_floor(query_x0, query_y0, query_x1, query_y1, 0.0f, "
        "&query_point, &query_line, &query_flags, &query_normal, -1, -1, -1, NULL, NULL); "
        "if (query_found) { %s %s %s } query_found; })",
        values[0], values[1], values[2], values[3], stores[0], stores[1], stores[2]);
    else written = snprintf(output, capacity,
        "({ int32_t query_line = (%s); Vec3 query_left, query_right; bool query_found = "
        "engine->stage_line_island_bounds(query_line, &query_left, &query_right); "
        "if (query_found) { %s %s %s %s } query_found; })", values[0], stores[0], stores[1], stores[2], stores[3]);
    if (written < 0 || (size_t)written >= capacity) return -1;
    *source = after; return 1;
}

int nativeScriptQueries_Fighter(const ScriptCodeBlock* block, const char* name,
    const char** source, char* output, size_t capacity)
{
    bool is_nearest = !strcmp(name, nearest);
    bool is_delta = !strcmp(name, delta);
    bool is_position = !strcmp(name, position) || is_delta;
    if (!strcmp(name, history) || !strcmp(name, floor_read) || !strcmp(name, island_read))
        return nativeScriptQueries_FighterGeometry(block, name, source, output, capacity);
    if (!is_nearest && !is_position && strcmp(name, translation)) return 0;
    if (!block || block->item_event || block->model_effect) return -1;
    char args[5][192], values[3][192]; const char* after;
    if (!nativeScriptCode_ParseArguments(*source, args, is_nearest ? 3 : is_position ? 4 : 5, &after)) return -1;
    if (is_nearest) {
        for (unsigned i = 0; i < 3; ++i) {
            ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
            if ((type != ScriptType_Float && type != ScriptType_Int) ||
                !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return -1;
        }
        int n = snprintf(output, capacity,
            "({ float query_x = (%s); float query_y = (%s); float query_z = (%s); "
            "Vec3 query_origin = { query_x, query_y, query_z }; uint32_t query_target = 0; "
            "bool query_found = engine->cpu_nearest_enemy_at(gobj, &query_origin, &query_target); "
            "(int32_t)(query_found ? query_target : 0); })", values[0], values[1], values[2]);
        if (n < 0 || (size_t)n >= capacity) return -1;
    } else {
        if (nativeScriptCode_ExpressionType(block, args[0]) != ScriptType_Int ||
            !nativeScriptCode_Expression(block, args[0], values[0], sizeof(values[0])) ||
            (!is_position && !nativeScriptArticleRefs_Declaration(block->articles, args[1]))) return -1;
        char stores[3][160];
        unsigned first_output = is_position ? 1 : 2;
        for (unsigned i = 0; i < 3; ++i) {
            for (unsigned j = 0; j < i; ++j) if (!strcmp(args[i+first_output], args[j+first_output])) return -1;
            if (!store(block->state, "state.", block->locals, block->local_types,
                block->local_count, args[i+first_output], i, false, stores[i], sizeof(stores[i]))) return -1;
        }
        if (emit_point_query(values[0], is_position ? NULL : args[1], is_delta ? "target_position_delta" : "target_position", stores, output, capacity) < 0) return -1;
    }
    *source = after; return 1;
}

int nativeScriptQueries_Article(const ArticleDeclaration* article,
    const NativeFighterParameters* parameters, const char* text,
    const char** after, char* output, size_t capacity)
{
    // Article terrain owns this name's six-output form. Its parser validates
    // item-state destinations; the fighter geometry form has four outputs.
    if (name_is(text, nearest) || name_is(text, history) || name_is(text, floor_read)) return -1;
    bool is_delta = name_is(text, delta);
    bool is_position = name_is(text, position) || is_delta;
    if (!is_position && !name_is(text, translation)) return 0;
    if (!article || !article->expression_callback) return -1;
    char args[5][192], handle[192], stores[3][160];
    const char* end;
    if (!nativeScriptCode_ParseArguments(text + strlen(is_delta ? delta : is_position ? position : translation), args, is_position ? 4 : 5, &end) ||
        nativeFighterItemScript_ExpressionType(args[0], article, parameters) != ScriptType_Int ||
        !nativeFighterItemScript_Expression(args[0], handle, sizeof(handle), article, parameters) ||
        (!is_position && !nativeScriptArticleRefs_Declaration(article->catalog, args[1]))) return -1;
    const ItemCallback* callback = article->expression_callback;
    unsigned first_output = is_position ? 1 : 2;
    for (unsigned i = 0; i < 3; ++i) {
        for (unsigned j = 0; j < i; ++j) if (!strcmp(args[i+first_output], args[j+first_output])) return -1;
        if (!store(&article->local_state, "item.state.", callback->locals, callback->local_types,
            callback->local_count, args[i+first_output], i, true, stores[i], sizeof(stores[i]))) return -1;
    }
    if (emit_point_query(handle, is_position ? NULL : args[1], is_delta ? "target_position_delta" : "target_position", stores, output, capacity) < 0) return -1;
    *after = end; return 1;
}

int nativeScriptQueries_Action(const ScriptCodeBlock* block, const char* source,
    char* output, size_t capacity)
{
    const char* name = name_is(source, nearest) ? nearest :
        name_is(source, translation) ? translation : name_is(source, position) ? position :
        name_is(source, delta) ? delta : name_is(source, history) ? history :
        name_is(source, floor_read) ? floor_read : name_is(source, island_read) ? island_read : NULL;
    if (!name) return 0;
    const char* after = source + strlen(name); char expression[4096];
    if (nativeScriptQueries_Fighter(block, name, &after, expression, sizeof(expression)) != 1 || *after) return -1;
    int n = snprintf(output, capacity, "%s;", expression);
    return n >= 0 && (size_t)n < capacity ? 1 : -1;
}
