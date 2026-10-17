#include "native_home.h"
#include <sysdolphin/baselib/native_workshop_gizmo.h>

#include <melee/gr/forward.h>
#include <melee/pl/forward.h>

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "gm_1601.h"
#include "gm_1A3F.h"
#include "gm_1A45.h"
#include "gmvs.h"
#include "gmvsmelee.h"
#include "gmvsmode.h"
#include "native_match_launch.h"
#include "native_online_session.h"
#include "native_practice.h"
#include "native_stock_content.h"
#include <melee/cm/camera.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/native_fighter_action_editor.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_source_package.h>
#include <melee/if/ifall.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lbvector.h>
#include <melee/lb/types.h>
#include <melee/mn/native_online_menu.h>
#include <melee/mn/types.h>
#include <melee/pl/player.h>
#include <melee/ui/native_shell.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/rumble_output.h>

static struct {
    CSSData* css;
    PlayerInitData local_players[GM_MAX_PLAYERS];
    NativeContentId costume_packages[GM_MAX_PLAYERS];
    NativeContentId costume_fighters[GM_MAX_PLAYERS];
    NativeContentId fighter_packages[GM_MAX_PLAYERS];
    uint8_t costume_characters[GM_MAX_PLAYERS];
    uint8_t costume_colors[GM_MAX_PLAYERS];
    NativeLobbySelection selection;
    bool have_local_players;
    bool start_requested;
    bool enter_stage_select;
    bool online_match;
    bool authoring;
    bool preview_active;
    bool preview_live_test;
    bool preview_restart;
    bool preview_transitioning;
    bool authoring_transitioning;
    unsigned preview_port;
    unsigned workshop_member;
    Fighter_GObj* workshop_pose_gobj;
    uint32_t workshop_pose_generation;
    FighterKind workshop_pose_kind;
    ftData* workshop_pose_data;
    const NativeSourcePackage* workshop_pose_source;
    NativeFighterLifetime workshop_pose_lifetime;
    NativeFighterAnimationClip workshop_pose_clip;
    FigaTree* workshop_pose_tree;
    const struct FtCostumeAssets* workshop_pose_costume;
    HSD_JObj* workshop_pose_root;
    FighterBone* workshop_pose_parts;
    float workshop_pose_frame;
    bool workshop_preview_hitboxes;
    bool workshop_camera_active;
    bool workshop_camera_owned;
    bool workshop_hud_hidden;
    bool workshop_hud_previously_hidden;
    bool workshop_background_saved;
    GXColor workshop_previous_background;
    Vec3 workshop_camera_target;
    float workshop_camera_yaw;
    float workshop_camera_pitch;
    float workshop_camera_distance;
    NativeContentId preview_candidate;
    NativeContentId preview_previous_candidate;
    NativeContentId preview_original_package;
    NativeContentId preview_original_costume;
    NativeContentId preview_original_costume_fighter;
    uint8_t preview_original_costume_character;
    uint8_t preview_original_costume_color;
    PlayerInitData preview_original_player;
    uint8_t result_local_ports;
    uint8_t result_local_seats[GM_MAX_PLAYERS];
    uint8_t physical_ports[GM_MAX_PLAYERS];
    uint8_t physical_count;
} home;

static bool has_package_id(const NativeContentId* id)
{
    const NativeContentId empty = { 0 };
    return memcmp(id, &empty, sizeof(empty)) != 0;
}

// Archive action indices identify animations, not fighter motion states.
// Several motion states share or omit animations, so the two sequences drift.
static bool workshop_motion_for_action(Fighter* fighter, unsigned action,
                                       FtMotionId* motion)
{
    if (fighter == NULL || motion == NULL) return false;
    const NativeSourcePackage* source = nativeFighterRuntime_SourcePackage(fighter->kind);
    if (source != NULL) {
        NativeFighterLifetime lifetime;
        if (!nativeFighterLifetime_Capture(fighter, &lifetime) ||
            !lifetime.source_owned || !source->ready || source->target != fighter->kind ||
            lifetime.program->bank != &source->compiler.bank ||
            lifetime.program->identity != source->program.identity ||
            fighter->ft_data != &source->assets.character.data ||
            fighter->x1C_actionStateList != source->assets.common_motions ||
            fighter->x20_actionStateList != source->assets.motions ||
            source->assets.facts.moves.common_count != (unsigned)fighter->x18 ||
            source->assets.facts.moves.first != (unsigned)fighter->x18) return false;
    } else if (fighter->kind >= Ft_Kind_CustomFirst) return false;
    for (unsigned state = 0; state < (unsigned) fighter->x18; ++state) {
        if (fighter->x1C_actionStateList[state].anim_id == (int) action) {
            *motion = (FtMotionId) state;
            return true;
        }
    }
    if (source == NULL || fighter->x20_actionStateList == NULL)
        return false;
    for (unsigned state = 0; state < source->assets.facts.moves.count; ++state) {
        if (fighter->x20_actionStateList[state].anim_id == (int) action) {
            *motion = (FtMotionId) (fighter->x18 + state);
            return true;
        }
    }
    return false;
}

bool nativeHome_CssPlayer(unsigned port, uint8_t* character, uint8_t* color)
{
    if (port >= GM_MAX_PLAYERS || home.css == NULL ||
        !nativeHome_IsSelection() ||
        has_package_id(&home.fighter_packages[port])) {
        return false;
    }
    const PlayerInitData* player = &home.css->vs.start.players[port];
    if (player->slot_type != Gm_PKind_Human &&
        player->slot_type != Gm_PKind_Cpu) {
        return false;
    }
    if (character != NULL) {
        *character = player->ckind;
    }
    if (color != NULL) {
        *color = player->color;
    }
    return true;
}

const NativeContentId* nativeHome_FighterPackage(unsigned port)
{
    return port < GM_MAX_PLAYERS &&
                   has_package_id(&home.fighter_packages[port])
               ? &home.fighter_packages[port]
               : NULL;
}

void nativeHome_SelectFighterPackage(unsigned port, const NativeContentId* id)
{
    if (port < GM_MAX_PLAYERS) {
        home.fighter_packages[port] = id != NULL ? *id : (NativeContentId){ 0 };
        home.costume_packages[port] = (NativeContentId){ 0 };
        home.costume_fighters[port] = (NativeContentId){ 0 };
    }
}

void nativeHome_ReplaceInstalledFighter(const NativeContentId* previous,
                                       const NativeContentId* next)
{
    if (previous == NULL || next == NULL) return;
    for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
        if (memcmp(&home.fighter_packages[port], previous,
                   sizeof(*previous)) == 0) {
            home.fighter_packages[port] = *next;
            home.costume_packages[port] = (NativeContentId){ 0 };
            home.costume_fighters[port] = (NativeContentId){ 0 };
        }
    }
    if (memcmp(&home.preview_original_package, previous,
               sizeof(*previous)) == 0) {
        home.preview_original_package = *next;
        home.preview_original_costume = (NativeContentId){ 0 };
        home.preview_original_costume_fighter = (NativeContentId){ 0 };
    }
    if (memcmp(&home.preview_candidate, previous,
               sizeof(*previous)) == 0) home.preview_candidate = *next;
    if (memcmp(&home.preview_previous_candidate, previous,
               sizeof(*previous)) == 0) home.preview_previous_candidate = *next;
}

const NativeContentId* nativeHome_CostumePackage(unsigned port)
{
    if (port >= GM_MAX_PLAYERS || home.css == NULL ||
        !nativeHome_IsSelection() ||
        !has_package_id(&home.costume_packages[port])) {
        return NULL;
    }
    const PlayerInitData* player = &home.css->vs.start.players[port];
    if (player->slot_type != Gm_PKind_Human &&
        player->slot_type != Gm_PKind_Cpu) {
        return NULL;
    }
    if (has_package_id(&home.fighter_packages[port])) {
        return memcmp(&home.costume_fighters[port],
                      &home.fighter_packages[port],
                      sizeof(NativeContentId)) == 0
                   ? &home.costume_packages[port] : NULL;
    }
    return !has_package_id(&home.costume_fighters[port]) &&
                   home.costume_characters[port] == player->ckind &&
                   home.costume_colors[port] == player->color
               ? &home.costume_packages[port] : NULL;
}

bool nativeHome_SelectCostumePackage(unsigned port, const NativeContentId* id)
{
    if (port >= GM_MAX_PLAYERS || home.css == NULL ||
        !nativeHome_IsSelection()) {
        return false;
    }
    const PlayerInitData* player = &home.css->vs.start.players[port];
    if (player->slot_type != Gm_PKind_Human &&
        player->slot_type != Gm_PKind_Cpu) {
        return false;
    }
    bool custom = has_package_id(&home.fighter_packages[port]);
    uint8_t character = custom ? 0 : player->ckind;
    uint8_t color = custom ? 0 : player->color;
    if (id == NULL || !has_package_id(id)) {
        home.costume_packages[port] = (NativeContentId){ 0 };
        return true;
    }
    char default_root[512];
    const char* root = nativeMatchLaunch_ContentRoot();
    if (root == NULL) {
        if (!nativeContentStore_DefaultRoot(default_root,
                                            sizeof(default_root))) {
            return false;
        }
        root = default_root;
    }
    uint8_t* package = NULL;
    size_t package_size;
    NativeCostumePackage metadata;
    const uint8_t* dat;
    size_t dat_size;
    bool valid = nativeContentStore_IsInstalled(root, id) &&
                 nativeContentStore_Read(root, id, &package, &package_size) &&
                 nativeContentStore_ParseCostume(package, package_size,
                                                 &metadata, &dat, &dat_size) &&
                 metadata.character == character &&
                 metadata.base_costume == color &&
                 memcmp(&metadata.fighter_id,
                        &home.fighter_packages[port],
                        sizeof(NativeContentId)) == 0;
    free(package);
    if (!valid) {
        return false;
    }
    home.costume_packages[port] = *id;
    home.costume_fighters[port] = home.fighter_packages[port];
    home.costume_characters[port] = character;
    home.costume_colors[port] = color;
    return true;
}

