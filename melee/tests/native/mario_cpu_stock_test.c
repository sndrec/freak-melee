// Generated callbacks versus stock source; no engine process is launched.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/kinds/ftMario/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(value) do { if (!(value)) { fprintf(stderr, "check failed %s:%d: %s\n", __FILE__, __LINE__, #value); exit(1); } } while (0)
static unsigned checks;
static NativeFighterParameters owned_parameters;
static Fighter fighter, target;
static HSD_GObj source;
static int ray_blocked, floor_found;
static float query_args[12];
static unsigned query_count, random_calls, run_calls;
static int run_index;
struct Fighter_804D64FC_t* Fighter_804D64FC;

bool mpCheckAll(Vec3* p, int* line, u32* flags, Vec3* n, int skip,
                int only, float x0, float y0, float x1, float y1)
{
    CHECK(skip == -1 && only == -1);
    CHECK(p && line && flags && n);
    query_count++;
    query_args[0] = x0; query_args[1] = y0;
    query_args[2] = x1; query_args[3] = y1;
    return ray_blocked;
}
bool ftCo_800A0FB0(Vec3* p, int* line, u32* flags, Vec3* n,
                   int a, int b, int c, float x0, float y0,
                   float x1, float y1, float extra)
{
    CHECK(a == -1 && b == -1 && c == -1 && extra == 0);
    CHECK(p && line && flags && n);
    query_count++;
    query_args[4] = x0; query_args[5] = y0;
    query_args[6] = x1; query_args[7] = y1;
    return floor_found;
}
// Overflow is intentionally excluded: the real source command buffer's
// assertion is not a gameplay policy and both callers receive ample capacity.
#undef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, message) CHECK(condition)
bool ftCo_CpuRangedAttackUnsafe(Fighter*, Fighter*, float);
f32 HSD_Randf(void);
#include "mario_cpu_original.inc"

static void stock_slot(HSD_GObj* gobj) { (void)gobj; }
#include "mario_owned_base.inc"
#include "mario_raw_rigs.inc"
#include "mario_generated.inc"

static void unused_host(void) { CHECK(0); }
static bool alive(HSD_GObj* gobj, uint32_t generation, uint32_t object_class)
{
    return gobj == &source && source.user_data == &fighter &&
           generation == (uint32_t)fighter.x8_spawnNum &&
           object_class == NativeCallbackSource_Fighter;
}
f32 HSD_Randf(void) { random_calls++; return original_randf(); }
void ftCo_800B4880(Fighter* fp, int index)
{
    run_calls++; run_index = index;
    original_command_script(fp, index);
}
static float random_float(void) { return HSD_Randf(); }

typedef struct Result {
    bool consumed;
    uint32_t random_seed;
    unsigned draws, runs, queries;
    int index, attempts, queue[8];
    unsigned queue_count, command_count;
    unsigned char commands[64];
    float query[12];
} Result;

static Result result(bool consumed)
{
    Result r = { 0 };
    r.consumed = consumed; r.random_seed = seed;
    r.draws = random_calls; r.runs = run_calls; r.index = run_index;
    r.attempts = fighter.cpu.x94; r.queue_count = fighter.cpu.xEC;
    memcpy(r.queue, fighter.cpu.xCC_array, sizeof(r.queue));
    r.command_count = (unsigned)(fighter.cpu.write_pos - fighter.cpu.buffer);
    CHECK(r.command_count <= sizeof(r.commands));
    memcpy(r.commands, fighter.cpu.buffer, r.command_count);
    r.queries = query_count; memcpy(r.query, query_args, sizeof(r.query));
    return r;
}
static void reset(int level, int attempts, uint32_t initial_seed)
{
    memset(&fighter, 0, sizeof(fighter));
    fighter.kind = Ft_Kind_Mario; fighter.x8_spawnNum = 0x80000027u;
    fighter.cpu.level = level; fighter.cpu.x94 = attempts;
    fighter.cpu.write_pos = fighter.cpu.buffer;
    source.user_data = &fighter;
    seed = initial_seed; random_calls = run_calls = query_count = 0;
    run_index = -1; memset(query_args, 0, sizeof(query_args));
}
static bool reference_capture(void)
{
    return stock_capture(&fighter);
}

static void load_parameters(const char* path)
{
    char text[32768]; FILE* file = fopen(path, "rb"); CHECK(file != NULL);
    size_t size = fread(text, 1, sizeof(text), file); CHECK(!ferror(file));
    CHECK(feof(file)); fclose(file);
    unsigned error = 0;
    CHECK(nativeFighterParameters_Parse(text, size, &owned_parameters, &error));
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < owned_parameters.count; ++i) words[i] = owned_parameters.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(words, owned_parameters.count, owned_parameters.schema_hash));
}

