#include "gm_1A3F.h"

#include "gm_1A36.h"
#include "gm_1A45.h"
#include "gmmain_lib.h"
#include "gmmain.h"
#ifdef MELEE_NATIVE
#include "gmvsmode.h"
#include "gmvsmelee.h"
#include "gm_1601.h"
#include "gmresult.h"
#include "native_match_launch.h"
#include "native_netplay_match.h"
#include "native_match_object_census.h"
#include "native_snapshot_census.h"
#include "native_match_os_heap.h"
#include "native_replay.h"
#include <melee/ft/native_script_fighter.h>
#include <melee/it/native_article_model_scene.h>
#include <melee/it/native_article_resource_scene.h>
#include <melee/it/native_article_resource_model.h>
#include <melee/it/native_article_resource_animation.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_hsd_arena.h>
#endif
#include "gmscdata.h"
#include "types.h"
#include <dolphin/vi.h>
#include <melee/db/db.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbcardnew.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/lbmthp.h>
#include <melee/lb/lbsnap.h>
#include <melee/lb/types.h>
#include <melee/ty/toy.h>
#include <melee/ty/tydisplay.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/devcom.h>
#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/video.h>

struct routingInfo {
    u8 curr_mode;     ///< ::GameModeKind
    u8 pending_mode;  ///< ::GameModeKind
    u8 prev_mode;     ///< ::GameModeKind
    u8 curr_state_id; ///< from ::GameModeState::id
    u8 prev_state_id;
    u8 next_state_id;
};
ASSERT_SIZE(struct routingInfo, 0x6);

struct stateMachine {
    struct routingInfo routing;
    struct routingInfo backup_routing;
    u8 pending_mode_change; ///< ::bool
    u8 (*get_override)(void);
};
ASSERT_SIZE(struct stateMachine, 0x14);

/* 1A3F48 */ static void preloadState(GameModeState*);
/* 1A4014 */ static void gm_801A4014(GameMode*);
/* 1A43A0 */ static u8 runGameMode(u8 mode, u8 first_state);
/* 479D30 */ static struct stateMachine state_machine;

void preloadState(GameModeState* state)
{
    PreloadedGameModeState* preloaded_state;

    lbDvd_80018CF4(state->preload);
    switch (state->info.scene_kind) {
    case GS_STAFFROLL:
    case GS_RESULTS:
#ifdef MELEE_NATIVE
        HSD_SisLib_803A6048(0x20000);
#else
        HSD_SisLib_803A6048(0xC000);
#endif
        break;
    case GS_CSS:
        HSD_SisLib_803A6048(0x2400);
        break;
    default:
        HSD_SisLib_803A6048(0x4800);
        break;
    }
    preloaded_state = lbDvd_GetPreloadCacheScene();
    if (lbHeap_80015BB8(2) == 0) {
        preloaded_state->is_heap_persistent[0] = true;
    }
    if (lbHeap_80015BB8(3) == 0) {
        preloaded_state->is_heap_persistent[1] = true;
    }
    lbDvd_80018254();
#ifndef MELEE_NATIVE
    lb_8001C5A4();
    lb_8001D1F4();
#endif
#ifndef MELEE_NATIVE
    lbSnap_8001E27C();
#endif
    Toy_803127D4();
    tyDisplay_8031C8B8();
}

static inline u8 firstState(GameModeState* state, u8 next_id)
{
    for (; state->id != (u8) -1; state++) {
        do {
            if (state->id == next_id) {
                break;
            }
        } while (0);
        return state->id;
    }
    return 0;
}

static inline u8 nextState(GameModeState* states)
{
    GameModeState* next = states;
    u8 curr_id = state_machine.routing.curr_state_id;
    int i;
    u8 next_id;
    GameModeState* cur = states;

    for (i = 0; (next_id = next->id) != (u8) -1; i++) {
        if (cur->id > curr_id) {
            return states[i].id;
        }
        cur++;
        next++;
    }

    return firstState(states, next_id);
}

