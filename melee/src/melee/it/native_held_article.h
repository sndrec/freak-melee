#ifndef MELEE_IT_NATIVE_HELD_ARTICLE_H
#define MELEE_IT_NATIVE_HELD_ARTICLE_H
#include <melee/ft/forward.h>
#include <stdint.h>
#include <stdbool.h>

// True means authored policy (or an invalid retained identity) claimed the
// operation. A refused authored request must never fall through to stock policy.
bool nativeHeldArticle_Status(Fighter_GObj*, int32_t* status);
bool nativeHeldArticle_Request(Fighter_GObj*, int32_t action,
                               float frame_rate, float duration);
// Notify the actual article author before this holder enters neutral. False
// means absent policy or invalidated item/source; it never requests fallback.
// Optional claimed distinguishes absent policy from a handler that removed its
// item. Only an unclaimed operation permits the caller's ordinary stock path.
bool nativeHeldArticle_NotifyNeutral(Fighter_GObj*, bool* claimed);
#endif
