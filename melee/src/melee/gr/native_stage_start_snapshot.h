#ifndef MELEE_GR_NATIVE_STAGE_START_SNAPSHOT_H
#define MELEE_GR_NATIVE_STAGE_START_SNAPSHOT_H
#include "stage_start_state.h"
#include <melee/gm/native_match_snapshot_pool.h>

typedef struct NativeStageStartSnapshotHeader {
    uint64_t epoch;
    uint32_t count,bytes,started,reserved;
} NativeStageStartSnapshotHeader;
typedef struct NativeStageStartSnapshotRecord {
    NativeMatchPoolReference object;
    HSD_GObjEvent callback;
} NativeStageStartSnapshotRecord;
// Bind immutable callback definitions to a new real GObj scene. This resets
// pending native storage; it does not execute any source callback.
bool nativeStageStart_BeginScene(void);
bool nativeStageStart_AdmitCallback(HSD_GObjEvent);
size_t nativeStageStart_SnapshotBytes(void);
bool nativeStageStart_Save(void*,size_t,size_t*);
bool nativeStageStart_Validate(const void*,size_t);
bool nativeStageStart_ValidateObjects(const void*,size_t,NativeMatchPoolSnapshotView);
bool nativeStageStart_Load(const void*,size_t);
#endif
