#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
static bool generate(const char* call, bool callbacks, const char* ending)
{
    char source[4096];
    snprintf(source, sizeof(source), "%sstate int created\n%s Probe:\n    on load:\n"
        "        state.created = articles.create_with_velocity(It_Kind_Ness_Bat, 1,2,3,4,5,6, -1, false, int(0), 7%s)\n",
        callbacks ? "" : "", callbacks ? "move" : "action", call);
    NativeFighterScriptError error = { 0 };
    FILE* file = tmpfile(); assert(file);
    bool result = nativeFighterScript_GenerateC(source, strlen(source),
        "article_collision_probe", NULL, NULL, callbacks ? &(const NativeFighterScriptConstruction){"ftDataNess",341,1} : NULL, file, &error);
    ++checks;
    if (result && ending) {
        char code[65536]; rewind(file);
        size_t count = fread(code, 1, sizeof(code) - 1, file);
        assert(feof(file)); code[count] = 0;
        assert(strstr(code, "engine->create_article_at(gobj"));
        assert(strstr(code, ending)); ++checks;
    }
    fclose(file); return result;
}

int main(void)
{
    for (unsigned callbacks = 0; callbacks < 2; ++callbacks) {
        assert(!generate("", callbacks, NULL));
        assert(generate(", true", callbacks, ", true)"));
        assert(generate(", false", callbacks, ", false)"));
        assert(!generate(", 0", callbacks, NULL));
        assert(!generate(", 1.5", callbacks, NULL));
        assert(!generate(", false, true", callbacks, NULL));
    }
    printf("Initial article collision compiler checks: %u\n", checks);
    return 0;
}
