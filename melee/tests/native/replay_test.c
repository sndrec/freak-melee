#include <melee/gm/native_replay.h>
#include <melee/gm/gmmain.h>

#include <dolphin/pad.h>
#include <sysdolphin/baselib/random.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool gmMain_ExitRequested;
static u32 random_seed;
u32* seed_ptr = &random_seed;
static PADReadCallback read_callback;
static uint64_t state_digest;

bool nativeMatchFixture_Apply(uint64_t sample)
{
    (void) sample;
    return true;
}

uint64_t nativeReplay_StateDigest(uint64_t sample_index)
{
    (void) sample_index;
    return state_digest;
}

bool nativeReplay_StateObserversEnabled(void)
{
    // This fixture supplies a digest observer without the engine trace module.
    return true;
}

void PADSetReadCallback(PADReadCallback callback)
{
    read_callback = callback;
}

static void require(bool result, const char* message)
{
    if (!result) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

int main(void)
{
    const char* path = "native_replay_test.bin";
    const char* alternate = "native_replay_alternate.bin";
    PADStatus pads[4] = { 0 };
    require(nativeReplay_Open(path, NativeReplay_Record,
                              UINT64_C(0x12345678)),
            "open recording");
    require(read_callback == NULL && !nativeReplay_IsPlayback(),
            "recording uses canonical pad hook");
    pads[0].button = 0x20;
    pads[0].err = 0;
    nativeReplay_RecordPads(pads); // Boot input before the runtime seed is set.
    random_seed = nativeReplay_Seed(0x1234ABCD);
    state_digest = 0xCAFE;
    pads[0].button = 0x100;
    pads[0].stickX = -43;
    nativeReplay_RecordPads(pads);
    random_seed = 0xAABBCCDD;
    state_digest = 0xBEEF;
    pads[0].button = 0x200;
    pads[2].err = PAD_ERR_NO_CONTROLLER;
    nativeReplay_RecordPads(pads);
    require(nativeReplay_SampleCount() == 3, "record count");
    require(nativeReplay_Close() && read_callback == NULL, "close recording");

    unsigned char recording[24 + 3 * 80];
    FILE* stream = fopen(path, "rb");
    require(stream && fread(recording, 1, sizeof(recording), stream) == sizeof(recording), "read alternate fixture");
    require(fclose(stream) == 0, "close alternate source");
    uint16_t corrected_button = PAD_BUTTON_X;
    memcpy(recording + 24 + 80 + 8, &corrected_button, sizeof(corrected_button));
    stream = fopen(alternate, "wb");
    require(stream && fwrite(recording, 1, sizeof(recording), stream) == sizeof(recording), "write alternate fixture");
    require(fclose(stream) == 0, "close alternate fixture");

    require(!nativeReplay_Open(path, NativeReplay_Verify,
                               UINT64_C(0x12345679)),
            "reject different match settings");

    random_seed = 0;
    state_digest = 0;
    require(nativeReplay_Open(path, NativeReplay_Verify,
                              UINT64_C(0x12345678)),
            "open playback");
    require(read_callback != NULL && nativeReplay_IsPlayback(),
            "playback replaces device pad read");
    require(!nativeReplay_PrepareAlternateInputs(alternate), "alternate inputs require inputs-only mode");
    memset(pads, 0, sizeof(pads));
    read_callback(pads);
    require(pads[0].button == 0x20, "boot sample restored");
    nativeReplay_RecordPads(pads);
    require(nativeReplay_SampleCount() == 1,
            "playback does not record canonical pads");
    random_seed = nativeReplay_Seed(0xFFFFFFFF);
    require(random_seed == 0x1234ABCD, "initial seed restored");
    state_digest = 0xCAFE;
    memset(pads, 0, sizeof(pads));
    read_callback(pads);
    require(pads[0].button == 0x100 && pads[0].stickX == -43,
            "match sample restored");
    random_seed = 0xAABBCCDD;
    state_digest = 0xBEEF;
    memset(pads, 0, sizeof(pads));
    read_callback(pads);
    require(pads[0].button == 0x200 &&
                pads[2].err == PAD_ERR_NO_CONTROLLER,
            "all pad states restored");
    require(nativeReplay_SampleCount() == 3, "playback count");
    require(nativeReplay_RewindToSample(1), "rewind playback");
    require(nativeReplay_SampleCount() == 1, "rewound playback count");
    random_seed = 0x1234ABCD;
    state_digest = 0xCAFE;
    memset(pads, 0, sizeof(pads));
    read_callback(pads);
    require(pads[0].button == 0x100 && pads[0].stickX == -43,
            "rewound sample restored");
    random_seed = 0xAABBCCDD;
    state_digest = 0xBEEF;
    read_callback(pads);
    read_callback(pads);
    require(gmMain_ExitRequested, "playback stops at end of recording");
    require(nativeReplay_Close(), "close playback");

    gmMain_ExitRequested = false;
    random_seed = 99;
    state_digest = 77;
    require(nativeReplay_Open(path, NativeReplay_Verify,
                              UINT64_C(0x12345678)),
            "open playback for branched timeline");
    nativeReplay_SetTimelineValidation(false);
    memset(pads, 0, sizeof(pads));
    read_callback(pads);
    require(!gmMain_ExitRequested && pads[0].button == 0x20,
            "branched timeline can continue consuming recorded input");
    require(nativeReplay_Close(), "close branched timeline playback");

    gmMain_ExitRequested = false;
    random_seed = 0;
    state_digest = 77;
    require(nativeReplay_Open(path, NativeReplay_Verify,
                              UINT64_C(0x12345678)),
            "open playback for state check");
    read_callback(pads);
    require(gmMain_ExitRequested, "fighter-state divergence stops playback");
    require(!nativeReplay_Close(), "fighter-state divergence reports failure");

    gmMain_ExitRequested = false;
    random_seed = 99;
    state_digest = 0;
    require(nativeReplay_Open(path, NativeReplay_Verify,
                              UINT64_C(0x12345678)),
            "open playback for divergence check");
    read_callback(pads);
    require(gmMain_ExitRequested, "RNG divergence stops playback");
    require(!nativeReplay_Close(), "RNG divergence reports failure");

    gmMain_ExitRequested = false;
    random_seed = 99;
    state_digest = 77;
    require(nativeReplay_Open(path, NativeReplay_InputsOnly,
                              UINT64_C(0x12345679)),
            "input comparison allows changed match content");
    require(!nativeReplay_UseAlternateInputs(true), "unprepared alternate refused");
    require(nativeReplay_PrepareAlternateInputs(alternate), "prepare alternate before input consumption");
    require(!nativeReplay_PrepareAlternateInputs(alternate), "duplicate preparation refused");
    read_callback(pads);
    require(!gmMain_ExitRequested && pads[0].button == 0x20,
            "comparison restores boot input despite divergent state");
    random_seed = nativeReplay_Seed(0);
    require(random_seed == 0x1234ABCD,
            "comparison reuses recorded initial seed");
    read_callback(pads);
    require(!gmMain_ExitRequested && pads[0].button == 0x100 &&
                pads[0].stickX == -43,
            "comparison restores match input despite divergent state");
    require(nativeReplay_RewindToSample(1) && nativeReplay_UseAlternateInputs(true), "select corrected stream at old sample");
    read_callback(pads);
    require(pads[0].button == PAD_BUTTON_X && pads[0].stickX == -43 && nativeReplay_SampleCount() == 2, "corrected pads preserve index and other fields");
    require(nativeReplay_RewindToSample(1), "rewind active alternate stream");
    read_callback(pads);
    require(pads[0].button == PAD_BUTTON_X, "corrected interval repeat");
    require(nativeReplay_UseAlternateInputs(false), "recover primary at current sample");
    read_callback(pads);
    require(pads[0].button == 0x200 && nativeReplay_SampleCount() == 3, "primary resumes without consuming skipped inputs");
    require(nativeReplay_Close(), "close input comparison");
    require(remove(path) == 0, "remove test recording");
    require(remove(alternate) == 0, "remove alternate recording");
    return 0;
}
