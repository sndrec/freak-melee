// Full extracted capture/defense callers and generated template callbacks.
// Terrain leaves are controlled boundaries; queue writes and RNG are original C.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/kinds/ftFox/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/types.h>
#include <melee/mp/mpisland.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "check failed %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
#undef GET_FIGHTER
#define GET_FIGHTER(gobj) ((Fighter*) (gobj)->user_data)
#undef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, message) CHECK(condition)
static unsigned comparisons, draws, finished, queries, hook_calls;
static Fighter fighter, target;
static HSD_GObj source, victim;
static Item threat;
static bool source_live, grabbing, has_island, ray_blocked, floor_found;
static int threat_blocked;
static float island_left, island_right, query_args[12];
static NativeFighterParameters parameters;
static const NativeFighterDefinition* registered;
struct Fighter_804D64FC_t* Fighter_804D64FC;

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == fighter.kind ? registered : NULL; }
bool nativeCallbackSource_Alive(HSD_GObj* gobj, uint32_t id, uint32_t object_class)
{
    return source_live && gobj == &source && source.user_data == &fighter &&
           id == (uint32_t)fighter.x8_spawnNum && object_class == NativeCallbackSource_Fighter;
}
static void ftCo_800A0C8C(Fighter* fp) { CHECK(fp == &fighter); finished++; }
static bool ftCo_IsGrabbing(Fighter* fp) { CHECK(fp == &fighter); return grabbing; }
mp_UnkStruct0* mpIsland_8005AC14(Vec3* point, float offset)
{
    CHECK(point == &fighter.cur_pos && offset == -10); queries++;
    return has_island ? (mp_UnkStruct0*)&threat : NULL;
}
void mpIsland_8005ACE8(mp_UnkStruct0* island, Vec3* left, Vec3* right)
{
    CHECK(island == (mp_UnkStruct0*)&threat);
    *left = (Vec3){ island_left, 0, 0 }; *right = (Vec3){ island_right, 0, 0 };
}
static int ftCo_800A2998(Fighter* fp, float value)
{ CHECK(fp == &fighter && value == 1); return threat_blocked; }
bool mpCheckAll(Vec3* p, int* line, u32* flags, Vec3* n, int skip,
                int only, float x0, float y0, float x1, float y1)
{
    CHECK(p && line && flags && n && skip == -1 && only == -1);
    queries++; query_args[0] = x0; query_args[1] = y0;
    query_args[2] = x1; query_args[3] = y1; return ray_blocked;
}
bool ftCo_800A0FB0(Vec3* p, int* line, u32* flags, Vec3* n, int a, int b, int c,
                   float x0, float y0, float x1, float y1, float extra)
{
    CHECK(p && line && flags && n && a == -1 && b == -1 && c == -1 && extra == 0);
    queries++; query_args[4] = x0; query_args[5] = y0;
    query_args[6] = x1; query_args[7] = y1; return floor_found;
}
f32 HSD_Randf(void);
#include "fox_cpu_original_helpers.inc"
f32 HSD_Randf(void) { draws++; return original_randf(); }
#include "fox_cpu_steps.inc"
static void stock_slot(HSD_GObj* gobj) { (void)gobj; }
#include "fox_cpu_base.inc"
#include "fox_cpu_generated.inc"
static void unused_host(void) { CHECK(0); }
static float random_float(void) { return HSD_Randf(); }

