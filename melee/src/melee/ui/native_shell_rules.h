#pragma once

#include <stdbool.h>

#ifdef __cplusplus
namespace Rml {
class ElementDocument;
}
void nativeShellRules_Init(Rml::ElementDocument* document);
void nativeShellRules_Update();
void nativeShellRules_Close();
extern "C" {
#endif
bool nativeShellRules_IsOpen(void);
#ifdef __cplusplus
}
#endif
