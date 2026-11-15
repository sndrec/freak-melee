#include "synth.h"

#include <math.h> // IWYU pragma: keep
#include <placeholder.h>
#include <string.h>

#include "debug.h"
#include "devcom.h"
#include "synth.static.h"
#include <dolphin/ar.h>
#include <dolphin/os.h>

/* 389334 */ static int HSD_Synth_80389334(int sfx_id, u8 vol, u8 vol2, u8 pan,
                                           int priority, int itd_flag,
                                           float pitch1, float pitch2,
                                           float mix_main, float mix_auxA,
                                           float mix_auxB);

void* HSD_AudioMalloc(size_t size)
{
    void* p = OSAllocFromHeap(HSD_Synth_804D6018, size);
    HSD_ASSERTREPORT(0x29U, p, "audio heap overflow.\n");
    return p;
}

void HSD_AudioFree(void* ptr)
{
    OSFreeToHeap(HSD_Synth_804D6018, ptr);
}

static int sfxLoadRequest;
static float HSD_Synth_804D6030 = 1.0f;

static void HSD_SynthSFXSampleLoadCallback(int result, void* context, void* addr,
                                           bool cancelflag)
{
    s32 i;

    if (HSD_Synth_804D7738 == 0) {
        int bankID = HSD_Synth_804C2A60[0].bankID;
        u32 count = hsd_SynthSFXLoadBuf[2];
        HSD_SfxBank* bank = HSD_AudioMalloc(sizeof(*bank) +
                                            count * sizeof(HSD_SfxEntry));
        bank->next = NULL;
        bank->entrynum = HSD_Synth_804C2A60[0].entrynum;
        bank->first_id = hsd_SynthSFXLoadBuf[3];
        bank->count = count;
        bank->aram_size = hsd_SynthSFXLoadBuf[1];
        HSD_ASSERT(0, HSD_SfxBankDecode(bank, HSD_Synth_804D7730,
            hsd_SynthSFXLoadBuf[0], hsd_SynthSFXBank[bankID],
            HSD_Synth_804D7784));
        HSD_AudioFree(HSD_Synth_804D7730);
        HSD_Synth_804D7730 = NULL;
        HSD_SfxBank** tail = &HSD_Synth_804C2AE0[bankID];
        while (*tail != NULL) {
            tail = &(*tail)->next;
        }
        *tail = bank;
        for (u32 j = 0; j < count; ++j) {
            HSD_SfxEntry* entry = &bank->entries[j];
            HSD_SfxEntry** bucket = &HSD_Synth_804C29E0[entry->id & 0x1F];
            entry->next = *bucket;
            *bucket = entry;
        }
        if (HSD_Synth_804C2A60[0].x8 != NULL) {
            HSD_Synth_804C2A60[0].x8(HSD_Synth_804C2A60[0].entrynum,
                                     HSD_Synth_804C2A60[0].xC);
        }
        hsd_SynthSFXBank[bankID] += hsd_SynthSFXLoadBuf[1];
    } else {
        if (HSD_Synth_804D7730 != NULL) {
            HSD_AudioFree(HSD_Synth_804D7730);
            HSD_Synth_804D7730 = NULL;
        }
        HSD_Synth_804D7738 = 0;
    }
    HSD_Synth_804D772C -= 1;
    for (i = 0; i < HSD_Synth_804D772C; i++) {
        HSD_Synth_804C2A60[i] = HSD_Synth_804C2A60[i + 1];
    }
    HSD_SynthSFXLoadNewProc();
}

static void HSD_SynthSFXMetadataLoadCallback(int request, void* context,
                                             void* buffer, bool canceled)
{
    if (HSD_Synth_804D7738 != 0) {
        HSD_SynthSFXSampleLoadCallback(request, context, buffer, canceled);
        return;
    }
    // Start samples only after metadata has released its destination buffer.
    // Cancellation now owns exactly one in-flight request at every stage.
    sfxLoadRequest = HSD_DevComRequest(
        HSD_Synth_804C2A60[0].entrynum,
        OSRoundUp32B(hsd_SynthSFXLoadBuf[0] + 0x10),
        hsd_SynthSFXBank[HSD_Synth_804C2A60[0].bankID],
        hsd_SynthSFXLoadBuf[1], 0x23, 1,
        HSD_SynthSFXSampleLoadCallback, NULL);
}

static void HSD_SynthSFXHeaderLoadCallback(int result, void* context, void* addr,
                                           bool cancelflag)
{
    if (HSD_Synth_804D7738 == 0) {
        // The initial 0x20-byte read contains four big-endian header words.
        u8* header = (u8*) hsd_SynthSFXLoadBuf;
        for (u32 i = 0; i < 4; ++i) {
            u32 word = ((u32) header[i * 4] << 24) |
                       ((u32) header[i * 4 + 1] << 16) |
                       ((u32) header[i * 4 + 2] << 8) | header[i * 4 + 3];
            hsd_SynthSFXLoadBuf[i] = word;
        }
        u32 header_size = hsd_SynthSFXLoadBuf[0];
        HSD_ASSERT(0, header_size >= 8 && header_size <= 16 * 1024 * 1024);
        HSD_ASSERT(0, hsd_SynthSFXLoadBuf[2] <= header_size / 72);
        HSD_ASSERT(0, hsd_SynthSFXLoadBuf[3] <= 0x7FFFFFFF -
                      hsd_SynthSFXLoadBuf[2]);
        int bankID = HSD_Synth_804C2A60[0].bankID;

        HSD_ASSERTREPORT(0xCD,
                         hsd_SynthSFXBankHead[bankID + 1] -
                                 hsd_SynthSFXBank[bankID] >=
                             hsd_SynthSFXLoadBuf[1],
                         "Can't load SFX file; bank(id=%d) buffer overflow.\n",
                         HSD_Synth_804C2A60[0].bankID);

        HSD_Synth_804D7730 = HSD_AudioMalloc(OSRoundUp32B(header_size));
        sfxLoadRequest = HSD_DevComRequest(
            HSD_Synth_804C2A60[0].entrynum, 0x10, (uintptr_t) HSD_Synth_804D7730,
            OSRoundUp32B(header_size), 0x21, 1,
            HSD_SynthSFXMetadataLoadCallback, NULL);
        return;
    }
    HSD_Synth_804D7730 = NULL;
    HSD_SynthSFXSampleLoadCallback(0, 0, NULL, 0);
}

void HSD_SynthSFXLoadNewProc(void)
{
    if (HSD_Synth_804D772C != 0) {
        sfxLoadRequest = HSD_DevComRequest(
            HSD_Synth_804C2A60[0].entrynum, 0, (size_t) hsd_SynthSFXLoadBuf,
            0x20, 0x21, 1, HSD_SynthSFXHeaderLoadCallback, NULL);
    }
}

