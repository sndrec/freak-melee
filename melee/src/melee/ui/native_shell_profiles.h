#pragma once
#include <stdbool.h>
#ifdef __cplusplus
namespace Rml {
    class ElementDocument;
}
void nativeShellProfiles_Init(Rml::ElementDocument* document);
void nativeShellProfiles_Update();
void nativeShellProfiles_Close();
extern "C" {
#endif
bool nativeShellProfiles_IsOpen(void);
#ifdef __cplusplus
}
#endif
