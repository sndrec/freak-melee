#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
// Developer inspection of original menu scenery, rendered from the ISO. The
// preset chooses artwork, not a running trophy/tournament game mode.
void nativeMenuReference_Select(const char* preset);
bool nativeMenuReference_Draw(void);
void nativeMenuReference_Unload(void);
#ifdef __cplusplus
}
#endif
