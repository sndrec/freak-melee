#ifndef GALE01_09F7F8
#define GALE01_09F7F8

#include <melee/ft/forward.h>

#include <dolphin/mtx.h>

enum {
    FT_EFFECT_ROTATING_JOINT = 0x8D,
    FT_EFFECT_MODEL_JOINT = 0x8E,
};

/* 09F834 */ void ftCo_8009F834(Fighter_GObj*, int, Fighter_Part, int, int,
                                Vec3*, Vec3*, float);
/* 0A0098 */ void ftCo_800A0098(Fighter* fp);

#endif
