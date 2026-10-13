#ifndef MELEE_DB_NATIVE_CONTROL_H
#define MELEE_DB_NATIVE_CONTROL_H

#include <dolphin/types.h>

void dbNativeControl_Init(void);
void dbNativeControl_Poll(void);
void dbNativeControl_OnInputSample(void);
void dbNativeControl_Shutdown(void);

#endif