int HSD_SynthSFXLoad(const char* filename, int bankID, void (*cb)(int, int),
                     int mode)
{
    int entrynum;

    HSD_ASSERTREPORT(0x103, (bankID >= 0 && bankID < hsd_SynthSFXBankNum),
                     "invalid bankID = %d; filename = %s\n", bankID, filename);

    entrynum = DVDConvertPathToEntrynum(filename);

    while (HSD_Synth_804D772C >= 6) {
        HSD_DevComPoll();
    }

    HSD_Synth_804C2A60[HSD_Synth_804D772C].entrynum = entrynum;
    HSD_Synth_804C2A60[HSD_Synth_804D772C].bankID = bankID;
    HSD_Synth_804C2A60[HSD_Synth_804D772C].x8 = cb;
    HSD_Synth_804C2A60[HSD_Synth_804D772C].xC = mode;
    HSD_Synth_804D772C += 1;

    if (HSD_Synth_804D772C == 1) {
        HSD_SynthSFXLoadNewProc();
    }

    return entrynum;
}

void HSD_SynthSFXWaitForLoadCompletion(void (*callback)(void))
{
    while (HSD_Synth_804D772C != 0) {
        HSD_DevComPoll();
        callback();
    }
}

int HSD_SynthSFXGetPendingLoadCount(void)
{
    return HSD_Synth_804D772C - HSD_Synth_804D7738;
}

int HSD_SynthSFXCancelLoad(int entrynum)
{
    int result = 0;

    if (HSD_Synth_804D772C != 0) {
        if (HSD_Synth_804C2A60[0].entrynum == entrynum &&
            HSD_Synth_804D7738 == 0)
        {
            HSD_Synth_804D7738 = 1;
            HSD_DevComCancelEx(sfxLoadRequest, 0, NULL, NULL);
            result = 1;
        } else {
            int idx = 1;
            while (idx < HSD_Synth_804D772C) {
                if (HSD_Synth_804C2A60[idx].entrynum == entrynum) {
                    int j;
                    for (j = idx; j < HSD_Synth_804D772C - 1; j++) {
                        HSD_Synth_804C2A60[j] = HSD_Synth_804C2A60[j + 1];
                    }
                    HSD_Synth_804D772C--;
                    result = 1;
                    break;
                }
                idx++;
            }
        }
    }
    return result;
}

void HSD_SynthSFXAllocateBank(int size)
{
    int base = hsd_SynthSFXBankHead[hsd_SynthSFXBankNum];

    hsd_SynthSFXBank[hsd_SynthSFXBankNum] = base;
    hsd_SynthSFXBankHead[hsd_SynthSFXBankNum + 1] = size + base;

    HSD_ASSERTREPORT(0x158,
                     hsd_SynthSFXBankHead[hsd_SynthSFXBankNum + 1] <=
                         hsd_SynthSFXBankAREnd,
                     "bank overflow\n");
    hsd_SynthSFXBankNum += 1;
}

#ifdef MUST_MATCH
static void order_data_0(void)
{
    (void) "bank stack underflow\n";
    (void) "hsd_SynthSFXBankNum";
}
#endif

static void HSD_SynthUnlinkBankEntries(HSD_SfxBank* bank)
{
    for (u32 i = 0; i < bank->count; ++i) {
        HSD_SfxEntry* entry = &bank->entries[i];
        HSD_SfxEntry** link = &HSD_Synth_804C29E0[entry->id & 0x1F];
        while (*link != NULL) {
            if (*link == entry) {
                *link = entry->next;
                break;
            }
            link = &(*link)->next;
        }
    }
}

void HSD_SynthSFXUnloadBank(int bank_id)
{
    HSD_SynthSFXStopRange(bank_id);
    HSD_SfxBank** head = &HSD_Synth_804C2AE0[bank_id];
    while (*head != NULL) {
        HSD_SfxBank* bank = *head;
        HSD_SynthUnlinkBankEntries(bank);
        *head = bank->next;
        HSD_AudioFree(bank);
    }
    hsd_SynthSFXBank[bank_id] = hsd_SynthSFXBankHead[bank_id];
}

void HSD_Synth_80388E08(int entrynum)
{
    for (u32 i = 0; i < ARRAY_SIZE(HSD_Synth_804C2AE0); ++i) {
        HSD_SfxBank** link = &HSD_Synth_804C2AE0[i];
        while (*link != NULL) {
            HSD_SfxBank* bank = *link;
            if (bank->entrynum == entrynum) {
                HSD_SynthUnlinkBankEntries(bank);
                *link = bank->next;
                HSD_AudioFree(bank);
                return;
            }
            link = &bank->next;
        }
    }
}

static void HSD_SynthSFXGroupDataReaddressCallback(int result, void* context,
                                                   void* addr, bool cancelflag)
{
    HSD_ASSERT(0x182, sfxGroupDataReaddressCounter > 0);
    sfxGroupDataReaddressCounter--;
}

#ifdef MUST_MATCH
static void order_data_1(void)
{
    (void) "Can't relocate SFX group; bank = %d; sfxgroup = %d\n";
    (void) "hsd_SynthSFXBank[bankID] + group->arsize <= "
           "hsd_SynthSFXBankHead[bankID + 1]";
}
#endif

void HSD_SynthSFXBankDeflag(int bank_id)
{
    HSD_SynthSFXStopRange(bank_id);
    u32 offset = hsd_SynthSFXBankHead[bank_id];
    for (HSD_SfxBank* bank = HSD_Synth_804C2AE0[bank_id]; bank != NULL;
         bank = bank->next) {
        if (bank->aram_base != offset) {
            sfxGroupDataReaddressCounter++;
            HSD_DevComRequest(0, bank->aram_base, offset, bank->aram_size,
                0x1B, 0, HSD_SynthSFXGroupDataReaddressCallback, NULL);
            HSD_SfxBankRelocate(bank, offset, HSD_Synth_804D7784);
        }
        offset += bank->aram_size;
    }
    hsd_SynthSFXBank[bank_id] = offset;
}

void HSD_SynthSFXBankDeflagSync(void)
{
    while (sfxGroupDataReaddressCounter) {
        HSD_DevComPoll();
        continue;
    }
}

u32 HSD_SynthGetSoundMode(void)
{
    return HSD_Synth_804D7754;
}

void HSD_SynthSetSoundMode(int mode)
{
    int i;
    HSD_Synth_804D7754 = mode;
    for (i = 0; i < (int) ARRAY_SIZE(hsd_SynthSFXNodes); i++) {
        if (hsd_SynthSFXNodes[i].x0 > 0) {
            HSD_SynthSFXUpdateMix(&hsd_SynthSFXNodes[i], 1);
        }
    }
}

