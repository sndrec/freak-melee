#include <stdio.h>
#include <melee/gm/native_match_control_history.h>
#include "gm_1A45.h"
#include "native_match_classes.h"

#include "gm_1A36.h"
#include "gm_1A45.static.h"
#include "gm_unsplit.h"
#include "gmmain.h"
#include "gmmain_lib.h"
#include "gmscdata.h"
#include "gmvs.h"
#include "native_match_launch.h"
#include "native_home.h"
#include "native_netplay_match.h"
#include "native_netplay_timing.h"
#include "native_rollback_census.h"
#include "native_rollback_probe.h"
#include "native_rollback_benchmark.h"
#include "native_match_object_census.h"
#include "native_snapshot_census.h"
#include "native_replay.h"
#include "native_gm_loop_snapshot.h"
#include "native_versus_control_snapshot.h"
#include <melee/pl/native_player_snapshot.h>
#include <melee/cm/native_camera_subject_snapshot.h>
#include <melee/cm/native_camera_snapshot.h>
#include <melee/gr/native_stage_snapshot.h>
#include <melee/gr/native_stage_hazard_snapshot.h>
#include <melee/gr/native_stage_generator_snapshot.h>
#include <melee/lb/native_dynamics_snapshot.h>
#include <melee/ef/native_effect_control_snapshot.h>
#include <sysdolphin/baselib/native_particle_control_snapshot.h>
#include <aurora/aurora.h>
#include <aurora/event.h>
#include <melee/db/db.h>
#include <melee/db/dbnativecontrol.h>
#include <melee/ui/native_shell.h>
#include <melee/ui/native_hub.h>
#include <melee/ui/native_shell_profiles.h>
#include <melee/ui/native_shell_rules.h>
#include <melee/if/ifcoget.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lb_0195.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbheap.h>
#include <SDL3/SDL_timer.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/audio_rollback.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/native_random_snapshot.h>
#include <sysdolphin/baselib/native_render_phase.h>
#include <sysdolphin/baselib/native_callback_profile.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/hsd_3924.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/perf.h>
#include <sysdolphin/baselib/sobjlib.h>
#include <sysdolphin/baselib/state.h>
#include <string.h>
#include <stdlib.h>

static u64 gm_803DA888[8] = {
    0, 0x82FFFA, 0, 0x8EFFFA, 0x800FFA, 0x808FFA, 0x800FFA, 0,
};

u64 gm_803DA8C8[2] = { -1, -1 };

// Actual mutable roots above remain in the page oracle. The frozen scene/code
// identity and in-flight guard below are host control, never rewound by Load.
#pragma clang section data = "" bss = ""
static struct {
    uint64_t epoch, object_epoch;
    GameSceneInfo* scene;
    GameSceneInfo descriptor;
    struct gm_DbPauseInputHandlers pause_handlers;
    Event pre_proc, debug_callback;
    HSD_GObjLibInitDataType config;
    unsigned activity;
    bool active;
} loop_snapshot;

int gm_GetDbPauseFlag(int bit)
{
    return gm_80479D58.unk_10.x0 & (1ULL << bit);
}

int gm_801A4624(void)
{
    return gm_80479D58.unk_10.x0;
}

void gm_SetDbPauseFlag(int bit)
{
    gm_80479D58.unk_10.x0 |= 1ULL << bit;
}

void gm_ClearDbPauseFlag(int bit)
{
    gm_80479D58.unk_10.x0 &= ~(1ULL << bit);
}

bool gm_801A46B8(int bit)
{
    return gm_80479D58.unk_10.x2 & (1ULL << bit);
}

bool fn_801A46F4(void)
{
    int i;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        HSD_PadStatus* pad = &HSD_PadMasterStatus[(u8) i];
        if (pad->err == 0 && (pad->trigger & HSD_PAD_DPADUP) &&
            (pad->button & HSD_PAD_X))
        {
            return true;
        }
    }
    return false;
}

bool fn_801A47E4(void)
{
    int i;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        HSD_PadStatus* pad = &HSD_PadMasterStatus[(u8) i];
        if (pad->err == 0 && (pad->trigger & HSD_PAD_Z)) {
            return true;
        }
    }
    return false;
}

u64 gm_801A48A4(u8 arg0)
{
    int i;
    u64 result = 0;

    for (i = 0; i < ARRAY_SIZE(gm_803DA888); i++) {
        if (arg0 & 1) {
            result |= gm_803DA888[i];
        }
        arg0 >>= 1;
    }

    return result;
}

