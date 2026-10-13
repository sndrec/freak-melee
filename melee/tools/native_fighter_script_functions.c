#include "native_fighter_script_functions.h"
#include "native_fighter_script_signal.h"
#include "native_fighter_script_model_effect.h"
#include "native_fighter_script_lifetime.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char* identifier(const char** cursor, size_t* length)
{
    const char* at = *cursor;
    while (*at && *at != '#') {
        if (*at == '"' || *at == '\'') {
            char quote = *at++;
            while (*at && *at != quote) {
                if (*at == '\\' && at[1]) ++at;
                ++at;
            }
            if (*at) ++at;
        } else if (isalpha((unsigned char)*at) || *at == '_') {
            const char* start = at++;
            while (isalnum((unsigned char)*at) || *at == '_' || *at == '.') ++at;
            *length = (size_t)(at - start); *cursor = at; return start;
        } else ++at;
    }
    *cursor = at; return NULL;
}

ScriptFunctionSet nativeScriptFunctions_References(const ScriptFunctionProgram* program,
                                         const char* source)
{
    ScriptFunctionSet references = { 0 };
    const char* token; size_t length;
    while ((token = identifier(&source, &length)) != NULL) {
        if (*source != '(') continue;
        for (unsigned i = 0; i < program->count; ++i)
            if (strlen(program->functions[i].name) == length &&
                !memcmp(token, program->functions[i].name, length))
                nativeScriptDependencies_Set(&references, i);
    }
    return references;
}

bool nativeScriptFunctions_ModelEffectContext(const char* source)
{
    const char* token; size_t length;
    while ((token = identifier(&source, &length)) != NULL)
        if ((length >= 7 && !memcmp(token, "effect.", 7)) ||
            (length >= 8 && !memcmp(token, "creator.", 8))) return true;
    return false;
}

static bool name_allowed(const char* name)
{
    static const char* reserved[] = {
        "func", "export", "move", "action", "article", "model_effect", "state", "attribute",
        "if", "elif", "else", "while", "return", "pass", "let", "true", "false",
        "abs", "sin", "cos", "atan2", "radians", "signbit", "random",
        "grounded", "airborne", "frame", "pi_half",
    };
    if (!nativeScriptCode_Identifier(name, strlen(name), 32)) return false;
    for (unsigned i = 0; i < sizeof(reserved) / sizeof(reserved[0]); ++i)
        if (strcmp(name, reserved[i]) == 0) return false;
    return true;
}

static bool declaration(ScriptFunction* function, char* text)
{
    char* open = strchr(text, '(');
    char* close = strrchr(text, ')');
    if (open == NULL || close == NULL || close < open) return false;
    if (strcmp(close + 1, ":") != 0) {
        const char* suffix = close + 1;
        function->return_type = strcmp(suffix, " -> bool:") == 0 ? ScriptType_Bool :
            strcmp(suffix, " -> int:") == 0 ? ScriptType_Int :
            strcmp(suffix, " -> float:") == 0 ? ScriptType_Float : ScriptType_Invalid;
        if (function->return_type == ScriptType_Invalid) return false;
    }
    *open = '\0';
    *close = '\0';
    const char* name = text + (function->exported ? 12 : 5);
    if (!name_allowed(name)) return false;
    strcpy(function->name, name);
    char* next = open + 1;
    if (*next == '\0') return true;
    if (function->exported) return false;
    for (;;) {
        char* comma = strchr(next, ',');
        if (comma != NULL) *comma = '\0';
        while (*next == ' ') ++next;
        size_t size = strlen(next);
        while (size != 0 && next[size - 1] == ' ') next[--size] = '\0';
        char* separator = strchr(next, ' ');
        if (separator == NULL || function->argument_count == SCRIPT_FUNCTION_ARGS)
            return false;
        *separator = '\0';
        const char* type = strcmp(next, "float") == 0 ? "float" :
            strcmp(next, "int") == 0 ? "int32_t" :
            strcmp(next, "bool") == 0 ? "bool" : NULL;
        const char* argument = separator + 1;
        if (type == NULL || !name_allowed(argument)) return false;
        for (unsigned i = 0; i < function->argument_count; ++i)
            if (strcmp(argument, function->argument_names[i]) == 0) return false;
        unsigned index = function->argument_count++;
        strcpy(function->argument_names[index], argument);
        function->argument_types[index] = type;
        if (comma == NULL) return true;
        next = comma + 1;
    }
}

