// Unchanged original Fire travel updater versus each full generated template.
// Production registry, lifetime and HSD joint channels are linked directly.
// Allocation, package lookup, common motion rows and deferred free are fixture
// boundaries; this does not test rendering, efLib_Update or rollback restore.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Runtime/platform.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftFox/forward.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterEntry)(const uint32_t*, unsigned, uint64_t);
typedef uint64_t (__cdecl* LayoutEntry)(void);
static const NativeFighterDefinition* active;
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static Fighter fighters[2];
static FighterBone parts[2][73];
static HSD_GObj creators[2], objects[2];
static EF_Effect effects[2];
static HSD_JObj graphs[2][3], origins[2];
static unsigned frees, comparisons, lifetime_checks, spawns;
static s32 spawned_part;

static void unexpected_host_call(void)
{ fputs("Unexpected Host call during FireTravel comparison\n", stderr); abort(); }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "HSD assertion %s:%lu: %s\n", file, line, expression); abort(); }
#include "fox_falco_model_effect_stock_fixture.h"

HSD_ObjAllocData efAsync_AllocData;
EF_EffectUsage efLib_Usage;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
void* HSD_ObjAlloc(HSD_ObjAllocData* allocator) { (void)allocator; abort(); }
void efAsync_QueueProcessDeferred(HSD_GObj* source, EF_QueuedEffect* queued)
{ (void)source; (void)queued; abort(); }

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == Ft_Kind_CustomFirst ? active : NULL; }
const EF_ModelRecipe* efSync_ModelRecipe(s32 recipe)
{
    static const EF_ModelRecipe joint = { EF_MODEL_JOINT, 1 };
 return (recipe == 0x48C) ? &joint : NULL;
}
void efLib_remove_user_data(void* data) { (void)data; }
void HSD_GObjFree(HSD_GObj* source)
{ assert(source == &objects[1]); ++frees; }
EF_SpawnResult efSync_Spawn(s32 recipe, HSD_GObj* parent, ...)
{
    assert(recipe == 0x48C && parent == &creators[1]);
    ++spawns;
    return (EF_SpawnResult){ .kind = EF_SPAWN_MODEL,
        .model_generation = effects[1].native_generation,
        .object.model = &effects[1], .member_count = 1,
        .members = { { &objects[1], effects[1].native_generation } } };
}
static void spawn(Fighter_GObj* source,
                  const NativeFighterModelEffect* policy, s32 part, Vec3* position, bool deferred)
{ spawned_part = part; nativeModelEffect_Spawn(source, policy, part, position, deferred); }
static Fighter_Part resolve_bone(Fighter* fighter, Fighter_Part role)
{
    assert(fighter == &fighters[1] && role == FtPart_HipN);
    return 4; // Independently verified against both actual retail maps.
}
static void change_motion(Fighter_GObj* source, FtMotionId motion,
    MotionFlags flags, float frame, float rate, float blend, Fighter_GObj* donor)
{
    assert(source == &creators[1] && motion == 353 && flags == 0);
    assert(frame == 0 && rate == 1 && blend == 0 && donor == NULL);
    fighters[1].motion_id = motion;
}
static void start_animation(Fighter_GObj* source)
{ assert(source == &creators[1]); }

