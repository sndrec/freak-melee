#include "native_rollback_session.h"

#ifdef MELEE_NATIVE

#include "native_rollback_pages.h"
#include <sysdolphin/baselib/audio_rollback.h>
#include <sysdolphin/baselib/rumble.h>

static void fail(NativeRollbackSession* session, const char* reason)
{
    if (session != NULL && !session->failed) {
        session->failure_reason = reason;
        session->failed = true;
    }
}

static void begin_resimulation_output(NativeRollbackSession* session)
{
    if (!session->rumble_output_suppressed) {
        HSD_PadRumbleBeginResimulation();
        session->rumble_output_suppressed = true;
    }
}

static void end_resimulation_output(NativeRollbackSession* session)
{
    if (session->rumble_output_suppressed) {
        HSD_PadRumbleEndResimulation(session->local_port_mask);
        session->rumble_output_suppressed = false;
    }
}

bool nativeRollbackSession_Init(NativeRollbackSession* session,
                                uint8_t local_port_mask,
                                uint32_t dirty_page_capacity)
{
    if (session == NULL || dirty_page_capacity == 0) {
        return false;
    }
    *session = (NativeRollbackSession){ 0 };
    session->local_port_mask = local_port_mask;
    if (!nativeRollbackInput_Init(&session->inputs, local_port_mask)) {
        fail(session, "could not initialize input history");
        return false;
    }
    if (!nativeRollbackRegions_Build(&session->regions)) {
        fail(session, "could not locate rollback regions");
        return false;
    }
    HSD_AudioRollbackReset();
    if (!nativeRollbackPages_Init(
            session->regions.regions, session->regions.count,
            session->regions.exact_regions, session->regions.exact_count,
            NATIVE_ROLLBACK_SESSION_HISTORY_FRAMES, dirty_page_capacity))
    {
        fail(session, "could not initialize page history");
        return false;
    }
    session->active = true;
    return true;
}

void nativeRollbackSession_Shutdown(NativeRollbackSession* session)
{
    if (session == NULL) {
        return;
    }
    if (session->frame_active) {
        nativeRollbackPages_EndFrame();
        session->frame_active = false;
    }
    HSD_AudioRollbackAbortFrame();
    end_resimulation_output(session);
    nativeRollbackPages_Shutdown();
    session->active = false;
    session->resimulating = false;
}

bool nativeRollbackSession_BeginFrame(NativeRollbackSession* session,
                                      uint64_t frame)
{
    if (session == NULL || !session->active || session->failed ||
        session->frame_active)
    {
        return false;
    }
    session->current_frame = frame;
    if (!HSD_AudioRollbackBeginFrame(frame, session->resimulating)) {
        fail(session, "could not begin the audio-intent frame");
        return false;
    }
    session->frame_active = nativeRollbackPages_BeginFrame(frame);
    if (!session->frame_active) {
        HSD_AudioRollbackAbortFrame();
        fail(session, "could not begin a page snapshot");
    }
    return session->frame_active;
}

bool nativeRollbackSession_ResolveInputs(NativeRollbackSession* session,
                                         uint64_t frame,
                                         const PADStatus local_pads[4],
                                         PADStatus result[4])
{
    if (session == NULL || !session->frame_active ||
        frame != session->current_frame)
    {
        return false;
    }
    bool available =
        session->resimulating
            ? nativeRollbackInput_GetFrame(&session->inputs, frame, result)
            : nativeRollbackInput_PrepareFrame(&session->inputs, frame,
                                               local_pads, result);
    if (!available) {
        fail(session, "input history could not provide the current frame");
    }
    return available;
}

bool nativeRollbackSession_SubmitRemote(NativeRollbackSession* session,
                                        uint64_t frame, uint32_t port,
                                        const PADStatus* input)
{
    return session != NULL && session->active && !session->failed &&
           nativeRollbackInput_SubmitRemote(&session->inputs, frame, port,
                                            input);
}

bool nativeRollbackSession_Restore(NativeRollbackSession* session,
                                   uint64_t target_frame,
                                   uint64_t current_frame,
                                   uint64_t* next_frame)
{
    if (session == NULL || next_frame == NULL || !session->active ||
        session->failed || session->frame_active ||
        target_frame >= current_frame)
    {
        return false;
    }
    begin_resimulation_output(session);
    if (!nativeRollbackPages_ToFrame(target_frame, current_frame)) {
        fail(session, "could not restore the target frame");
        end_resimulation_output(session);
        return false;
    }
    session->catch_up_frame = current_frame;
    session->current_frame = target_frame;
    session->resimulating = true;
    *next_frame = target_frame;
    return true;
}

NativeRollbackSessionStep
nativeRollbackSession_EndFrame(NativeRollbackSession* session, uint64_t frame,
                               uint64_t* next_frame)
{
    if (session == NULL || next_frame == NULL || !session->frame_active ||
        frame != session->current_frame)
    {
        fail(session, "invalid frame boundary");
        return NativeRollbackSession_Failed;
    }
    session->frame_active = false;
    if (!HSD_AudioRollbackEndFrame()) {
        nativeRollbackPages_EndFrame();
        fail(session, "audio-intent history overflowed");
        return NativeRollbackSession_Failed;
    }
    if (!nativeRollbackPages_EndFrame()) {
        fail(session, "dirty-page history overflowed");
        return NativeRollbackSession_Failed;
    }

    uint64_t current_frame = frame + 1;
    session->current_frame = current_frame;
    *next_frame = current_frame;

    uint64_t oldest=current_frame>NATIVE_ROLLBACK_INPUT_CAPACITY ?
        current_frame-NATIVE_ROLLBACK_INPUT_CAPACITY : 0;
    if (oldest>session->inputs.oldest_required_frame &&
        !nativeRollbackInput_ReleaseBefore(&session->inputs,oldest)) {
        fail(session,"required correction inputs would expire");
        return NativeRollbackSession_Failed;
    }

    uint64_t correction_frame;
    if (nativeRollbackInput_TakeCorrection(&session->inputs,
                                           &correction_frame))
    {
        uint64_t catch_up_frame =
            session->resimulating ? session->catch_up_frame : current_frame;
        if (correction_frame >= current_frame ||
            !nativeRollbackSession_Restore(session, correction_frame,
                                           current_frame, next_frame))
        {
            if (!session->failed) {
                fail(session, "input correction could not be restored");
            }
            return NativeRollbackSession_Failed;
        }
        if (catch_up_frame > session->catch_up_frame) {
            session->catch_up_frame = catch_up_frame;
        }
        return NativeRollbackSession_Restored;
    }

    if (session->resimulating) {
        if (current_frame > session->catch_up_frame) {
            fail(session, "resimulation advanced beyond its endpoint");
            return NativeRollbackSession_Failed;
        }
        if (current_frame == session->catch_up_frame) {
            session->resimulating = false;
            end_resimulation_output(session);
            return NativeRollbackSession_CaughtUp;
        }
    }
    return NativeRollbackSession_Continue;
}

void nativeRollbackSession_Abort(NativeRollbackSession* session,
                                 const char* reason)
{
    if (session == NULL) {
        return;
    }
    fail(session, reason != NULL ? reason : "rollback session aborted");
}

bool nativeRollbackSession_IsResimulating(const NativeRollbackSession* session)
{
    return session != NULL && session->resimulating;
}

bool nativeRollbackSession_HasFailed(const NativeRollbackSession* session)
{
    return session == NULL || session->failed;
}

const char*
nativeRollbackSession_FailureReason(const NativeRollbackSession* session)
{
    return session != NULL ? session->failure_reason : "missing session";
}

#endif
