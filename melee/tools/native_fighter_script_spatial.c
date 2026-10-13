#include "native_fighter_script_spatial.h"
#include <stdio.h>
#include <string.h>

static bool number(ScriptExpressionType type)
{ return type == ScriptType_Int || type == ScriptType_Float; }

static int emit_wind(const char values[7][384], char* output, size_t capacity)
{
    int n = snprintf(output, capacity,
        "{ Vec3 wind_position; wind_position.x=(%s); wind_position.y=(%s); "
        "wind_position.z=(%s); int32_t wind_frames=(%s); "
        "float wind_strength=(%s); float wind_decay=(%s); float wind_frequency=(%s); "
        "engine->spawn_wind(&wind_position, wind_frames, wind_strength, wind_decay, wind_frequency); }",
        values[0], values[1], values[2], values[3], values[4], values[5], values[6]);
    return n >= 0 && (size_t)n < capacity ? 1 : -1;
}

int nativeScriptSpatial_FighterAction(const ScriptCodeBlock* block,
    const char* source, char* output, size_t capacity)
{
    static const char prefix[] = "effects.wind_at_position(";
    if (strncmp(source, prefix, sizeof(prefix)-1)) return 0;
    if (!block || block->item_event || block->model_effect) return -1;
    char args[7][192], values[7][384]; const char* after;
    if (!nativeScriptCode_ParseArguments(source+sizeof(prefix)-2, args, 7, &after) || *after) return -1;
    for (unsigned i=0; i<7; ++i) {
        ScriptExpressionType type = nativeScriptCode_ExpressionType(block, args[i]);
        if ((i==3 ? type!=ScriptType_Int : !number(type)) ||
            !nativeScriptCode_Expression(block, args[i], values[i], sizeof(values[i]))) return -1;
    }
    return emit_wind(values, output, capacity);
}

static bool path_literal(const char* argument, char path[40])
{
    size_t n = strlen(argument);
    if (n<2 || n>41 || argument[0]!='"' || argument[n-1]!='"') return false;
    memcpy(path, argument+1, n-2); path[n-2]=0;
    const char* cursor=path; unsigned depth=0;
    while (*cursor) {
        if (++depth>12 || *cursor<'0' || *cursor>'9') return false;
        unsigned ordinal=0;
        do { ordinal=ordinal*10+(unsigned)(*cursor++-'0'); if (ordinal>255) return false; }
        while (*cursor>='0' && *cursor<='9');
        if (!*cursor) break;
        if (*cursor++!='/' || !*cursor) return false;
    }
    return true;
}

static bool destination(const ArticleDeclaration* article, const char* name,
    char axis, char* store, size_t capacity)
{
    if (!strncmp(name, "item.state.", 11)) {
        const ScriptStateField* field = nativeFighterScript_StateField(&article->local_state, name+11);
        if (!field || field->array || field->type!=ScriptState_Float) return false;
        int n=snprintf(store, capacity, "scripted_article_set_state_float(item, %u, joint_position.%c);", field->offset, axis);
        return n>=0 && (size_t)n<capacity;
    }
    const ItemCallback* callback=article->expression_callback;
    if (!callback) return false;
    for (unsigned i=0; i<callback->local_count; ++i) {
        if (strcmp(name, callback->locals[i]) || callback->local_types[i]!=ScriptType_Float) continue;
        int n=snprintf(store, capacity, "local_%s=joint_position.%c;", name, axis);
        return n>=0 && (size_t)n<capacity;
    }
    return false;
}

int nativeScriptSpatial_ItemAction(const ArticleDeclaration* article,
    const NativeFighterParameters* parameters, const char* source, char* output, size_t capacity)
{
    bool wind=!strncmp(source, "effects.wind_at_position(", 25);
    bool position=!strncmp(source, "item.joint_world_position(", 26);
    if (!wind && !position) return 0;
    if (!article) return -1;
    char args[7][192], values[7][384]; const char* after;
    unsigned count=wind?7:5;
    if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, count, &after) || *after) return -1;
    if (wind) {
        for (unsigned i=0; i<7; ++i) {
            ScriptExpressionType type=nativeFighterItemScript_ExpressionType(args[i], article, parameters);
            if ((i==3 ? type!=ScriptType_Int : !number(type)) ||
                !nativeFighterItemScript_Expression(args[i], values[i], sizeof(values[i]), article, parameters)) return -1;
        }
        return emit_wind(values, output, capacity);
    }
    char path[40], stores[3][192];
    if (!path_literal(args[1], path) ||
        nativeFighterItemScript_ExpressionType(args[0], article, parameters)!=ScriptType_Int ||
        !nativeFighterItemScript_Expression(args[0], values[0], sizeof(values[0]), article, parameters)) return -1;
    for (unsigned i=0; i<3; ++i) {
        for (unsigned j=0; j<i; ++j) if (!strcmp(args[i+2], args[j+2])) return -1;
        if (!destination(article, args[i+2], (char)('x'+i), stores[i], sizeof(stores[i]))) return -1;
    }
    int n=snprintf(output, capacity,
        "{ int32_t joint_model=(%s); Vec3 joint_position={0}; "
        "engine->item_joint_world_position(gobj, (unsigned)joint_model, \"%s\", &joint_position); %s %s %s }",
        values[0], path, stores[0], stores[1], stores[2]);
    return n>=0 && (size_t)n<capacity ? 1 : -1;
}
