#ifndef MELEE_FT_NATIVE_SCRIPT_MOVE_LOWERING_H
#define MELEE_FT_NATIVE_SCRIPT_MOVE_LOWERING_H
#include "native_script_moves.h"
// Desugar authored contextual move operations into explicit typed engine calls.
// Runs once at load; bytecode never looks up a source move/name while ticking.
bool nativeScriptMoveLowering_Apply(const NativeScriptMoveLayout*,const NativeScriptProgramIndex*,
    uint32_t function,NativeScriptIRArena*,uint32_t first,NativeScriptDiagnostic*);
#endif
