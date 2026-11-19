#include <melee/lb/lb_00CE.h>
#include "axdriver.h"
#include "audio_rollback.h"
#include "audio_aux.h"
#include "sfxcommands.h"
#include "sfxbank.h"

#include <math.h>
#include <stdatomic.h>
#include <limits.h>
#include <string.h>

#include "axdriver.static.h"
#include "debug.h"
#include "synth.h"
#include <dolphin/ax.h>
#include <dolphin/dvd.h>
#include <dolphin/os.h>

void AXDriverUnlink(HSD_SM* v, HSD_SM** head)
{
    HSD_SM* p;
    HSD_SM* n;

    if (v != NULL) {
        p = v->prev;
        n = v->next;
        v->next = NULL;
        v->prev = NULL;
        if (p != NULL) {
            p->next = n;
        }
        if (n != NULL) {
            n->prev = p;
        }
        if (*head == v) {
            *head = n;
        }
        HSD_ASSERT(113, *head != v);
    }
}

static inline void unk_inline(HSD_SM* v, HSD_SM** head)
{
    if (v == NULL) {
        return;
    }

    HSD_ASSERT(0x7A, *head != v);

    v->prev = NULL;
    if (*head != NULL) {
        (*head)->prev = v;
    }
    v->next = *head;
    *head = v;
}

static inline bool tmp(HSD_SM* v)
{
    int idx;
    u32 state;

    state = v->flags & SMSTATE_MASK;
    HSD_ASSERTMSG(0x92, state == SMSTATE_ACTIVE || state == SMSTATE_SLEEP,
                  "(v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE || "
                  "(v->flags&SMSTATE_MASK) == SMSTATE_SLEEP");

    idx = v->vID;
    if (v->vID != -1) {
        AXDriver_804C5920[idx & 0x3F] = 0;
        v->vID = -1;
        HSD_SynthSFXKeyOff(idx);
        AXDriver_804D77C8--;
    }

    v->flags &= ~0x20000000;
    v->flags &= ~SMSTATE_MASK;
    return true;
}

bool AXDriverKeyOff(int vid)
{
    bool result;
    int idx;
    HSD_SM* v;
    PAD_STACK(8);

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }

    v = &AXDriver_804C45A0[idx];

    if (v->unk != vid || !(v->flags & SMSTATE_MASK) ||
        !HSD_AudioRollbackShouldStop(vid))
    {
        return false;
    }

    if (v == NULL) {
        result = false;
    } else {
        result = tmp(v);
    }

    return result;
}

void HSD_AudioSFXKeyOffAll(void)
{
    HSD_SM* v = AXDriver_804D7794;
    PAD_STACK(8);

    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && HSD_AudioRollbackShouldStop(v->unk)) {
            if (v != NULL) {
                tmp(v);
            }
        }
        v = v->next;
    }
}

void HSD_AudioSFXKeyOffTrack(int track)
{
    HSD_SM* v = AXDriver_804D7794;

    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->track == track &&
            HSD_AudioRollbackShouldStop(v->unk))
        {
            if (v != NULL) {
                tmp(v);
            }
        }
        v = v->next;
    }
}

void AXDriverRetirePackageSounds(u32 first_id, u32 count)
{
    for (HSD_SM* voice = AXDriver_804D7794; voice; voice = voice->next) {
        if ((voice->flags & SMSTATE_MASK) &&
            (u32)voice->sound_id - first_id < count)
        {
            tmp(voice);
        }
    }
}

