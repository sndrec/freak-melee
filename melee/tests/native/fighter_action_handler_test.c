#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "script_lifetime_fixture.h"

#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/mp/forward.h>
#include <sysdolphin/baselib/gobj.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

// Creator resolution is a mocked engine boundary in these script decision
// fixtures. Live-ID/package/lifetime behavior is covered by article_creation.
static Fighter_GObj* item_creator(Item_GObj* object)
{
    Item* item = object != NULL ? object->user_data : NULL;
    return item != NULL ? item->xDD4_itemVar.native_article.attachment_owner : NULL;
}

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);

static int stock_calls;
static int authored_calls;
static float callback_damage;
static int article_state;
static int item_pickup_calls;
static int item_drop_calls;
static int item_visible_mode = -1;
static int item_invisible_mode = -1;
static int knockback_enter_mode = -1;
static int knockback_exit_mode = -1;
static int reload_calls;
static int ground_animation_calls;
static int align_floor_calls;
static int reset_z_calls;
static int switch_air_calls;
static int special_ground_calls;
static int owned_article_lookups;
static int owned_article_removals;
static int attached_article_spawns;
static int bone_attached_article_spawns;
static int article_pauses;
static int article_resumes;
static int article_bone_effects;
static int article_activations;
static int authored_projectile_spawns;
static int article_stage_collisions;
static int article_stage_bounces;
static int article_sounds;
static int article_interactions;
static int article_visible_calls;
static int article_scale_calls;
static bool article_last_visible;
static float article_last_scale;
static unsigned model_loads, model_dynamics, model_clears;
static bool model_arguments_valid = true, held_heavy;
static enum_t held_kind;
static int part_animation = -1;
static unsigned part_shows, part_hides;

static bool has_model_root(Fighter_GObj* gobj, const char* root)
{ (void) gobj; return strcmp(root, "Accessory_TopN_joint") == 0; }
static bool load_model(Fighter_GObj* gobj, unsigned slot, const char* root,
                       unsigned bone, int visibility, unsigned costume)
{
    (void) gobj; ++model_loads;
    model_arguments_valid &= slot == 2 && bone == 5 && visibility == -1 &&
        costume == 0 && strcmp(root, "Accessory_TopN_joint") == 0;
    return true;
}
static bool add_model_dynamics(Fighter_GObj* gobj, unsigned slot,
                                unsigned first, unsigned count)
{
    (void) gobj; ++model_dynamics;
    model_arguments_valid &= slot == 2 && first == 1 && count == 2;
    return true;
}
static void clear_models(Fighter_GObj* gobj) { (void) gobj; ++model_clears; }
static bool item_heavy(Item_GObj* item) { (void) item; return held_heavy; }
static enum_t item_hold(Item_GObj* item) { (void) item; return held_kind; }
static void set_part(Fighter_GObj* gobj, s32 part, s32 animation)
{
    (void) gobj;
    if (part == 0) part_animation = animation;
}
static void show_part(Fighter_GObj* gobj, s32 part)
{ (void) gobj; if (part == 0) ++part_shows; }
static void hide_part(Fighter_GObj* gobj, int part)
{ (void) gobj; if (part == 0) ++part_hides; }

static FtMotionId changed_motion;
static float changed_speed;

static void stock_animation(HSD_GObj* gobj)
{
    (void) gobj;
    ++stock_calls;
}

static void stock_special(HSD_GObj* gobj)
{
    (void) gobj;
}

static void change_motion(Fighter_GObj* gobj, FtMotionId motion,
                          MotionFlags flags, float frame, float speed,
                          float blend, Fighter_GObj* source)
{
    (void) gobj;
    changed_motion = motion;
    (void) flags;
    (void) frame;
    changed_speed = speed;
    (void) blend;
    (void) source;
}

static void ground_animation_velocity(Fighter_GObj* gobj)
{
    (void) gobj;
    ++ground_animation_calls;
}

static void align_root_to_floor(Fighter* fighter)
{
    (void) fighter;
    ++align_floor_calls;
}

static void set_root_rotation_z(Fighter* fighter, int bone, float angle)
{
    (void) fighter;
    if (bone == 0 && angle == 0) ++reset_z_calls;
}

static void switch_to_air(Fighter* fighter)
{
    (void) fighter;
    ++switch_air_calls;
}

