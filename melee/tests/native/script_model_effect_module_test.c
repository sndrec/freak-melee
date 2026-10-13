#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/ftdat.h>
#include <melee/ef/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef uint64_t (__cdecl* LayoutEntry)(void);
typedef int (__cdecl* ParametersEntry)(const uint32_t*, unsigned, uint64_t);

static Fighter fighter;
static HSD_GObj fighter_object, effect_object;
static EF_Effect effect;
static HSD_JObj root, child;
static bool fighter_live, effect_live, creator_present;
static unsigned writes, cancels, spawns, creator_queries;
static unsigned spawn_failure;
static float root_x, root_y, root_z;
static float root_rotation_y;
static bool visible, self_visible;
static const NativeFighterModelEffect* spawned[2];
static s32 spawned_parts[2];

static bool source_alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    if (source_class == NativeCallbackSource_Fighter)
        return object == &fighter_object && fighter_live && generation == (uint32_t)fighter.x8_spawnNum;
    if (source_class == NativeCallbackSource_ModelEffect)
        return object == &effect_object && effect_live && !effect.native_cancelled && generation == (uint32_t)effect.native_generation;
    return false;
}

static Fighter_GObj* creator(EF_Effect* target)
{
    assert(target == &effect); ++creator_queries;
    return creator_present && fighter_live ? &fighter_object : NULL;
}

static void cancel(EF_Effect* target)
{
    assert(target == &effect); ++cancels;
    target->native_cancelled = true;
    // Model deletion can be deferred: object remains in the live registry.
}

static HSD_JObj* effect_joint(EF_Effect* source, unsigned member)
{
    return source == &effect && member == 0 && effect_live && !effect.native_cancelled ? &root : NULL;
}

static HSD_JObj* resolve(HSD_JObj* origin, const char* path)
{
    assert(origin == &root);
    if (!*path) return &root;
    if (!strcmp(path, "0") || !strcmp(path, "1")) return &child;
    assert(!strcmp(path, "0/255")); return NULL;
}

static float read(HSD_JObj* joint, unsigned channel)
{
    assert(channel == 9); return joint != NULL;
}

static void write(HSD_JObj* joint, unsigned channel, float value)
{
    if (joint == NULL) { assert(channel == 14); return; }
    ++writes;
    if (joint == &root) {
        if (channel == 0) root_x = value;
        else if (channel == 1) root_y = value;
        else if (channel == 4) root_rotation_y = value;
        else { assert(channel == 5); root_z = value; }
    } else {
        assert(joint == &child);
        if (channel == 10) visible = value != 0;
        else { assert(channel == 15); self_visible = value != 0; }
    }
}

static Fighter_Part bone(Fighter* source, Fighter_Part role)
{
    assert(source == &fighter && role == FtPart_TopN); return 3;
}

static void spawn(Fighter_GObj* source, const NativeFighterModelEffect* descriptor, s32 part, Vec3* position, bool deferred)
{
    assert(source == &fighter_object && spawns < 2 && position == NULL && !deferred);
    spawned[spawns] = descriptor; spawned_parts[spawns++] = part;
    if (spawn_failure == 1) fighter_live = false;
    if (spawn_failure == 2) ++fighter.x8_spawnNum;
}

static void reset(void)
{
    fighter = (Fighter){ .gobj = &fighter_object, .x8_spawnNum = UINT32_C(0xFEDCBA98) };
    fighter_object = (HSD_GObj){ .user_data = &fighter };
    effect_object = (HSD_GObj){ .hsd_obj = &root };
    effect = (EF_Effect){ .gobj = &effect_object, .parent_gobj = &fighter_object,
        .native_generation = UINT32_C(0xCDEF0123) };
    fighter_live = effect_live = creator_present = true;
    writes = cancels = spawns = creator_queries = spawn_failure = 0;
    root_x = root_y = root_z = -77; visible = self_visible = false;
    root_rotation_y = -77;
}

static unsigned field_offset(const NativeFighterDefinition* definition, const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(definition->state_fields[i].name, name)) return definition->state_fields[i].offset;
    assert(false); return 0;
}