void HSD_SynthSFXStopNode(struct HSD_SynthSFXNode* node)
{
    int i;
    PAD_STACK(0x10);

    if (!(node->flags & 1) && node->x27 == 1 &&
        driverInactivatedCallback != NULL)
    {
        driverInactivatedCallback(node->x0);
    }
    for (i = 0; i < node->voice_count; i++) {
        HSD_AudioVoiceRelease(node->voice[i]);
        hsd_SynthSFXNodes[node->voice[i]->index].x0 = 0;
    }
}

void dropcallback(void* dropped)
{
    HSD_AudioVoice* voice = dropped;
    struct HSD_SynthSFXNode* node;
    int i;

    PAD_STACK(0x10);

    node = &hsd_SynthSFXNodes[voice->index];

    /// Search HSD_Synth_804C28E0 queue for this voice and remove it
    for (i = 0; i < HSD_Synth_804D7720; i++) {
        if (HSD_Synth_804C28E0[i] == voice) {
            --HSD_Synth_804D7720;
            memmove(&HSD_Synth_804C28E0[i], &HSD_Synth_804C28E0[i + 1],
                    (HSD_Synth_804D7720 - i) * sizeof(HSD_Synth_804C28E0[0]));
            break;
        }
    }

    if (node->x0 == 0) {
        return;
    }

    if (node->x0 == -1) {
        /// Secondary voice - follow to primary node
        node = &hsd_SynthSFXNodes[node->voice[0]->index];
    }

    if (!(node->flags & 1) && node->x27 == 1 &&
        driverInactivatedCallback != NULL)
    {
        driverInactivatedCallback(node->x0);
    }

    for (i = 0; i < node->voice_count; i++) {
        HSD_AudioVoice* v = node->voice[i];
        if (v != voice) {
            HSD_ASSERT(0, HSD_Synth_804D7720 < ARRAY_SIZE(HSD_Synth_804C28E0));
            HSD_Synth_804C28E0[HSD_Synth_804D7720++] = v;
        }
        hsd_SynthSFXNodes[node->voice[i]->index].x0 = 0;
    }

}

static AXPBMIX lbl_80407FB4 = { 0 };

int HSD_Synth_80389334(int sfx_id, u8 vol, u8 vol2, u8 pan, int priority,
                       int itd_flag, float pitch1, float pitch2,
                       float mix_main, float mix_auxA, float mix_auxB)
{
    HSD_AudioVoice* voices[2] = { NULL, NULL };
    UNUSED u8 stack_pad[8];
    AXPBVE ve;
    float vol_norm;
    float vol2_norm;
    int voice_idx;
    u32 node_idx;
    HSD_SfxEntry* sfx_entry;
    struct HSD_SynthSFXNode* sfx_node;

    PAD_STACK(0x14);

    sfx_entry = HSD_Synth_804C29E0[sfx_id & 0x1F];

    while (sfx_entry != NULL) {
        if (sfx_entry->id == sfx_id) {
            voice_idx = 0;
            while (voice_idx < sfx_entry->voice_count) {
                voices[voice_idx] =
                    HSD_AudioVoiceAcquire(priority + 1, dropcallback);
                if (voices[voice_idx] == NULL) {
                    if (voices[0] != NULL) {
                        HSD_AudioVoiceRelease(voices[0]);
                    }
                    return -1;
                }
                voice_idx += 1;
            }
            if ((sfx_entry->voice_count == 2) && (voices[0] == voices[1])) {
                HSD_AudioVoiceRelease(voices[0]);
                return -1;
            }
            if (sfx_entry->voice_count == 2) {
                hsd_SynthSFXNodes[voices[1]->index].x0 = -1;
                hsd_SynthSFXNodes[voices[1]->index].voice[0] = voices[0];
            }

            node_idx = voices[0]->index;

            sfx_node = &hsd_SynthSFXNodes[node_idx];
            sfx_node->x27 = 1;
            sfx_node->sfx_id = sfx_id;
            sfx_node->flags = 0;
            sfx_node->voice_count = sfx_entry->voice_count;
            sfx_node->xB = itd_flag;
            sfx_node->voice[0] = voices[0];
            sfx_node->voice[1] = voices[1];
            sfx_node->x14 = 0.00003125F * sfx_entry->sample_rate;
            sfx_node->x18[0] = pitch1;
            sfx_node->x18[1] = pitch2;
            vol_norm = 1 / 255.0F * vol;
            vol2_norm = 1 / 255.0F * vol2;
            sfx_node->unk28 = vol_norm;
            sfx_node->user_vol[0].volume = vol_norm;
            sfx_node->user_vol[0].x4 = 0;
            sfx_node->user_vol[0].x8_float = vol2_norm;
            sfx_node->user_vol[1].volume = vol2_norm;
            sfx_node->user_vol[1].x4 = 0;
            sfx_node->user_vol[1].x8 = pan;
            sfx_node->x44 = mix_main;
            sfx_node->x48 = mix_auxA;
            sfx_node->x4C = mix_auxB;
            HSD_SynthSFXUpdateVolume(sfx_node);
            HSD_SynthSFXUpdateMix(sfx_node, 0);
            ve.currentVolume =
                (32767.0F * (sfx_node->user_vol[0].x8_float *
                             (sfx_node->unk28 *
                              (HSD_Synth_804D6030 *
                               HSD_Synth_804C28E0_1784[sfx_node->xB].x1784))));
            ve.currentDelta = 0;
            sfx_node->x24 = ve.currentVolume;

            voice_idx = 0;
            while (voice_idx < sfx_entry->voice_count) {
                HSD_AudioVoicePriority(voices[voice_idx], priority);
                voices[voice_idx]->envelope = ve;
                u32 ratio = (u32) (65536.0F *
                    (sfx_node->x18[1] * (sfx_node->x14 * sfx_node->x18[0])));
                AXPBSRC source = { .ratioHi = ratio >> 16,
                                     .ratioLo = (u16) ratio };
                voices[voice_idx]->source = source;
                voices[voice_idx]->sample.address = sfx_entry->voices[voice_idx].address;
                voices[voice_idx]->sample.adpcm = sfx_entry->voices[voice_idx].adpcm;
                voices[voice_idx]->sample.loop = sfx_entry->voices[voice_idx].loop;
                HSD_ASSERT(0, HSD_AudioVoiceStart(voices[voice_idx]));
                voice_idx += 1;
            }
            HSD_Synth_804D7750 += 0x40;
            if (HSD_Synth_804D7750 < 0) {
                HSD_Synth_804D7750 = 0x40;
            }
            sfx_node->x0 = HSD_Synth_804D7750 + node_idx;
            return sfx_node->x0;
        }
        sfx_entry = sfx_entry->next;
    }

    return -1;
}

