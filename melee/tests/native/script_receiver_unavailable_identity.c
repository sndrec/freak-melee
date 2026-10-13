#include <melee/it/native_item_owner.h>
#include <melee/it/native_article.h>
#include <melee/ef/native_model_effect.h>

// These older isolated fixtures do not contain entity ownership. A receiver
// request must remain absent rather than inventing a successful native target.
Fighter_GObj* nativeItem_Creator(Item_GObj* actor) { (void)actor;return NULL; }
Fighter_GObj* nativeArticle_Creator(Item_GObj* actor) { (void)actor;return NULL; }
Fighter_GObj* nativeModelEffect_Creator(EF_Effect* actor) { (void)actor;return NULL; }
