#ifndef MELEE_LB_DYNAMICS_STATE_INTERNAL_H
#define MELEE_LB_DYNAMICS_STATE_INTERNAL_H
#include "types.h"

// Original engine ceilings. Every source chain and wind borrower addresses this
// actual storage, not a mirror or a second allocator.
enum { DYNAMICS_NODE_COUNT=320, WIND_FIELD_COUNT=8 };
typedef struct NativeDynamicsState {
    struct DynamicsData nodes[DYNAMICS_NODE_COUNT];
    struct lb_80011A50_t wind[WIND_FIELD_COUNT];
    struct DynamicsData* free_nodes;
    struct lb_80011A50_t* free_wind;
    struct lb_80011A50_t* active_wind;
    enum_t wind_status;
    u8 disabled;
} NativeDynamicsState;
extern NativeDynamicsState dynamics_state;
#endif
