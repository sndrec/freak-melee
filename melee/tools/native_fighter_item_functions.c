#include "native_fighter_item_functions.h"
#include "native_fighter_item_operations.h"
#include "native_fighter_script_functions.h"
#include "native_fighter_script_lifetime.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool fail(NativeFighterScriptError* error, unsigned line, const char* text)
{
    error->line = line;
    snprintf(error->message, sizeof(error->message), "%s", text);
    return false;
}

static bool name_allowed(const char* name)
{
    static const char* reserved[] = { "func", "export", "action", "article",
        "state", "item", "owner", "creator", "if", "elif", "else", "while",
        "return", "pass", "let", "true", "false", "random", "abs", "sin",
        "cos", "atan2", "radians", "int", "float", "bool", "pi", "tau" };
    if (!nativeScriptCode_Identifier(name, strlen(name), 32)) return false;
    for (unsigned i = 0; i < sizeof(reserved) / sizeof(*reserved); ++i)
        if (!strcmp(name, reserved[i])) return false;
    return true;
}

static bool declaration(NativeItemFunction* function, char* text)
{
    char* open = strchr(text, '('), *close = strrchr(text, ')');
    if (!open || !close || close < open) return false;
    function->return_type = ScriptType_Invalid;
    if (strcmp(close + 1, ":")) {
        const char* suffix = close + 1;
        if (!strcmp(suffix, " -> bool:")) function->return_type = ScriptType_Bool;
        else if (!strcmp(suffix, " -> int:")) function->return_type = ScriptType_Int;
        else if (!strcmp(suffix, " -> float:")) function->return_type = ScriptType_Float;
        else return false;
    }
    *open = *close = 0;
    if (!name_allowed(text + 9)) return false;
    strcpy(function->name, text + 9);
    char* next = open + 1;
    if (!*next) return true;
    for (;;) {
        char* comma = strchr(next, ',');
        if (comma) *comma = 0;
        while (*next == ' ') ++next;
        size_t n = strlen(next);
        while (n && next[n - 1] == ' ') next[--n] = 0;
        char* space = strchr(next, ' ');
        if (!space || function->argument_count == ITEM_FUNCTION_ARGS) return false;
        *space++ = 0;
        ScriptExpressionType type = !strcmp(next, "float") ? ScriptType_Float :
            !strcmp(next, "int") ? ScriptType_Int :
            !strcmp(next, "bool") ? ScriptType_Bool : ScriptType_Invalid;
        if (type == ScriptType_Invalid || !name_allowed(space)) return false;
        for (unsigned i = 0; i < function->argument_count; ++i)
            if (!strcmp(space, function->argument_names[i])) return false;
        unsigned index = function->argument_count++;
        strcpy(function->argument_names[index], space);
        function->argument_types[index] = type;
        if (!comma) return true;
        next = comma + 1;
    }
}

bool nativeItemFunctions_Parse(ArticleDeclaration* article, const char* source,
    size_t size, size_t start, unsigned line, NativeFighterScriptError* error)
{
    NativeItemFunction* active = NULL;
    for (size_t at = start; at < size;) {
        size_t end = at;
        while (end < size && source[end] != '\n') ++end;
        size_t next = end < size ? end + 1 : end, length = end - at;
        if (length && source[at + length - 1] == '\r') --length;
        ++line;
        if (length >= ITEM_LINE_BYTES) return fail(error, line, "Article helper line is too long.");
        char text[ITEM_LINE_BYTES];
        memcpy(text, source + at, length); text[length] = 0;
        while (length && text[length - 1] == ' ') text[--length] = 0;
        char* nonspace = text;
        while (*nonspace == ' ') ++nonspace;
        bool ignored = !*nonspace || *nonspace == '#';
        unsigned indent = (unsigned)(nonspace - text);
        if (!ignored && active && indent <= 4) { active->end = at; active = NULL; }
        if (!ignored && indent == 0) break;
        if (!ignored && !strncmp(text, "    func ", 9)) {
            if (!article->item_functions) {
                article->item_functions = calloc(1, sizeof(*article->item_functions));
                if (!article->item_functions) return fail(error, line, "Out of memory for article helpers.");
            }
            NativeItemFunctionProgram* program = article->item_functions;
            if (program->count == ITEM_FUNCTION_LIMIT)
                return fail(error, line, "At most 32 helpers are allowed per article.");
            active = &program->functions[program->count];
            if (!declaration(active, text))
                return fail(error, line, "Expected article func Name(typed arguments) with optional -> bool/int/float: (up to eight arguments).");
            for (unsigned i = 0; i < program->count; ++i)
                if (!strcmp(active->name, program->functions[i].name))
                    return fail(error, line, "Article helper is declared twice.");
            ++program->count;
            active->begin = at; active->end = size;
            active->line = line; active->first_body_line = line + 1;
            active->source = source;
            // Body begins immediately after the declaration, while begin also
            // includes the declaration for source stripping during the main pass.
            active->body.local_count = active->argument_count;
            active->body.helper = true;
            active->body.result_type = active->return_type;
            for (unsigned i = 0; i < active->argument_count; ++i) {
                strcpy(active->body.locals[i], active->argument_names[i]);
                active->body.local_types[i] = active->argument_types[i];
                active->body.local_indent[i] = 12;
            }
        }
        at = next;
    }
    return true;
}