bool nativeHome_IsSelection(void)
{
    return gm_GetCurrentGameMode() == GM_VS &&
           gm_GetCurrentSceneIndex() == gmVsMode_State_Css;
}

bool nativeHome_IsPractice(void)
{
    return gm_GetCurrentGameMode() == GM_VS &&
           gm_GetCurrentSceneIndex() == gmVsMode_State_Practice;
}

// Player indices are mutable roles. The Workshop selection is an immutable
// package member, resolved through owned runtime metadata and the live registry.
static Fighter_GObj* workshop_preview_actor(bool allow_dormant)
{
    if (!home.preview_active || home.preview_transitioning ||
        home.preview_port >= GM_MAX_PLAYERS || !nativeHome_IsPractice()) return NULL;
    Fighter_GObj* source = Player_GetEntityAtIndex((int) home.preview_port, 0);
    uint32_t source_generation;
    if (!nativeCallbackSource_Snapshot(source, NativeCallbackSource_Fighter,
                                       &source_generation)) return NULL;
    Fighter* leader = GET_FIGHTER(source);
    NativeFighterLifetime leader_lifetime;
    if (!nativeFighterLifetime_Capture(leader, &leader_lifetime)) return NULL;
    const NativeSourcePackage* package = nativeFighterRuntime_SourcePackage(leader->kind);
    Fighter_GObj* actor = source;
    if (package != NULL) {
        if (!leader_lifetime.source_owned) return NULL;
        const uint32_t id = nativeFighterRuntime_MemberId(source, home.workshop_member);
        if (id == 0) return NULL;
        actor = ftLib_8008741C(id);
        if (!nativeCallbackSource_Alive(actor, id, NativeCallbackSource_Fighter))
            return NULL;
    } else if (leader->kind >= Ft_Kind_CustomFirst || leader_lifetime.source_owned ||
               home.workshop_member != 0) {
        // The ordinary single stock preview has no owned family metadata.
        return NULL;
    }
    NativeFighterLifetime actor_lifetime;
    if (!nativeFighterLifetime_Current(&leader_lifetime) ||
        !nativeFighterLifetime_Capture(GET_FIGHTER(actor), &actor_lifetime)) return NULL;
    if (package != NULL && !actor_lifetime.source_owned) return NULL;
    // A dormant alternate form is not an active preview model. Never activate
    // it or apply another member's animation to the visible role as a fallback.
    if (!allow_dormant && GET_FIGHTER(actor)->x221F_b3) return NULL;
    return actor;
}

unsigned nativeHome_WorkshopInitialMember(unsigned port)
{
    // This is queried during scene creation, before EnterPractice clears the
    // transition marker or any new actors have been published in Player.
    if (!home.authoring || !home.preview_active || home.preview_live_test ||
        port >= GM_MAX_PLAYERS || port != home.preview_port ||
        home.workshop_member >= NATIVE_FIGHTER_FAMILY_MEMBERS) return 0;
    CharacterKind character = Player_GetPlayerCharacter((int) port);
    if (character < ChKind_CustomFirst || character >= ChKind_RuntimeCapacity ||
        !Player_HasAlternateMember((int) port)) return 0;
    FighterKind first = (FighterKind) Player_800325C8(character, false);
    FighterKind second = (FighterKind) Player_800325C8(character, true);
    if (first < Ft_Kind_CustomFirst || first >= Ft_Kind_RuntimeCapacity ||
        second < Ft_Kind_CustomFirst || second >= Ft_Kind_RuntimeCapacity ||
        !nativeFighterRuntime_AlternatePair(first, second)) return 0;
    const NativeSourcePackage* a = nativeFighterRuntime_SourcePackage(first);
    const NativeSourcePackage* b = nativeFighterRuntime_SourcePackage(second);
    const NativeScriptFighterProgram* pa = nativeScriptFighter_Program(first);
    const NativeScriptFighterProgram* pb = nativeScriptFighter_Program(second);
    if (!a || !b || a == b || !a->ready || !b->ready || a->target != first || b->target != second ||
        !pa || !pb || pa->bank != &a->compiler.bank || pb->bank != &b->compiler.bank ||
        !pa->bank->valid || !pb->bank->valid ||
        pa->identity != a->program.identity || pb->identity != b->program.identity) return 0;
    return home.workshop_member;
}

bool nativeHome_WorkshopSelectMember(unsigned member, bool restart_preview)
{
    if (member >= NATIVE_FIGHTER_FAMILY_MEMBERS) return false;
    nativeHome_WorkshopPoseRelease();
    home.workshop_member = member;
    nativeHome_WorkshopPoseInvalidate();
    Fighter_GObj* actor = workshop_preview_actor(true);
    if (actor == NULL) return false;
    if (!GET_FIGHTER(actor)->x221F_b3) return true;
    // Only an explicit dropdown request may recreate a dormant owned alternate.
    // Save/refresh selection and live testing never alter family creation order.
    if (!restart_preview || home.preview_live_test ||
        home.preview_port >= GM_MAX_PLAYERS) return false;
    CharacterKind character = Player_GetPlayerCharacter((int) home.preview_port);
    if (character < ChKind_CustomFirst || character >= ChKind_RuntimeCapacity ||
        !Player_HasAlternateMember((int) home.preview_port)) return false;
    FighterKind first = (FighterKind) Player_800325C8(character, false);
    FighterKind second = (FighterKind) Player_800325C8(character, true);
    if (!nativeFighterRuntime_AlternatePair(first, second)) return false;
    const NativeSourcePackage* a = nativeFighterRuntime_SourcePackage(first);
    const NativeSourcePackage* b = nativeFighterRuntime_SourcePackage(second);
    const NativeScriptFighterProgram* pa = nativeScriptFighter_Program(first);
    const NativeScriptFighterProgram* pb = nativeScriptFighter_Program(second);
    if (!a || !b || a == b || !a->ready || !b->ready || a->target != first || b->target != second ||
        !pa || !pb || pa->bank != &a->compiler.bank || pb->bank != &b->compiler.bank ||
        !pa->bank->valid || !pb->bank->valid ||
        pa->identity != a->program.identity || pb->identity != b->program.identity) return false;
    return nativeHome_RequestPreview(&home.preview_candidate, false);
}

bool nativeHome_PracticeWatch(unsigned port, NativeHomePracticeWatch* result)
{
    if (result == NULL || port >= 4 || !nativeHome_IsPractice()) return false;
    Fighter_GObj* gobj = home.authoring && home.preview_active
        ? workshop_preview_actor(false) : Player_GetEntityAtIndex((int) port, 0);
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter,
                                       &generation)) return false;
    Fighter* fighter = GET_FIGHTER(gobj);
    if (fighter == NULL) return false;
    *result = (NativeHomePracticeWatch){
        .motion_id = (unsigned) fighter->motion_id,
        .animation_frame = fighter->cur_anim_frame,
        .velocity_x = fighter->self_vel.x,
        .velocity_y = fighter->self_vel.y,
        .grounded = fighter->ground_or_air == GA_Ground,
    };
    return true;
}

static bool workshop_pose_span(const void* base, size_t size,
                              const void* value, size_t bytes)
{
    uintptr_t start = (uintptr_t) base, address = (uintptr_t) value;
    return base != NULL && value != NULL && address >= start &&
        address - start <= size && bytes <= size - (address - start);
}

// Validate the decoded bank and the exact part traversal used by the shared
// figatree loader before it walks the selected actor's live joints.
static bool workshop_pose_tree_valid(Fighter_GObj* object,
    const NativeFighterPoseAssets* assets, const FigaTree* tree)
{
    if (!workshop_pose_span(assets->animation_arena,
            assets->animation_arena_size, tree, sizeof(*tree)) ||
        !isfinite(tree->frames) || tree->frames < 0 ||
        !nativeFighterPose_Validate(object, NativeFighterPose_CopyCostume,
            NativeFighterPose_Live, FtPart_TopN, NULL, NULL)) return false;
    Fighter* fighter = GET_FIGHTER(object);
    unsigned parts = ftPartsTable[fighter->kind]->parts_num;
    unsigned tracks = 0, part = 0;
    for (unsigned node = 0;; ++node, ++part) {
        if (!workshop_pose_span(assets->animation_bytes, assets->animation_size,
            (const void*) ((uintptr_t) tree->nodes + node), 1)) return false;
        int count = tree->nodes[node];
        if (count == -1) break;
        if (count < 0 || node == MAX_FT_PARTS ||
            tracks > FT_ANIMATION_SCRATCH_TRACK_CAPACITY - (unsigned) count)
            return false;
        tracks += (unsigned) count;
        while (part < parts && !fighter->parts[part].flags_b1) ++part;
        while (part < parts && fighter->parts[part].flags_b2) {
            u32 mask = ftParts_8007506C(fighter->kind, part);
            if (mask == 0 || (mask & fighter->x594_bits)) break;
            ++part;
        }
        if (part == parts || (part != 0 &&
            fighter->parts[part].xC <= fighter->parts[0].xC)) return false;
    }
    if (tracks && !workshop_pose_span(assets->animation_arena,
        assets->animation_arena_size, tree->tracks,
        tracks * sizeof(*tree->tracks))) return false;
    for (unsigned i = 0; i < tracks; ++i)
        if (!workshop_pose_span(assets->animation_bytes, assets->animation_size,
            tree->tracks[i].ad_head, tree->tracks[i].length)) return false;
    return true;
}

