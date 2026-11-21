// Actual original effect updaters versus the full generated fighter modules.
// Construction/registration are resource boundaries. Group/queue/animation
// scheduling have their own production-body fixture, not simulated coverage here.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ef/eflib.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftDonkey/forward.h>
#include <melee/ft/kinds/ftKoopa/forward.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterEntry)(const uint32_t*, unsigned, uint64_t);
typedef uint64_t (__cdecl* LayoutEntry)(void);
static const NativeFighterDefinition* active;
HSD_GObj** HSD_GObjPLinkHead;
HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_ObjAllocData efAsync_AllocData;
EF_EffectUsage efLib_Usage;
static HSD_GObj* heads[32];
static Fighter fighters[2];
static HSD_GObj creators[2], objects[2][2];
static EF_Effect effects[2][2];
static HSD_JObj graphs[2][6], origins[2];
static unsigned comparisons, lifecycle_checks, frees;
static void unexpected_host_call(void) { abort(); }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr, "%s:%lu %s\n", file, line, expression); abort(); }
#include "donkey_bowser_model_effect_stock_fixture.h"
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == Ft_Kind_CustomFirst ? active : NULL; }
const EF_ModelRecipe* efSync_ModelRecipe(s32 recipe)
{
    static const EF_ModelRecipe position = { EF_MODEL_POSITION, 1 }, pair = { EF_MODEL_JOINT, 2 };
    return recipe == 0x4CC ? &position : recipe == 0x4DA ? &pair : NULL;
}
void* HSD_ObjAlloc(HSD_ObjAllocData* allocator) { (void)allocator; abort(); }
void efAsync_QueueProcessDeferred(HSD_GObj* source, EF_QueuedEffect* queued)
{ (void)source; (void)queued; abort(); }
void efLib_remove_user_data(void* data) { (void)data; }
void HSD_GObjFree(HSD_GObj* object)
{ assert(object == &objects[1][0] || object == &objects[1][1]); ++frees; }
EF_SpawnResult efSync_Spawn(s32 recipe, HSD_GObj* parent, ...)
{
    assert(parent == &creators[1]);
    const EF_ModelRecipe* layout = efSync_ModelRecipe(recipe); assert(layout);
    EF_SpawnResult result = { .kind = EF_SPAWN_MODEL, .object.model = &effects[1][0],
        .model_generation = effects[1][0].native_generation, .member_count = layout->model_count };
    for (unsigned i = 0; i < result.member_count; ++i)
        result.members[i] = (EF_ModelMember){ &objects[1][i], effects[1][i].native_generation };
    return result;
}
static void reset(unsigned family, unsigned mask, int motion, u32 command,
                  bool collision, float facing, float x, float y, unsigned offset)
{
    memset(heads, 0, sizeof(heads)); memset(fighters, 0, sizeof(fighters));
    memset(graphs, 0, sizeof(graphs)); memset(effects, 0, sizeof(effects));
    for (unsigned side = 0; side < 2; ++side) {
        for (unsigned root = 0; root < 6; root += 3) {
            graphs[side][root].child = &graphs[side][root + 1];
            graphs[side][root + 1].parent = graphs[side][root + 2].parent = &graphs[side][root];
            graphs[side][root + 1].next = &graphs[side][root + 2];
        }
        for (unsigned i = 0; i < 6; ++i) {
            graphs[side][i].rotate = (Quaternion){.x = .3f, .y = -.7f, .z = 1.5f};
            graphs[side][i].scale = (Vec3){1,1,1};
            graphs[side][i].flags = (mask & 1 ? JOBJ_HIDDEN : 0) |
                (mask & 2 ? JOBJ_MTX_DIRTY : 0) | (mask & 4 ? JOBJ_MTX_INDEP_SRT : 0);
        }
        if (mask & 8) graphs[side][4].flags |= JOBJ_INSTANCE;
        fighters[side].kind = Ft_Kind_CustomFirst;
        fighters[side].x8_spawnNum = 0xFEDCBA98;
        fighters[side].motion_id = motion;
        fighters[side].facing_dir = facing;
        fighters[side].cmd_vars[2] = command;
        fighters[side].coll_data.floor.normal = (Vec3){x,y,0};
        // Opposite generated-side stock union tests owned variable selection.
        fighters[side].mv.co.common.x10 = (void*)(uintptr_t)(side ? !collision : collision);
        fighters[side].native_module_state[offset] = collision;
        creators[side] = (HSD_GObj){.user_data = &fighters[side], .hsd_obj = &origins[side]};
        fighters[side].gobj = &creators[side];
        for (unsigned i = 0; i < 2; ++i) {
            objects[side][i] = (HSD_GObj){ .classifier = HSD_GOBJ_CLASS_EFFECT,
                .user_data_kind = 8, .user_data_remove_func = efLib_remove_user_data,
                .user_data = &effects[side][i], .hsd_obj = &graphs[side][i * 3] };
            effects[side][i] = (EF_Effect){ .gobj = &objects[side][i],
                .parent_gobj = &creators[side], .attach_jobj = family ? &origins[side] : NULL,
                .native_generation = 0xCDEF0123 + i, .next = i == 0 ? &effects[side][1] : NULL };
        }
    }
    heads[HSD_GOBJ_PLINK_FIGHTER] = &creators[1];
    heads[HSD_GOBJ_PLINK_EFFECT_UNK0] = &objects[1][0];
    if (family) objects[1][0].next = &objects[1][1];
    HSD_GObjPLinkHead = heads; frees = 0;
    Vec3 position = {1,2,3};
    nativeModelEffect_Spawn(&creators[1], &active->model_effects[0], -1, family ? NULL : &position, false);
    assert(effects[1][0].update == active->model_effects[0].update);
}
static void parameters(HMODULE library, const char* path)
{
    char text[32768]; FILE* file = fopen(path, "rb"); assert(file);
    size_t size = fread(text, 1, sizeof(text), file); fclose(file); assert(size < sizeof(text));
    NativeFighterParameters values; unsigned line;
    assert(nativeFighterParameters_Parse(text, size, &values, &line));
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < values.count; ++i) bits[i] = values.fields[i].bits;
    ParameterEntry bind = (ParameterEntry)GetProcAddress(library, "FreakMelee_SetFighterParameters");
    assert(bind && bind(bits, values.count, values.schema_hash));
}
int main(int argc, char** argv)
{
    assert(argc == 5);
    NativeFighterHost host = {0}; stock_fixture_host_traps(&host);
    static MotionState common[ftCo_MS_Count], motions[ftDk_MS_SelfCount];
    host.common_motion_states = common;
    host.spawn_model_effect = nativeModelEffect_Spawn;
    host.model_effect_joint = nativeModelEffect_Joint;
    host.model_effect_creator = nativeModelEffect_Creator;
    host.model_effect_cancel = nativeModelEffect_Cancel;
    host.model_joint_resolve = nativeModelJoint_Resolve;
    host.model_joint_read = nativeModelJoint_Read; host.model_joint_write = nativeModelJoint_Write;
    host.callback_source_alive = nativeCallbackSource_Alive; host.angle_atan2_float = atan2f;
    static const float slopes[][2] = {{0,1},{-0.f,1},{.6f,.8f},{-.6f,.8f},{1,0},{-1,0},{0,-1},{0,-0.f},{0,0}};
    static const float facings[] = {-1,1,0,-0.f};
    static const u32 commands[] = {0,1,2,3,UINT32_MAX};
    for (unsigned family = 0; family < 2; ++family) {
        HMODULE library = LoadLibraryA(argv[1 + family * 2]); assert(library);
        ModuleEntry entry = (ModuleEntry)GetProcAddress(library, "FreakMelee_GetFighterModule");
        LayoutEntry layout = (LayoutEntry)GetProcAddress(library, "FreakMelee_GetFighterModuleLayout");
        assert(entry && layout && layout() == nativeFighterModule_LayoutFingerprint());
        parameters(library, argv[2 + family * 2]);
        NativeFighterDefinition base = { .fighter_root = family ? "ftDataKoopa" : "ftDataDonkey",
            .source_kind = family ? Ft_Kind_Koopa : Ft_Kind_Donkey, .motion_state_first = 341,
            .motion_state_count = family ? ftKp_MS_SelfCount : ftDk_MS_SelfCount,
            .motion_states = motions, .joint_count = family ? 76 : 75, .state_bytes = 3 };
        const FtMotionId ground[2][4] = {{369,379,381,383},{341,347,359,361}};
        const FtMotionId air[2][4] = {{374,380,382,386},{344,353,360,362}};
        memcpy(base.special_ground_state, ground[family], sizeof(base.special_ground_state));
        memcpy(base.special_air_state, air[family], sizeof(base.special_air_state));
        active = entry(&base, &host); assert(active && active->model_effect_count == 1);
        assert(active->model_effects[0].gfx_id == (family ? 0x4DA : 0x4CC));
        unsigned offset = 0;
        if (family) {
            bool found = false;
            for (unsigned i = 0; i < active->state_field_count; ++i)
                if (!strcmp(active->state_fields[i].name,"spin_floor_contact")) {
                    offset = active->state_fields[i].offset; found = true;
                }
            assert(found);
        }
        EF_UpdateFn stock = family ? efLib_Cb_ftKp_SpecialHi : efLib_Cb_SetRotY_FromFighterDir;
        unsigned start = comparisons;
        for (unsigned mask = 0; mask < 16; ++mask)
        for (unsigned motion = 358; motion <= 361; ++motion)
        for (unsigned command = 0; command < 5; ++command)
        for (unsigned collision = 0; collision < 2; ++collision)
        for (unsigned facing = 0; facing < 4; ++facing)
        for (unsigned slope = 0; slope < 9; ++slope) {
            reset(family, mask, motion, commands[command], collision, facings[facing], slopes[slope][0], slopes[slope][1], offset);
            stock(&effects[0][0]); effects[1][0].update(&effects[1][0]);
            for (unsigned i = 0; i < 6; ++i) {
                assert(graphs[0][i].flags == graphs[1][i].flags);
                assert(!memcmp(&graphs[0][i].rotate, &graphs[1][i].rotate, sizeof(Vec3)));
            }
            assert(frees == 0); ++comparisons;
        }
        for (unsigned scenario = 0; scenario < 4; ++scenario) {
            reset(family, 0, 359, 1, true, 1, .6f, .8f, offset);
            EF_UpdateFn callback = effects[1][0].update;
            if (scenario == 0) ++fighters[1].x8_spawnNum;
            if (scenario == 1) active = NULL;
            if (scenario == 2) effects[1][0].native_generation++;
            if (scenario == 3) creators[1].hsd_obj = &graphs[1][1];
            HSD_JObj before[6]; memcpy(before,graphs[1],sizeof(before));
            callback(&effects[1][0]); assert(!memcmp(before,graphs[1],sizeof(before)));
            ++lifecycle_checks;
            if (!active) active = entry(&base, &host);
        }
        printf("%s: %u exact comparisons\n",family ? "FortressModels" : "HandSlapFacing", comparisons-start);
        active = NULL; FreeLibrary(library);
    }
    printf("%u comparisons, 0 failures; %u production-registry/lifetime rejection cases\n", comparisons,lifecycle_checks);
    return 0;
}