void gm_801A4970(struct gm_DbPauseInputHandlers* db_input)
{
    HSD_PadStatus* temp_r3;
    s8 var_r26;
    s8* temp_r4;
    u64 temp_ret;
    int i;
    PAD_STACK(8);

    var_r26 = 0;
    for (i = 0; i < PAD_MAX_CONTROLLERS; i++) {
        temp_r3 = &HSD_PadMasterStatus[(u8) i];
        if ((temp_r3->trigger & HSD_PAD_DPADRIGHT) &&
            (temp_r3->button & HSD_PAD_X))
        {
            lbHeap_80015DF8();
            OSReport("[hsdDumpClassStat] -- Report --\n");
            hsdDumpClassStat(NULL, 0, 1);
            OSReport("\n");
            OSReport("[HSD_ObjDumpStat] -- Report --\n");
            HSD_ObjDumpStat();
            OSReport("\n");
            db_PrintEntityCounts();
            db_PrintThreadInfo();
            if (gm_804D6728 != NULL) {
                gm_801653C8(gm_804D6728);
                gm_804D6728 = NULL;
            } else {
                gm_804D6728 = gm_80165388(0x19, 0x3F, 0, 0xFE);
                if (gm_804D6724 != NULL) {
                    gm_804D6724();
                }
            }
        }
    }

    if (db_input->check_pause != NULL && db_input->check_pause()) {
        if (gm_GetDbPauseFlag(0)) {
            gm_80479D58.unk_10.x0 &= ~1;
        } else {
            gm_80479D58.unk_10.x0 |= 1;
        }
    }
    if (gm_GetDbPauseFlag(0)) {
        if (db_input->check_framestep != NULL && db_input->check_framestep()) {
            gm_80479D58.unk_10.x2 |= 1;
        }
    }
}

void gm_SetDbPauseInputHandlers(Predicate check_db_pause,
                                Predicate check_db_framestep)
{
    gm_80479D58.unk_10.db_input.check_pause = check_db_pause;
    gm_80479D58.unk_10.db_input.check_framestep = check_db_framestep;
}

void gm_801A4B1C(void)
{
    gm_SetDbPauseInputHandlers(fn_801A46F4, fn_801A47E4);
}

void gm_SetPreGObjProcCallback(Event cb)
{
    gm_80479D58.unk_10.pre_gobj_proc = cb;
}

void gm_801A4B50(int arg0)
{
    gm_80479D58.unk_10.unk_34 = arg0;
}

void gm_801A4B60(void)
{
    gm_80479D58.unk_C = 1;
}

void gm_801A4B74(void)
{
    gm_80479D58.unk_C = 2;
}

void gm_801A4B88(struct GameSceneInfo* info)
{
    gm_804D6720 = info;
}

/// @brief returns a pointer to the current scenes enter data
void* gm_GetCurrentSceneEnterData(void)
{
    return gm_804D6720->enter_data;
}

/// @brief returns a pointer to the current scenes exit data
void* gm_GetCurrentSceneExitData(void)
{
    return gm_804D6720->exit_data;
}

u32 gm_801A4BA8(void)
{
    return gm_80479D58.unk_0;
}

u32 gm_801A4BB8(void)
{
    return gm_80479D58.unk_8;
}

HSD_GObj* gm_801A4BC8(void)
{
    return gm_804D672C;
}

void fn_801A4BD0(HSD_GObj* gobj) {}

void gm_801A4BD4(void)
{
    PAD_STACK(0x18);

    loop_snapshot.active = false;

    gm_SetDbPauseInputHandlers(fn_801A46F4, fn_801A47E4);
    gm_SetPreGObjProcCallback(NULL);
    gm_801A4B50(0);

    lbInput_SetGameSpeed(1.0f);
    HSD_GObj_803912E0(&gm_80479D48.initdata);
    gm_80479D48.initdata.gproc_pri_max = 0x18;
    HSD_SObjLib_804D7960 =
        HSD_GObj_803912A8(&gm_80479D48.initdata, &HSD_SObjLib_8040C3A4);
    HSD_SObjLib_803A44A4();
    gm_80479D48.initdata.unk_2 = &gm_80479D58.unk_10.unk_28;
    HSD_GObj_80391304(&gm_80479D48.initdata);
    hsd_80392474();
    un_802FF78C();
    gm_804D672C = GObj_Create(14, 0, 0);
    if (gm_804D672C != NULL) {
        HSD_GObj_SetupProc(gm_804D672C, fn_801A4BD0, 0);
    }
    gm_804D6728 = NULL;
    gm_804D6724 = NULL;
    gm_801A3E88();
    lbAudioAx_8002835C();
    lb_80014534();
}

GameScene* gm_FindGameSceneHandler(u8 kind)
{
    GameScene* cur;
    for (cur = gm_GetAllGameScenes(); cur->kind != GS_COUNT; cur++) {
        if (cur->kind == kind) {
            return cur;
        }
    }
    return NULL;
}

static inline u64 maybe_gm_801A48A4(u8 i)
{
    u64 temp_ret = gm_801A48A4(i);
    if (gm_80479D58.unk_10.unk_38_0) {
        return temp_ret;
    } else {
        return -1ULL;
    }
}

