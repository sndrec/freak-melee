#include "../../tools/native_fighter_script.h"
#include "../../tools/native_fighter_script_guard.h"
#include "../../tools/native_fighter_script_types.h"

#undef NDEBUG
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void compact(char* text)
{
    char* write = text;
    for (char* read = text; *read; ++read)
        if (!isspace((unsigned char) *read)) *write++ = *read;
    *write = 0;
}

static const char source[] =
    "state bool front_seen\n"
    "action GuardFilter:\n"
    "    on tick:\n"
    "        guard.front_only = true\n"
    "        guard.reactive = false\n"
    "        guard.active = true\n"
    "        callbacks.track_damage()\n"
    "        let bool front = guard.front_only\n"
    "        state.front_seen = front\n"
    "    on damage:\n"
    "        guard.front_only = false\n";

static bool generate(const char* text, FILE* output, bool diagnose)
{
    NativeFighterScriptError error = {0};
    bool good = nativeFighterScript_GenerateC(text, strlen(text),
        "guard_front_filter", NULL, NULL, NULL, output, &error);
    if (!good && diagnose)
        fprintf(stderr, "guard front filter line %u: %s\n", error.line, error.message);
    return good;
}

int main(int argc, char** argv)
{
    assert(nativeScriptTypes_Builtin("guard.front_only") == ScriptType_Bool);
    assert(!strcmp(nativeScriptGuard_Property("guard.front_only"), "fighter->x221B_b3"));
    assert(nativeScriptGuard_Property("guard.front_only.extra") == NULL);
    assert(nativeScriptGuard_Property("guard.front_only_filter") == NULL);
    ScriptCodeBlock block = {0}; char action[1024];
    assert(nativeScriptGuard_Action(&block, "guard.front_only = true", action, sizeof(action)));
    compact(action);
    assert(strstr(action, "fighter->x221B_b3=(true)"));
    assert(!strstr(action, "x221B_b0") && !strstr(action, "x221B_b1"));
    assert(!nativeScriptGuard_Action(&block, "guard.front_only.extra = true", action, sizeof(action)));

    FILE* file = tmpfile(); assert(file); assert(generate(source, file, true));
    assert(!fseek(file, 0, SEEK_END)); long size = ftell(file); assert(size > 0); rewind(file);
    char* code = malloc((size_t) size + 1); assert(code);
    assert(fread(code, 1, (size_t) size, file) == (size_t) size); code[size] = 0;
    assert(strstr(code, "bool local_front"));
    assert(strstr(code, "generated.on_tick ="));
    assert(strstr(code, "fighter->take_dmg_cb = authored_damage"));
    fclose(file);
    if (argc == 2) {
        file = fopen(argv[1], "wb"); assert(file);
        assert(fwrite(code, 1, (size_t) size, file) == (size_t) size); fclose(file);
    } else assert(argc == 1);
    compact(code);
    assert(strstr(code, "fighter->x221B_b3=(true)"));
    assert(strstr(code, "fighter->x221B_b3=(false)"));
    assert(strstr(code, "fighter->x221B_b0=true"));
    assert(strstr(code, "fighter->x221B_b1=false"));
    assert(!strstr(code, "fighter->x221B_b2="));
    free(code);

    const char* invalid[] = {
        "action Invalid:\n    on tick:\n        guard.front_only = missing_value\n",
        "action Invalid:\n    on tick:\n        guard.front_only.extra = true\n",
        "action Invalid:\n    on tick:\n        guard.front_only_filter = true\n",
        "action Invalid:\n    on tick:\n        let bool x = guard.front_only()\n",
        "article It_Kind_Link_HShot states 1:\n    state 0 anim 0:\n        on physics:\n            guard.front_only = true\n",
    };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        file = tmpfile(); assert(file);
        bool accepted = generate(invalid[i], file, false);
        if (accepted) fprintf(stderr, "Unexpected accepted guard negative %u:\n%s", i, invalid[i]);
        assert(!accepted); fclose(file);
    }
    puts("Guard front filter Bool reads/writes, distinct bits, full generator and negative contexts PASS");
    return 0;
}
