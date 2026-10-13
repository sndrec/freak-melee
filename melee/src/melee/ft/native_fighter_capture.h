#ifndef MELEE_FT_NATIVE_FIGHTER_CAPTURE_H
#define MELEE_FT_NATIVE_FIGHTER_CAPTURE_H
#include "native_fighter_lifetime.h"

// Retain the existing holder link; capture entry deliberately precedes the
// victim's reciprocal link. A missing victim is represented by victim.object=0.
typedef struct NativeFighterCapturePair {
    NativeFighterLifetime holder,victim;
} NativeFighterCapturePair;
bool nativeFighterCapture_Retain(Fighter_GObj*,NativeFighterCapturePair*);
bool nativeFighterCapture_Current(const NativeFighterCapturePair*,bool require_link);
bool nativeFighterCapture_Attached(const NativeFighterCapturePair*);
bool nativeFighterCapture_AdmitMotion(const NativeFighterLifetime*,FtMotionId);
bool nativeFighterCapture_Begin(Fighter_GObj* victim,Fighter_GObj* holder,
    FtMotionId,bool same_facing,bool force_air,bool track_position,bool holder_animation);
bool nativeFighterCapture_AttachToVictim(Fighter_GObj* holder);
void nativeFighterCapture_FollowVictim(Fighter_GObj* holder);
bool nativeFighterCapture_ReleaseThrow(Fighter_GObj* holder);
#endif
