#ifndef MELEE_GM_NATIVE_MATCH_CONTROL_HISTORY_H
#define MELEE_GM_NATIVE_MATCH_CONTROL_HISTORY_H
#include "native_match_arena.h"
// Scene construction only. The caller prevalidates the complete current owner
// inventory once; these methods admit actual mutable backing, never mirrors.
bool nativeRandom_AdmitHistory(NativeMatchArena*);
bool nativeMemory_AdmitHistory(NativeMatchArena*);
bool nativeID_AdmitHistory(NativeMatchArena*);
bool nativeGmInput_AdmitHistory(NativeMatchArena*);
bool nativeGmLoop_AdmitHistory(NativeMatchArena*);
bool nativeInputClock_AdmitHistory(NativeMatchArena*);
bool nativePlayer_AdmitHistory(NativeMatchArena*);
bool nativeCameraSubjects_AdmitHistory(NativeMatchArena*);
bool nativeCamera_AdmitHistory(NativeMatchArena*);
bool nativeStage_AdmitHistory(NativeMatchArena*);
bool nativeStageHazards_AdmitHistory(NativeMatchArena*);
bool nativeStageGenerator_AdmitHistory(NativeMatchArena*);
bool nativeStageSpawn_AdmitHistory(NativeMatchArena*);
bool nativeStageStart_AdmitHistory(NativeMatchArena*);
bool nativeDynamics_AdmitHistory(NativeMatchArena*);
bool nativeEffectControl_AdmitHistory(NativeMatchArena*);
bool nativeParticleControl_AdmitHistory(NativeMatchArena*);
bool nativeLightControl_AdmitHistory(NativeMatchArena*);
bool nativeMaterialTexture_AdmitHistory(NativeMatchArena*);
bool nativeVersusControl_AdmitHistory(NativeMatchArena*);
bool nativeController_AdmitHistory(NativeMatchArena*);
bool nativeRObj_AdmitHistory(NativeMatchArena*);
bool nativeTextureIdentity_AdmitHistory(NativeMatchArena*);
bool nativeMatchControls_AdmitHistory(NativeMatchArena*);
#endif
