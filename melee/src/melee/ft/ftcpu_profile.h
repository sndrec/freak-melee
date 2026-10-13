#ifndef MELEE_FT_FTCPU_PROFILE_H
#define MELEE_FT_FTCPU_PROFILE_H
#include "forward.h"

enum {
    FT_CPU_SCRIPT_COUNT = 62,
    FT_CPU_FIGHTER_COUNT = 32,
    FT_CPU_RUNTIME_FIGHTER_COUNT = Ft_Kind_RuntimeCapacity,
    FT_CPU_ATTACK_FAMILIES = 7,
    FT_CPU_ATTACK_MAX_ENTRIES = 32,
};

typedef struct ftCo_AttackEntry {
    s32 cmd;
    s32 x04;
    f32 x08;
    f32 x0C;
    f32 x10;
    f32 x14;
    f32 weight;
    s32 x1C;
    s32 x20;
} ftCo_AttackEntry;
#endif
