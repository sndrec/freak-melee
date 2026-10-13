#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/kinds/itfoxblaster.h>
#include <melee/ft/kinds/ftFox/ftfoxspecialn.h>
#include <melee/it/native_article_model.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Creator resolution is a mocked engine boundary in these script decision
// fixtures. Live-ID/package/lifetime behavior is covered by article_creation.
static Fighter_GObj* item_creator(Item_GObj* object)
{
    Item* item = object != NULL ? object->user_data : NULL;
    return item != NULL ? item->xDD4_itemVar.native_article.attachment_owner : NULL;
}

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

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
static unsigned bone_count(Fighter* fighter) { (void) fighter; return 60; }

static unsigned samples, sounds, lifecycle_samples;
static unsigned state_changes, advances, removals, destroyed_effects, flashes;
static float scale_value;
static bool incapacitated;
static s32 last_sound;
static bool requested_signal;
static bool requested_attachment;
static unsigned attachment_calls, release_calls, joint_effects;
static unsigned air_collisions, landing_collisions;
static bool target_found;
static HSD_GObj target_object;
static HSD_GObj* query_owner;
static void require(bool ok, const char* message);
static Fighter_GObj* find_fighter(Vec3* position, Vec3* result, Item_GObj* owner, float facing)
{
    require(position->x == 8 && position->y == 9 && owner == query_owner && facing == -1,
            "Fighter target query lost position, owner or facing");
    if (!target_found) return NULL;
    *result = (Vec3){11, 12, 13};
    return &target_object;
}
static HSD_GObj* find_item(Vec3* position, float facing)
{
    require(position->x == 8 && position->y == 9 && facing == -1,
            "Item target query lost position or facing");
    return target_found ? &target_object : NULL;
}
static void center_position(Item_GObj* object, Vec3* result)
{
    require(object == &target_object, "Target center sampled a different object");
    *result = (Vec3){21, 22, 23};
}
static void respond_to_stage(Item_GObj* object, HSD_GObjEvent contact)
{
    require(contact != NULL, "Stage response omitted its authored contact callback");
    contact(object);
}
static bool overlaps_owner(Item_GObj* object, unsigned hitbox)
{
    require(object->user_data != NULL && hitbox == 0, "Owner overlap lost hitbox index");
    return target_found;
}
static void slide_on_ground(Item_GObj* object, HSD_GObjEvent landed, HSD_GObjEvent left)
{
    require(landed != NULL && left != NULL, "Ground slide omitted a transition callback");
    (target_found ? landed : left)(object);
}
static Fighter_Part attached_bone;
static void change_motion(Fighter_GObj* object, FtMotionId state,
                          MotionFlags flags, float frame, float speed,
                          float blend, Fighter_GObj* other)
{
    (void)flags; (void)frame; (void)speed; (void)blend; (void)other;
    ((Fighter*)object->user_data)->motion_id = state;
}
static void start_animation(Fighter_GObj* object) { (void)object; }
static void require(bool ok, const char* message)
{
    if (ok) return;
    fprintf(stderr, "%s (sample %u)\n", message, samples);
    exit(1);
}
static void attach_to_owner(Item_GObj* object, HSD_GObj* owner, Fighter_Part bone)
{
    Item* item = object->user_data;
    require(owner == item->xDD4_itemVar.native_article.attachment_owner,
            "Attachment changed the owner");
    attached_bone = bone;
    ++attachment_calls;
}
static void release_attachment(Item_GObj* object, Vec3* velocity)
{
    Item* item = object->user_data;
    require(velocity->x == 1 && velocity->y == 2 && velocity->z == 3,
            "Release lost its authored velocity");
    Fighter* owner = item->xDD4_itemVar.native_article.attachment_owner->user_data;
    owner->item_gobj = NULL;
    item->xDC8_word.flags.x14 = true;
    ++release_calls;
}
static void joint_effect(Item_GObj* object, int effect, const char* path)
{
    require(object->user_data != NULL && effect == 1151 && strcmp(path, "0/0") == 0,
            "Article effect lost its joint path");
    ++joint_effects;
}
static bool stage_air_collision(Item_GObj* object)
{
    require(object->user_data != NULL, "Air collision lost its item");
    ++air_collisions;
    return false;
}
static bool stage_land_collision(Item_GObj* object)
{
    require(object->user_data != NULL, "Landing collision lost its item");
    ++landing_collisions;
    return false;
}
static uint32_t hash(const char* name)
{
    uint32_t value = UINT32_C(2166136261);
    for (; *name; ++name) {
        value ^= (unsigned char)*name;
        value *= UINT32_C(16777619);
    }
    return value;
}
static void signal(const NativeFighterArticleLogic* article, Item_GObj* object,
                   const char* name, float value)
{
    for (unsigned i = 0; i < article->signal_count; ++i) {
        if (article->signals[i].name_hash != hash(name)) continue;
        article->signals[i].callback(object, value);
        return;
    }
    require(false, "Expected named article signal is missing");
}
static bool send(Fighter_GObj* object, s32 handle, uint32_t name, float value)
{
    Fighter* fighter = object->user_data;
    require(handle == 17 && name == hash("impulse") &&
            value == fighter->facing_dir * 2.5f,
            "Fighter signal arguments were not compiled correctly");
    requested_signal = true;
    return true;
}
static s32 spawn_attachment(Fighter_GObj* object, ItemKind kind, Fighter_Part part)
{
    require(object->user_data != NULL && kind == It_Kind_Fox_Blaster &&
            part == FtPart_RThumbNb, "Expression-selected attachment arguments differ");
    requested_attachment = true;
    return 19;
}
static void play_sound(Item_GObj* object, s32 sound, u8 pan, u8 volume)
{
    (void)object;
    require(pan == 127 && volume == 64, "Blaster sound parameters differ");
    ++sounds;
    last_sound = sound;
}
static void change_item_state(Item_GObj* object, int state)
{
    Item* item = object->user_data;
    item->msid = state;
    item->on_accessory = NULL; // Item_80268E5C clears this on every transition.
    ++state_changes;
}
static void advance(Item_GObj* object) { (void)object; ++advances; }
static void remove_item(Item_GObj* object)
{
    ++removals;
    object->user_data = NULL; // Catch any generated accesses after removal.
}
static void destroy_effects(Item_GObj* object)
{
    require(object->user_data != NULL, "Effect cleanup accessed a removed item");
    ++destroyed_effects;
}
static bool is_incapacitated(Fighter_GObj* object) { (void)object; return incapacitated; }
static float root_scale(Fighter_GObj* object)
{
    return ((HSD_JObj*)object->hsd_obj)->scale.y;
}
static void uniform_scale(Item_GObj* object, float value)
{
    HSD_JObjSetScale(object->hsd_obj, &(Vec3){value, value, value});
}
static void attribute_scale(Item_GObj* object, float value)
{
    (void)object; scale_value = value;
}
static void directional_effect(Item_GObj* object, s32 effect,
                               Fighter_Part part, Vec3* offset, float angle)
{
    require(object->user_data != NULL && effect == 1166 &&
            part == FtPart_RThumbNb && offset->x == 0 &&
            offset->y == 1.2325000762939453f &&
            offset->z == 4.263599872589111f && angle == 0,
            "Muzzle effect differs from stock arguments");
    ++flashes;
}
static bool interact(Item_GObj* object, HSD_GObj* other)
{
    require(object->user_data != NULL && other != NULL, "Interaction arguments missing");
    return false;
}
static void reset_calls(void)
{
    state_changes = advances = removals = destroyed_effects = flashes = 0;
}
static int32_t field(Item* item, unsigned offset)
{
    int32_t value;
    memcpy(&value, item->xDD4_itemVar.native_article.state + offset, 4);
    return value;
}
static void set_field(Item* item, unsigned offset, int32_t value)
{
    memcpy(item->xDD4_itemVar.native_article.state + offset, &value, 4);
}
static void rig(HSD_JObj* joints)
{
    memset(joints, 0, sizeof(HSD_JObj) * 8);
    for (unsigned i = 0; i < 8; ++i) joints[i].scale = (Vec3){1, 1, 1};
    for (unsigned i = 0; i < 4; ++i) joints[i].child = &joints[i + 1];
    for (unsigned i = 4; i < 7; ++i) joints[i].next = &joints[i + 1];
}
static void compare(Item* a, Item* b, HSD_JObj* stock, HSD_JObj* scripted)
{
    ++samples;
    require(a->xDD4_itemVar.foxblaster.xDD4 == field(b, 0) &&
            a->xDD4_itemVar.foxblaster.xDD8 == field(b, 4) &&
            a->xDD4_itemVar.foxblaster.xDDC == field(b, 8),
            "Blaster recoil/open counters differ from original C");
    for (unsigned i = 0; i < 8; ++i) {
        require(memcmp(&stock[i].translate, &scripted[i].translate, sizeof(Vec3)) == 0 &&
                memcmp(&stock[i].scale, &scripted[i].scale, sizeof(Vec3)) == 0 &&
                memcmp(&stock[i].rotate, &scripted[i].rotate, sizeof(stock[i].rotate)) == 0 &&
                stock[i].flags == scripted[i].flags,
                "Blaster model transformation differs from original C");
    }
}
static void check_blaster(const NativeFighterArticleLogic* article, unsigned kind)
{
    require(article->state_count == 11 && article->signal_count == 6,
            "Blaster states/signals were not emitted");
    for (unsigned i = 1; i <= 8; ++i)
        require(article->logic->states[i].physics_updated ==
                article->logic->states[0].physics_updated &&
                article->logic->states[i].anim_id == (int)i,
                "Shared states must point directly to their common callback");
    for (unsigned state = 0; state <= 8; ++state)
        for (int opening = 0; opening < 5; ++opening)
            for (int direction = -1; direction <= 1; ++direction)
                for (int recoil = 0; recoil < 14; ++recoil) {
                    Item stock = { .kind = article->kind }, scripted = { .kind = article->kind };
                    HSD_JObj a[8], b[8];
                    rig(a); rig(b);
                    HSD_GObj first = { .user_data = &stock, .hsd_obj = &a[0] };
                    HSD_GObj second = { .user_data = &scripted, .hsd_obj = &b[0] };
            live_item = &second;
                    stock.xDD4_itemVar.foxblaster.xDD4 = recoil;
                    stock.xDD4_itemVar.foxblaster.xDD8 = opening;
                    stock.xDD4_itemVar.foxblaster.xDDC = direction;
                    stock.xDD4_itemVar.foxblaster.xDE0 = true;
                    set_field(&scripted, 0, recoil); set_field(&scripted, 4, opening);
                    set_field(&scripted, 8, direction);
                    scripted.xDD4_itemVar.native_article.state[12] = 1;
                    for (unsigned tick = 0; tick < 18; ++tick) {
                        // Original open/close callbacks (skip audio side effects).
                        if (tick == 3) {
                            it_802AE538(&first); signal(article, &second, "open", 0);
                        }
                        if (tick == 7) {
                            it_802AE608(&first); signal(article, &second, "close", 0);
                        }
                        if (tick == 10) {
                            it_802AE1D0(&first); signal(article, &second, "recoil", 0);
                        }
                        it_803F6CA8[state].physics_updated(&first);
                        article->logic->states[state].physics_updated(&second);
                        compare(&stock, &scripted, a, b);
                    }
                }
    Item scripted = { 0 };
    HSD_JObj joints[8]; rig(joints);
    HSD_GObj object = { .user_data = &scripted, .hsd_obj = joints };
            live_item = &object;
    article->logic->spawned(&object);
    unsigned before = sounds;
    signal(article, &object, "open", 0);
    signal(article, &object, "open", 0);
    require(sounds == before + 1 && last_sound == (kind == 0 ? 110085 : 100081),
            "Opening should play the variant sound once per article");
    signal(article, &object, "visibility", 2);
    signal(article, &object, "visibility", 2);
    require(sounds == before + 2 && last_sound == (kind == 0 ? 110100 : 100096),
            "Hiding should play the variant sound only on a visibility change");
    for (unsigned i = 1; i < 8; ++i)
        require(joints[i].flags & JOBJ_HIDDEN, "Visibility must hide the entire model subtree");
    require(!(joints[0].flags & JOBJ_HIDDEN), "Child visibility should preserve the root");
    signal(article, &object, "visibility", 1);
    for (unsigned i = 1; i < 8; ++i)
        require(!(joints[i].flags & JOBJ_HIDDEN), "Visibility must restore every child");
    // Missing model branches must not advance counters.
    joints[3].child = NULL;
    set_field(&scripted, 0, 1); set_field(&scripted, 4, 1);
    article->logic->states[0].physics_updated(&object);
    require(field(&scripted, 0) == 1 && field(&scripted, 4) == 1,
            "Missing joints changed recoil/open timing");
}
static void set_handle(Fighter* fighter, int32_t value)
{
    // The shared fighter-state table reserves two bool bytes, then aligns ints.
    memcpy(fighter->native_module_state + 4, &value, 4);
}
static int32_t handle(Fighter* fighter)
{
    int32_t value;
    memcpy(&value, fighter->native_module_state + 4, 4);
    return value;
}
static void check_lifecycle(const NativeFighterArticleLogic* article)
{
    require(article->attachment != NULL && article->attachment->origin_at_owner &&
            article->attachment->flatten_z && !article->attachment->initial_collision &&
            !article->attachment->register_as_held,
            "Gun attachment spawn policy differs from stock");
    HSD_JObj root = { .scale = {1, 0.625f, 1} };
    Fighter fighter = { 0 };
    HSD_GObj owner = { .classifier = HSD_GOBJ_CLASS_FIGHTER,
                      .user_data = &fighter, .hsd_obj = &root };
            live_fighter = &owner;
    const int pickup_states[] = {0, 9, 9, 3, 9, 9, 6, 7, 8, 9, 10};
    // Compare the script's decisions against the actual stock owner queries.
    // State changes/removal are mocked; this does not claim game-world parity.
    for (int motion = 0; motion <= 400; ++motion)
        for (unsigned state = 0; state < 9; ++state)
            for (unsigned flags = 0; flags < 8; ++flags) {
                fighter.motion_id = motion;
                fighter.x2070.x2071_b6 = (flags & 1) != 0;
                bool attached = !(flags & 2), missing = (flags & 4) != 0;
                fighter.u.fx.x222C_blasterGObj = missing ? NULL : &owner;
                fighter.take_dmg_cb = fighter.death2_cb = start_animation;
                set_handle(&fighter, missing ? -1 : 17);
                Item item = { .kind = article->kind, .msid = state };
                item.owner = attached ? &owner : NULL;
                item.xDD4_itemVar.native_article.attachment_owner = item.owner;
                HSD_GObj object = { .user_data = &item };
            live_item = &object;
                item.entity = &object;
                int action = ftFx_SpecialN_GetBlasterAction(attached ? &owner : NULL);
                bool cleanup = !attached || action == 9 ||
                    ftFx_SpecialN_CheckRemoveBlaster(&owner);
                bool release = !cleanup && ftFx_SpecialN_CheckBlasterAction(&owner);
                bool change = state != 2 && state != 5 && state != 6 &&
                    state != 7 && state != 8 && (int)state != action;
                reset_calls();
                signal(article, &object, "recoil", 0);
                HSD_GObjEvent accessory = item.on_accessory;
                require(accessory != NULL, "Recoil did not install the accessory callback");
                require(!article->logic->states[state].animated(&object),
                        "Immediate removal must not ask the item loop to remove again");
                require(state_changes == (unsigned)change && advances == (unsigned)change &&
                        item.msid == (change ? action : (int)state) &&
                        removals == (unsigned)(cleanup || release),
                        "Gun owner/state lifecycle differs from original queries");
                if (cleanup) {
                    require(destroyed_effects == 1 && item.owner == NULL &&
                            item.xDD4_itemVar.native_article.attachment_owner == NULL &&
                            (!attached || (handle(&fighter) == -1 &&
                            fighter.take_dmg_cb == NULL && fighter.death2_cb == NULL)),
                            "Gun cleanup did not clear its live owner references");
                } else if (release) {
                    require(destroyed_effects == 0 && handle(&fighter) == 17,
                            "Action-release path changed stock fighter cleanup timing");
                } else {
                    require(item.on_accessory == (change ? NULL : accessory),
                            "State change incorrectly restored a pending muzzle effect");
                    accessory(&object);
                    accessory(&object);
                    require(flashes == 1, "A pending shot must emit one muzzle flash");
                }
                ++lifecycle_samples;
            }
    for (int motion = 0; motion <= 400; ++motion)
        for (unsigned commands = 0; commands < 16; ++commands) {
            fighter.motion_id = motion;
            Item item = { .owner = &owner };
            item.xDD4_itemVar.native_article.attachment_owner = &owner;
            item.xDAC_itcmd_var0 = (commands & 1) != 0;
            item.xDB0_itcmd_var1 = (commands & 2) != 0;
            item.xDB4_itcmd_var2 = (commands & 4) != 0;
            item.xDB8_itcmd_var3 = (commands & 8) != 0;
            HSD_GObj object = { .user_data = &item };
            live_item = &object;
            item.entity = &object;
            reset_calls();
            article->logic->picked_up(&object);
            int action = ftFx_SpecialN_GetBlasterAction(&owner);
            require(item.msid == (commands ? 10 : pickup_states[action]) &&
                    state_changes == 1 && advances == 1,
                    "Pickup state/script ordering differs from stock");
            ++lifecycle_samples;
        }
    Item item = { .owner = &owner, .xDCC_flag.b3 = 1 };
    item.xDD4_itemVar.native_article.attachment_owner = &owner;
    HSD_JObj model = { .scale = {1, 1, 1} };
    HSD_GObj object = { .user_data = &item, .hsd_obj = &model };
            live_item = &object;
    item.entity = &object;
    article->logic->spawned(&object);
    require(!item.xDCC_flag.b3, "Gun must disable bounds removal when created");
    signal(article, &object, "scale", 0.85f);
    require(scale_value == 0.85f, "Attachment scaling lost its authored multiplier");
    for (unsigned i = 0; i < 2; ++i) {
        incapacitated = i != 0;
        article->logic->states[10].animated(&object);
        require(model.scale.y == (i ? 0.625f : 1), "Incapacitated owner scale was not followed");
    }
    for (unsigned collision = 0; collision < 2; ++collision) {
        item.owner = &owner; item.xDD4_itemVar.native_article.attachment_owner = &owner;
        fighter.take_dmg_cb = fighter.death2_cb = start_animation;
        set_handle(&fighter, 17); reset_calls();
        bool result = collision ? article->logic->states[9].collided(&object)
                                : article->logic->states[9].animated(&object);
        require(result && !removals && destroyed_effects == 1 && handle(&fighter) == -1 &&
                item.owner == NULL && fighter.take_dmg_cb == NULL && fighter.death2_cb == NULL,
                "State 9 must clear references and let the item loop remove it");
    }
    item.owner = &owner; item.xDD4_itemVar.native_article.attachment_owner = &owner;
    set_handle(&fighter, 17); reset_calls();
    signal(article, &object, "cleanup", 0);
    require(removals == 1 && destroyed_effects == 1 && handle(&fighter) == -1,
            "Fighter cleanup signal did not remove the attachment");
}
int main(int argc, char** argv)
{
    if (!callback_host_contract()) return 1;
    if (argc != 3) return 2;
    MotionState motion = { 0 };
    NativeFighterDefinition base = {
        .motion_states = &motion, .motion_state_first = 341, .motion_state_count = 1,
        .joint_count = 60,
    };
    NativeFighterHost host = { .callback_source_alive = callback_source_alive,
        .item_creator = item_creator,
        .bone_count = bone_count,
        .item_joint_value = nativeArticleModel_Read,
        .item_set_joint_value = nativeArticleModel_Write,
        .item_play_sound = play_sound,
        .signal_owned_article = send,
        .spawn_bone_attached_article = spawn_attachment,
        .change_motion_state = change_motion,
        .start_animation = start_animation,
        .item_change_state = change_item_state,
        .item_advance_animation = advance,
        .item_remove = remove_item,
        .destroy_effects = destroy_effects,
        .item_scale_by_attribute = attribute_scale,
        .item_spawn_directional_effect = directional_effect,
        .item_set_uniform_scale = uniform_scale,
        .fighter_incapacitated = is_incapacitated,
        .fighter_root_scale_y = root_scale,
        .item_interact = interact,
        .item_attach_to_owner = attach_to_owner,
        .item_release_attachment = release_attachment,
        .item_spawn_joint_effect = joint_effect,
        .item_stage_collision = stage_air_collision,
        .item_stage_collision_land = stage_land_collision,
        .item_find_fighter = find_fighter,
        .item_find_target = find_item,
        .item_center_position = center_position,
        .item_stage_response = respond_to_stage,
        .item_owner_overlap = overlaps_owner,
        .item_follow_ground = respond_to_stage,
        .item_bounce_to_ground = respond_to_stage,
        .item_slide_on_ground = slide_on_ground,
        .vector_normalize_xy = lbVector_NormalizeXY,
        .vector_angle_xy = lbVector_AngleXY,
        .vector_rotate = lbVector_Rotate,
    };
    for (unsigned pass = 0; pass < 2; ++pass) {
        HMODULE module = LoadLibraryExA(argv[pass + 1], NULL,
            LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        require(module != NULL, "Could not load generated article module");
        ModuleEntry entry = (ModuleEntry)GetProcAddress(module, "FreakMelee_GetFighterModule");
        require(entry != NULL, "Missing article module entry");
        const NativeFighterDefinition* definition = entry(&base, &host);
        require(definition != NULL, "Article module rejected host");
        if (pass == 0) {
            require(definition->article_logic_count == 2, "Both gun variants must be translated");
            for (unsigned i = 0; i < 2; ++i) {
                check_blaster(&definition->article_logics[i], i);
                check_lifecycle(&definition->article_logics[i]);
            }
            NativeFighterHost incomplete = host;
            incomplete.item_joint_value = NULL;
            require(entry(&base, &incomplete) == NULL, "Host checks missed a signal/model dependency");
            incomplete = host;
            incomplete.item_play_sound = NULL;
            require(entry(&base, &incomplete) == NULL, "Host checks missed a signal-only dependency");
        } else {
            Fighter fighter = { .facing_dir = -1 };
            HSD_GObj object = { .user_data = &fighter };
            live_fighter = &object;
            definition->motion_states[0].anim_cb(&object);
            require(requested_signal && requested_attachment, "Fighter did not send the signal/spawn request");
            Item item = { 0 };
            HSD_GObj item_object = { .user_data = &item };
            live_item = &item_object;
            item.entity = &item_object;
            const NativeFighterArticleLogic* article = &definition->article_logics[0];
            signal(article, &item_object, "impulse", -2.5f);
            float received; memcpy(&received, item.xDD4_itemVar.native_article.state, 4);
            require(received == -2.5f, "Signal value failed to reach article-local state");
            const float values[] = {3.25f, -7, 0.0625f};
            for (int i = -2; i <= 5; ++i) {
                signal(article, &item_object, "choose", (float)i);
                article->logic->states[2].physics_updated(&item_object);
                require(item.pos.x == (i >= 0 && i < 3 ? values[i] : 0),
                        "Constant table bounds/value is wrong");
            }
            require(article->logic->states[2].physics_updated == article->logic->states[0].physics_updated,
                    "Chained callback sharing introduced a forwarding function");
            for (int i = -2; i <= 5; ++i) {
                item.msid = 2; item.on_accessory = start_animation;
                reset_calls();
                signal(article, &item_object, "change", (float)i);
                bool valid = i >= 0 && i < 4;
                require(item.msid == (valid ? i : 2) && state_changes == (unsigned)valid &&
                        item.on_accessory == (valid ? NULL : start_animation),
                        "Expression-selected state violated its declared bounds");
            }
            NativeFighterHost incomplete = host;
            incomplete.signal_owned_article = NULL;
            require(entry(&base, &incomplete) == NULL, "Missing signal operation should reject fighter module");
            object.classifier = HSD_GOBJ_CLASS_FIGHTER;
            item.owner = &object;
            item.xDD4_itemVar.native_article.attachment_owner = &object;
            for (int bone = -1; bone <= 60; ++bone) {
                unsigned before = attachment_calls;
                signal(article, &item_object, "attach", (float)bone);
                bool valid = bone >= 0 && bone < 60;
                require(attachment_calls == before + valid &&
                        (!valid || attached_bone == bone),
                        "Attachment ignored the declared rig bounds");
            }
            item.xDD4_itemVar.native_article.attachment_owner = NULL;
            unsigned before = attachment_calls;
            signal(article, &item_object, "attach", 0);
            require(attachment_calls == before, "Attachment accepted a missing owner");
            item.xDD4_itemVar.native_article.attachment_owner = &object;
            HSD_GObj held = { 0 };
            fighter.item_gobj = &held;
            signal(article, &item_object, "release_preserve", 0);
            require(release_calls == 1 && fighter.item_gobj == &held &&
                    item.owner == &object && !item.xDC8_word.flags.x14,
                    "Projectile release lost the held item or throw-damage policy");
            signal(article, &item_object, "release", 0);
            require(release_calls == 2 && fighter.item_gobj == NULL && item.xDC8_word.flags.x14,
                    "Default release incorrectly preserved the held slot");
            signal(article, &item_object, "joint_effect", 0);
            signal(article, &item_object, "collision", 0);
            signal(article, &item_object, "collision", 1);
            require(joint_effects == 1 && air_collisions == 1 && landing_collisions == 1,
                    "Effect/collision operations dispatched incorrectly");
            signal(article, &item_object, "choose", 0);
            for (unsigned i = 0; i < 6; ++i) {
                signal(article, &item_object, "cycle", 0);
                article->logic->states[2].physics_updated(&item_object);
                require(item.pos.x == values[(i + 1) % 3], "Integer remainder lost its cycle");
            }
            incomplete = host;
            incomplete.item_release_attachment = NULL;
            require(entry(&base, &incomplete) == NULL, "Host missed release dependency");
            incomplete = host;
            incomplete.item_spawn_joint_effect = NULL;
            require(entry(&base, &incomplete) == NULL, "Host missed joint effect dependency");
            incomplete = host;
            incomplete.item_stage_collision_land = NULL;
            require(entry(&base, &incomplete) == NULL, "Host missed landing collision dependency");
            query_owner = &object;
            item.facing_dir = -1;
            for (unsigned query = 0; query < 2; ++query) {
                for (unsigned found = 0; found < 2; ++found) {
                    target_found = found;
                    item.pos = (Vec3){8, 9, 10};
                    signal(article, &item_object, query ? "item_target" : "fighter_target", 0);
                    Vec3 expected = !found ? (Vec3){0} : query ? (Vec3){21, 22, 23} : (Vec3){11, 12, 13};
                    require(memcmp(&item.pos, &expected, sizeof(expected)) == 0 &&
                            item.xDD0_flag.b3 == (bool)found,
                            "Target query output or missing-target reset is incorrect");
                }
            }
            signal(article, &item_object, "steering", 0);
            // Melee's Z rotation uses a polynomial approximation, not sinf/cosf.
            require(fabsf(item.x40_vel.x - 0.0052758984f) < 0.00001f && fabsf(item.x40_vel.y - 0.998597753f) < 0.00001f &&
                    item.x40_vel.z == 7, "XY steering lost its axes, radians or untouched Z");
            signal(article, &item_object, "response", 0);
            memcpy(&received, item.xDD4_itemVar.native_article.state, 4);
            require(received == 7, "Stage callback did not reach the article's shared signal");
            for (unsigned mode = 0; mode < 3; ++mode) {
                item.owner = &object;
                item.xDD4_itemVar.native_article.attachment_owner = mode == 2 ? NULL : &object;
                target_found = mode != 0;
                fighter.self_vel = (Vec3){0};
                signal(article, &item_object, "owner_update", 3.5f);
                require(fighter.self_vel.x == (mode == 1 ? 3.5f : 0) &&
                        fighter.self_vel.y == (mode == 1 ? 7 : 0),
                        "Owner function lost typed arguments or accepted a missing/nonoverlapping owner");
            }
            signal(article, &item_object, "response", 1);
            memcpy(&received, item.xDD4_itemVar.native_article.state, 4);
            require(received == -1, "Ground following missed the authored leave callback");
            signal(article, &item_object, "response", 2);
            memcpy(&received, item.xDD4_itemVar.native_article.state, 4);
            require(received == 1, "Air bouncing missed the authored landing callback");
            for (unsigned landed = 0; landed < 2; ++landed) {
                target_found = landed;
                signal(article, &item_object, "response", 3);
                memcpy(&received, item.xDD4_itemVar.native_article.state, 4);
                require(received == (landed ? 1 : -1), "Ground sliding selected the wrong transition callback");
            }
            const int32_t counters[] = {-2, -1, INT32_MAX};
            const int32_t expected_counters[] = {-1, 0, INT32_MIN};
            for (unsigned i = 0; i < 3; ++i) {
                set_field(&item, 4, counters[i]);
                signal(article, &item_object, "wrap", 0);
                require(field(&item, 4) == expected_counters[i] &&
                        item.xDD0_flag.b3 == (i == 2), "Unsigned serial arithmetic lost its wrap or comparison");
            }
            incomplete = host;
            incomplete.item_find_fighter = NULL;
            require(entry(&base, &incomplete) == NULL, "Host missed target query dependency");
            incomplete = host;
            incomplete.item_stage_response = NULL;
            require(entry(&base, &incomplete) == NULL, "Host missed stage response dependency");
            incomplete = host;
            incomplete.item_owner_overlap = NULL;
            require(entry(&base, &incomplete) == NULL, "Host missed owner overlap dependency");
        }
        FreeLibrary(module);
    }
    printf("Blaster parity: %u exact model samples, %u owner/state cases; pickup, cleanup, accessory timing and muzzle arguments passed.\n", samples, lifecycle_samples);
    return 0;
}
