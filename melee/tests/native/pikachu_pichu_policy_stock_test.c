// Generated imported callbacks versus unchanged extracted stock policy.
// Terrain/visibility/item leaves are explicit boundaries; queue/RNG are real C.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/kinds/ftPikachu/forward.h>
#include <melee/ft/kinds/ftPikachu/types.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "policy_capacity.inc"

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "check failed %s:%d: %s\n", __FILE__, __LINE__, #x); exit(1); } } while (0)
#undef GET_FIGHTER
#define GET_FIGHTER(gobj) ((Fighter*)(gobj)->user_data)
#undef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, message) CHECK(condition)
static Fighter fighter, target;
static HSD_GObj source;
static struct ftData data;
static unsigned char attributes[FT_CHARACTER_EXT_ATTR_CAPACITY];
static unsigned char attribute_backup[FT_CHARACTER_EXT_ATTR_CAPACITY];
static NativeFighterParameters parameters;
static unsigned comparisons, draws, queries, notifications, stats_count, phase_slots, stock_phase_slots;
static unsigned stats_order[4];
static int visibility[4], current_visibility[4], item_flags[4], knockback[2];
static bool ray_blocked, floor_found;
static float query_args[8];
static ftCollisionBox tested_box;
static bool source_live;

static bool alive(HSD_GObj* gobj, uint32_t id, uint32_t object_class)
{
    return source_live && gobj == &source && source.user_data == &fighter &&
        id == (uint32_t)fighter.x8_spawnNum && object_class == NativeCallbackSource_Fighter;
}
bool mpCheckAll(Vec3* p, int* line, u32* flags, Vec3* n, int skip, int only,
                float x0, float y0, float x1, float y1)
{
    CHECK(p && line && flags && n && skip == -1 && only == -1); queries++;
    query_args[0] = x0; query_args[1] = y0; query_args[2] = x1; query_args[3] = y1;
    return ray_blocked;
}
bool ftCo_800A0FB0(Vec3* p, int* line, u32* flags, Vec3* n, int a, int b, int c,
                   float x0, float y0, float x1, float y1, float extra)
{
    CHECK(p && line && flags && n && a == -1 && b == -1 && c == -1 && extra == 0); queries++;
    query_args[4] = x0; query_args[5] = y0; query_args[6] = x1; query_args[7] = y1;
    return floor_found;
}
void ftParts_80074A4C(HSD_GObj* gobj, int group, int value)
{ CHECK(gobj == &source && (unsigned)group < 4); visibility[group] = value; }
void ftParts_80074B0C(HSD_GObj* gobj, int group, int value)
{ CHECK(gobj == &source && (unsigned)group < 4); current_visibility[group] = value; }
void Fighter_OnItemPickup(HSD_GObj* gobj, bool flag, bool a, bool b)
{ CHECK(gobj == &source); notifications++; item_flags[0] = flag; item_flags[1] = a; item_flags[2] = b; item_flags[3] = 1; }
void Fighter_OnItemDrop(HSD_GObj* gobj, bool flag, bool a, bool b)
{ CHECK(gobj == &source); notifications++; item_flags[0] = flag; item_flags[1] = a; item_flags[2] = b; item_flags[3] = 2; }
void Fighter_OnItemVisible(HSD_GObj* gobj, bool flag)
{ CHECK(gobj == &source); notifications++; item_flags[0] = flag; item_flags[3] = 3; }
void Fighter_OnItemInvisible(HSD_GObj* gobj, bool flag)
{ CHECK(gobj == &source); notifications++; item_flags[0] = flag; item_flags[3] = 4; }
void Fighter_OnKnockbackEnter(HSD_GObj* gobj, s32 value)
{ CHECK(gobj == &source); notifications++; knockback[0] = value; }
void Fighter_OnKnockbackExit(HSD_GObj* gobj, s32 value)
{ CHECK(gobj == &source); notifications++; knockback[1] = value; }
void ft_800892A0(HSD_GObj* gobj)
{ CHECK(gobj == &source && stats_count < 4); stats_order[stats_count++] = 1; }
void ft_80089824(HSD_GObj* gobj)
{ CHECK(gobj == &source && stats_count < 4); stats_order[stats_count++] = 2; }
void efLib_PauseAll(HSD_GObj* gobj) { CHECK(gobj == &source); }
void efLib_ResumeAll(HSD_GObj* gobj) { CHECK(gobj == &source); }
bool ft_80082888(HSD_GObj* gobj, ftCollisionBox* box)
{ CHECK(gobj == &source); tested_box = *box; return true; }
bool ft_8008239C(HSD_GObj* gobj, int facing, ftCollisionBox* box)
{ CHECK(gobj == &source && facing == (int)fighter.facing_dir); tested_box = *box; return false; }
bool ftCliffCommon_80081298(HSD_GObj* gobj) { CHECK(gobj == &source); return true; }
void ftCo_LandingFallSpecial_Enter(HSD_GObj* gobj, bool flag, float lag) { CHECK(0); }
void ftPk_SpecialHi_ChangeMotion_Unk04(HSD_GObj* gobj) { CHECK(0); }
f32 HSD_Randf(void);
#include "policy_inline.inc"
#include "policy_original.inc"
f32 HSD_Randf(void) { draws++; return original_randf(); }
static void stock_slot(HSD_GObj* gobj) { CHECK(0); }
#include "policy_base.inc"
#include "policy_generated.inc"

