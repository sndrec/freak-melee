#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/gm/native_rollback_session.h>

static uint8_t tracked_memory[4096];
static uint32_t page_begin_count;
static uint32_t page_end_count;
static uint32_t page_restore_count;
static uint64_t restored_target;
static uint64_t restored_current;
static uint32_t audio_begin_count;
static uint32_t audio_end_count;
static uint32_t rumble_begin_count;
static uint32_t rumble_end_count;
static uint8_t resumed_local_mask;

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

bool nativeRollbackRegions_Build(NativeRollbackRegionSet* result)
{
    *result = (NativeRollbackRegionSet){ 0 };
    result->regions[0] = (NativeRollbackPagedRegion){
        .memory = tracked_memory,
        .size = sizeof(tracked_memory),
    };
    result->count = 1;
    result->tracked_bytes = sizeof(tracked_memory);
    return true;
}

bool nativeRollbackPages_Init(const NativeRollbackPagedRegion* regions,
                              uint32_t region_count,
                              const NativeRollbackExactRegion* exact_regions,
                              uint32_t exact_region_count,
                              uint32_t frame_capacity,
                              uint32_t dirty_page_capacity)
{
    (void) exact_regions;
    return regions != NULL && region_count == 1 && exact_region_count == 0 &&
           frame_capacity == NATIVE_ROLLBACK_SESSION_HISTORY_FRAMES &&
           dirty_page_capacity == 32;
}

void nativeRollbackPages_Shutdown(void) {}

bool nativeRollbackPages_BeginFrame(uint64_t frame)
{
    (void) frame;
    ++page_begin_count;
    return true;
}

bool nativeRollbackPages_EndFrame(void)
{
    ++page_end_count;
    return true;
}

bool nativeRollbackPages_ToFrame(uint64_t frame, uint64_t current_frame)
{
    ++page_restore_count;
    restored_target = frame;
    restored_current = current_frame;
    return true;
}

void HSD_AudioRollbackReset(void) {}

bool HSD_AudioRollbackBeginFrame(uint64_t frame, bool resimulating)
{
    (void) frame;
    (void) resimulating;
    ++audio_begin_count;
    return true;
}

void HSD_AudioRollbackAbortFrame(void) {}

bool HSD_AudioRollbackEndFrame(void)
{
    ++audio_end_count;
    return true;
}

void HSD_PadRumbleBeginResimulation(void)
{
    ++rumble_begin_count;
}

void HSD_PadRumbleEndResimulation(uint8_t local_port_mask)
{
    ++rumble_end_count;
    resumed_local_mask = local_port_mask;
}

static PADStatus pad(uint16_t buttons)
{
    return (PADStatus){
        .button = buttons,
        .err = PAD_ERR_NONE,
    };
}

int main(void)
{
    NativeRollbackSession session;
    PADStatus local[4] = { 0 };
    PADStatus resolved[4];
    uint64_t next_frame;

    require(nativeRollbackSession_Init(&session, 1U << 0, 32),
            "initialize rollback session");
    require(session.regions.tracked_bytes == sizeof(tracked_memory),
            "session owns the selected game-state boundary");

    local[0] = pad(PAD_BUTTON_X);
    require(nativeRollbackSession_BeginFrame(&session, 0), "begin frame 0");
    require(nativeRollbackSession_ResolveInputs(&session, 0, local, resolved),
            "resolve initial local and predicted inputs");
    require(resolved[0].button == PAD_BUTTON_X && resolved[1].button == 0,
            "local input is confirmed and remote input begins neutral");
    require(nativeRollbackSession_EndFrame(&session, 0, &next_frame) ==
                    NativeRollbackSession_Continue &&
                next_frame == 1,
            "commit frame 0");

    require(nativeRollbackSession_BeginFrame(&session, 1), "begin frame 1");
    require(nativeRollbackSession_ResolveInputs(&session, 1, local, resolved),
            "simulate frame 1 from a prediction");
    PADStatus confirmed = pad(PAD_BUTTON_A);
    require(nativeRollbackSession_SubmitRemote(&session, 1, 1, &confirmed),
            "submit a differing remote confirmation");
    require(nativeRollbackSession_EndFrame(&session, 1, &next_frame) ==
                    NativeRollbackSession_Restored &&
                next_frame == 1,
            "late input restores its earliest affected frame");
    require(page_restore_count == 1 && restored_target == 1 &&
                restored_current == 2,
            "session restores the page timeline from its live endpoint");
    require(nativeRollbackSession_IsResimulating(&session) &&
                rumble_begin_count == 1,
            "hidden catch-up suppresses physical rumble output");

    require(nativeRollbackSession_BeginFrame(&session, 1),
            "begin corrected frame 1");
    memset(resolved, 0, sizeof(resolved));
    require(nativeRollbackSession_ResolveInputs(&session, 1, local, resolved),
            "reuse stored inputs during resimulation");
    require(resolved[1].button == PAD_BUTTON_A,
            "corrected input survives restoring game state");
    require(nativeRollbackSession_EndFrame(&session, 1, &next_frame) ==
                    NativeRollbackSession_CaughtUp &&
                next_frame == 2,
            "corrected timeline reaches the former endpoint");
    require(!nativeRollbackSession_IsResimulating(&session) &&
                rumble_end_count == 1 && resumed_local_mask == (1U << 0),
            "committed endpoint resumes only local rumble output");

    require(nativeRollbackSession_Restore(&session, 1, 2, &next_frame) &&
                next_frame == 1,
            "diagnostics can deliberately repeat a completed interval");
    require(nativeRollbackSession_BeginFrame(&session, 1),
            "begin repeated corrected frame");
    require(nativeRollbackSession_ResolveInputs(&session, 1, local, resolved),
            "repeated interval uses the same stored input");
    require(nativeRollbackSession_EndFrame(&session, 1, &next_frame) ==
                NativeRollbackSession_CaughtUp,
            "repeated interval reaches the same endpoint");

    require(page_begin_count == 4 && page_end_count == 4 &&
                audio_begin_count == 4 && audio_end_count == 4,
            "each simulated frame brackets state and audio exactly once");
    require(page_restore_count == 2 && rumble_begin_count == 2 &&
                rumble_end_count == 2,
            "each restore has one output-suppression lifetime");
    nativeRollbackSession_Shutdown(&session);
    puts("Rollback session schedules late-input correction and catch-up");
    return 0;
}