ScriptFunctionProgram* nativeScriptFunctions_Parse(char* source, size_t size,
                                                   NativeFighterScriptError* error)
{
    ScriptFunctionProgram* program = calloc(1, sizeof(*program));
    if (program == NULL) {
        nativeScriptCode_Fail(error, 0, "Out of memory for script functions.");
        return NULL;
    }
    ScriptFunction* active = NULL;
    size_t offset = 0;
    unsigned line_number = 0;
    while (offset < size) {
        size_t end = offset;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - offset;
        ++line_number;
        if (length != 0 && source[offset + length - 1] == '\r') --length;
        if (length >= 512 || memchr(source + offset, '\0', length) != NULL) {
            nativeScriptCode_Fail(error, line_number, "Line is too long or contains NUL.");
            goto invalid;
        }
        char text[512];
        memcpy(text, source + offset, length);
        text[length] = '\0';
        while (length != 0 && (text[length - 1] == ' ' || text[length - 1] == '\t'))
            text[--length] = '\0';
        const char* nonspace = text;
        while (*nonspace == ' ') ++nonspace;
        const bool ignored = *nonspace == '\0' || *nonspace == '#';
        if (ignored && active != NULL) memset(source + offset, ' ', end - offset);
        if (!ignored && text[0] != ' ') {
            if (active != NULL && active->body_count == 0) {
                nativeScriptCode_Fail(error, active->line, "Function needs a body.");
                goto invalid;
            }
            active = NULL;
            if (strncmp(text, "func ", 5) == 0 ||
                strncmp(text, "export func ", 12) == 0) {
                if (program->count < SCRIPT_FUNCTIONS)
                    program->functions[program->count].exported =
                        strncmp(text, "export func ", 12) == 0;
                if (program->count == SCRIPT_FUNCTIONS ||
                    !declaration(&program->functions[program->count], text)) {
                    nativeScriptCode_Fail(error, line_number,
                        "Expected func Name(typed arguments): or export func Name(): (up to 128 functions; exports take no arguments).");
                    goto invalid;
                }
                active = &program->functions[program->count++];
                active->line = line_number;
                for (unsigned i = 0; i + 1 < program->count; ++i)
                    if (strcmp(program->functions[i].name, active->name) == 0) {
                        nativeScriptCode_Fail(error, line_number, "Duplicate function name.");
                        goto invalid;
                    }
                if (active->exported) {
                    active->export_hash = nativeScriptSignal_Hash(active->name);
                    if (active->export_hash == 0) {
                        nativeScriptCode_Fail(error, line_number,
                            "Export name hashes to reserved zero.");
                        goto invalid;
                    }
                    unsigned order = 0;
                    while (order < program->export_count &&
                           program->functions[program->export_order[order]].export_hash <
                               active->export_hash) ++order;
                    if (order < program->export_count &&
                        program->functions[program->export_order[order]].export_hash ==
                            active->export_hash) {
                        nativeScriptCode_Fail(error, line_number,
                            "Export names have colliding hashes.");
                        goto invalid;
                    }
                    memmove(program->export_order + order + 1,
                            program->export_order + order,
                            program->export_count - order);
                    program->export_order[order] = (uint8_t) (program->count - 1);
                    ++program->export_count;
                }
                memset(source + offset, ' ', end - offset);
            }
        } else if (!ignored && active != NULL) {
            if (strncmp(text, "    ", 4) != 0 ||
                active->body_count == SCRIPT_CODE_LINES || strlen(text) + 4 >= sizeof(active->body[0])) {
                nativeScriptCode_Fail(error, line_number, "Invalid or excess function body.");
                goto invalid;
            }
            const unsigned index = active->body_count++;
            strcpy(active->body[index], "    ");
            strcat(active->body[index], text);
            active->body_lines[index] = line_number;
            memset(source + offset, ' ', end - offset);
        }
        offset = end < size ? end + 1 : end;
    }
    if (active != NULL && active->body_count == 0) {
        nativeScriptCode_Fail(error, active->line, "Function needs a body.");
        goto invalid;
    }
    // Context requirements propagate through real helper calls before either
    // fighter or model-effect compilation. A helper cannot hide a context-only
    // property behind another helper name.
    for (unsigned i = 0; i < program->count; ++i)
        for (unsigned j = 0; j < program->functions[i].body_count; ++j)
            program->functions[i].model_effect_only |=
                nativeScriptFunctions_ModelEffectContext(program->functions[i].body[j]);
    bool changed;
    do {
        changed = false;
        for (unsigned i = 0; i < program->count; ++i) if (!program->functions[i].model_effect_only)
            for (unsigned j = 0; j < program->count; ++j) if (program->functions[j].model_effect_only) {
                for (unsigned k = 0; k < program->functions[i].body_count; ++k)
                    if (nativeScriptDependencies_Test(nativeScriptFunctions_References(program, program->functions[i].body[k]), j)) {
                        program->functions[i].model_effect_only = true; changed = true; break;
                    }
            }
    } while (changed);
    return program;
invalid:
    free(program);
    return NULL;
}

