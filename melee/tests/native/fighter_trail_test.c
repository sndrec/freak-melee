#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <sysdolphin/baselib/gobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterSetter)(const uint32_t*, unsigned, uint64_t);

int main(int argc, char** argv)
{
    if (argc != 4) return 2;
    char text[4096];
    FILE* file = fopen(argv[3], "rb");
    size_t size = file ? fread(text, 1, sizeof(text), file) : 0;
    if (file) fclose(file);
    NativeFighterParameters parameters;
    if (!size || !nativeFighterParameters_Parse(text, size, &parameters, NULL))
        return 1;
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    Fighter* fighter = calloc(1, sizeof(*fighter));
    Fighter* before = malloc(sizeof(*fighter));
    if (!fighter || !before) return 1;
    fighter->facing_dir = -1;
    HSD_GObj object = { .user_data = fighter };
    MotionState motion = { 0 };
    NativeFighterDefinition base = {
        .fighter_root = "ftDataTest", .motion_states = &motion,
        .motion_state_first = 341, .motion_state_count = 1, .joint_count = 50,
    };
    NativeFighterHost host = { 0 }; // Descriptor must require no engine calls.
    bool valid = true;
    for (unsigned form = 0; form < 2; ++form) {
        HMODULE library = LoadLibraryExA(argv[form + 1], NULL,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        Entry entry = library ? (Entry) GetProcAddress(library,
            "FreakMelee_GetFighterModule") : NULL;
        ParameterSetter setter = library ? (ParameterSetter) GetProcAddress(
            library, "FreakMelee_SetFighterParameters") : NULL;
        const NativeFighterDefinition* definition = entry ? entry(&base, &host) : NULL;
        if (!definition || !definition->on_trail || !setter) return 1;
        for (unsigned i = 0; i < parameters.count; ++i)
            values[i] = parameters.fields[i].bits;
        if (!setter(values, parameters.count, parameters.schema_hash)) return 1;
        memcpy(before, fighter, sizeof(*fighter));
        NativeFighterTrail trail = { .bone = -1 };
        definition->on_trail(&object, &trail);
        valid &= nativeFighterTrail_Valid(&trail, base.joint_count) &&
            !memcmp(before, fighter, sizeof(*fighter)) &&
            trail.bone == 38 && trail.axis == (form ? 2 : 0) &&
            trail.inner_distance == 2.25f && trail.outer_distance == 13.5f &&
            trail.inner_fade == (form ? 0.85f : 0.15f) && trail.outer_fade == 0.85f &&
            trail.start_alpha == 192 && trail.end_alpha == 0 &&
            trail.inner_red == 255 && trail.inner_green == 93 && trail.inner_blue == 17 &&
            trail.outer_red == 101 && trail.outer_green == 142 && trail.outer_blue == 255;
        // Owned value changes affect rendering data without recompiling, and
        // invalid joint/axis/color/NaN values cannot reach the renderer.
        values[0] = 49;
        valid &= setter(values, parameters.count, parameters.schema_hash);
        definition->on_trail(&object, &trail);
        valid &= trail.bone == 49 && nativeFighterTrail_Valid(&trail, 50);
        trail.bone = 50; valid &= !nativeFighterTrail_Valid(&trail, 50);
        trail.bone = -1; valid &= !nativeFighterTrail_Valid(&trail, 50);
        trail.bone = 49; trail.axis = 3; valid &= !nativeFighterTrail_Valid(&trail, 50);
        trail.axis = -1; valid &= !nativeFighterTrail_Valid(&trail, 50);
        trail.axis = 0; trail.start_alpha = 256; valid &= !nativeFighterTrail_Valid(&trail, 50);
        trail.start_alpha = 192; trail.inner_red = -1; valid &= !nativeFighterTrail_Valid(&trail, 50);
        trail.inner_red = 255; trail.outer_distance = NAN; valid &= !nativeFighterTrail_Valid(&trail, 50);
        valid &= !nativeFighterTrail_Valid(NULL, 50);
        FreeLibrary(library);
    }
    free(before); free(fighter);
    if (!valid) { fputs("Authored ribbon descriptor failed.\n", stderr); return 1; }
    puts("Both script forms supply owned ribbon data without changing fighter state.");
    return 0;
}