bool nativeItemFunctions_Contains(const ArticleDeclaration* article, size_t offset,
    bool* declaration_line)
{
    if (!article->item_functions) return false;
    for (unsigned i = 0; i < article->item_functions->count; ++i) {
        const NativeItemFunction* function = &article->item_functions->functions[i];
        if (offset >= function->begin && offset < function->end) {
            *declaration_line = offset == function->begin;
            return true;
        }
    }
    return false;
}

int nativeItemFunctions_Call(const ArticleDeclaration* article, const char* line,
    const NativeFighterParameters* parameters, char* output, size_t capacity)
{
    if (!article || !article->item_functions) return 0;
    const char* open = strchr(line, '(');
    if (!open) return 0;
    for (unsigned i = 0; i < article->item_functions->count; ++i) {
        const NativeItemFunction* function = &article->item_functions->functions[i];
        if (strlen(function->name) != (size_t)(open - line) ||
            memcmp(function->name, line, open - line)) continue;
        char arguments[ITEM_FUNCTION_ARGS][192]; const char* after;
        if (!nativeScriptCode_ParseArguments(open, arguments, function->argument_count, &after) || *after)
            return -1;
        char values[ITEM_FUNCTION_ARGS][512];
        bool ordered = false;
        for (unsigned a = 0; a < function->argument_count; ++a) {
            if (!nativeScriptTypes_Assignable(function->argument_types[a],
                    nativeFighterItemScript_ExpressionType(arguments[a], article, parameters)) ||
                !nativeFighterItemScript_Expression(arguments[a], values[a], sizeof(values[a]), article, parameters)) return -1;
            // Calls may consume RNG, mutate state, or retire this source. Bind
            // arguments left-to-right before entering the outer direct helper.
            ordered |= strchr(arguments[a], '(') != NULL;
        }
        int n = snprintf(output, capacity, ordered ? "({ " : "");
        if (n < 0 || (size_t)n >= capacity) return -1;
        size_t used = (size_t)n;
        if (ordered)
            for (unsigned a = 0; a < function->argument_count; ++a) {
                n = snprintf(output + used, capacity - used,
                    "__auto_type article_arg_%u = %s; ", a, values[a]);
                if (n < 0 || (size_t)n >= capacity - used) return -1;
                used += (size_t)n;
            }
        n = snprintf(output + used, capacity - used,
            "scripted_article_%u_helper_%u(gobj", article->index, i);
        if (n < 0 || (size_t)n >= capacity - used) return -1;
        used += (size_t)n;
        for (unsigned a = 0; a < function->argument_count; ++a) {
            n = ordered ? snprintf(output + used, capacity - used, ", article_arg_%u", a) :
                snprintf(output + used, capacity - used, ", %s", values[a]);
            if (n < 0 || (size_t)n >= capacity - used) return -1;
            used += (size_t)n;
        }
        n = snprintf(output + used, capacity - used, ordered ? "); });" : ");");
        if (n < 0 || (size_t)n >= capacity - used) return -1;
        if (!article->expression_callback) return -1;
        article->expression_callback->function_calls |= UINT32_C(1) << i;
        return 1;
    }
    return 0;
}

