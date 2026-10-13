#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/controller.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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
static unsigned bone_count(Fighter* fighter) { (void) fighter; return 76; }
static unsigned groups, saved_groups, sounds;
static bool valid = true;
static unsigned graph_installs;
static unsigned transition_checks, transition_stop, transition_kill;
static unsigned aerial_calls;
static bool contact_result, item_throw_result;
static enum_t item_use_category(Item_GObj* object) { valid &= object == live_item; return 3; }
static bool cstick_edge(Fighter* fighter) { valid &= fighter == live_fighter->user_data; return true; }
static FtMotionId aerial_motion(Fighter* fighter) { valid &= fighter == live_fighter->user_data; return 65; }
static NativeScriptVMStatus contact_allowed(Fighter_GObj* object,bool* allowed) { valid &= object == live_fighter; ++aerial_calls; *allowed=contact_result; return NativeScriptVM_Ok; }
static bool air_throw(Fighter_GObj* object, bool* out, struct NativeItemLifetime* retained) { valid &= object == live_fighter && out == NULL && retained == NULL; ++aerial_calls; return item_throw_result; }
static void air_gun(Fighter_GObj* object) { valid &= object == live_fighter; ++aerial_calls; }
static bool ledge_check(Fighter_GObj* object, int direction) { valid &= object == live_fighter && direction == -1; return true; }
static void landing_callback(Fighter_GObj* object, bool (*ledge)(Fighter_GObj*,int), HSD_GObjEvent landed)
{ valid &= object == live_fighter && ledge == ledge_check && ledge(object,-1); ++aerial_calls; landed(object); }
static bool check_transition(Fighter_GObj* object)
{
    valid &= object == live_fighter;
    ++transition_checks;
    if (transition_checks == transition_kill) ++((Fighter*)object->user_data)->x8_spawnNum;
    return transition_checks == transition_stop;
}
static bool install_model_graph(Fighter_GObj* object, int graph, int omission)
{
    valid &= object == live_fighter && graph == 6 && omission == 0;
    ++graph_installs;
    return true;
}
static int current_model_group(Fighter* fighter, int group)
{
    return group == 2 ? fighter->x5F4_arr[2].idx : -1;
}
static int previous_model_group(Fighter* fighter, int group)
{
    return group == 2 ? fighter->x5F4_arr[2].prev : -1;
}
static unsigned normal_accelerations;
static unsigned roll_collisions;
static void ground_standard_box(Fighter_GObj* object, ftCollisionBox* box)
{
    ++roll_collisions;
    valid &= box->top == 12 && box->bottom == -2 &&
        box->left.x == -3 && box->left.y == 4 &&
        box->right.x == 3 && box->right.y == 4;
}
static void air_accel_normal(Fighter* fighter)
{
    ++normal_accelerations;
    fighter->x74_self_accel.x = 0.25f;
}
static void air_accel_from(Fighter* fighter, float velocity)
{
    fighter->x74_self_accel.x = velocity + 7;
}
static void animation_velocity_y_adjusted(Fighter_GObj* object)
{
    ((Fighter*)object->user_data)->self_vel.y = 32;
}
static uint8_t costume_variant(Fighter* fighter)
{
    return fighter->x619_costume_id;
}
static unsigned random_draws;
static float random_float(void)
{
    ++random_draws;
    return 0.25f;
}
static Fighter* expected_fighter;
static unsigned victim_resets, box_sweeps;
static HSD_JObj* expected_joint;
static unsigned bone_queries;
static void joint_position(HSD_JObj* joint, Vec3* offset, Vec3* position)
{
    ++bone_queries;
    valid &= joint == expected_joint && offset && offset->x == 7 && offset->y == 8 && offset->z == 9;
    *position = (Vec3){20,30,40};
}
static unsigned vector_rotations;
static void rotate_vector(Vec3* vector, int axis, float angle)
{
    ++vector_rotations;
    valid &= vector->x == 1 && vector->y == 2 && vector->z == 3 && axis == 4 && angle == .5f;
    *vector = (Vec3){4,5,6};
}
static void clear_victims(HitCapsule* hit)
{ valid &= hit == &expected_fighter->x914[2]; ++victim_resets; }
static void initialize_collision(CollData* probe)
{ memset(probe, 0, sizeof(*probe)); probe->cur_pos.x = 123; }
static bool sweep_box(CollData* probe, ftCollisionBox* box)
{
    ++box_sweeps;
    valid &= probe->last_pos.x == 1 && probe->last_pos.y == 2 && probe->last_pos.z == 3 &&
        probe->cur_pos.x == 4 && probe->cur_pos.y == 5 && probe->cur_pos.z == 6 &&
        probe->x34_flags.b1234 == 5 && box->top == 7 && box->bottom == -8 &&
        box->left.x == -9 && box->left.y == 10 && box->right.x == 11 && box->right.y == 12;
    probe->env_flags = UINT32_C(0x80000001);
    return false; /* Raw contacts survive even when the shared sweep result is false. */
}
static Fighter_Part resolve_bone(Fighter* fighter, Fighter_Part part)
{ valid &= fighter == expected_fighter; return part; }
static unsigned floor_queries, hitbox_updates, escape_checks;
static unsigned throw_updates, throw_physics;
static Fighter_GObj* expected_victim;
static unsigned hold_calls, locomotion_calls;
static int clip_marker;
static unsigned cpu_bytes;
static u8 cpu_commands[32];
static bool island_found;
static unsigned body_calls;
static bool ranged_blocked, floor_blocked;
static unsigned candidate_calls;
static FighterKind victim_family = Ft_Kind_Zelda;
static FighterKind animation_family(Fighter* fighter)
{
    valid &= fighter == expected_victim->user_data;
    return victim_family;
}
static bool ranged_unsafe(Fighter* fighter, Fighter* target, float distance)
{
    valid &= fighter == expected_fighter && target == expected_victim->user_data && distance == 27;
    return ranged_blocked;
}
static void add_candidate(Fighter* fighter, int command)
{
    valid &= fighter == expected_fighter && command == 38;
    ++candidate_calls;
}
static bool read_floor(float x0,float y0,float x1,float y1,float thickness,
                       Vec3* point,int* line,u32* flags,Vec3* normal,
                       int a,int b,int c,bool(*filter)(Fighter_GObj*,int),Fighter_GObj* owner)
{
    valid &= point != NULL && line != NULL && flags != NULL && normal != NULL &&
        a == -1 && b == -1 && c == -1 && x0 == 2 && y0 == 3 &&
        x1 == 33 && y1 == 12 && thickness == 0 && !filter && !owner;
    if(floor_blocked){*point=(Vec3){0};*line=0;}
    return floor_blocked;
}
static bool floor_excluded(int32_t line){valid &= line==0;return false;}
static void hurtboxes(Fighter_GObj* object, HurtCapsuleState status)
{
    ++body_calls;
    valid &= object->user_data == expected_fighter && status == HurtCapsule_Enabled;
}
static void bone_hurtbox(Fighter_GObj* object, int bone, HurtCapsuleState status)
{
    ++body_calls;
    valid &= object->user_data == expected_fighter && bone == 6 && status == HurtCapsule_Intangible;
}
static void held_visibility(Fighter_GObj* object, bool visible)
{ ++body_calls; valid &= object->user_data == expected_fighter && !visible; }
static void unlock_ecb(Fighter* fighter)
{ ++body_calls; valid &= fighter == expected_fighter; }
static EF_SpawnResult position_effect(s32 effect, HSD_GObj* object, ...)
{
    va_list arguments;
    va_start(arguments, object);
    Vec3* position = va_arg(arguments, Vec3*);
    ++body_calls;
    valid &= effect == 1240 && object->user_data == expected_fighter &&
        position->x == -4 && position->y == 5 && position->z == 3;
    va_end(arguments);
    return (EF_SpawnResult){ 0 };
}
static void cpu_command(Fighter* fighter, u8 command)
{
    valid &= fighter == expected_fighter && cpu_bytes < sizeof(cpu_commands);
    if (cpu_bytes < sizeof(cpu_commands)) cpu_commands[cpu_bytes++] = command;
}
static void cpu_command_arg(Fighter* fighter, u8 command, u8 argument)
{ cpu_command(fighter, command); cpu_command(fighter, argument); }
static void cpu_command_pair(Fighter* fighter, u8 command, u8 a, u8 b)
{ cpu_command_arg(fighter, command, a); cpu_command(fighter, b); }
static mp_UnkStruct0* find_island(Vec3* point, float probe)
{
    valid &= point->x == -4 && point->y == 5 && point->z == 3 && probe == -10;
    return island_found ? (mp_UnkStruct0*) &clip_marker : NULL;
}
static void island_bounds(mp_UnkStruct0* island, Vec3* left, Vec3* right)
{
    valid &= island == (mp_UnkStruct0*) &clip_marker;
    *left = (Vec3){ -20, 0, 3 };
    *right = (Vec3){ 30, 4, 3 };
}
static FtWalkType walk_type(Fighter_GObj* object)
{ valid &= object->user_data == expected_fighter; return FtWalkType_Fast; }
static bool walk_requested(Fighter_GObj* object)
{ valid &= object->user_data == expected_fighter; return true; }
static bool walk_stop(Fighter_GObj* object)
{ valid &= object->user_data == expected_fighter; return false; }
static void enter_walk(Fighter_GObj* object, FtMotionId motion, u32 flags,
                        float frame, float slow, float middle, float fast,
                        float slow_rate, float middle_rate, float fast_rate, float accel)
{
    ++locomotion_calls;
    valid &= object->user_data == expected_fighter && motion == 352 && flags == 0 &&
        frame == 3 && slow == 10 && middle == 20 && fast == 30 &&
        slow_rate == 1 && middle_rate == 2 && fast_rate == 3 && accel == 1;
}
static void locomotion(Fighter_GObj* object)
{ ++locomotion_calls; valid &= object->user_data == expected_fighter; }
static float animation_length(Fighter_GObj* object)
{ valid &= object->user_data == expected_fighter; return 24; }
static FigaTree* animation_clip(Fighter* fighter, int action)
{ valid &= fighter == expected_fighter && action == 2; return (FigaTree*) &clip_marker; }
static float clip_length(FigaTree* clip)
{ valid &= clip == (FigaTree*) &clip_marker; return 6.5f; }
static ftCo_JumpInput jump_input(Fighter_GObj* object)
{ valid &= object->user_data == expected_fighter; return JumpInput_XY; }
static void short_hop(Fighter_GObj* object)
{
    ++locomotion_calls;
    Fighter* fighter = object->user_data;
    valid &= fighter == expected_fighter &&
        fighter->mv.co.kneebend.jump_input == JumpInput_XY &&
        !fighter->mv.co.kneebend.is_short_hop;
}
static void launch_jump(Fighter_GObj* object, bool sound, float multiplier)
{ ++locomotion_calls; valid &= object->user_data == expected_fighter && sound && multiplier == 1; }
static unsigned aerial_jumps;
static void begin_aerial_jump(Fighter_GObj* object, FtMotionId motion, Vec3* velocity,
                              bool reset_input_timer, bool animation_driven,
                              bool worn_item_sound)
{
    ++aerial_jumps;
    Fighter* fighter = object->user_data;
    valid &= fighter == expected_fighter && reset_input_timer && animation_driven &&
        !worn_item_sound && velocity->x == fighter->input.lstick[0].x * 2 &&
        velocity->y == 0 && velocity->z == 0 && motion ==
        (fighter->input.lstick[0].x * fighter->facing_dir > -.6f ? 27 : 28);
}
static void enter_landing(Fighter_GObj* object, FtMotionId motion, bool interrupt,
                           u32 flags, float frame, float rate)
{
    ++locomotion_calls;
    valid &= object->user_data == expected_fighter && motion == 359 && interrupt &&
        flags == 0 && frame == 0 && rate == 1;
}
static void hold(Fighter_GObj* object, FtMotionId motion)
{
    ++hold_calls;
    valid &= object == expected_victim && motion ==
        (hold_calls == 1 ? ftCo_MS_ShoulderedWait : ftCo_MS_ShoulderedTurn);
}
static void throw_victim(Fighter_GObj* object, FtMotionId motion, float rate)
{ ++hold_calls; valid &= object == expected_victim && motion == ftCo_MS_ThrownFF && rate == 1; }
static float common_attribute(NativeFighterCommonAttribute id)
{
    if (id == NativeFighterCommon_TapJumpThreshold) return .7f;
    if (id == NativeFighterCommon_AerialActionDownThreshold) return .8f;
    if (id == NativeFighterCommon_ShieldPushbackLightMin) return .25f;
    if (id == NativeFighterCommon_ShieldPushbackLightMax) return .5f;
    return id == NativeFighterCommon_ThrowHorizontal || id == NativeFighterCommon_AerialJumpReverse ? .6f : id == NativeFighterCommon_ThrowUp ? .5f : -.5f;
}
static void update_throw(Fighter_GObj* object)
{
    ++throw_updates;
    valid &= object->user_data == expected_fighter;
}
static void throw_motion(Fighter_GObj* object)
{
    ++throw_physics;
    valid &= object->user_data == expected_fighter;
}
static unsigned entries;
static void enter_motion(Fighter_GObj* object, FtMotionId motion, u32 flags,
                         float frame, float rate, float blend, Fighter_GObj* source)
{
    ++entries;
    valid &= object->user_data == expected_fighter && motion == 341 && flags == 0 &&
        frame == 0 && rate == 1 && blend == 0 && source == NULL;
}
static void empty(Fighter_GObj* object) { (void) object; }
static bool suppress_damage;
static unsigned damage_entries, damage_reactions, item_releases;
static bool reaction_suppressed(Fighter* fighter)
{ valid &= fighter == expected_fighter; return suppress_damage; }
static float scale_knockback(float value)
{ valid &= value == expected_fighter->dmg.kb_applied; return value * .5f; }
static s32 knockback_level(float value)
{ valid &= value == expected_fighter->dmg.kb_applied * .5f; return (s32) value; }
static void damage_enter(Fighter_GObj* object, int motion, float facing)
{ ++damage_entries; valid &= object->user_data == expected_fighter && motion == 350 && facing == 0; }
static void damage_reaction(Fighter_GObj* object, float facing)
{ ++damage_reactions; valid &= object->user_data == expected_fighter && facing == 0; }
static void release_item(Item_GObj* item, Vec3* velocity, float multiplier)
{
    ++item_releases;
    valid &= item == expected_fighter->item_gobj && velocity->x == 0 &&
        velocity->y == 0 && velocity->z == 0 && multiplier == 1;
}
static bool follow_floor(int line, Vec3* origin, int* output_line, Vec3* point,
                         u32* flags, Vec3* normal, float distance, float gap)
{
    ++floor_queries;
    valid &= line == 7 && origin->x == -4 && origin->y == 5 && origin->z == 3 &&
        output_line == NULL && flags == NULL && normal == NULL && gap == 3;
    *point = (Vec3){ origin->x + distance, 8, origin->z };
    return distance > 0;
}
static void hitbox_position(HitCapsule* capsule, Vec3* point)
{
    ++hitbox_updates;
    if (capsule == &expected_fighter->x914[0])
        valid &= point->x == 2 && point->y == 10 && point->z == 3;
    else if (capsule == &expected_fighter->x914[1])
        valid &= point->x == -4 && point->y == 7 && point->z == 3;
    else valid = false;
}
static bool escape(Fighter_GObj* object)
{
    ++escape_checks;
    valid &= object->user_data == expected_fighter;
    return true;
}
static void current_group(Fighter_GObj* object, int group, int value)
{
    ++groups;
    valid &= object->user_data != NULL && group == 1 && value == 1;
}
static void saved_group(Fighter_GObj* object, int group, int value)
{
    ++saved_groups;
    valid &= object->user_data == expected_fighter && group == 1 && value == 0;
}
static void sound(Fighter* fighter, enum_t id, u8 volume, u8 pan)
{
    ++sounds;
    valid &= fighter != NULL && id == 190115 && volume == 127 && pan == 64;
}
int main(int argc, char** argv)
{
    if (!callback_host_contract()) return 1;
    if (argc != 4) return 2;
    Fighter* fighter = calloc(1, sizeof(*fighter));
    if (!fighter) return 1;
    /* Roll's item-throw countdown aliases its first word, not its morph flag. */
    fighter->mv.co.escape.x4 = true;
    fighter->mv.co.itemthrow4.unk_timer = 1;
    --fighter->mv.co.itemthrow4.unk_timer;
    valid &= fighter->mv.co.escape.x4 == true;
    memset(&fighter->mv, 0, sizeof(fighter->mv));
    expected_fighter = fighter;
    FighterBone parts[76] = { 0 };
    HSD_JObj joint = {0};
    expected_joint = &joint;
    parts[3].joint = &joint;
    fighter->parts = parts;
    fighter->cur_pos = (Vec3){ -4, 5, 3 };
    fighter->coll_data.floor.index = 7;
    fighter->input.pressed_buttons = HSD_PAD_LR;
    fighter->facing_dir = 1;
    fighter->facing_dir1 = -1;
    fighter->x914[1].damage = 17;
    fighter->x5AC.model_num = 2;
    fighter->hurt_capsules_len = 1;
    fighter->hurt_capsules[0].capsule.bone_idx = 6;
    HSD_GObj object = { .user_data = fighter };
    live_fighter = &object;
    HSD_GObj held_object = {0};
    Item held = {0};
    held_object.user_data = &held;
    live_item = &held_object;
    Fighter* victim = calloc(1, sizeof(*victim));
    if (victim == NULL) return 1;
    HSD_GObj victim_object = { .user_data = victim };
    expected_victim = &victim_object;
    MotionState motions[1] = { 0 };
    NativeFighterDefinition base = { .code_key = "common_policy_probe",
        .fighter_root = "ftDataTest", .motion_state_first = 341,
        .motion_state_count = 1, .motion_states = motions, .joint_count = 76 };
    NativeFighterHost host = { .callback_source_alive = callback_source_alive, .bone_count = bone_count,
        .resolve_bone = resolve_bone, .begin_aerial_jump = begin_aerial_jump,
        .set_current_visibility_group = current_group,
        .current_model_group = current_model_group,
        .previous_model_group = previous_model_group,
        .install_model_graph = install_model_graph,
        .check_taunt = check_transition, .check_crouch = check_transition,
        .check_turn = check_transition, .check_walk = check_transition,
        .item_use_category = item_use_category, .cstick_attack_edge = cstick_edge,
        .aerial_attack_motion = aerial_motion, .contact_rebound_allowed = contact_allowed,
        .check_air_item_throw = air_throw, .enter_air_gun = air_gun,
        .air_special_landing_collision = landing_callback, .fall_special_ledge_check = ledge_check,
        .air_accel_normal = air_accel_normal,
        .air_accel_from = air_accel_from, .animation_velocity_y_adjusted = animation_velocity_y_adjusted,
        .clear_hitbox_victims = clear_victims,
        .vector_rotate = rotate_vector,
        .joint_position = joint_position,
        .initialize_collision = initialize_collision, .sweep_collision_box = sweep_box,
        .set_visibility_group = saved_group,
        .costume_variant = costume_variant,
        .random_float = random_float,
        .animation_family = animation_family, .cpu_ranged_unsafe = ranged_unsafe,
        .cpu_add_candidate = add_candidate, .stage_read_floor = read_floor, .stage_cpu_floor_excluded = floor_excluded,
        .play_sound = sound, .follow_floor = follow_floor,
        .set_hitbox_position = hitbox_position, .check_ground_escape = escape,
        .update_throw = update_throw, .throw_motion = throw_motion,
        .walk_type = walk_type, .walk_requested = walk_requested,
        .walk_stop_requested = walk_stop, .enter_walk = enter_walk,
        .update_walk_animation = locomotion, .walk_physics = locomotion,
        .animation_length = animation_length, .animation_clip = animation_clip,
        .clip_length = clip_length, .jump_input = jump_input,
        .check_short_hop = short_hop, .launch_jump = launch_jump,
        .jump_physics = locomotion, .enter_landing = enter_landing,
        .initialize_hold = hold, .set_hold_motion = hold,
        .set_throw_motion = throw_victim, .common_attribute = common_attribute,
        .update_damage_timer = locomotion, .damage_physics = locomotion,
        .check_heavy_item_throw = walk_requested,
        .damage_reaction_suppressed = reaction_suppressed,
        .scale_knockback = scale_knockback, .knockback_level = knockback_level,
        .enter_damage_motion = damage_enter, .enter_damage_reaction = damage_reaction,
        .release_held_item = release_item,
        .cpu_command = cpu_command, .cpu_command_arg = cpu_command_arg,
        .cpu_command_pair = cpu_command_pair, .find_floor_island = find_island,
        .island_bounds = island_bounds, .set_hurtboxes = hurtboxes,
        .set_bone_hurtbox = bone_hurtbox, .set_held_item_visible = held_visibility,
        .unlock_ecb = unlock_ecb, .spawn_effect = position_effect,
        .ground_standard_box_collision = ground_standard_box };
    for (unsigned form = 0; form < 2; ++form) {
        HMODULE library = LoadLibraryA(argv[form + 1]);
        Entry entry = library ? (Entry) GetProcAddress(library,
            "FreakMelee_GetFighterModule") : NULL;
        const NativeFighterDefinition* definition = entry ? entry(&base, &host) : NULL;
        NativeFighterHost missing_jump = host;
        missing_jump.begin_aerial_jump = NULL;
        if (entry == NULL || entry(&base, &missing_jump) != NULL) return 1;
        definition = entry(&base, &host);
        if (!definition || !definition->on_jab_start || !definition->on_forward_smash_start ||
            !definition->on_auxiliary_item_attach || !definition->on_auxiliary_item_visibility_reset ||
            !definition->on_cpu_recovery) return 1;
        if (!definition->on_wait_enter || !definition->on_crouch_wait_enter ||
            !definition->on_aerial_attack_enter) return 1;
        fighter->x5F4_arr[2].idx = 3;
        fighter->x5F4_arr[2].prev = 1;
        definition->on_wait_enter(&object);
        valid &= fighter->self_vel.x == 1 && fighter->self_vel.y == .75f &&
            graph_installs == form * 2 + 1 && fighter->x221B_b4 && !fighter->x221B_b2;
        definition->on_crouch_wait_enter(&object);
        valid &= fighter->self_vel.x == 3 && graph_installs == form * 2 + 2 &&
            !fighter->x221B_b4 && fighter->x221B_b2;
        for (unsigned stop = 1; stop <= 5; ++stop) {
            transition_stop = stop; transition_checks = 0;
            definition->on_aerial_attack_enter(&object);
            valid &= fighter->self_vel.x == 74 && transition_checks == (stop < 5 ? stop : 4);
        }
        for (unsigned kill = 1; kill <= 4; ++kill) {
            transition_stop = 5; transition_checks = 0; transition_kill = kill;
            fighter->self_vel.y = 0;
            definition->on_aerial_attack_enter(&object);
            valid &= transition_checks == kill && fighter->self_vel.y == 0;
        }
        transition_kill = 0;
        if (!definition->on_aerial_action_manual || !definition->on_aerial_action_automatic ||
            !definition->on_grounded_motion_setup) return 1;
        fighter->input.held_buttons[0] = 0;
        valid &= !definition->on_aerial_action_manual(&object);
        fighter->input.held_buttons[0] = HSD_PAD_X;
        valid &= definition->on_aerial_action_manual(&object);
        fighter->fall_fast = true; fighter->item_gobj = NULL;
        definition->on_grounded_motion_setup(&object);
        valid &= !fighter->fall_fast && fighter->self_vel.x == .7f &&
            fighter->self_vel.y == .8f && fighter->self_vel.z == -1;
        fighter->item_gobj = live_item;
        definition->on_grounded_motion_setup(&object);
        valid &= fighter->self_vel.z == 3;
        fighter->item_gobj = NULL;
        for (unsigned outcome = 0; outcome < 3; ++outcome) {
            contact_result = outcome != 0; item_throw_result = outcome == 1;
            aerial_calls = 0; fighter->self_vel.z = 0;
            valid &= definition->on_aerial_action_automatic(&object) == (outcome != 0);
            valid &= aerial_calls == (outcome == 0 ? 1 : outcome == 1 ? 2 : 4);
            valid &= fighter->self_vel.z == (outcome == 2 ? 95 : 0);
        }
        // The flag is bit 25: preserving only a boolean result is incorrect.
        valid &= definition->on_jab_start(&object) == Ft_MF_SkipAttackCount;
        if (!fighter->x21EC) return 1;
        fighter->x21EC(&object);
        valid &= fighter->gr_vel == 42;
        fighter->x21EC = NULL;
        fighter->x914[2].state = form == 0 ? HitCapsule_Enabled : HitCapsule_Unk2;
        fighter->x2219_b3 = false;
        definition->on_forward_smash_start(&object);
        valid &= fighter->self_vel.y == 43;
        valid &= fighter->self_vel.x == 98 &&
            fighter->allow_interrupt && fighter->x2222_b2;
        valid &= vector_rotations == form + 1;
        valid &= fighter->self_vel.z == 99 && fighter->gr_vel == (form == 0 ? 1 : 2);
        valid &= fighter->x914[2].state == HitCapsule_Enabled;
        valid &= fighter->x2219_b3;
        valid &= victim_resets == form + 1 && box_sweeps == form + 1;
        valid &= normal_accelerations == form + 1 && fighter->x74_self_accel.x == 0.25f;
        if (!definition->on_forward_smash_enter || !definition->on_up_smash_enter || !definition->on_down_smash_enter) return 1;
        fighter->facing_dir = -1;
        definition->on_forward_smash_enter(&object, .25f);
        valid &= fighter->self_vel.x == -.75f;
        definition->on_up_smash_enter(&object);
        valid &= fighter->self_vel.x == 20 && fighter->self_vel.y == 30 && fighter->self_vel.z == 40;
        valid &= bone_queries == form + 1;
        definition->on_down_smash_enter(&object);
        valid &= fighter->self_vel.x == 0 && fighter->self_vel.y == 0 && fighter->self_vel.z == 0;
        valid &= bone_queries == form + 1;
        fighter->facing_dir = 1;
        if (!definition->on_catch_allowed || !definition->on_catch_start ||
            !definition->on_catch_frame || !definition->on_catch_exit ||
            !definition->on_catch_wait_release || !definition->on_catch_pull_anchor ||
            !definition->on_catch_pull_ready || !definition->on_catch_pull_follow) return 1;
        valid &= !definition->on_catch_allowed(&object);
        definition->on_catch_start(&object);
        valid &= fighter->self_vel.x == 51;
        fighter->motion_id = 350;
        valid &= definition->on_catch_frame(&object);
        fighter->motion_id = 349;
        valid &= !definition->on_catch_frame(&object);
        definition->on_catch_exit(&object);
        valid &= fighter->self_vel.x == 52;
        definition->on_catch_wait_release(&object);
        valid &= fighter->self_vel.x == 53;
        valid &= definition->on_catch_pull_anchor(&object) &&
            !definition->on_catch_pull_ready(&object) && definition->on_catch_pull_follow(&object);
        fighter->x619_costume_id = 1;
        unsigned saved_before = saved_groups;
        definition->on_auxiliary_item_attach(&object);
        valid &= fighter->self_vel.z == 44;
        valid &= saved_groups == saved_before + 1;
        fighter->x619_costume_id = 99;
        definition->on_auxiliary_item_attach(&object);
        valid &= saved_groups == saved_before + 1;
        fighter->x619_costume_id = 1;
        definition->on_auxiliary_item_visibility_reset(&object);
        valid &= fighter->self_vel.z == 45;
        valid &= saved_groups == saved_before + 1;
        fighter->cpu.x54 = (Vec2){ 24, -6 };
        cpu_bytes = 0;
        unsigned draws_before = random_draws;
        definition->on_cpu_recovery(&object);
        valid &= random_draws == draws_before + 1 && fighter->self_vel.x == 0.25f;
        const u8 expected_recovery[] = { CpuCmd_SetLstickX, 24, CpuCmd_WaitFor, 6, CpuCmd_Done };
        valid &= cpu_bytes == sizeof(expected_recovery) &&
            memcmp(cpu_commands, expected_recovery, sizeof(expected_recovery)) == 0;
        if (!definition || !definition->on_cpu_initialize || !definition->on_cpu_spacing_update ||
            !definition->on_cpu_attack_motion || !definition->on_cpu_grab_motion ||
            !definition->on_cpu_capture_decision || !definition->on_cpu_attack_decision ||
            !definition->on_cpu_charge_decision || !definition->on_cpu_charge_cancel) return 1;
        cpu_bytes = 0;
        definition->on_cpu_initialize(&object);
        const u8 expected_commands[] = { CpuCmd_PressB, CpuCmd_WaitFor, 3,
            CpuCmd_LstickForwardClamped, 40, 127,
            CpuCmd_SetLstickX, 129, CpuCmd_LstickForwardClamped, 129, 216 };
        valid &= fighter->cpu.x56C == 8.5f && cpu_bytes == sizeof(expected_commands) &&
            memcmp(cpu_commands, expected_commands, sizeof(expected_commands)) == 0;
        definition->on_cpu_spacing_update(&object, 0.5f);
        valid &= fighter->cpu.x56C == 4.5f;
        fighter->motion_id = 369;
        valid &= definition->on_cpu_attack_motion(&object) &&
            !definition->on_cpu_grab_motion(&object);
        fighter->motion_id = 352;
        valid &= !definition->on_cpu_attack_motion(&object) &&
            definition->on_cpu_grab_motion(&object);
        island_found = true;
        valid &= definition->on_cpu_capture_decision(&object) && fighter->self_vel.x == 50;
        island_found = false;
        valid &= !definition->on_cpu_capture_decision(&object) && fighter->self_vel.x == 0;
        valid &= !definition->on_cpu_attack_decision(&object) &&
            definition->on_cpu_charge_decision(&object);
        definition->on_cpu_charge_cancel(&object);
        valid &= cpu_commands[cpu_bytes - 1] == CpuCmd_ReleaseR;
        if (!definition->on_roll_enter) return 1;
        definition->on_roll_enter(&object);
        if (!fighter->coll_cb) return 1;
        fighter->coll_cb(&object);
        valid &= roll_collisions == form + 1;
        if (!definition || !definition->on_jab_restart || !definition->on_dynamics_wind ||
            !definition->on_dynamics_ground_collision ||
            !definition->on_shield_start || !definition->on_shield_reflect_start ||
            !definition->on_death || !definition->on_body_rebound) return 1;
        if (!definition->on_load || !definition->on_throw_victim_motion ||
            !definition->on_cpu_pummel_special || !definition->on_cpu_ranged_allowed ||
            !definition->on_cpu_ranged_candidates || !definition->on_cpu_threat ||
            !definition->on_cpu_special_candidate) return 1;
        fighter->victim_gobj = &victim_object;
        victim_family = Ft_Kind_Zelda;
        valid &= definition->on_throw_victim_motion(&object, ftCo_MS_ThrowLw, 242) == ftCo_MS_ThrownlwWomen;
        victim_family = Ft_Kind_Mario;
        valid &= definition->on_throw_victim_motion(&object, ftCo_MS_ThrowLw, 242) == 242;
        fighter->victim_gobj = NULL;
        valid &= definition->on_throw_victim_motion(&object, ftCo_MS_ThrowF, 241) == 241;
        fighter->motion_id = 350;
        valid &= definition->on_cpu_pummel_special(&object);
        fighter->motion_id = 356;
        valid &= !definition->on_cpu_pummel_special(&object);
        fighter->cpu.xFA_b5 = true;
        valid &= definition->on_cpu_ranged_allowed(&object);
        fighter->cpu.xFA_b5 = false;
        valid &= !definition->on_cpu_ranged_allowed(&object);
        fighter->cpu.x44 = victim;
        candidate_calls = 0;
        ranged_blocked = false;
        definition->on_cpu_ranged_candidates(&object);
        valid &= candidate_calls == 0;
        ranged_blocked = true;
        definition->on_cpu_ranged_candidates(&object);
        valid &= candidate_calls == 1;
        if (!definition->on_presentation_item || !definition->on_cpu_projectile_hit) return 1;
        fighter->self_vel.x = -4;
        definition->on_presentation_item(&object);
        valid &= fighter->x74_self_accel.x == 3 && fighter->self_vel.y == 32 && fighter->self_vel.z == 73;
        valid &= definition->on_cpu_projectile_hit(&object, 20, true) == 2;
        valid &= definition->on_cpu_projectile_hit(&object, 21, false) == 2;
        valid &= definition->on_cpu_projectile_hit(&object, 21, true) == 3;
        fighter->cpu.xA4 = 19;
        valid &= definition->on_cpu_special_candidate(&object, 1, 123) && fighter->cpu.xA4 == 19;
        valid &= definition->on_cpu_special_candidate(&object, 1, 124) && fighter->cpu.xA4 == 0;
        fighter->cpu.xA4 = 19;
        valid &= definition->on_cpu_special_candidate(&object, 1, 125) && fighter->cpu.xA4 == 19;
        unsigned draws_before_candidate = random_draws;
        unsigned commands_before_candidate = cpu_bytes;
        fighter->ground_or_air = GA_Ground;
        valid &= definition->on_cpu_special_candidate(&object, -1, 40) == 0;
        fighter->ground_or_air = GA_Air;
        valid &= definition->on_cpu_special_candidate(&object, -1, 40) && fighter->cpu.xA4 == 38;
        valid &= definition->on_cpu_special_candidate(&object, -0.7853981573134661f, 40) && fighter->cpu.xA4 == 38;
        valid &= definition->on_cpu_special_candidate(&object, -0.7853981f, 40) == 0;
        valid &= definition->on_cpu_special_candidate(&object, 1, 40) == 0;
        valid &= definition->on_cpu_special_candidate(&object, 1, 200) && fighter->cpu.xA4 == 61;
        valid &= random_draws == draws_before_candidate && cpu_bytes == commands_before_candidate;
        fighter->ground_or_air = GA_Ground;
        fighter->cur_pos = (Vec3){ 2, 3, 0 };
        victim->cur_pos = (Vec3){ 33, 12, 0 };
        victim->cpu.x564 = 7;
        floor_blocked = false;
        valid &= !definition->on_cpu_threat(&object, &victim_object);
        floor_blocked = true;
        valid &= definition->on_cpu_threat(&object, &victim_object);
        fighter->cur_pos = (Vec3){ -4, 5, 3 };
        floor_queries = hitbox_updates = escape_checks = 0;
        body_calls = 0;
        fighter->x6A4_transNOffset = (Vec3){ 99, 88, 77 };
        fighter->x221E_b4 = true;
        throw_updates = throw_physics = 0;
        definition->on_load(&object);
        valid &= fighter->parts[FtPart_TransN].flags_b4 && fighter->x2221_b2;
        if (!definition->on_dynamics_interaction_start || !definition->on_aerial_jump ||
            !definition->on_worn_item_jump_sound) return 1;
        valid &= !definition->on_worn_item_jump_sound(&object);
        valid &= definition->on_dynamics_interaction_start(&object, 0, 2) == 8 &&
            definition->on_dynamics_interaction_start(&object, 1, 2) == 0;
        fighter->co_attrs.air_jump_h_multiplier = 2;
        unsigned aerial_before = aerial_jumps;
        for (int facing = -1; facing <= 1; facing += 2) {
            fighter->facing_dir = (float)facing;
            for (int direction = -1; direction <= 1; direction += 2) {
                fighter->input.lstick[0].x = direction * .75f;
                definition->on_aerial_jump(&object);
            }
        }
        valid &= aerial_jumps == aerial_before + 4;
        fighter->facing_dir = 1;
        valid &= body_calls == 5 && !fighter->x221E_b4 &&
            fighter->x6A4_transNOffset.x == 4 && fighter->x6A4_transNOffset.y == 0 &&
            fighter->x6A4_transNOffset.z == -2;
        valid &= floor_queries == 4 && hitbox_updates == 2 && escape_checks == 1 &&
            throw_updates == 1 && throw_physics == 1 &&
            fighter->self_vel.x == 17 && fighter->self_vel.z == -1 &&
            fighter->take_dmg_2_cb != NULL;
        if (fighter->take_dmg_2_cb) fighter->take_dmg_2_cb(&object);
        valid &= fighter->self_vel.y == 15 && fighter->take_dmg_2_cb == NULL;
        for (int motion = 0; motion <= 400; ++motion) {
            valid &= definition->on_jab_restart(&object, motion) == (motion == 46);
            fighter->motion_id = motion;
            valid &= definition->on_body_rebound(&object) == (motion == 369 || motion == 371);
        }
        fighter->item_gobj = NULL;
        valid &= definition->on_dynamics_wind(&object);
        HSD_GObj held = { 0 };
        fighter->item_gobj = &held;
        valid &= !definition->on_dynamics_wind(&object);
        fighter->ground_or_air = GA_Ground;
        fighter->x34_scale.y = 1;
        valid &= definition->on_dynamics_ground_collision(&object);
        fighter->ground_or_air = GA_Air;
        valid &= !definition->on_dynamics_ground_collision(&object);
        fighter->ground_or_air = GA_Ground;
        fighter->x34_scale.y = 1.8f;
        valid &= !definition->on_dynamics_ground_collision(&object);
        groups = sounds = 0;
        definition->on_shield_start(&object);
        definition->on_shield_reflect_start(&object);
        valid &= groups == 2 && sounds == 2;
        definition->on_death(&object);
        valid &= groups == 2;
        if (!definition->on_throw_finish || !definition->on_heavy_pickup_finish) return 1;
        valid &= definition->on_heavy_pickup_finish(&object);
        if (!definition->on_heavy_throw_enter || !definition->on_heavy_item_damage) return 1;
        fighter->anim_cb = fighter->input_cb = empty;
        definition->on_heavy_throw_enter(&object);
        valid &= fighter->anim_cb == NULL && fighter->input_cb == NULL &&
            fighter->phys_cb != NULL && fighter->coll_cb != NULL && fighter->accessory4_cb != NULL;
        fighter->phys_cb(&object);
        fighter->coll_cb(&object);
        fighter->accessory4_cb(&object);
        valid &= fighter->self_vel.x == 88 && fighter->self_vel.y == 99 && fighter->self_vel.z == 77;
        damage_entries = damage_reactions = item_releases = 0;
        fighter->dmg.kb_applied = 6;
        suppress_damage = true;
        definition->on_heavy_item_damage(&object);
        valid &= damage_entries == 1 && damage_reactions == 0 && item_releases == 0;
        suppress_damage = false;
        fighter->dmg.kb_applied = 4;
        definition->on_heavy_item_damage(&object);
        valid &= damage_entries == 2 && damage_reactions == 0 && item_releases == 0;
        fighter->dmg.kb_applied = 6;
        definition->on_heavy_item_damage(&object);
        valid &= damage_entries == 2 && damage_reactions == 1 && item_releases == 1;
        fighter->motion_id = ftCo_MS_ThrowHi;
        valid &= !definition->on_throw_finish(&object);
        fighter->motion_id = ftCo_MS_ThrowF;
        fighter->victim_gobj = &victim_object;
        fighter->facing_dir = -1;
        hold_calls = locomotion_calls = 0;
        valid &= definition->on_throw_finish(&object);
        valid &= hold_calls == 3 && locomotion_calls == 9 && victim->facing_dir == -1 &&
            fighter->self_vel.x == 32.5f && fighter->self_vel.y == JumpInput_XY &&
            fighter->self_vel.z == 12;
        fighter->victim_gobj = NULL;
        hold_calls = locomotion_calls = 0;
        valid &= !definition->on_throw_finish(&object) && hold_calls == 0 &&
            locomotion_calls == 9;
        FreeLibrary(library);
    }
    HMODULE single = LoadLibraryA(argv[3]);
    Entry single_entry = single ? (Entry) GetProcAddress(single,
        "FreakMelee_GetFighterModule") : NULL;
    base.special_ground_state[NativeFighterSpecial_Down] = 341;
    base.special_air_state[NativeFighterSpecial_Side] = 341;
    // Deliberately absent opposite state IDs must not be validated or indexed.
    base.special_air_lw = base.special_s = empty;
    host.change_motion_state = enter_motion;
    host.start_animation = empty;
    const NativeFighterDefinition* one_plane = single_entry ? single_entry(&base, &host) : NULL;
    valid &= one_plane && one_plane->special_lw && one_plane->special_air_s &&
        one_plane->special_air_lw == NULL && one_plane->special_s == NULL &&
        base.special_air_lw == empty && base.special_s == empty;
    if (one_plane && one_plane->special_lw && one_plane->special_air_s) {
        one_plane->special_lw(&object);
        one_plane->special_air_s(&object);
    }
    valid &= entries == 2;
    if (single) FreeLibrary(single);
    free(fighter);
    free(victim);
    if (!valid) { fputs("Common policy contract failed.\n", stderr); return 1; }
    puts("Both forms own jab queries, wind decisions and shield-entry presentation.");
    return 0;
}
