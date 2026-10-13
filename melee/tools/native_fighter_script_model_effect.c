#include "native_fighter_script_model_effect.h"
#include "native_fighter_script_functions.h"
#include "native_fighter_script_signal.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

ScriptModelEffectProgram* nativeScriptModelEffect_Parse(char* source, size_t size,
                                                       NativeFighterScriptError* error)
{
    ScriptModelEffectProgram* program = calloc(1, sizeof(*program));
    if (!program) { nativeScriptCode_Fail(error, 0, "Out of memory for model effects."); return NULL; }
    ScriptModelEffect* active = NULL;
    bool update = false;
    unsigned line = 0;
    for (size_t offset = 0; offset < size;) {
        size_t end = offset;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - offset;
        if (length && source[offset + length - 1] == '\r') --length;
        ++line;
        if (length >= 512 || memchr(source + offset, 0, length)) {
            nativeScriptCode_Fail(error, line, "Line is too long or contains NUL."); goto invalid;
        }
        char text[512]; memcpy(text, source + offset, length); text[length] = 0;
        while (length && (text[length - 1] == ' ' || text[length - 1] == '\t')) text[--length] = 0;
        const char* first = text; while (*first == ' ') ++first;
        bool ignored = !*first || *first == '#';
        if (!ignored && first == text) {
            if (active && !active->body_count) {
                nativeScriptCode_Fail(error, active->line, "Model effect needs a complete update body."); goto invalid;
            }
            active = NULL; update = false;
            if (!strncmp(text, "model_effect ", 13)) {
                char name[32], recipe[32], extra;
                if (program->count == SCRIPT_MODEL_EFFECTS ||
                    sscanf(text, "model_effect %31s %31s %c", name, recipe, &extra) != 2 ||
                    !nativeScriptCode_Identifier(name, strlen(name), sizeof(name))) {
                    nativeScriptCode_Fail(error, line, "Expected model_effect Name recipeID: (limit 32)."); goto invalid;
                }
                size_t n = strlen(recipe); char* tail; errno = 0;
                if (!n || recipe[n - 1] != ':' || !isdigit((unsigned char)recipe[0])) goto bad_recipe;
                recipe[n - 1] = 0;
                unsigned long long id = strtoull(recipe, &tail, 0);
                if (*tail || errno || id > INT32_MAX) {
bad_recipe:         nativeScriptCode_Fail(error, line, "Model effect recipe must be a nonnegative integer."); goto invalid;
                }
                uint32_t hash = nativeScriptSignal_Hash(name);
                unsigned order = 0;
                while (order < program->count && program->effects[program->order[order]].hash < hash) ++order;
                if (!hash || (order < program->count && program->effects[program->order[order]].hash == hash)) {
                    nativeScriptCode_Fail(error, line, "Duplicate or colliding model-effect name."); goto invalid;
                }
                memmove(program->order + order + 1, program->order + order, program->count - order);
                program->order[order] = (uint8_t)program->count;
                active = &program->effects[program->count++];
                strcpy(active->name, name); active->hash = hash; active->recipe = (int32_t)id; active->line = line;
            }
        } else if (!ignored && active) {
            if (!strcmp(text, "    on update:") && !update) update = true;
            else if (update && !strncmp(text, "        ", 8) && active->body_count < SCRIPT_CODE_LINES) {
                unsigned index = active->body_count++;
                strcpy(active->body[index], text); active->body_lines[index] = line;
            } else { nativeScriptCode_Fail(error, line, "Model effect supports one on update: handler."); goto invalid; }
        }
        if (active) memset(source + offset, ' ', end - offset);
        offset = end < size ? end + 1 : end;
    }
    if (active && !active->body_count) {
        nativeScriptCode_Fail(error, active->line, "Model effect needs a complete update body."); goto invalid;
    }
    return program;
invalid:
    free(program); return NULL;
}

void nativeScriptModelEffect_Free(ScriptModelEffectProgram* program)
{
    if (program) free(program->functions);
    free(program);
}

