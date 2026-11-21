// Original updater bodies versus callbacks loaded from complete generated DLLs.
// Registry, creator validation, lifetime and joint operations are production C.
// Entity construction/free, package registration and common motion rows are
// fixture boundaries. Rendering, animation scheduling and rollback are outside
// this source comparison; it makes no claim about those engine integrations.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/efasync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/types.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftMario/forward.h>
#include <melee/ft/kinds/ftLuigi/forward.h>
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
static FighterBone parts[2][61];
static HSD_GObj creators[2], objects[2];
static EF_Effect effects[2];
static HSD_JObj graphs[2][6], origins[2];
static unsigned frees, comparisons, lifecycle_checks;

static void unexpected_host_call(void)
{
    fputs("Visual update invoked a Host slot outside its declared scope\n", stderr);
    abort();
}
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "HSD assertion %s:%lu: %s\n", file, line, expression);
    abort();
}
#include "model_effect_stock_fixture.h"

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
 return (recipe == 0x47C || recipe == 0x509) ? &joint : NULL;
}
void efLib_remove_user_data(void* data) { (void)data; }
void HSD_GObjFree(HSD_GObj* source)
{
    assert(source == &objects[1]);
    ++frees; // Deferred-free boundary; production cancellation marks it dead.
}
EF_SpawnResult efSync_Spawn(s32 recipe, HSD_GObj* parent, ...)
{
    assert(parent == &creators[1]);
    assert(recipe == active->model_effects[0].gfx_id);
    return (EF_SpawnResult){ .kind = EF_SPAWN_MODEL,
        .model_generation = effects[1].native_generation,
        .object.model = &effects[1], .member_count = 1,
        .members = { { &objects[1], effects[1].native_generation } } };
}

static void reset(unsigned shape, unsigned mask, int motion, u32 command,
                  bool collision, float floor_x, float floor_y,
                  const NativeFighterStateField* field)
{
    memset(heads, 0, sizeof(heads));
    memset(fighters, 0, sizeof(fighters));
    memset(graphs, 0, sizeof(graphs));
    memset(effects, 0, sizeof(effects));
    memset(origins, 0, sizeof(origins));
    for (unsigned side = 0; side < 2; ++side) {
        HSD_JObj* graph = graphs[side];
        if (shape != 0) graph[0].child = &graph[1];
        graph[1].parent = &graph[0];
        if (shape == 2) graph[1].next = &graph[2];
        graph[2].parent = &graph[0];
        graph[2].child = &graph[3];
        graph[3].parent = graph[4].parent = &graph[2];
        graph[3].next = &graph[4];
        graph[3].child = &graph[5];
        graph[5].parent = &graph[3];
        for (unsigned i = 0; i < 6; ++i) {
            graph[i].flags = (mask & 1) ? JOBJ_HIDDEN : 0;
            graph[i].rotate = (Quaternion){ .x = .3f, .y = -.7f, .z = 1.5f, .w = 0 };
            graph[i].scale = (Vec3){ 1, 1, 1 };
        }
        if (mask & 2) graph[2].flags |= JOBJ_INSTANCE;
        if (mask & 4) graph[3].flags |= JOBJ_INSTANCE;
        if (mask & 8) graph[0].flags |= JOBJ_MTX_INDEP_SRT;
        if (mask & 16) graph[0].flags |= JOBJ_MTX_DIRTY;
        if (mask & 32) graph[3].flags |= JOBJ_MTX_INDEP_PARENT;
        fighters[side].kind = Ft_Kind_CustomFirst;
        fighters[side].x8_spawnNum = 0xF1234567U;
        fighters[side].motion_id = motion;
        fighters[side].cmd_vars[3] = command;
        fighters[side].coll_data.floor.normal = (Vec3){ floor_x, floor_y, 0 };
        fighters[side].parts = parts[side];
        parts[side][0].joint = &origins[side];
        // Conflicting generated-side union proves the policy uses owned state.
        int legacy = side ? !collision : collision;
        memcpy(&fighters[side].mv.co.common.x4.z, &legacy, sizeof(legacy));
        memset(fighters[side].native_module_state, 0xA5, active->state_bytes);
        fighters[side].native_module_state[field->offset] = collision;
        creators[side] = (HSD_GObj){ .user_data = &fighters[side], .hsd_obj = &origins[side] };
        fighters[side].gobj = &creators[side];
        objects[side] = (HSD_GObj){ .classifier = HSD_GOBJ_CLASS_EFFECT,
            .user_data_kind = 8, .user_data_remove_func = efLib_remove_user_data,
            .user_data = &effects[side], .hsd_obj = graph };
        effects[side] = (EF_Effect){ .gobj = &objects[side],
            .parent_gobj = &creators[side], .attach_jobj = &origins[side],
            .native_generation = 0xE1234567U, .native_attachment_part = -1 };
    }
    heads[HSD_GOBJ_PLINK_FIGHTER] = &creators[1];
    heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &objects[1];
    HSD_GObjPLinkHead = heads;
    frees = 0;
    nativeModelEffect_Spawn(&creators[1], &active->model_effects[0], -1, NULL, false);
    assert(effects[1].update == active->model_effects[0].update);
    assert(nativeModelEffect_Creator(&effects[1]) == &creators[1]);
    uint32_t generation;
    assert(nativeModelEffect_Snapshot(&objects[1], &generation));
    assert(generation == effects[1].native_generation);
    assert(!nativeModelEffect_Resolve(&objects[1], generation - 1));
    assert(nativeModelJoint_Resolve(graphs[1], "1") ==
        (shape == 2 ? &graphs[1][2] : NULL));
}

