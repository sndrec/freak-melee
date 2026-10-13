#ifndef MELEE_GR_STAGE_GENERATOR_STATE_H
#define MELEE_GR_STAGE_GENERATOR_STATE_H
#include "types.h"

enum { STAGE_GENERATOR_ENTRIES=81 };
typedef struct StageGeneratorControl {
    grZakoGenerator_SpawnDesc* x0;
    grZakoGenerator_Data* x4;
    s16 x8;
    u8 xA_b0:1,xA_b1:1,xA_b2:1,xA_b3:1,xA_b4:1,xA_b5:1,xA_b6:1,xA_b7:1;
} StageGeneratorControl;
typedef struct StageGeneratorState {
    StageGeneratorControl control;
    grZakoGenerator_Data table;
    HSD_GObj* driver;
} StageGeneratorState;
extern StageGeneratorState stage_generator_state;
#endif
