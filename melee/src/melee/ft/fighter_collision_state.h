#ifndef MELEE_FT_FIGHTER_COLLISION_STATE_H
#define MELEE_FT_FIGHTER_COLLISION_STATE_H
#include "types.h"
// Source collision logs can be borrowed by Fighter.dmg fields. Keep the complete
// small arrays resident and copied, even when their append counters are reset.
typedef struct FighterCollisionState {
    DmgLogEntry primary[20],secondary[20];
    int primary_count,secondary_count;
    s8 fighter_flags[8];
} FighterCollisionState;
extern FighterCollisionState fighter_collision_state;
#endif
