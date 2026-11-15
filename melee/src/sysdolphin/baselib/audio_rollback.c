#include "audio_rollback.h"

#ifdef MELEE_NATIVE

#include <string.h>

#include "axdriver.h"

enum {
    AUDIO_ROLLBACK_FRAME_CAPACITY = 16,
    AUDIO_ROLLBACK_START_CAPACITY = 32,
};

typedef struct HSD_AudioRollbackStart {
    int sound_id;
    int track;
    int channel;
    int instance;
} HSD_AudioRollbackStart;

typedef struct HSD_AudioRollbackFrame {
    uint64_t frame;
    uint32_t count;
    bool valid;
    HSD_AudioRollbackStart starts[AUDIO_ROLLBACK_START_CAPACITY];
} HSD_AudioRollbackFrame;

typedef struct HSD_AudioRollbackVoice {
    // Playback survives snapshot restores. Keep the start frame for each exact
    // pool generation so replayed commands cannot stop a future voice.
    uint64_t frame;
    int instance;
    bool valid;
} HSD_AudioRollbackVoice;

static struct HSD_AudioRollbackState {
    HSD_AudioRollbackFrame frames[AUDIO_ROLLBACK_FRAME_CAPACITY];
    HSD_AudioRollbackFrame pending;
    HSD_AudioRollbackVoice voices[HSD_AX_SOUND_CAPACITY];
    uint64_t reached[(HSD_AX_SOUND_CAPACITY + 63) / 64];
    bool stable_used[AUDIO_ROLLBACK_START_CAPACITY];
    HSD_AudioRollbackFrame* stable;
    HSD_AudioRollbackStats stats;
    bool active;
    bool resimulating;
    bool overflow;
} rollback_audio;

static bool starts_match(const HSD_AudioRollbackStart* start, int sound_id,
                         int track, int channel)
{
    return start->sound_id == sound_id && start->track == track &&
           start->channel == channel;
}

static bool append_start(const HSD_AudioRollbackStart* start)
{
    if (rollback_audio.pending.count == AUDIO_ROLLBACK_START_CAPACITY) {
        rollback_audio.overflow = true;
        return false;
    }
    rollback_audio.pending.starts[rollback_audio.pending.count++] = *start;
    return true;
}

void HSD_AudioRollbackReset(void)
{
    rollback_audio = (struct HSD_AudioRollbackState){ 0 };
}

bool HSD_AudioRollbackBeginFrame(uint64_t frame, bool resimulating)
{
    if (rollback_audio.active) {
        return false;
    }
    HSD_AudioRollbackFrame* stable =
        &rollback_audio.frames[frame % AUDIO_ROLLBACK_FRAME_CAPACITY];
    if (resimulating && (!stable->valid || stable->frame != frame)) {
        return false;
    }
    if (!resimulating) {
        *stable = (HSD_AudioRollbackFrame){ .frame = frame };
    }
    rollback_audio.pending = (HSD_AudioRollbackFrame){ .frame = frame };
    memset(rollback_audio.stable_used, 0, sizeof(rollback_audio.stable_used));
    memset(rollback_audio.reached, 0, sizeof(rollback_audio.reached));
    rollback_audio.stable = stable;
    rollback_audio.active = true;
    rollback_audio.resimulating = resimulating;
    rollback_audio.overflow = false;
    return true;
}

void HSD_AudioRollbackAbortFrame(void)
{
    rollback_audio.active = false;
    rollback_audio.resimulating = false;
    rollback_audio.stable = NULL;
}

bool HSD_AudioRollbackShouldStop(int instance)
{
    unsigned slot = (unsigned)instance & 0x7f;
    if (!rollback_audio.active || !rollback_audio.resimulating || instance < 0 ||
        slot >= HSD_AX_SOUND_CAPACITY)
    {
        return true;
    }
    const HSD_AudioRollbackVoice* voice = &rollback_audio.voices[slot];
    if (!voice->valid || voice->instance != instance ||
        voice->frame < rollback_audio.pending.frame)
    {
        return true;
    }
    if (voice->frame == rollback_audio.pending.frame &&
        (rollback_audio.reached[slot / 64] & (UINT64_C(1) << (slot % 64))))
    {
        return true;
    }
    ++rollback_audio.stats.protected_stops;
    return false;
}

bool HSD_AudioRollbackReuseStart(int sound_id, int track, int channel,
                                 int* instance)
{
    if (!rollback_audio.active || !rollback_audio.resimulating ||
        instance == NULL)
    {
        return false;
    }
    for (uint32_t i = 0; i < rollback_audio.stable->count; ++i) {
        HSD_AudioRollbackStart* stable = &rollback_audio.stable->starts[i];
        if (rollback_audio.stable_used[i] ||
            !starts_match(stable, sound_id, track, channel))
        {
            continue;
        }
        rollback_audio.stable_used[i] = true;
        append_start(stable);
        unsigned slot = (unsigned)stable->instance & 0x7f;
        if (stable->instance >= 0 && slot < HSD_AX_SOUND_CAPACITY &&
            rollback_audio.voices[slot].valid &&
            rollback_audio.voices[slot].instance == stable->instance)
        {
            rollback_audio.reached[slot / 64] |= UINT64_C(1) << (slot % 64);
        }
        *instance = stable->instance;
        ++rollback_audio.stats.reused_starts;
        return true;
    }
    return false;
}

void HSD_AudioRollbackRecordStart(int sound_id, int track, int channel,
                                  int instance)
{
    if (!rollback_audio.active) {
        return;
    }
    HSD_AudioRollbackStart start = {
        .sound_id = sound_id,
        .track = track,
        .channel = channel,
        .instance = instance,
    };
    append_start(&start);
    unsigned slot = (unsigned)instance & 0x7f;
    if (instance >= 0 && slot < HSD_AX_SOUND_CAPACITY) {
        rollback_audio.voices[slot] = (HSD_AudioRollbackVoice){
            .frame = rollback_audio.pending.frame,
            .instance = instance,
            .valid = true,
        };
        rollback_audio.reached[slot / 64] |= UINT64_C(1) << (slot % 64);
    }
    if (rollback_audio.resimulating) {
        ++rollback_audio.stats.new_starts;
    }
}

bool HSD_AudioRollbackEndFrame(void)
{
    if (!rollback_audio.active) {
        return false;
    }
    if (rollback_audio.resimulating) {
        // These are reconciliation cancellations, not replayed game commands.
        // They must stop starts that disappeared from the corrected frame.
        rollback_audio.active = false;
        for (uint32_t i = 0; i < rollback_audio.stable->count; ++i) {
            if (!rollback_audio.stable_used[i] &&
                rollback_audio.stable->starts[i].instance >= 0)
            {
                AXDriverKeyOff(rollback_audio.stable->starts[i].instance);
                ++rollback_audio.stats.canceled_starts;
            }
        }
    }
    bool complete = !rollback_audio.overflow;
    if (complete) {
        rollback_audio.pending.valid = true;
        *rollback_audio.stable = rollback_audio.pending;
    }
    HSD_AudioRollbackAbortFrame();
    return complete;
}

bool HSD_AudioRollbackFrameHasStarts(uint64_t frame)
{
    HSD_AudioRollbackFrame* saved =
        &rollback_audio.frames[frame % AUDIO_ROLLBACK_FRAME_CAPACITY];
    return saved->valid && saved->frame == frame && saved->count != 0;
}

HSD_AudioRollbackStats HSD_AudioRollbackGetStats(void)
{
    return rollback_audio.stats;
}

#endif