#ifdef MUST_MATCH
/// MSL sqrtf expansion (src/MSL/math_ppc.h) writing its result through a
/// caller-provided slot, as in sqrtf_store in lbcollision.c and mplib.c.
/// Evidence: retail AXDriver_8038BF6C keeps its eight sqrt results in
/// adjacent 4-byte stack temps at frame offsets 0x10..0x2C (an 8-byte
/// aligned base), one slot per call in source order, each accessed as a
/// stfs/lfs pair. The volatile-qualified accesses through the slot pointer
/// pin those pairs without changing behavior.
static inline float sqrtf_store(float x, volatile float* y)
{
    if (x > 0.0F) {
        double guess = __frsqrte((double) x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        guess = 0.5 * guess * (3.0 - guess * guess * x);
        *y = (float) (x * guess);
        return *(volatile float*) y;
    }
    return x;
}
#else
#define sqrtf_store(x, y) sqrtf(x)
#endif

void AXDriver_8038BF6C(HSD_SM* v)
{
    u32 flag;
    int i;
    /// sqrtf result slots; retail frame offsets 0x10..0x2C (base aligned 8)
    float sqrt_tmp[8] ATTRIBUTE_ALIGN(8);

    for (i = 0; i <= 9; i++) {
        flag = 1 << i;

        if (v->flags & flag) {
            switch (flag) {
            case 0x1: {
                float left_vol = (v->x26 * v->x24[0]) / 65535.0F;
                float right_vol = (v->x27 * v->x24[1]) / 65535.0F;
                float left_sqrt = sqrtf_store(left_vol, &sqrt_tmp[7]);
                float left_inv_sqrt =
                    sqrtf_store(1.0F - left_vol, &sqrt_tmp[6]);
                float right_sqrt = sqrtf_store(right_vol, &sqrt_tmp[5]);
                float right_inv_sqrt =
                    sqrtf_store(1.0F - right_vol, &sqrt_tmp[4]);
                float tmp2 = left_inv_sqrt * right_inv_sqrt;
                float pitch1 = lbPowf(2.0F, v->x20 / 1200.0F);
                float pitch2 = lbPowf(2.0F, v->fadetime / 1200.0F);

                v->vID = HSD_SynthSFXPlayWithGroup(
                    v->sample_id, v->x1A, v->volume,
                    (v->flags & 0x20000) ? v->pan : v->x1C, v->pri, v->itdflag,
                    v->track, pitch1, pitch2, left_inv_sqrt * tmp2, left_sqrt,
                    right_sqrt * left_inv_sqrt);

                if (v->vID != -1) {
                    AXDriver_804C5920[v->vID & 0x3F] = v;
                    v->flags &= 0xFFF4FF99;
                    if (v->flags & 0x40000) {
                        v->flags &= ~0x40000;
                    }
                    AXDriver_804D77C8++;
                } else {
                    HSD_ASSERT(0x13B,
                             (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
                    v->flags &= ~SMSTATE_MASK;
                    return;
                }
                break;
            }
            case 0x2:
                HSD_SynthSFXSetPriority(v->vID, v->pri);
                break;
            case 0x4:
                HSD_SynthSFXSetVolumeFade(v->vID, v->x1A, 0);
                break;
            case 0x8:
                HSD_SynthSFXSetUserVol(v->vID, v->x1C);
                break;
            case 0x10:
                break;
            case 0x20:
                HSD_SynthSFXSetPitchRatio(v->vID, 0,
                                          lbPowf(2.0F, v->x20 / 1200.0F));
                break;
            case 0x40:
                HSD_SynthSFXSetPitchRatio(v->vID, 1,
                                          lbPowf(2.0F, v->fadetime / 1200.0F));
                break;
            case 0x80: {
                float left_vol = (v->x26 * v->x24[0]) / 65535.0F;
                float right_vol = (v->x27 * v->x24[1]) / 65535.0F;
                float left_sqrt = sqrtf_store(left_vol, &sqrt_tmp[3]);
                float left_inv_sqrt =
                    sqrtf_store(1.0F - left_vol, &sqrt_tmp[2]);
                float right_sqrt = sqrtf_store(right_vol, &sqrt_tmp[1]);
                float right_inv_sqrt =
                    sqrtf_store(1.0F - right_vol, &sqrt_tmp[0]);

                HSD_SynthSFXSetMix(
                    v->vID, left_inv_sqrt * (left_inv_sqrt * right_inv_sqrt),
                    left_sqrt, right_sqrt * left_inv_sqrt);
                break;
            }
            case 0x100:
                v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_SLEEP;
                return;
            case 0x200:
                if (v != NULL) {
                    tmp(v);
                }
                return;
            }
        }

        v->flags &= ~flag;
    }
}

u32 AXDriver_8038C678(u32 param_type, u32 param_value)
{
    switch (param_type) {
    case 0:
        return param_value & 0xFFFFFF;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 14:
    case 15:
    case 20:
    case 21:
        return false;
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 16:
    case 17:
    case 18:
    case 19:
        return param_value >> 8 & 0xFFFF;
    case 12:
    case 13:
        return param_value >> 16 & 0xFF;
    default:
        return false;
    }
}

#define MIN2(x, y) ((x) < (y) ? (x) : (y))
#define MAX2(x, y) ((x) < (y) ? (y) : (x))
#define CLAMP(min, val, max) MAX2(MIN2(val, max), min)

void AXDriver_8038C6C0(HSD_SM* v)
{
    u32 cmd_type;
    u32 cmd_word;
    int cmd_size;
    int cmd_val;
    PAD_STACK(8);

    while (v->x30 == (s32) AXDriver_804D778C) {
        cmd_word = *v->cmd_stream;
        cmd_type = cmd_word >> 0x18U;

        cmd_size = AXDriver_8038C678(cmd_type, cmd_word);
        if (cmd_size != 0) {
            AXDriver_8038BF6C(v);
        }
        v->x30 += cmd_size;
        switch (cmd_type) {
        case 2:
            v->x2A = *v->cmd_stream;
            if (v->x2A == 0) {
                v->flags |= 0x100000;
            }
            break;
        case 3:
            if ((v->flags & 0x100000) || v->x2A != 0) {
                v->cmd_stream -= *v->cmd_stream & 0xFFFFFF;
                v->x2A--;
            }
            break;
        case 1:
            v->flags |= 1;
            v->sample_id = HSD_SfxIsPackageSound(v->sound_id)
                               ? v->sound_id
                               : (u16)*v->cmd_stream;
            break;
        case 4:
            v->flags |= 2;
            v->pri = *v->cmd_stream;
            break;
        case 5:
            v->flags |= 2;
            cmd_val = v->pri + (s8) (u8) *v->cmd_stream;
            v->pri = CLAMP(5, cmd_val, 0x1C);
            break;
        case 6:
            v->flags |= 4;
            v->x1A = *v->cmd_stream;
            break;
        case 7:
            v->flags |= 4;
            cmd_val = v->x1A + (s8) (u8) *v->cmd_stream;
            v->x1A = CLAMP(0, cmd_val, 0xFF);
            break;
        case 8:
            v->flags |= 8;
            v->x1C = *v->cmd_stream;
            break;
        case 9:
            v->flags |= 8;
            cmd_val = v->x1C + (s8) (u8) *v->cmd_stream;
            v->x1C = CLAMP(0, cmd_val, 0xFF);
            break;
        case 10:
            v->flags |= 0x10;
            v->x1E = *v->cmd_stream;
            break;
        case 11:
            v->flags |= 0x10;
            cmd_val = v->x1E + (s8) (u8) *v->cmd_stream;
            v->x1E = CLAMP(0, cmd_val, 0xFF);
            break;
        case 12:
            v->flags |= 0x20;
            v->x20 = (s16) (u16) *v->cmd_stream;
            break;
        case 13:
            v->flags |= 0x20;
            cmd_val = v->x20 + (s16) (u16) *v->cmd_stream;
            v->x20 = CLAMP(-0x2A30, cmd_val, 0x960);
            break;
        case 16:
            if (!(AXDriver_804D603C & 1)) {
                v->flags |= 0x80;
                v->x24[0] = *v->cmd_stream;
            }
            break;
        case 20:
            if (!(AXDriver_804D603C & 1)) {
                v->x26 = *v->cmd_stream;
            }
            break;
        case 21:
            if (!((AXDriver_804D603C >> 1U) & 1)) {
                v->x27 = *v->cmd_stream;
            }
            break;
        case 17:
            if (!(AXDriver_804D603C & 1)) {
                v->flags |= 0x80;
                cmd_val = v->x24[0] + (s8) (u8) *v->cmd_stream;
                v->x24[0] = CLAMP(0, cmd_val, 0xFF);
            }
            break;
        case 18:
            if (!((AXDriver_804D603C >> 1U) & 1)) {
                v->flags |= 0x80;
                v->x24[1] = *v->cmd_stream;
            }
            break;
        case 19:
            if (!(AXDriver_804D603C >> 1 & 1)) {
                v->flags |= 0x80;
                cmd_val = v->x24[1] + (s8) (u8) *v->cmd_stream;
                v->x24[1] = CLAMP(0, cmd_val, 0xFF);
            }
            break;
        case 15:
            v->flags |= 0x200;
            AXDriver_8038BF6C(v);
            return;
        case 14:
            v->flags |= 0x100;
            AXDriver_8038BF6C(v);
            return;
        }
        v->cmd_stream++;
    }
}

static void fn_8038CC1C(void)
{
    HSD_SM* v;
    HSD_SM* next;
    PAD_STACK(4);

    if (AXDriver_804D77E0 != 0) {
        AXDriver_804D77E0 = 0;
        if (AXDriver_804D6038 != -1) {
            float x =
                lbPowf(2.0F, CLAMP(-0x2A30, AXDriver_804D77E4, 0x960) / 1200.0F);
            HSD_SynthSFXSetPitchRatio(AXDriver_804D6038, 1, x);
        }
    }
    v = AXDriver_804D7794;
    AXDriver_804D778C++;
    while (v != NULL) {
        next = v->next;
        if (v->x30 == -1) {
            v->x30 = AXDriver_804D778C;
        } else if (v->flags & 0x20000000) {
            v->x30++;
        }
        switch (v->flags & SMSTATE_MASK) {
        case SMSTATE_ACTIVE:
            if (v->x30 == AXDriver_804D778C) {
                AXDriver_8038C6C0(v);
            }
            break;
        case 0:
            AXDriverUnlink(v, &AXDriver_804D7794);
            unk_inline(v, &AXDriver_804D7790);
            AXDriver_804D77D0--;
            break;
        case SMSTATE_SLEEP:
            if (v->flags & 0x40) {
                float x = lbPowf(2.0F, v->fadetime / 1200.0F);
                HSD_SynthSFXSetPitchRatio(v->vID, 1, x);
                v->flags &= 0xFFFFFFBF;
            }
            break;
        default:
            HSD_ASSERT(0x25F, 0);
            break;
        }
        v = next;
    }
}

static void fn_8038CEA4(s32 vID)
{
    HSD_SM* v;
    int idx = vID & 0x3F;

    HSD_ASSERT(0x26D, vID > 0);

    v = AXDriver_804C5920[idx];
    if (v == NULL || v->vID != vID) {
        return;
    }

    AXDriver_804C5920[idx] = NULL;
    v->flags &= ~0x20000000;
    v->flags &= ~SMSTATE_MASK;
    AXDriver_804D77C8--;
}

static void fn_8038CF48(s32 vID)
{
    HSD_SM* v;
    int idx;
    idx = vID & 0x3F;

    HSD_ASSERT(0x288, vID > 0);

    v = AXDriver_804C5920[idx];
    HSD_ASSERT(0x28A, v && v->vID == vID);

    v->flags |= 0x20000000;
}

static inline HSD_SM* AXDriver_8038CFF4_inline(void)
{
    if (AXDriver_804D7790 == NULL) {
        return NULL;
    } else {
        HSD_SM* v;
        v = AXDriver_804D7790;
        AXDriverUnlink(v, &AXDriver_804D7790);
        v->vID = -1;
        v->x30 = -1;
        v->flags = 0;
        v->flags &= ~SMSTATE_MASK;
        return v;
    }
}

static int AXDriverStartSound(int sound_id, u8 volume, u8 pan, int track,
                              int channel)
{
    HSD_SM* v;
    u32* commands = NULL;
    bool package_sound = HSD_SfxIsPackageSound(sound_id);
    if (package_sound) {
        if (!HSD_SynthHasPackageSound(sound_id)) {
            return -1;
        }
    } else {
        int bank_idx = sound_id / 10000;
        int bank_mem = sound_id % 10000;
        if (sound_id < 0 || bank_idx >= sfx_commands.bank_count) {
            return -1;
        }
        int sample_idx = bank_mem + sfx_commands.banks[bank_idx];
        if (sample_idx >= sfx_commands.stream_count ||
            (bank_idx < sfx_commands.bank_count - 1 &&
             sample_idx >= sfx_commands.banks[bank_idx + 1]))
        {
            return -1;
        }
        commands = sfx_commands.words + sfx_commands.streams[sample_idx] / 4;
    }

    if (track < 0 || track > 0xFF) {
        return -1;
    }

    if (channel < 0 || channel >= 0x10) {
        return -1;
    }

    if (AXDriver_804D77CC & (1 << channel)) {
        return -1;
    }

    v = AXDriver_8038CFF4_inline();

    if (v == NULL) {
        return -1;
    }

    v->sound_id = sound_id;
    if (package_sound) {
        // Start the sample, then retain the driver handle until the voice ends.
        v->package_commands[0] = 0x01000000 | sound_id;
        v->package_commands[1] = 0x0E000000;
        commands = v->package_commands;
    }
    v->cmd_stream = commands;
    v->x1A = 0xFF;
    v->volume = volume;
    v->x1C = 0x80;
    v->pan = pan;
    v->x20 = 0;
    v->fadetime = 0;
    v->x24[0] = AXDriver_804C5A20[0][channel];
    v->x24[1] = AXDriver_804C5A20[1][channel];
    v->x26 = 0xFF;
    v->x27 = 0xFF;
    v->pri = 5;
    v->track = track;
    v->itdflag = channel;
    v->flags |= 0x30000;

    if (vidhigh & 0xFE000000) {
        OSReport("vidhigh exceeds the max value\n");
        HSD_ASSERT(0x2EA, 0);
    }

    v->unk =
        (vidhigh << 7) | ((u8*) v - (u8*) AXDriver_804C45A0) / sizeof(HSD_SM);
    vidhigh++;

    v->flags = (v->flags & ~SMSTATE_MASK) | SMSTATE_ACTIVE;
    unk_inline(v, &AXDriver_804D7794);
    AXDriver_804D77D0++;

    return v->unk;
}

int AXDriver_8038CFF4(int sound_id, u8 volume, u8 pan, int track, int channel)
{
    int instance;
    if (HSD_AudioRollbackReuseStart(sound_id, track, channel,
                                    &instance))
    {
        // Position-dependent mix changes do not make this a different event.
        AXDriver_8038D2B4(instance, pan);
        AXDriver_8038D3B8(instance, volume);
        return instance;
    }
    instance = AXDriverStartSound(sound_id, volume, pan, track, channel);
    HSD_AudioRollbackRecordStart(sound_id, track, channel,
                                 instance);
    return instance;
}

bool AXDriver_8038D2B4(int vid, u8 pan)
{
    int idx;
    HSD_SM* v;
    u8 clamped;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    if (v->vID != -1) {
        HSD_SynthSFXSetUserVol(v->vID, MIN(pan, 0xFF));
    } else {
        HSD_ASSERT(0x30B, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->pan = pan;
        v->flags |= 0x20000;
    }
    return true;
}

bool AXDriver_8038D3B8(s32 vid, u8 volume)
{
    HSD_SM* v;
    s32 idx;
    s32 voice_id;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    voice_id = v->vID;
    if (voice_id != -1) {
        HSD_SynthSFXSetVolumeFade(voice_id, CLAMP(0, volume, 0xFF), 1);
    } else {
        HSD_ASSERT(0x34D, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->volume = volume;
        v->flags |= 0x10000;
    }
    return true;
}

bool AXDriver_8038D4E4(s32 vid, s16 pitch)
{
    HSD_SM* v;
    s32 idx;
    int clamped;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    clamped = CLAMP(-0x2A30, pitch, 0x960);
    v->fadetime = clamped;
    v->flags |= 0x40;
    return true;
}

bool AXDriver_8038D5B4(s32 vid, s32 aux_bus, u8 send_level)
{
    HSD_SM* v;
    float right_vol;
    float left_vol;
    float left_inv_sqrt;
    float left_sqrt;
    float right_sqrt;
    float right_inv_sqrt;
    int idx;
    int lock_flag;
    int unused;
    int clamped;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    if (aux_bus < 0 || aux_bus > 1) {
        return false;
    }
    if (aux_bus == 0) {
        lock_flag = AXDriver_804D603C & 1;
    } else {
        lock_flag = (AXDriver_804D603C >> 1) & 1;
    }
    if (lock_flag != 1) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    clamped = CLAMP(0, send_level, 0xFF);
    if (v->vID != -1) {
        v->x24[aux_bus] = clamped;
        left_vol = (f32) (v->x26 * v->x24[0]) / 65535.0F;
        right_vol = (f32) (v->x27 * v->x24[1]) / 65535.0F;
        left_sqrt = sqrtf(left_vol);
        left_inv_sqrt = sqrtf(1.0F - left_vol);
        right_sqrt = sqrtf(right_vol);
        right_inv_sqrt = sqrtf(1.0F - right_vol);
        HSD_SynthSFXSetMix(v->vID,
                           left_inv_sqrt * (left_inv_sqrt * right_inv_sqrt),
                           left_sqrt, right_sqrt * left_inv_sqrt);
    } else {
        HSD_ASSERT(0x3AB, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->x24[aux_bus] = clamped;
        v->flags |= 0x80000;
    }
    return true;
}

bool AXDriver_8038D914(s32 channel, s32 aux_bus, s8 send_level)
{
    HSD_SM* v;

    if (channel < 0 || channel >= 0x10) {
        return false;
    }
    if (aux_bus < 0 || aux_bus > 1) {
        return false;
    }
    v = AXDriver_804D7794;
    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->itdflag == channel) {
            AXDriver_8038D5B4(v->unk, aux_bus, (u8) send_level);
        }
        v = v->next;
    }
    AXDriver_804C5A20[aux_bus][channel] = send_level;
    return true;
}

bool AXDriver_8038D9D8(int vid)
{
    HSD_SM* v;
    int idx;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    if (HSD_SynthSFXCheck(v->vID) == -1) {
        return false;
    }
    return true;
}

// This direct DVD read completes on Aurora's worker. Only its result crosses
// threads; command parsing and all synth state remain on the game thread.
static _Atomic(s32) command_read_result = INT_MIN;

static void fn_8038DA5C(s32 result, DVDFileInfo* fileInfo)
{
    atomic_store_explicit(&command_read_result, result, memory_order_release);
}

void AXDriver_8038DA70(const char* path, void (*callback)(void))
{
    DVDFileInfo fileInfo;
    s32 entrynum;
    s32 alignedSize;

    entrynum = DVDConvertPathToEntrynum(path);
    if (entrynum == -1 || DVDFastOpen(entrynum, &fileInfo) == 0) {
        OSReport("can not open %s\n", path);
        return;
    }

    AXDriver_804D779C = fileInfo.length;
    if (AXDriver_804D779C == 0) {
        OSReport("file size of \"%s\" is 0\n", path);
        DVDClose(&fileInfo);
        return;
    }

    alignedSize = (AXDriver_804D779C + 0x1F) & ~0x1F;
    AXDriver_804D7798 = HSD_AudioMalloc(alignedSize);
    atomic_store_explicit(&command_read_result, INT_MIN, memory_order_relaxed);
    s32 result;
    if (DVDReadAsyncPrio(&fileInfo, AXDriver_804D7798, alignedSize, 0,
                         fn_8038DA5C, 2)) {
        while ((result = atomic_load_explicit(&command_read_result,
                                              memory_order_acquire)) == INT_MIN) {
            callback();
        }
    } else {
        result = -1;
    }
    DVDClose(&fileInfo);
    if (result < 0 || (u32) result < AXDriver_804D779C) {
        OSReport("audio command file read failed: %s (%ld)\n", path, result);
        HSD_AudioFree(AXDriver_804D7798);
        AXDriver_804D7798 = NULL;
        return;
    }

    HSD_ASSERTREPORT(0, HSD_SfxCommandsDecode(&sfx_commands,
        AXDriver_804D7798, AXDriver_804D779C),
        "invalid audio command file: %s\n", path);
}

void AXDriver_8038DCFC(void)
{
    if (AXDriver_804D7798 != NULL) {
        HSD_AudioFree(AXDriver_804D7798);
    }
    AXDriver_804D7798 = NULL;
    sfx_commands = (HSD_SfxCommands) { 0 };
}

void AXDriver_8038E498(int voices, int priority, int sample_rate,
                       int aram_size)
{
    int i;

    for (i = 0; i < 0x60; i++) {
        AXDriver_804C45A0[i].flags &= ~SMSTATE_MASK;
        unk_inline(&AXDriver_804C45A0[i], &AXDriver_804D7790);
    }

    HSD_SynthInit(voices, priority, sample_rate, aram_size);
    HSD_SynthSFXSetDriverMasterClockCallback(fn_8038CC1C);
    HSD_SynthSFXSetDriverInactivatedCallback(fn_8038CEA4);
    HSD_SynthSFXSetDriverPauseCallback(fn_8038CF48);

    HSD_AudioAuxReset();
}

int AXDriver_8038E5D4(void)
{
    return AXDriver_804D77C8;
}

int AXDriver_8038E5DC(void)
{
    return AXDriver_804D77D0;
}

static bool AXDriver_8038E5E4(int vid)
{
    int idx;
    HSD_SM* v;

    idx = vid & 0x7F;
    if (vid < 0 || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    if (v->vID != -1) {
        HSD_SynthSFXPause(v->vID);
    } else {
        HSD_ASSERT(0x5D6, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
        v->flags |= 0x20000000;
    }
    return true;
}

bool AXDriver_8038E6C0(int channel)
{
    HSD_SM* v;

    if (channel < 0 || channel >= 0x10) {
        return false;
    }
    v = AXDriver_804D7794;
    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->itdflag == channel) {
            AXDriver_8038E5E4(v->unk);
        }
        v = v->next;
    }
    AXDriver_804D77CC |= 1 << channel;
    return true;
}

static bool AXDriver_8038E768(int vid)
{
    int idx;
    HSD_SM* v;

    idx = vid & 0x7F;
    if ((vid < 0) || idx >= 0x60) {
        return false;
    }
    v = &AXDriver_804C45A0[idx];
    if (v->unk != vid || !(v->flags & SMSTATE_MASK)) {
        return false;
    }
    if (v->vID != -1) {
        HSD_SynthSFXResume(v->vID);
    } else {
        HSD_ASSERT(0x619, (v->flags&SMSTATE_MASK) == SMSTATE_ACTIVE);
    }
    v->flags &= 0xDFFFFFFF;
    return true;
}

bool AXDriver_8038E844(int channel)
{
    HSD_SM* v;

    if ((channel < 0) || (channel >= 0x10)) {
        return false;
    }
    v = AXDriver_804D7794;
    while (v != NULL) {
        if ((v->flags & SMSTATE_MASK) && v->itdflag == channel) {
            AXDriver_8038E768(v->unk);
        }
        v = v->next;
    }
    AXDriver_804D77CC &= ~(1 << channel);
    return true;
}

bool AXDriver_8038E8EC(const char* path, u8 volume, int track)
{
    int entrynum = DVDConvertPathToEntrynum(path);
    if (AXDriver_804D6038 != -1) {
        HSD_SynthSFXKeyOff(AXDriver_804D6038);
    }
    AXDriver_804D6038 = HSD_Synth_8038B5AC(entrynum, -1, volume, track);
    AXDriver_804D77E8 = AXDriver_804D778C;
    return true;
}

bool AXDriverStop(void)
{
    if (AXDriver_804D6038 == -1) {
        return false;
    }
    HSD_SynthSFXKeyOff(AXDriver_804D6038);
    AXDriver_804D6038 = -1;
    return true;
}

bool AXDriverPause(void)
{
    if (AXDriver_804D6038 == -1) {
        return false;
    }
    HSD_SynthSFXPause(AXDriver_804D6038);
    return true;
}

bool AXDriverResume(void)
{
    if (AXDriver_804D6038 == -1) {
        return false;
    }
    HSD_SynthSFXResume(AXDriver_804D6038);
    return true;
}

bool AXDriver_8038EA18(void)
{
    if (HSD_SynthSFXCheck(AXDriver_804D6038) == -1) {
        return false;
    }
    return true;
}