static inline struct HSD_SynthSFXNode* getNode(int sfx_id)
{
    struct HSD_SynthSFXNode* node = &hsd_SynthSFXNodes[sfx_id & 0x3F];
    if (sfx_id > 0 && node->x0 == sfx_id) {
        return node;
    } else {
        return NULL;
    }
}

int HSD_SynthSFXPlayWithGroup(int sfx_id, u8 vol, u8 vol2, u8 pan,
                              int priority, int itd_flag, int group,
                              f32 pitch1, f32 pitch2, f32 mix_main,
                              f32 mix_auxA, f32 mix_auxB)
{
    int result;
    int nodeID;
    struct HSD_SynthSFXNode* node;

    PAD_STACK(0x8);

    HSD_ASSERTREPORT(0x30B, group >= 0 && group < HSD_SYNTHSFXGROUP_MAX,
                     "sfx group ID %d out of range.", group);

    if (group != 0) {
        nodeID = HSD_Synth_804C28E0_1844[group];
        if (nodeID > 0) {
            node = getNode(nodeID);
            if (node != NULL && node->flags != 1) {
                if (node->x27 == 1 && driverInactivatedCallback != NULL) {
                    driverInactivatedCallback(node->x0);
                }
                HSD_SynthSFXKeyOff(HSD_Synth_804C28E0_1844[group]);
            }
        }
    }

    result = HSD_Synth_80389334(sfx_id, vol, vol2, pan, priority, itd_flag,
                                pitch1, pitch2, mix_main, mix_auxA, mix_auxB);
    HSD_Synth_804C28E0_1844[group] = result;
    return result;
}

static inline void freeVoices(struct HSD_SynthSFXNode* node)
{
    int j;
    for (j = 0; j < node->voice_count; j++) {
        HSD_AudioVoiceRelease(node->voice[j]);
        hsd_SynthSFXNodes[node->voice[j]->index].x0 = 0;
    }
}

void HSD_SynthSFXKeyOff(int id)
{
    struct HSD_SynthSFXNode* node;
    int i;

    PAD_STACK(0x10);

    if ((node = getNode(id))) {
        if (node->flags & 8) {
            if (!(node->flags & 1) && node->x27 == 1 &&
                driverInactivatedCallback != NULL)
            {
                driverInactivatedCallback(node->x0);
            }
            freeVoices(node);
        } else if (!(node->flags & 1)) {
            node->flags |= 1;
            HSD_SynthSFXUpdateVolume(node);
            for (i = 0; i < node->voice_count; i++) {
                HSD_AudioVoicePriority(node->voice[i], 1U);
            }
        }
    }
}

static inline void stopRange(size_t lo, size_t hi)
{
    size_t addr;
    int i;
    for (i = 0; i < 0x40; i++) {
        struct HSD_SynthSFXNode* node = &hsd_SynthSFXNodes[i];
        if (hsd_SynthSFXNodes[i].x0 > 0) {
            const AXPBADDR* address = &node->voice[0]->sample.address;
            addr = ((u32) address->currentAddressHi << 16) |
                   address->currentAddressLo;
            if (addr >= lo && addr < hi) {
                HSD_SynthSFXStopNode(&hsd_SynthSFXNodes[i]);
            }
        }
    }
}

void HSD_SynthSFXStopRange(int bank_id)
{
    stopRange(hsd_SynthSFXBankHead[bank_id + 0] * 2,
              hsd_SynthSFXBankHead[bank_id + 1] * 2);
}

void HSD_SynthSFXPause(int sfx_id)
{
    struct HSD_SynthSFXNode* node;

    if ((node = getNode(sfx_id))) {
        node->flags |= 2;
        HSD_SynthSFXUpdateVolume(node);
        if (node->flags & 8) {
            node->flags |= 6;
        }
    }
}

void HSD_SynthSFXResume(int sfx_id)
{
    struct HSD_SynthSFXNode* node;

    if ((node = getNode(sfx_id))) {
        if (node->flags & 2) {
            node->flags &= ~6;
            HSD_SynthSFXUpdatePitch(node);
            HSD_SynthSFXUpdateVolume(node);
        }
    }
}

int HSD_SynthSFXCheck(int sfx_id)
{
    struct HSD_SynthSFXNode* node = getNode(sfx_id);
    int i;

    if (node != NULL) {
        if (node->flags & 1) {
            return -1;
        }
        for (i = 0; i < node->voice_count; i++) {
            if (node->voice[i]->running == 0) {
                return -1;
            }
        }
        return sfx_id;
    }
    return -1;
}

void HSD_SynthSFXSetVolumeFade(int sfx_id, u8 vol, int flag)
{
    struct HSD_SynthSFXNode* node;

    if ((node = getNode(sfx_id))) {
        HSD_ASSERT(0x376, flag >= 0 && flag < USERVOL_NUM);
        node->user_vol[flag].volume = 1 / 255.0 * vol;
        node->user_vol[flag].x4 = 1;
        HSD_SynthSFXUpdateVolume(node);
    }
}

void HSD_SynthSFXSetUserVol(int sfx_id, u8 vol)
{
    struct HSD_SynthSFXNode* node;

    if ((node = getNode(sfx_id))) {
        node->user_vol[1].x8 = vol;
        HSD_SynthSFXUpdateMix(node, 1);
    }
}

void HSD_SynthSFXSetMix(int sfx_id, float mix_main, float mix_auxA,
                        float mix_auxB)
{
    int* unused;
    struct HSD_SynthSFXNode* node;

    if ((node = getNode(sfx_id))) {
        node->x44 = mix_main;
        node->x48 = mix_auxA;
        node->x4C = mix_auxB;
        HSD_SynthSFXUpdateMix(node, 1);
    }
}

void HSD_SynthSFXUpdatePitch(struct HSD_SynthSFXNode* node)
{
    float ratio;

    if (node->flags & 4) {
        ratio = 0.0F;
    } else {
        ratio = node->x14 * node->x18[0] * node->x18[1];
    }
    if (!(node->flags & 8)) {
        int i;
        for (i = 0; i < node->voice_count; i++) {
            {
                u32 fixed_ratio = (u32) (65536.0f * ratio);
                if (fixed_ratio > 0x40000) fixed_ratio = 0x40000;
                node->voice[i]->source.ratioHi = fixed_ratio >> 16;
                node->voice[i]->source.ratioLo = (u16) fixed_ratio;
            }
        }
    }
}

void HSD_SynthSFXSetPitchRatio(int sfx_id, int flag, float ratio)
{
    struct HSD_SynthSFXNode* node = getNode(sfx_id);

    if (node != NULL && !(node->flags & 1)) {
        HSD_ASSERT(0x3A7, flag == 0 || flag == 1);

        node->x18[flag] = ratio;

        HSD_SynthSFXUpdatePitch(node);
    }
}