static void compare(EF_UpdateFn stock, const char* family)
{
    uint8_t before[sizeof(fighters[1].native_module_state)];
    memcpy(before, fighters[1].native_module_state, sizeof(before));
    stock(&effects[0]);
    effects[1].update(&effects[1]);
    for (unsigned i = 0; i < 6; ++i) {
        // Stock roots are Euler joints. The unused fourth rotation word is
        // outside these callbacks' behavior and is not a parity requirement.
        if (graphs[0][i].flags != graphs[1][i].flags ||
            memcmp(&graphs[0][i].rotate, &graphs[1][i].rotate, sizeof(Vec3))) {
            fprintf(stderr, "%s mismatch case %u node %u flags %08lx/%08lx z %.9g/%.9g\n",
                family, comparisons, i, graphs[0][i].flags, graphs[1][i].flags,
                graphs[0][i].rotate.z, graphs[1][i].rotate.z);
            abort();
        }
        assert(nativeModelJoint_Read(&graphs[1][i], 5) == graphs[0][i].rotate.z);
        assert(nativeModelJoint_Read(&graphs[1][i], 10) ==
            !(graphs[0][i].flags & JOBJ_HIDDEN));
    }
    assert(!memcmp(before, fighters[1].native_module_state, sizeof(before)));
    assert(frees == 0);
    ++comparisons;
}

static void lifecycle(const NativeFighterStateField* field, unsigned scenario)
{
    reset(2, 0, 349, 1, true, .6f, .8f, field);
    EF_UpdateFn update = effects[1].update;
    switch (scenario) {
    case 0: ++fighters[1].x8_spawnNum; break;
    case 1: heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
    case 2: heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = NULL; break;
    case 3: effects[1].native_cancelled = true; break;
    case 4: creators[1].hsd_obj = &graphs[1][1]; break;
    case 5: effects[1].native_author = NULL; break;
    case 6: effects[1].native_policy = NULL; break;
    case 7: effects[1].native_creator_generation--; break;
    }
    HSD_JObj before[6];
    memcpy(before, graphs[1], sizeof(before));
    update(&effects[1]);
    assert(!memcmp(before, graphs[1], sizeof(before)));
    assert(frees == (scenario == 6 ? 1U : 0U));
    ++lifecycle_checks;
}

static void load_parameters(HMODULE library, const char* path)
{
    char text[32768];
    FILE* file = fopen(path, "rb");
    assert(file);
    size_t size = fread(text, 1, sizeof(text), file);
    assert(!ferror(file) && size < sizeof(text));
    fclose(file);
    NativeFighterParameters values;
    unsigned line;
    assert(nativeFighterParameters_Parse(text, size, &values, &line));
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < values.count; ++i) bits[i] = values.fields[i].bits;
    ParameterEntry bind = (ParameterEntry)GetProcAddress(library, "FreakMelee_SetFighterParameters");
    assert(bind && bind(bits, values.count, values.schema_hash));
}

