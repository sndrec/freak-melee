#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <melee/gm/native_rollback_input.h>

static void require(bool result, const char* message)
{
    if (!result) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static PADStatus pad(uint16_t button, int8_t stick_x, uint8_t trigger)
{
    return (PADStatus){
        .button = button,
        .stickX = stick_x,
        .triggerLeft = trigger,
        .err = PAD_ERR_NONE,
    };
}

int main(void)
{
    NativeRollbackInputHistory history;
    PADStatus local[4] = { 0 };
    PADStatus frame[4];
    uint64_t correction;
    uint64_t latest_confirmed;
    require(nativeRollbackInput_Init(&history, (1U << 0) | (1U << 2)),
            "initialize two-local-player history");

    local[0] = pad(PAD_BUTTON_A, 12, 0);
    local[2] = pad(PAD_BUTTON_B, -8, 0);
    require(nativeRollbackInput_PrepareFrame(&history, 0, local, frame),
            "prepare first predicted frame");
    require(frame[0].button == PAD_BUTTON_A && frame[2].button == PAD_BUTTON_B,
            "shared-PC local slots are recorded independently");
    require(frame[1].button == 0 && frame[3].button == 0,
            "remote slots begin with neutral prediction");
    require(nativeRollbackInput_ConfirmedMask(&history, 0) ==
                ((1U << 0) | (1U << 2)),
            "frame confirmation mask exposes local ownership");

    PADStatus remote = pad(0, 0, 25);
    require(nativeRollbackInput_SubmitRemote(&history, 0, 1, &remote),
            "confirm equivalent first remote input");
    require(!nativeRollbackInput_TakeCorrection(&history, &correction),
            "deadzone-equivalent confirmation needs no rollback");
    require((nativeRollbackInput_ConfirmedMask(&history, 0) & (1U << 1)) != 0,
            "remote confirmation appears in the frame mask");
    require(!nativeRollbackInput_LatestConfirmedFrame(
                &history, (1U << 1) | (1U << 3), &latest_confirmed),
            "latest confirmation requires every requested port");

    memset(local, 0, sizeof(local));
    local[0] = pad(PAD_BUTTON_X, 20, 0);
    local[2] = pad(PAD_BUTTON_Y, -20, 0);
    require(nativeRollbackInput_PrepareFrame(&history, 1, local, frame),
            "prepare second predicted frame");
    require(frame[1].triggerLeft == 25,
            "missing remote input predicts latest confirmation");
    require(nativeRollbackInput_PrepareFrame(&history, 2, local, frame),
            "prepare later predicted frame");
    require(frame[1].triggerLeft == 25,
            "later frame initially carries stale prediction");

    remote = pad(PAD_BUTTON_B, 40, 90);
    require(nativeRollbackInput_SubmitRemote(&history, 1, 1, &remote),
            "submit mismatching remote input");
    require(nativeRollbackInput_TakeCorrection(&history, &correction) &&
                correction == 1,
            "mismatch requests rollback from predicted frame");
    require(nativeRollbackInput_GetFrame(&history, 1, frame) &&
                frame[1].button == PAD_BUTTON_B && frame[1].stickX == 40,
            "confirmed input replaces prediction");

    require(nativeRollbackInput_GetFrame(&history, 2, frame) &&
                frame[1].button == PAD_BUTTON_B && frame[1].stickX == 40,
            "correction propagates through later predicted frames");
    require(nativeRollbackInput_SubmitRemote(&history, 1, 1, &remote),
            "matching repeated confirmation is accepted");
    PADStatus contradiction = pad(PAD_BUTTON_A, 40, 90);
    require(!nativeRollbackInput_SubmitRemote(&history, 1, 1, &contradiction),
            "contradictory repeated confirmation is rejected");

    PADStatus future = pad(PAD_BUTTON_Y, -30, 100);
    require(nativeRollbackInput_SubmitRemote(&history, 3, 3, &future),
            "queue future remote confirmation");
    require(nativeRollbackInput_PrepareFrame(&history, 3, local, frame),
            "prepare frame with queued confirmation");
    require(frame[3].button == PAD_BUTTON_Y &&
                !(history.frames[3].predicted_mask & (1U << 3)),
            "future confirmation is consumed without prediction");
    require(nativeRollbackInput_LatestConfirmedFrame(
                &history, (1U << 1) | (1U << 3), &latest_confirmed) &&
                latest_confirmed == 1,
            "slowest remote port bounds the fully confirmed timeline");

    for (uint64_t at = 4; at < NATIVE_ROLLBACK_INPUT_CAPACITY + 4; ++at) {
        if(at>=NATIVE_ROLLBACK_INPUT_CAPACITY)
            require(nativeRollbackInput_ReleaseBefore(&history,at-NATIVE_ROLLBACK_INPUT_CAPACITY+1),
                    "release frames beyond the owner's retained window");
        require(nativeRollbackInput_PrepareFrame(&history, at, local, frame),
                "advance bounded history");
        if (at == NATIVE_ROLLBACK_INPUT_CAPACITY / 2)
            require(nativeRollbackInput_ConfirmedMask(&history, 0) == 7 &&
                    nativeRollbackInput_GetFrame(&history, 0, frame),
                    "retain confirmed inputs long enough for delayed checksum comparison");
    }
    require(!nativeRollbackInput_GetFrame(&history, 0, frame),
            "expired frame is unavailable");
    require(!nativeRollbackInput_SubmitRemote(&history, 0, 1, &remote),
            "expired remote input is rejected");
    require(!nativeRollbackInput_SubmitRemote(&history, 20, 0, &remote),
            "remote input cannot replace a local slot");

    puts("Rollback input history predicts, confirms, and requests correction");
    return 0;
}
