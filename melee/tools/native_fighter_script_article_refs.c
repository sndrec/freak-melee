#include "native_fighter_script_article_refs.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

enum { CATALOG_LINE_BYTES = 1024, ARTICLE_MOTION_STATES = 32 };

static bool fail(NativeFighterScriptError* error, unsigned line,
                 const char* message)
{
    error->line = line;
    snprintf(error->message, sizeof(error->message), "%s", message);
    return false;
}

static bool identifier(const char* text, size_t length, size_t capacity)
{
    if (!length || length >= capacity ||
        (!isalpha((unsigned char)*text) && *text != '_')) return false;
    for (size_t i = 1; i < length; ++i)
        if (!isalnum((unsigned char)text[i]) && text[i] != '_') return false;
    return true;
}

const NativeScriptArticleDeclaration* nativeScriptArticleRefs_Declaration(
    const NativeScriptArticleCatalog* catalog, const char* kind)
{
    if (catalog == NULL || kind == NULL) return NULL;
    for (unsigned i = 0; i < catalog->count; ++i)
        if (!strcmp(catalog->declarations[i].kind, kind))
            return &catalog->declarations[i];
    return NULL;
}

bool nativeScriptArticleRefs_CatalogParse(NativeScriptArticleCatalog* catalog,
                                         const char* source, size_t size,
                                         NativeFighterScriptError* error)
{
    if (!catalog || !source || !error ||
        size > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY) return false;
    memset(catalog, 0, sizeof(*catalog));
    *error = (NativeFighterScriptError){ 0 };
    NativeScriptArticleDeclaration* article = NULL;
    unsigned line_number = 0;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        size_t length = end - start;
        ++line_number;
        if (length && source[start + length - 1] == '\r') --length;
        if (length >= CATALOG_LINE_BYTES)
            return fail(error, line_number, "Article catalog line is too long.");
        char line[CATALOG_LINE_BYTES];
        memcpy(line, source + start, length); line[length] = 0;
        while (length && line[length - 1] == ' ') line[--length] = 0;
        start = end < size ? end + 1 : end;
        if (!length || line[0] == '#') continue;
        if (line[0] != ' ') article = NULL;
        if (!strncmp(line, "article ", 8)) {
            char kind[64]; unsigned states; int consumed = 0;
            if (sscanf(line, "article %63s states %u:%n", kind, &states,
                       &consumed) != 2 || !consumed || line[consumed] ||
                strncmp(kind, "It_Kind_", 8) ||
                !identifier(kind, strlen(kind), sizeof(kind)) ||
                !states || states > ARTICLE_MOTION_STATES)
                return fail(error, line_number,
                            "Expected `article It_Kind_Name states N:`.");
            if (catalog->count == SCRIPT_ARTICLE_DECLARATIONS)
                return fail(error, line_number, "At most 16 articles may be declared.");
            if (nativeScriptArticleRefs_Declaration(catalog, kind))
                return fail(error, line_number, "Article kind is declared twice.");
            article = &catalog->declarations[catalog->count++];
            strcpy(article->kind, kind); article->motion_states = states;
            article->state.bytes_limit = SCRIPT_ARTICLE_STATE_BYTES;
        } else if (article && (!strncmp(line, "    state bool ", 15) ||
                              !strncmp(line, "    state int ", 14) ||
                              !strncmp(line, "    state float ", 16))) {
            if (!nativeFighterScript_AddStateField(&article->state, line + 4))
                return fail(error, line_number,
                    "Invalid article state: unique typed fields, at most 64 fields/512 bytes.");
        }
    }
    return true;
}

static void spaces(const char** cursor)
{
    while (isspace((unsigned char)**cursor)) ++*cursor;
}

static bool copy_expression(const char* begin, const char* end, char* output)
{
    while (begin < end && isspace((unsigned char)*begin)) ++begin;
    while (end > begin && isspace((unsigned char)end[-1])) --end;
    size_t length = (size_t)(end - begin);
    if (!length || length >= SCRIPT_ARTICLE_REF_EXPRESSION) return false;
    memcpy(output, begin, length); output[length] = 0;
    return true;
}

