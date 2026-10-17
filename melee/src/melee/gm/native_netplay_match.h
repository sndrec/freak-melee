#ifndef MELEE_GM_NATIVE_NETPLAY_MATCH_H
#define MELEE_GM_NATIVE_NETPLAY_MATCH_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stdint.h>

#include "native_confirmed_checkpoint.h"

bool nativeNetplayMatch_Prepare(bool sudden_death);
bool nativeNetplayMatch_Init(void);
struct MatchEnd;
void nativeNetplayMatch_Shutdown(bool continue_into_sudden_death,
                                 const struct MatchEnd* result);
bool nativeNetplayMatch_BeginFrame(uint64_t* frame);
NativeConfirmedCheckpointStep nativeNetplayMatch_EndFrame(uint64_t frame,
                                                      uint64_t* next_frame, bool ending);
void nativeNetplayMatch_Abort(const char* reason);
bool nativeNetplayMatch_IsResimulating(void);
bool nativeNetplayMatch_HasFailed(void);
bool nativeNetplayMatch_ConsumeLobbyReturn(void);
void nativeNetplayMatch_Inspect(void);

#endif
#endif