void gm_801A4D34(void (*on_frame)(void), UNUSED GameSceneInfo* info)
{
    int pad_queue_count;
    int i;
    struct gm_80479D58_t* temp_r25;

    PAD_STACK(28);

    temp_r25 = &gm_80479D58;
    temp_r25->unk_10.x0 = 0;
    temp_r25->unk_10.x1 = 0;
    temp_r25->unk_10.x2 = 0;
    temp_r25->unk_10.x3 = 0;
    temp_r25->unk_10.unk_20 = temp_r25->unk_10.unk_28 = 0;
    gm_80479D58.unk_0 = 0;
    gm_80479D58.unk_4 = 0;
    gm_80479D58.unk_8 = 0;
    gm_80479D58.unk_C = 0;
    HSD_PadFlushQueue(HSD_PAD_FLUSH_QUEUE_LEAVE1);
    if (loop_snapshot.epoch == UINT64_MAX) abort();
    ++loop_snapshot.epoch;
    loop_snapshot.object_epoch = nativeGObjPool.lifetime;
    loop_snapshot.scene = gm_804D6720;
    loop_snapshot.descriptor = *info;
    loop_snapshot.pause_handlers = temp_r25->unk_10.db_input;
    loop_snapshot.pre_proc = temp_r25->unk_10.pre_gobj_proc;
    loop_snapshot.debug_callback = gm_804D6724;
    loop_snapshot.config = gm_80479D48.initdata;
    loop_snapshot.active = true;
    nativePlayerSnapshot_BeginScene();
    nativeCameraSubjects_BeginScene();
    nativeCamera_BeginScene();
    nativeEffectControl_BeginScene();
    nativeParticleControl_BeginScene();
    nativeVersusControl_BeginScene();
    if(info->scene_kind==GS_VS || info->scene_kind==GS_SUDDEN_DEATH || info->scene_kind==GS_TRAINING) {
        nativeStage_BeginScene();
        nativeStageHazards_BeginScene();
        nativeStageGenerator_BeginScene();
    }
    nativeMatchClasses_Initialize();
    nativeSnapshotCensus_AdmitArenas(info->scene_kind);
#ifndef MELEE_NATIVE
    lb_8001CF18();
#endif

    bool rollback_census = nativeMatchLaunch_RollbackCensus() &&
                           info->scene_kind == GS_VS &&
                           nativeRollbackCensus_Init();
    bool rollback_probe =
        nativeMatchLaunch_RollbackProbe() && info->scene_kind == GS_VS &&
        nativeRollbackProbe_Init(nativeMatchLaunch_RollbackCorrectionProbe(),
                                 nativeMatchLaunch_RollbackCorrectionFrame());
    bool rollback_benchmark = nativeMatchLaunch_RollbackBenchmark() &&
        info->scene_kind == GS_VS && nativeRollbackBenchmark_Init();
    bool netplay_requested = nativeMatchLaunch_Netplay() != NULL;
    bool netplay = netplay_requested &&
                   (info->scene_kind == GS_VS ||
                    info->scene_kind == GS_SUDDEN_DEATH) &&
                   nativeNetplayMatch_Init();
    bool rollback_active = rollback_probe || rollback_benchmark || netplay;
    const char* no_gpu = getenv("FREAK_MELEE_NO_GPU_RESIM");
    // Netplay reconstructs confirmed/predicted gameplay without GPU output.
    // Raw-memory diagnostic probes keep their rendered oracle unless opted in.
    bool no_gpu_resim = no_gpu ? !strcmp(no_gpu, "1") : netplay;
    const char* skip_models = getenv("FREAK_MELEE_SKIP_MODEL_RESIM");
    bool skip_model_resim = no_gpu_resim || (skip_models && !strcmp(skip_models, "1"));
    uint64_t cpu_render_frames = 0;
    nativeRenderModelPrimitivesSkipped = 0;
    nativeRenderParticleBatchesSkipped = 0;
    if (skip_model_resim) OSReport("[native-render] Resimulation omits model primitives; state callbacks and matrix setup retained.\n");
    uint64_t rollback_frame = 0;
    bool compact_journal=nativeSnapshotCensus_UsesReplayJournal();
    bool compact_before_input=nativeSnapshotCensus_BeforeInput();
    bool compact_timing=nativeSnapshotCensus_TimingEnabled();
    bool netplay_timing=netplay && nativeNetplayTimingEnabled;
    if(compact_journal)HSD_AudioRollbackReset();
    if (nativeMatchLaunch_RollbackProbe() && !rollback_probe) {
        gmMain_ExitRequested = true;
    }
    if (nativeMatchLaunch_RollbackBenchmark() && !rollback_benchmark) {
        gmMain_ExitRequested = true;
    }
    if (netplay_requested && !netplay) {
        gm_Scene_Vs_Abort(OUTCOME_NO_CONTEST);
    }

    while (temp_r25->unk_C == 0 && !gmMain_ExitRequested) {
        if(compact_before_input) {
            nativeSnapshotCensus_Sample(info->scene_kind,temp_r25->unk_0,
                                       nativeReplay_SampleCount());
            if(nativeSnapshotCensus_HasFailed()) {gmMain_ExitRequested=true;break;}
        }
        if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_Netplay);
        bool began_rollback_frame =
            rollback_probe
                ? nativeRollbackProbe_BeginFrame(rollback_frame)
                : rollback_benchmark ? nativeRollbackBenchmark_BeginFrame(rollback_frame)
                : !netplay || nativeNetplayMatch_BeginFrame(&rollback_frame);
        if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_Netplay);
        if (!began_rollback_frame) {
            if (netplay) {
                gm_Scene_Vs_Abort(OUTCOME_NO_CONTEST);
            } else {
                gmMain_ExitRequested = true;
            }
            break;
        }
        if(!netplay && nativeMatchLaunch_SnapshotCensus()) {
            const char* trace=getenv("FREAK_MELEE_REPLAY_STATE_TRACE");
            nativeRandom_TraceFrame(nativeReplay_SampleCount(),trace && !strcmp(trace,"1"),
                nativeSnapshotCensus_IsResimulating());
        }
        if(compact_journal && !HSD_AudioRollbackBeginFrame(nativeReplay_SampleCount(),nativeSnapshotCensus_IsResimulating())) {
            nativeSnapshotCensus_Abort("cannot begin compact replay audio-intent frame");
            gmMain_ExitRequested=true;break;
        }
        ++loop_snapshot.activity;
        if (netplay) nativeCallbackProfile_SetFrame(rollback_frame,
                                        nativeNetplayMatch_IsResimulating());
        gmMainLib_8046B0F0.xC = false;

        if (rollback_benchmark) nativeRollbackBenchmark_BeginInput();
        if(compact_timing)nativeSnapshotCensus_BeginTiming(NativeCompactTiming_Input);
        if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_Input);
        do {
            dbNativeControl_Poll();
            const AuroraEvent* event = aurora_update();
            for (; event && event->type != AURORA_NONE; ++event) {
                if (event->type == AURORA_SDL_EVENT) nativeHub_Event(&event->sdl);
                if (event->type == AURORA_EXIT) {
                    gmMain_ExitRequested = true;
                } else if (event->type == AURORA_SDL_EVENT &&
                           event->sdl.type == SDL_EVENT_KEY_DOWN &&
                           event->sdl.key.scancode == SDL_SCANCODE_F12 &&
                           !event->sdl.key.repeat)
                {
                    db_RequestScreenshot();
                }
            }
            if (gmMain_ExitRequested) {
                break;
            }
            pad_queue_count = lbInput_Poll(nativeMatchLaunch_Fast()
                                               ? lbInput_VirtualClockNowNs()
                                               : netplay && nativeNetplayMatch_IsResimulating()
                                                     ? lbInput_NextSampleTimeNs()
                                                     : SDL_GetTicksNS());
            if (lbInput_DidSample()) {
                dbNativeControl_OnInputSample();
            }
            if (rollback_benchmark) nativeRollbackBenchmark_InputPoll(pad_queue_count == 0);
            lb_800195D0();
            if (pad_queue_count == 0) {
                if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_InputWait);
                SDL_Delay(1);
                if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_InputWait);
            }
        } while (pad_queue_count == 0);
        if (rollback_benchmark) nativeRollbackBenchmark_EndInput();
        if(compact_timing)nativeSnapshotCensus_EndTiming(NativeCompactTiming_Input);
        if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_Input);
        if (gmMain_ExitRequested) {
            --loop_snapshot.activity;
            break;
        }
        if (rollback_active && pad_queue_count > 1) {
            pad_queue_count = 1;
        }

        for (i = 0; i < pad_queue_count; i++) {
            if (rollback_census) {
                nativeRollbackCensus_BeginSimulation();
            }
            if (rollback_benchmark) nativeRollbackBenchmark_BeginSimulation();
            if(compact_timing)nativeSnapshotCensus_BeginTiming(NativeCompactTiming_Simulation);
            if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_Simulation);
            HSD_PerfSetStartTime();
            lb_800198E0();
            nativeHome_Tick();
            if (DbLevel >= DbLKind_DebugRom) {
                gm_801A4970(&temp_r25->unk_10.db_input);
            }
            if (gm_801A46B8(0) || !gm_GetDbPauseFlag(0)) {
                temp_r25->unk_10.unk_38_0 = true;
            } else {
                temp_r25->unk_10.unk_38_0 = false;
            }
            if (gm_80479D58.unk_10.unk_38_0) {
                lb_80019900();
                if (lb_80019A30(0)) {
                    nativeHome_ApplyResultsInput();
                    if (!rollback_active) nativeHub_Tick();
                    if (nativeHome_IsSelection() &&
                        (nativeHub_IsVisible() || nativeShellProfiles_IsOpen() ||
                         nativeShellRules_IsOpen() || nativeShell_IsEditingText()))
                    {
                        // Raw controller sampling remains available to rebinding;
                        // only the CSS-facing inputs are neutral while editing.
                        for (int port = 0; port < 4; ++port) {
                            s8 error = HSD_PadCopyStatus[port].err;
                            memset(&HSD_PadCopyStatus[port], 0, sizeof(HSD_PadStatus));
                            HSD_PadCopyStatus[port].err = error;
                        }
                    }
                    gm_EvaluateAllControllerInputs();
                }
                if (lb_80019A30(0) && on_frame != NULL) {
                    on_frame();
                }
            }
            if (gm_80479D58.unk_10.x0 != gm_80479D58.unk_10.x1 ||
                temp_r25->unk_10.x2 != temp_r25->unk_10.x3)
            {
                temp_r25->unk_10.unk_20 =
                    maybe_gm_801A48A4(temp_r25->unk_10.x0);
                temp_r25->unk_10.x1 = temp_r25->unk_10.x0;
                temp_r25->unk_10.x3 = temp_r25->unk_10.x2;
                temp_r25->unk_10.x2 = 0;
            }
            temp_r25->unk_10.unk_28 = temp_r25->unk_10.unk_20;
            if (!lb_80019A30(0)) {
                temp_r25->unk_10.unk_28 |=
                    gm_803DA8C8[temp_r25->unk_10.unk_34];
            }
            if (!lb_80019A30(1)) {
                temp_r25->unk_10.unk_28 |=
                    ~gm_803DA8C8[temp_r25->unk_10.unk_34];
            }
            if (DbLevel >= DbLKind_DebugRom) {
                db_CheckScreenshot();
            }
            lbAudioAx_80027DF8();
            if (temp_r25->unk_10.pre_gobj_proc != NULL) {
                temp_r25->unk_10.pre_gobj_proc();
            }
            HSD_GObj_80390CFC();
            if (temp_r25->unk_0 != -2) {
                temp_r25->unk_0++;
            }
            if (gm_80479D58.unk_10.unk_38_0 && lb_80019A30(0)) {
                if (temp_r25->unk_8 != -2) {
                    temp_r25->unk_8++;
                }
            }
            HSD_PerfSetCPUTime();
            gmMainLib_8046B0F0.xC = false;
            if (rollback_census) {
                nativeRollbackCensus_EndSimulation();
            }
            if (rollback_benchmark) nativeRollbackBenchmark_EndSimulation();
            if(compact_timing)nativeSnapshotCensus_EndTiming(NativeCompactTiming_Simulation);
            if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_Simulation);
            if (temp_r25->unk_C != 0) {
                break;
            }
        }
        if (temp_r25->unk_C == 2) {
            --loop_snapshot.activity;
            if (netplay) {
                uint64_t next_frame;
                if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_Netplay);
                NativeConfirmedCheckpointStep step=nativeNetplayMatch_EndFrame(
                    rollback_frame,&next_frame,true);
                if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_Netplay);
                rollback_frame=next_frame;
                if (step==NativeConfirmedCheckpoint_Failed)gm_Scene_Vs_Abort(OUTCOME_NO_CONTEST);
                else if (nativeNetplayMatch_IsResimulating())continue;
            }
            break;
        }

        // Accelerated gameplay can advance several times per display frame.
        // Keep polling between those updates instead of blocking on vsync.
        if (!rollback_active && temp_r25->unk_10.unk_38_0 && !lb_80019A30(1)) {
            --loop_snapshot.activity;
            continue;
        }

        lb_800195D0();
        bool hidden_rollback_frame =
            (rollback_probe && nativeRollbackProbe_IsResimulating()) ||
            (rollback_benchmark && nativeRollbackBenchmark_IsResimulating()) ||
            (netplay && nativeNetplayMatch_IsResimulating()) ||
            nativeSnapshotCensus_IsResimulating();
        if (rollback_benchmark) nativeRollbackBenchmark_BeginRender();
        if(compact_timing)nativeSnapshotCensus_BeginTiming(NativeCompactTiming_Drawing);
        if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_Drawing);
        if (!hidden_rollback_frame) {
            nativeShell_Update();
        }
        if(compact_timing)nativeSnapshotCensus_BeginTiming(NativeCompactTiming_RenderBegin);
        if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_RenderBegin);
        bool cpu_render_frame = no_gpu_resim && hidden_rollback_frame;
        bool began_render = cpu_render_frame ? aurora_begin_frame_without_output() : hidden_rollback_frame
                                ? aurora_begin_frame_without_present()
                                : aurora_begin_frame();
        if(compact_timing)nativeSnapshotCensus_EndTiming(NativeCompactTiming_RenderBegin);
        if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_RenderBegin);
        if (!began_render) {
            if (rollback_probe) {
                nativeRollbackProbe_Abort("Aurora rejected a frame");
                gmMain_ExitRequested = true;
            } else if (rollback_benchmark) {
                nativeRollbackBenchmark_Abort("Aurora rejected a frame");
                gmMain_ExitRequested = true;
            } else if (netplay) {
                nativeNetplayMatch_Abort("Aurora rejected a frame");
                gm_Scene_Vs_Abort(OUTCOME_NO_CONTEST);
            }
            --loop_snapshot.activity;
            continue;
        }
        GXInvalidateVtxCache();
        GXInvalidateTexAll();
        if (rollback_census) {
            nativeRollbackCensus_BeginRender();
        }
        if(compact_timing)nativeSnapshotCensus_BeginTiming(NativeCompactTiming_HsdDraw);
        if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_HsdDraw);
        HSD_StartRender(HSD_RP_SCREEN);
        nativeRenderGeometryEnabled = !(skip_model_resim && hidden_rollback_frame);
        nativeRenderGraphicsEnabled = !cpu_render_frame;
        HSD_GObj_80390FC0();
        nativeRenderGeometryEnabled=true;
        nativeRenderGraphicsEnabled=true;
        // These descriptors cache submitted geometry only. A model-free pass
        // must not leave a preceding frame's vertex bindings as its cache.
        if (skip_model_resim) HSD_ClearVtxDesc();
        HSD_Init_803755A8();
        if (!hidden_rollback_frame) nativeHub_Draw();
        if(compact_timing)nativeSnapshotCensus_EndTiming(NativeCompactTiming_HsdDraw);
        if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_HsdDraw);
        if (rollback_census) {
            nativeRollbackCensus_EndRender();
        }
        HSD_PerfSetDrawTime();
        {
            HSD_VIStatus* video = HSD_VIGetVIStatus();
            GXSetCopyFilter(video->rmode.aa, video->rmode.sample_pattern,
                            video->vf, video->rmode.vfilter);
            GXSetDispCopyGamma(video->gamma);
            HSD_StateSetColorUpdate(video->update_clr);
            HSD_StateSetAlphaUpdate(video->update_alpha);
            HSD_StateSetZMode(video->update_z, GX_LEQUAL, GX_TRUE);
            GXSetCopyClear(video->clear_clr, video->clear_z);
        }
        if(compact_timing)nativeSnapshotCensus_BeginTiming(NativeCompactTiming_RenderEnd);
        if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_RenderEnd);
        if (cpu_render_frame) {
            aurora_end_frame_without_output();
            ++cpu_render_frames;
        } else if (hidden_rollback_frame) {
            aurora_end_frame_without_present();
        } else {
            db_TakeScreenshotIfPending();
            aurora_end_frame();
        }
        if(compact_timing)nativeSnapshotCensus_EndTiming(NativeCompactTiming_RenderEnd);
        if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_RenderEnd);
        if (rollback_benchmark) nativeRollbackBenchmark_EndRender();
        if(compact_timing)nativeSnapshotCensus_EndTiming(NativeCompactTiming_Drawing);
        if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_Drawing);
        if(netplay_timing)nativeNetplayTiming_Submitted(rollback_frame,hidden_rollback_frame);
        if (temp_r25->unk_4 != -2U) {
            temp_r25->unk_4++;
        }
        HSD_PerfSetTotalTime();
        HSD_PerfInitStat();
        if(compact_journal && !HSD_AudioRollbackEndFrame()) {
            nativeSnapshotCensus_Abort("cannot end compact replay audio-intent frame");gmMain_ExitRequested=true;
        }
        --loop_snapshot.activity;
        if(nativeMatchLaunch_SnapshotCensus()) {
            for(uint32_t slot=0;slot<nativeGObjPool.initialized_count;++slot)
                if(nativeGObjPool.slots[slot].occupied)
                    nativeRandom_TraceBytes("gobj",nativeGObjPool.payload+slot*nativeGObjPool.stride,nativeGObjPool.object_size);
        }
        if (nativeMatchLaunch_SnapshotCensus() && !compact_before_input)
            nativeSnapshotCensus_Sample(info->scene_kind, temp_r25->unk_0,
                                       nativeReplay_SampleCount());
        if (nativeMatchLaunch_ObjectCensus())
            nativeMatchObjectCensus_Sample(rollback_active ? rollback_frame : temp_r25->unk_0,
                nativeReplay_SampleCount(), hidden_rollback_frame);
        if (rollback_probe) {
            uint64_t next_frame;
            NativeRollbackProbeStep step =
                nativeRollbackProbe_EndFrame(rollback_frame, &next_frame);
            rollback_frame = next_frame;
            if (step == NativeRollbackProbe_Succeeded ||
                step == NativeRollbackProbe_Failed)
            {
                gmMain_ExitRequested = true;
            }
        } else if (rollback_benchmark) {
            uint64_t next_frame;
            bool continuing = nativeRollbackBenchmark_EndFrame(rollback_frame, &next_frame);
            rollback_frame = next_frame;
            if (!continuing) gmMain_ExitRequested = true;
        } else if (netplay) {
            uint64_t next_frame;
            if(netplay_timing)nativeNetplayTiming_Begin(NativeNetplayTiming_Netplay);
            NativeConfirmedCheckpointStep step =
                nativeNetplayMatch_EndFrame(rollback_frame, &next_frame,temp_r25->unk_C!=0);
            if(netplay_timing)nativeNetplayTiming_End(NativeNetplayTiming_Netplay);
            rollback_frame = next_frame;
            if (step == NativeConfirmedCheckpoint_Failed) {
                gm_Scene_Vs_Abort(OUTCOME_NO_CONTEST);
            }
        }
    }
    if(compact_journal)HSD_AudioRollbackAbortFrame();
    if (no_gpu_resim) OSReport("[native-render] CPU-only GX frames: %llu.\n",
        (unsigned long long)cpu_render_frames);
    if (skip_model_resim) OSReport("[native-render] Skipped model primitives: %llu.\n",
        (unsigned long long)nativeRenderModelPrimitivesSkipped);
    if (no_gpu_resim) OSReport("[native-render] Skipped particle batches: %llu.\n",
        (unsigned long long)nativeRenderParticleBatchesSkipped);
    if(!netplay)nativeRandom_TraceFrame(0,false,false);
    nativePlayerSnapshot_EndScene();
    nativeCameraSubjects_EndScene();
    nativeCamera_EndScene();
    nativeStage_EndScene();
    nativeStageHazards_EndScene();
    nativeStageGenerator_EndScene();
    nativeDynamics_EndScene();
    nativeEffectControl_EndScene();
    nativeVersusControl_EndScene();
    nativeParticleControl_EndScene();
    loop_snapshot.active = false;
    if (rollback_probe) {
        nativeRollbackProbe_Shutdown();
    }
    if (rollback_benchmark) nativeRollbackBenchmark_Shutdown();
    if (rollback_census) {
        nativeRollbackCensus_Shutdown();
    }
    GXDrawDone();
}

