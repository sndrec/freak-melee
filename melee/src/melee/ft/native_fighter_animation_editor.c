#include "native_fighter_animation_editor.h"

#ifdef MELEE_NATIVE

#include <melee/lb/lbanim_dat.h>
#include <sysdolphin/baselib/fobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_TRACKS = 4096, MAX_KEYS = 65536 };

typedef struct AnimationClip {
    HSD_Dat dat;
    FigaTree tree;
    FigaTrack tracks[MAX_TRACKS];
    u32 nodes;
    unsigned track_count;
    unsigned offset;
    unsigned size;
    const char* name;
} AnimationClip;

typedef struct KeyTiming {
    int previous_frame;
    int frame;
    size_t previous_wait_offset;
    size_t current_wait_offset;
    size_t previous_wait_size;
    size_t current_wait_size;
    unsigned previous_wait;
    unsigned current_wait;
    bool previous_timed;
    bool current_timed;
} KeyTiming;

static uint32_t be32(const uint8_t* p)
{
    return (uint32_t) p[0] << 24 | (uint32_t) p[1] << 16 |
           (uint32_t) p[2] << 8 | p[3];
}

typedef struct ClipDescriptor {
    const char* name;
    int32_t offset;
    int32_t size;
} ClipDescriptor;

static bool descriptor(const NativeFighterProject* project,
                       NativeFighterAnimationClip reference,
                       ClipDescriptor* result)
{
    HSD_Dat fighter;
    u32 root, table, string, offset, size;
    const char* external;
    unsigned count, field;
    if (project == NULL || result == NULL) return false;
    switch (reference.bank) {
    case NativeFighterAnimation_Action:
        count = project->action_count;
        field = 12;
        break;
    case NativeFighterAnimation_Demo:
        count = project->demo_count;
        field = 20;
        break;
    default: return false;
    }
    if (reference.index >= count || count > UINT32_MAX / 24 ||
        HSD_DatOpen(&fighter, project->fighter_dat,
                    project->fighter_size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&fighter, project->fighter_root, &root) ||
        HSD_DatResolve(&fighter, root + field, &table, &external) !=
            HSD_DAT_REF_LOCAL ||
        HSD_DatBytes(&fighter, table, count * 24) == NULL ||
        HSD_DatResolve(&fighter, table + reference.index * 24, &string,
                       &external) != HSD_DAT_REF_LOCAL ||
        !HSD_DatReadU32(&fighter, table + reference.index * 24 + 4, &offset) ||
        !HSD_DatReadU32(&fighter, table + reference.index * 24 + 8, &size))
        return false;
    const char* name = (const char*) HSD_DatBytes(&fighter, string, 1);
    if (name == NULL || *name == '\0' ||
        memchr(name, 0, fighter.data_size - string) == NULL) return false;
    *result = (ClipDescriptor){ name, (int32_t) offset, (int32_t) size };
    return true;
}

// Demo descriptors may intentionally refer to an ISO result archive which is
// not materialized in this project's bank. Missing roots remain unresolved;
// malformed or duplicate embedded roots are rejected.
static bool locate_blob(const NativeFighterProject* project,
                        const ClipDescriptor* metadata, HSD_Dat* dat,
                        unsigned* offset, unsigned* bytes, bool* found)
{
    const uint8_t* bank = project->animation_dat;
    size_t bank_size = project->animation_size;
    *found = false;
    if (bank == NULL) return false;
    if (metadata->offset >= 0 && metadata->size >= 32 &&
        (size_t) metadata->offset <= bank_size &&
        (size_t) metadata->size <= bank_size - (size_t) metadata->offset) {
        u32 root;
        if (HSD_DatOpen(dat, bank + metadata->offset, metadata->size) ==
                HSD_DAT_OK && HSD_DatFindRoot(dat, metadata->name, &root)) {
            *offset = (unsigned) metadata->offset;
            *bytes = (unsigned) metadata->size;
            *found = true;
            return true;
        }
    }
    for (size_t cursor = 0; cursor < bank_size;) {
        if (bank[cursor] == 0xFF) { ++cursor; continue; }
        if (bank_size - cursor < 32) return false;
        uint32_t size = be32(bank + cursor);
        HSD_Dat candidate;
        u32 root;
        if (size < 32 || size > bank_size - cursor || cursor > UINT32_MAX ||
            HSD_DatOpen(&candidate, bank + cursor, size) != HSD_DAT_OK)
            return false;
        if (HSD_DatFindRoot(&candidate, metadata->name, &root)) {
            if (*found) return false;
            *dat = candidate;
            *offset = (unsigned) cursor;
            *bytes = size;
            *found = true;
        }
        cursor += size;
    }
    return true;
}

static bool open_clip(const NativeFighterProject* project,
                      NativeFighterAnimationClip reference, AnimationClip* clip)
{
    ClipDescriptor metadata;
    bool found;
    u32 root;
    if (!descriptor(project, reference, &metadata) ||
        !locate_blob(project, &metadata, &clip->dat, &clip->offset,
                     &clip->size, &found) || !found ||
        !HSD_DatFindRoot(&clip->dat, metadata.name, &root) ||
        lbAnim_DecodeFigaTree(&clip->dat, root, &clip->tree,
                              clip->tracks, MAX_TRACKS, &clip->nodes) !=
            HSD_DAT_OK) return false;
    clip->name = metadata.name;
    clip->track_count = 0;
    for (unsigned i = 0; i < clip->nodes; ++i)
        clip->track_count += (uint8_t) clip->tree.nodes[i];
    return true;
}

