#ifndef MELEE_FT_NATIVE_SCRIPT_RUNTIME_H
#define MELEE_FT_NATIVE_SCRIPT_RUNTIME_H
#include "native_script_event_payload.h"
#include "native_script_vm.h"
#include "native_script_receiver.h"
enum { NATIVE_SCRIPT_RUNTIME_REGISTERS=65536, NATIVE_SCRIPT_RUNTIME_FRAMES=64 };
// Shared synchronous execution scratch; resident programs/actor state are owned
// separately. Native reentry shares this stack and must leave it balanced.
NativeScriptVM* nativeScriptRuntime_VM(void);
NativeScriptEventPayloadPool* nativeScriptRuntime_Payloads(void);
NativeScriptReceiverPool* nativeScriptRuntime_Receivers(void);
bool nativeScriptRuntime_Idle(void);
#endif