int main(int argc, char** argv)
{
    assert(argc == 7); // Mario DLL/FMP, Dr. Mario DLL/FMP, Luigi DLL/FMP.
    NativeFighterHost host = {0};
    stock_fixture_host_traps(&host);
    static MotionState common[ftCo_MS_Count], motions[ftLg_MS_SelfCount];
    host.common_motion_states = common;
    host.model_effect_joint = nativeModelEffect_Joint;
    host.spawn_model_effect = nativeModelEffect_Spawn;
    host.model_effect_creator = nativeModelEffect_Creator;
    host.model_effect_cancel = nativeModelEffect_Cancel;
    host.model_joint_resolve = nativeModelJoint_Resolve;
    host.model_joint_read = nativeModelJoint_Read;
    host.model_joint_write = nativeModelJoint_Write;
    host.callback_source_alive = nativeCallbackSource_Alive;
    host.angle_atan2 = atan2;
    host.angle_atan2_float = atan2f;
    static const float slopes[][2] = {
        {0, 1}, {-0.0f, 1}, {.6f, .8f}, {-.6f, .8f}, {1, 0}, {-1, 0},
        {0, -1}, {0, -0.0f}, {0, 0}, {.25f, -.75f}
    };
    const u32 commands[] = {0, 1, UINT32_MAX};
    for (unsigned family = 0; family < 3; ++family) {
        HMODULE library = LoadLibraryA(argv[1 + family * 2]);
        assert(library);
        ModuleEntry entry = (ModuleEntry)GetProcAddress(library, "FreakMelee_GetFighterModule");
        LayoutEntry layout = (LayoutEntry)GetProcAddress(library, "FreakMelee_GetFighterModuleLayout");
        assert(entry && layout && layout() == nativeFighterModule_LayoutFingerprint());
        load_parameters(library, argv[2 + family * 2]);
        NativeFighterDefinition base = { .code_key = "effect_fixture",
            .fighter_root = family == 2 ? "ftDataLuigi" : family ? "ftDataDrmario" : "ftDataMario",
            .source_kind = family == 2 ? Ft_Kind_Luigi : family ? Ft_Kind_DrMario : Ft_Kind_Mario,
            .motion_state_first = ftCo_MS_Count, .motion_states = motions,
            .motion_state_count = family == 2 ? ftLg_MS_SelfCount : ftMr_MS_SelfCount,
            .joint_count = 61, .state_bytes = 3,
            .special_ground_state = {343, 345, 347, 349},
            .special_air_state = {344, 346, 348, 350} };
        if (family == 2) {
            const FtMotionId ground[] = {341, 343, 355, 357};
            const FtMotionId air[] = {342, 349, 356, 358};
            memcpy(base.special_ground_state, ground, sizeof(ground));
            memcpy(base.special_air_state, air, sizeof(air));
        }
        active = entry(&base, &host);
        assert(active && active->model_effect_count == 1);
        NativeFighterHost missing = host;
        missing.model_joint_write = NULL;
        assert(!entry(&base, &missing));
        assert(nativeModelEffect_Validate(active));
        assert(active->model_effects[0].gfx_id == (family == 2 ? 0x509 : 0x47C));
        const char* field_name = family == 2 ? "cyclone_collision" : "tornado_collision";
        const NativeFighterStateField* field = NULL;
        for (unsigned i = 0; i < active->state_field_count; ++i)
            if (!strcmp(active->state_fields[i].name, field_name)) field = &active->state_fields[i];
        assert(field && field->type == NativeFighterState_Bool && field->count == 1);
        EF_UpdateFn stock = family == 2 ? efLib_Cb_ftLg_SpecialLw : efLib_Cb_ftMr_SpecialLw;
        const int motions_to_test[] = {349, 357, 0};
        unsigned start = comparisons;
        for (unsigned shape = 0; shape < 3; ++shape)
        for (unsigned mask = 0; mask < 64; ++mask)
        for (unsigned motion = 0; motion < 3; ++motion)
        for (unsigned command = 0; command < 3; ++command)
        for (unsigned collision = 0; collision < 2; ++collision)
        for (unsigned slope = 0; slope < sizeof(slopes)/sizeof(slopes[0]); ++slope) {
            reset(shape, mask, motions_to_test[motion], commands[command], collision,
                slopes[slope][0], slopes[slope][1], field);
            compare(stock, field_name);
        }
        for (unsigned scenario = 0; scenario < 8; ++scenario) lifecycle(field, scenario);
        printf("%s: %u exact original/generated effect comparisons\n", field_name, comparisons - start);
        nativeModelEffect_Clear();
        active = NULL;
        FreeLibrary(library);
    }
    printf("%u comparisons, 0 failures; %u production-registry/lifetime rejection cases\n",
        comparisons, lifecycle_checks);
    return 0;
}
