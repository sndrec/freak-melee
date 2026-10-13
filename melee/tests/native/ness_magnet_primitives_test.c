#include "../../tools/native_fighter_script.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, failures;

static void check(bool valid, const char* message)
{
    ++checks;
    if (!valid) {
        ++failures;
        fprintf(stderr, "%s\n", message);
    }
}

static bool generate(const char* body, bool states, char* output, size_t capacity)
{
    char script[8192];
    snprintf(script, sizeof(script),
        "%sstate int joint\n%s%s",
        states ? "" : "",
        states ? "move Probe:\n    on state_367_physics:\n"
               : "action Probe:\n    on load:\n", body);
    NativeFighterScriptError error = { 0 };
    FILE* file = tmpfile();
    if (file == NULL) return false;
    bool valid = nativeFighterScript_GenerateC(
        script, strlen(script), "absorber_probe", NULL, NULL, states ? &(const NativeFighterScriptConstruction){"ftDataNess",341,36} : NULL, file, &error);
    if (valid && output != NULL) {
        rewind(file);
        size_t size = fread(output, 1, capacity - 1, file);
        output[size] = '\0';
        valid = !ferror(file) && size < capacity - 1;
    }
    fclose(file);
    if (!valid && output != NULL)
        fprintf(stderr, "line %u: %s\n", error.line, error.message);
    return valid;
}

int main(void)
{
    static const char body[] =
        "        state.joint = 23\n"
        "        absorber.create(state.joint, 1.25, -2, 0, 3)\n"
        "        absorber.active = absorber.hit\n"
        "        absorber.hit = false\n"
        "        absorber.skip_position_update = false\n"
        "        facing.direction = absorber.direction\n"
        "        damage.heal(float(int(absorber.damage * 3)))\n"
        "        effects.spawn_async_attached(1264, state.joint)\n"
        "        effects.spawn_async_attached(1264, TransN)\n";
    static const char* required[] = {
        "fighter->x2218_b6 = fighter->x2218_b7;",
        "fighter->x2218_b7 = false;",
        "fighter->absorb_hit.skip_update_pos = false;",
        "fighter->AbsorbAttr.x1A40_absorbHitDirection",
        "fighter->AbsorbAttr.x1A44_damageTaken",
        "engine->create_absorber(gobj, &(AbsorbDesc){ bone,",
        "bone >= 0 && (unsigned)bone < engine->bone_count(fighter)",
        "fighter->parts[bone].joint != NULL",
        "engine->spawn_async_effect(gobj, &fighter->x60C, 0,",
        "fighter->parts[bone].joint",
        "engine->resolve_bone(fighter, FtPart_TransN)",
        "host->create_absorber == 0",
        "host->heal_damage == 0",
        "host->bone_count == 0",
        "host->spawn_async_effect == 0",
    };
    for (unsigned state = 0; state < 2; ++state) {
        char generated[65536];
        bool valid = generate(body, state != 0, generated, sizeof(generated));
        check(valid, "generic absorber script must compile in both forms");
        if (!valid) continue;
        for (unsigned i = 0; i < sizeof(required) / sizeof(required[0]); ++i)
            check(strstr(generated, required[i]) != NULL, required[i]);
    }
    static const char* invalid[] = {
        "        absorber.damage = 1\n",
        "        absorber.direction = 1\n",
        "        absorber.create(23, 0, 0, 0)\n",
        "        absorber.create(23, 0, 0, 0, 1, 2)\n",
        "        absorber.create(UnknownJoint, 0, 0, 0, 1)\n",
        "        effects.spawn_async_attached(1264, UnknownJoint)\n",
        "        effects.spawn_async_attached(1264, 23, 0)\n",
        "        damage.heal()\n",
        "        damage.heal(1, 2)\n",
    };
    for (unsigned state = 0; state < 2; ++state)
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
            check(!generate(invalid[i], state != 0, NULL, 0), invalid[i]);
    printf("Generic absorber compiler: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