bool nativeScriptFunctions_Call(const ScriptCodeBlock* caller, const char* source,
                                 char* output, size_t capacity)
{
    if (caller->functions == NULL) return false;
    const char* open = strchr(source, '(');
    const size_t size = strlen(source);
    if (open == NULL || size == 0 || source[size - 1] != ')') return false;
    unsigned index;
    for (index = 0; index < caller->functions->count; ++index) {
        const char* name = caller->functions->functions[index].name;
        if (strlen(name) == (size_t) (open - source) &&
            memcmp(name, source, open - source) == 0) break;
    }
    if (index == caller->functions->count) return false;
    const ScriptFunction* function = &caller->functions->functions[index];
    if (function->disabled || (!caller->model_effect && function->model_effect_only)) return false;
    char arguments[SCRIPT_FUNCTION_ARGS][1024];
    bool ordered = false;
    const char* start = open + 1;
    const char* close = source + size - 1;
    unsigned count = 0, depth = 0;
    for (const char* cursor = start; cursor <= close; ++cursor) {
        if (cursor != close && *cursor == '(') ++depth;
        else if (cursor != close && *cursor == ')') {
            if (depth == 0) return false;
            --depth;
        }
        if (cursor != close && (*cursor != ',' || depth != 0)) continue;
        const char* first = start;
        const char* last = cursor;
        while (first < last && *first == ' ') ++first;
        while (last > first && last[-1] == ' ') --last;
        if (first == last) {
            if (cursor != close || count != 0 || depth != 0) return false;
        } else {
            if (count == function->argument_count || last - first >= 192 || depth != 0)
                return false;
            char expression[192];
            memcpy(expression, first, last - first);
            expression[last - first] = '\0';
            if (!nativeScriptTypes_Assignable(
                    nativeScriptTypes_CType(function->argument_types[count]),
                    nativeScriptCode_ExpressionType(caller, expression))) return false;
            if (!nativeScriptCode_Expression(caller, expression, arguments[count],
                                             sizeof(arguments[count]))) return false;
            ordered |= nativeScriptDependencies_Any(nativeScriptFunctions_References(caller->functions, expression)) ||
                nativeScriptLifetime_DirectEffect(arguments[count]);
            ++count;
        }
        start = cursor + 1;
    }
    if (count != function->argument_count) return false;
    size_t used = 0;
    if (ordered) {
        used = snprintf(output, capacity, "({ ");
        if (used >= capacity) return false;
        for (unsigned i=0; i<count; ++i) {
            int written = snprintf(output+used, capacity-used,
                "__auto_type script_arg_%u = %s; ", i, arguments[i]);
            if (written < 0 || (size_t)written >= capacity-used) return false;
            used += written;
        }
    }
    int written = snprintf(output+used, capacity-used, caller->model_effect ?
        "script_effect_fn_%s(effect" : "script_fn_%s(gobj", function->name);
    if (written < 0 || (size_t)written >= capacity-used) return false;
    used += written;
    for (unsigned i=0; i<count; ++i) {
        written = ordered ? snprintf(output+used, capacity-used, ", script_arg_%u", i) :
            snprintf(output+used, capacity-used, ", %s", arguments[i]);
        if (written < 0 || (size_t)written >= capacity-used) return false;
        used += written;
    }
    if (snprintf(output+used, capacity-used, ordered ? "); });" : ");") >= capacity-used) return false;
    return true;
}

