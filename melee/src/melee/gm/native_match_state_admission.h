#ifndef MELEE_GM_NATIVE_MATCH_STATE_ADMISSION_H
#define MELEE_GM_NATIVE_MATCH_STATE_ADMISSION_H
#include <stdbool.h>
#include <stddef.h>
// Construction-time ownership preflight and root admission. Scratch is temporary;
// the admitted owners are their live backing, never copies of the scratch data.
bool nativeMatchState_Admit(void* scratch, size_t bytes);
#endif
