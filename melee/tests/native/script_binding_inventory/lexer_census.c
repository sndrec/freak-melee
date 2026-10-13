#include "native_script_expression.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SOURCE_BYTES = 1024 * 1024, LINE_TOKENS = 1024 };
static char source[SOURCE_BYTES];
static NativeScriptExpressionToken tokens[LINE_TOKENS];

static void json_text(const char* text, unsigned length)
{
    putchar('"');
    for (unsigned i = 0; i < length; ++i) {
        unsigned char c = (unsigned char)text[i];
        if (c == '"' || c == '\\') printf("\\%c", c);
        else if (c < 32) printf("\\u%04x", c);
        else putchar(c);
    }
    putchar('"');
}

// Inspect one source line at a time so declaration grammar (including helper
// return arrows) cannot hide token records for later executable statements.
int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    FILE* input = fopen(argv[1], "rb");
    if (!input) return 3;
    unsigned bytes = (unsigned)fread(source, 1, sizeof(source), input);
    bool oversized = !feof(input);
    fclose(input);
    if (oversized) return 4;
    bool comma = false;
    printf("{\"tokens\":[");
    for (unsigned offset = 0, line = 1; offset < bytes; ++line) {
        unsigned end = offset;
        while (end < bytes && source[end] != '\n') ++end;
        unsigned count = 0;
        NativeScriptDiagnostic diagnostic = {0};
        NativeScriptSourceSpan span = {offset, end-offset, line, 1};
        if (nativeScriptExpression_Tokenize(source, bytes, span, tokens,
                LINE_TOKENS, &count, &diagnostic)) {
            for (unsigned i = 0; i < count; ++i) {
                const NativeScriptExpressionToken* token = &tokens[i];
                if (token->kind == NativeScriptToken_End) continue;
                if (comma) putchar(',');
                comma = true;
                printf("{\"line\":%u,\"column\":%u,\"kind\":\"%s\",\"text\":",
                    line, token->source.column,
                    token->kind == NativeScriptToken_Name ? "name" : "other");
                json_text(source + token->source.offset, token->source.length);
                putchar('}');
            }
        }
        offset = end < bytes ? end+1 : end;
    }
    printf("],\"errors\":[");
    comma = false;
    for (unsigned offset = 0, line = 1; offset < bytes; ++line) {
        unsigned end = offset;
        while (end < bytes && source[end] != '\n') ++end;
        unsigned count = 0;
        NativeScriptDiagnostic diagnostic = {0};
        NativeScriptSourceSpan span = {offset, end-offset, line, 1};
        if (!nativeScriptExpression_Tokenize(source, bytes, span, tokens,
                LINE_TOKENS, &count, &diagnostic)) {
            if (comma) putchar(',');
            comma = true;
            printf("{\"line\":%u,\"error\":", line);
            json_text(diagnostic.message, (unsigned)strlen(diagnostic.message));
            putchar('}');
        }
        offset = end < bytes ? end+1 : end;
    }
    puts("]}");
    return 0;
}