static void edit_parameter(const char* name, uint32_t value)
{
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY]; bool found = false;
    for (unsigned i = 0; i < owned_parameters.count; ++i) {
        if (!strcmp(owned_parameters.fields[i].name, name)) {
            owned_parameters.fields[i].bits = value; found = true;
        }
        words[i] = owned_parameters.fields[i].bits;
    }
    CHECK(found);
    CHECK(FreakMelee_SetFighterParameters(words, owned_parameters.count, owned_parameters.schema_hash));
}
static uint32_t float_bits(float value)
{
    uint32_t bits; memcpy(&bits, &value, sizeof(bits)); return bits;
}

int main(int argc, char** argv)
{
    CHECK(argc == 3); load_parameters(argv[1]);
    NativeFighterHost host = { 0 };
#include "mario_required_host.inc"
    host.callback_source_alive = alive;
    host.random_float = random_float;
    host.cpu_run_script = ftCo_800B4880;
    host.cpu_ranged_unsafe = ftCo_CpuRangedAttackUnsafe;
    host.cpu_add_candidate = ftCo_CpuAddAttackCandidate;
    const NativeFighterDefinition* module = FreakMelee_GetFighterModule(&ftMarioClone_Definition, &host);
    CHECK(module != NULL);
    CHECK(module->source_kind == (atoi(argv[2]) ? Ft_Kind_None : Ft_Kind_Mario));
    CHECK(module->bone_count == 51 && module->joint_count == 61);
    CHECK(module->bones == ftMarioClone_Bones && module->article_assets == ftMarioClone_ArticleAssets);
    CHECK(module->article_logic_count == 2 && module->effect_bank == 1);
    CHECK(module->demo_motion_names == &ftMarioClone_DemoMotionNames);
    CHECK(module->action_count == 303 && module->demo_count == 16);
    CHECK(module->sound_banks == (UINT64_C(1) << 18));
    CHECK(module->asset_layout.article_mask == 0x005);
#define ENTRY(field) CHECK(module->field && module->field != ftMarioClone_Definition.field)
    ENTRY(special_n); ENTRY(special_air_n); ENTRY(special_s); ENTRY(special_air_s);
    ENTRY(special_hi); ENTRY(special_air_hi); ENTRY(special_lw); ENTRY(special_air_lw);
#undef ENTRY
    if (atoi(argv[2])) CHECK(module->neutral_article == NULL);
    CHECK(module->on_load && module->on_load_special_attrs && module->on_death);
    CHECK(module->on_item_pickup_ext && module->on_item_drop_ext);
    CHECK(module->on_item_pickup && module->on_item_drop);
    CHECK(module->on_item_visible && module->on_item_invisible);
    CHECK(module->on_knockback_enter && module->on_knockback_exit);
    for (unsigned i = 0; i < 51; ++i) {
        CHECK(ftMarioClone_Bones[i].part == rawMario[i][0]);
        CHECK(ftMarioClone_Bones[i].joint == rawMario[i][1]);
        CHECK(ftDrMarioClone_Bones[i].part == rawDrMario[i][0]);
        CHECK(ftDrMarioClone_Bones[i].joint == rawDrMario[i][1]);
    }
    unsigned phases = 0;
    for (unsigned i = 0; i < module->motion_state_count; ++i) {
        const MotionState* old = &ftMr_Init_MotionStateTable[i];
        const MotionState* now = &module->motion_states[i];
#define PHASE(field) if (old->field) { CHECK(now->field && now->field != old->field); phases++; } else { CHECK(now->field == NULL); }
        PHASE(anim_cb); PHASE(input_cb); PHASE(phys_cb); PHASE(coll_cb);
#undef PHASE
        CHECK(now->cam_cb == old->cam_cb);
    }
    CHECK(phases == 32);
    if (atoi(argv[2])) {
        MotionState invalid[ftMr_MS_SelfCount];
        memcpy(invalid, ftMr_Init_MotionStateTable, sizeof(invalid));
        invalid[0].anim_cb = stock_slot;
        NativeFighterDefinition incomplete = ftMarioClone_Definition;
        incomplete.motion_states = invalid;
        CHECK(FreakMelee_GetFighterModule(&incomplete, &host) == NULL);
        module = FreakMelee_GetFighterModule(&ftMarioClone_Definition, &host);
        CHECK(module != NULL);
    }

    // Exercise the actual source command parser with one-/two-argument commands
    // and Done, rather than reducing run_script to a counter-only mock.
    uint8_t command[] = { CpuCmd_PressBFor, 1, CpuCmd_SetLstickX, 88,
                         CpuCmd_LstickForwardClamped, 0x90, 0x34, CpuCmd_Done };
    uint8_t* scripts[FT_CPU_SCRIPT_COUNT] = { 0 };
    scripts[53] = command;
    scripts[52] = command;
    struct Fighter_804D64FC_t commands = { .cmdscripts = scripts };
    Fighter_804D64FC = &commands;
    for (int level = 0; level <= 9; ++level)
        for (int attempts = 0; attempts < 3; ++attempts)
            for (uint32_t initial = 0; initial < 1024; ++initial) {
                reset(level, attempts, initial * 977u + 0x80123456u);
                Result old = result(reference_capture());
                reset(level, attempts, initial * 977u + 0x80123456u);
                Result now = result(module->on_cpu_capture_decision(&source));
                CHECK(memcmp(&old, &now, sizeof(old)) == 0); checks++;
            }
    for (int facing = -1; facing <= 1; ++facing)
        for (int has_target = 0; has_target < 2; ++has_target)
            for (int blocked = 0; blocked < 2; ++blocked)
                for (int floor = 0; floor < 2; ++floor)
                    for (unsigned count = 0; count <= 8; ++count)
                        for (unsigned position = 0; position < 3; ++position) {
                            Result old, now;
                            for (unsigned side = 0; side < 2; ++side) {
                                reset(9, 2, 0x12345678u);
                                fighter.facing_dir = (float)facing;
                                fighter.cur_pos.x = position == 0 ? -19.25f : position == 1 ? 0 : 81.125f;
                                fighter.cur_pos.y = position == 0 ? -3.5f : position == 1 ? 0 : 24.25f;
                                target.cur_pos.x = 67.5f; target.cur_pos.y = 22.25f;
                                fighter.cpu.x44 = has_target ? &target : NULL;
                                fighter.cpu.xEC = count;
                                for (unsigned i = 0; i < 8; ++i) fighter.cpu.xCC_array[i] = 1000 + i;
                                ray_blocked = blocked; floor_found = floor;
                                if (side == 0) { stock_ranged(&fighter); old = result(false); }
                                else { module->on_cpu_ranged_candidates(&source); now = result(false); }
                            }
                            CHECK(memcmp(&old, &now, sizeof(old)) == 0); checks++;
                        }
    // Exact production bounds: invalid commands or a full queue do not append.
    for (int candidate = -1; candidate <= FT_CPU_SCRIPT_COUNT; ++candidate) {
        reset(0, 0, 7); fighter.cpu.xEC = 7;
        ftCo_CpuAddAttackCandidate(&fighter, candidate);
        CHECK(fighter.cpu.xEC == (candidate > 0 && candidate < FT_CPU_SCRIPT_COUNT ? 8 : 7));
        checks++;
    }
    // Parameter changes are separate authored-policy checks, not stock parity.
    edit_parameter("cpu_capture_first_attempt", 1);
    edit_parameter("cpu_capture_level_limit", 8);
    edit_parameter("cpu_capture_chance_per_level", float_bits(1.0f));
    edit_parameter("cpu_capture_script", 52);
    reset(7, 1, 87);
    CHECK(module->on_cpu_capture_decision(&source));
    CHECK(random_calls == 1 && run_calls == 1 && run_index == 52);
    CHECK(fighter.cpu.write_pos - fighter.cpu.buffer == sizeof(command));
    CHECK(fighter.cpu.x94 == 1);
    reset(8, 1, 87);
    CHECK(!module->on_cpu_capture_decision(&source)); CHECK(random_calls == 0 && run_calls == 0);
    reset(7, 0, 87);
    CHECK(!module->on_cpu_capture_decision(&source)); CHECK(random_calls == 0 && run_calls == 0);
    edit_parameter("cpu_fireball_range", float_bits(50.0f));
    edit_parameter("cpu_fireball_candidate", 47);
    reset(9, 0, 87); fighter.facing_dir = 1;
    fighter.cur_pos.x = 12.25f; fighter.cur_pos.y = 3.5f;
    floor_found = 0;
    module->on_cpu_ranged_candidates(&source);
    CHECK(fighter.cpu.xEC == 1 && fighter.cpu.xCC_array[0] == 47);
    CHECK(query_count == 1 && query_args[4] == 62.25f && query_args[5] == 8.5f);
    printf("Mario source comparisons passed: %u CPU cases; 4 parameter checks; %u owned motion phases; 51 rig pairs each; independent=%s\n", checks, phases, argv[2]);
    return 0;
}
