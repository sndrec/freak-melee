#include "../../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned programs;
static const char* routes[] = {
    "action Policy:\n    on tick:\n        items.reject_healing_pickup(true)\n        family.retire_companion_on_sleep(false)\n",
    "move Policy:\n    on state_341_physics:\n        items.reject_healing_pickup(true)\n        family.retire_companion_on_sleep(false)\n",
};
static char* generate(const char* source, bool accepted, const NativeFighterScriptConstruction* construction)
{
    FILE* file = tmpfile(); assert(file);
    NativeFighterScriptError error = {0}; ++programs;
    bool ok = nativeFighterScript_GenerateC(source, strlen(source), "common_init_policy", NULL, NULL, construction, file, &error);
    if (ok != accepted) fprintf(stderr, "Unexpected %s: line%u %s\n%s", ok ? "accept" : "reject", error.line, error.message, source);
    assert(ok == accepted);
    if (!ok) { assert(error.line); fclose(file); return NULL; }
    fseek(file, 0, SEEK_END); long size = ftell(file); rewind(file);
    char* code = malloc(size + 1); assert(code);
    assert(fread(code, 1, size, file) == (size_t)size); code[size] = 0;
    fclose(file); return code;
}
int main(int argc, char** argv)
{
    assert(argc == 1 || argc == 3);
    const char policy[] = "surface_friction ignore\nsound_remap 0xFFFFFFFC 13 0x1FBFD 0x1FC62 0x66\nsound_remap 0x3 13 0x1FC63 0x1FCC8 -0x66\nsound_ko_cue 0xFFFFFFFC 0x1FBD1\n";
    for (unsigned route = 0; route < 2; ++route) {
        char source[4096]; snprintf(source, sizeof(source), "%s%s", policy, routes[route]);
        char* code = generate(source, true, NULL);
        assert(strstr(code, "generated.ignore_surface_friction = true"));
        assert(strstr(code, "generated.sound_policy = &authored_sound_policy"));
        assert(strstr(code, "fighter->x2222_b4 = (true)"));
        assert(strstr(code, "fighter->x2222_b5 = (false)"));
        if (argc == 3) { FILE* out = fopen(argv[route + 1], "wb"); assert(out); assert(fwrite(code, 1, strlen(code), out) == strlen(code)); assert(fclose(out) == 0); }
        free(code);
        snprintf(source, sizeof(source), "surface_friction stage # comment\n%s", routes[route]);
        code = generate(source, true, NULL); assert(strstr(code, "generated.ignore_surface_friction = false")); free(code);
        code = generate(routes[route], true, NULL); assert(!strstr(code, "generated.ignore_surface_friction =")); assert(!strstr(code, "authored_sound_policy")); free(code);
        const char* invalid[] = {
            "surface_friction yes\n", "surface_friction ignore more\n", "surface_friction ignore\nsurface_friction stage\n", "    surface_friction ignore\n",
            "sound_remap 0 13 1 2 1\n", "sound_remap -1 13 1 2 1\n", "sound_remap 0x100000000 13 1 2 1\n", "sound_remap 1 64 1 2 1\n", "sound_remap 1 13 2 1 1\n", "sound_remap 1 13 0 1 -1\n", "sound_remap 1 13 1 2147483647 1\n", "sound_remap 1 13 1 2 1.0\n", "sound_remap 1 13 1 2 1 extra\n", "sound_remap 1 13 1 2\n", "sound_ko_cue 0 1\n", "sound_ko_cue 1 -1\n", "sound_ko_cue 1 1\nsound_ko_cue 1 2\n",
        };
        for (unsigned i = 0; i < sizeof(invalid)/sizeof(*invalid); ++i) {
            snprintf(source, sizeof(source), "%s%s", invalid[i], routes[route]); generate(source, false, NULL);
        }
        snprintf(source, sizeof(source), "%ssurface_friction ignore\n", routes[route]); generate(source, false, NULL);
        snprintf(source, sizeof(source), "%ssound_remap 1 13 1 2 1\n", routes[route]); generate(source, false, NULL);
        snprintf(source, sizeof(source), "%ssound_ko_cue 1 1\n", routes[route]); generate(source, false, NULL);
        source[0] = 0;
        for (unsigned i = 0; i < 16; ++i) strcat(source, "sound_remap 1 13 1 2 0\n");
        strcat(source, routes[route]); code = generate(source, true, NULL); free(code);
        source[0] = 0;
        for (unsigned i = 0; i < 17; ++i) strcat(source, "sound_remap 1 13 1 2 0\n");
        strcat(source, routes[route]); generate(source, false, NULL);
        source[0] = 0;
        for (unsigned i = 0; i < 9; ++i) { char cue[64]; snprintf(cue, sizeof(cue), "sound_ko_cue %u 1\n", 1u << i); strcat(source, cue); }
        strcat(source, routes[route]); generate(source, false, NULL);
        for (unsigned field = 0; field < 2; ++field) {
            snprintf(source, sizeof(source), "%s    on load:\n        %s(1)\n", routes[route], field ? "family.retire_companion_on_sleep" : "items.reject_healing_pickup"); generate(source, false, NULL);
        }
    }
    printf("PASS %u common-init compiler programs, both routes and strict invalid declarations/types\n", programs);
    return 0;
}
