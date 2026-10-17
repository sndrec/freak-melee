#ifndef MELEE_UI_NATIVE_SHELL_COSTUMES_H
#define MELEE_UI_NATIVE_SHELL_COSTUMES_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#ifdef __cplusplus
#include <RmlUi/Core.h>
void nativeShellCostumes_Init(Rml::ElementDocument* document);
void nativeShellCostumes_Update(void);
void nativeShellCostumes_RefreshInstalled(void);
void nativeShellCostumes_Open(void);
void nativeShellCostumes_Close(void);
extern "C" {
#endif
bool nativeShellCostumes_IsOpen(void);
#ifdef __cplusplus
}
#endif

#endif
#endif
