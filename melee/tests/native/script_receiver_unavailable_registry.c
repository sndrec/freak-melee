#include <melee/ft/native_fighter_runtime.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobjplink.h>
HSD_GObj** HSD_GObjPLinkHead;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ (void)kind;return NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* actor,uint32_t* generation)
{ (void)actor;(void)generation;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* actor,uint32_t generation)
{ (void)actor;(void)generation;return NULL; }
