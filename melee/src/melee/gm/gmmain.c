#include <aurora/aurora.h>
#include <aurora/main.h>
#include <aurora/dvd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysdolphin/baselib/audio_output.h>
#include <string.h>

#include <Runtime/platform.h>

#include <sysdolphin/baselib/forward.h>

#include "gm_unsplit.h"
#include "gmmain.h"
#include "gmmain_lib.h"
#include "native_match_launch.h"
#include <melee/ft/native_script_fighter_launch.h>
#include <melee/ft/native_script_fighter.h>
#include <sysdolphin/baselib/audio_output.h>
#include "native_online_session.h"
#include "native_replay.h"
#include "native_rollback_probe.h"
#include "native_rollback_benchmark.h"
#include "native_match_object_census.h"
#include "native_snapshot_census.h"
#include "native_settings.h"
#include "types.h"
#include <melee/ui/native_shell.h>
#ifndef MELEE_NATIVE
#include <dolphin/card.h>
#endif
#include <dolphin/dvd.h>
#include <dolphin/gx.h>
#include <dolphin/gx/GXAurora.h>
#include <dolphin/os.h>
#include <dolphin/pad.h>
#include <dolphin/vi.h>
#include <melee/db/db.h>
#include <melee/db/dbnativecontrol.h>
#include <melee/lb/lb_0195.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/lbcardgame.h>
#include <melee/lb/lbcardnew.h>
#include <melee/lb/lbdvd.h>
#include <melee/lb/lbheap.h>
#include <melee/lb/lblanguage.h>
#include <melee/lb/lbmemory.h>
#include <melee/lb/lbmthp.h>
#include <melee/lb/lbsnap.h>
#include <melee/lb/lbtime.h>
#include <sysdolphin/baselib/controller.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/initialize.h>
#include <sysdolphin/baselib/random.h>
#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/video.h>

#include "gmmain_state.h"

bool gmMain_ExitRequested;

static u32 arena_size;
static bool gmMain_804D6594;

static void gmMain_8015FD24(void)
{
    PADSetSpec(5);
    HSD_PadInit(5, gmMain_8046B108, 12, gmMain_8046B1F8);
    HSD_PadLibData.clamp_stickType = 0;
    HSD_PadLibData.clamp_stickShift = 1;
    HSD_PadLibData.clamp_stickMax = 80;
    HSD_PadLibData.clamp_stickMin = 0;
    HSD_PadLibData.scale_stick = 80;
    HSD_PadLibData.clamp_analogLRShift = 1;
    HSD_PadLibData.clamp_analogLRMax = 140;
    HSD_PadLibData.clamp_analogLRMin = 0;
    HSD_PadLibData.scale_analogLR = 140;
}

/// set debug level
static void gmMain_8015FDA4(void)
{
    if (DVDConvertPathToEntrynum("/develop.ini") != -1) {
        db_804D6B20 = true;
        if (db_gameLaunchButtonState & HSD_PAD_X) {
            enum_t level = DbLevel;
            switch (level) {
            case DbLKind_NoDebugRom:
                level = DbLKind_DebugRom;
                break;
            case DbLKind_DebugDevelop:
                level = DbLKind_Develop;
                break;
            case DbLKind_DebugRom:
                level = DbLKind_NoDebugRom;
                break;
            case DbLKind_Develop:
                level = DbLKind_DebugDevelop;
                break;
            default:
                level = DbLKind_DebugDevelop;
                break;
            }
            DbLevel = level;
        } else if (db_gameLaunchButtonState & HSD_PAD_Y) {
            enum_t level = DbLevel;
            switch (level) {
            case DbLKind_NoDebugRom:
                level = DbLKind_DebugRom;
                break;
            case DbLKind_DebugDevelop:
                level = DbLKind_Develop;
                break;
            default:
                break;
            }
            DbLevel = level;
            db_804D6B20 = false;
        }
    } else {
        HSD_ASSERT(210, DbLevel == DbLKind_NoDebugRom);
        DbLevel = DbLKind_Master;
    }
}

