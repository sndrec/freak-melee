#ifndef MELEE_FT_NATIVE_FIGHTER_ANIMATION_EDITOR_H
#define MELEE_FT_NATIVE_FIGHTER_ANIMATION_EDITOR_H

#ifdef MELEE_NATIVE

#include "native_fighter_project.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFighterAnimationInfo {
    float frames;
    unsigned joints;
    unsigned tracks;
    unsigned clip_offset;
    unsigned clip_size;
    // Borrowed descriptor name; valid until the project's fighter DAT changes.
    const char* name;
} NativeFighterAnimationInfo;

typedef struct NativeFighterAnimationTrack {
    unsigned joint;
    unsigned channel;
    int start_frame;
    unsigned encoded_bytes;
    unsigned keys;
} NativeFighterAnimationTrack;

typedef struct NativeFighterAnimationKey {
    int frame;
    unsigned interpolation;
    float value;
    float tangent;
    bool editable;
    bool tangent_editable;
} NativeFighterAnimationKey;

typedef enum NativeFighterAnimationBank {
    NativeFighterAnimation_Action,
    NativeFighterAnimation_Demo,
} NativeFighterAnimationBank;

typedef struct NativeFighterAnimationClip {
    NativeFighterAnimationBank bank;
    unsigned index;
} NativeFighterAnimationClip;

// References select the action or demo table explicitly; track/key indices
// retain their source order. Values
// and spline tangents change in place only when the original quantization
// represents them exactly; other clips, tracks and timing stay byte-identical.
bool nativeFighterAnimation_Info(const NativeFighterProject* project,
                                 NativeFighterAnimationClip clip,
                                 NativeFighterAnimationInfo* info);
// Change the source figatree's end frame. Refuse to truncate encoded keys;
// fighter action looping is separate from this clip metadata.
bool nativeFighterAnimation_SetFrames(NativeFighterProject* project,
                                      NativeFighterAnimationClip clip, float frames);
bool nativeFighterAnimation_Tracks(const NativeFighterProject* project,
                                   NativeFighterAnimationClip clip,
                                   NativeFighterAnimationTrack* tracks,
                                   size_t capacity, size_t* count);
bool nativeFighterAnimation_Keys(const NativeFighterProject* project,
                                 NativeFighterAnimationClip clip, unsigned track,
                                 NativeFighterAnimationKey* keys,
                                 size_t capacity, size_t* count);
bool nativeFighterAnimation_SetKey(NativeFighterProject* project,
                                   NativeFighterAnimationClip clip, unsigned track,
                                   unsigned key, float value);
// Snap a dragged value to the source track's representable precision/range.
// Float tracks retain the requested value; packed integer tracks use their
// original fractional scale. Does not change the draft.
bool nativeFighterAnimation_SnapValue(const NativeFighterProject* project,
                                      NativeFighterAnimationClip clip, unsigned track,
                                      float requested, float* snapped);
bool nativeFighterAnimation_SetTangent(NativeFighterProject* project,
                                       NativeFighterAnimationClip clip, unsigned track,
                                       unsigned key, float tangent);
// Moves a timed key between its immediate timed neighbors by changing the two
// adjacent encoded waits. The following keys and clip duration stay fixed.
bool nativeFighterAnimation_MoveKey(NativeFighterProject* project,
                                    NativeFighterAnimationClip clip, unsigned track,
                                    unsigned key, int frame);
// Re-encode one imported transform track and grow its DAT clip when a new
// frame is needed. Existing encoded values, tangent bytes, opcodes and key
// frames are retained; the new key uses linear interpolation. On success the
// animation bank allocation may change, so callers must not keep pointers
// into it. Existing keyed frames are updated in place instead.
bool nativeFighterAnimation_InsertKey(NativeFighterProject* project,
                                       NativeFighterAnimationClip clip, unsigned track,
                                        int frame, float value);
// Key a bone/channel, creating a float track when the clip did not animate it.
// A new track retains default_value at frame zero before the authored key.
bool nativeFighterAnimation_KeyChannel(NativeFighterProject* project,
    NativeFighterAnimationClip clip, unsigned joint, unsigned channel, int frame,
    float value, float default_value);
// Remove one interior timed transform key, preserving the other key payloads,
// frames and following track data. A source endpoint or slope-only key cannot
// be removed by this operation.
bool nativeFighterAnimation_DeleteKey(NativeFighterProject* project,
                                      NativeFighterAnimationClip clip, unsigned track,
                                      unsigned key);
// Evaluate a transform channel with Melee's own FOBJ interpreter. This is the
// value seen by the animation system at the requested (possibly fractional)
// frame, distinct from the encoded key's value.
bool nativeFighterAnimation_Sample(const NativeFighterProject* project,
                                    NativeFighterAnimationClip clip, unsigned track,
                                    float frame, float* value);

#ifdef __cplusplus
}
#endif

#endif
#endif
