#ifndef MELEE_FT_NATIVE_SCRIPT_BINDING_CPU_CONTROL_H
#define MELEE_FT_NATIVE_SCRIPT_BINDING_CPU_CONTROL_H
#include "native_script_vm.h"
#include "ftcmdscript.h"
// Closed set audited against the authored CPU emitter and native enum.
static inline int nativeScriptCPUControl_CommandOperands(uint32_t command)
{
    switch(command) {
    case CpuCmd_PressA:case CpuCmd_ReleaseA:case CpuCmd_PressB:case CpuCmd_ReleaseB:
    case CpuCmd_PressX:case CpuCmd_ReleaseX:case CpuCmd_PressY:case CpuCmd_ReleaseY:
    case CpuCmd_PressR:case CpuCmd_ReleaseR:case CpuCmd_PressL:case CpuCmd_ReleaseL:
    case CpuCmd_PressZ:case CpuCmd_ReleaseZ:case CpuCmd_PressUp:case CpuCmd_ReleaseUp:
    case CpuCmd_PressDown:case CpuCmd_ReleaseDown:case CpuCmd_PressRight:case CpuCmd_ReleaseRight:
    case CpuCmd_PressLeft:case CpuCmd_ReleaseLeft:case CpuCmd_PressStart:case CpuCmd_ReleaseStart:
    case CpuCmd_ReleaseAll:case CpuCmd_Done:return 0;
    case CpuCmd_SetLstickX:case CpuCmd_SetLstickY:case CpuCmd_SetCstickX:case CpuCmd_SetCstickY:
    case CpuCmd_SetRtrigger:case CpuCmd_SetLtrigger:case CpuCmd_PressAFor:case CpuCmd_ReleaseAFor:
    case CpuCmd_PressBFor:case CpuCmd_ReleaseBFor:case CpuCmd_PressXFor:case CpuCmd_ReleaseXFor:
    case CpuCmd_PressYFor:case CpuCmd_ReleaseYFor:case CpuCmd_WaitFor:
    case CpuCmd_LstickTowardDestination:case CpuCmd_LstickXTowardDestination:case CpuCmd_LstickXForward:
    case CpuCmd_WaitIfMotionId:case CpuCmd_Unk0x93:case CpuCmd_LstickTowardFighter:case CpuCmd_LstickXTowardFighter:return 1;
    case CpuCmd_LstickTowardDestinationClamped:case CpuCmd_LstickXTowardDestinationClamped:case CpuCmd_LstickForwardClamped:return 2;
    default:return -1;
    }
}
bool nativeScriptBindingCPUControl_Read(NativeScriptRuntimeContext*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingCPUControl_Write(NativeScriptRuntimeContext*,uint32_t,const NativeScriptValue*,NativeScriptDiagnostic*);
bool nativeScriptBindingCPUControl_Invoke(NativeScriptRuntimeContext*,uint32_t,NativeScriptValue*,uint32_t,NativeScriptValue*,NativeScriptDiagnostic*);
#endif
