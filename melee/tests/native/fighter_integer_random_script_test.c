#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_module.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static int random_bounds[8];
static unsigned random_calls;

static s32 highest_choice(s32 count)
{
    if (random_calls < 8) random_bounds[random_calls] = count;
    ++random_calls;
    return count - 1;
}

static int state_int(const Fighter* fighter, unsigned offset)
{
    int value;
    memcpy(&value, fighter->native_module_state + offset, sizeof(value));
    return value;
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 2;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    MotionState motions[12] = { 0 };
    NativeFighterDefinition base = {
        .fighter_root = "ftDataRook",
        .motion_states = motions,
        .motion_state_first = 341,
        .motion_state_count = 12,
    };
    NativeFighterHost host = { .random_int = highest_choice };
    const NativeFighterDefinition* definition =
        entry != NULL ? entry(&base, &host) : NULL;
    NativeFighterHost missing = { 0 };
    bool valid = definition != NULL && entry(&base, &missing) == NULL &&
                 definition->state_bytes == 12;
    Fighter first = { 0 }, second = { 0 };
    HSD_GObj first_object = { .user_data = &first };
    HSD_GObj second_object = { .user_data = &second };
    if (valid) {
        definition->on_load(&first_object);
        definition->on_load(&second_object);
        valid = state_int(&first, 4) == 9 && state_int(&first, 8) == 9 &&
                state_int(&second, 4) == 9;
        definition->on_tick(&first_object);
        valid &= state_int(&first, 4) == 8 && state_int(&first, 8) == 9;
        definition->on_tick(&first_object);
        valid &= state_int(&first, 4) == 7 && state_int(&first, 8) == 8;
        definition->on_tick(&first_object);
        valid &= state_int(&first, 4) == 6 && state_int(&first, 8) == 7;
        valid &= state_int(&second, 4) == 9 && state_int(&second, 8) == 9;
        valid &= random_calls == 3 && random_bounds[0] == 9 &&
                 random_bounds[1] == 8 && random_bounds[2] == 7;
        valid &= first.take_dmg_cb != NULL && first.death2_cb != NULL;
        if (first.take_dmg_cb != NULL) first.take_dmg_cb(&first_object);
        if (first.death2_cb != NULL) first.death2_cb(&first_object);
        valid &= state_int(&first, 4) == 16 &&
                 state_int(&first, 8) == 17 &&
                 state_int(&second, 4) == 9;
        valid &= definition->on_item_pickup != NULL;
        if (definition->on_item_pickup != NULL) {
            definition->on_item_pickup(&first_object, false);
            valid &= state_int(&first, 4) == 16;
            definition->on_item_pickup(&first_object, true);
            valid &= state_int(&first, 4) == 17 &&
                     state_int(&second, 4) == 9;
        }
    }
    FreeLibrary(library);
    if (!valid) {
        fputs("Integer state or bounded random selection diverged.\n", stderr);
        return 1;
    }
    puts("Integer state remains fighter-local; bounded random selection excludes the previous two choices.");
    return 0;
}