void nativeHome_WorkshopPoseRelease(void)
{
    nativeWorkshopGizmo_Clear();
    nativeModelView_Clear();
    if (home.workshop_camera_owned && nativeHome_IsPractice() &&
        Camera_80030178()) Camera_800300F0();
    home.workshop_camera_active = false;
    home.workshop_camera_owned = false;
    if (home.workshop_hud_hidden && !home.workshop_hud_previously_hidden)
        ifAll_ShowHUD();
    home.workshop_hud_hidden = false;
    if (home.workshop_background_saved && nativeHome_IsPractice()) {
        GXColor color = home.workshop_previous_background;
        Camera_SetBackgroundColor(color.r, color.g, color.b);
    }
    home.workshop_background_saved = false;
    // Release the exact retained actor, independent of later Player role swaps.
    // Registry validation precedes every read of its possibly retired storage.
    Fighter_GObj* posed = home.workshop_pose_gobj;
    if (nativeHome_IsPractice() &&
        nativeCallbackSource_Alive(posed, home.workshop_pose_generation,
                                    NativeCallbackSource_Fighter)) {
        Fighter* fighter = GET_FIGHTER(posed);
        if (home.workshop_pose_clip.bank == NativeFighterAnimation_Demo &&
            fighter->kind == home.workshop_pose_kind &&
            fighter->ft_data == home.workshop_pose_data &&
            nativeFighterRuntime_SourcePackage(fighter->kind) ==
                home.workshop_pose_source &&
            nativeFighterLifetime_Current(&home.workshop_pose_lifetime)) {
            NativeFighterPoseAssets assets;
            if (nativeFighterRuntime_PoseAssets(posed, &assets) &&
                nativeFighterRuntime_PoseClip(posed, fighter->anim_id) == fighter->x590 &&
                workshop_pose_tree_valid(posed, &assets, fighter->x590) &&
                isfinite(fighter->cur_anim_frame)) {
                ftAnim_80070758(posed->hsd_obj);
                ftAnim_8006FA58(fighter, FtPart_TopN, fighter->x108_costume_joint);
                ftAnim_8006F7C8(fighter, FtPart_TopN, false, fighter->x590);
                HSD_ForeachAnim(posed->hsd_obj, JOBJ_TYPE, 1,
                    HSD_AObjSetRate, AOBJ_ARG_AF, fighter->frame_speed_mul);
                ftAnim_80070710(posed->hsd_obj, fighter->cur_anim_frame);
                HSD_JObjAnimAll(posed->hsd_obj);
            }
        }
        if (nativeCallbackSource_Alive(posed, home.workshop_pose_generation,
                NativeCallbackSource_Fighter) && GET_FIGHTER(posed) == fighter &&
            home.workshop_preview_hitboxes &&
            fighter->kind == home.workshop_pose_kind &&
            fighter->ft_data == home.workshop_pose_data &&
            nativeFighterRuntime_SourcePackage(fighter->kind) ==
                home.workshop_pose_source &&
            nativeFighterLifetime_Current(&home.workshop_pose_lifetime)) {
            for (unsigned i = 0; i < 4; ++i)
                fighter->x914[i].state = HitCapsule_Disabled;
        }
        if (nativeCallbackSource_Alive(posed, home.workshop_pose_generation,
                NativeCallbackSource_Fighter) &&
            nativeFighterLifetime_Current(&home.workshop_pose_lifetime) &&
            nativeFighterRuntime_SourcePackage(GET_FIGHTER(posed)->kind) ==
                home.workshop_pose_source) HSD_GObj_80390C84(posed);
    }
    home.workshop_preview_hitboxes = false;
    home.workshop_pose_gobj = NULL;
    home.workshop_pose_generation = 0;
    home.workshop_pose_data = NULL;
    home.workshop_pose_source = NULL;
    home.workshop_pose_lifetime = (NativeFighterLifetime){0};
    home.workshop_pose_clip.index = UINT_MAX;
    home.workshop_pose_tree = NULL;
    home.workshop_pose_costume = NULL;
    home.workshop_pose_root = NULL;
    home.workshop_pose_parts = NULL;
}

static void workshop_camera_apply(void)
{
    const float yaw = home.workshop_camera_yaw;
    const float pitch = home.workshop_camera_pitch;
    const float distance = home.workshop_camera_distance;
    const float horizontal = cosf(pitch) * distance;
    Vec3 eye = {
        home.workshop_camera_target.x + sinf(yaw) * horizontal,
        home.workshop_camera_target.y + sinf(pitch) * distance,
        home.workshop_camera_target.z + cosf(yaw) * horizontal,
    };
    cm_80453004.free_int_pos = home.workshop_camera_target;
    cm_80453004.free_eye_pos = eye;
    HSD_GObj* camera = Camera_80030A50();
    if (camera != NULL && camera->hsd_obj != NULL) {
        HSD_CObjSetInterest(camera->hsd_obj, &home.workshop_camera_target);
        HSD_CObjSetEyePosition(camera->hsd_obj, &eye);
    }
}

bool nativeHome_WorkshopCameraActive(void)
{
    return home.workshop_camera_active && home.authoring &&
           home.preview_active && !home.preview_live_test &&
           !home.preview_transitioning && nativeHome_IsPractice() &&
           workshop_preview_actor(false) == home.workshop_pose_gobj &&
           nativeCallbackSource_Alive(home.workshop_pose_gobj,
               home.workshop_pose_generation, NativeCallbackSource_Fighter);
}

static void workshop_preview_presentation_begin(void)
{
    if (!home.workshop_hud_hidden) {
        home.workshop_hud_previously_hidden = ifAll_IsHUDHidden();
        ifAll_HideHUD();
        home.workshop_hud_hidden = true;
    }
    if (!home.workshop_background_saved) {
        home.workshop_previous_background = Camera_GetBackgroundColor();
        home.workshop_background_saved = true;
    }
    Camera_SetBackgroundColor(28, 31, 39);
}

void nativeHome_WorkshopSceneStarted(void)
{
    if (!home.authoring || !home.preview_active || home.preview_live_test ||
        !nativeHome_IsPractice()) return;
    workshop_preview_presentation_begin();
}

void nativeHome_WorkshopCameraDrag(float dx, float dy, bool pan)
{
    if (!nativeHome_WorkshopCameraActive()) return;
    if (pan) {
        const float yaw = home.workshop_camera_yaw;
        const float pitch = home.workshop_camera_pitch;
        const float scale = home.workshop_camera_distance * 0.0015f;
        home.workshop_camera_target.x +=
            (-cosf(yaw) * dx - sinf(yaw) * sinf(pitch) * dy) * scale;
        home.workshop_camera_target.y += cosf(pitch) * dy * scale;
        home.workshop_camera_target.z +=
            (sinf(yaw) * dx - cosf(yaw) * sinf(pitch) * dy) * scale;
    } else {
        home.workshop_camera_yaw -= dx * 0.01f;
        home.workshop_camera_pitch = fmaxf(-1.45f,
            fminf(1.45f, home.workshop_camera_pitch + dy * 0.01f));
    }
    workshop_camera_apply();
}

void nativeHome_WorkshopCameraZoom(float wheel_steps)
{
    if (!nativeHome_WorkshopCameraActive() || !isfinite(wheel_steps)) return;
    home.workshop_camera_distance = fmaxf(8.0f, fminf(220.0f,
        home.workshop_camera_distance * expf(wheel_steps * 0.14f)));
    workshop_camera_apply();
}

void nativeHome_WorkshopCameraReset(bool front)
{
    if (!nativeHome_WorkshopCameraActive()) return;
    if (front) {
        home.workshop_camera_yaw = 0.0f;
        home.workshop_camera_pitch = 0.0f;
    } else {
        Fighter_GObj* gobj = workshop_preview_actor(false);
        if (gobj != NULL && GET_FIGHTER(gobj) != NULL) {
            home.workshop_camera_target = GET_FIGHTER(gobj)->cur_pos;
            home.workshop_camera_target.y += 8.0f;
        }
        home.workshop_camera_yaw = 0.0f;
        home.workshop_camera_pitch = 0.12f;
        home.workshop_camera_distance = 45.0f;
    }
    workshop_camera_apply();
}

