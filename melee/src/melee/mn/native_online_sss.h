#ifndef MELEE_MN_NATIVE_ONLINE_SSS_H
#define MELEE_MN_NATIVE_ONLINE_SSS_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

typedef struct SSSData SSSData;

typedef enum NativeOnlineSssAction {
    NativeOnlineSssAction_Offline,
    NativeOnlineSssAction_OwnerInput,
    NativeOnlineSssAction_Wait,
    NativeOnlineSssAction_Start,
    NativeOnlineSssAction_Failed,
    NativeOnlineSssAction_Cancelled,
} NativeOnlineSssAction;

void nativeOnlineSss_Prepare(SSSData* sss);
bool nativeOnlineSss_IsActive(void);
NativeOnlineSssAction nativeOnlineSss_Pump(SSSData* sss,
                                           bool stage_selected,
                                           uint8_t stage);
void nativeOnlineSss_Reset(void);

#endif
#endif
