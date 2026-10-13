#include "native_fighter_item_tables.h"
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

bool nativeScriptItemTable_Parse(ScriptItemTable* table, const char* source)
{
    static const char prefix[] = "    table float ";
    if (strncmp(source, prefix, sizeof(prefix) - 1) != 0) return false;
    source += sizeof(prefix) - 1;
    unsigned used = 0;
    if (!isalpha((unsigned char) *source) && *source != '_') return false;
    while (isalnum((unsigned char) *source) || *source == '_') {
        if (used + 1 >= sizeof(table->name)) return false;
        table->name[used++] = *source++;
    }
    table->name[used] = '\0';
    if (strncmp(source, " = [", 4) != 0) return false;
    source += 4;
    table->count = 0;
    for (;;) {
        while (*source == ' ') ++source;
        if (table->count == SCRIPT_ITEM_TABLE_VALUES ||
            (!isdigit((unsigned char) *source) && *source != '-' &&
             *source != '+' && *source != '.')) return false;
        errno = 0;
        char* end;
        float value = strtof(source, &end);
        if (end == source || errno || !isfinite(value)) return false;
        table->values[table->count++] = value;
        source = end;
        while (*source == ' ') ++source;
        if (*source == ']') return source[1] == '\0';
        if (*source++ != ',') return false;
    }
}

void nativeScriptItemTable_Emit(const ScriptItemTable* table, unsigned article,
                               unsigned index, FILE* output)
{
    fprintf(output, "static const float scripted_article_%u_table_%u[] = {",
            article, index);
    for (unsigned i = 0; i < table->count; ++i)
        fprintf(output, " %.9ef,", (double) table->values[i]);
    fputs(" };\n", output);
    // Evaluate the index once and keep malformed script state out of memory.
    fprintf(output,
        "static inline float scripted_article_%u_lookup_%u(int32_t index) {\n"
        "    return (uint32_t) index < %u ? scripted_article_%u_table_%u[index] : 0.0f;\n"
        "}\n", article, index, table->count, article, index);
}
