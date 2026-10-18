#pragma once
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef enum NativeSpectatorState {
    NativeSpectator_Idle,
    NativeSpectator_Requested,
    NativeSpectator_Queued,
    NativeSpectator_Armed,
    NativeSpectator_Playing,
    NativeSpectator_Results,
    NativeSpectator_Stopped,
    NativeSpectator_Failed,
} NativeSpectatorState;
bool nativeSpectatorMatch_Request(void);
void nativeSpectatorMatch_Stop(void);
bool nativeSpectatorMatch_QueueScene(void);
void nativeSpectatorMatch_LoadScene(void* unused);
void nativeSpectatorMatch_Prepare(bool sudden_death);
bool nativeSpectatorMatch_Init(void);
// 1: one frame ready, 0: wait for input history, -1: leave the scene.
int nativeSpectatorMatch_PollFrame(void);
bool nativeSpectatorMatch_EndFrame(void);
bool nativeSpectatorMatch_EndPhase(bool continues);
void nativeSpectatorMatch_Finish(void);
NativeSpectatorState nativeSpectatorMatch_State(void);
bool nativeSpectatorMatch_HideFrame(void);
bool nativeSpectatorMatch_HasFailed(void);
void nativeSpectatorMatch_Inspect(void);
#ifdef __cplusplus
}
#endif
