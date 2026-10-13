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
    uint8_t volume;
    uint8_t pan;
} HSD_AudioRollbackStart;

typedef struct HSD_AudioRollbackFrame {
    uint64_t frame;
    uint32_t count;
    bool valid;
    HSD_AudioRollbackStart starts[AUDIO_ROLLBACK_START_CAPACITY];
} HSD_AudioRollbackFrame;

static struct HSD_AudioRollbackState {
    HSD_AudioRollbackFrame frames[AUDIO_ROLLBACK_FRAME_CAPACITY];
    HSD_AudioRollbackFrame pending;
    bool stable_used[AUDIO_ROLLBACK_START_CAPACITY];
    HSD_AudioRollbackFrame* stable;
    HSD_AudioRollbackStats stats;
    bool active;
    bool resimulating;
    bool overflow;
} rollback_audio;

static bool starts_match(const HSD_AudioRollbackStart* start, int sound_id,
                         uint8_t volume, uint8_t pan, int track, int channel)
{
    return start->sound_id == sound_id && start->volume == volume &&
           start->pan == pan && start->track == track &&
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

bool HSD_AudioRollbackReuseStart(int sound_id, uint8_t volume, uint8_t pan,
                                 int track, int channel, int* instance)
{
    if (!rollback_audio.active || !rollback_audio.resimulating ||
        instance == NULL)
    {
        return false;
    }
    for (uint32_t i = 0; i < rollback_audio.stable->count; ++i) {
        HSD_AudioRollbackStart* stable = &rollback_audio.stable->starts[i];
        if (rollback_audio.stable_used[i] ||
            !starts_match(stable, sound_id, volume, pan, track, channel))
        {
            continue;
        }
        rollback_audio.stable_used[i] = true;
        append_start(stable);
        *instance = stable->instance;
        ++rollback_audio.stats.reused_starts;
        return true;
    }
    return false;
}

void HSD_AudioRollbackRecordStart(int sound_id, uint8_t volume, uint8_t pan,
                                  int track, int channel, int instance)
{
    if (!rollback_audio.active) {
        return;
    }
    HSD_AudioRollbackStart start = {
        .sound_id = sound_id,
        .track = track,
        .channel = channel,
        .instance = instance,
        .volume = volume,
        .pan = pan,
    };
    append_start(&start);
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
