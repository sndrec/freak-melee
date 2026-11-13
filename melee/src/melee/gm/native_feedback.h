#pragma once
#include <stdbool.h>

union SDL_Event;
#ifdef __cplusplus
extern "C" {
#endif
bool nativeFeedback_Event(const union SDL_Event* event);
bool nativeFeedback_Open(void);
bool nativeFeedback_IsOpen(void);
bool nativeFeedback_PausesSimulation(void);
void nativeFeedback_Update(void);
void nativeFeedback_Shutdown(void);
#ifdef __cplusplus
}
#endif
