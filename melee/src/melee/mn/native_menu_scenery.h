#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
// Original ISO menu scenery, shared by startup and developer inspection.
// Loading/unloading happens at menu transitions; drawing uses the authored camera.
void nativeMenuScenery_Load(const char* preset);
bool nativeMenuScenery_Draw(void);
void nativeMenuScenery_Unload(void);
#ifdef __cplusplus
}
#endif