static bool special_ground_collision(Fighter_GObj* gobj)
{
    (void) gobj;
    ++special_ground_calls;
    return false;
}

static bool animation_finished(HSD_GObj* gobj)
{
    (void) gobj;
    return false;
}

static void finish_action(HSD_GObj* gobj)
{
    (void) gobj;
    ++authored_calls;
}

static void self_damage(Fighter_GObj* gobj, float damage)
{
    (void) gobj;
    callback_damage += damage;
}

static void fighter_item_pickup(Fighter_GObj* gobj, bool flag,
                                bool item_part, bool hand_part)
{
    (void) gobj;
    if (flag && item_part && hand_part) ++item_pickup_calls;
}

static void fighter_item_drop(Fighter_GObj* gobj, bool flag,
                              bool item_part, bool hand_part)
{
    (void) gobj;
    if (!flag && !item_part && hand_part) ++item_drop_calls;
}

static void fighter_item_visible(Fighter_GObj* gobj, bool mode)
{
    (void) gobj;
    item_visible_mode = mode;
}

static void fighter_item_invisible(Fighter_GObj* gobj, bool mode)
{
    (void) gobj;
    item_invisible_mode = mode;
}

static void fighter_knockback_enter(Fighter_GObj* gobj, s32 mode)
{
    (void) gobj;
    knockback_enter_mode = mode;
}

static void fighter_knockback_exit(Fighter_GObj* gobj, s32 mode)
{
    (void) gobj;
    knockback_exit_mode = mode;
}

static void reload_ext_attributes(Fighter_GObj* gobj)
{
    (void) gobj;
    ++reload_calls;
}

static void article_change_state(Item_GObj* gobj, int state)
{
    (void) gobj;
    article_state = state;
}

static void article_set_lifetime(Item_GObj* gobj, float frames)
{
    ((Item*) gobj->user_data)->xD44_lifeTimer = frames;
}

static bool article_reflect(Item_GObj* gobj)
{
    ((Item*) gobj->user_data)->facing_dir *= -1;
    return false;
}

static bool owned_article_exists(Fighter_GObj* gobj, s32 item_number)
{
    (void) gobj;
    ++owned_article_lookups;
    return item_number == 17;
}

static bool remove_owned_article(Fighter_GObj* gobj, s32 item_number)
{
    (void) gobj;
    ++owned_article_removals;
    return item_number == 17;
}

static s32 spawn_attached_article(Fighter_GObj* gobj, ItemKind kind,
                                  Fighter_Part part, s32 variant,
                                  s32 appearance_state, s32 initial_state,
                                  s32 attached_state, float lifetime)
{
    (void) gobj;
    ++attached_article_spawns;
    return kind == It_Kind_Mario_Fire && part == FtPart_TopN &&
           variant == 2 && appearance_state == 1 && initial_state == 0 &&
           attached_state == 1 && lifetime == 60.0f ? 29 : -1;
}

static s32 spawn_bone_attached_article(Fighter_GObj* gobj, ItemKind kind,
                                       Fighter_Part part)
{
    (void) gobj;
    ++bone_attached_article_spawns;
    return kind == It_Kind_Mario_Fire && part == FtPart_TopN ? 31 : -1;
}

static bool pause_owned_article(Fighter_GObj* gobj, s32 item_number)
{
    (void) gobj;
    if (item_number != 31) return false;
    ++article_pauses;
    return true;
}

static bool resume_owned_article(Fighter_GObj* gobj, s32 item_number)
{
    (void) gobj;
    if (item_number != 31) return false;
    ++article_resumes;
    return true;
}

static Fighter_Part resolve_bone(Fighter* fighter, Fighter_Part part)
{
    (void) fighter;
    return part == FtPart_TopN ? 0 : 1;
}

static void joint_position(HSD_JObj* joint, Vec3* offset, Vec3* position)
{
    (void) joint;
    (void) offset;
    *position = (Vec3) { 3.0f, 4.0f, 0.0f };
}

static void spawn_authored_projectile(Fighter_GObj* gobj, Vec3* position,
                                     ItemKind kind, s32 variant, float facing,
                                     const NativeFighterProjectileLaunch* launch)
{
    (void) gobj;
    if (position->x == 3.0f && position->y == 4.0f &&
        kind == It_Kind_Mario_Fire && variant == 2 && facing == 0.0f &&
        launch == NULL)
        ++authored_projectile_spawns;
}

