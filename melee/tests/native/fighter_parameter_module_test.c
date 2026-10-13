#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_module.h>
#include <sysdolphin/baselib/gobj.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>

#include <stdio.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterSetter)(const uint32_t*, unsigned, uint64_t);

static void start_animation(Fighter_GObj* object) { (void) object; }
static void change_motion(Fighter_GObj* object, FtMotionId motion,
                          MotionFlags flags, float frame, float speed,
                          float blend, Fighter_GObj* source)
{
    (void) object;
    (void) motion;
    (void) flags;
    (void) frame;
    (void) speed;
    (void) blend;
    (void) source;
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 2;
    Entry entry = (Entry) GetProcAddress(library, "FreakMelee_GetFighterModule");
    ParameterSetter setter = (ParameterSetter) GetProcAddress(
        library, "FreakMelee_SetFighterParameters");
    NativeFighterParameters parameters;
    const char* source = "float lift = 1.25\nint uses = -2\nbool ready = true\n"
        "float It_Kind_Fox_Laser.speed = 5.5\nfloat It_Kind_Fox_Laser.lifetime = 35\n"
        "bool It_Kind_Fox_Laser.enabled = true\nint It_Kind_Fox_Laser.limit = 3\n";
    bool valid = entry != NULL && setter != NULL &&
        nativeFighterParameters_Parse(source, strlen(source), &parameters, NULL);
    uint32_t bits[7];
    for (unsigned i = 0; valid && i < 7; ++i)
        bits[i] = parameters.fields[i].bits;
    valid = valid && !setter(bits, 2, parameters.schema_hash) &&
        !setter(bits, 7, parameters.schema_hash + 1) &&
        setter(bits, 7, parameters.schema_hash);
    MotionState motions[1] = { 0 };
    NativeFighterDefinition base = {
        .fighter_root = "ftDataMario",
        .motion_states = motions,
        .motion_state_first = 341,
        .motion_state_count = 1,
    };
    NativeFighterHost host = {
        .change_motion_state = change_motion,
        .start_animation = start_animation,
    };
    const NativeFighterDefinition* definition = valid ? entry(&base, &host) : NULL;
    valid = valid && definition != NULL &&
        definition->motion_states[0].phys_cb != NULL &&
        definition->on_landing_action != NULL && definition->on_landing == NULL;
    Fighter fighter = { 0 };
    HSD_GObj object = { .user_data = &fighter };
    if (valid) {
        definition->motion_states[0].phys_cb(&object);
        valid = fighter.self_vel.y == 1.25f && fighter.self_vel.x == -2.0f;
        definition->on_landing_action(&object);
        valid = valid && fighter.self_vel.x == 0.0f && fighter.self_vel.y == 1.25f;
        // A null source DAT deliberately proves that both callbacks and
        // launch initialization read package values, not inherited offsets.
        Item item = { 0 };
        HSD_GObj article_object = { .user_data = &item };
        const NativeFighterArticleLogic* article = &definition->article_logics[0];
        float values[3];
        article->projectile->read_values(NULL, values);
        article->logic->states[0].physics_updated(&article_object);
        valid = valid && values[0] == 5.5f && values[1] == 0 && values[2] == 35 &&
            item.xD44_lifeTimer == 35 && article->logic->states[0].animated(&article_object);
        source = "float lift = 2.5\nint uses = 7\nbool ready = false\n"
            "float It_Kind_Fox_Laser.speed = 8.25\nfloat It_Kind_Fox_Laser.lifetime = 21\n"
            "bool It_Kind_Fox_Laser.enabled = false\nint It_Kind_Fox_Laser.limit = 2\n";
        valid = valid && nativeFighterParameters_Parse(source, strlen(source),
                                                       &parameters, NULL);
        for (unsigned i = 0; i < 7; ++i) bits[i] = parameters.fields[i].bits;
        valid = valid && setter(bits, 7, parameters.schema_hash);
        fighter.self_vel.x = 0;
        definition->motion_states[0].phys_cb(&object);
        valid = valid && fighter.self_vel.y == 2.5f && fighter.self_vel.x == 0;
        article->projectile->read_values(NULL, values);
        article->logic->states[0].physics_updated(&article_object);
        valid = valid && values[0] == 8.25f && values[2] == 21 &&
            item.xD44_lifeTimer == 21 && !article->logic->states[0].animated(&article_object);
    }
    FreeLibrary(library);
    if (!valid) {
        fputs("Loaded fighter parameter behavior failed.\n", stderr);
        return 1;
    }
    puts("Loaded fighter parameters changed behavior without a rebuild");
    return 0;
}
