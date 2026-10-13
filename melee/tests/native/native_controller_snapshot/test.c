#include <sysdolphin/baselib/native_controller_snapshot.h>
#include <sysdolphin/baselib/native_controller_snapshot_internal.h>
#include <sysdolphin/baselib/rumble_output.h>
#include <melee/gm/native_profile_input.h>
#include <melee/gm/native_replay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern HSD_RumbleData HSD_Rumble_804C22E0[4];
static unsigned checks, refusals, motor_calls, pad_reads, pad_resets, recordings;
#define CHECK(test) do { ++checks; if (!(test)) { fprintf(stderr, "line %d: %s\n", __LINE__, #test); exit(1); } } while (0)
static HSD_PadData queue[5], foreign_queue[5];
static HSD_PadRumbleListData lists[12];
static PADStatus input[4];
static unsigned char saved[8192], after[8192], malformed[8193], final[8192];
static size_t saved_bytes;
static u16 commands[] = {0x2006, 0x4002, 0x8003, 0x2002, 0xA000, 0};

// External hardware and replay/profile hooks are controlled here. Every pad
// queue, clamp/normalization/history and rumble command operation is production.
BOOL PADInit(void) { return true; }
u32 PADRead(PADStatus* status) { ++pad_reads; memcpy(status, input, sizeof(input)); return 0; }
int PADReset(unsigned long mask) { (void)mask; ++pad_resets; return true; }
void PADControlMotor(s32 port, u32 command)
{ CHECK(port >= 0 && port < 4 && command <= PAD_MOTOR_STOP_HARD); ++motor_calls; }
bool nativeReplay_IsPlayback(void) { return true; }
void nativeReplay_RecordPads(const PADStatus* status) { CHECK(status); ++recordings; }
void nativeProfileInput_Apply(PADStatus status[4]) { (void)status; CHECK(false); }

static void filter(PADStatus status[4])
{
    CHECK(status && nativeControllerInputDepth == 1);
    size_t written = 991;
    CHECK(!nativeController_SnapshotBytes());
    CHECK(!nativeController_Save(after, sizeof(after), &written) && written == 991);
    CHECK(!nativeController_ValidateSnapshot(saved, saved_bytes));
    CHECK(!nativeController_Load(saved, saved_bytes));
}

static void other_filter(PADStatus status[4]) { (void)status; }

static void sample(unsigned frame, bool consume)
{
    memset(input, 0, sizeof(input));
    for (unsigned port = 0; port < 4; ++port) {
        input[port].button = frame % 4 ? PAD_BUTTON_A : PAD_BUTTON_B;
        input[port].stickX = (s8)((int)(frame * 7 + port * 11) % 80 - 40);
        input[port].stickY = (s8)(40 - (int)(frame * 3 + port * 5) % 80);
        input[port].substickX = 17;
        input[port].triggerLeft = 80;
        input[port].extButton = frame % 2 ? NATIVE_PROFILE_EXT_SHORT_HOP : NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED;
    }
    if (frame == 4) input[2].err = PAD_ERR_TRANSFER;
    if (frame == 5) input[3].err = PAD_ERR_NO_CONTROLLER;
    HSD_PadRenewRawStatus(false);
    CHECK(!nativeControllerInputDepth);
    if (consume) {
        HSD_PadRenewMasterStatus();
        HSD_PadRenewCopyStatus();
        HSD_PadRenewGameStatus();
    }
}

static void reject(size_t bytes)
{
    unsigned char before[8192]; size_t before_bytes, after_bytes;
    CHECK(nativeController_Save(before, sizeof(before), &before_bytes));
    unsigned motors = motor_calls, reads = pad_reads, records = recordings;
    CHECK(!nativeController_ValidateSnapshot(malformed, bytes));
    CHECK(!nativeController_Load(malformed, bytes));
    CHECK(nativeController_Save(after, sizeof(after), &after_bytes));
    CHECK(before_bytes == after_bytes && !memcmp(before, after, before_bytes));
    CHECK(motors == motor_calls && reads == pad_reads && records == recordings);
    ++refusals;
}