static bool article_stage_collision(Item_GObj* gobj)
{
    (void) gobj;
    ++article_stage_collisions;
    return true;
}

static bool article_bounce_stage(Item_GObj* gobj)
{
    (void) gobj;
    ++article_stage_bounces;
    return true;
}

static void article_play_sound(Item_GObj* gobj, s32 sound, u8 pan, u8 volume)
{
    (void) gobj;
    if (sound == 90030 && pan == 127 && volume == 64)
        ++article_sounds;
}

static void item_spawn_effect_on_bone(Item_GObj* gobj, s32 effect,
                                      u32 bone_index, u8 channel)
{
    (void) gobj;
    if (effect == 1149 && bone_index == 16 && channel == 0)
        ++article_bone_effects;
}

static void item_activate_attached(Item_GObj* gobj)
{
    (void) gobj;
    ++article_activations;
}

static bool item_interact(Item_GObj* gobj, HSD_GObj* other)
{
    (void) gobj;
    if (other != NULL) ++article_interactions;
    return true;
}

static float fighter_model_scale(Fighter_GObj* gobj)
{
    (void) gobj;
    return 1.25f;
}

static float fighter_root_scale_y(Fighter_GObj* gobj)
{
    (void) gobj;
    return 0.75f;
}

static bool fighter_incapacitated(Fighter_GObj* gobj)
{
    return gobj != NULL;
}

static void item_set_uniform_scale(Item_GObj* gobj, float scale)
{
    (void) gobj;
    ++article_scale_calls;
    article_last_scale = scale;
}

static void item_set_child_visible(Item_GObj* gobj, bool visible)
{
    (void) gobj;
    ++article_visible_calls;
    article_last_visible = visible;
}

static float article_state_float(const Item* item, unsigned offset)
{
    float value;
    memcpy(&value, item->xDD4_itemVar.native_article.state + offset, sizeof(value));
    return value;
}

static unsigned article_joint_reads, article_joint_writes;
static bool article_joint_arguments_valid = true;
static float item_joint_value(Item_GObj* gobj, unsigned model, const char* path, unsigned channel)
{
    article_joint_arguments_valid &= gobj != NULL && model == 0 &&
        strcmp(path, "0/1") == 0 && channel == 3;
    ++article_joint_reads;
    return -42.0f;
}
static void item_set_joint_value(Item_GObj* gobj, unsigned model, const char* path,
                                  unsigned channel, float value)
{
    article_joint_arguments_valid &= gobj != NULL && model == 0 &&
        ((strcmp(path, "0/1") == 0 && channel == 1 && value == -0.425f) ||
         (path[0] == '\0' && channel == 8 && value == 2.5f));
    ++article_joint_writes;
}

