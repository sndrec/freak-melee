#ifndef MELEE_PL_PLAYER_INTERNAL_H
#define MELEE_PL_PLAYER_INTERNAL_H

#include "player.h"
#include <sysdolphin/baselib/debug.h>

typedef struct ftMapping {
    s8 internal_id;
    s8 extra_internal_id;
    s8 has_transformation;
} ftMapping;

extern ftMapping ftMapping_list[ChKind_RuntimeCapacity];
extern StaticPlayer player_slots[Gm_Player_NumMax];
extern HSD_ObjAllocData Player_AllocData;
extern u16 staleAttackInstance;
extern u16 unk_804D6480;

// Shared inline bounds/mapping checks; original bodies, no forwarding.
static inline bool hasExtraFighterId(ftMapping* data)
{
    return data->extra_internal_id != -1;
}

static inline void Player_CheckSlot(int slot)
{
    if (slot < 0 || !(slot < Gm_Player_NumMax)) {
        HSD_ASSERTREPORT(102, 0, "cant get player struct! %d\n", slot);
    }
}

#endif
