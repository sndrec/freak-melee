#include <melee/it/native_article.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include "actual_owner.inc"
// Legacy article/effect receivers are unavailable. Item creator capture uses
// the complete extracted native_item_owner.c bodies, not this absence boundary.
Fighter_GObj* nativeArticle_Creator(Item_GObj* object)
{ (void)object;return NULL; }
Fighter_GObj* nativeModelEffect_Creator(EF_Effect* effect)
{ (void)effect;return NULL; }