ScriptExpressionType nativeItemFunctions_Result(const ArticleDeclaration* article,
    const char* source, bool* found)
{
    *found = false;
    if (!article || !article->item_functions) return ScriptType_Invalid;
    const char* end = source;
    while (isalnum((unsigned char)*end) || *end == '_') ++end;
    if (*end != '(') return ScriptType_Invalid;
    for (unsigned i = 0; i < article->item_functions->count; ++i) {
        const NativeItemFunction* function = &article->item_functions->functions[i];
        if (strlen(function->name) == (size_t)(end - source) &&
            !memcmp(function->name, source, end - source)) {
            *found = true; return function->return_type;
        }
    }
    return ScriptType_Invalid;
}

int nativeItemFunctions_Expression(const ArticleDeclaration* article, const char* source,
    const NativeFighterParameters* parameters, char* output, size_t capacity,
    const char** after)
{
    bool found;
    ScriptExpressionType type = nativeItemFunctions_Result(article, source, &found);
    if (!found) return 0;
    if (type == ScriptType_Invalid) return -1;
    const char* end = strchr(source, '(');
    unsigned depth = 0;
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
    if ((size_t)(end - source) >= sizeof(call)) return -1;
    memcpy(call, source, end - source); call[end - source] = 0;
    if (nativeItemFunctions_Call(article, call, parameters, output, capacity) != 1) return -1;
    size_t length = strlen(output);
    if (!length || output[length - 1] != ';') return -1;
    output[length - 1] = 0;
    *after = end;
    return 1;
}

static bool mark_calls(ItemCallback* body, const ArticleDeclaration* article)
{
    for (unsigned i = 0; i < body->count; ++i) {
        char result[ITEM_LINE_BYTES]; size_t used = 0;
        const char* cursor = body->lines[i];
        while (*cursor) {
            bool replaced = false;
            for (unsigned h = 0; h < article->item_functions->count; ++h) {
                if (!article->item_functions->functions[h].may_invalidate_source) continue;
                char from[64], to[80];
                int f = snprintf(from, sizeof(from), "scripted_article_%u_helper_%u(", article->index, h);
                int t = snprintf(to, sizeof(to), "scripted_article_%u_mutating_%shelper_%u(",
                    article->index, article->item_functions->functions[h].return_type == ScriptType_Invalid ? "" : "value_", h);
                if (!strncmp(cursor, from, f)) {
                    if (used + t >= sizeof(result)) return false;
                    memcpy(result + used, to, t); used += t; cursor += f;
                    replaced = true; break;
                }
            }
            if (!replaced) {
                if (used + 1 >= sizeof(result)) return false;
                result[used++] = *cursor++;
            }
        }
        result[used] = 0; strcpy(body->lines[i], result);
    }
    return true;
}

static bool visit(ArticleDeclaration* article, unsigned node, unsigned origin_line,
    uint8_t visited[ITEM_FUNCTION_LIMIT + NATIVE_FIGHTER_ARTICLE_SIGNAL_LIMIT],
    NativeFighterScriptError* error)
{
    NativeItemFunction* function = node < ITEM_FUNCTION_LIMIT
        ? &article->item_functions->functions[node] : NULL;
    ItemCallback* body = function ? &function->body
        : &article->signals[node - ITEM_FUNCTION_LIMIT].callback;
    if (visited[node] == 2) return true;
    if (visited[node] == 1)
        return fail(error, origin_line, "Recursive article helpers or local signals are not allowed.");
    visited[node] = 1;
    for (unsigned i = 0; i < article->item_functions->count; ++i)
        if (body->function_calls & (UINT32_C(1) << i)) {
            if (!visit(article, i, origin_line, visited, error)) return false;
            if (function)
                function->may_invalidate_source |= article->item_functions->functions[i].may_invalidate_source;
        }
    for (unsigned i = 0; i < article->signal_count; ++i)
        if (body->signal_calls & (UINT32_C(1) << i))
            if (!visit(article, ITEM_FUNCTION_LIMIT + i, origin_line, visited, error)) return false;
    if (function)
        for (unsigned i = 0; i < body->count; ++i)
            function->may_invalidate_source |= nativeScriptLifetime_LineMayInvalidate(
                body->lines[i], article->functions, ScriptCallback_Item);
    visited[node] = 2;
    return true;
}

