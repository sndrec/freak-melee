#ifndef MELEE_FT_CHARA_FTKOOPA_TYPES_H
#define MELEE_FT_CHARA_FTKOOPA_TYPES_H

#include <Runtime/platform.h>

struct ftKoopa_FighterVars {
    /* 0x222C */ float x222C;
    /* 0x2230 */ float x2230;
};

union ftKoopa_MotionVars {
    // These are word-sized flags/counters, not host pointers or C booleans.
    // Common throw callbacks read the same slots through other union members.
    /// @todo Proper state name.
    struct ftKoopa_State1Vars {
        s32 x0;
        u32 x4;
        s32 x8;
        s32 xC;
    } unk1;
    /// @todo Possibly #ftKoopa_State1Vars.
    struct ftKoopa_SpecialSVars {
        /* fp+2340 */ s32 b_held;
        /* fp+2344 */ u32 x4;
        /* fp+2348 */ int facing_dir;
        /* fp+234C */ s32 xC;
        /* fp+2350 */ s32 x10;
        /* fp+2354 */ s32 x14;
        /* fp+2358 */ s32 x18;
    } specials;
};

STATIC_ASSERT(offsetof(union ftKoopa_MotionVars, unk1.x4) == 4);
STATIC_ASSERT(offsetof(union ftKoopa_MotionVars, unk1.xC) == 12);
STATIC_ASSERT(offsetof(union ftKoopa_MotionVars, specials.x10) == 16);

typedef struct _ftKoopaAttributes {
    float x0;
    s32 x4;
    float x8;
    float xC;
    float x10;
    float x14;
    float x18;
    float x1C;
    s32 x20;
    float x24;
    float x28;
    u32 x2C;
    float x30;
    float x34;
    float x38;
    float x3C;
    float x40;
    float x44;
    float x48;
    float x4C;
    u32 unk50;
    float x54;
    float x58;
    float x5C;
    float x60;
    float x64;
    float x68;
    float x6C;
    float x70;
    float x74;
    float x78;
    float x7C;
    float x80;
    float x84;
    float x88;
    float x8C;
    float x90;
    float x94;
    float x98;
    float x9C;
} ftKoopaAttributes;

typedef struct _ftKoopaVars {
    float x0;
    float x4;
} ftKoopaVars;

#endif
