#include "../../tools/native_fighter_script.h"
#undef NDEBUG
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static bool generate(const char* source, char** text, const NativeFighterScriptConstruction* construction)
{
    FILE* output = tmpfile(); assert(output);
    NativeFighterScriptError error = {0};
    bool accepted = nativeFighterScript_GenerateC(source, strlen(source),
        "cpu_defense_probe", NULL, NULL, construction, output, &error);
    if (accepted && text) {
        assert(!fseek(output, 0, SEEK_END)); long size = ftell(output);
        assert(size > 0 && !fseek(output, 0, SEEK_SET));
        *text = malloc((size_t)size + 1); assert(*text);
        assert(fread(*text, 1, size, output) == (size_t)size); (*text)[size] = 0;
    }
    fclose(output); checks++; return accepted;
}

int main(void)
{
    for (unsigned states = 0; states < 2; states++) {
        char source[2048];
        snprintf(source, sizeof(source), "%s"
            "func Response(int category):\n"
            "    if category == 2: cpu.queue(Cpu.press_b)\n"
            "    else: cpu.queue(Cpu.press_r)\n"
            "%s Defense:\n"
            "    on cpu_projectile_defense:\n"
            "        let int category = event.category\n"
            "        Response(category)\n"
            "        return category == 2\n",
            states ? "" : "", states ? "move" : "action");
        char* code = NULL; assert(generate(source, &code, states ? &(const NativeFighterScriptConstruction){"ftDataFox",341,1} : NULL));
        assert(strstr(code, "HSD_GObj* gobj, int projectile_category)"));
        assert(strstr(code, "local_category = projectile_category"));
        assert(strstr(code, "generated.on_cpu_projectile_defense = authored_"));
        assert(strstr(code, "host->cpu_command == 0"));
        free(code);
        snprintf(source, sizeof(source), "%s%s Wrong:\n"
            "    on load:\n        let int category = event.category\n",
            states ? "" : "", states ? "move" : "action");
        assert(!generate(source, NULL, &(const NativeFighterScriptConstruction){"ftDataFox",341,1}));
        snprintf(source, sizeof(source), "%s%s Wrong:\n"
            "    on cpu_projectile_defense:\n        event.category = 2\n",
            states ? "" : "", states ? "move" : "action");
        // The category is input context, never a writable engine field.
        assert(!generate(source, NULL, &(const NativeFighterScriptConstruction){"ftDataFox",341,1}));
    }
    assert(!generate("func ContextLeak():\n    let int category = event.category\n"
                     "action Probe:\n    on cpu_projectile_defense:\n"
                     "        ContextLeak()\n        return false\n", NULL, &(const NativeFighterScriptConstruction){"ftDataFox",341,1}));
    printf("CPU projectile-defense compiler checks passed: %u\n", checks);
    return 0;
}
