#ifndef MELEE_GM_NATIVE_SNAPSHOT_CENSUS_H
#define MELEE_GM_NATIVE_SNAPSHOT_CENSUS_H
#include <stdbool.h>
#include <stdint.h>

// Offline observation of existing owners at a complete post-draw boundary.
// Optional self-restore/bounded older-capture experiment; no complete candidate
// closure or full-match performance claim. Optional replay-only rendered
// resimulation compares its own compact endpoint under a broad recovery guard.
// Scratch and
// diagnostics belong to the host and never enter game history.
bool nativeSnapshotCensus_Init(void);
void nativeSnapshotCensus_AdmitArenas(uint32_t scene);
void nativeSnapshotCensus_Sample(uint32_t scene, uint64_t frame,
                                 uint64_t input_sample);
void nativeSnapshotCensus_Abort(const char* reason);
void nativeSnapshotCensus_Close(void);
bool nativeSnapshotCensus_HasFailed(void);
bool nativeSnapshotCensus_IsResimulating(void);
bool nativeSnapshotCensus_UsesReplayJournal(void);
bool nativeSnapshotCensus_BeforeInput(void);
enum NativeCompactTimingPhase { NativeCompactTiming_Input,NativeCompactTiming_Simulation,
    NativeCompactTiming_Drawing,NativeCompactTiming_RenderBegin,NativeCompactTiming_HsdDraw,
    NativeCompactTiming_RenderEnd,NativeCompactTiming_Count };
bool nativeSnapshotCensus_TimingEnabled(void);
void nativeSnapshotCensus_BeginTiming(enum NativeCompactTimingPhase);
void nativeSnapshotCensus_EndTiming(enum NativeCompactTimingPhase);
#endif