bool nativeItemFunctions_Compile(ArticleDeclaration* article,
    const NativeFighterParameters* parameters, NativeFighterScriptError* error)
{
    if (!article->item_functions) return true;
    // Helpers have no callback payload; pass signal data explicitly as arguments.
    article->in_signal = false;
    for (unsigned i = 0; i < article->item_functions->count; ++i) {
        NativeItemFunction* function = &article->item_functions->functions[i];
        const char* source = function->source;
        size_t at = function->begin;
        while (at < function->end && source[at] != '\n') ++at;
        if (at < function->end) ++at;
        unsigned line = function->first_body_line;
        for (; at < function->end; ++line) {
            size_t end = at;
            while (end < function->end && source[end] != '\n') ++end;
            size_t n = end - at;
            if (n && source[at + n - 1] == '\r') --n;
            char text[ITEM_LINE_BYTES]; memcpy(text, source + at, n); text[n] = 0;
            while (n && text[n - 1] == ' ') text[--n] = 0;
            const char* content = text; while (*content == ' ') ++content;
            if (*content && *content != '#') {
                unsigned indent = (unsigned)(content - text);
                if (indent < 8 || !nativeItemStatements_Add(&function->body,
                        content, indent + 4, false, article, false, parameters))
                    return fail(error, line, "Invalid article helper statement, argument type or indentation.");
            }
            at = end < function->end ? end + 1 : end;
        }
        article->expression_callback = NULL;
        function->source = NULL;
        if (function->return_type != ScriptType_Invalid &&
            (function->body.block_depth || !function->body.count ||
             strncmp(function->body.lines[function->body.count - 1], "return ", 7)))
            return fail(error, function->line, "Typed article helper needs a final unconditional return value.");
        if (!function->body.count || !nativeItemStatements_Finish(&function->body))
            return fail(error, function->line, "Article helper needs a complete body.");
    }
    uint8_t visited[ITEM_FUNCTION_LIMIT + NATIVE_FIGHTER_ARTICLE_SIGNAL_LIMIT] = { 0 };
    for (unsigned i = 0; i < article->item_functions->count; ++i)
        if (!visit(article, i, article->item_functions->functions[i].line, visited, error)) return false;
    for (unsigned i = 0; i < article->item_functions->count; ++i)
        if (!mark_calls(&article->item_functions->functions[i].body, article)) return false;
    for (unsigned i = 0; i < article->state_count; ++i)
        for (unsigned c = 0; c < ARTICLE_STATE_CALLBACKS; ++c)
            if (!mark_calls(&article->states[i].callbacks[c], article)) return false;
    for (unsigned i = 0; i < ARTICLE_LOGIC_CALLBACKS; ++i)
        if (!mark_calls(&article->callbacks[i], article)) return false;
    for (unsigned i = 0; i < article->signal_count; ++i)
        if (!mark_calls(&article->signals[i].callback, article)) return false;
    return true;
}

ScriptExpressionType nativeItemFunctions_RecipientResult(const ArticleDeclaration* article,
    const char* source)
{
    if (!article) return ScriptType_Invalid;
    char args[9][192];
    const ScriptFunction* fn = nativeItemOperations_FighterFunction(source, article->functions, args);
    return fn ? fn->return_type : ScriptType_Invalid;
}