static bool loop_config_matches(const HSD_GObjLibInitDataType* config)
{
    const HSD_GObjLibInitDataType* fixed = &loop_snapshot.config;
    return config->p_link_max == fixed->p_link_max &&
        config->gx_link_max == fixed->gx_link_max &&
        config->gproc_pri_max == fixed->gproc_pri_max &&
        config->funcs == fixed->funcs && config->unk_2 == fixed->unk_2;
}

bool nativeGmLoop_SnapshotBoundary(void)
{
    return loop_snapshot.active && !loop_snapshot.activity &&
        loop_snapshot.epoch && loop_snapshot.object_epoch &&
        nativeGObjPool.lifetime == loop_snapshot.object_epoch &&
        gm_804D6720 == loop_snapshot.scene && gm_804D6720 &&
        gm_804D6720->scene_kind == loop_snapshot.descriptor.scene_kind &&
        gm_804D6720->enter_data == loop_snapshot.descriptor.enter_data &&
        gm_804D6720->exit_data == loop_snapshot.descriptor.exit_data &&
        !HSD_GObj_CurrentInvokedProcGObj && !HSD_GObj_CurrentInvokedProc &&
        !HSD_GObj_804D7818 && !HSD_GObj_RenderCallbackDepth &&
        !HSD_GObj_DelayedProcInfo.flags;
}