int main(int argc, char** argv)
{
    const char* iso_path;
    int parsed = nativeMatchLaunch_Parse(argc, argv, &iso_path);
    if (parsed <= 0) {
        return parsed == 0 ? 0 : 1;
    }
    if (nativeMatchLaunch_ScriptSource() &&
        !nativeScriptFighterLaunch_Load(nativeMatchLaunch_ScriptKind(),nativeMatchLaunch_ScriptSource())) {
        fprintf(stderr,"Could not load interpreted fighter source.\n");return 1;
    }
    HSD_AudioOutputSetMuted(nativeMatchLaunch_Muted());
    if (!aurora_dvd_open(iso_path)) {
        fprintf(stderr, "Could not open game disc: %s\n", iso_path);
        return 1;
    }
    const DVDDiskID* disc = DVDGetCurrentDiskID();
    if (memcmp(disc->gameName, "GALE", 4) != 0 ||
        memcmp(disc->company, "01", 2) != 0 || disc->gameVersion != 2) {
        fprintf(stderr, "The client requires the USA v1.02 Melee disc.\n");
        aurora_dvd_close();
        return 1;
    }
    const char* render_cache = getenv("FREAK_MELEE_RENDER_CACHE");
    AuroraConfig config = { .appName = "Freak Melee",
        .cachePath = render_cache && *render_cache ? render_cache : NULL,
        .windowWidth = 1440, .windowHeight = 810,
        .vsync = !nativeMatchLaunch_Fast(),
        .hideWindow = nativeMatchLaunch_Hidden(),
        .logLevel = LOG_INFO,
        .mem1Size = 128 * 1024 * 1024,
        .mem2Size = 16 * 1024 * 1024 };
    aurora_initialize(argc, argv, &config);
    if (!nativeShell_Init()) {
        aurora_dvd_close();
        aurora_shutdown();
        return 1;
    }

    OSInit();
    VIInit();
#ifdef TARGET_PC
    VICenterWindow();
#endif
    DVDInit();
    PADInit();
    if (!nativeMatchLaunch_ApplyProfiles()) {
        aurora_dvd_close();
        nativeShell_Shutdown();
        aurora_shutdown();
        return 1;
    }
#ifndef MELEE_NATIVE
    CARDInit();
#endif
    db_GetGameLaunchButtonState();
    if (gmMain_ExitRequested) {
        bool replay_ok = nativeReplay_Close();
        aurora_dvd_close();
        nativeShell_Shutdown();
        aurora_shutdown();
        return replay_ok ? 0 : 1;
    }
    gmMain_8015FDA4();
    arena_size = (intptr_t) OSGetArenaHi() - (intptr_t) OSGetArenaLo();
    HSD_SetInitParameter(HSD_INIT_RENDER_MODE_OBJ, &GXNtsc480IntDf);
    HSD_SetInitParameter(HSD_INIT_HEAP_MAX_NUM, 4);
    db_SetupCrashHandler();
    GXInit(NULL, 0);
    AuroraSetViewportPolicy(AURORA_VIEWPORT_FIT);
    HSD_InitComponent();
    // The boot-button PADRead is not part of the canonical HSD input stream.
    // Install playback only after it, so sample zero belongs to the match.
    NativeReplayMode replay_mode = nativeMatchLaunch_ReplayInputsOnly()
        ? NativeReplay_InputsOnly
        : nativeMatchLaunch_ReplayPlayback()
            ? NativeReplay_Verify : NativeReplay_Record;
    if (!nativeReplay_Open(nativeMatchLaunch_ReplayPath(), replay_mode,
                           nativeMatchLaunch_Signature())) {
        aurora_dvd_close();
        nativeShell_Shutdown();
        aurora_shutdown();
        return 1;
    }
    nativeReplay_SetActionTraceEnabled(nativeMatchLaunch_TraceActions());
    nativeReplay_SetFighterTraceEnabled(
        nativeMatchLaunch_TraceFighterState());
    *seed_ptr = nativeReplay_Seed(OSGetTick());
    lbAudioAx_8002838C();
    lb_80019AAC(&gmMain_8015FD24);
    HSD_VISetBlack(0);
    lbMemory_8001564C();
    lbHeap_80015F3C();
    lbDvd_80018F68();
#ifndef MELEE_NATIVE
    lb_8001C5BC();
    lb_8001D21C();
#endif
#ifndef MELEE_NATIVE
    lbSnap_8001E290();
#endif
    gmMainLib_8015FCC0();
    lbMthp_8001F87C();
    HSD_SisLib_803A6048(0xC000);
    gmMainLib_8015FBA4();
    if (!nativeMatchLaunch_IsDirect()) {
        if (nativeSettings_Load(gmMainLib_GetGameRules(),
                                gmMainLib_GetGamePrefs()))
        {
            GameRules* rules = gmMainLib_GetGameRules();
            OSReport("[settings] Loaded versus mode %u, time %u, stocks %u.\n",
                     rules->mode, rules->time_limit, rules->stock_count);
        }
    }

    db_InitScreenshot();
    dbNativeControl_Init();
    OSReport("# ---------------------------------------------\n");
    OSReport("#    Super Smash Bros. Melee\n");
    OSReport("#\n");
    OSReport("# Distribution %d\n", lbLang_GetLanguageSetting());
    OSReport("# Language %d\n", lbLang_GetSavedLanguage());
    OSReport("# DbLevel %d\n", DbLevel);
    OSReport("# Arena Size %d MB\n", arena_size / (1024 * 1024));
    {
        uintptr_t free_aram_start;
        uintptr_t free_aram_end;
        lbMemory_800154BC(&free_aram_start, &free_aram_end);
        OSReport("# ARAM Free Size %d MB\n",
                 (free_aram_end - free_aram_start) / (1024 * 1024));
    }
    OSReport("# %s\n", db_build_timestamp);
    {
        struct datetime dt;
        gm_801692E8(lbTime_GetTimeInSeconds(), &dt);
        OSReport("# GC Calendar Year %d Month %d Day %d\n", dt.year, dt.month,
                 dt.day);
        OSReport("#             Hour %d Min %d Sec %d \n", dt.hour, dt.minute,
                 dt.second);
    }
    OSReport("#\n\n");
    gmMain_804D6594 = false;
    if (gmMain_804D6594) {
        db_DisableItemSpawns();
    } else {
        db_EnableItemSpawns();
    }

    db_ClearFPUExceptions();
    bool snapshot_ready = !nativeMatchLaunch_SnapshotCensus() ||
                         nativeSnapshotCensus_Init();
    if (snapshot_ready) gm_801A4510();
    if (nativeMatchLaunch_SnapshotCensus()) nativeSnapshotCensus_Close();
    if (nativeMatchLaunch_ObjectCensus()) nativeMatchObjectCensus_Close();
    if (nativeMatchLaunch_ScriptSource() ||
        nativeMatchLaunch_TraceFighterState()) nativeScriptFighter_Report();
    bool replay_ok = nativeReplay_Close();
    nativeOnlineSession_Stop();
    dbNativeControl_Shutdown();
    HSD_AudioOutputShutdown();
    aurora_dvd_close();
    nativeShell_Shutdown();
    aurora_shutdown();
    return replay_ok && !nativeRollbackProbe_HasFailed() &&
                   !nativeRollbackBenchmark_HasFailed() &&
                   !nativeMatchObjectCensus_HasFailed() &&
                   !nativeSnapshotCensus_HasFailed() &&
                   !nativeMatchLaunch_DirectPreparationFailed()
               ? 0 : 1;
}
