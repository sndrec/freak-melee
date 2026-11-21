#pragma once

#include "fobj.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

// Admission-time description of the operands consumed by FObjLoadData and
// FObjLoadWait. Article census also distinguishes unsupported encodings from
// malformed streams, so callers can report which resources are missing.
typedef enum HSD_TrackStatus {
    HSD_TRACK_VALID, HSD_TRACK_UNSUPPORTED, HSD_TRACK_INVALID,
} HSD_TrackStatus;

typedef struct HSD_TrackFacts {
    unsigned work;             // opcode groups plus keys
    bool constant;             // every group uses constant interpolation
    bool has_palette_default;  // truncates to 255
    float min_value, max_without_default;
} HSD_TrackFacts;

static inline unsigned HSD_TrackScalarWidth(u8 frac)
{
    if (frac == HSD_A_FRAC_FLOAT) return 4;
    switch (frac & 0xe0) {
    case HSD_A_FRAC_S16: case HSD_A_FRAC_U16: return 2;
    case HSD_A_FRAC_S8: case HSD_A_FRAC_U8: return 1;
    default: return 0;
    }
}

static inline HSD_TrackStatus HSD_TrackInspect(
    const u8* bytes, u32 length, u8 value_format, u8 slope_format,
    unsigned work_limit, HSD_TrackFacts* facts)
{
    *facts = (HSD_TrackFacts){ .constant = true };
    if (!length) return bytes ? HSD_TRACK_VALID : HSD_TRACK_UNSUPPORTED;
    if (!bytes || (uintptr_t)bytes > UINTPTR_MAX - length) return HSD_TRACK_INVALID;
    u32 at = 0, keys = 0, immediate_keys = 0;
    while (at < length) {
        if (facts->work == work_limit) return HSD_TRACK_INVALID;
        ++facts->work;
        unsigned header = bytes[at++], op = header & 15;
        unsigned count = ((header >> 4) & 7) + 1;
        for (unsigned shift = 3; header & 128; shift += 7) {
            if (at == length || shift >= 16) return HSD_TRACK_INVALID;
            header = bytes[at++];
            if ((header & 127) > (UINT16_MAX - count) >> shift) return HSD_TRACK_INVALID;
            count += (header & 127) << shift;
        }
        if (count > 65536 - keys || count > work_limit - facts->work)
            return HSD_TRACK_INVALID;
        if (op < HSD_A_OP_CON || op > HSD_A_OP_KEY) return HSD_TRACK_UNSUPPORTED;
        facts->constant &= op == HSD_A_OP_CON;
        keys += count;
        facts->work += count;
        const unsigned first = op == HSD_A_OP_SLP ? 1 : 0;
        const unsigned last = op == HSD_A_OP_SPL || op == HSD_A_OP_SLP ? 2 : 1;
        for (unsigned i = 0; i < count; ++i) {
            // Playback consumes zero-time keys together before yielding a frame.
            if (++immediate_keys > 256) return HSD_TRACK_INVALID;
            for (unsigned k = first; k < last; ++k) {
                u8 frac = k ? slope_format : value_format;
                unsigned width = HSD_TrackScalarWidth(frac);
                if (!width || (frac & 31) == 31) return HSD_TRACK_UNSUPPORTED;
                if (width > length - at) return HSD_TRACK_INVALID;
                const u8* p = bytes + at;
                at += width;
                float value;
                if (width == 4) {
                    uint32_t bits = p[0] | (uint32_t)p[1] << 8 |
                                    (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
                    if ((bits & 0x7f800000) == 0x7f800000) return HSD_TRACK_INVALID;
                    memcpy(&value, &bits, sizeof(value));
                } else if (facts->constant) {
                    int number = width == 2 ? p[0] | (unsigned)p[1] << 8 : p[0];
                    if ((frac & 0xe0) == HSD_A_FRAC_S16) number = (int16_t)number;
                    if ((frac & 0xe0) == HSD_A_FRAC_S8) number = (int8_t)number;
                    value = (float)number / (float)(1u << (frac & 31));
                } else continue;
                if (facts->constant) {
                    if (value < facts->min_value) facts->min_value = value;
                    if (value >= 255 && value < 256) facts->has_palette_default = true;
                    else if (value > facts->max_without_default) facts->max_without_default = value;
                }
            }
            // A final value may end at EOF. Slope-only keys have no wait.
            if (op == HSD_A_OP_SLP || at == length) continue;
            unsigned wait = 0;
            for (unsigned shift = 0;; shift += 7) {
                if (at == length || shift > 14) return HSD_TRACK_INVALID;
                unsigned b = bytes[at++];
                if (shift == 14 && (b & 127) > 3) return HSD_TRACK_INVALID;
                wait |= (b & 127) << shift;
                if (!(b & 128)) break;
            }
            if (wait) immediate_keys = 0;
        }
    }
    return HSD_TRACK_VALID;
}

static inline bool HSD_TrackFitsTable(const HSD_TrackFacts* facts, unsigned entries,
                                     bool allow_palette_default)
{
    return entries && facts->work && facts->constant && facts->min_value > -1 &&
           facts->max_without_default < entries &&
           (!facts->has_palette_default || entries > 255 || allow_palette_default);
}