static bool loop_overlap(const void* a, size_t an, const void* b, size_t bn)
{
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return an > UINTPTR_MAX-x || bn > UINTPTR_MAX-y ||
        (x < y+bn && y < x+an);
}

static bool loop_buffer(const void* data, size_t bytes)
{
    if (!data || !bytes || bytes > UINTPTR_MAX-(uintptr_t)data) return false;
#define EXCLUDE(root) if (loop_overlap(data, bytes, &(root), sizeof(root))) return false
    EXCLUDE(loop_snapshot); EXCLUDE(gm_80479D48); EXCLUDE(gm_80479D58);
    EXCLUDE(gm_804D6720); EXCLUDE(gm_804D6724); EXCLUDE(gm_804D6728);
    EXCLUDE(gm_804D672C); EXCLUDE(gm_803DA8C8);
#undef EXCLUDE
    return true;
}

static bool loop_reference(const HSD_GObj* object, NativeMatchPoolReference ref)
{
    if (!object) return !ref.pool && !ref.lifetime && !ref.slot && !ref.generation;
    return ref.pool == &nativeGObjPool && ref.lifetime == loop_snapshot.object_epoch &&
        ref.slot < nativeGObjPool.capacity && ref.generation &&
        nativeGObjPool.payload + ref.slot*nativeGObjPool.stride == (const uint8_t*)object;
}

