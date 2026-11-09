#include <melee/it/article_model_layers.h>
#include <melee/gm/native_semantic_state.h>

#include <melee/cm/types.h>
#include <melee/ft/types.h>
#include <melee/gm/types.h>
#include <melee/gr/types.h>
#include <melee/it/types.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool value, const char* message)
{
    if (!value) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static Item_GObj* source_item;
static uint64_t item_digest(const Item* item)
{
    return nativeSemanticState_AddItem(nativeSemanticState_Begin(),item,NULL);
}
static void transient_item_fields(void)
{
    Item a={0},b={0};
    uint64_t original=item_digest(&a);
    b.x378_itemColl.contact=(Vec3){31.0f,-12.0f,9.0f};
    require(original==item_digest(&b),"collision query output affected idle digest");
    b.x378_itemColl.cur_pos.x=1.0f;
    require(original!=item_digest(&b),"actual collision position was ignored");
    b=a;b.x524_cmd.frame_count=40.0f;b.x524_cmd.timer=19.0f;
    require(original==item_digest(&b),"inactive command scratch affected digest");
    union CmdUnion command={0};
    b.x524_cmd.u=&command;
    require(original!=item_digest(&b),"command activation was ignored");
    a=b;b.x524_cmd.frame_count=18.0f;
    require(item_digest(&a)==item_digest(&b),"derived command clock affected digest");
    b.x524_cmd.timer=20.0f;
    require(item_digest(&a)!=item_digest(&b),"active command wait was ignored");
    a=(Item){0};
    a.x5D4_hitboxes[0].hit.state=HitCapsule_Enabled;b=a;
    HitCapsule* changed=&b.x5D4_hitboxes[0].hit;
    changed->hurt_coll_pos=(Vec3){-42.0f,17.0f,3.0f};changed->coll_distance=0.25f;
    changed->victims_1[0].x4=16;changed->victims_2[11].x4=32;
    original=item_digest(&a);
    require(original==item_digest(&b),"fresh hit-result/empty victim scratch affected digest");
    changed->damage=10;
    require(original!=item_digest(&b),"active hitbox damage was ignored");changed->damage=0;
    changed->x4C.x=1;
    require(original!=item_digest(&b),"current capsule position was ignored");changed->x4C.x=0;
    changed->x58.x=1;
    require(original!=item_digest(&b),"previous capsule position was ignored");changed->x58.x=0;

    HSD_GObj targets[2]={0};
    a.x5D4_hitboxes[0].hit.victims_1[0].victim=&targets[0];
    b=a;b.x5D4_hitboxes[0].hit.victims_1[0].x4=1;
    require(item_digest(&a)!=item_digest(&b),"live victim cooldown was ignored");
    a.x5D4_hitboxes[0].hit.victims_1[0].victim=NULL;
    a.x5D4_hitboxes[0].hit.victims_2[11].victim=&targets[0];b=a;
    b.x5D4_hitboxes[0].hit.victims_2[11].x4=1;
    require(item_digest(&a)!=item_digest(&b),"secondary live victim cooldown was ignored");

    NativeSemanticObjectMap map={.fighter_count=2,.fighters={&targets[0],&targets[1]}};
    a=(Item){0};b=a;
    b.x5D4_hitboxes[0].hit.victims_1[0].victim=&targets[0];
    b.x5D4_hitboxes[0].hit.owner=&targets[1];
    original=nativeSemanticState_AddItemRelations(nativeSemanticState_Begin(),&a,&map);
    require(original==nativeSemanticState_AddItemRelations(nativeSemanticState_Begin(),&b,&map),
        "disabled capsule retained victim references affected relations");
    a.x5D4_hitboxes[0].hit.state=HitCapsule_Enabled;b.x5D4_hitboxes[0].hit.state=HitCapsule_Enabled;
    original=nativeSemanticState_AddItemRelations(nativeSemanticState_Begin(),&a,&map);
    require(original!=nativeSemanticState_AddItemRelations(nativeSemanticState_Begin(),&b,&map),
        "active capsule victim/owner references were ignored");
    b=a;b.x5D4_hitboxes[0].hit.victims_2[0].victim=&targets[0];
    require(original!=nativeSemanticState_AddItemRelations(nativeSemanticState_Begin(),&b,&map),
        "active secondary victim reference was ignored");
    b=a;b.x378_itemColl.prev_env_flags=Collide_FloorMask;
    require(item_digest(&a)!=item_digest(&b),"previous collision flags were ignored");
    a=(Item){.kind=It_Kind_Foods};b=a;b.xDB4_itcmd_var2=1;
    require(item_digest(&a)==item_digest(&b),"stock Food unused command word affected digest");
    HSD_GObj authored={0};source_item=&authored;
    a.entity=b.entity=&authored;
    require(item_digest(&a)!=item_digest(&b),"authored Food command word was ignored");
    source_item=NULL;a.entity=b.entity=NULL;
    a.kind=b.kind=It_Kind_Ness_PKFire;
    require(item_digest(&a)!=item_digest(&b),"ordinary item command word was ignored");
}

// Controlled registry boundaries; storage/name/type reads below use the real
// production implementation. The live gameplay cohort covers actual actors.
static Fighter_GObj* source_attachment_owner;
static NativeScriptReceiverView source_view;
static bool source_generation_live = true;

bool nativeScriptItem_IsAttached(Item_GObj* object)
{
    return object != NULL && object == source_item;
}

bool nativeCallbackSource_Snapshot(HSD_GObj* object, uint32_t source_class,
                                   uint32_t* generation)
{
    if (object != source_item || source_class != NativeCallbackSource_Item ||
        !source_generation_live) return false;
    *generation = 42;
    return true;
}

bool nativeScriptItem_View(Item_GObj* object, uint32_t generation,
                            NativeScriptReceiverView* result)
{
    if (object != source_item || generation != 42 ||
        !source_generation_live) return false;
    *result = source_view;
    return true;
}

Fighter_GObj* nativeScriptItem_AttachmentOwner(Item_GObj* object)
{
    return object == source_item ? source_attachment_owner : NULL;
}

static uint64_t digest_fixture(VsSceneController* controller,
                               StaticPlayer* player, Fighter* fighter,
                               Item* item, Ground* ground, Camera* camera,
                               CmSubject* subject_tail, HSD_JObj* stage_root)
{
    uint64_t hash = nativeSemanticState_Begin();
    hash = nativeSemanticState_AddController(hash, controller);
    hash = nativeSemanticState_AddCamera(hash, camera, subject_tail);
    hash = nativeSemanticState_AddPlayer(hash, player);
    hash = nativeSemanticState_AddFighter(hash, fighter, 0, NULL);
    hash = nativeSemanticState_AddItem(hash, item, NULL);
    hash = nativeSemanticState_AddGround(hash, ground);
    return nativeSemanticState_AddJointTree(hash, stage_root);
}

int main(void)
{
    VsSceneController controller = { 0 };
    StaticPlayer player = { 0 };
    Fighter fighter = { 0 };
    Item item = { 0 };
    Ground ground = { 0 };
    Camera camera = { 0 };
    CmSubject subject = { 0 };
    HSD_AObj stage_anim = { 0 };
    HSD_JObj stage_root = { 0 };
    HSD_JObj stage_child = { 0 };

    controller.frame_count = 120;
    controller.timer_seconds = 358;
    controller.fighters[0].respawn_timer = 7;
    player.player_state = 2;
    player.player_character = CKind_Falco;
    player.stocks = 3;
    player.stale_moves.current_index = 4;
    player.stale_moves.StaleMoves[4].move_id = 12;
    fighter.kind = Ft_Kind_Ganon;
    fighter.player_id = 0;
    fighter.motion_id = ftCo_MS_Turn;
    fighter.cur_pos.x = 3.5f;
    fighter.x8c_kb_vel.y = 2.0f;
    fighter.dmg.x1830_percent = 42.0f;
    fighter.dmg.x195c_hitlag_frames = 6.0f;
    fighter.bury_grkind = Gr_Kind_MuteCity;
    fighter.bury_timer_1 = 11;
    fighter.bury_timer_2 = 17;
    fighter.x2330.x = 23;
    fighter.x2330.y = 29;
    fighter.x2338.x = 31;
    fighter.x2338.y = 37;
    fighter.u.gn.during_specials_start = 41;
    fighter.u.gn.during_specials = 43;
    fighter.mv.co.turn.has_turned = false;
    fighter.mv.co.turn.facing_after = -1.0f;
    fighter.mv.co.turn.x8 = 1.0f;
    fighter.mv.co.turn.frames_to_turn = 3.0f;
    fighter.mv.co.turn.just_turned = true;
    fighter.mv.co.turn.x1C = HSD_PAD_A;
    fighter.x914[0].state = HitCapsule_Enabled;
    fighter.x914[0].damage = 13.0f;
    fighter.x914[0].x4C.x = 2.0f;
    item.kind = It_Kind_BombHei;
    item.msid = 2;
    item.pos.y = 8.0f;
    item.xD44_lifeTimer = 90.0f;
    ground.x0 = 4;
    ground.map_id = 2;
    ground.x20[3] = 19;
    camera.mode = CAMERA_STANDARD;
    camera.transform.position.z = 300.0f;
    camera.transform.target_position.z = 280.0f;
    camera.quake_scale = 1.0f;
    subject.state = 1;
    subject.pos.x = 4.0f;
    subject.ext.h.x = 3.0f;
    stage_anim.flags = AOBJ_LOOP;
    stage_anim.curr_frame = 12.0f;
    stage_anim.framerate = 1.0f;
    stage_root.aobj = &stage_anim;
    stage_root.scale.x = stage_root.scale.y = stage_root.scale.z = 1.0f;
    stage_root.child = &stage_child;
    stage_child.parent = &stage_root;
    stage_child.translate.x = 5.0f;
    stage_child.scale.x = stage_child.scale.y = stage_child.scale.z = 1.0f;

    uint64_t original = digest_fixture(&controller, &player, &fighter, &item,
                                       &ground, &camera, &subject, &stage_root);

    VsSceneController controller_copy = controller;
    StaticPlayer player_copy = player;
    Fighter fighter_copy = fighter;
    Item item_copy = item;
    Ground ground_copy = ground;
    Camera camera_copy = camera;
    CmSubject subject_copy = subject;
    HSD_AObj stage_anim_copy = stage_anim;
    HSD_JObj stage_root_copy = stage_root;
    HSD_JObj stage_child_copy = stage_child;
    stage_root_copy.aobj = &stage_anim_copy;
    stage_root_copy.child = &stage_child_copy;
    stage_child_copy.parent = &stage_root_copy;

    controller_copy.unk_1C = (u32*) (uintptr_t) 0x11111111;
    player_copy.player_entity[0] =
        (HSD_GObj*) (uintptr_t) 0x22222222;
    fighter_copy.gobj = (HSD_GObj*) (uintptr_t) 0x33333333;
    item_copy.entity = (HSD_GObj*) (uintptr_t) 0x44444444;
    ground_copy.gobj = (HSD_GObj*) (uintptr_t) 0x55555555;
    ground_copy.x0 = 0x12345678;
    ground_copy.xC0 = 123456.0f;
    uint64_t relocated = digest_fixture(
        &controller_copy, &player_copy, &fighter_copy, &item_copy,
        &ground_copy, &camera_copy, &subject_copy, &stage_root_copy);
    require(original == relocated,
            "semantic digest changed with process-local pointer values");

    item_copy.x5D4_hitboxes[2].hit.damage = 99.0f;
    require(original == digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "disabled item hitbox payload affected the digest");
    item.x5D4_hitboxes[2].hit.state = HitCapsule_Enabled;
    item_copy = item;
    item_copy.x5D4_hitboxes[2].hit.damage = 99.0f;
    uint64_t active_hitbox = digest_fixture(
        &controller_copy, &player_copy, &fighter_copy, &item, &ground_copy,
        &camera_copy, &subject_copy, &stage_root_copy);
    require(active_hitbox != digest_fixture(
                                &controller_copy, &player_copy, &fighter_copy,
                                &item_copy, &ground_copy, &camera_copy,
                                &subject_copy, &stage_root_copy),
            "active item hitbox damage was not detected");
    item.x5D4_hitboxes[2].hit.state = HitCapsule_Disabled;
    item_copy = item;

    item.xAC8_hurtboxNum = 1;
    item.xACC_itemHurtbox[0].state = HurtCapsule_Enabled;
    item.xACC_itemHurtbox[0].scale = 3.0f;
    item_copy = item;
    item_copy.xACC_itemHurtbox[0].a_pos.x = 94.0f;
    item_copy.xACC_itemHurtbox[0].bone_idx = 0x12345678;
    uint64_t fresh_hurtbox = nativeSemanticState_AddItem(
        nativeSemanticState_Begin(), &item, NULL);
    require(fresh_hurtbox == nativeSemanticState_AddItem(
                                 nativeSemanticState_Begin(), &item_copy, NULL),
            "unused item hurtbox cache affected the digest");
    item.xACC_itemHurtbox[0].skip_update_pos = 1;
    item_copy = item;
    item_copy.xACC_itemHurtbox[0].a_pos.x = 94.0f;
    require(nativeSemanticState_AddItem(nativeSemanticState_Begin(), &item,
                                        NULL) !=
                nativeSemanticState_AddItem(nativeSemanticState_Begin(),
                                            &item_copy, NULL),
            "active item hurtbox cache position was not detected");
    item.xAC8_hurtboxNum = 0;
    item.xACC_itemHurtbox[0].skip_update_pos = 0;
    item_copy = item;

    item_copy.xCAC_angle = 177;
    require(nativeSemanticState_AddItem(nativeSemanticState_Begin(), &item,
                                        NULL) ==
                nativeSemanticState_AddItem(nativeSemanticState_Begin(),
                                            &item_copy, NULL),
            "inactive item damage angle affected the digest");
    item.xCC8_knockback = 5.0f;
    item_copy = item;
    item_copy.xCAC_angle = 177;
    require(nativeSemanticState_AddItem(nativeSemanticState_Begin(), &item,
                                        NULL) !=
                nativeSemanticState_AddItem(nativeSemanticState_Begin(),
                                            &item_copy, NULL),
            "active item damage angle was not detected");
    item.xCC8_knockback = 0.0f;
    item_copy = item;

    item.kind = It_Kind_S_Scope;
    item_copy = item;
    item_copy.xDAC_itcmd_var0 = 0xE6452F40;
    item_copy.xDB0_itcmd_var1 = 0x21E;
    item_copy.xDB4_itcmd_var2 = 0x60;
    item_copy.xDB8_itcmd_var3 = 0x1234;
    require(nativeSemanticState_AddItem(nativeSemanticState_Begin(), &item,
                                        NULL) ==
                nativeSemanticState_AddItem(nativeSemanticState_Begin(),
                                            &item_copy, NULL),
            "Super Scope unused command words affected the digest");
    item_copy.xDC8_word.flags.x19 = !item.xDC8_word.flags.x19;
    require(nativeSemanticState_AddItem(nativeSemanticState_Begin(), &item,
                                        NULL) !=
                nativeSemanticState_AddItem(nativeSemanticState_Begin(),
                                            &item_copy, NULL),
            "Super Scope active flags were not detected");
    item.kind = It_Kind_DrMario_Vitamin;
    item_copy = item;
    item_copy.xDAC_itcmd_var0 = 1;
    require(nativeSemanticState_AddItem(nativeSemanticState_Begin(), &item,
                                        NULL) !=
                nativeSemanticState_AddItem(nativeSemanticState_Begin(),
                                            &item_copy, NULL),
            "Dr. Mario pill command state was not detected");
    item.kind = It_Kind_BombHei;
    item_copy = item;

    ground.x10_flags.b6 = 1;
    ground_copy = ground;
    ground_copy.xC0 = 123456.0f;
    uint64_t active_material = digest_fixture(
        &controller_copy, &player_copy, &fighter_copy, &item_copy, &ground,
        &camera_copy, &subject_copy, &stage_root_copy);
    require(active_material != digest_fixture(
                                   &controller_copy, &player_copy,
                                   &fighter_copy, &item_copy, &ground_copy,
                                   &camera_copy, &subject_copy,
                                   &stage_root_copy),
            "active ground material alpha was not detected");
    ground.x10_flags.b6 = 0;
    ground_copy = ground;

    fighter_copy.x914[0].x4C.x += 1.0f;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "fighter hitbox movement was not detected");
    fighter_copy = fighter;

    fighter_copy.mv.co.turn.frames_to_turn -= 1.0f;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "common fighter motion state was not detected");
    fighter_copy = fighter;

    Fighter damage_fighter = fighter;
    damage_fighter.motion_id = ftCo_MS_DamageFlyN;
    memset(&damage_fighter.mv, 0, sizeof(damage_fighter.mv));
    damage_fighter.mv.co.damage.x0 = 9.0f;
    damage_fighter.mv.co.damage.x4 = 1;
    damage_fighter.mv.co.damage.x8 = 2;
    damage_fighter.mv.co.damage.x14 = 3.0f;
    damage_fighter.mv.co.damage.x18 = 4;
    damage_fighter.mv.co.damage.x19 = 5;
    damage_fighter.mv.co.damage.x1A = 6;
    damage_fighter.mv.co.damage.x1B = 7;
    uint64_t damage_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &damage_fighter, 0, NULL);
    damage_fighter.mv.co.damage.x1B -= 1;
    require(damage_digest != nativeSemanticState_AddFighter(
                                 nativeSemanticState_Begin(), &damage_fighter,
                                 0, NULL),
            "damage motion state was not detected");

    Fighter clone_mario = { .kind = Ft_Kind_CustomFirst,
                            .behavior_kind = Ft_Kind_Mario };
    uint64_t clone_charge_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &clone_mario, 0, NULL);
    clone_mario.u.mr.x2234_tornadoCharge = true;
    require(clone_charge_digest != nativeSemanticState_AddFighter(
                                       nativeSemanticState_Begin(),
                                       &clone_mario, 0, NULL),
            "cloned Mario charge state was not detected");
    clone_mario.u.mr.x2234_tornadoCharge = false;
    uint64_t authored_state_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &clone_mario, 0, NULL);
    clone_mario.native_module_state[9] = 0xA5;
    require(authored_state_digest != nativeSemanticState_AddFighter(
                                         nativeSemanticState_Begin(),
                                         &clone_mario, 0, NULL),
            "authored per-fighter state was not detected");

    Fighter guard_fighter = fighter;
    guard_fighter.motion_id = ftCo_MS_Guard;
    memset(&guard_fighter.mv, 0, sizeof(guard_fighter.mv));
    guard_fighter.mv.co.guard.x0 = 1.0f;
    guard_fighter.mv.co.guard.x4 = 2.0f;
    guard_fighter.mv.co.guard.x8 = 3.0f;
    guard_fighter.mv.co.guard.xC = true;
    guard_fighter.mv.co.guard.x10 = 4.0f;
    guard_fighter.mv.co.guard.x14 = 5.0f;
    guard_fighter.mv.co.guard.x18 = 6.0f;
    guard_fighter.mv.co.guard.x1C = 7;
    guard_fighter.mv.co.guard.x20 = 8;
    guard_fighter.mv.co.guard.x24 = 9;
    guard_fighter.mv.co.guard.x2C = 10.0f;
    uint64_t guard_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &guard_fighter, 0, NULL);
    guard_fighter.mv.co.guard.x24 -= 1;
    require(guard_digest != nativeSemanticState_AddFighter(
                                nativeSemanticState_Begin(), &guard_fighter,
                                0, NULL),
            "guard motion state was not detected");

    Fighter ledge_fighter = fighter;
    ledge_fighter.motion_id = ftCo_MS_CliffWait;
    memset(&ledge_fighter.mv, 0, sizeof(ledge_fighter.mv));
    ledge_fighter.mv.co.cliff.ledge_id = 12;
    ledge_fighter.mv.co.cliff.x4 = 30.0f;
    ledge_fighter.mv.co.cliff.x8 = true;
    uint64_t ledge_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &ledge_fighter, 0, NULL);
    ledge_fighter.mv.co.cliff.x4 -= 1.0f;
    require(ledge_digest != nativeSemanticState_AddFighter(
                                nativeSemanticState_Begin(), &ledge_fighter,
                                0, NULL),
            "ledge motion state was not detected");

    Fighter item_throw_fighter = fighter;
    item_throw_fighter.motion_id = ftCo_MS_LightThrowAirF4;
    memset(&item_throw_fighter.mv, 0, sizeof(item_throw_fighter.mv));
    item_throw_fighter.mv.co.itemthrow.facing_dir = -1.0f;
    item_throw_fighter.mv.co.itemthrow4.anim_spd = 0.75f;
    item_throw_fighter.mv.co.itemthrow4.x8.x = 1.0f;
    item_throw_fighter.mv.co.itemthrow4.x8.y = 2.0f;
    item_throw_fighter.mv.co.itemthrow4.x8.z = 3.0f;
    uint64_t item_throw_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &item_throw_fighter, 0, NULL);
    item_throw_fighter.mv.co.itemthrow4.x8.y += 1.0f;
    require(item_throw_digest != nativeSemanticState_AddFighter(
                                    nativeSemanticState_Begin(),
                                    &item_throw_fighter, 0, NULL),
            "item-throw motion state was not detected");

    Fighter capture_fighter = fighter;
    capture_fighter.motion_id = ftCo_MS_CaptureWaitHi;
    memset(&capture_fighter.mv, 0, sizeof(capture_fighter.mv));
    capture_fighter.mv.co.capturewait.x0 = 11.0f;
    capture_fighter.mv.co.capturewait.x4 = 2.0f;
    capture_fighter.mv.co.capturewait.x8 = true;
    capture_fighter.mv.co.capturewait.xC = false;
    uint64_t capture_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &capture_fighter, 0, NULL);
    capture_fighter.mv.co.capturewait.xC = true;
    require(capture_digest != nativeSemanticState_AddFighter(
                                  nativeSemanticState_Begin(),
                                  &capture_fighter, 0, NULL),
            "capture-wait motion state was not detected");

    Fighter thrown_fighter = fighter;
    thrown_fighter.motion_id = ftCo_MS_ThrownF;
    memset(&thrown_fighter.mv, 0, sizeof(thrown_fighter.mv));
    thrown_fighter.mv.co.thrown.unk_bool = true;
    thrown_fighter.mv.co.thrown.anim_timer = 14.0f;
    thrown_fighter.mv.co.thrown.xC = false;
    uint64_t thrown_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &thrown_fighter, 0, NULL);
    thrown_fighter.mv.co.thrown.anim_timer -= 1.0f;
    require(thrown_digest != nativeSemanticState_AddFighter(
                                 nativeSemanticState_Begin(),
                                 &thrown_fighter, 0, NULL),
            "thrown motion state was not detected");

    Fighter shouldered_fighter = fighter;
    shouldered_fighter.motion_id = ftCo_MS_ShoulderedWait;
    memset(&shouldered_fighter.mv, 0, sizeof(shouldered_fighter.mv));
    shouldered_fighter.mv.co.shouldered.x0 = 6.0f;
    shouldered_fighter.mv.co.shouldered.x4 = true;
    uint64_t shouldered_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &shouldered_fighter, 0, NULL);
    shouldered_fighter.mv.co.shouldered.x0 -= 1.0f;
    require(shouldered_digest != nativeSemanticState_AddFighter(
                                     nativeSemanticState_Begin(),
                                     &shouldered_fighter, 0, NULL),
            "shouldered motion state was not detected");

    Fighter warp_fighter = fighter;
    warp_fighter.motion_id = ftCo_MS_WarpStarFall;
    memset(&warp_fighter.mv, 0, sizeof(warp_fighter.mv));
    warp_fighter.mv.co.warpstar.cur_pos.x = 1.0f;
    warp_fighter.mv.co.warpstar.cur_pos.y = 2.0f;
    warp_fighter.mv.co.warpstar.self_vel.y = -3.0f;
    warp_fighter.mv.co.warpstar.facing_dir = -1.0f;
    warp_fighter.mv.co.warpstar.x1C = 45;
    warp_fighter.mv.co.warpstar.ecb.bottom = -4.0f;
    uint64_t warp_digest = nativeSemanticState_AddFighter(
        nativeSemanticState_Begin(), &warp_fighter, 0, NULL);
    warp_fighter.mv.co.warpstar.ecb.bottom -= 1.0f;
    require(warp_digest != nativeSemanticState_AddFighter(
                                nativeSemanticState_Begin(), &warp_fighter,
                                0, NULL),
            "special common motion state was not detected");

    fighter_copy.u.gn.during_specials += 1;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "character-specific fighter state was not detected");
    fighter_copy = fighter;

    fighter_copy.bury_timer_2 += 1;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "fighter common tail state was not detected");
    fighter_copy = fighter;

    item_copy.xD44_lifeTimer -= 1.0f;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "item lifetime movement was not detected");
    item_copy = item;

    ground_copy.x20[3] += 1;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "ground controller movement was not detected");
    ground_copy = ground;

    stage_anim_copy.curr_frame += 1.0f;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "stage animation movement was not detected");
    stage_anim_copy = stage_anim;

    player_copy.stale_moves.StaleMoves[4].move_id += 1;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "player staleness movement was not detected");
    player_copy = player;

    camera_copy.transform.target_position.x += 1.0f;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "camera target movement was not detected");
    camera_copy = camera;

    subject_copy.force_inactive = true;
    require(original != digest_fixture(&controller_copy, &player_copy,
                                       &fighter_copy, &item_copy, &ground_copy,
                                       &camera_copy, &subject_copy,
                                       &stage_root_copy),
            "camera subject state was not detected");

    puts("Portable semantic gameplay state digest passed");

    HSD_GObj fighters_a[2] = { 0 };
    HSD_GObj items_a[2] = { 0 };
    HSD_GObj ground_a = { 0 };
    fighters_a[0].next = &fighters_a[1];
    items_a[0].next = &items_a[1];
    NativeSemanticObjectMap objects_a;
    nativeSemanticState_BuildObjectMap(&objects_a, &fighters_a[0], &items_a[0],
                                       &ground_a);
    Fighter clone_mario_relation = { .kind = Ft_Kind_CustomFirst,
                                     .behavior_kind = Ft_Kind_Mario };
    clone_mario_relation.u.mr.x223C_capeGObj = &items_a[0];
    uint64_t clone_cape_digest = nativeSemanticState_AddFighterRelations(
        nativeSemanticState_Begin(), &clone_mario_relation, &objects_a);
    clone_mario_relation.u.mr.x223C_capeGObj = &items_a[1];
    require(clone_cape_digest != nativeSemanticState_AddFighterRelations(
                                     nativeSemanticState_Begin(),
                                     &clone_mario_relation, &objects_a),
            "cloned Mario cape relation was not detected");
    Fighter related_fighter_a = { .kind = Ft_Kind_Fox,
                                  .gobj = &fighters_a[0],
                                  .victim_gobj = &fighters_a[1],
                                  .item_gobj = &items_a[0] };
    related_fighter_a.u.fx.x222C_blasterGObj = &items_a[1];
    related_fighter_a.motion_id = ftCo_MS_ThrownFF;
    related_fighter_a.mv.co.fighterthrow.victim = &fighters_a[1];
    related_fighter_a.x914[0].victims_1[0].victim = &fighters_a[1];
    Item related_item_a = { .entity = &items_a[0],
                            .owner = &fighters_a[0],
                            .toucher = &items_a[1] };
    uint64_t relations_a = nativeSemanticState_AddObjectMap(
        nativeSemanticState_Begin(), &objects_a);
    relations_a = nativeSemanticState_AddFighterRelations(
        relations_a, &related_fighter_a, &objects_a);
    relations_a = nativeSemanticState_AddItemRelations(
        relations_a, &related_item_a, &objects_a);

    HSD_GObj fighters_b[2] = { 0 };
    HSD_GObj items_b[2] = { 0 };
    HSD_GObj ground_b = { 0 };
    fighters_b[0].next = &fighters_b[1];
    items_b[0].next = &items_b[1];
    NativeSemanticObjectMap objects_b;
    nativeSemanticState_BuildObjectMap(&objects_b, &fighters_b[0], &items_b[0],
                                       &ground_b);
    Fighter related_fighter_b = { .kind = Ft_Kind_Fox,
                                  .gobj = &fighters_b[0],
                                  .victim_gobj = &fighters_b[1],
                                  .item_gobj = &items_b[0] };
    related_fighter_b.u.fx.x222C_blasterGObj = &items_b[1];
    related_fighter_b.motion_id = ftCo_MS_ThrownFF;
    related_fighter_b.mv.co.fighterthrow.victim = &fighters_b[1];
    related_fighter_b.x914[0].victims_1[0].victim = &fighters_b[1];
    Item related_item_b = { .entity = &items_b[0],
                            .owner = &fighters_b[0],
                            .toucher = &items_b[1] };
    uint64_t relations_b = nativeSemanticState_AddObjectMap(
        nativeSemanticState_Begin(), &objects_b);
    relations_b = nativeSemanticState_AddFighterRelations(
        relations_b, &related_fighter_b, &objects_b);
    relations_b = nativeSemanticState_AddItemRelations(
        relations_b, &related_item_b, &objects_b);
    require(relations_a == relations_b,
            "portable object relations depended on process addresses");
    related_fighter_b.u.fx.x222C_blasterGObj = &items_b[0];
    relations_b = nativeSemanticState_AddObjectMap(
        nativeSemanticState_Begin(), &objects_b);
    relations_b = nativeSemanticState_AddFighterRelations(
        relations_b, &related_fighter_b, &objects_b);
    relations_b = nativeSemanticState_AddItemRelations(
        relations_b, &related_item_b, &objects_b);
    require(relations_a != relations_b,
            "changed character-specific object relation was not detected");
    related_fighter_b.u.fx.x222C_blasterGObj = &items_b[1];
    related_fighter_b.mv.co.fighterthrow.victim = &fighters_b[0];
    relations_b = nativeSemanticState_AddObjectMap(
        nativeSemanticState_Begin(), &objects_b);
    relations_b = nativeSemanticState_AddFighterRelations(
        relations_b, &related_fighter_b, &objects_b);
    relations_b = nativeSemanticState_AddItemRelations(
        relations_b, &related_item_b, &objects_b);
    require(relations_a != relations_b,
            "changed thrown-victim relation was not detected");
    related_fighter_b.mv.co.fighterthrow.victim = &fighters_b[1];
    related_item_b.toucher = &fighters_b[1];
    relations_b = nativeSemanticState_AddObjectMap(
        nativeSemanticState_Begin(), &objects_b);
    relations_b = nativeSemanticState_AddFighterRelations(
        relations_b, &related_fighter_b, &objects_b);
    relations_b = nativeSemanticState_AddItemRelations(
        relations_b, &related_item_b, &objects_b);
    require(relations_a != relations_b,
            "changed object relationship was not detected");

    related_fighter_a.motion_id = ftCo_MS_Barrel;
    memset(&related_fighter_a.mv, 0, sizeof(related_fighter_a.mv));
    related_fighter_a.mv.co.barrel.x8 = &items_a[1];
    related_fighter_b.motion_id = ftCo_MS_Barrel;
    memset(&related_fighter_b.mv, 0, sizeof(related_fighter_b.mv));
    related_fighter_b.mv.co.barrel.x8 = &items_b[1];
    relations_a = nativeSemanticState_AddFighterRelations(
        nativeSemanticState_Begin(), &related_fighter_a, &objects_a);
    relations_b = nativeSemanticState_AddFighterRelations(
        nativeSemanticState_Begin(), &related_fighter_b, &objects_b);
    require(relations_a == relations_b,
            "portable special-motion relation depended on process addresses");
    related_fighter_b.mv.co.barrel.x8 = &items_b[0];
    relations_b = nativeSemanticState_AddFighterRelations(
        nativeSemanticState_Begin(), &related_fighter_b, &objects_b);
    require(relations_a != relations_b,
            "changed special-motion relation was not detected");

    // Immutable model layers are outside character state, including Chef
    // variant and Rescue retained ownership.
    ArticleModelLayers visibility_a = { 0 }, visibility_b = { 0 };
    Item chef_a = { .kind = It_Kind_GameWatch_Chef };
    Item chef_b = { .kind = It_Kind_GameWatch_Chef };
    for (s32 variant = 0; variant < IT_GAMEWATCH_CHEF_VARIANT_COUNT; ++variant) {
        chef_a.xDD4_itemVar.gamewatchchef.variant = variant;
        chef_b.xDD4_itemVar.gamewatchchef.variant = variant;
        chef_a.model_layers = &visibility_a;
        chef_b.model_layers = &visibility_b;
        require(chef_a.xDD4_itemVar.gamewatchchef.variant == variant &&
                    chef_b.xDD4_itemVar.gamewatchchef.variant == variant,
                "article visibility pointer overwrote Chef variant");
        require(chef_a.model_layers == &visibility_a,
                "Chef article layer binding differs from immutable metadata");
        uint64_t chef_hash = nativeSemanticState_AddItem(
            nativeSemanticState_Begin(), &chef_a, NULL);
        require(chef_hash == nativeSemanticState_AddItem(
                    nativeSemanticState_Begin(), &chef_b, NULL),
                "Chef state digest depended on an asset pointer address");
        chef_b.xDD4_itemVar.gamewatchchef.variant =
            (variant + 1) % IT_GAMEWATCH_CHEF_VARIANT_COUNT;
        require(chef_hash != nativeSemanticState_AddItem(
                    nativeSemanticState_Begin(), &chef_b, NULL),
                "Chef variant change was not detected");
    }
    related_item_a.kind = related_item_b.kind = It_Kind_GameWatch_Rescue;
    related_item_b.toucher = &items_b[1];
    related_item_a.xDD4_itemVar.gamewatchrescue.fighter_owner = &fighters_a[0];
    related_item_b.xDD4_itemVar.gamewatchrescue.fighter_owner = &fighters_b[0];
    related_item_a.model_layers = &visibility_a;
    related_item_b.model_layers = &visibility_b;
    require(related_item_a.xDD4_itemVar.gamewatchrescue.fighter_owner == &fighters_a[0] &&
                related_item_b.xDD4_itemVar.gamewatchrescue.fighter_owner == &fighters_b[0],
            "article visibility pointer overwrote Rescue owner");
    relations_a = nativeSemanticState_AddItemRelations(
        nativeSemanticState_Begin(), &related_item_a, &objects_a);
    relations_b = nativeSemanticState_AddItemRelations(
        nativeSemanticState_Begin(), &related_item_b, &objects_b);
    require(relations_a == relations_b,
            "Rescue owner relation depended on process addresses");
    related_item_b.xDD4_itemVar.gamewatchrescue.fighter_owner = &fighters_b[1];
    require(relations_a != nativeSemanticState_AddItemRelations(
                nativeSemanticState_Begin(), &related_item_b, &objects_b),
            "Rescue owner change was not detected");

    // Source Chef owns a real declared scalar; its unused native union is not
    // part of that backend's behavior. Observe the scalar without mutating it.
    const char state_source[] = "variant";
    NativeScriptSourceNode nodes[3] = {
        { .kind = NativeScriptSource_Root },
        { .kind = NativeScriptSource_Article },
        { .kind = NativeScriptSource_State },
    };
    NativeScriptSourceTree tree = {
        .source = state_source, .source_bytes = 7,
        .nodes = nodes, .count = 3, .capacity = 3,
    };
    NativeScriptValueDeclaration declaration = {
        .name = { .length = 7 }, .source_node = 2, .owner = 1,
        .count = 1, .type = NativeScriptType_I32,
        .default_value = { .type = NativeScriptType_I32 },
    };
    NativeScriptProgramIndex program = {
        .tree = &tree, .values = &declaration,
        .value_count = 1, .value_capacity = 1,
    };
    NativeScriptStorageField field;
    NativeScriptStorageOwner owners[2];
    NativeScriptStorageLayout layout = {
        .fields = &field, .field_capacity = 1,
        .owners = owners, .owner_capacity = 2,
    };
    NativeScriptDiagnostic diagnostic;
    NativeScriptStorageInstance storage;
    uint8_t bytes[4];
    require(nativeScriptStorage_BuildLayout(&layout, &program, &diagnostic) &&
                nativeScriptStorage_Create(&layout, NULL, 1, 3, 42,
                    bytes, sizeof(bytes), &storage, &diagnostic),
            "source observation fixture storage was not prepared");
    source_item = &items_b[0];
    source_view = (NativeScriptReceiverView) { &program, &storage };
    chef_a.entity = &items_a[0];
    chef_b.entity = source_item;
    chef_b.xDD4_itemVar.gamewatchchef.variant = 93; // Dead stock scratch.
    for (s32 variant = 0; variant < IT_GAMEWATCH_CHEF_VARIANT_COUNT; ++variant) {
        NativeScriptValue value = {
            .type = NativeScriptType_I32, .as.i64 = variant,
        };
        chef_a.xDD4_itemVar.gamewatchchef.variant = variant;
        require(nativeScriptStorage_Write(&storage, NATIVE_SCRIPT_STATE_SEMANTIC,
                    NULL, 0, &value, &diagnostic), "source variant write failed");
        uint64_t stock = nativeSemanticState_AddItem(
            nativeSemanticState_Begin(), &chef_a, NULL);
        require(stock == nativeSemanticState_AddItem(
                    nativeSemanticState_Begin(), &chef_b, NULL),
                "source variant was compared through dead native scratch");
        value.as.i64 = (variant + 1) % IT_GAMEWATCH_CHEF_VARIANT_COUNT;
        require(nativeScriptStorage_Write(&storage, NATIVE_SCRIPT_STATE_SEMANTIC,
                    NULL, 0, &value, &diagnostic), "source variant rewrite failed");
        require(stock != nativeSemanticState_AddItem(
                    nativeSemanticState_Begin(), &chef_b, NULL),
                "changed source variant was not detected");
    }
    chef_a.xDD4_itemVar.gamewatchchef.variant = 1;
    NativeScriptValue value = { .type = NativeScriptType_I32, .as.i64 = 1 };
    require(nativeScriptStorage_Write(&storage, NATIVE_SCRIPT_STATE_SEMANTIC,
                NULL, 0, &value, &diagnostic), "source variant reset failed");
    uint64_t stock = nativeSemanticState_AddItem(
        nativeSemanticState_Begin(), &chef_a, NULL);
    source_generation_live = false;
    require(stock != nativeSemanticState_AddItem(
                nativeSemanticState_Begin(), &chef_b, NULL),
            "retired source generation hid a comparison failure");
    source_generation_live = true;
    declaration.owner = 0;
    require(stock != nativeSemanticState_AddItem(
                nativeSemanticState_Begin(), &chef_b, NULL),
            "different owner declaration hid a comparison failure");
    declaration.owner = 1;
    declaration.name.length = 6;
    require(stock != nativeSemanticState_AddItem(
                nativeSemanticState_Begin(), &chef_b, NULL),
            "missing source declaration fell back to native scratch");
    declaration.name.length = 7;
    nativeScriptStorage_Destroy(&storage);
    require(stock != nativeSemanticState_AddItem(
                nativeSemanticState_Begin(), &chef_b, NULL),
            "retired storage hid a comparison failure");
    require(chef_b.xDD4_itemVar.gamewatchchef.variant == 93,
            "observation modified native item scratch");

    // The live generation-checked attachment relation is Rescue's actual
    // control owner; changing it must still change the relation digest.
    source_attachment_owner = &fighters_b[0];
    related_item_b.xDD4_itemVar.gamewatchrescue.fighter_owner = NULL;
    relations_a = nativeSemanticState_AddItemRelations(
        nativeSemanticState_Begin(), &related_item_a, &objects_a);
    require(relations_a == nativeSemanticState_AddItemRelations(
                nativeSemanticState_Begin(), &related_item_b, &objects_b),
            "source Rescue owner was compared through dead native scratch");
    source_attachment_owner = &fighters_b[1];
    require(relations_a != nativeSemanticState_AddItemRelations(
                nativeSemanticState_Begin(), &related_item_b, &objects_b),
            "changed source Rescue attachment owner was not detected");
    source_attachment_owner = NULL;
    require(relations_a != nativeSemanticState_AddItemRelations(
                nativeSemanticState_Begin(), &related_item_b, &objects_b),
            "cleared source Rescue attachment owner was not detected");
    source_item = NULL;

    transient_item_fields();
    puts("Portable semantic object relationships and source observations passed");
    return 0;
}
