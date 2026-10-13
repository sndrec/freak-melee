#include "native_script_runtime.h"
// Host scratch must not rewind expired payload handles or stale frame capacity.
// This unit is excluded from .mstate; snapshots require an idle runtime.
static NativeScriptValue registers[NATIVE_SCRIPT_RUNTIME_REGISTERS];
static NativeScriptVMFrame frames[NATIVE_SCRIPT_RUNTIME_FRAMES];
static NativeScriptVM vm={.registers=registers,.register_capacity=NATIVE_SCRIPT_RUNTIME_REGISTERS,
    .frames=frames,.frame_capacity=NATIVE_SCRIPT_RUNTIME_FRAMES};
static NativeScriptEventPayloadFrame payload_frames[NATIVE_SCRIPT_RUNTIME_FRAMES];
static NativeScriptEventPayloadPool payloads={.frames=payload_frames,.capacity=NATIVE_SCRIPT_RUNTIME_FRAMES};
static NativeScriptReceiverRecord receiver_records[8192];
static NativeScriptReceiverFrame receiver_frames[NATIVE_SCRIPT_RUNTIME_FRAMES];
static NativeScriptReceiverPool receivers={.records=receiver_records,.capacity=8192,
    .frames=receiver_frames,.frame_capacity=NATIVE_SCRIPT_RUNTIME_FRAMES};
NativeScriptVM* nativeScriptRuntime_VM(void){return &vm;}
NativeScriptEventPayloadPool* nativeScriptRuntime_Payloads(void){return &payloads;}
NativeScriptReceiverPool* nativeScriptRuntime_Receivers(void){return &receivers;}
bool nativeScriptRuntime_Idle(void){return vm.depth==0 && vm.register_top==0 && payloads.depth==0 && receivers.depth==0 && receivers.top==0;}