bool nativeGmLoop_ValidateSnapshot(const void* data, size_t bytes)
{
    if (!nativeGmLoop_SnapshotBoundary() || bytes != sizeof(NativeGmLoopSnapshot) ||
        !loop_buffer(data, bytes)) return false;
    NativeGmLoopSnapshot state;
    memcpy(&state, data, sizeof(state));
    return state.epoch == loop_snapshot.epoch &&
        state.object_epoch == loop_snapshot.object_epoch &&
        state.scene == loop_snapshot.scene && loop_config_matches(&state.config) &&
        state.loop.unk_C >= 0 && state.loop.unk_C <= 2 &&
        state.loop.unk_10.unk_34 >= 0 && state.loop.unk_10.unk_34 <= 1 &&
        state.loop.unk_10.db_input.check_pause == loop_snapshot.pause_handlers.check_pause &&
        state.loop.unk_10.db_input.check_framestep == loop_snapshot.pause_handlers.check_framestep &&
        state.loop.unk_10.pre_gobj_proc == loop_snapshot.pre_proc &&
        state.debug_callback == loop_snapshot.debug_callback &&
        loop_reference(state.scheduler_object, state.scheduler_reference) &&
        loop_reference(state.debug_display, state.debug_reference);
}

bool nativeGmLoop_Save(void* data, size_t capacity, size_t* written)
{
    size_t bytes = sizeof(NativeGmLoopSnapshot);
    if (!nativeGmLoop_SnapshotBoundary() || capacity < bytes || !loop_buffer(data, bytes) ||
        !loop_buffer(written, sizeof(*written)) ||
        loop_overlap(data, bytes, written, sizeof(*written))) return false;
    NativeGmLoopSnapshot state = {0};
    state.epoch = loop_snapshot.epoch;
    state.object_epoch = loop_snapshot.object_epoch;
    state.scene = gm_804D6720;
    memcpy(&state.loop, &gm_80479D58, sizeof(state.loop));
    memcpy(&state.config, &gm_80479D48.initdata, sizeof(state.config));
    state.config_pad = gm_80479D48.pad;
    state.scheduler_object = gm_804D672C;
    state.debug_display = gm_804D6728;
    state.debug_callback = gm_804D6724;
    memcpy(state.cadence_masks, gm_803DA8C8, sizeof(state.cadence_masks));
    if ((state.scheduler_object && !nativeMatchPool_Reference(&nativeGObjPool,
            state.scheduler_object, &state.scheduler_reference)) ||
        (state.debug_display && !nativeMatchPool_Reference(&nativeGObjPool,
            state.debug_display, &state.debug_reference)) ||
        !nativeGmLoop_ValidateSnapshot(&state, bytes)) return false;
    memcpy(data, &state, bytes);
    *written = bytes;
    return true;
}