typedef struct Result {
    uint32_t rng;
    unsigned draws, finished, queries, count;
    int attempts, behavior, candidates[8];
    unsigned char commands[256];
    unsigned candidate_count;
    float query[12];
} Result;
static Result result(void)
{
    Result r = {0}; r.rng = seed; r.draws = draws; r.finished = finished; r.queries = queries;
    r.attempts = fighter.cpu.x94; r.behavior = fighter.cpu.x18;
    r.count = (unsigned)(fighter.cpu.write_pos - fighter.cpu.buffer);
    CHECK(r.count <= sizeof(r.commands)); memcpy(r.commands, fighter.cpu.buffer, r.count);
    r.candidate_count = fighter.cpu.xEC; memcpy(r.candidates, fighter.cpu.xCC_array, sizeof(r.candidates));
    memcpy(r.query, query_args, sizeof(r.query)); return r;
}
static void same(Result old, Result now) { CHECK(!memcmp(&old, &now, sizeof(old))); comparisons++; }
static void reset(int kind, int level, int attempts, uint32_t initial)
{
    memset(&fighter, 0, sizeof(fighter)); memset(&target, 0, sizeof(target));
    memset(&threat, 0, sizeof(threat)); memset(query_args, 0, sizeof(query_args));
    source_live = true; source.user_data = &fighter; victim.user_data = &target;
    fighter.gobj = &source; fighter.kind = kind; fighter.x8_spawnNum = 0x80123459u;
    fighter.cpu.level = level; fighter.cpu.x94 = attempts; fighter.cpu.x18 = 17; fighter.cpu.x1C = 12;
    fighter.cpu.write_pos = fighter.cpu.buffer; fighter.facing_dir = 1;
    fighter.victim_gobj = &victim; fighter.motion_id = ftCo_MS_CatchWait;
    fighter.cur_pos.x = 8.5f; target.cur_pos.x = 80.25f;
    grabbing = true; has_island = false; threat_blocked = 0; ray_blocked = floor_found = false;
    island_left = -15; island_right = 54;
    seed = initial; draws = finished = queries = hook_calls = 0; registered = NULL;
}
static void load_parameters(const char* path)
{
    char text[32768]; FILE* file = fopen(path, "rb"); CHECK(file);
    size_t size = fread(text, 1, sizeof(text), file); CHECK(!ferror(file) && feof(file)); fclose(file);
    unsigned error = 0; CHECK(nativeFighterParameters_Parse(text, size, &parameters, &error));
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for (unsigned i = 0; i < parameters.count; i++) words[i] = parameters.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(words, parameters.count, parameters.schema_hash));
}
static void edit_parameter(const char* name, uint32_t bits)
{
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY]; bool found = false;
    for (unsigned i = 0; i < parameters.count; i++) {
        if (!strcmp(parameters.fields[i].name, name)) { parameters.fields[i].bits = bits; found = true; }
        words[i] = parameters.fields[i].bits;
    }
    CHECK(found); CHECK(FreakMelee_SetFighterParameters(words, parameters.count, parameters.schema_hash));
}
static uint32_t float_bits(float value) { uint32_t bits; memcpy(&bits, &value, sizeof(bits)); return bits; }
static int mutation;
static bool mutating_defense(HSD_GObj* gobj, int category)
{
    CHECK(gobj == &source && category >= 1 && category <= 3); hook_calls++;
    if (mutation == 1) source_live = false;
    if (mutation == 2) fighter.x8_spawnNum++;
    return false;
}
static bool mutating_capture(HSD_GObj* gobj)
{ CHECK(gobj == &source); return mutating_defense(gobj, 1); }