static void unused_host(void) { CHECK(0); }
static uint8_t costume(Fighter* fp) { CHECK(fp == &fighter); return fp->x619_costume_id; }
static void copy_attributes(HSD_GObj* gobj)
{
    CHECK(gobj == &source);
    memcpy(fighter.dat_attrs_backup, data.ext_attr, FT_CHARACTER_EXT_ATTR_CAPACITY);
    fighter.dat_attrs = fighter.dat_attrs_backup;
}
static void reload_attributes(HSD_GObj* gobj)
{ CHECK(gobj == &source); if (fighter.dat_attrs) memcpy(fighter.dat_attrs, data.ext_attr, FT_CHARACTER_EXT_ATTR_CAPACITY); }
static float parameter(const char* name)
{
    for (unsigned i = 0; i < parameters.count; i++) if (!strcmp(parameters.fields[i].name, name)) {
        float value; memcpy(&value, &parameters.fields[i].bits, sizeof(value)); return value;
    }
    CHECK(0); return 0;
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
static void reset(int kind, uint32_t initial)
{
    memset(&fighter, 0, sizeof(fighter)); memset(&target, 0, sizeof(target));
    source.user_data = &fighter; source_live = true; fighter.gobj = &source;
    fighter.kind = kind; fighter.x8_spawnNum = UINT32_C(0x80123456); fighter.facing_dir = 1;
    fighter.ft_data = &data; data.ext_attr = attributes; fighter.dat_attrs_backup = attribute_backup; fighter.dat_attrs = attribute_backup;
    fighter.x5AC.model_num = 4; fighter.cpu.write_pos = fighter.cpu.buffer;
    fighter.cpu.x44 = &target; fighter.cur_pos = (Vec3){ 11.25f, 42.75f, 0 }; target.cur_pos = (Vec3){ 19.75f, 53.25f, 0 };
    for (unsigned i = 0; i < 4; i++) visibility[i] = current_visibility[i] = 37;
    memset(item_flags, 0, sizeof(item_flags)); memset(knockback, 0, sizeof(knockback));
    memset(stats_order, 0, sizeof(stats_order)); stats_count = notifications = draws = queries = 0;
    memset(query_args, 0, sizeof(query_args)); memset(&tested_box, 0, sizeof(tested_box));
    seed = initial; ray_blocked = floor_found = false;
}
typedef struct CpuResult {
    uint32_t rng; unsigned draws, count, queries;
    unsigned candidate_count; int candidates[8]; uint8_t commands[256]; float query[8], spacing;
} CpuResult;
static CpuResult cpu_result(void)
{
    CpuResult r = {0}; r.rng = seed; r.draws = draws; r.queries = queries;
    r.count = (unsigned)(fighter.cpu.write_pos - fighter.cpu.buffer); CHECK(r.count <= sizeof(r.commands));
    memcpy(r.commands, fighter.cpu.buffer, r.count); r.candidate_count = fighter.cpu.xEC;
    memcpy(r.candidates, fighter.cpu.xCC_array, sizeof(r.candidates)); memcpy(r.query, query_args, sizeof(r.query));
    r.spacing = fighter.cpu.x56C; return r;
}
static void same_cpu(CpuResult a, CpuResult b) { CHECK(!memcmp(&a, &b, sizeof(a))); comparisons++; }
static void edit_parameter(const char* name, uint32_t bits)
{
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY]; bool found = false;
    for (unsigned i = 0; i < parameters.count; i++) {
        if (!strcmp(parameters.fields[i].name, name)) { parameters.fields[i].bits = bits; found = true; }
        words[i] = parameters.fields[i].bits;
    }
    CHECK(found); CHECK(FreakMelee_SetFighterParameters(words, parameters.count, parameters.schema_hash));
}
static uint32_t float_bits(float f) { uint32_t bits; memcpy(&bits, &f, sizeof(bits)); return bits; }