void nativeHome_WorkshopPreviewHitboxes(const NativeFighterHitbox hitboxes[4],
                                        unsigned active_mask)
{
    if (home.workshop_pose_gobj == NULL || !nativeHome_IsPractice() ||
        !nativeCallbackSource_Alive(home.workshop_pose_gobj,
            home.workshop_pose_generation, NativeCallbackSource_Fighter) ||
        workshop_preview_actor(false) != home.workshop_pose_gobj ||
        !home.authoring || !home.preview_active || home.preview_live_test)
        return;
    Fighter* fighter = GET_FIGHTER(home.workshop_pose_gobj);
    if (fighter->kind != home.workshop_pose_kind ||
        fighter->ft_data != home.workshop_pose_data ||
        nativeFighterRuntime_SourcePackage(fighter->kind) != home.workshop_pose_source ||
        !nativeFighterLifetime_Current(&home.workshop_pose_lifetime) ||
        fighter->parts == NULL || ftPartsTable == NULL || fighter->kind < 0 ||
        fighter->kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[fighter->kind] == NULL) return;
    home.workshop_preview_hitboxes = true;
    for (unsigned id = 0; id < 4; ++id) {
        HitCapsule* capsule = &fighter->x914[id];
        capsule->state = HitCapsule_Disabled;
        if (hitboxes == NULL || !(active_mask & (1u << id))) continue;
        const NativeFighterHitbox* hitbox = &hitboxes[id];
        const unsigned bone = hitbox->use_common_bone_ids
            ? ftParts_GetBoneIndex(fighter, hitbox->bone) : hitbox->bone;
        if (bone >= ftPartsTable[fighter->kind]->parts_num ||
            bone >= MAX_FT_PARTS || fighter->parts[bone].joint == NULL)
            continue;
        capsule->jobj = fighter->parts[bone].joint;
        capsule->x4 = hitbox->hit_group;
        capsule->scale = hitbox->radius / 256.0f;
        capsule->b_offset.x = hitbox->offset_z / 256.0f;
        capsule->b_offset.y = hitbox->offset_y / 256.0f;
        capsule->b_offset.z = hitbox->offset_x / 256.0f;
        capsule->element = hitbox->element;
        capsule->x43_b1 = hitbox->ignore_fighter_scale;
        capsule->x43_b2 = 0;
        capsule->state = HitCapsule_Enabled;
        ftColl_8007AD18(fighter, capsule);
    }
}

bool nativeHome_WorkshopPose(NativeFighterAnimationClip clip, float frame)
{
    if (!home.authoring || !home.preview_active || home.preview_live_test ||
        home.preview_transitioning || !nativeHome_IsPractice() ||
        (unsigned)clip.bank > NativeFighterAnimation_Demo ||
        clip.index >= 1024 || !isfinite(frame) ||
        frame < 0.0f ||
        (clip.bank == NativeFighterAnimation_Action && frame > 1000.0f))
        return false;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(gobj, NativeCallbackSource_Fighter,
                                       &generation)) {
        nativeHome_WorkshopPoseRelease();
        return false;
    }
    Fighter* fighter = GET_FIGHTER(gobj);
    FighterKind kind = fighter->kind;
    ftData* data = fighter->ft_data;
    const NativeSourcePackage* source = nativeFighterRuntime_SourcePackage(kind);
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fighter, &lifetime) ||
        (source != NULL && (!lifetime.source_owned || !source->ready || source->target != kind ||
            lifetime.program->bank != &source->compiler.bank ||
            lifetime.program->identity != source->program.identity)) ||
        (source == NULL && (kind >= Ft_Kind_CustomFirst || lifetime.source_owned))) return false;
    FtMotionId motion = 0;
    FigaTree* tree = NULL;
    const struct FtCostumeAssets* costume = NULL;
    if (clip.bank == NativeFighterAnimation_Action) {
        if (!workshop_motion_for_action(fighter, clip.index, &motion))
            return false;
    } else {
        NativeFighterPoseAssets assets;
        if (!nativeFighterRuntime_PoseAssets(gobj, &assets) ||
            clip.index >= assets.demo_count ||
            !workshop_pose_span(assets.character->import.arena,
                assets.character->import.arena_used, data->x14,
                assets.demo_count * sizeof(*data->x14))) return false;
        tree = data->x14[clip.index].x14;
        costume = assets.costume;
        bool cached = home.workshop_pose_gobj == gobj &&
            home.workshop_pose_generation == generation &&
            home.workshop_pose_kind == kind && home.workshop_pose_data == data &&
            home.workshop_pose_source == source &&
            nativeFighterLifetime_Current(&home.workshop_pose_lifetime) &&
            home.workshop_pose_clip.bank == clip.bank &&
            home.workshop_pose_clip.index == clip.index &&
            home.workshop_pose_tree == tree && home.workshop_pose_costume == costume &&
            home.workshop_pose_root == gobj->hsd_obj &&
            home.workshop_pose_parts == fighter->parts;
        if ((!cached && (!workshop_pose_tree_valid(gobj, &assets, tree) ||
                nativeFighterRuntime_PoseClip(gobj, fighter->anim_id) != fighter->x590 ||
                !workshop_pose_tree_valid(gobj, &assets, fighter->x590))) ||
            !isfinite(tree->frames) || frame > tree->frames) return false;
    }
    if (home.workshop_pose_gobj != gobj ||
        home.workshop_pose_generation != generation)
        nativeHome_WorkshopPoseRelease();
    if (!home.workshop_camera_active) {
        HSD_GObj* camera = Camera_80030A50();
        if (camera != NULL && camera->hsd_obj != NULL) {
            home.workshop_camera_owned = !Camera_80030178();
            Camera_8003006C();
            home.workshop_camera_target = GET_FIGHTER(gobj)->cur_pos;
            home.workshop_camera_target.y += 8.0f;
            home.workshop_camera_yaw = 0.0f;
            home.workshop_camera_pitch = 0.12f;
            home.workshop_camera_distance = 45.0f;
            cm_80453004.free_fov = 40.0f;
            home.workshop_camera_active = true;
            workshop_camera_apply();
        }
    }
    workshop_preview_presentation_begin();
    if (home.workshop_pose_gobj == gobj &&
        home.workshop_pose_kind == kind && home.workshop_pose_data == data &&
        home.workshop_pose_source == source &&
        nativeFighterLifetime_Current(&home.workshop_pose_lifetime) &&
        home.workshop_pose_clip.bank == clip.bank &&
        home.workshop_pose_clip.index == clip.index &&
        home.workshop_pose_tree == tree &&
        (clip.bank == NativeFighterAnimation_Action ||
            (home.workshop_pose_costume == costume &&
             home.workshop_pose_root == gobj->hsd_obj &&
             home.workshop_pose_parts == fighter->parts)) &&
        home.workshop_pose_frame == frame) return true;
    HSD_GObj_80390C5C(gobj);
    home.workshop_pose_gobj = gobj;
    home.workshop_pose_generation = generation;
    home.workshop_pose_kind = kind;
    home.workshop_pose_data = data;
    home.workshop_pose_source = source;
    home.workshop_pose_lifetime = lifetime;
    bool reload = home.workshop_pose_clip.bank != clip.bank ||
        home.workshop_pose_clip.index != clip.index ||
        home.workshop_pose_tree != tree || home.workshop_pose_costume != costume ||
        home.workshop_pose_root != gobj->hsd_obj ||
        home.workshop_pose_parts != fighter->parts;
    home.workshop_pose_clip = clip;
    home.workshop_pose_tree = tree;
    home.workshop_pose_costume = costume;
    home.workshop_pose_root = gobj->hsd_obj;
    home.workshop_pose_parts = fighter->parts;
    home.workshop_pose_frame = frame;
    if (clip.bank == NativeFighterAnimation_Action) {
        Fighter_ChangeMotionState(gobj, motion,
                              Ft_MF_SkipAnimVel | Ft_MF_SkipAttackCount |
                                  Ft_MF_SkipRumble,
                                  frame, 0.0f, 0.0f, NULL);
    } else {
        // The demo tree changes only the paused live rig. Gameplay action,
        // blend skeleton, callbacks and subaction bookkeeping remain intact.
        if (reload) {
            ftAnim_80070758(gobj->hsd_obj);
            ftAnim_8006FA58(fighter, FtPart_TopN, fighter->x108_costume_joint);
            ftAnim_8006F7C8(fighter, FtPart_TopN, false, tree);
            HSD_ForeachAnim(gobj->hsd_obj, JOBJ_TYPE, 1,
                HSD_AObjSetRate, AOBJ_ARG_AF, 0.0f);
        }
        ftAnim_80070710(gobj->hsd_obj, frame);
        HSD_JObjAnimAll(gobj->hsd_obj);
    }
    // Motion entry can synchronously dispatch authored callbacks.
    if (!nativeCallbackSource_Alive(gobj, generation, NativeCallbackSource_Fighter) ||
        GET_FIGHTER(gobj) != fighter || fighter->kind != kind ||
        fighter->ft_data != data || nativeFighterRuntime_SourcePackage(kind) != source ||
        !nativeFighterLifetime_Current(&lifetime) ||
        workshop_preview_actor(false) != gobj) {
        nativeHome_WorkshopPoseRelease();
        return false;
    }
    return true;
}

bool nativeHome_WorkshopBoneScreen(unsigned joint,
                                   NativeHomeWorkshopBoneScreen* result)
{
    if (result == NULL || !home.authoring || !home.preview_active ||
        home.preview_transitioning || !nativeHome_IsPractice()) return false;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL) return false;
    Fighter* fighter = GET_FIGHTER(gobj);
    if (fighter->parts == NULL || ftPartsTable == NULL || fighter->kind < 0 ||
        fighter->kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[fighter->kind] == NULL ||
        joint >= ftPartsTable[fighter->kind]->parts_num ||
        joint >= MAX_FT_PARTS || fighter->parts == NULL ||
        fighter->parts[joint].joint == NULL) return false;
    MtxPtr matrix = HSD_JObjGetMtxPtr(fighter->parts[joint].joint);
    const float point[3] = {
        matrix[0][3], matrix[1][3], matrix[2][3]
    };
    return nativeHome_WorkshopWorldScreen(point, result);
}