bool nativeFighterAnimation_PreviewTree(const NativeFighterProject* project,
    NativeFighterAnimationClip clip, void* bytes, size_t byte_capacity,
    FigaTree* tree, FigaTrack* tracks, size_t track_capacity)
{
    ClipDescriptor metadata;
    HSD_Dat dat;
    unsigned offset, size;
    u32 root, nodes;
    bool found;
    if (!descriptor(project, clip, &metadata) ||
        !locate_blob(project, &metadata, &dat, &offset, &size, &found) ||
        !found || size > byte_capacity) return false;
    memcpy(bytes, project->animation_dat + offset, size);
    return HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
        HSD_DatFindRoot(&dat, metadata.name, &root) &&
        lbAnim_DecodeFigaTree(&dat, root, tree, tracks, track_capacity,
                             &nodes) == HSD_DAT_OK;
}

static bool packed(const uint8_t* bytes, size_t length, size_t* at,
                   unsigned* value)
{
    unsigned result = 0;
    unsigned shift = 0;
    for (unsigned i = 0; i < 5 && *at < length; ++i) {
        unsigned byte = bytes[(*at)++];
        if (shift == 28 && (byte & 0x7F) > 15) return false;
        result |= (byte & 0x7F) << shift;
        if (!(byte & 0x80)) {
            *value = result;
            return true;
        }
        shift += 7;
    }
    return false;
}

static unsigned width(uint8_t frac)
{
    switch (frac & 0xE0) {
    case HSD_A_FRAC_FLOAT: return 4;
    case HSD_A_FRAC_S16:
    case HSD_A_FRAC_U16: return 2;
    case HSD_A_FRAC_S8:
    case HSD_A_FRAC_U8: return 1;
    default: return 0;
    }
}

