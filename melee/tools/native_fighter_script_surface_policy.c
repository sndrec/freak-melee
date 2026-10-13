#include "native_fighter_script_surface_policy.h"
#include "native_fighter_script_code.h"
#include <ctype.h>
#include <string.h>

bool nativeScriptSurfacePolicy_Parse(char* source, size_t size,
                                     ScriptSurfacePolicy* policy,
                                     NativeFighterScriptError* error)
{
    *policy = (ScriptSurfacePolicy){0};
    unsigned line = 0;
    bool code_seen = false;
    for (size_t start = 0; start < size;) {
        size_t end = start;
        while (end < size && source[end] != '\n') ++end;
        ++line;
        size_t at = start;
        while (at < end && (source[at] == ' ' || source[at] == '\t')) ++at;
        const char* text = source + at;
        const size_t length = end - at;
        const char word[] = "surface_friction";
        const size_t prefix = sizeof(word) - 1;
        if (length >= prefix && !memcmp(text, word, prefix) &&
            (length == prefix || (!isalnum((unsigned char)text[prefix]) && text[prefix] != '_'))) {
            if (at != start || code_seen || policy->declared)
                return nativeScriptCode_Fail(error, line,
                    "surface_friction is one top-level declaration before code blocks.");
            size_t value = prefix;
            if (value == length || !isspace((unsigned char)text[value]))
                return nativeScriptCode_Fail(error, line, "Expected surface_friction stage or ignore.");
            while (value < length && isspace((unsigned char)text[value])) ++value;
            const size_t begin = value;
            while (value < length && isalpha((unsigned char)text[value])) ++value;
            const size_t n = value - begin;
            const bool stage = n == 5 && !memcmp(text + begin, "stage", 5);
            const bool ignore = n == 6 && !memcmp(text + begin, "ignore", 6);
            while (value < length && isspace((unsigned char)text[value])) ++value;
            if ((!stage && !ignore) || (value < length && text[value] != '#'))
                return nativeScriptCode_Fail(error, line, "Expected surface_friction stage or ignore.");
            policy->declared = true; policy->ignore = ignore;
            memset(source + start, ' ', end - start);
        } else if (at == start && length) {
            static const char* blocks[] = {"on ", "action ", "move ", "func ", "export func ", "article ", "item ", "model_effect "};
            for (unsigned i = 0; i < sizeof(blocks)/sizeof(blocks[0]); ++i)
                code_seen |= length >= strlen(blocks[i]) && !memcmp(text, blocks[i], strlen(blocks[i]));
        }
        start = end < size ? end + 1 : end;
    }
    return true;
}

void nativeScriptSurfacePolicy_EmitDefinition(const ScriptSurfacePolicy* policy,
                                             FILE* output)
{
    if (policy != NULL && policy->declared)
        fprintf(output, "    generated.ignore_surface_friction = %s;\n",
                policy->ignore ? "true" : "false");
}