bool nativeScriptModelEffect_Compile(ScriptModelEffectProgram* program, const ScriptStateTable* state,
    const NativeScriptArticleCatalog* articles, ScriptFunctionProgram* functions, NativeFighterScriptError* error)
{
    if (!program || !program->count) return true;
    program->functions = malloc(sizeof(*program->functions));
    if (!program->functions) return nativeScriptCode_Fail(error, 0, "Out of memory compiling effect helpers.");
    *program->functions = *functions;
    ScriptFunctionProgram* helpers = program->functions;
    helpers->model_effect_context = true; helpers->export_count = 0;
    ScriptFunctionSet referenced = { 0 }, validate = { 0 };
    for (unsigned j = 0; j < program->count; ++j)
        for (unsigned k = 0; k < program->effects[j].body_count; ++k)
            nativeScriptDependencies_Union(&referenced, nativeScriptFunctions_References(helpers, program->effects[j].body[k]));
    for (unsigned i = 0; i < helpers->count; ++i) {
        memset(&helpers->functions[i].code, 0, sizeof(helpers->functions[i].code));
        if (helpers->functions[i].model_effect_only) nativeScriptDependencies_Set(&validate, i);
    }
    nativeScriptDependencies_Union(&validate, referenced);
    bool changed;
    do {
        changed = false;
        ScriptFunctionSet before = validate, used_before = referenced;
        for (unsigned i = 0; i < helpers->count; ++i) if (nativeScriptDependencies_Test(validate, i))
            for (unsigned k = 0; k < helpers->functions[i].body_count; ++k) {
                ScriptFunctionSet calls = nativeScriptFunctions_References(helpers, helpers->functions[i].body[k]);
                nativeScriptDependencies_Union(&validate, calls);
                if (nativeScriptDependencies_Test(referenced, i)) nativeScriptDependencies_Union(&referenced, calls);
            }
        changed = !nativeScriptDependencies_Equal(before, validate) || !nativeScriptDependencies_Equal(used_before, referenced);
    } while (changed);
    program->helper_calls = referenced;
    for (unsigned i = 0; i < helpers->count; ++i)
        helpers->functions[i].disabled = !(nativeScriptDependencies_Test(validate, i));
    if (!nativeScriptFunctions_Compile(helpers, state, articles, false, 0, 0, error)) return false;
    for (unsigned i = 0; i < program->count; ++i) {
        ScriptModelEffect* effect = &program->effects[i];
        effect->code = (ScriptCodeBlock){ .state = state, .articles = articles,
            .functions = helpers, .model_effect = true };
        for (unsigned j = 0; j < effect->body_count; ++j)
            if (!nativeScriptCode_AddLine(&effect->code, effect->body[j]))
                return nativeScriptCode_Fail(error, effect->body_lines[j], "Invalid model-effect update statement, expression or indentation.");
        if (effect->code.last_opens)
            return nativeScriptCode_Fail(error, effect->line, "Incomplete model-effect update body.");
    }
    return true;
}

static void entry(FILE* output, ScriptExpressionType result)
{
    const char* neutral = result == ScriptType_Invalid ? "" :
        result == ScriptType_Float ? " 0.0f" : result == ScriptType_Int ? " 0" : " false";
    fprintf(output, "    HSD_GObj* effect_gobj = effect->gobj;\n"
          "    const uint32_t callback_generation = effect->native_generation;\n"
          "    if (!engine->callback_source_alive(effect_gobj, callback_generation, NativeCallbackSource_ModelEffect)) return%s;\n"
          "    Fighter_GObj* gobj = engine->model_effect_creator(effect);\n"
          "    if (!gobj) { engine->model_effect_cancel(effect); return%s; }\n"
          "    Fighter* fighter = gobj->user_data;\n", neutral, neutral);
}

