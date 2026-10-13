#ifndef MELEE_IF_TIME_STATE_INTERNAL_H
#define MELEE_IF_TIME_STATE_INTERNAL_H
#include <sysdolphin/baselib/forward.h>
#include <melee/sc/forward.h>
typedef struct IfTimeState {
    HSD_GObj* match_timer;
    HSD_GObj* countdown_timer;
    HSD_JObj* digits[10];
    unsigned char countdown_seconds;
    char padding[7];
    struct DynamicModelDesc** countdown_timer_models;
} IfTimeState;
extern IfTimeState if_time_state;
#endif
