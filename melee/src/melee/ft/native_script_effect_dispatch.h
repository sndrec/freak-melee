#ifndef MELEE_FT_NATIVE_SCRIPT_EFFECT_DISPATCH_H
#define MELEE_FT_NATIVE_SCRIPT_EFFECT_DISPATCH_H
#include <stdint.h>
#include <stdbool.h>
typedef enum NativeScriptEffectArguments {
    NativeScriptEffectArguments_Unsupported,
    NativeScriptEffectArguments_Joint,
    NativeScriptEffectArguments_JointScalar,
    NativeScriptEffectArguments_Position,
    NativeScriptEffectArguments_TwoJoint,
    NativeScriptEffectArguments_PositionScalar,
    NativeScriptEffectArguments_PositionTwoScalar,
    NativeScriptEffectArguments_JointOffset,
} NativeScriptEffectArguments;
// Actual efSync -> efAsync/efAlt cases with an exact explicit argument order.
// Scalar/position pointers are consumed or copied synchronously, never retained.
// Joint also admits the audited Attach_Scale forwarder and the typed
// Attach_Scale_FacingDir model callee behind erased va_arg(void*). Both use
// actual JObj attachments; the model copies creator-root scale and updates
// facing from its live Fighter parent. No caller stack position is retained.
// TwoJoint admits only audited immediate two-HSD_JObj* constructors, including
// typed Attach callees behind erased va_arg(void*). Other erased/forwarded
// payloads remain excluded.
NativeScriptEffectArguments nativeScriptEffectDispatch_Arguments(int32_t effect);
enum NativeScriptEffectPositionRequirement {
    NativeScriptEffectPosition_Root = 1u << 0,
    NativeScriptEffectPosition_FighterParent = 1u << 1,
    NativeScriptEffectPosition_CreatesModel = 1u << 2,
};
// Exact Position ABI is separate from native creator-context requirements.
// Root reads creator JObj scale during construction; FighterParent installs a
// later update reading actual Fighter userdata. CreatesModel alone needs no
// creator root. Unsupported ABI/NULL output returns false without publication.
bool nativeScriptEffectDispatch_PositionRequirements(int32_t effect,
                                                     uint32_t* requirements);
bool nativeScriptEffectDispatch_PositionScalarRequirements(int32_t effect,
                                                           uint32_t* requirements);
bool nativeScriptEffectDispatch_PositionTwoScalarRequirements(int32_t effect,
                                                             uint32_t* requirements);
#endif