void nativeScriptModelEffect_Emit(const ScriptModelEffectProgram* program, FILE* output)
{
    if (!program || !program->count) return;
    fputs("#include <melee/ef/types.h>\n", output);
    const ScriptFunctionProgram* helpers = program->functions;
    for (unsigned i = 0; i < helpers->count; ++i) if (nativeScriptDependencies_Test(program->helper_calls, i)) {
        const ScriptFunction* fn = &helpers->functions[i];
        fprintf(output, "static %s script_effect_fn_%s(EF_Effect* effect", fn->return_type == ScriptType_Invalid ? "void" : nativeScriptTypes_Name(fn->return_type), fn->name);
        for (unsigned j = 0; j < fn->argument_count; ++j) fprintf(output, ", %s local_%s", fn->argument_types[j], fn->argument_names[j]);
        fputs(");\n", output);
    }
    for (unsigned i = 0; i < helpers->count; ++i) if (nativeScriptDependencies_Test(program->helper_calls, i)) {
        const ScriptFunction* fn = &helpers->functions[i];
        fprintf(output, "static %s script_effect_fn_%s(EF_Effect* effect", fn->return_type == ScriptType_Invalid ? "void" : nativeScriptTypes_Name(fn->return_type), fn->name);
        for (unsigned j = 0; j < fn->argument_count; ++j) fprintf(output, ", %s local_%s", fn->argument_types[j], fn->argument_names[j]);
        fputs(") {\n", output); entry(output, fn->return_type);
        nativeScriptCode_EmitLines(&fn->code, output); fputs("}\n", output);
    }
    for (unsigned i = 0; i < program->count; ++i) {
        fprintf(output, "static void script_model_effect_%s(EF_Effect* effect) {\n", program->effects[i].name);
        entry(output, ScriptType_Invalid); nativeScriptCode_EmitLines(&program->effects[i].code, output); fputs("}\n", output);
    }
    fputs("static const NativeFighterModelEffect script_model_effects[] = {\n", output);
    for (unsigned i = 0; i < program->count; ++i) {
        const ScriptModelEffect* effect = &program->effects[program->order[i]];
        fprintf(output, "    { UINT32_C(%u), %d, script_model_effect_%s },\n", effect->hash, effect->recipe, effect->name);
    }
    fputs("};\n", output);
}

void nativeScriptModelEffect_EmitChecks(const ScriptModelEffectProgram* program, FILE* output)
{
    if (!program || !program->count) return;
    fputs(" || host->callback_source_alive == 0 || host->model_effect_creator == 0 || host->model_effect_cancel == 0", output);
    for (unsigned i = 0; i < program->count; ++i) nativeScriptCode_EmitHostChecks(&program->effects[i].code, output);
    for (unsigned i = 0; i < program->functions->count; ++i)
        if (nativeScriptDependencies_Test(program->helper_calls, i)) nativeScriptCode_EmitHostChecks(&program->functions->functions[i].code, output);
}

void nativeScriptModelEffect_EmitDefinition(const ScriptModelEffectProgram* program, FILE* output)
{
    fprintf(output, "    generated.model_effect_count = %u; generated.model_effects = %s;\n",
        program ? program->count : 0, program && program->count ? "script_model_effects" : "NULL");
}

static int joint(const char* source, char path[40], unsigned* channel, unsigned* member, const char** after)
{
    if (strncmp(source, "effect.joint(", 13)) return 0;
    char args[2][192]; const char* suffix;
    *member = 0;
    if (nativeScriptCode_ParseArguments(source + 12, args, 2, &suffix)) {
        char* end;
        if (!isdigit((unsigned char)args[1][0])) return -1;
        unsigned long index = strtoul(args[1], &end, 10);
        if (*end || index >= 4) return -1;
        *member = (unsigned)index;
    } else if (!nativeScriptCode_ParseArguments(source + 12, args, 1, &suffix)) return -1;
    size_t size = strlen(args[0]);
    if (size < 2 || size > 41 || args[0][0] != '"' || args[0][size - 1] != '"') return -1;
    memcpy(path, args[0] + 1, size - 2); path[size - 2] = 0;
    unsigned depth = 0;
    for (const char* p = path; *p;) {
        if (++depth > 12 || !isdigit((unsigned char)*p)) return -1;
        unsigned ordinal = 0;
        while (isdigit((unsigned char)*p)) { ordinal = ordinal * 10 + (unsigned)(*p++ - '0'); if (ordinal > 255) return -1; }
        if (*p && (*p++ != '/' || !*p)) return -1;
    }
    static const char* properties[] = { ".translation.x", ".translation.y", ".translation.z",
        ".rotation.x", ".rotation.y", ".rotation.z", ".scale.x", ".scale.y", ".scale.z",
        ".exists", ".visible", ".quaternion.x", ".quaternion.y", ".quaternion.z", ".quaternion.w", ".self_visible" };
    for (unsigned i = 0; i < 16; ++i) {
        size_t length = strlen(properties[i]);
        if (!strncmp(suffix, properties[i], length) && !(isalnum((unsigned char)suffix[length]) || suffix[length] == '_' || suffix[length] == '.')) {
            *channel = i; *after = suffix + length; return 1;
        }
    }
    return -1;
}

