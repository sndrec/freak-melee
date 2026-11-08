#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativePresentation_Load(void);
bool nativePresentation_CustomModels(void);
bool nativePresentation_SetCustomModels(bool show);
#ifdef __cplusplus
}
#endif
