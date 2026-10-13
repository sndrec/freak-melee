#ifndef MELEE_LB_NATIVE_DYNAMICS_SNAPSHOT_H
#define MELEE_LB_NATIVE_DYNAMICS_SNAPSHOT_H
#include "dynamics_state_internal.h"
#include <stdint.h>

// Retained physical wind slot, not a per-request generation: original priority
// replacement intentionally reuses the same slot and existing caller aliases.
typedef struct NativeWindReference {
    uint64_t epoch;
    uint32_t slot,reserved;
} NativeWindReference;
bool nativeDynamics_WindReference(const struct lb_80011A50_t*,NativeWindReference*);
struct lb_80011A50_t* nativeDynamics_ResolveWind(NativeWindReference);

typedef struct NativeDynamicsSnapshot {
    uint64_t epoch;
    NativeDynamicsState state;
} NativeDynamicsSnapshot;
// Called after the original reset and at match exit. No allocations or gameplay
// callbacks. Reset/reinitialization retires all previous owner snapshots.
void nativeDynamics_BeginScene(void);
void nativeDynamics_EndScene(void);
bool nativeDynamics_Save(void*,size_t capacity,size_t* written);
bool nativeDynamics_Validate(const void*,size_t bytes);
bool nativeDynamics_Load(const void*,size_t bytes);
#endif