int nativeItemFunctions_RecipientCall(const ArticleDeclaration* article, const char* source,
    const NativeFighterParameters* parameters, bool expression, char* output,
    size_t capacity, const char** after)
{
    bool creator = !strncmp(source, "creator.call(", 13);
    if (!creator && strncmp(source, "owner.call(", 11)) return 0;
    if (!article || !article->expression_callback) return -1;
    const char* end = strchr(source, '(');
    unsigned depth = 0;
    do {
        if (!*end) return -1;
        if (*end == '(') ++depth;
        else if (*end == ')') --depth;
        ++end;
    } while (depth);
    const char* suffix = end; while (*suffix == ' ') ++suffix;
    if (expression ? (*suffix == '.' || *suffix == '[' || *suffix == '(') : *suffix != 0) return -1;
    char call[512], args[9][192], values[ITEM_FUNCTION_ARGS][512];
    if ((size_t)(end - source) >= sizeof(call)) return -1;
    memcpy(call, source, end - source); call[end - source] = 0;
    const ScriptFunction* fn = nativeItemOperations_FighterFunction(call, article->functions, args);
    if (!fn || (expression && fn->return_type == ScriptType_Invalid)) return -1;
    bool ordered = false;
    for (unsigned a = 0; a < fn->argument_count; ++a) {
        if (!nativeScriptTypes_Assignable(nativeScriptTypes_CType(fn->argument_types[a]),
                nativeFighterItemScript_ExpressionType(args[a + 1], article, parameters)) ||
            !nativeFighterItemScript_Expression(args[a + 1], values[a], sizeof(values[a]), article, parameters)) return -1;
        ordered |= strchr(args[a + 1], '(') != NULL;
    }
    const char* neutral = fn->return_type == ScriptType_Float ? "0.0f" :
        fn->return_type == ScriptType_Bool ? "false" : "0";
    int n = expression ? snprintf(output, capacity, "({ %s recipient_result = %s; ",
        nativeScriptTypes_Name(fn->return_type), neutral) : snprintf(output, capacity, "{ ");
    if (n < 0 || (size_t)n >= capacity) return -1;
    size_t used = n;
#define APPEND(...) do { n = snprintf(output + used, capacity - used, __VA_ARGS__); \
    if (n < 0 || (size_t)n >= capacity - used) return -1; used += (size_t)n; } while (0)
    APPEND("HSD_GObj* recipient = engine->item_creator(gobj); if (recipient != NULL%s) { uint32_t recipient_generation = ((Fighter*)recipient->user_data)->x8_spawnNum; ",
        creator ? "" : " && item->owner == recipient");
    if (ordered)
        for (unsigned a = 0; a < fn->argument_count; ++a)
            APPEND("__auto_type recipient_arg_%u = %s; ", a, values[a]);
    APPEND("if (engine->callback_source_alive(recipient, recipient_generation, NativeCallbackSource_Fighter) && engine->item_creator(gobj) == recipient%s) { %sscript_fn_%s(recipient",
        creator ? "" : " && item->owner == recipient",
        expression ? "recipient_result = " : "", fn->name);
    for (unsigned a = 0; a < fn->argument_count; ++a) {
        if (ordered) APPEND(", recipient_arg_%u", a);
        else APPEND(", %s", values[a]);
    }
    APPEND("); ");
    if (expression)
        APPEND("if (!engine->callback_source_alive(recipient, recipient_generation, NativeCallbackSource_Fighter) || engine->item_creator(gobj) != recipient%s) recipient_result = %s; ",
            creator ? "" : " || item->owner != recipient", neutral);
    APPEND(expression ? "} } recipient_result; })" : "} } }");
#undef APPEND
    *after = end;
    return 1;
}

void nativeItemFunctions_Emit(const ArticleDeclaration* article, FILE* output)
{
    if (!article->item_functions) return;
    for (unsigned pass = 0; pass < 2; ++pass)
        for (unsigned i = 0; i < article->item_functions->count; ++i) {
            const NativeItemFunction* function = &article->item_functions->functions[i];
            fprintf(output, "static %s scripted_article_%u_%s%shelper_%u(HSD_GObj* gobj",
                function->return_type == ScriptType_Invalid ? "void" : nativeScriptTypes_Name(function->return_type),
                article->index, function->may_invalidate_source ? "mutating_" : "",
                function->may_invalidate_source && function->return_type != ScriptType_Invalid ? "value_" : "", i);
            for (unsigned a = 0; a < function->argument_count; ++a)
                fprintf(output, ", %s local_%s", nativeScriptTypes_Name(function->argument_types[a]),
                    function->argument_names[a]);
            if (!pass) fputs(");\n", output);
            else {
                fputs(") {\n    Item* item = gobj->user_data;\n", output);
                nativeItemStatements_Emit(output, &function->body, article->functions, false);
                fputs("}\n", output);
            }
        }
}