static void reset(unsigned shape, unsigned mask, int motion,
                  float facing, float angle, unsigned offset)
{
    memset(heads, 0, sizeof(heads));
    memset(fighters, 0, sizeof(fighters));
    memset(parts, 0, sizeof(parts));
    memset(graphs, 0, sizeof(graphs));
    memset(effects, 0, sizeof(effects));
    for (unsigned side = 0; side < 2; ++side) {
        if (shape) {
            graphs[side][0].child = &graphs[side][1];
            graphs[side][1].parent = graphs[side][2].parent = &graphs[side][0];
            graphs[side][1].next = &graphs[side][2];
        }
        for (unsigned node = 0; node < 3; ++node) {
            graphs[side][node].rotate = (Quaternion){ .x = .3f, .y = -.7f, .z = 1.5f, .w = .25f };
            graphs[side][node].translate = (Vec3){ .2f, .4f, .6f };
            graphs[side][node].scale = (Vec3){ 1, 2, 3 };
        }
        if (mask & 1) graphs[side][0].flags |= JOBJ_MTX_INDEP_SRT;
        if (mask & 2) graphs[side][0].flags |= JOBJ_MTX_DIRTY;
        if (mask & 4) graphs[side][1].flags |= JOBJ_INSTANCE;
        if (mask & 8) graphs[side][2].flags |= JOBJ_MTX_INDEP_PARENT;
        fighters[side].kind = Ft_Kind_CustomFirst;
        fighters[side].x8_spawnNum = UINT32_C(0xF1234567);
        fighters[side].motion_id = motion;
        fighters[side].facing_dir = facing;
        fighters[side].parts = parts[side];
        parts[side][4].joint = &origins[side];
        // The generated callback must never consult the conflicting stock union.
        fighters[side].mv.co.common.x4.x = side ? -123.75f : angle;
        memset(fighters[side].native_module_state, 0xA5, sizeof(fighters[side].native_module_state));
        memcpy(fighters[side].native_module_state + offset, &angle, sizeof(angle));
        creators[side] = (HSD_GObj){ .user_data = &fighters[side], .hsd_obj = &origins[side] };
        fighters[side].gobj = &creators[side];
        objects[side] = (HSD_GObj){ .classifier = HSD_GOBJ_CLASS_EFFECT,
            .user_data_kind = 8, .user_data_remove_func = efLib_remove_user_data,
            .user_data = &effects[side], .hsd_obj = graphs[side] };
        effects[side] = (EF_Effect){ .gobj = &objects[side],
            .parent_gobj = &creators[side], .attach_jobj = &origins[side],
            .native_generation = UINT32_C(0xE1234567), .native_attachment_part = 4 };
    }
    heads[HSD_GOBJ_PLINK_FIGHTER] = &creators[1];
    heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &objects[1];
    HSD_GObjPLinkHead = heads;
    frees = spawns = 0;
    nativeModelEffect_Spawn(&creators[1], &active->model_effects[0], 4, NULL, false);
    assert(spawns == 1 && effects[1].update == active->model_effects[0].update);
    assert(nativeModelEffect_Creator(&effects[1]) == &creators[1]);
}

static void compare(void)
{
    uint8_t state[sizeof(fighters[1].native_module_state)];
    memcpy(state, fighters[1].native_module_state, sizeof(state));
    efLib_Cb_SetRotYZ_FromFighter(&effects[0]);
    effects[1].update(&effects[1]);
    for (unsigned node = 0; node < 3; ++node) {
        HSD_JObj *stock = &graphs[0][node], *script = &graphs[1][node];
        assert(stock->flags == script->flags);
        assert(!memcmp(&stock->rotate, &script->rotate, sizeof(Quaternion)));
        assert(!memcmp(&stock->translate, &script->translate, sizeof(Vec3)));
        assert(!memcmp(&stock->scale, &script->scale, sizeof(Vec3)));
    }
    assert(!memcmp(state, fighters[1].native_module_state, sizeof(state)));
    assert(frees == 0);
    ++comparisons;
}

static void lifecycle(unsigned offset, unsigned scenario)
{
    reset(1, 0, 355, 1, .75f, offset);
    EF_UpdateFn update = effects[1].update;
    switch (scenario) {
    case 0: ++fighters[1].x8_spawnNum; break;
    case 1: heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
    case 2: heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = NULL; break;
    case 3: effects[1].native_cancelled = true; break;
    case 4: parts[1][4].joint = &graphs[1][1]; break;
    case 5: effects[1].native_author = NULL; break;
    case 6: effects[1].native_policy = NULL; break;
    case 7: --effects[1].native_creator_generation; break;
    }
    HSD_JObj before[3]; memcpy(before, graphs[1], sizeof(before));
    update(&effects[1]);
    assert(!memcmp(before, graphs[1], sizeof(before)));
    assert(frees == (scenario == 6 ? 1U : 0U));
    ++lifetime_checks;
}

static void parameters(HMODULE library, const char* path)
{
    char text[32768]; FILE* input = fopen(path, "rb"); assert(input);
    size_t bytes = fread(text, 1, sizeof(text), input);
    assert(!ferror(input) && bytes < sizeof(text)); fclose(input);
    NativeFighterParameters values; unsigned line;
    assert(nativeFighterParameters_Parse(text, bytes, &values, &line));
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < values.count; ++i) bits[i] = values.fields[i].bits;
    ParameterEntry upload = (ParameterEntry)GetProcAddress(library, "FreakMelee_SetFighterParameters");
    assert(upload && upload(bits, values.count, values.schema_hash));
}