ScriptExpressionType nativeScriptFunctions_Result(const ScriptCodeBlock* caller,
                                                   const char* source)
{
    if (!caller->functions) return ScriptType_Invalid;
    const char* open = strchr(source, '(');
    if (!open) return ScriptType_Invalid;
    unsigned depth = 0;
    const char* end = open;
    do {
        if (!*end) return ScriptType_Invalid;
        if (*end == '(') ++depth;
        else if (*end == ')') --depth;
        ++end;
    } while (depth);
    if (*end) return ScriptType_Invalid;
    for (unsigned i = 0; i < caller->functions->count; ++i) {
        const ScriptFunction* fn = &caller->functions->functions[i];
        if (strlen(fn->name) == (size_t)(open-source) &&
            !memcmp(source, fn->name, open-source) && !fn->disabled &&
            (caller->model_effect || !fn->model_effect_only)) return fn->return_type;
    }
    return ScriptType_Invalid;
}

int nativeScriptFunctions_Expression(const ScriptCodeBlock* caller, const char* source,
                                     char* output, size_t capacity, const char** after)
{
    if (!caller->functions) return 0;
    const char* open = source;
    while (isalnum((unsigned char)*open) || *open == '_') ++open;
    if (*open != '(') return 0;
    const ScriptFunction* function = NULL;
    for (unsigned i = 0; i < caller->functions->count; ++i)
        if (strlen(caller->functions->functions[i].name) == (size_t)(open-source) &&
            !memcmp(source, caller->functions->functions[i].name, open-source)) {
            function = &caller->functions->functions[i]; break;
        }
    if (!function) return 0;
    if (function->return_type == ScriptType_Invalid) return -1;
    unsigned depth = 0;
    const char* end = open;
    do {
        if (!*end) return -1;
        if (*end == '(') ++depth;
        else if (*end == ')') --depth;
        ++end;
    } while (depth);
    const char* suffix = end;
    while (*suffix == ' ') ++suffix;
    if (*suffix == '.' || *suffix == '[' || *suffix == '(') return -1;
    char call[512];
    if ((size_t)(end-source) >= sizeof(call)) return -1;
    memcpy(call, source, end-source); call[end-source] = 0;
    if (!nativeScriptFunctions_Call(caller, call, output, capacity)) return -1;
    output[strlen(output)-1] = 0;
    *after = end;
    return 1;
}

