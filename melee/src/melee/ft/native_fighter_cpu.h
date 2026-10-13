#ifndef MELEE_FT_NATIVE_FIGHTER_CPU_H
#define MELEE_FT_NATIVE_FIGHTER_CPU_H

#include "ftcpu_profile.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeFighterCpuProfile {
    ftCo_AttackEntry attacks[FT_CPU_ATTACK_FAMILIES][FT_CPU_ATTACK_MAX_ENTRIES + 1];
    float distance_threshold;
} NativeFighterCpuProfile;

// Fixed storage; parsing/import happens only when opening a project or match.
bool nativeFighterCpu_Parse(const char* text, size_t size,
                            NativeFighterCpuProfile* profile);
bool nativeFighterCpu_Write(FILE* output, const NativeFighterCpuProfile* profile);
bool nativeFighterCpu_ImportStock(const void* bytes, size_t size, FighterKind kind,
                                  NativeFighterCpuProfile* profile);

#ifdef __cplusplus
}
#endif

#endif