static bool decode_value(const uint8_t* bytes, size_t length, size_t* at,
                         uint8_t frac, float* value)
{
    unsigned bytes_needed = width(frac);
    if (!bytes_needed || bytes_needed > length - *at) return false;
    const uint8_t* p = bytes + *at;
    *at += bytes_needed;
    if ((frac & 0xE0) == HSD_A_FRAC_FLOAT) {
        uint32_t bits = (uint32_t) p[0] | (uint32_t) p[1] << 8 |
                        (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;
        memcpy(value, &bits, 4);
    } else {
        int number = bytes_needed == 2 ? (p[0] | p[1] << 8) : p[0];
        if ((frac & 0xE0) == HSD_A_FRAC_S16) number = (int16_t) number;
        if ((frac & 0xE0) == HSD_A_FRAC_S8) number = (int8_t) number;
        *value = (float) number / (float) (1U << (frac & 31));
    }
    return isfinite(*value);
}

static bool encode_value(uint8_t* destination, uint8_t frac, float value)
{
    if (!isfinite(value)) return false;
    if ((frac & 0xE0) == HSD_A_FRAC_FLOAT) {
        uint32_t bits;
        memcpy(&bits, &value, 4);
        for (unsigned i = 0; i < 4; ++i)
            destination[i] = (uint8_t) (bits >> (i * 8));
        return true;
    }
    double scaled = (double) value * (double) (1U << (frac & 31));
    int minimum, maximum;
    switch (frac & 0xE0) {
    case HSD_A_FRAC_S16: minimum = -32768; maximum = 32767; break;
    case HSD_A_FRAC_U16: minimum = 0; maximum = 65535; break;
    case HSD_A_FRAC_S8: minimum = -128; maximum = 127; break;
    case HSD_A_FRAC_U8: minimum = 0; maximum = 255; break;
    default: return false;
    }
    if (scaled < minimum || scaled > maximum || trunc(scaled) != scaled)
        return false;
    unsigned bits = (unsigned) (int) scaled;
    destination[0] = (uint8_t) bits;
    if (width(frac) == 2) destination[1] = (uint8_t) (bits >> 8);
    return true;
}

static bool scan_keys(const FigaTrack* track, NativeFighterAnimationKey* keys,
                      size_t capacity, size_t* count, unsigned selected,
                      size_t* value_offset, size_t* tangent_offset,
                      KeyTiming* timing)
{
    const uint8_t* bytes = track->ad_head;
    size_t length = track->length, at = 0;
    int frame = (int16_t) track->startframe;
    *count = 0;
    while (at < length) {
        unsigned header = bytes[at++];
        unsigned op = header & 15;
        unsigned packed_count = ((header >> 4) & 7) + 1;
        unsigned shift = 3;
        if (header & 0x80) {
            unsigned byte;
            do {
                if (at >= length || shift >= 31) return false;
                byte = bytes[at++];
                if ((byte & 0x7F) > (UINT32_MAX - packed_count) >> shift)
                    return false;
                packed_count += (byte & 0x7F) << shift;
                shift += 7;
            } while (byte & 0x80);
        }
        if (op < HSD_A_OP_CON || op > HSD_A_OP_KEY ||
            packed_count > MAX_KEYS - *count) return false;
        for (unsigned i = 0; i < packed_count; ++i) {
            float value = 0, slope = 0;
            size_t source = at;
            bool has_value = op != HSD_A_OP_SLP;
            bool has_tangent = op == HSD_A_OP_SPL || op == HSD_A_OP_SLP;
            if (has_value && !decode_value(bytes, length, &at,
                                           track->frac_value, &value))
                return false;
            size_t slope_source = at;
            if (has_tangent &&
                !decode_value(bytes, length, &at, track->frac_slope,
                              &slope)) return false;
            unsigned wait = 0;
            size_t wait_offset = at;
            bool timed = op != HSD_A_OP_SLP && op != HSD_A_OP_KEY;
            if (op != HSD_A_OP_SLP && op != HSD_A_OP_KEY &&
                !packed(bytes, length, &at, &wait)) return false;
            if (timing != NULL && selected != 0 &&
                *count == (size_t) selected - 1) {
                timing->previous_frame = frame;
                timing->previous_wait_offset = wait_offset;
                timing->previous_wait_size = at - wait_offset;
                timing->previous_wait = wait;
                timing->previous_timed = timed;
            }
            if (timing != NULL && *count == selected) {
                timing->frame = frame;
                timing->current_wait_offset = wait_offset;
                timing->current_wait_size = at - wait_offset;
                timing->current_wait = wait;
                timing->current_timed = timed;
            }
            if (keys != NULL) {
                if (*count >= capacity) return false;
                keys[*count] = (NativeFighterAnimationKey){
                    frame, op, value, slope, has_value, has_tangent
                };
            }
            if (*count == selected && value_offset != NULL)
                *value_offset = has_value ? source : SIZE_MAX;
            if (*count == selected && tangent_offset != NULL)
                *tangent_offset = has_tangent ? slope_source : SIZE_MAX;
            ++*count;
            if ((int64_t) frame + wait > INT32_MAX) return false;
            frame += (int) wait;
        }
    }
    return true;
}

bool nativeFighterAnimation_Info(const NativeFighterProject* project,
                                 NativeFighterAnimationClip reference,
                                 NativeFighterAnimationInfo* info)
{
    if (info == NULL) return false;
    AnimationClip clip;
    if (!open_clip(project, reference, &clip)) return false;
    *info = (NativeFighterAnimationInfo){
        clip.tree.frames, clip.nodes, clip.track_count, clip.offset, clip.size,
        clip.name
    };
    return true;
}

bool nativeFighterAnimation_SetFrames(NativeFighterProject* project,
                                      NativeFighterAnimationClip reference, float frames)
{
    AnimationClip clip;
    u32 root;
    if (project == NULL || !isfinite(frames) || frames < 0 ||
        frames > 65535 || !open_clip(project, reference, &clip) ||
        !HSD_DatFindRoot(&clip.dat, clip.name, &root)) return false;
    for (unsigned track = 0; frames < clip.tree.frames &&
                             track < clip.track_count; ++track) {
        const FigaTrack* source = &clip.tracks[track];
        size_t count = 0;
        if (!scan_keys(source, NULL, 0, &count, UINT32_MAX,
                       NULL, NULL, NULL)) return false;
        NativeFighterAnimationKey* keys = malloc(count * sizeof(*keys));
        if (keys == NULL && count != 0) return false;
        bool valid = scan_keys(source, keys, count, &count, UINT32_MAX,
                               NULL, NULL, NULL);
        for (size_t key = 0; valid && key < count; ++key)
            if (keys[key].frame > frames) valid = false;
        free(keys);
        if (!valid) return false;
    }
    const size_t absolute = (size_t) clip.offset + 32 + root + 8;
    if (absolute > project->animation_size ||
        4 > project->animation_size - absolute) return false;
    u32 bits;
    memcpy(&bits, &frames, sizeof(bits));
    project->animation_dat[absolute] = (u8) (bits >> 24);
    project->animation_dat[absolute + 1] = (u8) (bits >> 16);
    project->animation_dat[absolute + 2] = (u8) (bits >> 8);
    project->animation_dat[absolute + 3] = (u8) bits;
    return true;
}

bool nativeFighterAnimation_Tracks(const NativeFighterProject* project,
                                   NativeFighterAnimationClip reference,
                                   NativeFighterAnimationTrack* tracks,
                                   size_t capacity, size_t* count)
{
    if (count == NULL) return false;
    AnimationClip clip;
    if (!open_clip(project, reference, &clip) ||
        (tracks != NULL && capacity < clip.track_count)) return false;
    *count = clip.track_count;
    unsigned track = 0;
    for (unsigned joint = 0; joint < clip.nodes; ++joint) {
        for (unsigned i = 0; i < (uint8_t) clip.tree.nodes[joint]; ++i) {
            const FigaTrack* source = &clip.tracks[track];
            size_t keys;
            if (!scan_keys(source, NULL, 0, &keys, UINT32_MAX,
                           NULL, NULL, NULL))
                return false;
            if (tracks != NULL) tracks[track] = (NativeFighterAnimationTrack){
                joint, source->obj_type, (int16_t) source->startframe,
                source->length, (unsigned) keys
            };
            ++track;
        }
    }
    return true;
}

bool nativeFighterAnimation_Keys(const NativeFighterProject* project,
                                 NativeFighterAnimationClip reference, unsigned track,
                                 NativeFighterAnimationKey* keys,
                                 size_t capacity, size_t* count)
{
    if (count == NULL) return false;
    AnimationClip clip;
    return open_clip(project, reference, &clip) && track < clip.track_count &&
           scan_keys(&clip.tracks[track], keys, capacity, count,
                     UINT32_MAX, NULL, NULL, NULL);
}

static bool set_key_field(NativeFighterProject* project, NativeFighterAnimationClip reference,
                          unsigned track, unsigned key, float value,
                          bool tangent)
{
    AnimationClip clip;
    if (!open_clip(project, reference, &clip) || track >= clip.track_count)
        return false;
    const FigaTrack* source = &clip.tracks[track];
    if (source->obj_type < HSD_A_J_ROTX ||
        source->obj_type > HSD_A_J_SCAZ ||
        source->obj_type == HSD_A_J_PATH) return false;
    size_t count, offset = SIZE_MAX;
    if (!scan_keys(source, NULL, 0, &count, key,
                   tangent ? NULL : &offset,
                   tangent ? &offset : NULL, NULL) ||
        key >= count || offset == SIZE_MAX) return false;
    uint8_t replacement[4];
    uint8_t frac = tangent ? source->frac_slope : source->frac_value;
    if (!encode_value(replacement, frac, value)) return false;
    size_t bytes_needed = width(frac);
    size_t absolute = (size_t) (source->ad_head -
                               project->animation_dat) + offset;
    if (absolute > project->animation_size ||
        bytes_needed > project->animation_size - absolute) return false;
    memcpy(project->animation_dat + absolute, replacement, bytes_needed);
    return true;
}

bool nativeFighterAnimation_SetKey(NativeFighterProject* project,
                                   NativeFighterAnimationClip reference, unsigned track,
                                   unsigned key, float value)
{
    return set_key_field(project, reference, track, key, value, false);
}

bool nativeFighterAnimation_Snap(const NativeFighterProject* project,
                                 NativeFighterAnimationClip reference,
                                 unsigned track, bool tangent,
                                 float requested, float* snapped)
{
    AnimationClip clip;
    if (snapped == NULL || !isfinite(requested) ||
        !open_clip(project, reference, &clip) || track >= clip.track_count)
        return false;
    const FigaTrack* source = &clip.tracks[track];
    if (source->obj_type < HSD_A_J_ROTX ||
        source->obj_type > HSD_A_J_SCAZ ||
        source->obj_type == HSD_A_J_PATH) return false;
    const uint8_t frac = tangent ? source->frac_slope : source->frac_value;
    if ((frac & 0xE0) == HSD_A_FRAC_FLOAT) {
        *snapped = requested;
        return true;
    }
    double minimum, maximum;
    switch (frac & 0xE0) {
    case HSD_A_FRAC_S16: minimum = -32768; maximum = 32767; break;
    case HSD_A_FRAC_U16: minimum = 0; maximum = 65535; break;
    case HSD_A_FRAC_S8: minimum = -128; maximum = 127; break;
    case HSD_A_FRAC_U8: minimum = 0; maximum = 255; break;
    default: return false;
    }
    const double scale = (double) (1U << (frac & 31));
    const double quantized = nearbyint(fmax(minimum, fmin(maximum,
                                         (double) requested * scale)));
    *snapped = (float) (quantized / scale);
    return true;
}

bool nativeFighterAnimation_SetTangent(NativeFighterProject* project,
                                       NativeFighterAnimationClip reference, unsigned track,
                                       unsigned key, float tangent)
{
    return set_key_field(project, reference, track, key, tangent, true);
}

static size_t packed_width(unsigned value)
{
    size_t count = 1;
    while (value >= 128) {
        value >>= 7;
        ++count;
    }
    return count;
}

static void write_packed(uint8_t* destination, size_t width, unsigned value)
{
    for (size_t i = 0; i < width; ++i) {
        destination[i] = (uint8_t) (value & 0x7F) |
                         (i + 1 < width ? 0x80 : 0);
        value >>= 7;
    }
}

typedef struct AuthorKey {
    int frame;
    unsigned wait;
    uint8_t op;
    uint8_t payload[8];
    uint8_t payload_size;
} AuthorKey;

static void write_be32(uint8_t* dst, uint32_t value)
{
    dst[0] = (uint8_t) (value >> 24);
    dst[1] = (uint8_t) (value >> 16);
    dst[2] = (uint8_t) (value >> 8);
    dst[3] = (uint8_t) value;
}

static bool author_keys(const FigaTrack* track, AuthorKey* keys,
                        size_t capacity, size_t* count)
{
    const uint8_t* bytes = track->ad_head;
    size_t at = 0;
    int frame = (int16_t) track->startframe;
    *count = 0;
    while (at < track->length) {
        uint8_t header = bytes[at++];
        unsigned op = header & 15;
        unsigned run = ((header >> 4) & 7) + 1;
        unsigned shift = 3;
        if (header & 0x80) {
            unsigned extra;
            do {
                if (at >= track->length || shift >= 31) return false;
                extra = bytes[at++];
                run += (extra & 0x7F) << shift;
                shift += 7;
            } while (extra & 0x80);
        }
        if (op < HSD_A_OP_CON || op > HSD_A_OP_KEY ||
            run > capacity - *count) return false;
        for (unsigned i = 0; i < run; ++i) {
            AuthorKey* key = &keys[(*count)++];
            key->op = (uint8_t) op;
            key->frame = frame;
            key->payload_size = (uint8_t)
                ((op == HSD_A_OP_SLP ? 0 : width(track->frac_value)) +
                 ((op == HSD_A_OP_SPL || op == HSD_A_OP_SLP) ?
                    width(track->frac_slope) : 0));
            if (key->payload_size == 0 || key->payload_size > 8 ||
                key->payload_size > track->length - at) return false;
            memcpy(key->payload, bytes + at, key->payload_size);
            at += key->payload_size;
            key->wait = 0;
            if (op != HSD_A_OP_SLP && op != HSD_A_OP_KEY &&
                !packed(bytes, track->length, &at, &key->wait)) return false;
            if ((int64_t) frame + key->wait > INT32_MAX) return false;
            frame += (int) key->wait;
        }
    }
    return true;
}

// Build the descriptor changes in a private fighter DAT. Only physically
// present bank clips are rebased: unresolved ISO demo records are untouched.
// Every alias in either table referring to the resized blob gets its new size.
static bool rebase_descriptors(const NativeFighterProject* project,
                               unsigned old_offset, unsigned old_size,
                               size_t new_size, uint8_t* fighter_bytes)
{
    HSD_Dat fighter;
    u32 root;
    if (new_size > INT32_MAX ||
        HSD_DatOpen(&fighter, project->fighter_dat, project->fighter_size) !=
            HSD_DAT_OK ||
        !HSD_DatFindRoot(&fighter, project->fighter_root, &root)) return false;
    memcpy(fighter_bytes, project->fighter_dat, project->fighter_size);
    const unsigned counts[2] = { project->action_count, project->demo_count };
    for (unsigned bank = 0; bank < 2; ++bank) {
        if (counts[bank] == 0) continue;
        u32 table;
        const char* external;
        if (counts[bank] > UINT32_MAX / 24 ||
            HSD_DatResolve(&fighter, root + (bank == 0 ? 12 : 20),
                           &table, &external) != HSD_DAT_REF_LOCAL ||
            HSD_DatBytes(&fighter, table, counts[bank] * 24) == NULL)
            return false;
        for (unsigned index = 0; index < counts[bank]; ++index) {
            const u32 record = table + index * 24;
            u32 string, offset, size;
            HSD_DatReferenceKind kind = HSD_DatResolve(&fighter, record,
                                                &string, &external);
            if (kind == HSD_DAT_REF_NULL) continue;
            if (kind != HSD_DAT_REF_LOCAL ||
                !HSD_DatReadU32(&fighter, record + 4, &offset) ||
                !HSD_DatReadU32(&fighter, record + 8, &size)) return false;
            const char* name = (const char*) HSD_DatBytes(&fighter, string, 1);
            if (name == NULL || memchr(name, 0, fighter.data_size-string) == NULL)
                return false;
            if (*name == '\0') continue;
            ClipDescriptor metadata = { name, (int32_t) offset, (int32_t) size };
            HSD_Dat clip;
            unsigned physical_offset, physical_size;
            bool found;
            if (!locate_blob(project, &metadata, &clip, &physical_offset,
                             &physical_size, &found)) return false;
            if (!found) continue;
            int64_t rebased = physical_offset;
            size_t resized = physical_size;
            if (physical_offset == old_offset) {
                if (physical_size != old_size) return false;
                resized = new_size;
            } else if (physical_offset > old_offset) {
                if (physical_offset < (uint64_t) old_offset + old_size)
                    return false;
                rebased += (int64_t) new_size - old_size;
            } else if ((uint64_t) physical_offset + physical_size > old_offset)
                return false;
            if (rebased < 0 || rebased > INT32_MAX || resized > INT32_MAX)
                return false;
            write_be32(fighter_bytes + 32 + record + 4, (u32) rebased);
            write_be32(fighter_bytes + 32 + record + 8, (u32) resized);
        }
    }
    return true;
}

static bool store_track_stream(NativeFighterProject* project,
                               NativeFighterAnimationClip reference, unsigned track,
                               AnimationClip* clip, int startframe,
                               const AuthorKey* keys, size_t count,
                               size_t check_index, float check_value,
                               bool check_value_enabled)
{
    if (count == 0 || startframe < INT16_MIN || startframe > INT16_MAX)
        return false;
    size_t stream_size = 0;
    for (size_t i = 0; i < count; ++i) {
        stream_size += 1 + keys[i].payload_size;
        if (keys[i].op != HSD_A_OP_SLP && keys[i].op != HSD_A_OP_KEY)
            stream_size += packed_width(keys[i].wait);
    }
    if (stream_size > UINT16_MAX) return false;
    uint8_t* stream = malloc(stream_size);
    if (stream == NULL) return false;
    size_t cursor = 0;
    for (size_t i = 0; i < count; ++i) {
        const AuthorKey* key = &keys[i];
        stream[cursor++] = key->op;
        memcpy(stream + cursor, key->payload, key->payload_size);
        cursor += key->payload_size;
        if (key->op != HSD_A_OP_SLP && key->op != HSD_A_OP_KEY) {
            const size_t bytes = packed_width(key->wait);
            write_packed(stream + cursor, bytes, key->wait);
            cursor += bytes;
        }
    }
    u32 root, track_table;
    const char* external;
    bool ok = cursor == stream_size &&
        HSD_DatFindRoot(&clip->dat, clip->name, &root) &&
        HSD_DatResolve(&clip->dat, root + 16, &track_table, &external) ==
            HSD_DAT_REF_LOCAL;
    if (!ok) { free(stream); return false; }
    const size_t descriptor = 32 + (size_t) track_table + track * 12;
    const size_t append = (clip->dat.data_size + 3u) & ~3u;
    const size_t added_size = (append - clip->dat.data_size +
                               stream_size + 3u) & ~3u;
    const size_t new_clip_size = clip->size + added_size;
    const size_t new_bank_size = project->animation_size + added_size;
    if (descriptor + 12 > 32 + clip->dat.data_size ||
        new_clip_size > UINT32_MAX || new_bank_size >
            NATIVE_CONTENT_MAX_PACKAGE_BYTES || append > UINT32_MAX) {
        free(stream);
        return false;
    }
    uint8_t* bank = malloc(new_bank_size);
    if (bank == NULL) { free(stream); return false; }
    const uint8_t* original = project->animation_dat;
    const size_t start = clip->offset;
    memcpy(bank, original, start);
    memcpy(bank + start, original + start, 32 + clip->dat.data_size);
    memset(bank + start + 32 + clip->dat.data_size, 0, added_size);
    memcpy(bank + start + 32 + append, stream, stream_size);
    memcpy(bank + start + 32 + clip->dat.data_size + added_size,
           original + start + 32 + clip->dat.data_size,
           clip->size - 32 - clip->dat.data_size);
    memcpy(bank + start + new_clip_size, original + start + clip->size,
           project->animation_size - start - clip->size);
    free(stream);
    write_be32(bank + start, (u32) new_clip_size);
    write_be32(bank + start + 4, clip->dat.data_size + (u32) added_size);
    bank[start + descriptor] = (uint8_t) (stream_size >> 8);
    bank[start + descriptor + 1] = (uint8_t) stream_size;
    bank[start + descriptor + 2] = (uint8_t) (startframe >> 8);
    bank[start + descriptor + 3] = (uint8_t) startframe;
    write_be32(bank + start + descriptor + 8, (u32) append);

    uint8_t* fighter = malloc(project->fighter_size);
    if (fighter == NULL ||
        !rebase_descriptors(project, clip->offset, clip->size,
                            new_clip_size, fighter)) {
        free(fighter);
        free(bank);
        return false;
    }
    NativeFighterProject candidate = *project;
    candidate.fighter_dat = fighter;
    candidate.animation_dat = bank;
    candidate.animation_size = new_bank_size;
    NativeFighterAnimationInfo verified_info;
    size_t verified_count;
    NativeFighterAnimationKey* verified = malloc(count * sizeof(*verified));
    ok = verified != NULL &&
         nativeFighterAnimation_Info(&candidate, reference, &verified_info) &&
         nativeFighterAnimation_Keys(&candidate, reference, track, verified,
                                     count, &verified_count) &&
         verified_count == count;
    for (size_t i = 0; ok && i < count; ++i)
        if (verified[i].frame != keys[i].frame) ok = false;
    if (ok && check_value_enabled &&
        (check_index >= count || verified[check_index].value != check_value))
        ok = false;
    free(verified);
    if (ok) {
        memcpy(project->fighter_dat, fighter, project->fighter_size);
        free(project->animation_dat);
        project->animation_dat = bank;
        project->animation_size = new_bank_size;
    } else free(bank);
    free(fighter);
    return ok;
}

bool nativeFighterAnimation_InsertKey(NativeFighterProject* project,
                                      NativeFighterAnimationClip reference, unsigned track,
                                      int frame, float value)
{
    AnimationClip clip;
    if (frame < 0 || !isfinite(value) ||
        !open_clip(project, reference, &clip) || track >= clip.track_count ||
        frame > (int) clip.tree.frames) return false;
    const FigaTrack* source = &clip.tracks[track];
    if (source->obj_type < HSD_A_J_ROTX ||
        source->obj_type > HSD_A_J_SCAZ ||
        source->obj_type == HSD_A_J_PATH ||
        source->length == 0) return false;
    uint8_t encoded[4];
    if (!encode_value(encoded, source->frac_value, value)) return false;
    size_t count;
    if (!scan_keys(source, NULL, 0, &count, UINT32_MAX,
                   NULL, NULL, NULL) || count == 0 || count >= MAX_KEYS)
        return false;
    AuthorKey* keys = malloc((count + 1) * sizeof(*keys));
    if (keys == NULL) return false;
    size_t parsed;
    bool ok = author_keys(source, keys, count, &parsed) && parsed == count;
    size_t insert = count;
    for (size_t i = 0; ok && i < count; ++i) {
        if (keys[i].frame == frame && keys[i].op != HSD_A_OP_SLP) {
            free(keys);
            return nativeFighterAnimation_SetKey(project, reference, track,
                                                 (unsigned) i, value);
        }
        if (keys[i].frame > frame) { insert = i; break; }
    }
    // A timed predecessor determines when the newly inserted key is loaded.
    // Initial keys before startframe require changing the track descriptor,
    // which is deliberately rejected rather than silently moving the clip.
    const bool prepend = insert == 0 && frame < keys[0].frame;
    size_t predecessor = count;
    for (size_t i = 0; ok && i < insert; ++i)
        if (keys[i].op != HSD_A_OP_SLP && keys[i].op != HSD_A_OP_KEY &&
            keys[i].frame < frame &&
            (predecessor == count || keys[i].frame >= keys[predecessor].frame))
            predecessor = i;
    if (!prepend &&
        (predecessor == count ||
         (insert < count && keys[predecessor].frame +
              (int64_t) keys[predecessor].wait != keys[insert].frame) ||
         frame <= keys[predecessor].frame)) ok = false;
    if (prepend && frame > INT16_MAX) ok = false;
    if (!ok) { free(keys); return false; }
    unsigned next = insert < count ? (unsigned) (keys[insert].frame - frame) : 0;
    if (!prepend)
        keys[predecessor].wait = (unsigned) (frame - keys[predecessor].frame);
    memmove(keys + insert + 1, keys + insert,
            (count - insert) * sizeof(*keys));
    AuthorKey* added = &keys[insert];
    memset(added, 0, sizeof(*added));
    added->frame = frame;
    added->wait = next;
    added->op = HSD_A_OP_LIN;
    added->payload_size = (uint8_t) width(source->frac_value);
    memcpy(added->payload, encoded, added->payload_size);

    ok = store_track_stream(project, reference, track, &clip,
        prepend ? frame : (int16_t) source->startframe,
        keys, count + 1, insert, value, true);
    free(keys);
    return ok;
}

bool nativeFighterAnimation_KeyChannel(NativeFighterProject* project,
                                       NativeFighterAnimationClip reference, unsigned joint,
                                       unsigned channel, int frame,
                                       float value, float default_value)
{
    AnimationClip clip;
    if (joint >= 512 || channel < HSD_A_J_ROTX || channel > HSD_A_J_SCAZ ||
        channel == HSD_A_J_PATH || frame < 0 || !isfinite(value) ||
        !isfinite(default_value) || !open_clip(project, reference, &clip) ||
        frame > clip.tree.frames || clip.track_count + 1 >= MAX_TRACKS)
    {
        return false;
    }
    unsigned insertion = 0;
    for (unsigned j = 0; j < joint && j < clip.nodes; ++j) {
        insertion += (uint8_t) clip.tree.nodes[j];
    }
    unsigned bone_tracks =
        joint < clip.nodes ? (uint8_t) clip.tree.nodes[joint] : 0;
    for (unsigned i = insertion; i < insertion + bone_tracks; ++i) {
        if (clip.tracks[i].obj_type == channel) {
            float snapped;
            return nativeFighterAnimation_Snap(project, reference, i, false, value,
                                                    &snapped) &&
                   nativeFighterAnimation_InsertKey(project, reference, i, frame,
                                                    snapped);
        }
    }
    if (bone_tracks >= 254) {
        return false;
    }
    insertion += bone_tracks;
    u32 root, old_table;
    const char* external;
    if (!HSD_DatFindRoot(&clip.dat, clip.name, &root) ||
        HSD_DatResolve(&clip.dat, root + 16, &old_table, &external) !=
            HSD_DAT_REF_LOCAL)
    {
        return false;
    }
    const unsigned nodes = joint < clip.nodes ? clip.nodes : joint + 1;
    const size_t node_offset = (clip.dat.data_size + 3u) & ~3u;
    const size_t table_offset = (node_offset + nodes + 1 + 3u) & ~3u;
    const size_t stream_offset = table_offset + (clip.track_count + 1) * 12;
    uint8_t stream[20];
    size_t stream_size = 0;
    {
        // FObj needs a second endpoint to establish interpolation, even for
        // a constant channel first authored at frame zero.
        unsigned first_wait =
            frame > 0 ? (unsigned) frame
                      : (unsigned) fmaxf(1, ceilf(clip.tree.frames));
        stream[stream_size++] = HSD_A_OP_LIN;
        encode_value(stream + stream_size, HSD_A_FRAC_FLOAT,
                     frame > 0 ? default_value : value);
        stream_size += 4;
        size_t wait_size = packed_width(first_wait);
        write_packed(stream + stream_size, wait_size, first_wait);
        stream_size += wait_size;
    }
    stream[stream_size++] = HSD_A_OP_LIN;
    encode_value(stream + stream_size, HSD_A_FRAC_FLOAT, value);
    stream_size += 4;
    stream[stream_size++] = 0;
    const size_t data_size = (stream_offset + stream_size + 3u) & ~3u;
    const size_t new_relocations =
        clip.dat.relocation_count + clip.track_count + 1;
    const size_t tail_size =
        clip.size - 32 - clip.dat.data_size - clip.dat.relocation_count * 4;
    const size_t clip_size = 32 + data_size + new_relocations * 4 + tail_size;
    const size_t bank_size = project->animation_size - clip.size + clip_size;
    if (bank_size > NATIVE_CONTENT_MAX_PACKAGE_BYTES || clip_size > UINT32_MAX)
    {
        return false;
    }
    uint8_t* bank = calloc(1, bank_size);
    if (bank == NULL) {
        return false;
    }
    memcpy(bank, project->animation_dat, clip.offset);
    uint8_t* bytes = bank + clip.offset;
    memcpy(bytes, project->animation_dat + clip.offset,
           32 + clip.dat.data_size);
    uint8_t* data = bytes + 32;
    memcpy(data + node_offset, clip.tree.nodes, clip.nodes);
    data[node_offset + joint] = (uint8_t) (bone_tracks + 1);
    data[node_offset + nodes] = 0xFF;
    for (unsigned t = 0; t < clip.track_count + 1; ++t) {
        uint8_t* descriptor = data + table_offset + t * 12;
        if (t == insertion) {
            descriptor[0] = (uint8_t) (stream_size >> 8);
            descriptor[1] = (uint8_t) stream_size;
            descriptor[4] = (uint8_t) channel;
            descriptor[5] = descriptor[6] = HSD_A_FRAC_FLOAT;
            write_be32(descriptor + 8, (uint32_t) stream_offset);
        } else {
            memcpy(descriptor,
                   clip.dat.data + old_table +
                       (t < insertion ? t : t - 1) * 12,
                   12);
        }
    }
    memcpy(data + stream_offset, stream, stream_size);
    write_be32(data + root + 12, (uint32_t) node_offset);
    write_be32(data + root + 16, (uint32_t) table_offset);
    memcpy(data + data_size, clip.dat.relocations,
           clip.dat.relocation_count * 4);
    for (unsigned t = 0; t < clip.track_count + 1; ++t) {
        write_be32(data + data_size + (clip.dat.relocation_count + t) * 4,
                   (uint32_t) table_offset + t * 12 + 8);
    }
    memcpy(data + data_size + new_relocations * 4, clip.dat.roots, tail_size);
    write_be32(bytes, (uint32_t) clip_size);
    write_be32(bytes + 4, (uint32_t) data_size);
    write_be32(bytes + 8, (uint32_t) new_relocations);
    memcpy(bytes + clip_size, project->animation_dat + clip.offset + clip.size,
           project->animation_size - clip.offset - clip.size);
    uint8_t* fighter = malloc(project->fighter_size);
    if (fighter == NULL ||
        !rebase_descriptors(project, clip.offset, clip.size,
                            clip_size, fighter)) {
        free(fighter);
        free(bank);
        return false;
    }
    NativeFighterProject candidate = *project;
    candidate.fighter_dat = fighter;
    candidate.animation_dat = bank;
    candidate.animation_size = bank_size;
    float sampled;
    bool valid = nativeFighterAnimation_Sample(&candidate, reference, insertion,
                                               (float) frame, &sampled) &&
                 fabsf(sampled - value) < 0.00001f;
    if (valid) {
        memcpy(project->fighter_dat, fighter, project->fighter_size);
        free(project->animation_dat);
        project->animation_dat = bank;
        project->animation_size = bank_size;
    } else {
        free(bank);
    }
    free(fighter);
    return valid;
}

bool nativeFighterAnimation_DeleteKey(NativeFighterProject* project,
                                      NativeFighterAnimationClip reference, unsigned track,
                                      unsigned key)
{
    AnimationClip clip;
    if (!open_clip(project, reference, &clip) || track >= clip.track_count)
        return false;
    const FigaTrack* source = &clip.tracks[track];
    if (source->obj_type < HSD_A_J_ROTX ||
        source->obj_type > HSD_A_J_SCAZ ||
        source->obj_type == HSD_A_J_PATH) return false;
    size_t count;
    if (!scan_keys(source, NULL, 0, &count, UINT32_MAX,
                   NULL, NULL, NULL) || key == 0 || key + 1 >= count)
        return false;
    AuthorKey* keys = malloc(count * sizeof(*keys));
    if (keys == NULL) return false;
    size_t parsed;
    bool valid = author_keys(source, keys, count, &parsed) && parsed == count &&
        keys[key - 1].op != HSD_A_OP_SLP &&
        keys[key - 1].op != HSD_A_OP_KEY &&
        keys[key].op != HSD_A_OP_SLP &&
        keys[key].op != HSD_A_OP_KEY &&
        keys[key - 1].frame < keys[key].frame &&
        keys[key].frame < keys[key + 1].frame &&
        (int64_t) keys[key - 1].frame + keys[key - 1].wait ==
            keys[key].frame &&
        (int64_t) keys[key].frame + keys[key].wait ==
            keys[key + 1].frame;
    if (valid) {
        keys[key - 1].wait = (unsigned) (keys[key + 1].frame -
                                          keys[key - 1].frame);
        memmove(keys + key, keys + key + 1,
                (count - key - 1) * sizeof(*keys));
        valid = store_track_stream(project, reference, track, &clip,
            (int16_t) source->startframe, keys, count - 1, 0, 0, false);
    }
    free(keys);
    return valid;
}

bool nativeFighterAnimation_MoveKey(NativeFighterProject* project,
                                    NativeFighterAnimationClip reference, unsigned track,
                                    unsigned key, int frame)
{
    AnimationClip clip;
    if (!open_clip(project, reference, &clip) || track >= clip.track_count ||
        key == 0) return false;
    const FigaTrack* source = &clip.tracks[track];
    if (source->obj_type < HSD_A_J_ROTX ||
        source->obj_type > HSD_A_J_SCAZ ||
        source->obj_type == HSD_A_J_PATH) return false;
    size_t count;
    KeyTiming timing = { 0 };
    if (!scan_keys(source, NULL, 0, &count, key, NULL, NULL, &timing) ||
        key + 1 >= count || !timing.previous_timed ||
        !timing.current_timed || timing.previous_wait == 0 ||
        timing.current_wait == 0) return false;
    int64_t next = (int64_t) timing.frame + timing.current_wait;
    if (frame <= timing.previous_frame || frame >= next) return false;
    if (frame == timing.frame) return true;
    unsigned before = (unsigned) (frame - timing.previous_frame);
    unsigned after = (unsigned) (next - frame);
    if (packed_width(before) != timing.previous_wait_size ||
        packed_width(after) != timing.current_wait_size ||
        timing.previous_wait_size > 5 || timing.current_wait_size > 5)
        return false;
    size_t base = (size_t) (source->ad_head - project->animation_dat);
    size_t previous = base + timing.previous_wait_offset;
    size_t current = base + timing.current_wait_offset;
    if (previous > project->animation_size ||
        timing.previous_wait_size > project->animation_size - previous ||
        current > project->animation_size ||
        timing.current_wait_size > project->animation_size - current)
        return false;
    uint8_t saved_previous[5], saved_current[5];
    memcpy(saved_previous, project->animation_dat + previous,
           timing.previous_wait_size);
    memcpy(saved_current, project->animation_dat + current,
           timing.current_wait_size);
    write_packed(project->animation_dat + previous,
                 timing.previous_wait_size, before);
    write_packed(project->animation_dat + current,
                 timing.current_wait_size, after);
    KeyTiming verified = { 0 };
    size_t verified_count;
    bool valid = scan_keys(source, NULL, 0, &verified_count, key,
                           NULL, NULL, &verified) &&
                 verified_count == count && verified.frame == frame;
    if (!valid) {
        memcpy(project->animation_dat + previous, saved_previous,
               timing.previous_wait_size);
        memcpy(project->animation_dat + current, saved_current,
               timing.current_wait_size);
    }
    return valid;
}

typedef struct AnimationSample {
    bool seen;
    float value;
} AnimationSample;

static void capture_sample(void* context, enum_t type, HSD_ObjData* data)
{
    AnimationSample* sample = context;
    (void) type;
    sample->seen = true;
    sample->value = data->fv;
}

bool nativeFighterAnimation_Sample(const NativeFighterProject* project,
                                    NativeFighterAnimationClip reference, unsigned track,
                                    float frame, float* value)
{
    AnimationClip clip;
    if (value == NULL || !isfinite(frame) || frame < 0 || frame > 65535 ||
        !open_clip(project, reference, &clip) || track >= clip.track_count ||
        frame > clip.tree.frames) return false;
    const FigaTrack* source = &clip.tracks[track];
    if (source->obj_type < HSD_A_J_ROTX ||
        source->obj_type > HSD_A_J_SCAZ ||
        source->obj_type == HSD_A_J_PATH) return false;
    HSD_FObj fobj = { 0 };
    fobj.startframe = source->startframe;
    fobj.obj_type = source->obj_type;
    fobj.frac_value = source->frac_value;
    fobj.frac_slope = source->frac_slope;
    fobj.ad_head = source->ad_head;
    fobj.length = source->length;
    HSD_FObjReqAnimAll(&fobj, 0);
    AnimationSample sample = { 0 };
    float previous = 0;
    for (unsigned step = 0; (float) step <= frame; ++step) {
        HSD_FObjInterpretAnim(&fobj, &sample, capture_sample,
                              (float) step - previous);
        previous = (float) step;
    }
    if (previous < frame)
        HSD_FObjInterpretAnim(&fobj, &sample, capture_sample,
                              frame - previous);
    if (!sample.seen || !isfinite(sample.value)) return false;
    *value = sample.value;
    return true;
}

#endif