static const struct {
    const char* name;
    const char* field;
    const char* comparison;
} callback_slots[] = {
    { "callbacks.animation", "anim_cb", NULL },
    { "callbacks.interrupt", "input_cb", NULL },
    { "callbacks.physics", "phys_cb", NULL },
    { "callbacks.collision", "coll_cb", NULL },
    { "callbacks.accessory", "accessory4_cb", NULL },
    { "callbacks.accessory_before", "accessory2_cb", "callbacks.accessory_before_is" },
    { "callbacks.accessory_hitlag", "accessory3_cb", "callbacks.accessory_hitlag_is" },
    { "callbacks.pre_hitlag", "pre_hitlag_cb", NULL },
    { "callbacks.post_hitlag", "post_hitlag_cb", NULL },
    { "callbacks.reflect", "x21F8", NULL },
    { "callbacks.damage", "take_dmg_cb", "callbacks.damage_is" },
    { "callbacks.damage_secondary", "take_dmg_2_cb", "callbacks.damage_secondary_is" },
    { "callbacks.death1", "death1_cb", "callbacks.death1_is" },
    { "callbacks.death2", "death2_cb", "callbacks.death2_is" },
    { "callbacks.death3", "death3_cb", NULL },
    { "callbacks.state_changed", "x21EC", NULL },
};

bool nativeScriptFunctions_CallbackPointer(const ScriptCodeBlock* caller, const char* name,
                                           char* pointer, size_t capacity)
{
    if (caller == NULL || caller->model_effect) return false;
    if (strcmp(name, "None") == 0)
        return snprintf(pointer, capacity, "NULL") < capacity;
    if (caller->functions == NULL) return false;
    for (unsigned i = 0; i < caller->functions->count; ++i) {
        const ScriptFunction* function = &caller->functions->functions[i];
        if (strcmp(function->name, name) != 0) continue;
        if (function->argument_count != 0 || function->return_type != ScriptType_Invalid || function->disabled ||
            function->model_effect_only) return false;
        return snprintf(pointer, capacity, "script_fn_%s", function->name) < capacity;
    }
    return false;
}

bool nativeScriptFunctions_BindCallback(ScriptCodeBlock* caller, const char* source,
                                         char* output, size_t capacity)
{
    static const struct { const char* name; const char* host; } calls[] = {
        { "collision.air_aerial_landing_callback", "air_aerial_landing_collision" },
        { "attack.update_rapid_jab", "rapid_jab_should_finish" },
    };
    for (unsigned i = 0; i < sizeof(calls) / sizeof(calls[0]); ++i) {
        size_t length = strlen(calls[i].name);
        if (strncmp(source, calls[i].name, length) != 0 || source[length] != '(') continue;
        char argument[1][192], pointer[64]; const char* after;
        if (caller == NULL || caller->model_effect || caller->item_event ||
            !nativeScriptCode_ParseArguments(source + length, argument, 1, &after) || *after ||
            strcmp(argument[0], "None") == 0 ||
            !nativeScriptFunctions_CallbackPointer(caller, argument[0], pointer, sizeof(pointer))) return false;
        if (strcmp(calls[i].name, "attack.update_rapid_jab") == 0)
            return snprintf(output, capacity, "if (engine->%s(gobj)) %s(gobj);", calls[i].host, pointer) < capacity;
        return snprintf(output, capacity, "engine->%s(gobj, %s);", calls[i].host, pointer) < capacity;
    }
    static const char landing[] = "collision.air_landing_callback";
    if (strncmp(source, landing, sizeof(landing) - 1) == 0 && source[sizeof(landing) - 1] == '(') {
        char argument[1][192], pointer[64]; const char* after;
        if (!nativeScriptCode_ParseArguments(source + sizeof(landing) - 1, argument, 1, &after) || *after ||
            strcmp(argument[0], "None") == 0 ||
            !nativeScriptFunctions_CallbackPointer(caller, argument[0], pointer, sizeof(pointer))) return false;
        return snprintf(output, capacity,
            "engine->air_special_landing_collision(gobj, engine->fall_special_ledge_check, %s);", pointer) < capacity;
    }
    for (unsigned slot = 0; slot < sizeof(callback_slots) / sizeof(callback_slots[0]); ++slot) {
        size_t prefix = strlen(callback_slots[slot].name);
        if (strncmp(source, callback_slots[slot].name, prefix) != 0 || source[prefix] != '(') continue;
        char argument[1][192], pointer[64];
        const char* after;
        if (!nativeScriptCode_ParseArguments(source + prefix, argument, 1, &after) || *after ||
            !nativeScriptFunctions_CallbackPointer(caller, argument[0], pointer, sizeof(pointer))) return false;
        // Assignment retains a direct pointer; it is not a synchronous call.
        return snprintf(output, capacity, "fighter->%s = %s;",
                        callback_slots[slot].field, pointer) < capacity;
    }
    return false;
}