static inline GameModeState* findState(GameModeState* state)
{
    int i, j;
    for (i = state_machine.routing.curr_state_id; i < U8_MAX; i++) {
        for (j = 0; state[j].id != (u8) -1; j++) {
            if (i == state[j].id) {
                return &state[j];
            }
        }
    }
    return NULL;
}

void gm_801A4014(GameMode* mode)
{
    GameScene* scene;
    GameModeState* state;
    struct stateMachine* sm;
    struct GameSceneInfo* info;
    u32 dead; ///< @todo regswap hack
    PAD_STACK(2 * 4);

    sm = &state_machine;
    state = findState(mode->states);
    sm->routing.curr_state_id = state->id;

#ifdef MELEE_NATIVE
    bool netplay_requested = nativeMatchLaunch_Netplay() != NULL &&
                             (state->info.scene_kind == GS_VS ||
                              state->info.scene_kind == GS_SUDDEN_DEATH);
    if (netplay_requested && nativeMatchLaunch_SnapshotCensus()) {
        nativeSnapshotCensus_Abort("snapshot census is offline only");
        gmMain_ExitRequested = true;
        return;
    }
    if (netplay_requested && nativeMatchLaunch_ObjectCensus()) {
        nativeMatchObjectCensus_Abort("object census is offline only");
        gmMain_ExitRequested = true;
        return;
    }
    if (netplay_requested) {
        nativeNetplayMatch_Prepare(state->info.scene_kind == GS_SUDDEN_DEATH);
    }
#endif
    // The prior scene's exit callbacks and rollback shutdown have finished.
    // Retire its logical roots before preload can invalidate pointed userdata.
    // Expire private frames before destructors release sealed reservations.
    // Pool/object lifetimes remain live until their ordered teardown below.
    nativeMatchScene_Retire();
    nativeMatchOsHeap_Retire();
    HSD_ASSERTREPORT(0, nativeArticleResourceScene_Retire(),
        "Cannot retire resource nodes during an active callback or history.\n");
    HSD_ASSERTREPORT(0, nativeArticleResourceAnimation_Retire(),
        "Cannot retire resource animations during an active callback or history.\n");
    HSD_ASSERTREPORT(0, nativeArticleResourceModel_Retire(),
        "Cannot retire resource graphs during an active callback or history.\n");
    HSD_ASSERTREPORT(0, nativeArticleModelScene_Retire(),
        "Cannot retire secondary models during an active callback or history.\n");
    HSD_ASSERTREPORT(0, nativeGObjPool_RetireScene(),
        "Cannot retire GObj ownership during an active callback.\n");
    // Class/ordinary-object backing can survive the scene. Its history schema
    // cannot: retire admission before preload initializes this scene's types.
    if(nativeHsdArena.lifetime)
        HSD_ASSERTREPORT(0,nativeHsdArena_ResetHistory(),"Cannot retire HSD history.\n");
    preloadState(state);
    if (nativeMatchLaunch_TraceFighterState())
        OSReport("[scene] enter kind=%u sample=%llu\n", state->info.scene_kind,
            (unsigned long long) nativeReplay_SampleCount());
    if (nativeMatchLaunch_ObjectCensus())
        nativeMatchObjectCensus_BeginScene(state->info.scene_kind);
    if (state->on_enter != NULL) {
        state->on_enter(state);
    }
    info = &state->info;
    scene =
        (GameScene*) ((uintptr_t) gm_FindGameSceneHandler(info->scene_kind) |
                      (dead = 0));
    gm_801A4BD4();
    gm_801A4B88(info);
    if (scene->on_enter != NULL) {
        scene->on_enter(info->enter_data);
    }
    if (nativeMatchLaunch_ObjectCensus())
        nativeMatchObjectCensus_Sample(0, nativeReplay_SampleCount(), false);
    gm_801A4D34(scene->on_frame, info);
#ifdef MELEE_NATIVE
    if (nativeMatchLaunch_TraceFighterState()) {
        OSReport("[scene] exit kind=%u sample=%llu terminal=%u\n", info->scene_kind,
            (unsigned long long) nativeReplay_SampleCount(), gmMain_ExitRequested);
        // Shutdown reports actors in the terminal scene. Preserve the report
        // of actors leaving an ordinary transition before exit destroys them.
        if (!gmMain_ExitRequested) nativeScriptFighter_Report();
    }
#endif
    if (scene->on_exit != NULL) {
        scene->on_exit(info->exit_data);
    }
#ifdef MELEE_NATIVE
    bool sudden_death =
        info->scene_kind == GS_VS && !gmMain_ExitRequested &&
        (!netplay_requested || !nativeNetplayMatch_HasFailed()) &&
        !gm_WasMatchCanceled(gmVsMelee_VsExitInfo.match_end.outcome) &&
        gm_MatchHasMultipleWinners(&gmVsMelee_VsExitInfo.match_end);
    bool fighter_stage_failed = false;
    if (sudden_death &&
        !nativeMatchLaunch_PrepareSuddenDeathFighters()) {
        OSReport("[native fighter] Could not prepare sudden death fighters.\n");
        fighter_stage_failed = true;
        if (netplay_requested) {
            nativeNetplayMatch_Abort("could not prepare sudden death fighters");
        }
    }
    if (netplay_requested) {
        const MatchEnd* result = info->scene_kind == GS_VS
                                     ? &gmVsMelee_VsExitInfo.match_end
                                     : &gmVsMelee_SuddenDeathExitInfo.match_end;
        nativeNetplayMatch_Shutdown(sudden_death && !fighter_stage_failed,
                                    fighter_stage_failed || gmMain_ExitRequested
                                        ? NULL : result);
    }
#endif
    if (state->on_exit != NULL) {
        state->on_exit(state);
    }
    if (nativeMatchLaunch_ObjectCensus()) nativeMatchObjectCensus_FinishScene();
#ifdef MELEE_NATIVE
    if (fighter_stage_failed) {
        gm_SetNextGameModeStateId(gmVsMode_State_Css);
    }
#endif

    state_machine.routing.prev_state_id = sm->routing.curr_state_id;

    if (sm->routing.next_state_id) {
        sm->routing.curr_state_id = sm->routing.next_state_id - 1;
        sm->routing.next_state_id = 0;
    } else {
        sm->routing.curr_state_id = nextState(mode->states);
    }
#if !defined(MELEE_NATIVE)
    lb_8001CDB4();
    lb_8001B760(11);
#endif
    lbMthp_8001F800();
}