bool nativeHome_WorkshopWorldScreen(const float point[3],
                                    NativeHomeWorkshopBoneScreen* result)
{
    if (point == NULL || result == NULL || !home.authoring ||
        !home.preview_active || home.preview_transitioning ||
        !nativeHome_IsPractice()) return false;
    HSD_GObj* game_camera = Camera_80030A50();
    if (game_camera == NULL) return false;
    HSD_CObj* camera = game_camera->hsd_obj;
    if (camera == NULL) return false;
    Vec3 world = { point[0], point[1], point[2] };
    Vec3 screen;
    if (!isfinite(world.x) || !isfinite(world.y) || !isfinite(world.z) ||
        lbVector_WorldToScreen(camera, &world, &screen, 0) == NULL)
        return false;
    const float width = camera->viewport.xmax - camera->viewport.xmin;
    const float height = camera->viewport.ymax - camera->viewport.ymin;
    if (width <= 0.0f || height <= 0.0f) return false;
    result->x = (screen.x - camera->viewport.xmin) / width;
    result->y = (screen.y - camera->viewport.ymin) / height;
    return isfinite(result->x) && isfinite(result->y);
}

bool nativeHome_WorkshopBoneParent(unsigned joint, unsigned* parent)
{
    if (parent == NULL || !home.authoring || !home.preview_active ||
        home.preview_transitioning || !nativeHome_IsPractice()) return false;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL) return false;
    Fighter* fighter = GET_FIGHTER(gobj);
    if (ftPartsTable == NULL || fighter->kind < 0 ||
        fighter->kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[fighter->kind] == NULL || fighter->parts == NULL ||
        joint >= ftPartsTable[fighter->kind]->parts_num ||
        joint >= MAX_FT_PARTS || fighter->parts[joint].joint == NULL)
        return false;
    *parent = UINT_MAX;
    HSD_JObj* ancestor = fighter->parts[joint].joint->parent;
    for (unsigned depth = 0; ancestor != NULL && depth < MAX_FT_PARTS;
         ancestor = ancestor->parent, ++depth) {
        for (unsigned candidate = 0;
             candidate < ftPartsTable[fighter->kind]->parts_num &&
             candidate < MAX_FT_PARTS; ++candidate) {
            if (fighter->parts[candidate].joint == ancestor) {
                *parent = candidate;
                return true;
            }
        }
    }
    return true;
}

unsigned nativeHome_WorkshopBoneCount(void)
{
    if (!home.authoring || !home.preview_active ||
        home.preview_transitioning || !nativeHome_IsPractice()) return 0;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL) return 0;
    Fighter* fighter = GET_FIGHTER(gobj);
    if (ftPartsTable == NULL || fighter->kind < 0 ||
        fighter->kind >= Ft_Kind_RuntimeCapacity ||
        ftPartsTable[fighter->kind] == NULL) return 0;
    unsigned count = ftPartsTable[fighter->kind]->parts_num;
    return count < MAX_FT_PARTS ? count : MAX_FT_PARTS;
}

const char* nativeHome_WorkshopBoneRole(unsigned joint)
{
    static const char* const roles[] = {
        "TopN", "TransN", "XRotN", "YRotN", "HipN", "WaistN",
        "LLegJA", "LLegJ", "LKneeJ", "LFootJA", "LFootJ",
        "RLegJA", "RLegJ", "RKneeJ", "RFootJA", "RFootJ",
        "BustN", "LShoulderN", "LShoulderJA", "LShoulderJ",
        "LArmJ", "LHandN", "L1stNa", "L1stNb", "L2ndNa",
        "L2ndNb", "L3rdNa", "L3rdNb", "L4thNa", "L4thNb",
        "LThumbNa", "LThumbNb", "LHandNb", "NeckN", "HeadN",
        "RShoulderN", "RShoulderJA", "RShoulderJ", "RArmJ",
        "RHandN", "R1stNa", "R1stNb", "R2ndNa", "R2ndNb",
        "R3rdNa", "R3rdNb", "R4thNa", "R4thNb", "RThumbNa",
        "RThumbNb", "RHandNb", "ThrowN", "TransN2"
    };
    if (joint >= nativeHome_WorkshopBoneCount()) return NULL;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL) return NULL;
    Fighter* fighter = GET_FIGHTER(gobj);
    FighterPartsTable* parts = ftPartsTable[fighter->kind];
    if (parts == NULL || parts->part_to_joint == NULL) return NULL;
    for (unsigned role = 0; role < sizeof(roles) / sizeof(roles[0]); ++role)
        if (parts->part_to_joint[role] == joint) return roles[role];
    return NULL;
}

bool nativeHome_WorkshopBoneLocal(unsigned joint,
                                  NativeHomeWorkshopBoneLocal* result)
{
    if (result == NULL || joint >= nativeHome_WorkshopBoneCount())
        return false;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL ||
        GET_FIGHTER(gobj)->parts == NULL) return false;
    HSD_JObj* bone = GET_FIGHTER(gobj)->parts[joint].joint;
    if (bone == NULL) return false;
    result->rotation[0] = bone->rotate.x;
    result->rotation[1] = bone->rotate.y;
    result->rotation[2] = bone->rotate.z;
    result->scale[0] = bone->scale.x;
    result->scale[1] = bone->scale.y;
    result->scale[2] = bone->scale.z;
    result->position[0] = bone->translate.x;
    result->position[1] = bone->translate.y;
    result->position[2] = bone->translate.z;
    return true;
}

bool nativeHome_WorkshopSetBoneLocal(unsigned joint,
                                     const NativeHomeWorkshopBoneLocal* value)
{
    if (value == NULL || home.preview_live_test ||
        joint >= nativeHome_WorkshopBoneCount())
        return false;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL ||
        GET_FIGHTER(gobj)->parts == NULL) return false;
    HSD_JObj* bone = GET_FIGHTER(gobj)->parts[joint].joint;
    if (bone == NULL) return false;
    for (unsigned axis = 0; axis < 3; ++axis)
        if (!isfinite(value->rotation[axis]) ||
            !isfinite(value->scale[axis]) ||
            !isfinite(value->position[axis])) return false;
    bone->rotate.x = value->rotation[0];
    bone->rotate.y = value->rotation[1];
    bone->rotate.z = value->rotation[2];
    bone->scale.x = value->scale[0];
    bone->scale.y = value->scale[1];
    bone->scale.z = value->scale[2];
    bone->translate.x = value->position[0];
    bone->translate.y = value->position[1];
    bone->translate.z = value->position[2];
    HSD_JObjSetMtxDirty(bone);
    return true;
}

void nativeHome_WorkshopMeshView(const NativeHomeWorkshopMesh* meshes,
                                 unsigned count, int highlight,
                                 int weight_mesh, unsigned weight_joint, bool override_visibility)
{
    nativeModelView_Clear();
    const unsigned bone_count = nativeHome_WorkshopBoneCount();
    if (!nativeHome_WorkshopCameraActive() || bone_count == 0 ||
        (count != 0 && meshes == NULL))
    {
        return;
    }
    Fighter_GObj* actor = workshop_preview_actor(false);
    if (actor == NULL || GET_FIGHTER(actor) == NULL ||
        GET_FIGHTER(actor)->parts == NULL) return;
    Fighter* fighter = GET_FIGHTER(actor);
    HSD_JObj* weight_bone = weight_joint < bone_count
                                ? fighter->parts[weight_joint].joint
                                : NULL;
    nativeModelView_Begin(count, override_visibility);
    for (unsigned index = 0; index < count; ++index) {
        const unsigned part = meshes[index].joint;
        HSD_JObj* owner = part < bone_count
                              ? fighter->parts[part].joint
                              : NULL;
        HSD_DObj* mesh = NULL;
        if (owner && !(owner->flags & (JOBJ_PTCL | JOBJ_SPLINE))) {
            mesh = owner->u.dobj;
            for (unsigned ordinal = 0; mesh && ordinal < meshes[index].ordinal;
                 ++ordinal)
            {
                mesh = mesh->next;
            }
        }
        nativeModelView_Mesh(index, owner, mesh, meshes[index].hidden,
                             (int) index == highlight,
                             (int) index == weight_mesh ? weight_bone : NULL);
    }
}

void nativeHome_WorkshopGizmo(const float center[3], const float axes[3][3], float size, unsigned operation, unsigned selected)
{
    Fighter_GObj* actor = workshop_preview_actor(false);
    if (!nativeHome_WorkshopCameraActive() || !actor) { nativeWorkshopGizmo_Clear(); return; }
    nativeWorkshopGizmo_Set(GET_JOBJ(actor), center, axes, size, operation, selected);
}

bool nativeHome_WorkshopBoneMatrix(unsigned joint, float result[3][4])
{
    if (result == NULL || joint >= nativeHome_WorkshopBoneCount()) {
        return false;
    }
    Fighter_GObj* actor = workshop_preview_actor(false);
    if (actor == NULL || GET_FIGHTER(actor) == NULL ||
        GET_FIGHTER(actor)->parts == NULL) return false;
    Fighter* fighter = GET_FIGHTER(actor);
    HSD_JObj* bone = fighter->parts[joint].joint;
    if (!bone) {
        return false;
    }
    HSD_JObjSetupMatrix(bone);
    memcpy(result, bone->mtx, sizeof(Mtx));
    return true;
}