bool nativeScriptFunctions_CallbackExpression(const ScriptCodeBlock* caller,
                                                const char* name,
                                                const char** source,
                                                char* output, size_t capacity)
{
    for (unsigned slot = 0; slot < sizeof(callback_slots) / sizeof(callback_slots[0]); ++slot) {
        if (callback_slots[slot].comparison == NULL ||
            strcmp(name, callback_slots[slot].comparison) != 0) continue;
        char argument[1][192], pointer[64];
        const char* after;
        if (source == NULL || *source == NULL ||
            !nativeScriptCode_ParseArguments(*source, argument, 1, &after) ||
            !nativeScriptFunctions_CallbackPointer(caller, argument[0], pointer, sizeof(pointer))) return false;
        int length = snprintf(output, capacity, "(fighter->%s == %s)",
                              callback_slots[slot].field, pointer);
        if (length < 0 || (size_t)length >= capacity) return false;
        *source = after;
        return true;
    }
    return false;
}

bool nativeScriptFunctions_Compile(ScriptFunctionProgram* program,
                                    const ScriptStateTable* state,
                                    const NativeScriptArticleCatalog* articles,
                                    bool special_program,
                                    unsigned motion_first, unsigned motion_count,
                                    NativeFighterScriptError* error)
{
    for (unsigned i = 0; i < program->count; ++i) {
        ScriptFunction* function = &program->functions[i];
        if (function->disabled) continue;
        if (!program->model_effect_context && function->model_effect_only) {
            if (function->exported || !program->model_effects || !program->model_effects->count)
                return nativeScriptCode_Fail(error, function->line,
                    "Effect-context helper cannot be exported or used without model effects.");
            continue;
        }
        ScriptCodeBlock* code = &function->code;
        code->model_effect = program->model_effect_context;
        code->state = state;
        code->articles = articles;
        code->functions = program;
        code->special_program = special_program;
        code->collision = true;
        code->predicate = function->return_type != ScriptType_Invalid;
        code->integer_result = function->return_type == ScriptType_Int;
        code->floating_result = function->return_type == ScriptType_Float;
        code->motion_state_first = motion_first;
        code->motion_state_count = motion_count;
        for (unsigned j = 0; j < function->argument_count; ++j) {
            strcpy(code->locals[j], function->argument_names[j]);
            code->local_depth[j] = 0;
            code->local_types[j] = nativeScriptTypes_CType(function->argument_types[j]);
        }
        code->local_count = function->argument_count;
        for (unsigned j = 0; j < function->body_count; ++j)
            if (!nativeScriptCode_AddLine(code, function->body[j]))
                return nativeScriptCode_Fail(error, function->body_lines[j],
                    "Invalid function statement, call arguments, expression or indentation.");
        if (code->last_opens)
            return nativeScriptCode_Fail(error, function->line, "Incomplete function body.");
        if (function->return_type != ScriptType_Invalid &&
            (!code->count || code->lines[code->count-1].depth != 0 ||
             strncmp(code->lines[code->count-1].text, "return ", 7)))
            return nativeScriptCode_Fail(error, function->line,
                "Typed helper needs a final unconditional return value.");
        if (code->landing)
            return nativeScriptCode_Fail(error, function->line,
                "Declare special-landing callbacks in the motion collision handler, not a helper.");
    }
    ScriptFunctionSet reachable[SCRIPT_FUNCTIONS];
    for (unsigned i = 0; i < program->count; ++i)
        reachable[i] = program->functions[i].code.function_calls;
    for (unsigned k = 0; k < program->count; ++k)
        for (unsigned i = 0; i < program->count; ++i)
            if (nativeScriptDependencies_Test(reachable[i], k)) nativeScriptDependencies_Union(&reachable[i], reachable[k]);
    for (unsigned i = 0; i < program->count; ++i)
        if (nativeScriptDependencies_Test(reachable[i], i))
            return nativeScriptCode_Fail(error, program->functions[i].line,
                                        "Recursive script functions are not supported.");
    // The closure is complete before emission. Helpers keep their declared
    // signatures; ordered lifetime checks are emitted at actual call sites.
    for (unsigned i = 0; i < program->count; ++i) {
        bool effect = program->functions[i].code.may_invalidate_source;
        for (unsigned j = 0; j < program->count; ++j)
            if (nativeScriptDependencies_Test(reachable[i], j))
                effect |= program->functions[j].code.may_invalidate_source;
        program->functions[i].may_invalidate_source = effect;
    }
    return true;
}

