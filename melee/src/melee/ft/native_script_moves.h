#ifndef MELEE_FT_NATIVE_SCRIPT_MOVES_H
#define MELEE_FT_NATIVE_SCRIPT_MOVES_H
#include "native_script_program.h"
#include "native_script_events.h"

enum { NativeScriptMove_Neutral,NativeScriptMove_Side,NativeScriptMove_Up,NativeScriptMove_Down,
    NativeScriptMove_Count };
enum { NativeScriptMotion_Animation,NativeScriptMotion_Interrupt,NativeScriptMotion_Physics,
    NativeScriptMotion_Collision,NativeScriptMotion_Accessory,NativeScriptMotion_Count };
// Immutable numeric layout copied from the installed fighter's owned definition.
// No native callback address, inherited behavior ID or asset pointer is stored.
typedef struct NativeScriptMoveLayout {
    uint32_t common_count,first,count;
    uint32_t ground[NativeScriptMove_Count],air[NativeScriptMove_Count];
    uint32_t transition_flags[NativeScriptMove_Count];
} NativeScriptMoveLayout;
bool nativeScriptMoves_Validate(const NativeScriptMoveLayout*,NativeScriptDiagnostic*);
bool nativeScriptMoves_Contains(const NativeScriptMoveLayout*,uint32_t motion);
bool nativeScriptMoves_Metadata(void*,const NativeScriptSourceTree*,uint32_t node,NativeScriptDiagnostic*);
// 1 is a literal, 0 unrelated, -1 invalid authored layout/context.
int nativeScriptMoves_TryResolve(const NativeScriptMoveLayout*,const NativeScriptProgramIndex*,
    uint32_t function,const NativeScriptIRArena*,const NativeScriptIRNode*,NativeScriptIRResolution*,NativeScriptDiagnostic*);
typedef struct NativeScriptMotionTarget {
    uint32_t motions[2],count,phase;
} NativeScriptMotionTarget;
bool nativeScriptMoves_Target(const NativeScriptMoveLayout*,const NativeScriptSourceTree*,
    const NativeScriptSourceNode* callback,NativeScriptMotionTarget*,NativeScriptDiagnostic*);
#endif