void* gm_GetGameModeStateEnterData(GameModeState* scene)
{
    return scene->info.enter_data;
}

void* gm_GetGameModeStateExitData(GameModeState* state)
{
    return state->info.exit_data;
}

void gm_SetGameModeStateId(u8 id)
{
    state_machine.routing.curr_state_id = id;
    state_machine.routing.prev_state_id = id;
}

/// @note Actually sets the next scene to the scene following the input
void gm_SetNextGameModeStateId(u8 curr_id)
{
    state_machine.routing.next_state_id = curr_id + 1;
}

u8 gm_GetPreviousSceneIndex(void)
{
    return state_machine.routing.prev_state_id;
}

u8 gm_GetCurrentSceneIndex(void)
{
    return state_machine.routing.curr_state_id;
}

void gm_SetNewGameModePending(void)
{
    state_machine.pending_mode_change = true;
}

void gm_SetPendingGameMode(u8 pending_mode)
{
    state_machine.routing.pending_mode = pending_mode;
}

void gm_ChangeGameModeAfterCurrentScene(int pending_mode)
{
    state_machine.routing.pending_mode = pending_mode;
    state_machine.pending_mode_change = true;
}

u8 gm_GetCurrentGameMode(void)
{
    return state_machine.routing.curr_mode;
}