int main(int argc, char** argv)
{
    if (argc != 3) return 2;
    unsigned checks = 0;
    for (unsigned route = 0; route < 2; ++route) {
        HMODULE library = LoadLibraryExA(argv[route + 1], NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        assert(library);
        ModuleEntry entry = (ModuleEntry)GetProcAddress(library, "FreakMelee_GetFighterModule");
        LayoutEntry layout = (LayoutEntry)GetProcAddress(library, "FreakMelee_GetFighterModuleLayout");
        ParametersEntry upload = (ParametersEntry)GetProcAddress(library, "FreakMelee_SetFighterParameters");
        assert(entry && layout && upload && layout() == nativeFighterModule_LayoutFingerprint());
        NativeFighterParameters parameters; unsigned line = 0;
        const char values[] = "int air_motion = 357\nfloat amount = 0.5\n";
        assert(nativeFighterParameters_Parse(values, strlen(values), &parameters, &line));
        uint32_t bits[2] = { parameters.fields[0].bits, parameters.fields[1].bits };
        assert(upload(bits, parameters.count, parameters.schema_hash));
        MotionState motion = { 0 };
        NativeFighterDefinition base = { .fighter_root = "ftDataLuigi", .motion_state_first = 341,
            .motion_state_count = 1, .motion_states = &motion };
        NativeFighterHost host = { .callback_source_alive = source_alive,
            .model_effect_creator = creator, .model_effect_cancel = cancel,
            .model_joint_resolve = resolve, .model_joint_read = read, .model_joint_write = write,
            .spawn_model_effect = spawn, .model_effect_joint = effect_joint, .resolve_bone = bone, .angle_atan2_float = atan2f };
        const NativeFighterDefinition* definition = entry(&base, &host);
        assert(definition && definition->model_effect_count == 2 && definition->model_effects);
        assert(definition->model_effects[0].name_hash < definition->model_effects[1].name_hash);
        EF_UpdateFn update = NULL;
        for (unsigned i = 0; i < 2; ++i)
            if (definition->model_effects[i].gfx_id == 0x509) update = definition->model_effects[i].update;
        assert(update);
#define MISSING(member) do { NativeFighterHost missing = host; missing.member = NULL; assert(!entry(&base, &missing)); ++checks; } while (0)
        MISSING(callback_source_alive); MISSING(model_effect_creator); MISSING(model_effect_cancel);
        MISSING(model_effect_joint); MISSING(model_joint_resolve); MISSING(model_joint_read); MISSING(model_joint_write);
        MISSING(spawn_model_effect); MISSING(resolve_bone); MISSING(angle_atan2_float);
#undef MISSING
        unsigned collision_offset = field_offset(definition, "collision");
        for (unsigned air = 0; air < 2; ++air)
            for (unsigned command = 0; command < 2; ++command)
                for (unsigned collision = 0; collision < 2; ++collision)
                    for (int slope = -1; slope <= 1; ++slope)
                    for (int direction = -1; direction <= 1; ++direction) {
                        reset(); fighter.motion_id = air ? 357 : 355;
                        fighter.facing_dir = (float)direction;
                        fighter.cmd_vars[3] = command;
                        fighter.native_module_state[collision_offset] = (uint8_t)collision;
                        fighter.coll_data.floor.normal = (Vec3){ (float)slope, 1, 0 };
                        update(&effect);
                        assert(cancels == 0 && writes == 6 && visible == (air != 0) && self_visible);
                        assert(root_x == 99 && root_y == 100);
                        assert(root_rotation_y == (float)direction);
                        assert(root_z == (command && collision ? -atan2f((float)slope, 1) : 0));
                        ++checks;
                    }
        reset(); fighter.cmd_vars[0] = 1; update(&effect);
        assert(cancels == 1 && effect_live && writes == 3 && root_x == -77 && root_y == -77); ++checks;
        reset(); creator_present = false; update(&effect);
        assert(cancels == 1 && writes == 0 && creator_queries == 1); ++checks;
        reset(); effect_live = false; update(&effect);
        assert(cancels == 0 && writes == 0 && creator_queries == 0); ++checks;
        HSD_GObjEvent constructor = route ? definition->motion_states[0].phys_cb : definition->on_load;
        assert(constructor);
        reset(); constructor(&fighter_object);
        assert(spawns == 2 && spawned[0]->gfx_id == 0x509 && spawned_parts[0] == -1 &&
            spawned[1]->gfx_id == 0x47C && spawned_parts[1] == 3 && fighter.x2219_b0); ++checks;
        for (unsigned failure = 1; failure <= 2; ++failure) {
            reset(); spawn_failure = failure; constructor(&fighter_object);
            assert(spawns == 1 && !fighter.x2219_b0); ++checks;
        }
        FreeLibrary(library);
    }
    printf("Model effect generated modules: %u decision, binding and lifetime cases passed.\n", checks);
    return 0;
}