bool nativeHome_WorkshopRay(float x, float y, float origin[3],
                            float direction[3])
{
    if (!nativeHome_WorkshopCameraActive()) {
        return false;
    }
    HSD_GObj* gobj = Camera_80030A50();
    if (!gobj || !gobj->hsd_obj) {
        return false;
    }
    HSD_CObj* camera = gobj->hsd_obj;
    MtxPtr inv = HSD_CObjGetInvViewingMtxPtr(camera);
    if (!inv) {
        return false;
    }
    const float tangent = tanf(HSD_CObjGetFov(camera) * 0.00872664626f);
    const Vec3 local = { (2 * x - 1) * tangent * HSD_CObjGetAspect(camera),
                         (1 - 2 * y) * tangent, -1 };
    for (unsigned i = 0; i < 3; ++i) {
        origin[i] = inv[i][3];
        direction[i] =
            inv[i][0] * local.x + inv[i][1] * local.y + inv[i][2] * local.z;
    }
    const float length =
        sqrtf(direction[0] * direction[0] + direction[1] * direction[1] +
              direction[2] * direction[2]);
    if (length < 1e-6f) {
        return false;
    }
    for (unsigned i = 0; i < 3; ++i) {
        direction[i] /= length;
    }
    return true;
}

void nativeHome_WorkshopPoseInvalidate(void)
{
    home.workshop_pose_clip.index = UINT_MAX;
    home.workshop_pose_tree = NULL;
    home.workshop_pose_frame = -1.0f;
}

bool nativeHome_WorkshopHitboxScreen(unsigned hitbox, unsigned angle,
                                    NativeHomeWorkshopHitboxScreen* result)
{
    if (result == NULL || hitbox >= 4 || !home.authoring ||
        !home.preview_active || home.preview_transitioning ||
        !nativeHome_IsPractice()) return false;
    Fighter_GObj* gobj = workshop_preview_actor(false);
    if (gobj == NULL || GET_FIGHTER(gobj) == NULL) return false;
    Fighter* fighter = GET_FIGHTER(gobj);
    const HitCapsule* hit = &fighter->x914[hitbox];
    if (hit->state == HitCapsule_Disabled || hit->x43_b2) return false;
    HSD_GObj* game_camera = Camera_80030A50();
    if (game_camera == NULL || game_camera->hsd_obj == NULL) return false;
    HSD_CObj* camera = game_camera->hsd_obj;
    Vec3 world = { (hit->x58.x + hit->x4C.x) * 0.5f,
                   (hit->x58.y + hit->x4C.y) * 0.5f,
                   (hit->x58.z + hit->x4C.z) * 0.5f };
    Vec3 screen;
    if (!isfinite(world.x) || !isfinite(world.y) || !isfinite(world.z) ||
        lbVector_WorldToScreen(camera, &world, &screen, 0) == NULL)
        return false;
    const float width = camera->viewport.xmax - camera->viewport.xmin;
    const float height = camera->viewport.ymax - camera->viewport.ymin;
    if (width <= 0.0f || height <= 0.0f) return false;
    result->x = (screen.x - camera->viewport.xmin) / width;
    result->y = (screen.y - camera->viewport.ymin) / height;
    result->direction_valid = false;
    if (!isfinite(result->x) || !isfinite(result->y)) return false;
    // 361 is the Sakurai angle and depends on the victim's state/knockback.
    // Other values above 360 are special engine encodings, not fixed vectors.
    if (angle <= 360) {
        const float radians = angle * (3.14159265358979323846f / 180.0f);
        Vec3 tip = {
            world.x + cosf(radians) *
                (fighter->facing_dir < 0.0f ? -10.0f : 10.0f),
            world.y + sinf(radians) * 10.0f,
            world.z,
        };
        Vec3 projected;
        if (lbVector_WorldToScreen(camera, &tip, &projected, 0) != NULL) {
            result->direction_x = (projected.x - screen.x) / width;
            result->direction_y = (projected.y - screen.y) / height;
            result->direction_valid = isfinite(result->direction_x) &&
                                      isfinite(result->direction_y);
        }
    }
    return true;
}

void nativeHome_SetAuthoring(bool active)
{
    if (!active) nativeHome_WorkshopPoseRelease();
    home.authoring = active;
    if (active) {
        nativeHome_CancelStartMatch();
    } else if (home.preview_active && !home.preview_transitioning &&
               nativeHome_IsPractice()) {
        // The preview package is cached, not installed. Leave practice before
        // the room can consider this roster ready again.
        gm_801A4B60();
    }
}

bool nativeHome_PracticeTransitioning(void)
{
    return home.preview_transitioning || home.authoring_transitioning;
}

bool nativeHome_RequestAuthoringPractice(const NativeContentId* fighter, bool live_test)
{
    if (fighter == NULL || !has_package_id(fighter) || !home.authoring ||
        home.css == NULL || !nativeHome_IsSelection() ||
        nativeHome_PracticeTransitioning() ||
        nativeOnlineSession_State() != NativeLobbySessionState_Disconnected)
        return false;
    unsigned preview_port = GM_MAX_PLAYERS;
    for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
        const PlayerInitData* player = &home.css->vs.start.players[port];
        if (preview_port == GM_MAX_PLAYERS &&
            player->slot_type == Gm_PKind_Human) preview_port = port;
    }
    if (preview_port == GM_MAX_PLAYERS) preview_port = 0;
    for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
        if (port == preview_port) continue;
        const PlayerInitData* player = &home.css->vs.start.players[port];
        if ((player->slot_type == Gm_PKind_Human ||
             player->slot_type == Gm_PKind_Cpu) &&
            player->ckind == ChKind_None &&
            !has_package_id(&home.fighter_packages[port])) return false;
    }
    home.preview_port = preview_port;
    home.preview_original_player =
        home.css->vs.start.players[preview_port];
    home.preview_original_package = home.fighter_packages[preview_port];
    home.preview_original_costume = home.costume_packages[preview_port];
    home.preview_original_costume_fighter =
        home.costume_fighters[preview_port];
    home.preview_original_costume_character =
        home.costume_characters[preview_port];
    home.preview_original_costume_color = home.costume_colors[preview_port];
    home.preview_candidate = *fighter;
    home.preview_previous_candidate = (NativeContentId){ 0 };
    home.preview_active = true;
    home.preview_live_test = live_test;
    home.preview_restart = false;
    home.preview_transitioning = true;
    home.fighter_packages[preview_port] = *fighter;
    home.costume_packages[preview_port] = (NativeContentId){ 0 };
    home.css->vs.start.players[preview_port].slot_type = Gm_PKind_Human;
    home.css->vs.start.players[preview_port].ckind = CKind_Mario;
    home.authoring_transitioning = true;
    home.css->pending_scene_change = 1;
    gm_801A4B60();
    return true;
}

bool nativeHome_RequestPreview(const NativeContentId* id, bool live_test)
{
    nativeHome_WorkshopPoseRelease();
    if (id == NULL || !has_package_id(id) || !home.authoring ||
        !nativeHome_IsPractice() || home.preview_transitioning ||
        nativeOnlineSession_State() != NativeLobbySessionState_Disconnected) {
        return false;
    }
    unsigned port = 0;
    if (home.preview_active) {
        port = home.preview_port;
        home.preview_previous_candidate = home.fighter_packages[port];
    } else {
        while (port < 4 &&
               home.local_players[port].slot_type != Gm_PKind_Human) {
            ++port;
        }
        if (port == 4) {
            return false;
        }
        home.preview_port = port;
        home.preview_original_player = home.local_players[port];
        home.preview_original_package = home.fighter_packages[port];
        home.preview_original_costume = home.costume_packages[port];
        home.preview_original_costume_fighter =
            home.costume_fighters[port];
        home.preview_original_costume_character =
            home.costume_characters[port];
        home.preview_original_costume_color = home.costume_colors[port];
        home.preview_previous_candidate = (NativeContentId){ 0 };
    }
    home.preview_active = true;
    home.preview_live_test = live_test;
    home.preview_restart = true;
    home.preview_transitioning = true;
    home.preview_candidate = *id;
    gm_801A4B60();
    return true;
}

bool nativeHome_ReturnToWorkshopPreview(void)
{
    if (!home.authoring || !home.preview_active || !home.preview_live_test ||
        !nativeHome_IsPractice() || home.preview_transitioning ||
        home.preview_port >= GM_MAX_PLAYERS) return false;
    nativeHome_WorkshopPoseRelease();
    home.preview_live_test = false;
    home.preview_restart = true;
    home.preview_transitioning = true;
    home.preview_candidate = home.fighter_packages[home.preview_port];
    gm_801A4B60();
    return true;
}

static void finish_preview(void)
{
    if (!home.preview_active) {
        return;
    }
    unsigned port = home.preview_port;
    home.local_players[port] = home.preview_original_player;
    home.fighter_packages[port] = home.preview_original_package;
    home.costume_packages[port] = home.preview_original_costume;
    home.costume_fighters[port] =
        home.preview_original_costume_fighter;
    home.costume_characters[port] =
        home.preview_original_costume_character;
    home.costume_colors[port] = home.preview_original_costume_color;
    home.preview_active = false;
    home.preview_live_test = false;
    home.preview_restart = false;
    home.preview_transitioning = false;
    home.preview_candidate = (NativeContentId){ 0 };
    home.preview_previous_candidate = (NativeContentId){ 0 };
}