u8 gm_GetPreviousGameMode(void)
{
    return state_machine.routing.prev_mode;
}

void gm_SetGameModeOverride(u8 (*mode)(void))
{
    state_machine.get_override = mode;
}

bool gm_Is1PMode(u8 mode)
{
    switch (mode) {
    case GM_CLASSIC:
    case GM_ADVENTURE:
    case GM_ALLSTAR:
    case GM_TARGET_TEST:
    case GM_TRAINING:
    case GM_HOME_RUN_CONTEST:
    case GM_10MAN_VS:
    case GM_100MAN_VS:
    case GM_3MIN_VS:
    case GM_15MIN_VS:
    case GM_ENDLESS_VS:
    case GM_CRUEL_VS:
    case GM_EVENT:
        return true;
    }
    return false;
}

static inline GameMode* findMode(u8 kind)
{
    GameMode* cur;
    for (cur = gm_GetAllGameModes(); cur->kind != GM_COUNT; cur++) {
        if (cur->kind == kind) {
            return cur;
        }
    }
    return NULL;
}

u8 runGameMode(u8 mode_kind, u8 first_state)
{
    u8 override;
    GameMode* mode;
    struct stateMachine* sm = &state_machine;
    PAD_STACK(2 * 4);

    mode = findMode(mode_kind);

    state_machine.pending_mode_change = false;
    state_machine.routing.curr_state_id = first_state;
    state_machine.routing.prev_state_id = first_state;
    state_machine.routing.next_state_id = 0;
    lbDvd_80018F58(mode->preloaded);
    if (mode->on_load != NULL) {
        mode->on_load();
    }
    while (!sm->pending_mode_change && !gmMain_ExitRequested) {
        if (state_machine.get_override != NULL &&
            (override = state_machine.get_override(), override != GM_COUNT))
        {
            state_machine.backup_routing = state_machine.routing;
            sm->pending_mode_change = false;
            sm->routing.curr_state_id = 0;
            sm->routing.prev_state_id = 0;
            sm->routing.next_state_id = 0;

            gm_801A4014(findMode(override));
            state_machine.routing = state_machine.backup_routing;
        } else {
            gm_801A4014(mode);
        }
    }
    if (mode->on_unload != NULL) {
        mode->on_unload();
    }
    return state_machine.routing.pending_mode;
}

/// UnclePunch: Scene_Main
void gm_801A4510(void)
{
    GameMode* modes;
    struct stateMachine* gamestate = &state_machine;
    int i;
    PAD_STACK(2 * 4);

    gm_GetAllGameModes();
    memzero(&state_machine, sizeof(struct stateMachine));
    modes = gm_GetAllGameModes();
    for (i = 0; modes[i].kind != GM_COUNT; i++) {
        if (modes[i].on_init != NULL) {
            modes[i].on_init();
        }
    }
    // Native defaults are initialized by gmMainLib_8015FBA4 before this loop.
    u8 first_state = 0;
#ifdef MELEE_NATIVE
    if (nativeMatchLaunch_IsDirect()) {
        if (!nativeMatchLaunch_PrepareDirectFighters()) {
            gmMain_ExitRequested = true;
            return;
        }
        nativeMatchLaunch_Apply();
        state_machine.routing.curr_mode = GM_VS;
        first_state = gmVsMode_State_Vs;
    } else
#endif
    {
        state_machine.routing.curr_mode = GM_VS;
    }
    state_machine.routing.prev_mode = GM_BOOT;

    while (!gmMain_ExitRequested) {
        u8 next_mode = runGameMode(state_machine.routing.curr_mode,
                                   first_state);
        first_state = 0;

        gamestate->routing.prev_mode = gamestate->routing.curr_mode;
        gamestate->routing.curr_mode = next_mode;
    }
}
