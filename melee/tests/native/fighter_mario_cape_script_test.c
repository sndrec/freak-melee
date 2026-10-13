#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftMario/types.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>
#include <string.h>

// Creator resolution is a mocked engine boundary in these script decision
// fixtures. Live-ID/package/lifetime behavior is covered by article_creation.
static Fighter_GObj* item_creator(Item_GObj* object)
{
    Item* item = object != NULL ? object->user_data : NULL;
    return item != NULL ? item->xDD4_itemVar.native_article.attachment_owner : NULL;
}

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static HSD_GObj cape;
static Item cape_data;
static bool cape_live;
static unsigned spawned, removed, paused, resumed;
static unsigned reflect_created, reflect_updated;
static unsigned air_decel_calls, fall_calls;
static unsigned wind_calls;
static float last_wind_scale, last_wind_speed;
static bool wind_arguments_valid = true;
static float last_gravity, last_terminal;
static bool spawn_arguments_valid;
static ReflectDesc* expected_reflection;
static const NativeFighterDefinition* active_module;
static Fighter_GObj* active_owner;
static bool callback_source_alive(HSD_GObj* object, uint32_t generation, uint32_t source_class)
{
    if (object == NULL || object->user_data == NULL) return false;
    if (source_class == NativeCallbackSource_Fighter && object == active_owner)
        return (uint32_t)((Fighter*)object->user_data)->x8_spawnNum == generation;
    if (source_class == NativeCallbackSource_Item && object == &cape)
        return cape_live && object->user_data == &cape_data && cape_data.x1C == generation;
    return false;
}