int main(int argc, char** argv)
{
    CHECK(argc == 3); load_parameters(argv[1]);
    int kind = !strcmp(argv[2], "fox") ? Ft_Kind_Fox : Ft_Kind_Falco;
    NativeFighterHost host = {0};
#include "fox_cpu_required_host.inc"
    host.callback_source_alive = nativeCallbackSource_Alive; host.random_float = random_float;
    host.cpu_command = ftCo_800B463C; host.cpu_command_arg = ftCo_800B46B8; host.cpu_command_pair = ftCo_800B4778;
    host.cpu_run_script = ftCo_800B4880; host.cpu_ranged_unsafe = ftCo_CpuRangedAttackUnsafe;
    host.cpu_add_candidate = ftCo_CpuAddAttackCandidate;
    const NativeFighterDefinition* module = FreakMelee_GetFighterModule(&TEST_BASE, &host); CHECK(module);
    CHECK(module->on_cpu_capture_decision && module->on_cpu_projectile_defense && module->on_cpu_recovery && module->on_cpu_ranged_candidates);
    // Test the imported variant against the stock resource identity explicitly.
    bool variant_found = false;
    for (unsigned i = 0; i < parameters.count; i++) if (!strcmp(parameters.fields[i].name, "blaster_gun_kind")) {
        CHECK((int)parameters.fields[i].bits == (kind == Ft_Kind_Fox ? It_Kind_Fox_Blaster : It_Kind_Falco_Blaster)); variant_found = true;
    }
    CHECK(variant_found);
    uint8_t command[] = { CpuCmd_PressBFor, 1, CpuCmd_SetLstickX, 88, CpuCmd_LstickForwardClamped, 0x90, 0x34, CpuCmd_Done };
    uint8_t* scripts[FT_CPU_SCRIPT_COUNT]; for (unsigned i = 0; i < FT_CPU_SCRIPT_COUNT; i++) scripts[i] = command;
    struct Fighter_804D64FC_t commands = { .cmdscripts = scripts }; Fighter_804D64FC = &commands;
    for (int level = 0; level <= 9; level++)
        for (int attempts = 0; attempts < 3; attempts++)
            for (int conditions = 0; conditions < 6; conditions++)
                for (uint32_t initial = 0; initial < 128; initial++) {
                    Result old, now;
                    for (int side = 0; side < 2; side++) {
                        reset(side ? 100 : kind, level, attempts, initial * 977u + 0x80123456u);
                        if (conditions == 0) fighter.victim_gobj = NULL;
                        grabbing = conditions != 1; has_island = conditions >= 3;
                        if (conditions == 4) fighter.cur_pos.x = -10;
                        if (conditions == 5) fighter.cur_pos.x = 50;
                        if (side) { registered = module; owned_capture_step(&fighter); now = result(); }
                        else { stock_capture_step(&fighter); old = result(); }
                    }
                    same(old, now);
                }
    const int motions[] = {ftCo_MS_Wait, ftCo_MS_Guard, ftCo_MS_EscapeF, ftCo_MS_EscapeB, ftCo_MS_EscapeAir, ftCo_MS_EscapeN};
    const int items[] = {It_Kind_Fox_Laser, It_Kind_Flipper, It_Kind_TaruCann, It_Kind_DKinoko};
    for (int level = 0; level <= 9; level++)
        for (int air = 0; air < 2; air++)
            for (int category = 0; category <= 3; category++)
                for (unsigned motion = 0; motion < sizeof(motions)/sizeof(*motions); motion++)
                    for (unsigned item = 0; item < sizeof(items)/sizeof(*items); item++)
                        for (uint32_t initial = 0; initial < 64; initial++) {
                            Result old, now;
                            for (int side = 0; side < 2; side++) {
                                reset(side ? 100 : kind, level, 2, initial * 991u + 0x92345678u);
                                fighter.ground_or_air = air ? GA_Air : GA_Ground; fighter.motion_id = motions[motion];
                                fighter.cpu.xF8_b12 = category; threat.kind = items[item]; threat.pos.x = initial & 1 ? -4 : 20;
                                fighter.cpu.xF4 = &threat; fighter.cpu.xF0 = initial & 2 ? &target : NULL;
                                threat_blocked = initial & 4;
                                if (side) { registered = module; owned_defense_step(&fighter); now = result(); }
                                else { stock_defense_step(&fighter); old = result(); }
                            }
                            same(old, now);
                        }
    // No hook remains exactly the original stock path, including its RNG draw.
    for (int air = 0; air < 2; air++) for (int category = 0; category <= 3; category++) {
        reset(kind, 5, 0, 87); fighter.ground_or_air = air; fighter.cpu.xF8_b12 = category; fighter.cpu.xF4 = &threat;
        stock_defense_step(&fighter); Result old = result();
        reset(kind, 5, 0, 87); fighter.ground_or_air = air; fighter.cpu.xF8_b12 = category; fighter.cpu.xF4 = &threat;
        owned_defense_step(&fighter); same(old, result());
    }
    for (int facing = -1; facing <= 1; facing++) for (int target_present = 0; target_present < 2; target_present++)
        for (int blocked = 0; blocked < 2; blocked++) for (int floor = 0; floor < 2; floor++)
            for (unsigned count = 0; count <= 8; count++) {
                Result old, now;
                for (int side = 0; side < 2; side++) {
                    reset(kind, 9, 2, 23); fighter.facing_dir = facing; fighter.cur_pos.y = 3.25f;
                    fighter.cpu.x44 = target_present ? &target : NULL; fighter.cpu.xEC = count;
                    for (unsigned i = 0; i < 8; i++) fighter.cpu.xCC_array[i] = 1000 + i;
                    ray_blocked = blocked; floor_found = floor;
                    if (side) { module->on_cpu_ranged_candidates(&source); now = result(); }
                    else { stock_ranged(&fighter); old = result(); }
                }
                same(old, now);
            }
    reset(kind, 9, 0, 77); stock_recovery(&fighter); Result old = result();
    reset(kind, 9, 0, 77); module->on_cpu_recovery(&source); same(old, result());
    // A surviving false hook continues common defense; removed/recycled sources
    // cannot consume RNG, write commands, finish a step or touch capture counters.
    NativeFighterDefinition mutant = *module;
    mutant.on_cpu_projectile_defense = mutating_defense; mutant.on_cpu_capture_decision = mutating_capture;
    for (mutation = 0; mutation <= 2; mutation++) for (int air = 0; air < 2; air++) {
        reset(kind, 5, 0, 17); registered = &mutant; fighter.ground_or_air = air;
        fighter.motion_id = ftCo_MS_Wait; fighter.cpu.xF8_b12 = air ? 3 : 2; fighter.cpu.xF4 = &threat;
        owned_defense_step(&fighter); CHECK(hook_calls == 1);
        CHECK(draws == 0 && finished == 0 && fighter.cpu.x94 == 0);
        CHECK((fighter.cpu.write_pos != fighter.cpu.buffer) == (mutation == 0)); comparisons++;
        reset(kind, 5, 0, 17); registered = &mutant; owned_capture_step(&fighter); CHECK(hook_calls == 1);
        if (mutation) CHECK(draws == 0 && finished == 0 && fighter.cpu.write_pos == fighter.cpu.buffer && fighter.cpu.x94 == 0);
        else CHECK(draws > 0); comparisons++;
    }
    // Editing FMP values proves decisions remain owned, independent of a kind.
    edit_parameter("cpu_capture_attempt", 1); edit_parameter("cpu_capture_min_level", 6);
    edit_parameter("cpu_capture_chance_per_level", float_bits(1)); edit_parameter("cpu_capture_script", 52);
    edit_parameter("blaster_gun_kind", It_Kind_Fox_Blaster);
    reset(100, 7, 1, 87); CHECK(module->on_cpu_capture_decision(&source)); CHECK(draws == 1 && fighter.cpu.x94 == 1);
    CHECK(fighter.cpu.write_pos - fighter.cpu.buffer == sizeof(command)); comparisons++;
    reset(100, 6, 1, 87); CHECK(!module->on_cpu_capture_decision(&source)); CHECK(draws == 0); comparisons++;
    edit_parameter("cpu_ground_reflect_threshold", float_bits(1)); reset(100, 7, 0, 87);
    CHECK(!module->on_cpu_projectile_defense(&source, 2)); CHECK(draws == 1 && fighter.cpu.write_pos == fighter.cpu.buffer); comparisons++;
    edit_parameter("cpu_ground_reflect_threshold", float_bits(-1)); edit_parameter("cpu_reflect_stick_y", -64);
    edit_parameter("cpu_reflect_level_ceiling", 7); reset(100, 7, 0, 87);
    CHECK(module->on_cpu_projectile_defense(&source, 2));
    CHECK(draws == 1 && (u8)fighter.cpu.buffer[3] == (u8)-64); comparisons++;
    edit_parameter("cpu_blaster_range", float_bits(50)); edit_parameter("cpu_blaster_candidate", 47);
    reset(100, 9, 0, 87); module->on_cpu_ranged_candidates(&source);
    CHECK(fighter.cpu.xEC == 1 && fighter.cpu.xCC_array[0] == 47 && query_args[4] == 58.5f); comparisons++;
    edit_parameter("cpu_recover_stick", 77); edit_parameter("cpu_recover_wait_frames", 29);
    reset(100, 9, 0, 87); module->on_cpu_recovery(&source);
    CHECK(fighter.cpu.buffer[1] == 77 && fighter.cpu.buffer[3] == 77 && fighter.cpu.buffer[8] == 29); comparisons++;
    printf("%s CPU source comparisons passed: %u; full capture/defense callers, RNG, counters, queue capacity, recovery, parameters, lifetime\n", argv[2], comparisons);
    return 0;
}