void HSD_SynthSFXSetPriority(int id, int prio)
{
    struct HSD_SynthSFXNode* node = getNode(id);
    int i;

    if (node != NULL && !(node->flags & 1)) {
        for (i = 0; i < node->voice_count; i++) {
            HSD_AudioVoicePriority(node->voice[i], prio);
        }
    }
}

static inline int user_vol_dst_offset(int k)
{
    return k * 3;
}

static inline int user_vol_src_offset(int k)
{
    return k * 3;
}

void HSD_Synth_8038A000(void)
{
    struct HSD_SynthSFXNode* node;
    int i;
    struct HSD_SynthSFXNode** pnode;
    AXPBVE ve;

    if (HSD_Synth_804D7758 != 0) {
        int ch;
        for (ch = 0; ch < 16; ch++) {
            if (HSD_Synth_804D7758 & (1 << ch)) {
                int cnt = HSD_Synth_804C28E0_1784[ch].x178C;
                if (cnt != 0) {
                    f32 cur = HSD_Synth_804C28E0_1784[ch].x1784;
                    HSD_Synth_804C28E0_1784[ch].x1784 =
                        (cur * ((f32) cnt - 1.0f)) / (f32) cnt +
                        HSD_Synth_804C28E0_1784[ch].x1788 / (f32) cnt;
                    HSD_Synth_804C28E0_1784[ch].x178C -= 1;
                }
                if (HSD_Synth_804C28E0_1784[ch].x178C == 0) {
                    HSD_Synth_804C28E0_1784[ch].x1784 =
                        HSD_Synth_804C28E0_1784[ch].x1788;
                    HSD_Synth_804D7758 &= ~(1 << ch);
                }
            }
        }
    }
    pnode = &HSD_Synth_804D774C;
    while (*pnode != NULL) {
        int active = 0;
        s32 vol;
        s32 delta;
        s32 delta_tmp;
        node = *pnode;

        if (node->x0 <= 0) {
            node->volume_update_pending = 0;
            *pnode = node->x20;
            continue;
        }
        {
            int k;
            for (k = 0; k < USERVOL_NUM; k++) {
                if (*(volatile int*) &node->user_vol[k].x4 != 0) {
                    int c = node->user_vol[k].x4;
                    (&node->user_vol[k]
                          .volume)[user_vol_dst_offset(k) - k * 3 - 1] =
                        ((&node->user_vol[k]
                               .volume)[user_vol_src_offset(k) - k * 3 - 1] *
                         ((f32) c - 1.0f)) /
                            (f32) c +
                        node->user_vol[k].volume / (f32) c;
                    node->user_vol[k].x4 -= 1;
                    if (node->user_vol[k].x4 != 0) {
                        active = 1;
                    }
                }
            }
        }
        if (HSD_Synth_804C28E0_1784[node->xB].x178C != 0) {
            active = 1;
        }
        if (!(node->flags & 3)) {
            vol = 32767.0f *
                  (node->user_vol[0].x8_float *
                   (node->unk28 * (HSD_Synth_804D6030 *
                                   HSD_Synth_804C28E0_1784[node->xB].x1784)));
        } else {
            vol = 0;
            active = 0;
        }
        delta_tmp = (vol - node->x24) / 160;
        delta = delta_tmp;
        if (delta_tmp > 0x14) {
            delta = 0x14;
        } else if (delta < -0x14) {
            delta = -0x14;
        }
        for (i = 0; i < node->voice_count; i++) {
            node->voice[i]->envelope.currentDelta = (s16) delta;
        }
        node->x24 += delta * 0xA0;
        if (delta == 0 && (active == 0 || (f32) vol == 0.0f)) {
            node->x24 = (u16) vol;
            ve.currentVolume = vol;
            ve.currentDelta = 0;
            for (i = 0; i < node->voice_count; i++) {
                node->voice[i]->envelope = ve;
            }
            if (active == 0) {
                u8 flags;
                node->volume_update_pending = 0;
                *pnode = node->x20;
                flags = node->flags;
                if (flags & 1) {
                    for (i = 0; i < node->voice_count; i++) {
                        HSD_AudioVoiceRelease(node->voice[i]);
                        hsd_SynthSFXNodes[node->voice[i]->index].x0 = 0;
                    }
                } else if ((flags & 6) == 2) {
                    node->flags = flags | 6;
                    for (i = 0; i < node->voice_count; i++) {
                        HSD_SynthSFXUpdatePitch(node);
                    }
                    if (driverPauseCallback != NULL && node->x27 == 1) {
                        driverPauseCallback(node->x0);
                    }
                }
                continue;
            }
        }
        pnode = &node->x20;
    }
}

void HSD_SynthSFXUpdateVolume(struct HSD_SynthSFXNode* node)
{
    if (!node->volume_update_pending && !(node->flags & 8)) {
        node->volume_update_pending = true;
        node->x20 = HSD_Synth_804D774C;
        HSD_Synth_804D774C = node;
    }
}

static u8 lbl_8040806C[] = {
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x02, 0x02, 0x03, 0x03, 0x04, 0x04,
    0x05, 0x05, 0x06, 0x06, 0x07, 0x07, 0x08, 0x08, 0x09, 0x09, 0x0A, 0x0A,
    0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0E, 0x0F, 0x0F, 0x10, 0x10,
    0x11, 0x11, 0x12, 0x12, 0x13, 0x13, 0x14, 0x14, 0x15, 0x15, 0x16, 0x16,
    0x16, 0x17, 0x17, 0x17, 0x17, 0x18, 0x18, 0x18, 0x18, 0x19, 0x19, 0x19,
    0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1B,
    0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1D,
    0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1E,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
    0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
};

static inline void my_memzero(void* dst_raw, size_t size)
{
    int i;
    int* dst = dst_raw;
    for (i = 0; i < size; i += 4) {
        *dst++ = 0;
    }
}