// Find a delimiter outside nested calls/indices. Typed expression translation
// handles operators and atoms; this parser owns only the reference grammar.
static const char* delimiter(const char* cursor, char stop)
{
    char closings[32]; unsigned depth = 0;
    for (; *cursor; ++cursor) {
        char c = *cursor;
        if (!depth && c == stop) return cursor;
        if (c == '(' || c == '[') {
            if (depth == sizeof(closings)) return NULL;
            closings[depth++] = c == '(' ? ')' : ']';
        } else if (c == ')' || c == ']') {
            if (!depth || closings[--depth] != c) return NULL;
        } else if (c == ',' && !depth) return NULL;
    }
    return NULL;
}

static bool property_end(char c)
{
    return !isalnum((unsigned char)c) && c != '_' && c != '.' && c != '[';
}

int nativeScriptArticleRefs_Reference(const NativeScriptArticleCatalog* catalog,
                                      const char* source,
                                      NativeScriptArticleReference* reference)
{
    static const char prefix[] = "articles.ref(";
    if (!source || strncmp(source, prefix, sizeof(prefix) - 1)) return 0;
    if (!catalog || !reference) return -1;
    *reference = (NativeScriptArticleReference){ 0 };
    const char* begin = source + sizeof(prefix) - 1;
    const char* cursor = delimiter(begin, ',');
    if (!cursor || !copy_expression(begin, cursor, reference->handle)) return -1;
    ++cursor; spaces(&cursor); begin = cursor;
    while (isalnum((unsigned char)*cursor) || *cursor == '_') ++cursor;
    size_t length = (size_t)(cursor - begin);
    char kind[64];
    if (!identifier(begin, length, sizeof(kind))) return -1;
    memcpy(kind, begin, length); kind[length] = 0;
    reference->declaration = nativeScriptArticleRefs_Declaration(catalog, kind);
    if (!reference->declaration) return -1;
    spaces(&cursor);
    if (strncmp(cursor, ").", 2)) return -1;
    cursor += 2;
    if (!strncmp(cursor, "exists", 6) && property_end(cursor[6])) {
        reference->type = ScriptState_Bool;
        reference->after = cursor + 6;
        return 1;
    }
    if (!strncmp(cursor, "current_owner", 13) && property_end(cursor[13])) {
        reference->type = ScriptState_Int;
        reference->current_owner = true;
        reference->after = cursor + 13;
        return 1;
    }
    if (strncmp(cursor, "state.", 6)) return -1;
    cursor += 6; begin = cursor;
    while (isalnum((unsigned char)*cursor) || *cursor == '_') ++cursor;
    length = (size_t)(cursor - begin);
    char name[32];
    if (!identifier(begin, length, sizeof(name))) return -1;
    memcpy(name, begin, length); name[length] = 0;
    const ScriptStateTable* table = &reference->declaration->state;
    reference->field = nativeFighterScript_StateField(table, name);
    if (!reference->field) return -1;
    reference->field_index = (unsigned)(reference->field - table->fields);
    reference->type = reference->field->type;
    if (reference->field->array) {
        spaces(&cursor);
        if (*cursor++ != '[') return -1;
        begin = cursor; cursor = delimiter(begin, ']');
        if (!cursor || !copy_expression(begin, cursor, reference->index)) return -1;
        ++cursor;
    } else {
        const char* next = cursor; spaces(&next);
        if (*next == '[') return -1;
        strcpy(reference->index, "0");
    }
    if (!property_end(*cursor)) return -1;
    reference->after = cursor;
    return 1;
}

bool nativeScriptArticleRefs_Emit(const NativeScriptArticleReference* reference,
                                 const char* handle, const char* index,
                                 char* output, size_t capacity)
{
    if (!reference || !reference->declaration || !handle || !*handle ||
        !output || !capacity) return false;
    int written;
    if (!reference->field) {
        written = snprintf(output, capacity,
            "engine->article_ref_%s(gobj, (uint32_t)(%s), %s)",
            reference->current_owner ? "current_owner" : "exists",
            handle, reference->declaration->kind);
    } else {
        if (!index || !*index) return false;
        const char* type = reference->type == ScriptState_Bool ? "bool" :
                           reference->type == ScriptState_Int ? "int" : "float";
        written = snprintf(output, capacity,
            "engine->article_ref_%s(gobj, (uint32_t)(%s), %s, %u, (int32_t)(%s))",
            type, handle, reference->declaration->kind,
            reference->field_index, index);
    }
    return written >= 0 && (size_t)written < capacity;
}
