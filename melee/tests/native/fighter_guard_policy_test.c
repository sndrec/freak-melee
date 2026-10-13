#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <string.h>

// Only explicitly registered fixture objects are live; compare full IDs/classes.
static HSD_GObj* live_fighter;
static HSD_GObj* live_item;
static bool callback_source_alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    if (source_class == NativeCallbackSource_Fighter && object == live_fighter && object && object->user_data)
        return (uint32_t)((Fighter*)object->user_data)->x8_spawnNum == generation;
    if (source_class == NativeCallbackSource_Item && object == live_item && object && object->user_data)
        return (uint32_t)((Item*)object->user_data)->x1C == generation;
    return false;
}

static bool callback_host_contract(void)
{
    Fighter source = { .x8_spawnNum = UINT32_C(0xFEDCBA98) };
    Item article = { .x1C = UINT32_C(0x80000001) };
    HSD_GObj owner = { .user_data = &source }, object = { .user_data = &article };
    live_fighter = &owner; live_item = &object;
    bool result = callback_source_alive(&owner, UINT32_C(0xFEDCBA98), NativeCallbackSource_Fighter) &&
        callback_source_alive(&object, UINT32_C(0x80000001), NativeCallbackSource_Item) &&
        !callback_source_alive(&owner, UINT32_C(0x7EDCBA98), NativeCallbackSource_Fighter) &&
        !callback_source_alive(&object, UINT32_C(1), NativeCallbackSource_Item) &&
        !callback_source_alive(&owner, source.x8_spawnNum, NativeCallbackSource_Item) &&
        !callback_source_alive(&object, article.x1C, NativeCallbackSource_Fighter) &&
        !callback_source_alive(NULL, 0, NativeCallbackSource_Item);
    live_fighter = NULL; live_item = NULL;
    return result;
}