static void prepare_preload(const VsModeData* vs, bool practice)
{
    struct GameCache* cache = &lbDvd_GetPreloadCacheScene()->game_cache;
    cache->mode_kind = GM_VS;
    cache->stkind = practice ? St_Kind_Practice : vs->start.rules.stkind;
    for (int port = 0; port < 8; ++port) {
        cache->entries[port] = (PreloadCacheSceneEntry){ ChKind_None, 0, 1 };
    }
    for (int port = 0; port < 4; ++port) {
        const PlayerInitData* player = &vs->start.players[port];
        if (player->slot_type != Gm_PKind_NA) {
            cache->entries[port] =
                (PreloadCacheSceneEntry){
                    player->ckind >= ChKind_CustomFirst ? ChKind_None
                                                       : player->ckind,
                    player->color, 1 };
        }
    }
    if (practice) {
        cache->entries[NATIVE_PRACTICE_SANDBAG_SLOT] =
            (PreloadCacheSceneEntry){ ChKind_Sandbag, 0, 1 };
    }
    lbDvd_80018254();
}

static bool has_opposing_teams(const PlayerInitData players[GM_MAX_PLAYERS])
{
    unsigned teams = 0;
    for (int port = 0; port < GM_MAX_PLAYERS; ++port) {
        if (players[port].slot_type != Gm_PKind_NA) {
            if (players[port].team > 2) {
                return false;
            }
            teams |= 1U << players[port].team;
        }
    }
    return (teams & (teams - 1)) != 0;
}

// Presence order is also the canonical peer/port assignment order.
static bool collect_room_players(VsModeData* combined, bool* owner_requested,
                                 NativeContentId costume_packages[4],
                                 NativeContentId fighter_packages[4])
{
    const NativeSignalingPresence* presence = nativeOnlineSession_Presence();
    if (presence == NULL) {
        return false;
    }
    unsigned port = 0;
    *owner_requested = false;
    if (costume_packages != NULL) {
        memset(costume_packages, 0, sizeof(NativeContentId) * 4);
    }
    NativeContentId selected_fighters[4] = { 0 };
    if (fighter_packages != NULL) {
        memset(fighter_packages, 0, sizeof(NativeContentId) * 4);
    }
    for (unsigned peer = 0; peer < presence->participant_count; ++peer) {
        const NativeSignalingParticipant* participant =
            &presence->participants[peer];
        if (participant->active_mask == 0) {
            continue;
        }
        NativeLobbySelection selection;
        if (!nativeLobbySelection_Decode(&selection, participant->selection,
                                         participant->selection_size) ||
            !selection.confirmed ||
            selection.player_count != participant->local_players)
        {
            return false;
        }
        if (strcmp(participant->peer_id, presence->owner_peer_id) == 0) {
            *owner_requested = selection.start_requested;
        }
        for (unsigned local = 0; local < selection.player_count; ++local) {
            if ((participant->active_mask & (1U << local)) == 0) {
                continue;
            }
            if (port >= 4) {
                return false;
            }
            const NativeLobbyPlayerSelection* choice = &selection.players[local];
            uint8_t character, color;
            if (has_package_id(&choice->character_package_id)) {
                if (choice->character_id != 0 || choice->costume_id != 0 ||
                    has_package_id(&choice->costume_package_id)) {
                    return false;
                }
                selected_fighters[port] = choice->character_package_id;
                character = ChKind_CustomFirst;
                color = 0;
            } else {
                if (!nativeStockContent_CharacterFromId(
                        choice->character_id, &character) ||
                    !nativeStockContent_CostumeFromId(
                        choice->costume_id, character, &color)) {
                    return false;
                }
            }
            if (combined != NULL) {
                PlayerInitData* player = &combined->start.players[port];
                gm_SetupPlayerDefaults(player);
                player->slot_type = Gm_PKind_Human;
                player->ckind = character;
                player->color = color;
                player->team = choice->team;
            }
            if (costume_packages != NULL) {
                costume_packages[port] = choice->costume_package_id;
            }
            if (fighter_packages != NULL) {
                fighter_packages[port] = selected_fighters[port];
            }
            ++port;
        }
    }
    if (combined != NULL) {
        NativeContentId ids[4] = { 0 };
        unsigned count = 0;
        for (unsigned i = 0; i < port; ++i) {
            if (!has_package_id(&selected_fighters[i])) {
                continue;
            }
            unsigned index = 0;
            while (index < count &&
                   memcmp(ids[index].bytes, selected_fighters[i].bytes,
                          NATIVE_CONTENT_ID_BYTES) < 0) {
                ++index;
            }
            if (index < count &&
                memcmp(ids[index].bytes, selected_fighters[i].bytes,
                       NATIVE_CONTENT_ID_BYTES) == 0) {
                continue;
            }
            memmove(&ids[index + 1], &ids[index],
                    (count - index) * sizeof(ids[0]));
            ids[index] = selected_fighters[i];
            ++count;
        }
        for (unsigned i = 0; i < port; ++i) {
            if (!has_package_id(&selected_fighters[i])) {
                continue;
            }
            for (unsigned index = 0; index < count; ++index) {
                if (memcmp(ids[index].bytes, selected_fighters[i].bytes,
                           NATIVE_CONTENT_ID_BYTES) == 0) {
                    combined->start.players[i].ckind =
                        ChKind_CustomFirst + index;
                    break;
                }
            }
        }
    }
    if (combined != NULL) {
        for (unsigned unused = port; unused < GM_MAX_PLAYERS; ++unused) {
            gm_SetupPlayerDefaults(&combined->start.players[unused]);
        }
    }
    return port >= 2;
}

bool nativeHome_RoomRosterReady(bool* owner_requested,
                                NativeContentId costume_packages[4],
                                NativeContentId fighter_packages[4])
{
    return !home.authoring && !home.preview_active &&
           collect_room_players(NULL, owner_requested, costume_packages,
                                fighter_packages);
}

bool nativeHome_LocalRosterMatchesRoom(void)
{
    uint8_t count = nativeOnlineSession_LocalPlayers();
    if (count != home.selection.player_count || count != home.physical_count) {
        return false;
    }
    for (int port = 0; port < 4; ++port) {
        if (home.local_players[port].slot_type != Gm_PKind_Human &&
            home.local_players[port].slot_type != Gm_PKind_NA) {
            return false;
        }
    }
    return true;
}

bool nativeHome_CanStartMatch(void)
{
    if (home.authoring || home.preview_active ||
        !nativeHome_IsPractice() || home.enter_stage_select) {
        return false;
    }
    NativeLobbySessionState state = nativeOnlineSession_State();
    if (state == NativeLobbySessionState_Disconnected) {
        unsigned count = 0;
        for (int i = 0; i < 4; ++i) {
            count += home.local_players[i].slot_type != Gm_PKind_NA;
        }
        return count >= 2 &&
               (!gmVsMelee_GetVsData()->start.rules.is_teams ||
                has_opposing_teams(home.local_players));
    }
    bool requested;
    VsModeData combined = *gmVsMelee_GetVsData();
    return state == NativeLobbySessionState_Waiting &&
           nativeHome_LocalRosterMatchesRoom() &&
           nativeOnlineSession_IsOwner() &&
           collect_room_players(&combined, &requested, NULL, NULL) &&
           (!combined.start.rules.is_teams ||
            has_opposing_teams(combined.start.players));
}

bool nativeHome_TeamsNeedOpponents(void)
{
    if (!nativeHome_IsPractice() ||
        !gmVsMelee_GetVsData()->start.rules.is_teams) {
        return false;
    }
    if (nativeOnlineSession_State() == NativeLobbySessionState_Disconnected) {
        unsigned count = 0;
        for (int port = 0; port < GM_MAX_PLAYERS; ++port) {
            count += home.local_players[port].slot_type != Gm_PKind_NA;
        }
        return count >= 2 && !has_opposing_teams(home.local_players);
    }
    VsModeData combined = *gmVsMelee_GetVsData();
    bool requested;
    return nativeOnlineSession_IsOwner() &&
           collect_room_players(&combined, &requested, NULL, NULL) &&
           !has_opposing_teams(combined.start.players);
}

void nativeHome_RequestStartMatch(void)
{
    if (nativeHome_CanStartMatch()) {
        home.start_requested = true;
    }
}

void nativeHome_CancelStartMatch(void)
{
    home.start_requested = false;
    home.selection.start_requested = false;
}

void nativeHome_EnterSelection(CSSData* css)
{
    home.css = css;
    home.authoring_transitioning = false;
    const s8 physical_ports[4] = { 0, 1, 2, 3 };
    HSD_RumbleOutputSetPortMapping(physical_ports);
    nativeMatchLaunch_ClearNetplay();
    home.start_requested = false;
    home.enter_stage_select = false;
    home.online_match = false;
    home.result_local_ports = 0;
    home.selection.confirmed = false;
    home.selection.start_requested = false;
    if (home.have_local_players) {
        memcpy(css->vs.start.players, home.local_players,
               sizeof(home.local_players));
        for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
            if (has_package_id(&home.fighter_packages[port])) {
                css->vs.start.players[port].ckind = ChKind_None;
            }
        }
    }
    if (css->vs.start.rules.stkind == St_Kind_Practice ||
        css->vs.start.rules.stkind == 0)
    {
        css->vs.start.rules.stkind = St_Kind_Battle;
    }
}