static void signature(const ScriptFunction* function, FILE* output)
{
    fprintf(output, "static %s script_fn_%s(HSD_GObj* gobj",
        function->return_type == ScriptType_Invalid ? "void" : nativeScriptTypes_Name(function->return_type), function->name);
    for (unsigned i = 0; i < function->argument_count; ++i)
        fprintf(output, ", %s local_%s", function->argument_types[i],
                function->argument_names[i]);
    fputc(')', output);
}

void nativeScriptFunctions_Emit(const ScriptFunctionProgram* program, FILE* output)
{
    for (unsigned i = 0; i < program->count; ++i) {
        if (program->functions[i].disabled || program->functions[i].model_effect_only) continue;
        signature(&program->functions[i], output);
        fputs(";\n", output);
    }
    for (unsigned i = 0; i < program->count; ++i) {
        if (program->functions[i].disabled || program->functions[i].model_effect_only) continue;
        signature(&program->functions[i], output);
        fputs(" {\n    Fighter* fighter = gobj->user_data;\n", output);
        nativeScriptCode_EmitLines(&program->functions[i].code, output);
        fputs("}\n", output);
    }
    if (program->export_count != 0) {
        fputs("static const NativeFighterCallbackExport script_callback_exports[] = {\n", output);
        for (unsigned i = 0; i < program->export_count; ++i) {
            const ScriptFunction* function =
                &program->functions[program->export_order[i]];
            const char* member = function->return_type == ScriptType_Bool ? "bool_callback" :
                function->return_type == ScriptType_Int ? "int_callback" :
                function->return_type == ScriptType_Float ? "float_callback" : "callback";
            const char* result = function->return_type == ScriptType_Bool ? "Bool" :
                function->return_type == ScriptType_Int ? "Int" :
                function->return_type == ScriptType_Float ? "Float" : "Void";
            fprintf(output, "    { .name_hash = UINT32_C(%u), .%s = script_fn_%s, .result_type = NativeFighterReturn_%s },\n",
                    function->export_hash, member, function->name, result);
        }
        fputs("};\n", output);
    }
}

void nativeScriptFunctions_EmitDefinitionExports(const ScriptFunctionProgram* program,
                                                FILE* output)
{
    fprintf(output, "    generated.callback_export_count = %u;\n"
                    "    generated.callback_exports = %s;\n",
            program->export_count,
            program->export_count ? "script_callback_exports" : "NULL");
}
