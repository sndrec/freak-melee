#pragma once
#include <dolphin/types.h>
#include <stdint.h>

#define FT_ACTION_HITBOX_DISTANCE_SCALE 0.003906f

// Package clip indices are local to the fighter that runs opcode 17.
enum { FT_ACTION_CLIP_ID_BASE = 0x40000000, FT_ACTION_CLIP_COUNT = 256 };
enum {
    // All six-bit opcodes are in use. Clear-all ignores its 26 payload bits.
    // Reserve a tagged payload followed by the exact script function name.
    FT_ACTION_SCRIPT_OPCODE = 64, // Editor opcode, never serialized directly.
    FT_ACTION_SCRIPT_NAME_MAX = 127,
    FT_ACTION_SCRIPT_TAG = 0x2e180,
    FT_ACTION_SCRIPT_BOUND = 0x1000000, // Set only on prepared native commands.
    FT_ACTION_MAX_WORDS = 1 + (FT_ACTION_SCRIPT_NAME_MAX + 3) / 4,
};

static inline bool ftAction_IsScript(unsigned opcode, u32 value)
{
    return opcode == 16 &&
        (value & ~(FT_ACTION_SCRIPT_NAME_MAX | FT_ACTION_SCRIPT_BOUND)) == FT_ACTION_SCRIPT_TAG;
}

static inline unsigned ftAction_ScriptWords(u32 value)
{
    return 1 + ((value & FT_ACTION_SCRIPT_NAME_MAX) + 3) / 4;
}

static inline unsigned ftAction_SourceOpcode(u32 word)
{
    const unsigned opcode = word >> 26;
    const u32 value = word & 0x3ffffff;
    return !(value & FT_ACTION_SCRIPT_BOUND) && ftAction_IsScript(opcode, value)
        ? FT_ACTION_SCRIPT_OPCODE : opcode;
}
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
u32 ftActionData_NativeWord(unsigned opcode, unsigned word, u32 source);
u32 ftActionData_SourceFieldMaximum(unsigned opcode, unsigned field, unsigned width);
unsigned ftActionData_EncodeScript(const char* name, uint32_t words[FT_ACTION_MAX_WORDS]);
bool ftActionData_DecodeScript(const u32* words, unsigned count,
                               char name[FT_ACTION_SCRIPT_NAME_MAX + 1]);
#ifdef __cplusplus
}
#endif