bool nativeHome_LockSelection(CSSData* css)
{
    memcpy(home.local_players, css->vs.start.players, sizeof(home.local_players));
    for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
        if (home.local_players[port].slot_type == Gm_PKind_NA) {
            home.fighter_packages[port] = (NativeContentId){ 0 };
        }
    }
    uint8_t custom_kinds[GM_MAX_PLAYERS];
    if (!nativeMatchLaunch_ConfigureLocalFighters(home.fighter_packages,
                                                   custom_kinds,
                                                   home.preview_active)) {
        if (home.preview_transitioning) {
            if (has_package_id(&home.preview_previous_candidate)) {
                // The new package failed before practice loaded. Re-enter
                // practice with the last working preview exactly once.
                home.preview_candidate = home.preview_previous_candidate;
                home.preview_previous_candidate = (NativeContentId){ 0 };
                home.preview_restart = true;
                nativeShell_ShowRoomError(
                    "New preview could not load; restoring previous preview.");
                return false;
            }
            finish_preview();
        }
        nativeShell_ShowRoomError("Selected fighter could not load.");
        return false;
    }
    for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
        if (has_package_id(&home.fighter_packages[port])) {
            home.local_players[port].ckind = custom_kinds[port];
            css->vs.start.players[port].ckind = custom_kinds[port];
            css->vs.start.players[port].color = 0;
        }
    }
    home.have_local_players = true;
    home.selection = (NativeLobbySelection){ 0 };
    home.physical_count = 0;
    for (int i = 0; i < 4; ++i) {
        const PlayerInitData* player = &home.local_players[i];
        if (player->slot_type == Gm_PKind_NA) {
            continue;
        }
        if (player->slot_type == Gm_PKind_Human) {
            home.physical_ports[home.physical_count++] = (uint8_t) i;
        }
        NativeLobbyPlayerSelection* choice =
            &home.selection.players[home.selection.player_count++];
        if (has_package_id(&home.fighter_packages[i])) {
            choice->character_package_id = home.fighter_packages[i];
        } else {
            choice->character_id = nativeStockContent_CharacterId(player->ckind);
            choice->costume_id = nativeStockContent_CostumeId(player->ckind, player->color);
        }
        const NativeContentId* costume = nativeHome_CostumePackage(i);
        if (costume != NULL) {
            choice->costume_package_id = *costume;
        }
        choice->team = player->team;
        choice->selected = true;
    }
    if (!nativeLobbySelection_Validate(&home.selection)) {
        return false;
    }
    nativeCostumeRuntime_Clear();
    for (unsigned port = 0; port < GM_MAX_PLAYERS; ++port) {
        const NativeContentId* costume = &home.costume_packages[port];
        const PlayerInitData* player = &home.local_players[port];
        if (!has_package_id(costume) ||
            player->slot_type == Gm_PKind_NA) {
            continue;
        }
        bool custom = has_package_id(&home.fighter_packages[port]);
        if (memcmp(&home.costume_fighters[port],
                   &home.fighter_packages[port],
                   sizeof(NativeContentId)) != 0 ||
            (!custom &&
             (home.costume_characters[port] != player->ckind ||
              home.costume_colors[port] != player->color))) {
            continue;
        }
        bool loaded = custom
            ? nativeCostumeRuntime_LoadCustom(
                  port, costume, &home.fighter_packages[port],
                  Ft_Kind_CustomFirst +
                      (player->ckind - ChKind_CustomFirst) * NATIVE_FIGHTER_FAMILY_MEMBERS,
                  nativeMatchLaunch_ContentRoot(), true)
            : nativeCostumeRuntime_LoadInstalled(
                  port, costume, player->ckind, player->color,
                  nativeMatchLaunch_ContentRoot());
        if (!loaded) {
            nativeCostumeRuntime_Clear();
            nativeShell_ShowRoomError("Selected costume could not load.");
            return false;
        }
    }
    home.css = NULL;
    prepare_preload(&css->vs, true);
    return true;
}

int nativeHome_PhysicalPortForLocalPlayer(unsigned local_player)
{
    return local_player < home.physical_count
               ? home.physical_ports[local_player]
               : -1;
}

void nativeHome_EnterPractice(StartMeleeData* start)
{
    const int editor_port = home.authoring && home.preview_active &&
                                    !home.preview_live_test &&
                                    home.preview_port < GM_MAX_PLAYERS
                                ? (int) home.preview_port
                                : -1;
    nativePractice_Configure(start, editor_port);
    home.selection.confirmed = true;
    home.preview_transitioning = false;
    home.authoring_transitioning = false;
    home.preview_previous_candidate = (NativeContentId){ 0 };
}

void nativeHome_ExitPractice(void)
{
    nativeHome_WorkshopPoseRelease();
    if (home.preview_active && !home.preview_restart) {
        finish_preview();
    }
    if (home.enter_stage_select) {
        prepare_preload(gmVsMelee_GetVsData(), false);
        gm_SetNextGameModeStateId(gmVsMode_State_Sss);
    } else {
        home.selection.confirmed = false;
        home.selection.start_requested = false;
        gm_SetNextGameModeStateId(gmVsMode_State_Css);
    }
}

void nativeHome_EnterResults(void)
{
    const NativeNetplayLaunchConfig* config = nativeMatchLaunch_Netplay();
    home.result_local_ports = home.online_match && config != NULL
                                 ? config->local_port_mask : 0;
    if (config != NULL) {
        memcpy(home.result_local_seats, config->local_seats,
               sizeof(home.result_local_seats));
    }
}

void nativeHome_ApplyResultsInput(void)
{
    if (home.result_local_ports != 0 && gm_GetCurrentGameMode() == GM_VS &&
        gm_GetCurrentSceneIndex() == gmVsMode_State_Results) {
        // GameStatus retains each physical port's button history. CopyStatus
        // was remapped last tick, so its trigger edges are not local history.
        for (unsigned port = 0; port < 4; ++port) {
            if ((home.result_local_ports & (1U << port)) != 0 &&
                gmVsMelee_GetVsData()->start.players[port].slot_type ==
                    Gm_PKind_Human) {
                int physical = nativeHome_PhysicalPortForLocalPlayer(
                    home.result_local_seats[port]);
                HSD_PadCopyStatus[port] =
                    physical >= 0 ? HSD_PadGameStatus[physical]
                                  : (HSD_PadStatus){
                                        .err = PAD_ERR_NO_CONTROLLER,
                                    };
            } else {
                HSD_PadCopyStatus[port] = (HSD_PadStatus){
                    .err = PAD_ERR_NO_CONTROLLER,
                };
            }
        }
    }
}

void nativeHome_Tick(void)
{
    if (!nativeHome_IsSelection() && !nativeHome_IsPractice()) {
        return;
    }
    NativeLobbySessionState previous_state = nativeOnlineSession_State();
    nativeOnlineMenu_Pump();
    if (nativeHome_IsSelection() && home.preview_restart &&
        home.css != NULL) {
        unsigned port = home.preview_port;
        home.fighter_packages[port] = home.preview_candidate;
        home.costume_packages[port] = (NativeContentId){ 0 };
        home.css->pending_scene_change = 1;
        home.preview_restart = false;
        gm_801A4B60();
        return;
    }
    if (previous_state != NativeLobbySessionState_Disconnected &&
        nativeOnlineSession_State() == NativeLobbySessionState_Disconnected) {
        // A cancelled online start must not become an offline start.
        home.start_requested = false;
    }
    if (nativeOnlineSession_State() == NativeLobbySessionState_Waiting) {
        if (home.start_requested && nativeHome_TeamsNeedOpponents()) {
            home.start_requested = false;
        }
        NativeLobbySelection published = home.selection;
        uint8_t count = nativeOnlineSession_LocalPlayers();
        if (home.authoring || home.preview_active ||
            !nativeHome_LocalRosterMatchesRoom() ||
            !nativeHome_IsPractice()) {
            published = (NativeLobbySelection){ .player_count = count };
            if (!home.authoring && !home.preview_active &&
                nativeHome_IsSelection() && home.css) {
                unsigned seat = 0;
                for (unsigned port = 0; port < GM_MAX_PLAYERS && seat < count; ++port) {
                    const PlayerInitData* player = &home.css->vs.start.players[port];
                    if (player->slot_type != Gm_PKind_Human) continue;
                    NativeLobbyPlayerSelection* choice = &published.players[seat++];
                    const NativeContentId* package = nativeHome_FighterPackage(port);
                    if (package) {
                        choice->character_package_id = *package;
                        choice->selected = true;
                    } else if (player->ckind < CKind_Playable_Count) {
                        choice->character_id = nativeStockContent_CharacterId(player->ckind);
                        choice->costume_id = nativeStockContent_CostumeId(player->ckind, player->color);
                        choice->selected = true;
                    }
                    if (choice->selected) choice->team = player->team;
                }
            }
        } else {
            published.start_requested = home.start_requested;
        }
        nativeOnlineSession_UpdateSelection(&published);
    }
    if (home.authoring || home.preview_active ||
        !nativeHome_IsPractice() || home.enter_stage_select) {
        return;
    }
    if (nativeOnlineSession_State() == NativeLobbySessionState_Disconnected) {
        home.enter_stage_select = home.start_requested;
    } else if (nativeOnlineSession_State() == NativeLobbySessionState_Waiting) {
        bool owner_requested;
        VsModeData combined = *gmVsMelee_GetVsData();
        if (collect_room_players(&combined, &owner_requested, NULL, NULL) &&
            owner_requested &&
            (!nativeOnlineSession_IsOwner() ||
             !combined.start.rules.is_teams ||
             has_opposing_teams(combined.start.players))) {
            *gmVsMelee_GetVsData() = combined;
            home.enter_stage_select = true;
            home.online_match = true;
        }
    }
    if (home.enter_stage_select) {
        gm_Scene_Vs_Abort(OUTCOME_NO_CONTEST);
    }
}
