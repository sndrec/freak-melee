#include "native_fighter_held_motion.h"
#include "native_fighter_runtime.h"
#include "native_callback_lifetime.h"
#include "native_fighter_lifetime.h"
#include "native_script_bindings.h"
#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <sysdolphin/baselib/gobj.h>

int32_t nativeFighterHeldMotion_Select(Fighter_GObj* object,
                                      int32_t common_motion,
                                      int32_t previous_motion)
{
    uint32_t generation;
    if (!nativeCallbackSource_Snapshot(object, NativeCallbackSource_Fighter, &generation)) return -1;
    Fighter* fighter = object->user_data;
    NativeFighterLifetime lifetime;
    if (!nativeFighterLifetime_Capture(fighter, &lifetime)) return -1;
    if (lifetime.source_owned) {
        NativeScriptRuntimeContext context={.source=object,.source_class=NativeCallbackSource_Fighter,
            .generation=generation,.scene_epoch=lifetime.reference.lifetime};
        const NativeScriptMoveLayout* layout=nativeScriptBindings_SourceFighterLayout(&context,NULL);
        if(layout!=&lifetime.program->bank->motion_layout || common_motion<0 ||
           !nativeScriptMoves_Contains(layout,(uint32_t)common_motion))return -1;
        if (!nativeScriptFighter_HasHook(object, NativeScriptFighter_HeldMotion))
            return common_motion;
        NativeScriptValue arguments[2] = {
            {.type=NativeScriptType_I32,.as.i64=common_motion},
            {.type=NativeScriptType_I32,.as.i64=previous_motion}};
        NativeScriptValue result;
        if (nativeScriptFighter_Run(object, NativeScriptFighter_HeldMotion,
                arguments, 2, NULL, &result) != NativeScriptVM_Ok ||
            !nativeFighterLifetime_Current(&lifetime) ||
            nativeScriptBindings_SourceFighterLayout(&context,NULL)!=layout) return -1;
        int32_t selected = (int32_t)result.as.i64;
        return selected >= 0 && lifetime.program->bank->has_motion_layout &&
            nativeScriptMoves_Contains(&lifetime.program->bank->motion_layout,
                                      (uint32_t)selected) ? selected : common_motion;
    }
    FighterKind kind = fighter->kind;
    const NativeFighterDefinition* definition = nativeFighterRuntime_Definition(kind);
    if (definition != NULL && definition->on_held_motion != NULL) {
        if (!nativeCallbackDispatch_Enter()) return -1;
        int32_t selected = definition->on_held_motion(object, common_motion,
                                                     previous_motion);
        nativeCallbackDispatch_Leave();
        if (!nativeCallbackSource_Alive(object, generation, NativeCallbackSource_Fighter) ||
            fighter->kind != kind || nativeFighterRuntime_Definition(kind) != definition) return -1;
        if (selected >= 0 && (selected < ftCo_MS_Count ||
            (selected >= definition->motion_state_first &&
             (uint32_t)(selected - definition->motion_state_first) < definition->motion_state_count)))
            return selected;
        return common_motion;
    }
    // Raw stock policy remains here only when no authored policy is declared.
    // This tests the original saved article pointer, not the current held slot.
    if (kind == Ft_Kind_Peach && fighter->u.pe.parasol_gobj_0 != NULL) {
        if (common_motion == ftCo_MS_ItemParasolOpen) return ftPe_MS_ItemParasolOpen;
        if (common_motion == ftCo_MS_ItemParasolFallSpecial) return ftPe_MS_ItemParasolFall;
    }
    return common_motion;
}