int nativeScriptModelEffect_Expression(const ScriptCodeBlock* block, const char* source,
                                       char* output, size_t capacity, const char** after)
{
    if (!block->model_effect) return 0;
    char path[40]; unsigned channel, member;
    int found = joint(source, path, &channel, &member, after);
    if (found) {
        if (found < 0) return -1;
        int n = snprintf(output, capacity, "engine->model_joint_read(engine->model_joint_resolve(engine->model_effect_joint(effect, %u), \"%s\"), %u)", member, path, channel);
        return n >= 0 && (size_t)n < capacity ? 1 : -1;
    }
    NativeScriptStateReference ref;
    found = nativeScriptState_Reference(block->state, source, "creator.state.", &ref);
    if (found) {
        if (found < 0) return -1;
        char index[1024] = "0";
        if (ref.field->array) {
            if (nativeScriptCode_ExpressionType(block, ref.index) != ScriptType_Int ||
                !nativeScriptCode_Expression(block, ref.index, index, sizeof(index)) ||
                !nativeScriptState_ArrayRead(ref.field, "fighter->native_module_state + script_state_base", index, output, capacity)) return -1;
        } else {
            int written = ref.field->type == ScriptState_Bool
                ? snprintf(output, capacity, "fighter->native_module_state[script_state_base + %u]", ref.field->offset)
                : snprintf(output, capacity, "script_state_get_%s(fighter, script_state_base + %u)", ref.field->type == ScriptState_Int ? "int" : "float", ref.field->offset);
            if (written < 0 || (size_t)written >= capacity) return -1;
        }
        *after = ref.after; return 1;
    }
    static const struct { const char* name; const char* emitted; } props[] = {
        { "creator.motion", "fighter->motion_id" },
        { "creator.facing", "fighter->facing_dir" },
        { "creator.command0", "fighter->cmd_vars[0]" }, { "creator.command1", "fighter->cmd_vars[1]" },
        { "creator.command2", "fighter->cmd_vars[2]" }, { "creator.command3", "fighter->cmd_vars[3]" },
        { "creator.floor.normal.x", "fighter->coll_data.floor.normal.x" },
        { "creator.floor.normal.y", "fighter->coll_data.floor.normal.y" },
        { "creator.floor.normal.z", "fighter->coll_data.floor.normal.z" },
    };
    for (unsigned i = 0; i < sizeof(props) / sizeof(props[0]); ++i) {
        size_t n = strlen(props[i].name);
        if (!strncmp(source, props[i].name, n) && !(isalnum((unsigned char)source[n]) || source[n] == '.' || source[n] == '_')) {
            if (snprintf(output, capacity, "%s", props[i].emitted) >= capacity) return -1;
            *after = source + n; return 1;
        }
    }
    return !strncmp(source, "creator.", 8) || !strncmp(source, "effect.", 7) ? -1 : 0;
}

ScriptExpressionType nativeScriptModelEffect_Type(const ScriptCodeBlock* block, const char* atom)
{
    char path[40]; unsigned channel, member; const char* after;
    int found = joint(atom, path, &channel, &member, &after);
    if (found) return found > 0 && !*after ? (channel == 9 || channel == 10 || channel == 15 ? ScriptType_Bool : ScriptType_Float) : ScriptType_Invalid;
    NativeScriptStateReference ref;
    found = nativeScriptState_Reference(block->state, atom, "creator.state.", &ref);
    if (found) return found > 0 && !*ref.after && (!ref.field->array || nativeScriptCode_ExpressionType(block, ref.index) == ScriptType_Int) ? nativeScriptTypes_State(ref.field->type) : ScriptType_Invalid;
    if (!strcmp(atom, "creator.motion") || (!strncmp(atom, "creator.command", 15) && strlen(atom) == 16 && atom[15] >= '0' && atom[15] <= '3')) return ScriptType_Int;
    if (!strcmp(atom, "creator.facing")) return ScriptType_Float;
    if (!strncmp(atom, "creator.floor.normal.", 21) && strlen(atom) == 22 && strchr("xyz", atom[21])) return ScriptType_Float;
    return ScriptType_Invalid;
}

bool nativeScriptModelEffect_AllowedName(const ScriptCodeBlock* block, const char* name)
{
    static const char* generic[] = { "true", "false", "and", "or", "not", "abs", "sin", "cos", "atan2", "atan2_float", "radians", "signbit", "int", "uint", "pi_half", "pi" };
    for (unsigned i = 0; i < sizeof(generic) / sizeof(generic[0]); ++i) if (!strcmp(name, generic[i])) return true;
    for (unsigned i = 0; i < block->local_count; ++i) if (!strcmp(name, block->locals[i])) return true;
    if (!strncmp(name, "attrs.", 6) || !strncmp(name, "program.attrs.", 14)) {
        ScriptScalar scalar;
        return nativeScriptCode_ParseScalar(name, block->state, &scalar) && scalar.parameter_index_plus_one;
    }
    return false;
}