typedef const NativeFighterDefinition* (__cdecl* Entry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
static Fighter fighter, caught;
static HSD_GObj object, caught_object;
static unsigned events, captures, constraints, body_statuses;
static bool valid = true;
static void install(Fighter_GObj* owner)
{ valid &= owner == &object && events++ == 0; }
static void reflect(Fighter_GObj* owner, HSD_GObjEvent response)
{
    valid &= owner == &object && events++ == 1 && response != NULL;
    if (response != NULL) response(owner);
}
static void sdi(Fighter_GObj* owner) { (void) owner; }
static void post_sdi(Fighter_GObj* owner) { (void) owner; }
static int current_group(Fighter* owner, int group)
{ valid &= owner == &fighter && group == 0; return owner->x5F4_arr[0].idx; }
static void enclose(Fighter_GObj* owner, float radius)
{ valid &= owner == &object && radius == 3.125f; ++captures; }
static void constrain(Fighter* owner, Fighter* victim)
{ valid &= owner == &fighter && victim == &caught; ++constraints; }
static void body(Fighter_GObj* victim, enum_t status)
{ valid &= victim == &caught_object && status == 2; ++body_statuses; }

int main(int argc, char** argv)
{
    if (!callback_host_contract()) return 1;
    if (argc != 3) return 2;
    object.user_data = &fighter;
    live_fighter = &object;
    caught_object.user_data = &caught;
    MotionState motion = { 0 };
    NativeFighterDefinition base = { .code_key = "guard_policy_probe",
        .fighter_root = "ftDataTest", .motion_state_first = 341,
        .motion_state_count = 1, .motion_states = &motion };
    NativeFighterHost host = { .callback_source_alive = callback_source_alive, .install_guard = install,
        .install_guard_reflector = reflect, .guard_sdi = sdi,
        .guard_post_sdi = post_sdi, .current_model_group = current_group,
        .capture_enclose_caught = enclose, .constrain_caught = constrain,
        .set_body_status = body };
    for (unsigned form = 0; form < 2; ++form) {
        HMODULE library = LoadLibraryA(argv[form + 1]);
        Entry entry = library ? (Entry)GetProcAddress(library,
            "FreakMelee_GetFighterModule") : NULL;
        const NativeFighterDefinition* policy = entry ? entry(&base, &host) : NULL;
        if (!policy || !policy->on_shield_transition || !policy->on_shield_scale ||
            !policy->on_shield_alpha || !policy->on_spotdodge_enter ||
            !policy->on_capture_wait_setup || !policy->on_capture_wait_enter ||
            !policy->on_catch_pull_frame || !policy->on_throw_victim_setup) return 1;
        for (int mode = NativeFighterShield_Start;
             mode <= NativeFighterShield_ReflectStart; ++mode)
        for (unsigned special = 0; special < 2; ++special) {
            memset(&fighter, 0, sizeof(fighter));
            fighter.x673 = 13;
            events = 0;
            policy->on_shield_transition(&object, mode, special != 0);
            valid &= events == 2 && fighter.mv.co.guard.x0 == mode + 0.25f &&
                fighter.mv.co.guard.xC == (special != 0) &&
                fighter.mv.co.guard.x10 == 5 && fighter.mv.co.guard.x14 == 6 &&
                fighter.mv.co.guard.x18 == 7 && fighter.mv.co.guard.x1C == 8 &&
                fighter.mv.co.guard.x20 == 9 && fighter.mv.co.guard.x24 == 10 &&
                fighter.x221A_b7 && fighter.x221C_b3 && fighter.x221C_b1 &&
                fighter.x221C_b2 && fighter.x670_timer_lstick_tilt_x == 254 &&
                fighter.x673 == 13 && fighter.trigger_analog_timer == 253 &&
                fighter.dmg.armor1 == 120 && fighter.dmg.armor0 == 0 &&
                fighter.hitlag_cb == sdi && fighter.post_hitlag_cb == post_sdi &&
                fighter.pre_hitlag_cb == NULL;
        }
        fighter.co_attrs.initial_shield_size = 2.375f;
        valid &= policy->on_shield_scale(&object) == 2.375f &&
                 !policy->on_shield_alpha(&object);
        fighter.cur_anim_frame = -4.25f;
        valid &= policy->on_catch_pull_frame(&object) == -3.875f;
        fighter.cur_anim_frame = 10.125f;
        valid &= policy->on_catch_pull_frame(&object) == 10.5f;
        fighter.x5F4_arr[0].idx = 1;
        fighter.x5F4_arr[0].prev = 0;
        policy->on_spotdodge_enter(&object);
        valid &= fighter.self_vel.x == 1;
        fighter.victim_gobj = &caught_object;
        caught.cur_pos.y = 3.125f;
        captures = constraints = body_statuses = 0;
        policy->on_capture_wait_setup(&object, &caught_object);
        policy->on_capture_wait_enter(&object, &caught_object);
        policy->on_throw_victim_setup(&object);
        valid &= captures == 1 && constraints == 1 && body_statuses == 1;
        fighter.victim_gobj = NULL;
        policy->on_capture_wait_enter(&object, &caught_object);
        policy->on_throw_victim_setup(&object);
        valid &= constraints == 1 && body_statuses == 1;
        for (unsigned context = 0; context < 3; ++context) {
            fighter.motion_id = context == 0 ? ftCo_MS_Fall :
                context == 1 ? ftCo_MS_FallAerial : ftCo_MS_JumpAerialF;
            fighter.mv.co.fall.smid = 23;
            policy->on_load_special_attrs(&object);
            valid &= fighter.mv.co.fall.smid == (context < 2 ? 7 : 23) &&
                     fighter.self_vel.y == (context < 2 ? 7 : -1);
        }
        FreeLibrary(library);
    }
    if (!valid) { fputs("Guard/capture policy contract failed\n", stderr); return 1; }
    puts("Both forms preserve float queries, guard slots and holder/caught contexts");
    return 0;
}