void HSD_SynthSFXUpdateMix(struct HSD_SynthSFXNode* node, int interpolate)
{
    int i;
    int shiftL;
    int shiftR;

    f32 l;
    f32 r;

    if (HSD_Synth_804D7754 != 0) {
        l = 32767.0F *
            sqrtf_accurate(0.003921569F * (0xFF - node->user_vol[1].x8));
        r = 32767.0F * sqrtf_accurate(0.003921569F * node->user_vol[1].x8);
    } else {
        l = 23169.768F;
        r = 23169.768F;
    }
    if (node->voice_count == 1) {
        lbl_80407FB4.vL = l * node->x44;
        lbl_80407FB4.vR = r * node->x44;
        lbl_80407FB4.vAuxAL = l * node->x48;
        lbl_80407FB4.vAuxAR = r * node->x48;
        lbl_80407FB4.vAuxBL = l * node->x4C;
        lbl_80407FB4.vAuxBR = r * node->x4C;
        HSD_AudioVoiceConfigureMix(node->voice[0], &lbl_80407FB4);
    } else if (HSD_Synth_804D7754 != 0) {
        lbl_80407FB4.vL = l * node->x44;
        lbl_80407FB4.vAuxAL = l * node->x48;
        lbl_80407FB4.vAuxBL = l * node->x4C;
        HSD_AudioVoiceConfigureMix(node->voice[0], &lbl_80407FB4);
        lbl_80407FB4.vAuxBL = 0;
        lbl_80407FB4.vAuxAL = 0;
        lbl_80407FB4.vL = 0;
        lbl_80407FB4.vR = r * node->x44;
        lbl_80407FB4.vAuxAR = r * node->x48;
        lbl_80407FB4.vAuxBR = r * node->x4C;
        HSD_AudioVoiceConfigureMix(node->voice[1], &lbl_80407FB4);
    } else {
        l /= 2;
        lbl_80407FB4.vL = l * node->x44;
        lbl_80407FB4.vR = l * node->x44;
        lbl_80407FB4.vAuxAL = l * node->x48;
        lbl_80407FB4.vAuxAR = l * node->x48;
        lbl_80407FB4.vAuxBL = l * node->x4C;
        lbl_80407FB4.vAuxBR = l * node->x4C;
        HSD_AudioVoiceConfigureMix(node->voice[0], &lbl_80407FB4);
        HSD_AudioVoiceConfigureMix(node->voice[1], &lbl_80407FB4);
    }

    my_memzero(&lbl_80407FB4, sizeof(lbl_80407FB4));

    shiftL = 0;
    shiftR = 0;
    if (HSD_Synth_804D7754 != 0) {
        if (node->user_vol[1].x8 < 0x80) {
            shiftL = lbl_8040806C[0x7F - node->user_vol[1].x8];
        } else {
            shiftR = lbl_8040806C[node->user_vol[1].x8 - 0x80];
        }
    }
    for (i = 0; i < node->voice_count; i++) {
        if (shiftL != 0 || shiftR != 0 || node->voice[0]->itd.flag == 1) {
            if (node->voice[i]->itd.flag == 0)
            {
                node->voice[i]->itd.flag = 1;
                if (interpolate == 0) {
                    node->voice[i]->itd.shiftR = shiftR;
                    node->voice[i]->itd.shiftL = shiftL;
                }
                node->voice[i]->itd.targetShiftR = shiftR;
                node->voice[i]->itd.targetShiftL = shiftL;
            } else {
                node->voice[i]->itd.targetShiftL = shiftL;
                node->voice[i]->itd.targetShiftR = shiftR;
            }
        }
    }
}

static inline void updateAllVolume(u32 mask)
{
    int i;
    HSD_Synth_804D7758 |= mask;
    for (i = 0; i < 0x40; i++) {
        struct HSD_SynthSFXNode* node = &hsd_SynthSFXNodes[i];
        if (node->x0 > 0 && ((1 << node->xB) & mask)) {
            HSD_SynthSFXUpdateVolume(node);
        }
    }
}

void HSD_SynthSFXUpdateAllVolume(int vol, u16 fade_frames, int channel)
{
    HSD_Synth_804C28E0_1784[channel].x1788 = 1 / 255.0F * vol;
    if (HSD_Synth_804C28E0_1784[channel].x1784 !=
        HSD_Synth_804C28E0_1784[channel].x1788)
    {
        HSD_Synth_804C28E0_1784[channel].x178C = fade_frames;
        updateAllVolume(1 << channel);
    }
}

void HSD_SynthSFXSetDriverInactivatedCallback(UNK_T callback)
{
    driverInactivatedCallback = callback;
}

void HSD_SynthSFXSetDriverMasterClockCallback(UNK_T callback)
{
    driverMasterClockCallback = callback;
}

void HSD_SynthSFXSetDriverPauseCallback(void (*callback)(s32))
{
    driverPauseCallback = callback;
}

// The native audio scheduler must invoke this on the game thread.
void HSD_SynthCallback(void)
{
    int i;
    int j;
    int base;
    PAD_STACK(0x8);

    base = HSD_Synth_804D775C % 8 * 8;

    while (HSD_Synth_804D7720-- > 0) {
        if (HSD_Synth_804C28E0[HSD_Synth_804D7720] != 0) {
            HSD_AudioVoiceRelease(HSD_Synth_804C28E0[HSD_Synth_804D7720]);
        }
    }
    HSD_Synth_804D7720 = 0;

    for (i = 0; i < 8; i++) {
        struct HSD_SynthSFXNode* node = &hsd_SynthSFXNodes[base + i];
        if (node->x0 > 0) {
            if (!(node->flags & 8) && node->voice[0]->running == 0 &&
                (node->voice_count == 1 || node->voice[1]->running == 0))
            {
                HSD_SynthSFXStopNode(node);
            }
        }
    }

    HSD_Synth_8038A000();

    if (driverMasterClockCallback != NULL) {
        driverMasterClockCallback(HSD_Synth_804D775C);
    }

    HSD_Synth_8038ADD0();
    HSD_Synth_804D775C++;
}

void HSD_SynthResetStreamCounters(int result, void* context, void* buf, bool b)
{
    HSD_Synth_804D776C = HSD_Synth_804D7768;
    HSD_Synth_804D7778 = 0;
}

void HSD_Synth_8038AD74(int request, void* context, void* buffer, bool canceled)
{
    HSD_ASSERT(0, HSD_StreamDecodeChunk(&streamChunks[HSD_Synth_804D7768],
                                       streamChunkBytes));
    uintptr_t src = (uintptr_t) context;
    HSD_DevComRequest(HSD_Synth_804D7764, src,
                      HSD_Synth_804D7780 + (HSD_Synth_804D7768 << 16),
                      streamChunks[HSD_Synth_804D7768].byte_size, 0x23, 0,
                      HSD_SynthResetStreamCounters, 0);
}