static void change_motion(Fighter_GObj* gobj, FtMotionId state,
                          MotionFlags flags, float frame, float speed,
                          float blend, Fighter_GObj* other)
{
    (void) flags; (void) frame; (void) speed; (void) blend; (void) other;
    ((Fighter*) gobj->user_data)->motion_id = state;
}
static void start_animation(Fighter_GObj* gobj) { (void) gobj; }
static bool frames_remaining(Fighter_GObj* gobj)
{ (void) gobj; return true; }
static void noop_gobj(Fighter_GObj* gobj) { (void) gobj; }
static void noop_fighter(Fighter* fighter) { (void) fighter; }
static bool ground_lost(Fighter_GObj* gobj)
{ (void) gobj; return false; }
static GroundOrAir landed(Fighter_GObj* gobj)
{ (void) gobj; return GA_Air; }
static void to_air(Fighter* fighter) { fighter->ground_or_air = GA_Air; }
static void to_ground(Fighter* fighter)
{ fighter->ground_or_air = GA_Ground; }
static Fighter_Part resolve_bone(Fighter* fighter, Fighter_Part part)
{ (void) fighter; return part; }
static void joint_position(HSD_JObj* joint, Vec3* offset, Vec3* position)
{
    (void) joint; (void) offset;
    *position = (Vec3){ 1, 2, 3 };
}
static s32 spawn_cape(Fighter_GObj* gobj, ItemKind kind, Fighter_Part part)
{
    ++spawned;
    Fighter* fighter = gobj->user_data;
    spawn_arguments_valid = fighter->facing_dir == -1.0f &&
        part == FtPart_RThumbNb && kind == It_Kind_Mario_Cape;
    cape.user_data = &cape_data;
    cape_data.entity = &cape;
    ++cape_data.x1C;
    cape_data.owner = gobj;
    cape_data.xDD4_itemVar.native_article.attachment_owner = gobj;
    fighter->x1984_heldItemSpec = &cape;
    cape_live = true;
    return (s32)cape_data.x1C;
}
static bool owns_cape(Fighter_GObj* owner, s32 handle)
{
    return cape_live && owner == active_owner && handle == (s32)cape_data.x1C;
}
static bool remove_cape(Fighter_GObj* owner, s32 handle)
{
    if (!owns_cape(owner, handle)) return false;
    ++removed;
    active_module->article_logics[0].logic->destroyed(&cape);
    cape_live = false;
    return true;
}
static bool pause_cape(Fighter_GObj* owner, s32 handle)
{ if (!owns_cape(owner, handle)) return false; ++paused; return true; }
static bool resume_cape(Fighter_GObj* owner, s32 handle)
{ if (!owns_cape(owner, handle)) return false; ++resumed; return true; }
static void item_effect(Item_GObj* item, s32 effect, u32 bone, u8 mode)
{ (void) item; (void) effect; (void) bone; (void) mode; }
static void item_state(Item_GObj* item, int state)
{ (void) item; (void) state; }
static bool item_interact(Item_GObj* item, HSD_GObj* other)
{ (void) item; (void) other; return false; }
static void create_reflector(Fighter_GObj* gobj, ReflectDesc* desc,
                             HSD_GObjEvent callback)
{
    if (gobj == active_owner && callback == NULL &&
        desc->x0_bone_id == expected_reflection->x0_bone_id &&
        desc->x4_max_damage == expected_reflection->x4_max_damage &&
        desc->x8_offset.x == expected_reflection->x8_offset.x &&
        desc->x8_offset.y == expected_reflection->x8_offset.y &&
        desc->x8_offset.z == expected_reflection->x8_offset.z &&
        desc->x14_size == expected_reflection->x14_size &&
        desc->x18_damage_mul == expected_reflection->x18_damage_mul &&
        desc->x1C_speed_mul == expected_reflection->x1C_speed_mul &&
        desc->x20_behavior == expected_reflection->x20_behavior) ++reflect_created;
}
static void update_reflector(Fighter_GObj* gobj)
{ (void) gobj; ++reflect_updated; }
static void air_decel(Fighter* fighter, float amount)
{ (void) fighter; if (amount == 0.3f) ++air_decel_calls; }
static void fall(Fighter* fighter, float gravity, float terminal)
{
    (void) fighter; ++fall_calls;
    last_gravity = gravity; last_terminal = terminal;
}
static void spawn_wind(Vec3* position, int id, float scale,
                       float speed, float angle)
{
    ++wind_calls;
    wind_arguments_valid = wind_arguments_valid &&
        position->x == -2 && position->y == 2 && position->z == 3 &&
        id == 120 && angle == 1.0471976f;
    last_wind_scale = scale;
    last_wind_speed = speed;
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 1;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    MotionState states[2] = { 0 };
    NativeFighterDefinition base = {
        .motion_states = states,
        .motion_state_count = 2,
        .motion_state_first = 345,
        .special_ground_state = { [NativeFighterSpecial_Side] = 345 },
        .special_air_state = { [NativeFighterSpecial_Side] = 346 },
    };
    NativeFighterHost host = {
        .callback_source_alive = callback_source_alive,
        .item_creator = item_creator,
        .change_motion_state = change_motion,
        .start_animation = start_animation,
        .animation_frames_remaining = frames_remaining,
        .finish_ground_action = noop_gobj,
        .enter_fall = noop_gobj,
        .ground_friction = noop_gobj,
        .fall_basic = noop_fighter,
        .fall = fall,
        .air_decel = air_decel,
        .special_ground_collision = ground_lost,
        .air_collision = landed,
        .ground_to_air = to_air,
        .air_to_ground = to_ground,
        .resolve_bone = resolve_bone,
        .joint_position = joint_position,
        .spawn_wind = spawn_wind,
        .spawn_bone_attached_article = spawn_cape,
        .owned_article_exists = owns_cape,
        .remove_owned_article = remove_cape,
        .pause_owned_article = pause_cape,
        .resume_owned_article = resume_cape,
        .item_change_state = item_state,
        .item_spawn_effect_on_bone = item_effect,
        .item_activate_attached = noop_gobj,
        .item_interact = item_interact,
        .create_reflector = create_reflector,
        .update_reflector = update_reflector,
    };
    bool ok = entry != NULL;
    if (ok) {
        NativeFighterHost missing = host;
        missing.spawn_bone_attached_article = NULL;
        ok = entry(&base, &missing) == NULL;
    }
    const NativeFighterDefinition* module = ok ? entry(&base, &host) : NULL;
    ok = module != NULL && module->special_s != NULL &&
         module->special_air_s != NULL && module->state_bytes >= 2 &&
         module->on_landing_action != NULL && module->on_death != NULL &&
         module->on_cpu_capture_decision != NULL &&
         module->article_logic_count == 1;

    ftMario_DatAttrs attrs = { 0 };
    attrs.specials.cape_kind = It_Kind_Mario_Cape;
    attrs.specials.vel_x_decay = 2.0f;
    attrs.specials.vel.x = 0.3f;
    attrs.specials.vel.y = 1.5f;
    attrs.specials.grav = 0.2f;
    attrs.specials.terminal_vel = 2.5f;
    attrs.cape_reflection = (ReflectDesc){
        .x0_bone_id = 5, .x4_max_damage = 73,
        .x8_offset = { 1.25f, -2.5f, 3.75f }, .x14_size = 4.5f,
        .x18_damage_mul = 1.5f, .x1C_speed_mul = 2.25f, .x20_behavior = 1,
    };
    expected_reflection = &attrs.cape_reflection;
    FighterBone bones[FtPart_RThumbNb + 1] = { 0 };
    Fighter fighter = {
        .x8_spawnNum = 101,
        .dat_attrs = &attrs,
        .parts = bones,
        .facing_dir = -1.0f,
        .ground_or_air = GA_Ground,
        .self_vel = { .x = 2.0f, .y = 3.0f },
    };
    HSD_GObj object = { .user_data = &fighter };
    active_module = module;
    active_owner = &object;
    if (ok) {
        Fighter victim = { 0 };
        HSD_GObj victim_object = { .user_data = &victim };
        ok = !module->on_cpu_capture_decision(&object);
        fighter.victim_gobj = &victim_object;
        victim.dmg.x1830_percent = 99.75f;
        ok = ok && module->on_cpu_capture_decision(&object);
        victim.dmg.x1830_percent = 100;
        ok = ok && !module->on_cpu_capture_decision(&object);
        victim.dmg.x1830_percent = 180;
        ok = ok && !module->on_cpu_capture_decision(&object);
        fighter.victim_gobj = NULL;
        module->special_s(&object);
        ok = ok && fighter.motion_id == 345 && fighter.self_vel.y == 0.0f &&
             fighter.accessory4_cb != NULL &&
             fighter.u.mr.x223C_capeGObj == NULL;
        if (fighter.accessory4_cb) fighter.accessory4_cb(&object);
        ok = ok && spawned == 1 && spawn_arguments_valid &&
             fighter.u.mr.x223C_capeGObj == NULL &&
             fighter.x1984_heldItemSpec == &cape &&
             fighter.accessory4_cb == NULL &&
             fighter.take_dmg_cb != NULL && fighter.death2_cb != NULL &&
             fighter.pre_hitlag_cb != NULL && fighter.post_hitlag_cb != NULL;
        if (fighter.pre_hitlag_cb) fighter.pre_hitlag_cb(&object);
        if (fighter.post_hitlag_cb) fighter.post_hitlag_cb(&object);
        fighter.cmd_vars[0] = 1;
        fighter.cmd_vars[1] = 1;
        module->motion_states[0].phys_cb(&object);
        fighter.cmd_vars[1] = 0;
        fighter.reflecting = true;
        module->motion_states[0].phys_cb(&object);
        ok = ok && reflect_created == 1 && reflect_updated == 2 &&
             !fighter.reflecting && paused == 1 && resumed == 1 &&
             wind_calls == 1 && wind_arguments_valid &&
             last_wind_scale == 0.9f && last_wind_speed == 0.02f;
        if (fighter.take_dmg_cb) fighter.take_dmg_cb(&object);
        ok = ok && removed == 1 && resumed == 2 &&
             fighter.u.mr.x223C_capeGObj == NULL &&
             fighter.take_dmg_cb == NULL && fighter.death2_cb == NULL;
        // This next direct callback case supplies a fresh live article; the
        // preceding damage case removed the owned instance.
        cape_live = true;
        ++cape_data.x1C;
        fighter.motion_id = 344;
        ok = ok && module->article_logics[0].logic->states[0].animated(&cape);
        fighter.self_vel.x = 2.0f;
        module->special_air_s(&object);
        ok = ok && fighter.motion_id == 346 &&
             fighter.self_vel.x == 1.0f &&
             fighter.u.mr.x223C_capeGObj == NULL;
        fighter.cmd_vars[0] = 1;
        fighter.u.mr.x2238_isCapeBoost = true;
        module->motion_states[1].phys_cb(&object);
        ok = ok && fighter.cmd_vars[0] == 2 &&
             fighter.u.mr.x2238_isCapeBoost &&
             fighter.self_vel.y == 1.5f &&
             fall_calls == 1 && last_gravity == 0.2f &&
             last_terminal == 2.5f && air_decel_calls == 1;
        ok = ok && wind_calls == 2 && wind_arguments_valid &&
             last_wind_scale == 3.0f && last_wind_speed == 0.1f;
        module->motion_states[1].coll_cb(&object);
        ok = ok && fighter.motion_id == 345 &&
             fighter.ground_or_air == GA_Ground &&
             fighter.u.mr.x2238_isCapeBoost;
        if (fighter.accessory4_cb) fighter.accessory4_cb(&object);
        ok = ok && spawned == 2 &&
             !module->article_logics[0].logic->states[0].animated(&cape);
        fighter.motion_id = 29;
        ok = ok && module->article_logics[0].logic->states[0].animated(&cape);
        module->article_logics[0].logic->destroyed(&cape);
        cape_live = false;
        ok = ok && resumed == 2 &&
             fighter.u.mr.x223C_capeGObj == NULL &&
             fighter.take_dmg_cb == NULL && fighter.death2_cb == NULL;
        module->special_air_s(&object);
        fighter.cmd_vars[0] = 1;
        module->motion_states[1].phys_cb(&object);
        ok = ok && fighter.self_vel.y == 1.5f;
        module->special_air_s(&object);
        fighter.cmd_vars[0] = 1;
        module->motion_states[1].phys_cb(&object);
        ok = ok && fighter.self_vel.y == 0.0f;
        module->on_landing_action(&object);
        module->special_air_s(&object);
        fighter.cmd_vars[0] = 1;
        module->motion_states[1].phys_cb(&object);
        ok = ok && fighter.self_vel.y == 1.5f;
        module->on_death(&object);
        module->special_air_s(&object);
        fighter.cmd_vars[0] = 1;
        module->motion_states[1].phys_cb(&object);
        ok = ok && fighter.self_vel.y == 1.5f;
    }
    FreeLibrary(library);
    if (!ok) fputs("Mario Cape module behavior failed.\n", stderr);
    return ok ? 0 : 1;
}
