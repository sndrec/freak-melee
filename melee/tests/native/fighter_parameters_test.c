#include <melee/ft/native_fighter_parameters.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int parse(const char* source, NativeFighterParameters* fields)
{
    unsigned line = 0;
    return nativeFighterParameters_Parse(source, strlen(source), fields, &line);
}

int main(void)
{
    NativeFighterParameters first, changed, schema;
    if (!parse("# Per-fighter values\nfloat launch = 1.25\n"
               "int uses = -2\nbool ready = true\n", &first) ||
        first.count != 3 || first.fields[0].bits != UINT32_C(0x3fa00000) ||
        first.fields[1].bits != UINT32_C(0xfffffffe) ||
        first.fields[2].bits != 1 ||
        !parse("float launch = 2.5\nint uses = 9\n"
               "bool ready = false\n", &changed) ||
        changed.schema_hash != first.schema_hash ||
        !parse("float launch = 2.5\nfloat uses = 9\n"
               "bool ready = false\n", &schema) ||
        schema.schema_hash == first.schema_hash ||
        parse("float launch = 1\nint launch = 2\n", &schema) ||
        parse("float bad = nan\n", &schema) ||
        parse("bool ready = 1\n", &schema) ||
        parse("float .speed = 1\n", &schema) ||
        parse("float Article..speed = 1\n", &schema) ||
        parse("float Article.speed. = 1\n", &schema) ||
        !parse("float A.speed = 1\nfloat B.speed = 2\n", &schema) ||
        schema.count != 2) {
        fputs("Fighter parameter parsing or schema failed.\n", stderr);
        return 1;
    }
    puts("Fighter parameters passed");
    return 0;
}