static inline void HSD_Synth_8038ADD0_inline(u32 pos)
{
    s32 src;

    if ((s32) ((HSD_Synth_804D776C + 1) % 3) != pos) {
        if (HSD_Synth_804D7778 != 0 ||
            HSD_Synth_804D7768 != HSD_Synth_804D776C)
        {
            return;
        }
        if (getNode(HSD_Synth_804D7760) != NULL) {
            src = streamChunks[HSD_Synth_804D776C].next_offset;
            if (src == -1) {
                HSD_Synth_804D776C = (HSD_Synth_804D776C + 1) % 3;
            } else {
                HSD_Synth_804D7768 = (HSD_Synth_804D776C + 1) % 3;
                HSD_Synth_804D7778 = 1;
                HSD_DevComRequest(
                    HSD_Synth_804D7764, src,
                    (uintptr_t) streamChunkBytes, 0x20, 0x21,
                    0, HSD_Synth_8038AD74,
                    (void*) (uintptr_t) ((u32) src + 0x20));
            }
        }
    }
}

void HSD_Synth_8038ADD0(void)
{
    struct HSD_SynthSFXNode* node = getNode(HSD_Synth_804D7760);
    u32 pos;
    s32 i;

    if (node == NULL) {
        return;
    }
    if (node->flags & 8) {
        return;
    }
    const AXPBADDR* address = &node->voice[0]->sample.address;
    u32 current = ((u32) address->currentAddressHi << 16) |
                  address->currentAddressLo;
    pos = (current - HSD_Synth_804D7780 * 2) >> 17;
    if (pos >= ARRAY_SIZE(streamChunks)) {
        HSD_SynthSFXStopNode(node);
        return;
    }
    if (pos != HSD_Synth_804D7774) {
        HSD_Synth_804D7774 = pos;
        for (i = 0; i < node->voice_count; i++) {
            {
                u32 address = (HSD_Synth_804D7780 + (HSD_Synth_804D7774 << 0x10)) * 2 +
                    i * streamChunks[HSD_Synth_804D7774].byte_size +
                    streamChunks[HSD_Synth_804D7774].end_address;
                node->voice[i]->sample.address.endAddressHi = address >> 16;
                node->voice[i]->sample.address.endAddressLo = (u16) address;
            }
        }
    }
    if (pos == HSD_Synth_804D7770 && pos != HSD_Synth_804D776C) {
        if ((u32) streamChunks[HSD_Synth_804D7770].next_offset == -1U) {
            HSD_Synth_804D7770 = (HSD_Synth_804D7770 + 1) % 3;
            for (i = 0; i < node->voice_count; i++) {
                node->voice[i]->sample.address.loopFlag = 0;
                {
                u32 address = HSD_Synth_804D7784;
                node->voice[i]->sample.address.loopAddressHi = address >> 16;
                node->voice[i]->sample.address.loopAddressLo = (u16) address;
            }
            }
        } else {
            HSD_Synth_804D7770 = (HSD_Synth_804D7770 + 1) % 3;
            for (i = 0; i < node->voice_count; i++) {
                {
                u32 address = (HSD_Synth_804D7780 + (HSD_Synth_804D7770 << 0x10)) * 2 +
                        i * streamChunks[HSD_Synth_804D7770].byte_size + 2;
                node->voice[i]->sample.address.loopAddressHi = address >> 16;
                node->voice[i]->sample.address.loopAddressLo = (u16) address;
            }
                node->voice[i]->sample.loop = streamChunks[HSD_Synth_804D7770].loop[i];
            }
        }
    }
    HSD_Synth_8038ADD0_inline(pos);
}

void HSD_Synth_8038B120(int request, void* context, void* buffer, bool canceled)
{
    AXPBVE ve;
    int i;
    struct HSD_SynthSFXNode* node;

    PAD_STACK(0x10);

    node = getNode(HSD_Synth_804D7760);
    if (node != NULL) {
        if (!(node->flags & 2)) {
            ve.currentVolume =
                (32767.0F *
                 (node->user_vol[0].x8_float *
                  (node->unk28 * (HSD_Synth_804D6030 *
                                  HSD_Synth_804C28E0_1784[node->xB].x1784))));
        } else {
            ve.currentVolume = 0;
        }
        ve.currentDelta = 0;
        node->x24 = ve.currentVolume;
        for (i = 0; i < node->voice_count; i++) {
            node->voice[i]->envelope = ve;
            u32 ratio = 0;
            if (!(node->flags & 4)) {
                ratio = (u32) (65536.0F *
                    (node->x14 * node->x18[0] * node->x18[1]));
            }
            AXPBSRC source = { .ratioHi = ratio >> 16,
                                 .ratioLo = (u16) ratio };
            node->voice[i]->source = source;
            {
                u32 address = (HSD_Synth_804D7780 + (HSD_Synth_804D7768 << 16)) * 2 +
                    i * streamChunks[HSD_Synth_804D7768].byte_size + 2;
                node->voice[i]->sample.address.currentAddressHi = address >> 16;
                node->voice[i]->sample.address.currentAddressLo = (u16) address;
            }
            {
                u32 address = (HSD_Synth_804D7780 + (HSD_Synth_804D7768 << 16)) * 2 +
                    i * streamChunks[HSD_Synth_804D7768].byte_size +
                    streamChunks[HSD_Synth_804D7768].end_address;
                node->voice[i]->sample.address.endAddressHi = address >> 16;
                node->voice[i]->sample.address.endAddressLo = (u16) address;
            }
            {
                u32 address = (HSD_Synth_804D7780 + (HSD_Synth_804D7768 << 16)) * 2 +
                    i * streamChunks[HSD_Synth_804D7768].byte_size + 2;
                node->voice[i]->sample.address.loopAddressHi = address >> 16;
                node->voice[i]->sample.address.loopAddressLo = (u16) address;
            }
            HSD_ASSERT(0, HSD_AudioVoiceStart(node->voice[i]));
        }
        node->flags &= ~8;
        if (!node->volume_update_pending && !(node->flags & 8)) {
            node->volume_update_pending = 1;
            node->x20 = HSD_Synth_804D774C;
            HSD_Synth_804D774C = node;
        }
        HSD_SynthSFXUpdateMix(node, 0);
        HSD_Synth_804D7778 = 0;
    } else {
        HSD_Synth_804D7778 = 0;
    }
}

void HSD_SynthPStreamFirstHakoHeaderCallback(int request, void* context, void* buffer, bool canceled)
{
    HSD_ASSERT(0, HSD_StreamDecodeChunk(&streamChunks[HSD_Synth_804D7768],
                                       streamChunkBytes));
    HSD_DevComRequest(HSD_Synth_804D7764, 0xA0,
                      HSD_Synth_804D7780 + (HSD_Synth_804D7768 << 16),
                      streamChunks[HSD_Synth_804D7768].byte_size, 0x23, 0,
                      HSD_Synth_8038B120, 0);
}

