#include "native_fighter_item_program.h"
#include "native_fighter_script_lifetime.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
bool nativeItemStatements_Finish(ItemCallback* callback)
{
    if (callback == NULL) return true;
    if (callback->needs_body) return false;
    while (callback->block_depth) {
        if (callback->count == CALLBACK_LIMIT) return false;
        strcpy(callback->lines[callback->count++], "}");
        --callback->block_depth;
    }
    return true;
}

// A held-status query cannot consume RNG, mutate engine state or call helpers.
// Inspect source tokens, not generated C substrings or quoted identifiers.
static bool read_only_calls(const char* source)
{
    static const char* allowed[] = { "abs", "min", "max", "sin", "cos", "sqrt",
        "atan2", "radians", "wrap_angle", "wrap_signed_angle", "int", "uint", "float",
        "articles.ref", "targets.held_id", "targets.held_kind",
        "targets.held_motion", "targets.held_frame" };
    while (*source) {
        if (*source == '"') {
            ++source;
            while (*source && *source != '"') ++source;
            if (!*source) return false;
            ++source; continue;
        }
        if (!isalpha((unsigned char)*source) && *source != '_') { ++source; continue; }
        const char* begin = source++;
        while (isalnum((unsigned char)*source) || *source == '_' || *source == '.') ++source;
        const char* next = source;
        while (isspace((unsigned char)*next)) ++next;
        if (*next != '(') continue;
        bool found = false;
        for (unsigned i = 0; i < sizeof(allowed) / sizeof(*allowed); ++i)
            if (strlen(allowed[i]) == (size_t)(source - begin) &&
                strncmp(begin, allowed[i], (size_t)(source - begin)) == 0) { found = true; break; }
        if (!found) return false;
    }
    return true;
}

