#ifndef MELEE_FT_NATIVE_FIGHTER_RIG_H
#define MELEE_FT_NATIVE_FIGHTER_RIG_H
#include "ftparts.h"
#include <sysdolphin/baselib/dat.h>
#include <stdint.h>

enum { NATIVE_RIG_HASH_SLOTS = 512, NATIVE_RIG_EXTRA = 1 };
#define NATIVE_RIG_ROOT "Freak_RigBones"
typedef struct NativeFighterRig {
    uint32_t next_id;
    uint16_t count;
    uint16_t mapped;
    bool authored;
    uint32_t ids[MAX_FT_PARTS];
    uint8_t flags[MAX_FT_PARTS];
    char names[MAX_FT_PARTS][64];
    uint32_t keys[NATIVE_RIG_HASH_SLOTS];
    int16_t parts[NATIVE_RIG_HASH_SLOTS];
} NativeFighterRig;

#ifdef __cplusplus
extern "C" {
#endif
// Immutable owned-rig data, admitted during scene construction. When a fighter
// has no authored bone table its original part numbers are its initial IDs.
// A removed ID stays missing; new bones take monotonically increasing IDs.
bool nativeFighterRig_Read(const HSD_Dat*, unsigned parts, NativeFighterRig*,
    char* error, size_t error_size);
// These translate identifiers, independently of whether a runtime joint is
// currently present. The caller still checks its admitted live parts array.
static inline int nativeFighterRig_Resolve(const NativeFighterRig* rig, int64_t id)
{
    if (!rig || id < 0 || id > INT32_MAX) return -1;
    if (!rig->authored) return (int)id;
    unsigned at = ((uint32_t)id * UINT32_C(2654435761)) & (NATIVE_RIG_HASH_SLOTS - 1);
    for (unsigned probe = 0; probe < NATIVE_RIG_HASH_SLOTS; ++probe) {
        if (rig->keys[at] == (uint32_t)id) return rig->parts[at];
        if (rig->keys[at] == UINT32_MAX) return -1;
        at = (at + 1) & (NATIVE_RIG_HASH_SLOTS - 1);
    }
    return -1;
}
static inline int32_t nativeFighterRig_Id(const NativeFighterRig* rig, unsigned part)
{
    if (!rig || part > INT32_MAX) return -1;
    return !rig->authored ? (int32_t)part :
        part < MAX_FT_PARTS && rig->ids[part] != UINT32_MAX ? (int32_t)rig->ids[part] : -1;
}
int32_t nativeFighterRig_Find(const NativeFighterRig*, const char* name);
// Rebuild the derived lookup after an authoring edit. Output stays unchanged on
// failure. All live IDs and nonempty names must be unique.
bool nativeFighterRig_Index(NativeFighterRig*, char* error, size_t error_size);
struct NativeFighterProject;
// Authoring transaction; the fighter DAT owns this table across costumes.
bool nativeFighterRig_Write(struct NativeFighterProject*, const NativeFighterRig*,
    char* error, size_t error_size);
#ifdef __cplusplus
}
#endif
#endif