int main(int argc, char** argv)
{
    CHECK(argc == 3); load_parameters(argv[1]); int kind = !strcmp(argv[2], "pikachu") ? Ft_Kind_Pikachu : Ft_Kind_Pichu;
    NativeFighterHost host = {0};
#include "policy_required_host.inc"
    host.callback_source_alive = alive; host.costume_variant = costume;
    host.set_visibility_group = ftParts_80074A4C; host.set_current_visibility_group = ftParts_80074B0C;
    host.fighter_item_pickup = Fighter_OnItemPickup; host.fighter_item_drop = Fighter_OnItemDrop;
    host.fighter_item_visible = Fighter_OnItemVisible; host.fighter_item_invisible = Fighter_OnItemInvisible;
    host.fighter_knockback_enter = Fighter_OnKnockbackEnter; host.fighter_knockback_exit = Fighter_OnKnockbackExit;
    host.record_attack = ft_800892A0; host.refresh_attack = ft_80089824;
    host.pause_effects = efLib_PauseAll; host.resume_effects = efLib_ResumeAll;
    host.copy_ext_attributes = copy_attributes; host.reload_ext_attributes = reload_attributes;
    host.cpu_command = ftCo_800B463C; host.cpu_command_arg = ftCo_800B46B8;
    host.cpu_ranged_unsafe = ftCo_CpuRangedAttackUnsafe; host.cpu_add_candidate = ftCo_CpuAddAttackCandidate;
    host.random_float = HSD_Randf; host.ground_box_collision = ft_80082888;
    host.air_box_ledge = ft_8008239C; host.check_cliff_catch = ftCliffCommon_80081298;
    const NativeFighterDefinition* module = FreakMelee_GetFighterModule(&TEST_BASE, &host); CHECK(module);
    CHECK(module->source_kind == Ft_Kind_None && module->bone_count == 29 && module->article_logic_count == 3);
    CHECK(module->joint_count == (kind == Ft_Kind_Pikachu ? 53 : 46));
    CHECK(module->demo_motion_names == TEST_BASE.demo_motion_names && module->article_assets == TEST_BASE.article_assets);
    CHECK(module->motion_state_first == 341 && module->motion_state_count == 26);
    CHECK(!module->on_landing && !module->on_user_data_remove && !module->on_late_update);
    for (unsigned i = 0; i < module->motion_state_count; i++) {
        MotionState* old = &TEST_BASE.motion_states[i]; MotionState* now = &module->motion_states[i];
        CHECK(old->anim_id == now->anim_id && old->x4_flags == now->x4_flags && old->move_id == now->move_id && old->cam_cb == now->cam_cb);
        CHECK(now->anim_cb != NULL && now->anim_cb != old->anim_cb); phase_slots++;
        CHECK(now->input_cb != NULL && now->input_cb != old->input_cb); phase_slots++;
        CHECK(now->phys_cb != NULL && now->phys_cb != old->phys_cb); phase_slots++;
        CHECK(now->coll_cb != NULL && now->coll_cb != old->coll_cb); phase_slots++;
        stock_phase_slots += (old->anim_cb != NULL) + (old->input_cb != NULL) +
                            (old->phys_cb != NULL) + (old->coll_cb != NULL);
    }
    // Death compares final visibility. The existing Pichu script first hides
    // all headwear then reveals the selected group, an unobservable same-call
    // difference from the original per-group assignment order.
    for (unsigned costume_id = 0; costume_id < 8; costume_id++) {
        reset(kind, 1); fighter.x619_costume_id = costume_id;
        if (kind == Ft_Kind_Pikachu) ftPk_Init_OnDeath(&source); else ftPc_Init_OnDeath(&source);
        int expected[4]; memcpy(expected, visibility, sizeof(expected));
        reset(100, 1); fighter.x619_costume_id = costume_id; module->on_death(&source);
        CHECK(!memcmp(expected, visibility, sizeof(expected))); comparisons++;
    }
    for (unsigned flag = 0; flag < 2; flag++) for (unsigned event = 0; event < 8; event++) {
        int expected[4], expected_knockback[2];
        reset(kind, 1);
        if (event < 2) { if (kind == Ft_Kind_Pikachu) ftPk_Init_OnItemPickup(&source, flag); else ftPc_Init_OnItemPickup(&source, flag); }
        else if (event < 4) { if (kind == Ft_Kind_Pikachu) ftPk_Init_OnItemDrop(&source, flag); else ftPc_Init_OnItemDrop(&source, flag); }
        else if (event == 4) { if (kind == Ft_Kind_Pikachu) ftPk_Init_OnItemVisible(&source); else ftPc_Init_OnItemVisible(&source); }
        else if (event == 5) { if (kind == Ft_Kind_Pikachu) ftPk_Init_OnItemInvisible(&source); else ftPc_Init_OnItemInvisible(&source); }
        else if (event == 6) { if (kind == Ft_Kind_Pikachu) ftPk_Init_OnKnockbackEnter(&source); else ftPc_Init_OnKnockbackEnter(&source); }
        else { if (kind == Ft_Kind_Pikachu) ftPk_Init_OnKnockbackExit(&source); else ftPc_Init_OnKnockbackExit(&source); }
        memcpy(expected, item_flags, sizeof(expected)); memcpy(expected_knockback, knockback, sizeof(expected_knockback));
        reset(100, 1);
        if (event == 0) module->on_item_pickup(&source, flag); else if (event == 1) module->on_item_pickup_ext(&source, flag);
        else if (event == 2) module->on_item_drop(&source, flag); else if (event == 3) module->on_item_drop_ext(&source, flag);
        else if (event == 4) module->on_item_visible(&source); else if (event == 5) module->on_item_invisible(&source);
        else if (event == 6) module->on_knockback_enter(&source); else module->on_knockback_exit(&source);
        CHECK(notifications == 1 && !memcmp(expected, item_flags, sizeof(expected)) && !memcmp(expected_knockback, knockback, sizeof(expected_knockback))); comparisons++;
    }
    for (unsigned event = 0; event < 2; event++) {
        reset(kind, 1);
        if (kind == Ft_Kind_Pikachu) { if (event) ftPk_Init_UnkMotionStates2(&source); else ftPk_Init_UnkMotionStates1(&source); }
        int expected[4], current[4]; memcpy(expected, visibility, sizeof(expected)); memcpy(current, current_visibility, sizeof(current));
        reset(100, 1); if (event) module->on_auxiliary_item_visibility_reset(&source); else module->on_auxiliary_item_attach(&source);
        CHECK(!memcmp(expected, visibility, sizeof(expected)) && !memcmp(current, current_visibility, sizeof(current))); comparisons++;
    }
    reset(kind, 1); int flags = stock_jab_flags(&fighter); CHECK(fighter.x21EC); fighter.x21EC(&source);
    reset(100, 1); CHECK(module->on_jab_start(&source) == flags && fighter.x21EC); fighter.x21EC(&source);
    CHECK(stats_count == 2 && stats_order[0] == 1 && stats_order[1] == 2); comparisons++;
    CHECK(module->on_jab_restart(&source, ftCo_MS_Attack12) && !module->on_jab_restart(&source, ftCo_MS_Attack11)); comparisons++;
    Fighter_SetEffectHitlagCallbacks(&fighter); HSD_GObjEvent pause = fighter.pre_hitlag_cb, resume = fighter.post_hitlag_cb;
    fighter.pre_hitlag_cb = fighter.post_hitlag_cb = NULL; module->on_forward_smash_start(&source);
    CHECK(fighter.pre_hitlag_cb == pause && fighter.post_hitlag_cb == resume); comparisons++;
    const float distances[] = {-100, -60.000004f, -60, -0.0f, 0, 30, 60, 60.000004f, 100};
    for (unsigned d = 0; d < sizeof(distances)/sizeof(*distances); d++)
        for (unsigned initial = 0; initial < 1024; initial++) for (int prefix = 0; prefix < 2; prefix++) {
            CpuResult old, now;
            for (int side = 0; side < 2; side++) {
                reset(side ? 100 : kind, initial * 977u + UINT32_C(0x80123456)); fighter.cur_pos.x = distances[d]; fighter.cpu.x54.x = 0;
                if (prefix) ftCo_800B46B8(&fighter, CpuCmd_SetLstickX, 17);
                if (side) { module->on_cpu_recovery(&source); now = cpu_result(); }
                else { ftCo_800A92CC(&fighter); old = cpu_result(); }
            }
            same_cpu(old, now);
        }
    for (unsigned count = 0; count <= 8; count++) for (unsigned conditions = 0; conditions < 16; conditions++) {
        CpuResult old, now;
        for (int side = 0; side < 2; side++) {
            reset(side ? 100 : kind, 91); fighter.facing_dir = conditions & 1 ? -1 : 1;
            ray_blocked = conditions & 2; floor_found = conditions & 4; if (conditions & 8) fighter.cpu.x44 = NULL;
            fighter.cpu.xEC = count; for (unsigned i = 0; i < 8; i++) fighter.cpu.xCC_array[i] = (int)i + 100;
            if (side) { module->on_cpu_ranged_candidates(&source); now = cpu_result(); }
            else { stock_ranged(&fighter); old = cpu_result(); }
        }
        same_cpu(old, now);
    }
    for (unsigned initial = 0; initial < 512; initial++) {
        reset(kind, initial * 991u + 3); fighter.cpu.x7C = 600; ftCo_800A0CB0(&fighter); CpuResult old = cpu_result();
        reset(100, initial * 991u + 3); float x = HSD_Randf(); x = x * x * x; module->on_cpu_spacing_update(&source, 1.0f - x);
        same_cpu(old, cpu_result());
    }
    reset(kind, 1); stock_initialize(&fighter); float initial_spacing = fighter.cpu.x56C;
    reset(100, 1); module->on_cpu_initialize(&source); CHECK(fighter.cpu.x56C == initial_spacing); comparisons++;
    for (int motion = 0; motion <= 370; motion++) {
        fighter.motion_id = motion; CHECK(module->on_cpu_attack_motion(&source) == (motion >= 341)); comparisons++;
    }
    CHECK(!module->on_cpu_grab_motion(&source) && !module->on_cpu_attack_decision(&source) && !module->on_cpu_capture_decision(&source));
    CHECK(!module->on_cpu_pummel_special(&source) && !module->on_cpu_charge_decision(&source) && module->on_cpu_ranged_allowed(&source));
    CHECK(!module->on_cpu_threat(&source, &source)); module->on_cpu_charge_cancel(&source); comparisons++;
    const char* box_names[] = {"hi_box_top", "hi_box_bottom", "hi_box_left_x", "hi_box_left_y", "hi_box_right_x", "hi_box_right_y"};
    for (unsigned i = 0; i < 6; i++) { float value = parameter(box_names[i]); memcpy(attributes + 224 + 4*i, &value, sizeof(value)); }
    const float scales[] = {0, 0.5f, 1, 1.25f, 2, 3.125f};
    for (unsigned s = 0; s < sizeof(scales)/sizeof(*scales); s++) for (int air = 0; air < 2; air++) {
        reset(kind, 1); fighter.x34_scale.y = scales[s]; ftPk_Init_LoadSpecialAttrs(&source);
        if (air) ftPk_SpecialAirHiEnd_Coll(&source); else ftPk_SpecialHiEnd_Coll(&source);
        ftCollisionBox expected = tested_box;
        reset(100, 1); fighter.x34_scale.y = scales[s]; module->on_load_special_attrs(&source);
        if (air) module->motion_states[358 - 341].coll_cb(&source); else module->motion_states[355 - 341].coll_cb(&source);
        CHECK(!memcmp(&expected, &tested_box, sizeof(expected))); comparisons++;
    }
    reset(100, 1); module->on_load(&source); CHECK(fighter.can_walljump == (kind == Ft_Kind_Pichu));
    CHECK(!memcmp(fighter.dat_attrs, attributes, sizeof(ftPikachuAttributes))); comparisons++;
    // Editable defaults remain data, not compiled family decisions.
    edit_parameter("cpu_initial_spacing", float_bits(6.25f)); module->on_cpu_initialize(&source); CHECK(fighter.cpu.x56C == 6.25f);
    edit_parameter("cpu_spacing_range", float_bits(9)); module->on_cpu_spacing_update(&source, 0.25f); CHECK(fighter.cpu.x56C == 2.25f);
    edit_parameter("cpu_recovery_horizontal_threshold", float_bits(20)); edit_parameter("cpu_recovery_wait", 7); edit_parameter("cpu_recovery_diagonal_stick", 73);
    reset(100, 71); fighter.cur_pos.x = 30; module->on_cpu_recovery(&source); CHECK(draws == 0);
    CHECK(fighter.cpu.buffer[8] == 73 && fighter.cpu.buffer[10] == 73 && fighter.cpu.buffer[12] == 7);
    edit_parameter("cpu_ranged_probe_distance", float_bits(19)); reset(100, 3); module->on_cpu_ranged_candidates(&source);
    CHECK(query_args[4] == fighter.cur_pos.x + 19); unsigned edits = 4;
    printf("PASS %u stock policy comparisons, %u owned motion phases (%u originally non-null), %u edited-parameter checks (%s)\n", comparisons, phase_slots, stock_phase_slots, edits, argv[2]);
    return 0;
}