int nativeScriptModelEffect_Action(ScriptCodeBlock* block, const char* source, char* output, size_t capacity)
{
    if (!block->model_effect) return 0;
    if (!strcmp(source, "effect.cancel()")) return snprintf(output, capacity,
        "engine->model_effect_cancel(effect); return%s;",
        block->floating_result ? " 0.0f" : block->integer_result ? " 0" : block->predicate ? " false" : "") < capacity ? 1 : -1;
    char path[40]; unsigned channel, member; const char* after;
    int found = joint(source, path, &channel, &member, &after);
    if (found) {
        char value[1024];
        if (found < 0 || channel == 9 || strncmp(after, " = ", 3) ||
            !nativeScriptTypes_Assignable(channel == 10 || channel == 15 ? ScriptType_Bool : ScriptType_Float, nativeScriptCode_ExpressionType(block, after + 3)) ||
            !nativeScriptCode_Expression(block, after + 3, value, sizeof(value))) return -1;
        return snprintf(output, capacity, "engine->model_joint_write(engine->model_joint_resolve(engine->model_effect_joint(effect, %u), \"%s\"), %u, %s);", member, path, channel, value) < capacity ? 1 : -1;
    }
    // The rest uses the same local declaration/assignment/return grammar.
    if (!strcmp(source, "return") || !strncmp(source, "return ", 7) ||
        !strcmp(source, "pass") || !strncmp(source, "let ", 4)) return 0;
    for (unsigned i = 0; i < block->local_count; ++i) {
        size_t n = strlen(block->locals[i]);
        if (!strncmp(source, block->locals[i], n) && !strncmp(source + n, " = ", 3)) return 0;
    }
    return -1;
}

bool nativeScriptModelEffect_Spawn(const ScriptCodeBlock* block, const char* source, char* output, size_t capacity)
{
    if (!block->functions || !block->functions->model_effects || block->model_effect) return false;
    bool root = !strncmp(source, "effects.spawn_root(", 19);
    bool at = !strncmp(source, "effects.spawn_at(", 17);
    bool deferred = !strncmp(source, "effects.spawn_async_on_bone(", 28);
    if (!root && !at && !deferred && strncmp(source, "effects.spawn_on_bone(", 22)) return false;
    char args[4][192]; const char* after;
    if (!nativeScriptCode_ParseArguments(strchr(source, '('), args, root ? 1 : at ? 4 : 2, &after) || *after) return false;
    const ScriptModelEffectProgram* program = block->functions->model_effects;
    for (unsigned i = 0; i < program->count; ++i) {
        const ScriptModelEffect* effect = &program->effects[program->order[i]];
        if (strcmp(args[0], effect->name)) continue;
        if (root) return snprintf(output, capacity, "engine->spawn_model_effect(gobj, &script_model_effects[%u], -1, NULL, false); fighter->x2219_b0 = true;", i) < capacity;
        if (at) {
            char xyz[3][512];
            for (unsigned j = 0; j < 3; ++j)
                if (!nativeScriptTypes_Assignable(ScriptType_Float, nativeScriptCode_ExpressionType(block, args[j + 1])) ||
                    !nativeScriptCode_Expression(block, args[j + 1], xyz[j], sizeof(xyz[j]))) return false;
            return snprintf(output, capacity, "{ Vec3 model_position; model_position.x = %s; model_position.y = %s; model_position.z = %s; engine->spawn_model_effect(gobj, &script_model_effects[%u], -1, &model_position, false); fighter->x2219_b0 = true; }", xyz[0], xyz[1], xyz[2], i) < capacity;
        }
        const char* bone = nativeScriptCode_BoneSymbol(args[1], strlen(args[1]));
        if (!bone) return false;
        return snprintf(output, capacity, "engine->spawn_model_effect(gobj, &script_model_effects[%u], engine->resolve_bone(fighter, %s), NULL, %s); fighter->x2219_b0 = true;", i, bone, deferred ? "true" : "false") < capacity;
    }
    return false;
}