bool nativeGmLoop_Load(const void* data, size_t bytes)
{
    if (!nativeGmLoop_ValidateSnapshot(data, bytes)) return false;
    NativeGmLoopSnapshot state;
    memcpy(&state, data, sizeof(state));
    if ((state.scheduler_object && nativeMatchPool_Resolve(state.scheduler_reference) != state.scheduler_object) ||
        (state.debug_display && nativeMatchPool_Resolve(state.debug_reference) != state.debug_display)) return false;
    memcpy(&gm_80479D58, &state.loop, sizeof(state.loop));
    memcpy(&gm_80479D48.initdata, &state.config, sizeof(state.config));
    gm_80479D48.pad = state.config_pad;
    gm_804D672C = state.scheduler_object;
    gm_804D6728 = state.debug_display;
    gm_804D6724 = state.debug_callback;
    gm_804D6720 = state.scene;
    memcpy(gm_803DA8C8, state.cadence_masks, sizeof(gm_803DA8C8));
    return true;
}

#ifdef MELEE_NATIVE
bool nativeGmLoop_AdmitHistory(NativeMatchArena* arena)
{
    if(!arena || !arena->lifetime)return false;
#define ROOT(v) do { if(!nativeMatchArena_AddRoot(arena,&(v),sizeof(v))){fprintf(stderr,"[snapshot-census] control-root refused: %s\n",#v);return false;} } while(0)
    ROOT(gm_804D6720);
    // The borrowed GObj process mask is already a captured subspan of this
    // loop record. Admit the prefix/suffix without capturing that word twice.
    if(gm_80479D48.initdata.unk_2==&gm_80479D58.unk_10.unk_28){
        size_t before=offsetof(struct gm_80479D58_t,unk_10.unk_28);
        size_t after=before+sizeof(gm_80479D58.unk_10.unk_28);
        if(!nativeMatchArena_AddRoot(arena,&gm_80479D58,before))return false;
        if(after<sizeof(gm_80479D58) && !nativeMatchArena_AddRoot(arena,
            (uint8_t*)&gm_80479D58+after,sizeof(gm_80479D58)-after))return false;
    }else ROOT(gm_80479D58);
    ROOT(gm_80479D48.initdata);
    ROOT(gm_80479D48.pad);
    ROOT(gm_804D672C);
    ROOT(gm_804D6728);
    ROOT(gm_804D6724);
    ROOT(gm_803DA8C8);
#undef ROOT
    return true;
}
#endif
