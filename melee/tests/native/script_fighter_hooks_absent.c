#include <melee/ft/native_script_fighter.h>
// Generic primitive fixtures have registered engine actors, but no attached
// interpreted Fighter instance. Keep native hook branches genuinely absent.
// A fixture explicitly supplying attachment metadata provides its own admission
// query; the authored policy and scheduler operations below stay unavailable.
#ifndef SCRIPT_FIXTURE_SOURCE_ATTACHED
bool nativeScriptFighter_IsAttached(Fighter_GObj* object){(void)object;return false;}
#endif
#ifndef SCRIPT_FIXTURE_SOURCE_ACTOR
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object){(void)object;return NativeScriptVM_SourceRetired;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){(void)kind;return NULL;}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view)
{(void)object;(void)generation;(void)view;return false;}
#endif
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* object,uint32_t hook,
    const NativeScriptValue* inputs,uint32_t count, NativeScriptEventRecordIO* record,NativeScriptValue* output)
{(void)record;(void)object;(void)hook;(void)inputs;(void)count;(void)output;return NativeScriptVM_Error;}