bool nativeItemStatements_Add(ItemCallback* callback, const char* line,
                              unsigned indent, bool predicate,
                              ArticleDeclaration* article, bool interaction,
                              const NativeFighterParameters* parameters)
{
    if (!nativeScriptLifetime_TargetSurface(line)) return false;
    if (!callback->helper && callback->result_type == ScriptType_Int &&
        (!read_only_calls(line) || strncmp(line, "while ", 6) == 0)) return false;
    if (indent < 12 || (indent & 3)) return false;
    while (callback->local_count && callback->local_indent[callback->local_count - 1] > indent)
        --callback->local_count;
    article->expression_callback = callback;
    if (!interaction && strstr(line, "item.attachment_owner_is(") != NULL) return false;
    if (callback->needs_body &&
        indent != callback->block_indent[callback->block_depth - 1] + 4)
        return false;
    callback->needs_body = false;
    bool continuation = strncmp(line, "elif ", 5) == 0 ||
                        strcmp(line, "else:") == 0;
    while (callback->block_depth &&
           indent <= callback->block_indent[callback->block_depth - 1]) {
        if (continuation && indent == callback->block_indent[callback->block_depth - 1]) break;
        if (callback->count == CALLBACK_LIMIT) return false;
        strcpy(callback->lines[callback->count++], "}");
        --callback->block_depth;
    }
    if (continuation) {
        if (!callback->block_depth ||
            !callback->block_else[callback->block_depth - 1] ||
            indent != callback->block_indent[callback->block_depth - 1]) return false;
    } else if (indent != 12 + callback->block_depth * 4) return false;
    if (callback->count == CALLBACK_LIMIT) return false;
    char* output = callback->lines[callback->count];
    size_t length = strlen(line);
    bool loop = strncmp(line, "while ", 6) == 0;
    bool conditional = loop || strncmp(line, "if ", 3) == 0 || strncmp(line, "elif ", 5) == 0;
    if (conditional && length && line[length - 1] == ':') {
        unsigned prefix = loop ? 6 : continuation ? 5 : 3;
        char condition[ITEM_LINE_BYTES], translated[ITEM_LINE_BYTES - 32];
        memcpy(condition, line + prefix, length - prefix - 1);
        condition[length - prefix - 1] = 0;
        if (!nativeFighterItemScript_Expression(condition, translated, sizeof(translated), article, parameters)) return false;
        snprintf(output, ITEM_LINE_BYTES, "%s%s (%s) {", continuation ? "} else " : "", loop ? "while" : "if", translated);
    } else if (strcmp(line, "else:") == 0) {
        strcpy(output, "} else {");
        callback->block_else[callback->block_depth - 1] = false;
    } else {
        if (continuation) return false;
        bool declaration = strncmp(line, "let ", 4) == 0;
        const char* name = line + (declaration ? 4 : 0);
        ScriptExpressionType type = declaration ? nativeScriptTypes_Declaration(&name) : ScriptType_Invalid;
        const char* assign = strstr(name, " = ");
        char local[32];
        unsigned local_index = callback->local_count;
        if (assign && (size_t)(assign - name) < sizeof(local)) {
            memcpy(local, name, assign - name); local[assign - name] = 0;
            for (unsigned i = 0; i < callback->local_count; ++i)
                if (strcmp(callback->locals[i], local) == 0) { local_index = i; break; }
        } else local[0] = 0;
        if (callback->result_type != ScriptType_Invalid && strncmp(line, "return ", 7) == 0) {
            char value[ITEM_LINE_BYTES - 16];
            if (!(callback->helper ? nativeScriptTypes_Assignable(callback->result_type,
                    nativeFighterItemScript_ExpressionType(line + 7, article, parameters)) :
                    nativeFighterItemScript_ExpressionType(line + 7, article, parameters) == callback->result_type) ||
                !nativeFighterItemScript_Expression(line + 7, value, sizeof(value), article, parameters)) return false;
            int written = snprintf(output, ITEM_LINE_BYTES, "return %s;", value);
            if (written < 0 || written >= ITEM_LINE_BYTES) return false;
        } else if (callback->helper && (strcmp(line, "return") == 0 || strncmp(line, "return ", 7) == 0)) {
            if (callback->result_type != ScriptType_Invalid || strcmp(line, "return")) return false;
            strcpy(output, "return;");
        } else if (declaration || local_index < callback->local_count) {
            if (!assign || !nativeScriptCode_Identifier(local, strlen(local), sizeof(local)) ||
                (declaration && (local_index != callback->local_count ||
                    local_index == SCRIPT_CODE_LOCALS))) return false;
            if (!declaration) type = callback->local_types[local_index];
            if (strstr(assign + 3, "targets.call(") && type != ScriptType_Bool) return false;
            char value[ITEM_LINE_BYTES - 64];
            if (!nativeScriptTypes_Assignable(type,
                    nativeFighterItemScript_ExpressionType(assign + 3, article, parameters)) ||
                !nativeFighterItemScript_Expression(assign + 3, value, sizeof(value), article, parameters)) return false;
            int written = declaration
                ? snprintf(output, ITEM_LINE_BYTES, "%s local_%s = %s;", nativeScriptTypes_Name(type), local, value)
                : snprintf(output, ITEM_LINE_BYTES, "local_%s = %s;", local, value);
            if (written < 0 || written >= ITEM_LINE_BYTES) return false;
            if (declaration) {
                strcpy(callback->locals[local_index], local);
                callback->local_types[local_index] = type;
                callback->local_indent[local_index] = indent;
                ++callback->local_count;
            }
        } else if (!callback->helper && callback->result_type == ScriptType_Int && strcmp(line, "pass") != 0) {
            return false;
        } else if (!nativeFighterItemScript_Action(line, predicate ? ITEM_HIT : ITEM_TICK,
                                        article, interaction, parameters, output,
                                        ITEM_LINE_BYTES)) return false;
        ++callback->count;
        return true;
    }
    if (!continuation) {
        if (callback->block_depth == 8) return false;
        callback->block_indent[callback->block_depth] = indent;
        callback->block_else[callback->block_depth++] = !loop;
    }
    ++callback->count;
    callback->needs_body = true;
    return true;
}

bool nativeItemStatements_MayInvalidate(const ItemCallback* callback,
                                    const ScriptFunctionProgram* functions)
{
    for (unsigned line = 0; line < callback->count; ++line)
        if (nativeScriptLifetime_LineMayInvalidate(callback->lines[line], functions,
                                                   ScriptCallback_Item)) return true;
    return false;
}

void nativeItemStatements_Emit(FILE* output, const ItemCallback* callback,
                               const ScriptFunctionProgram* functions, bool predicate)
{
    if (nativeItemStatements_MayInvalidate(callback, functions))
        nativeScriptLifetime_EmitEntry(output, ScriptCallback_Item);
    for (unsigned line = 0; line < callback->count; ++line) {
        fputs("    ", output);
        nativeScriptLifetime_EmitLine(output, callback->lines[line], functions,
            ScriptCallback_Item, callback->result_type == ScriptType_Float ? ScriptCallback_Float :
                callback->result_type == ScriptType_Bool ? ScriptCallback_Bool :
                callback->result_type == ScriptType_Int ? ScriptCallback_Int :
                predicate ? ScriptCallback_Bool : ScriptCallback_Void);
        fputc('\n', output);
    }
}