int main(void)
{
    CHECK(!nativeController_SnapshotBytes());
    HSD_PadInit(5, queue, 12, lists);
    HSD_PadLibData.repeat_start = 3;
    HSD_PadLibData.repeat_interval = 2;
    HSD_PadSetRawFilter(filter);
    CHECK(nativeController_Save(saved, sizeof(saved), &saved_bytes));
    CHECK(saved_bytes == sizeof(NativeControllerSnapshotHeader) + sizeof(queue) + sizeof(lists));
    HSD_PadRumbleBeginResimulation();
    CHECK(HSD_PadRumbleAdd(0, 11, -2, 4, commands));
    CHECK(HSD_PadRumbleAdd(1, 22, 20, 2, commands));
    HSD_PadRumbleOn(3);
    for (unsigned i = 0; i < 8; ++i) sample(i, i % 3 == 2);
    CHECK(HSD_PadLibData.qcount == 5); // Full queue with wrapped indices.
    CHECK(nativeController_Save(saved, sizeof(saved), &saved_bytes));
    NativeControllerSnapshotHeader header;
    memcpy(&header, saved, sizeof(header));
    CHECK(header.rumble[0].nb_list == 1 && header.rumble[1].nb_list == 1);
    unsigned raw_calls = pad_reads, motors = motor_calls, records = recordings;
    CHECK(nativeController_Load(saved, saved_bytes));
    CHECK(raw_calls == pad_reads && motors == motor_calls && records == recordings);

    // Repeat the same queued/resolved inputs and advancing command cursors
    // from the captured boundary. Suppression remains active through Load.
    for (unsigned i = 0; i < 12; ++i) sample(i, true);
    size_t final_bytes, bytes;
    CHECK(nativeController_Save(final, sizeof(final), &final_bytes));
    CHECK(HSD_PadRumbleAdd(2, 99, -2, 1, commands));
    HSD_PadRumbleListData* reused = HSD_Rumble_804C22E0[2].listdatap;
    CHECK(nativeController_Load(saved, saved_bytes));
    for (unsigned i = 0; i < 12; ++i) sample(i, true);
    CHECK(nativeController_Save(after, sizeof(after), &bytes));
    CHECK(bytes == final_bytes && !memcmp(final, after, bytes));
    CHECK(!motor_calls);
    CHECK(HSD_PadRumbleAdd(2, 99, -2, 1, commands));
    CHECK(HSD_Rumble_804C22E0[2].listdatap == reused);
    CHECK(nativeController_Load(saved, saved_bytes));
    CHECK(nativeController_Save(after, sizeof(after), &bytes));
    CHECK(bytes == saved_bytes && !memcmp(saved, after, bytes));

    memcpy(malformed, saved, saved_bytes); reject(saved_bytes - 1);
    memcpy(malformed, saved, saved_bytes); reject(saved_bytes + 1);
    NativeControllerSnapshotHeader bad = header;
    ++bad.epoch;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.queue = foreign_queue;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.qread = 5;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.qcount = 6;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.qwrite = (bad.library.qwrite + 1) % 5;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.rumble_info.max_list = 11;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.rumble[0].nb_list++;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.rumble[0].listdatap = (void*)((uintptr_t)lists + 1);
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.rumble_info.listdatap = lists + 12;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    bad = header; bad.library.rumble_info.listdatap = bad.rumble[0].listdatap;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed, &bad, sizeof(bad)); reject(saved_bytes);
    // A late record corruption must not partially restore the queue/statuses.
    HSD_PadRumbleListData item;
    size_t last = sizeof(header) + sizeof(queue) + 11 * sizeof(item);
    memcpy(&item, saved + last, sizeof(item)); item.next = lists + 11;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed + last, &item, sizeof(item)); reject(saved_bytes);
    size_t active = sizeof(header) + sizeof(queue);
    memcpy(&item, saved + active, sizeof(item)); item.headp = NULL;
    memcpy(malformed, saved, saved_bytes); memcpy(malformed + active, &item, sizeof(item)); reject(saved_bytes);
    memcpy(&item, saved + active, sizeof(item)); item.listp = (u16*)((uintptr_t)item.listp + 1);
    memcpy(malformed, saved, saved_bytes); memcpy(malformed + active, &item, sizeof(item)); reject(saved_bytes);
    size_t written = 991;
    CHECK(!nativeController_Save(queue, sizeof(saved), &written) && written == 991);
    CHECK(!nativeController_Save(after, sizeof(after), (size_t*)lists));
    CHECK(!nativeController_Save(after, sizeof(after), (size_t*)after));
    CHECK(!nativeController_Save(after, saved_bytes - 1, &written) && written == 991);

    // Layout and callback changes are explicit history retirement, including
    // reinitialization with identical addresses. Restores never call PADInit.
    HSD_PadSetRawFilter(other_filter);
    CHECK(!nativeController_Load(saved, saved_bytes));
    CHECK(nativeController_Save(after, sizeof(after), &bytes));
    HSD_PadSetRawFilter(other_filter);
    CHECK(nativeController_ValidateSnapshot(after, bytes));
    HSD_PadInit(5, queue, 12, lists);
    CHECK(!nativeController_Load(after, bytes));
    HSD_PadInit(5, queue, 0, NULL);
    CHECK(nativeController_Save(after, sizeof(after), &bytes));
    CHECK(bytes == sizeof(header) + sizeof(queue));
    CHECK(nativeController_Load(after, bytes));
    for (unsigned port = 0; port < 4; ++port) input[port].err = PAD_ERR_NO_CONTROLLER;
    HSD_PadRenewRawStatus(true); // All disconnected: return before queue write.
    CHECK(!nativeControllerInputDepth && HSD_PadLibData.qcount == 0);
    CHECK(nativeController_SnapshotBytes() == bytes);
    HSD_PadRumbleEndResimulation(0xF);
    CHECK(motor_calls == 4);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"refusals\":%u,\"snapshot_bytes\":%zu,\"pad_samples\":%u,\"suppressed_restore_motor_calls\":0,\"final_reconciled_motor_calls\":%u}\n",
        checks, refusals, saved_bytes, pad_reads, motor_calls);
    return 0;
}
