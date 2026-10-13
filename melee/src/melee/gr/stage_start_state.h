#ifndef MELEE_GR_STAGE_START_STATE_H
#define MELEE_GR_STAGE_START_STATE_H
#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Actual source callback list storage. Registration and consumption keep the
// original linked-list order; snapshots can own every pending node explicitly.
typedef struct GroundStartCallback {
    struct GroundStartCallback* next;
    HSD_GObj* gobj;
    HSD_GObjEvent callback;
} GroundStartCallback;
typedef struct StageStartState {
    GroundStartCallback nodes[NATIVE_GOBJ_CAPACITY];
    unsigned count;
    bool started;
} StageStartState;
extern StageStartState stage_start_state;
#endif
