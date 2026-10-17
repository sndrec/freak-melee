#pragma once
#include "mncharsel_types.h"
#include <melee/mn/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativeCssInteraction_Begin(CSSData* css, CSSDoorsData* doors,
                                CSSCursorData** hands, CSSCharModel** cookies,
                                CSSIcon* icons);
void nativeCssInteraction_End(void);
// Consumes native cookie input after the stock menu moves its hand.
// Other controls (CPU slots, costumes and sliders) keep their existing input.
bool nativeCssInteraction_Hand(CSSCursorData* hand);
void nativeCssInteraction_Cookie(CSSCharModel* cookie, HSD_JObj* joint);
bool nativeCssInteraction_CanReady(void);
bool nativeCssInteraction_Random(unsigned slot);
void nativeCssInteraction_Inspect(void);
#ifdef __cplusplus
}
#endif