void HSD_SynthPStreamHeaderCallback(int arg0, void* arg1, void* arg2,
                                    bool cancelflag)
{
    HSD_StreamInfo info;
    HSD_ASSERT(0, HSD_StreamDecodeInfo(&info, arg2));
    struct HSD_SynthSFXNode* node;
    int i;

    node = getNode(HSD_Synth_804D7760);
    if (node != NULL) {
        node->voice_count = info.voice_count;
        if (node->voice_count == 2) {
            node->voice[1] = HSD_AudioVoiceAcquire(0x1D, dropcallback);
            HSD_ASSERTMSG(0x5CF, node->voice[1], "entry->voice[1]");
        }
        node->x14 = 0.00003125f * (f32) info.sample_rate;
        for (i = 0; i < node->voice_count; i++) {
            node->voice[i]->music = true;
            node->voice[i]->sample.address = info.address[i];
            node->voice[i]->sample.adpcm = info.adpcm[i];
        }
        HSD_Synth_804D7774 = (HSD_Synth_804D7774 + 2) % 3;
        HSD_Synth_804D776C = HSD_Synth_804D7770 = HSD_Synth_804D7768 =
            HSD_Synth_804D7774;
        HSD_DevComRequest(
            HSD_Synth_804D7764, 0x80,
            (uintptr_t) streamChunkBytes, 0x20, 0x21, 0,
            HSD_SynthPStreamFirstHakoHeaderCallback,
            NULL);
    } else {
        HSD_Synth_804D7778 = 0;
    }
}

static inline void HSD_Synth_8038B5AC_inline(void)
{
    struct HSD_SynthSFXNode* node = getNode(HSD_Synth_804D7760);
    int i;

    if (!node) {
        return;
    }

    if (node->flags & 8) {
        HSD_SynthSFXStopNode(node);
    } else if (!(node->flags & 1)) {
        node->flags |= 1;
        if (node->volume_update_pending == 0 && !(node->flags & 8)) {
            node->volume_update_pending = 1;
            node->x20 = HSD_Synth_804D774C;
            HSD_Synth_804D774C = node;
        }
        for (i = 0; i < node->voice_count; i++) {
            HSD_AudioVoicePriority(node->voice[i], 1);
        }
    }
}

int HSD_Synth_8038B5AC(int entrynum, u8 vol, u8 vol2, int channel)
{
    struct HSD_SynthSFXNode* voice_node;
    int idx;
    HSD_AudioVoice* voice;

    PAD_STACK(8);

    do {
        HSD_DevComPoll();
    } while (HSD_Synth_804D7778 != 0);

    HSD_Synth_804D7778 = 1;

    if (getNode(HSD_Synth_804D7760) != NULL) {
        HSD_Synth_8038B5AC_inline();
    }
    HSD_Synth_804D7764 = entrynum;
    voice = HSD_AudioVoiceAcquire(0x1D, dropcallback);
    voice->music = true;
    idx = voice->index;
    voice_node = &hsd_SynthSFXNodes[idx];
    HSD_Synth_804D7750 += 0x40;
    if (HSD_Synth_804D7750 < 0) {
        HSD_Synth_804D7750 = 0x40;
    }
    voice_node->x27 = 2;
    voice_node->x0 = HSD_Synth_804D7760 = HSD_Synth_804D7750 + idx;
    voice_node->sfx_id = 0;
    voice_node->flags = 8;
    voice_node->voice_count = 1;
    voice_node->xB = (u8) channel;
    voice_node->voice[0] = voice;
    voice_node->x18[0] = 1.0F;
    voice_node->x18[1] = 1.0F;
    voice_node->unk28 = 0.003921569F * vol;
    voice_node->user_vol[0].volume = 0.003921569F * vol;
    voice_node->user_vol[0].x4 = 0;
    voice_node->user_vol[0].x8_float = 0.003921569F * vol2;
    voice_node->user_vol[1].volume = 0.003921569F * vol2;
    voice_node->user_vol[1].x4 = 0;
    voice_node->user_vol[1].x8 = 0x80;
    voice_node->x44 = 1.0F;
    voice_node->x48 = 0.0F;
    voice_node->x4C = 0.0F;
    HSD_DevComRequest(entrynum, 0, 0, 0x80, 0x22, 1,
                      HSD_SynthPStreamHeaderCallback, NULL);
    return HSD_Synth_804D7760;
}

void HSD_SynthStreamSetVolume(f32 volume)
{
    HSD_Synth_804D6030 = volume;
    updateAllVolume(0xFFFF);
}

void HSD_SynthInit(int dsp_size, int voices, int stream_size, int bank_size)
{
    HSD_AudioVoiceInit();
    HSD_Synth_804D7784 = ARAlloc(0x500);
    HSD_DevComRequest(0, 0, HSD_Synth_804D7784, 0x500, 3, 0, 0, 0);
    HSD_Synth_804D7784 *= 2;
    hsd_SynthSFXBankHead[0] = ARAlloc(bank_size);
    /// Loop unrolled to match original binary (loop version adds 8 bytes
    /// to stack frame due to compiler allocating space for the counter)
    HSD_Synth_804C28E0_1784[0].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[0].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[0].x178C = 0;
    HSD_Synth_804C28E0_1784[1].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[1].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[1].x178C = 0;
    HSD_Synth_804C28E0_1784[2].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[2].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[2].x178C = 0;
    HSD_Synth_804C28E0_1784[3].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[3].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[3].x178C = 0;
    HSD_Synth_804C28E0_1784[4].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[4].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[4].x178C = 0;
    HSD_Synth_804C28E0_1784[5].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[5].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[5].x178C = 0;
    HSD_Synth_804C28E0_1784[6].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[6].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[6].x178C = 0;
    HSD_Synth_804C28E0_1784[7].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[7].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[7].x178C = 0;
    HSD_Synth_804C28E0_1784[8].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[8].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[8].x178C = 0;
    HSD_Synth_804C28E0_1784[9].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[9].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[9].x178C = 0;
    HSD_Synth_804C28E0_1784[10].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[10].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[10].x178C = 0;
    HSD_Synth_804C28E0_1784[11].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[11].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[11].x178C = 0;
    HSD_Synth_804C28E0_1784[12].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[12].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[12].x178C = 0;
    HSD_Synth_804C28E0_1784[13].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[13].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[13].x178C = 0;
    HSD_Synth_804C28E0_1784[14].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[14].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[14].x178C = 0;
    HSD_Synth_804C28E0_1784[15].x1784 = 1.0F;
    HSD_Synth_804C28E0_1784[15].x1788 = 1.0F;
    HSD_Synth_804C28E0_1784[15].x178C = 0;
    hsd_SynthSFXBankAREnd = hsd_SynthSFXBankHead[0] + bank_size;
    HSD_Synth_804D7780 = ARAlloc(0x30000);
}

/// @remarks Nothing in the DOL references this; it is recovered from the tail
/// of this TU's `.data`.
static u8 HSD_Synth_804080FC[0x44] = { 0 };
