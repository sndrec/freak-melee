#include <melee/it/native_script_item_actor.h>
#include <melee/ft/native_script_receiver.h>
// The Fighter fixture has no Item actors. Cross-class lookups remain absent.
bool nativeScriptItem_View(Item_GObj* actor,uint32_t generation,NativeScriptReceiverView* view)
{ (void)actor;(void)generation;(void)view;return false; }
bool nativeScriptItem_HasActors(FighterKind kind) { (void)kind;return false; }