int main(int argc, char** argv)
{
    if (argc != 2) return 2;
    HMODULE library = LoadLibraryExA(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (library == NULL) return 2;
    ModuleEntry entry = (ModuleEntry) GetProcAddress(
        library, "FreakMelee_GetFighterModule");
    MotionState motions[2] = { 0 };
    motions[0].anim_cb = stock_animation;
    NativeFighterDefinition base = {
        .fighter_root = "ftDataMario",
        .sound_banks = (UINT64_C(1) << 11) | (UINT64_C(1) << 9),
        .motion_states = motions,
        .motion_state_first = 341,
        .motion_state_count = 2,
        .special_n = stock_special,
        .special_air_n = stock_special,
    };
    NativeFighterHost host = {
        .callback_source_alive = script_lifetime_alive,
        .item_creator = item_creator,
        .has_model_root = has_model_root,
        .load_model_attachment = load_model,
        .add_attachment_dynamics = add_model_dynamics,
        .clear_model_attachments = clear_models,
        .item_is_heavy = item_heavy,
        .item_hold_kind = item_hold,
        .set_part_animation = set_part,
        .show_part_animation = show_part,
        .hide_part_animation = hide_part,
        .animation_frames_remaining = animation_finished,
        .finish_ground_action = finish_action,
        .change_motion_state = change_motion,
        .ground_animation_velocity = ground_animation_velocity,
        .align_root_to_floor = align_root_to_floor,
        .set_root_rotation_z = set_root_rotation_z,
        .ground_to_air = switch_to_air,
        .special_ground_collision = special_ground_collision,
        .start_animation = stock_special,
        .self_damage = self_damage,
        .fighter_item_pickup = fighter_item_pickup,
        .fighter_item_drop = fighter_item_drop,
        .fighter_item_visible = fighter_item_visible,
        .fighter_item_invisible = fighter_item_invisible,
        .fighter_knockback_enter = fighter_knockback_enter,
        .fighter_knockback_exit = fighter_knockback_exit,
        .reload_ext_attributes = reload_ext_attributes,
        .item_change_state = article_change_state,
        .item_set_lifetime = article_set_lifetime,
        .item_reflect = article_reflect,
        .owned_article_exists = owned_article_exists,
        .remove_owned_article = remove_owned_article,
        .spawn_attached_article = spawn_attached_article,
        .spawn_bone_attached_article = spawn_bone_attached_article,
        .pause_owned_article = pause_owned_article,
        .resume_owned_article = resume_owned_article,
        .resolve_bone = resolve_bone,
        .joint_position = joint_position,
        .spawn_authored_projectile = spawn_authored_projectile,
        .item_stage_collision = article_stage_collision,
        .item_bounce_stage = article_bounce_stage,
        .item_play_sound = article_play_sound,
        .item_spawn_effect_on_bone = item_spawn_effect_on_bone,
        .item_activate_attached = item_activate_attached,
        .fighter_model_scale = fighter_model_scale,
        .fighter_root_scale_y = fighter_root_scale_y,
        .fighter_incapacitated = fighter_incapacitated,
        .sqrt_float = sqrtf,
        .item_set_uniform_scale = item_set_uniform_scale,
        .item_set_child_visible = item_set_child_visible,
        .item_interact = item_interact,
        .item_joint_value = item_joint_value,
        .item_set_joint_value = item_set_joint_value,
    };
    NativeFighterHost incomplete_host = host;
    incomplete_host.spawn_attached_article = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Attached spawn accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.spawn_bone_attached_article = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Bone-attached spawn accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.pause_owned_article = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article pause accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.resume_owned_article = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article resume accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.item_spawn_effect_on_bone = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article bone effect accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.item_activate_attached = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article activation accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.owned_article_exists = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article lookup accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.remove_owned_article = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article removal accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.item_bounce_stage = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article bounce accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.item_set_child_visible = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article visibility accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.spawn_authored_projectile = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Authored projectile accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.item_interact = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article interaction accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.fighter_incapacitated = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article holder predicate accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    incomplete_host = host;
    incomplete_host.sqrt_float = NULL;
    if (entry != NULL && entry(&base, &incomplete_host) != NULL) {
        fputs("Article sqrt accepted a missing host operation.\n", stderr);
        FreeLibrary(library);
        return 1;
    }
    const NativeFighterDefinition* scripted =
        entry != NULL ? entry(&base, &host) : NULL;
    bool valid = scripted != NULL && scripted->motion_states != motions &&
                 scripted->sound_banks == base.sound_banks &&
                 scripted->motion_states[0].anim_cb != stock_animation &&
                 motions[0].anim_cb == stock_animation &&
                 scripted->item_count == 1 && scripted->items != NULL &&
                 scripted->article_logic_count == 1 &&
                 scripted->article_logics != NULL &&
                 scripted->article_logics[0].kind == It_Kind_Mario_Fire &&
                 scripted->article_logics[0].state_count == 2 &&
                 scripted->article_logics[0].projectile != NULL &&
                 scripted->article_logics[0].projectile->appearance_state == 0 &&
                 scripted->article_logics[0].projectile->item_state == 0 &&
                 scripted->article_logics[0].projectile->read_values != NULL &&
                 scripted->article_logics[0].projectile->auto_lifetime &&
                 scripted->article_logics[0].projectile->initial_velocity &&
                 scripted->article_logics[0].projectile->initial_update &&
                 scripted->article_logics[0].projectile->on_launched == NULL;
    if (valid) {
        float article_attributes[3] = { 5.5f, 0, 45.0f };
        Article article_data = {
            .x4_specialAttributes = article_attributes,
        };
        float launch_values[3];
        scripted->article_logics[0].projectile->read_values(&article_data, launch_values);
        valid = memcmp(launch_values, article_attributes, sizeof(launch_values)) == 0;
        Item item = {
            .facing_dir = -1.0f,
            .xC4_article_data = &article_data,
            .x1C = 23,
        };
        HSD_GObj item_object = { .user_data = &item };
        script_lifetime_register(&item_object, NativeCallbackSource_Item);
        item.entity = &item_object;
        Fighter fighter_owner = { .motion_id = 341 };
        fighter_owner.cmd_vars[1] = 1;
        fighter_owner.take_dmg_cb = stock_special;
        fighter_owner.death2_cb = stock_special;
        int32_t tracked_article = 23;
        memcpy(fighter_owner.native_module_state + 8, &tracked_article,
               sizeof(tracked_article));
        fighter_owner.native_module_state[12] = 1;
        float fraction = 0.25f;
        memcpy(fighter_owner.native_module_state + 16, &fraction,
               sizeof(fraction));
        HSD_GObj fighter_owner_object = {
            .user_data = &fighter_owner,
            .classifier = HSD_GOBJ_CLASS_FIGHTER,
        };
        script_lifetime_register(&fighter_owner_object, NativeCallbackSource_Fighter);
        item.owner = &fighter_owner_object;
        item.xDD4_itemVar.native_article.attachment_owner =
            &fighter_owner_object;
        item.xDD4_itemVar.native_article.attachment_facing = -1.0f;
        scripted->items[0].on_thrown(&item_object);
        valid &= item.x40_vel.x == -2.0f;
        ItemLogicTable* article = scripted->article_logics[0].logic;
        if (article != NULL && article->evt_unk != NULL) {
            HSD_GObj other = { 0 };
            article->evt_unk(&item_object, &other);
            valid &= article_interactions == 1;
        } else valid = false;
        valid &= article != NULL && article->states != NULL &&
                 article->states[0].anim_id == -1 &&
                 article->states[0].animated != NULL &&
                 article->states[0].physics_updated != NULL &&
                 article->states[0].collided != NULL &&
                 article->states[1].anim_id == 2 &&
                 article->spawned != NULL &&
                 article->reflected != NULL &&
                 article->dmg_dealt != NULL;
        if (valid) {
            valid &= article->states[1].animated(&item_object) &&
                     article_state_float(&item, 0) == 1.0f &&
                     article_state_float(&item, 4) == 0.25f &&
                     article_state_float(&item, 8) == 3.0f &&
                     fighter_owner.native_module_state[12] == 0;
            memcpy(&fraction, fighter_owner.native_module_state + 16,
                   sizeof(fraction));
            valid &= fraction == 0.5f;
            article->spawned(&item_object);
            if (article->picked_up != NULL) article->picked_up(&item_object);
            valid &= item.xD44_lifeTimer == 45 &&
                     article_joint_reads == 1 && article_joint_writes == 2 &&
                     article_joint_arguments_valid &&
                     article_state_float(&item, 8) == -42.0f;
            int32_t exact_counter;
            memcpy(&exact_counter, item.xDD4_itemVar.native_article.state + 32, 4);
            valid &= exact_counter == 16777217 &&
                     item.xDD4_itemVar.native_article.state[36] == 1;
            valid &= !article->states[0].animated(&item_object);
            valid &= item.xD44_lifeTimer == 44;
            valid &= article_state_float(&item, 0) == 7.0f &&
                     article_state_float(&item, 8) == 5.5f &&
                     article_state_float(&item, 12) == 0.75f &&
                     article_state_float(&item, 16) == 341.0f &&
                     article_state_float(&item, 20) == 0.75f &&
                     article_state_float(&item, 24) == 23.0f &&
                     article_state_float(&item, 28) == 1.0f &&
                     item.xDD4_itemVar.native_article.attachment_owner ==
                         &fighter_owner_object &&
                     item.xDD4_itemVar.native_article.attachment_facing ==
                         -1.0f &&
                     item.xD0C == 2 &&
                     item.xDC8_word.flags.x15 == 0 &&
                     item.xDAC_itcmd_var0 == 1 &&
                     article_visible_calls == 1 && article_last_visible &&
                     article_scale_calls == 1 && article_last_scale == 1.25f;
            memcpy(&tracked_article, fighter_owner.native_module_state + 8,
                   sizeof(tracked_article));
            valid &= tracked_article == 24;
            valid &= fighter_owner.take_dmg_cb == NULL &&
                     fighter_owner.death2_cb == NULL;
            item.xDD4_itemVar.native_article.attachment_owner = NULL;
            item.xDAC_itcmd_var0 = 0;
            valid &= !article->states[0].animated(&item_object) &&
                     item.xDAC_itcmd_var0 == 0 &&
                     article_state_float(&item, 12) == 1.0f &&
                     article_state_float(&item, 16) == 341.0f &&
                     article_state_float(&item, 20) == 0.75f &&
                     article_state_float(&item, 24) == 0.0f &&
                     article_state_float(&item, 28) == 1.0f &&
                     article_visible_calls == 2 && !article_last_visible &&
                     article_scale_calls == 2 && article_last_scale == 1.0f;
            item.owner = NULL;
            valid &= !article->states[0].animated(&item_object) &&
                     article_state_float(&item, 16) == -1.0f &&
                     article_state_float(&item, 20) == 1.0f;
            valid &= article_state_float(&item, 28) == 0.0f;
            article->states[0].physics_updated(&item_object);
            memcpy(&exact_counter, item.xDD4_itemVar.native_article.state + 32, 4);
            valid &= exact_counter == 16777218 &&
                     item.xDD4_itemVar.native_article.state[36] == 0;
            valid &= item.x40_vel.x == -3.0f &&
                     article->states[0].collided(&item_object) &&
                     article->dmg_dealt(&item_object);
            valid &= article_state_float(&item, 4) == 1.0f &&
                     article_stage_collisions == 1 &&
                     article_stage_bounces == 1 && article_sounds == 1;
            item.xD44_lifeTimer = 1;
            valid &= !article->states[0].animated(&item_object) &&
                     article_state == 1 &&
                     article->states[1].animated(&item_object);
            valid &= !article->reflected(&item_object) &&
                     item.facing_dir == 1.0f;
        }
        float extended_attributes[8] = { 0 };
        extended_attributes[3] = 1.5f;
        int32_t limit = 7;
        memcpy((char*) extended_attributes + 16, &limit, sizeof(limit));
        Fighter fighter = { 0 };
        fighter.x673 = 3;
        FighterBone bones[1] = { 0 };
        fighter.parts = bones;
        fighter.dat_attrs = extended_attributes;
        HSD_GObj object = { .user_data = &fighter };
        script_lifetime_register(&object, NativeCallbackSource_Fighter);
        valid &= scripted->on_idle_variants != NULL &&
                 scripted->on_motion_is_angled != NULL;
        if (scripted->on_idle_variants != NULL) {
            valid &= scripted->on_idle_variants(&object);
            HSD_GObj held_for_idle = { 0 };
            fighter.item_gobj = &held_for_idle;
            valid &= !scripted->on_idle_variants(&object);
            fighter.item_gobj = NULL;
        }
        if (scripted->on_motion_is_angled != NULL) {
            for (int motion = 0; motion <= 400; ++motion) {
                bool expected = motion >= ftCo_MS_AttackS3Hi &&
                                motion <= ftCo_MS_AttackS4LwS &&
                                (motion < ftCo_MS_AttackS3LwS ||
                                 motion > ftCo_MS_AttackS3LwS + 2);
                valid &= scripted->on_motion_is_angled(&object, motion) == expected;
            }
        }
        if (scripted->on_load != NULL) scripted->on_load(&object);
        int32_t bone_attached_id = -1;
        memcpy(&bone_attached_id, fighter.native_module_state + 20,
               sizeof(bone_attached_id));
        valid &= fighter.x2070.count_thrown_items &&
                 attached_article_spawns == 1 &&
                 bone_attached_article_spawns == 1 &&
                 bone_attached_id == 31 &&
                 article_pauses == 1 && article_resumes == 1 &&
                 authored_projectile_spawns == 1;
        if (fighter.pre_hitlag_cb != NULL)
            fighter.pre_hitlag_cb(&object);
        if (fighter.post_hitlag_cb != NULL)
            fighter.post_hitlag_cb(&object);
        valid &= article_pauses == 2 && article_resumes == 2 &&
                 article_bone_effects >= 1 && article_activations == 1;
        if (scripted->on_load_special_attrs != NULL)
            scripted->on_load_special_attrs(&object);
        scripted->motion_states[0].anim_cb(&object);
        scripted->motion_states[0].phys_cb(&object);
        fighter.facing_dir = -1;
        fighter.coll_data.env_flags = Collide_RightWallPush;
        scripted->motion_states[0].coll_cb(&object);
        fighter.coll_data.env_flags = Collide_RightWallHug;
        scripted->motion_states[0].coll_cb(&object);
        if (fighter.take_dmg_cb != NULL) fighter.take_dmg_cb(&object);
        valid &= scripted->on_wait_input != NULL &&
                 scripted->on_action_status != NULL;
        fighter.x682 = 1;
        fighter.input.held_buttons[0] = HSD_PAD_DPADDOWN;
        valid &= !scripted->on_wait_input(&object);
        fighter.input.held_buttons[0] = 0;
        valid &= scripted->on_wait_input(&object);
        fighter.x682 = 2;
        valid &= !scripted->on_wait_input(&object);
        fighter.player_id = 2;
        fighter.x221F_b4 = true;
        valid &= scripted->on_action_status(&object);
        fighter.x221F_b4 = false;
        valid &= !scripted->on_action_status(&object);
        valid &= fighter.death1_cb == scripted->on_death1 &&
                 fighter.death1_cb != NULL;
        if (fighter.death1_cb != NULL) fighter.death1_cb(&object);
        valid &= fighter.death1_cb == NULL;
        if (fighter.death2_cb != NULL) fighter.death2_cb(&object);
        if (fighter.deal_dmg_cb != NULL) fighter.deal_dmg_cb(&object);
        if (fighter.hurtbox_detect_cb != NULL)
            fighter.hurtbox_detect_cb(&object);
        if (scripted->on_tick != NULL) scripted->on_tick(&object);
        if (scripted->on_late_update != NULL)
            scripted->on_late_update(&object);
        if (scripted->on_color_animation_reset != NULL)
            scripted->on_color_animation_reset(&object);
        if (scripted->on_item_pickup != NULL)
            scripted->on_item_pickup(&object, true);
        if (scripted->on_item_drop_ext != NULL)
            scripted->on_item_drop_ext(&object, false);
        if (scripted->on_knockback_enter != NULL)
            scripted->on_knockback_enter(&object);
        if (scripted->on_knockback_exit != NULL)
            scripted->on_knockback_exit(&object);
        if (scripted->on_item_visible != NULL)
            scripted->on_item_visible(&object);
        if (scripted->on_item_invisible != NULL)
            scripted->on_item_invisible(&object);
        motions[0].anim_cb(&object);
        valid &= authored_calls == 1 && stock_calls == 1 &&
                callback_damage == 114.5f &&
                item_pickup_calls == 1 && item_drop_calls == 1 &&
                item_visible_mode == 1 && item_invisible_mode == 0 &&
                knockback_enter_mode == 3 && knockback_exit_mode == 3 &&
                reload_calls == 1 && special_ground_calls == 2;
        valid &= owned_article_lookups == 1 && owned_article_removals == 1;
        valid &= ground_animation_calls == 1 && align_floor_calls == 1 &&
                 reset_z_calls == 1 && switch_air_calls == 1 &&
                 changed_motion == 342 && changed_speed == 1.5f;
        valid &= scripted->owns_model_attachments && model_arguments_valid &&
                 model_loads == 1 && model_dynamics == 1 && part_shows == 0 &&
                 part_hides == 0 && part_animation == -1;
        HSD_GObj held = { 0 };
        fighter.item_gobj = &held;
        held_kind = 3;
        scripted->on_item_pickup(&object, false);
        valid &= part_animation == 0 && part_shows == 0;
        held_kind = 4;
        scripted->on_item_pickup(&object, true);
        valid &= part_animation == 1 && part_shows == 1;
        held_heavy = true; part_animation = -1;
        scripted->on_item_pickup(&object, true);
        scripted->on_item_visible(&object);
        scripted->on_item_invisible(&object);
        valid &= part_animation == -1 && part_shows == 1 && part_hides == 0;
        held_heavy = false; held_kind = 1;
        scripted->on_item_pickup(&object, false);
        valid &= part_animation == -1;
        scripted->on_item_visible(&object);
        scripted->on_item_invisible(&object);
        valid &= part_shows == 2 && part_hides == 1;
        scripted->on_user_data_remove(&object);
        valid &= model_clears == 1;
        base.motion_state_first = 342;
        valid &= entry(&base, &host) == NULL;
    }
    FreeLibrary(library);
    if (!valid) {
        fputs("Script action did not replace only its selected motion callback.\n",
              stderr);
        return 1;
    }
    puts("A named action replaced its callback without modifying the stock table.");
    return 0;
}
