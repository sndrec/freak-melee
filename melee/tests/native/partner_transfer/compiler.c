#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned programs;
static void generate(const char* source, bool expected, const char* path, const NativeFighterScriptConstruction* construction)
{
    FILE* output = path ? fopen(path, "w") : tmpfile(); assert(output);
    NativeFighterScriptError error = {0}; ++programs;
    bool ok = nativeFighterScript_GenerateC(source, strlen(source), "partner_transfer",
        NULL, NULL, construction, output, &error);
    if (ok != expected) fprintf(stderr, "Unexpected %s at %u: %s\n%s\n",
        ok ? "accept" : "reject", error.line, error.message, source);
    assert(ok == expected); if (!ok) assert(error.line); fclose(output);
}

int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    const char* invalid[] = {
        "forms.activate_partner(1)", "forms.activate_partner(false)",
        "forms.activate_partner() + 1", "forms.activate_partner",
        "let float result = forms.activate_partner(0)",
    };
    for (unsigned route = 0; route < 2; ++route) {
        char source[2048];
        const char* prefix = route ? "" : "";
        const char* entry = route ? "move Transfer:\n    on state_341_physics:\n" :
            "action Transfer:\n    on load:\n";
        snprintf(source, sizeof(source), "%sstate bool switched\nstate int after\n"
            "state int activated\n%s        state.switched = forms.activate_partner()\n"
            "        state.after = 99\n%s    on form_activated:\n        state.activated = 77\n",
            prefix, entry, route ? "move Arrival:\n" : "action Arrival:\n");
        generate(source, true, argc == 3 ? argv[route + 1] : NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        snprintf(source, sizeof(source), "%s%s        forms.activate_partner()\n", prefix, entry);
        generate(source, true, NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
            snprintf(source, sizeof(source), "%s%s        %s\n", prefix, entry, invalid[i]);
            generate(source, false, NULL, route ? &(const NativeFighterScriptConstruction){"ftDataTest",341,1} : NULL);
        }
    }
    generate("article It_Kind_Link_HShot states 1:\n    state 0 anim -1:\n"
             "        on physics:\n            forms.activate_partner()\n", false, NULL, NULL);
    generate("article It_Kind_Link_HShot states 1:\n    state bool switched\n"
             "    state 0 anim -1:\n        on physics:\n"
             "            item.state.switched = forms.activate_partner()\n", false, NULL, NULL);
    printf("PASS %u partner transfer compiler programs in both generator routes\n", programs);
    return 0;
}
