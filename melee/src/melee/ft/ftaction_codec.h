#pragma once
#include <dolphin/types.h>

#define FT_ACTION_HITBOX_DISTANCE_SCALE 0.003906f

// Package clip indices are local to the fighter that runs opcode 17.
enum { FT_ACTION_CLIP_ID_BASE = 0x40000000, FT_ACTION_CLIP_COUNT = 256 };
enum {
    FT_ACTION_THROW_OPCODE = 34,
    FT_ACTION_THROW_FRACTIONAL_OPCODE = 63,
    FT_ACTION_THROW_WORDS = 3,
    FT_ACTION_THROW_FRACTIONAL_WORDS = 4,
    FT_ACTION_THROW_DAMAGE_MAX = 0x7FFFFF,
};

static inline bool ftAction_IsThrowHitbox(unsigned opcode)
{
    return opcode == FT_ACTION_THROW_OPCODE ||
           opcode == FT_ACTION_THROW_FRACTIONAL_OPCODE;
}
#ifdef __cplusplus
extern "C" {
#endif
// Source bit widths run from most-significant bit to least-significant bit.
// A zero count denotes one complete payload word. Word zero begins with
// opcode.
unsigned ftActionData_SourceFieldWidths(unsigned opcode, unsigned word,
                                        u8 widths[8]);
u32 ftActionData_SourceCommandWidth(u32 source_word);
u32 ftActionData_SourceFieldMaximum(unsigned opcode, unsigned field, unsigned width);
#ifdef __cplusplus
}
#endif
