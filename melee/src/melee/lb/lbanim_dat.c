#include "lbanim_dat.h"
#include <sysdolphin/baselib/native_fobj_stream.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>

HSD_DatResult lbAnim_DecodeFigaTree(const HSD_Dat* dat, u32 offset,
    FigaTree* tree, FigaTrack* tracks, size_t capacity, u32* node_count)
{
    FigaTree decoded = { 0 };
    u32 type, nodes_offset, tracks_offset;
    const char* external;
    size_t track_count = 0;
    u32 count = 0;
    size_t stream_bytes = 0;
    if (!tree || !tracks || !node_count || !HSD_DatBytes(dat, offset, 20))
        return HSD_DAT_DESCRIPTOR;
    HSD_DatReadU32(dat, offset, &type);
    decoded.type = (int) type;
    HSD_DatReadU32(dat, offset + 4, &decoded.flags);
    HSD_DatReadF32(dat, offset + 8, &decoded.frames);
    if (!isfinite(decoded.frames) || decoded.frames < 0 ||
        HSD_DatResolve(dat, offset + 12, &nodes_offset, &external) != HSD_DAT_REF_LOCAL ||
        HSD_DatResolve(dat, offset + 16, &tracks_offset, &external) != HSD_DAT_REF_LOCAL)
        return HSD_DAT_DESCRIPTOR;
    for (;;) {
        const u8* node = HSD_DatBytes(dat, nodes_offset + count, 1);
        if (!node) return HSD_DAT_DESCRIPTOR;
        if (*node == 255) break;
        if (*node > 127 || count == 512) return HSD_DAT_DESCRIPTOR;
        track_count += *node;
        if (track_count > capacity) return HSD_DAT_CAPACITY;
        ++count;
    }
    if (!HSD_DatBytes(dat, tracks_offset, track_count * 12))
        return HSD_DAT_DESCRIPTOR;
    for (size_t i = 0; i < track_count; ++i) {
        u32 source = tracks_offset + (u32) i * 12, stream_offset;
        FigaTrack track = { 0 };
        const u8* bytes = HSD_DatBytes(dat, source, 12);
        HSD_DatReadU16(dat, source, &track.length);
        HSD_DatReadU16(dat, source + 2, &track.startframe);
        track.obj_type = bytes[4];
        track.frac_value = bytes[5];
        track.frac_slope = bytes[6];
        if (HSD_DatResolve(dat, source + 8, &stream_offset, &external) != HSD_DAT_REF_LOCAL ||
            !(track.ad_head = (u8*) HSD_DatBytes(dat, stream_offset, track.length)))
            return HSD_DAT_DESCRIPTOR;
        // FigaTree playback constructs joint controllers without external targets.
        if (track.obj_type < HSD_A_J_ROTX || track.obj_type > HSD_A_J_BRANCH ||
            track.obj_type == HSD_A_J_PATH) return HSD_DAT_DESCRIPTOR;
        stream_bytes += track.length;
        if (stream_bytes > 4 * 1024 * 1024) return HSD_DAT_CAPACITY;
        HSD_TrackFacts facts;
        if (HSD_TrackInspect(track.ad_head, track.length, track.frac_value, track.frac_slope,
                             131072, &facts) != HSD_TRACK_VALID) return HSD_DAT_DESCRIPTOR;
        tracks[i] = track;
    }
    decoded.nodes = (s8*) HSD_DatBytes(dat, nodes_offset, count + 1);
    decoded.tracks = tracks;
    *tree = decoded;
    *node_count = count;
    return HSD_DAT_OK;
}