int main(int argc, char** argv)
{
    assert(argc == 5); // Fox DLL/FMP then Falco DLL/FMP.
    NativeFighterHost host = {0}; stock_fixture_host_traps(&host);
    static MotionState common[ftCo_MS_Count], motions[ftFx_MS_SelfCount];
    host.common_motion_states = common;
    host.change_motion_state = change_motion;
    host.start_animation = start_animation;
    host.model_effect_joint = nativeModelEffect_Joint;
    host.spawn_model_effect = spawn;
    host.resolve_bone = resolve_bone;
    host.model_effect_creator = nativeModelEffect_Creator;
    host.model_effect_cancel = nativeModelEffect_Cancel;
    host.model_joint_resolve = nativeModelJoint_Resolve;
    host.model_joint_read = nativeModelJoint_Read;
    host.model_joint_write = nativeModelJoint_Write;
    host.callback_source_alive = nativeCallbackSource_Alive;
    const float angles[] = {0, -0.0f, .1f, -.1f, .75f, -.75f,
        M_PI_2_F, -M_PI_2_F, M_PI_F, -M_PI_F, 1000, -1000};
    const float directions[] = {-1, 1, -0.0f, 0};
    for (unsigned family = 0; family < 2; ++family) {
        HMODULE library = LoadLibraryA(argv[1 + family * 2]); assert(library);
        ModuleEntry entry = (ModuleEntry)GetProcAddress(library, "FreakMelee_GetFighterModule");
        LayoutEntry layout = (LayoutEntry)GetProcAddress(library, "FreakMelee_GetFighterModuleLayout");
        assert(entry && layout && layout() == nativeFighterModule_LayoutFingerprint());
        parameters(library, argv[2 + family * 2]);
        NativeFighterDefinition base = { .code_key = "fire_travel_fixture",
            .fighter_root = family ? "ftDataFalco" : "ftDataFox",
            .source_kind = family ? Ft_Kind_Falco : Ft_Kind_Fox,
            .motion_state_first = 341, .motion_states = motions,
            .motion_state_count = ftFx_MS_SelfCount, .joint_count = family ? 67 : 73,
            .bones = family ? ftFalcoClone_Bones : ftFoxClone_Bones,
            .bone_count = sizeof(ftFoxClone_Bones)/sizeof(ftFoxClone_Bones[0]),
            .special_ground_state = {341,347,353,360},
            .special_air_state = {344,350,354,365} };
        active = entry(&base, &host);
        assert(active && active->model_effect_count == 1);
        assert(active->bone_count == 52 && active->bones[4].part == FtPart_HipN &&
               active->bones[4].joint == 4);
        assert(nativeModelEffect_Validate(active) && active->model_effects[0].gfx_id == 0x48C);
        const NativeFighterStateField* angle = NULL;
        for (unsigned i = 0; i < active->state_field_count; ++i)
            if (!strcmp(active->state_fields[i].name, "fire_angle")) angle = &active->state_fields[i];
        assert(angle && angle->type == NativeFighterState_Float && angle->count == 1);
        unsigned start = comparisons;
        for (unsigned shape = 0; shape < 2; ++shape)
        for (unsigned mask = 0; mask < 16; ++mask)
        for (int motion = 355; motion <= 357; ++motion)
        for (unsigned facing = 0; facing < sizeof(directions)/sizeof(directions[0]); ++facing)
        for (unsigned value = 0; value < sizeof(angles)/sizeof(angles[0]); ++value) {
            reset(shape, mask, motion, directions[facing], angles[value], angle->offset);
            compare();
            // Prove each tick reads current facing/angle, not creation snapshots.
            fighters[0].facing_dir = fighters[1].facing_dir = -1;
            fighters[0].mv.co.common.x4.x = -.375f;
            const float changed_angle = -.375f;
            memcpy(fighters[1].native_module_state + angle->offset, &changed_angle, sizeof(changed_angle));
            compare();
        }
        // Actual authored accessory selects the named travel recipe and mapped Hip.
        reset(0, 0, 355, 1, .5f, angle->offset);
        fighters[1].x2219_b0 = false; spawned_part = -2; spawns = 0;
        assert(active->special_hi);
        active->special_hi(&creators[1]);
        assert(fighters[1].accessory4_cb);
        fighters[1].motion_id = 355;
        fighters[1].accessory4_cb(&creators[1]);
        assert(spawns == 1 && spawned_part == 4 && fighters[1].x2219_b0);
        for (unsigned scenario = 0; scenario < 8; ++scenario) lifecycle(angle->offset, scenario);
        printf("%s: %u exact original/generated FireTravel comparisons\n", family ? "Falco" : "Fox", comparisons - start);
        nativeModelEffect_Clear(); active = NULL; FreeLibrary(library);
    }
    printf("%u comparisons, 0 failures; %u production-registry/lifetime rejection cases\n", comparisons, lifetime_checks);
    return 0;
}
