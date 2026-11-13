#pragma once
#include "mncharsel_types.h"
#include "native_css_pointer.h"
#include <melee/mn/forward.h>
#include <melee/sc/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativeCssInteraction_Begin(CSSData* css, CSSDoorsData* doors,
                                CSSCursorData** hands, CSSCharModel** cookies,
                                CSSIcon* icons, HSD_JObj* menu,
                                const StaticModelDesc* source);
void nativeCssInteraction_End(void);
void nativeCssInteraction_DrawLocalCursor(HSD_GObj* object, int pass);
// Consumes native cookie input after the stock menu moves its hand.
// Other controls (CPU slots, costumes and sliders) keep their existing input.
bool nativeCssInteraction_Hand(CSSCursorData* hand, u32* trigger);
void nativeCssInteraction_Cookie(CSSCharModel* cookie, HSD_JObj* joint);
bool nativeCssInteraction_CanReady(void);
bool nativeCssInteraction_SetLocalPlayers(unsigned count);
bool nativeCssInteraction_Random(unsigned slot);
void nativeCssInteraction_Inspect(void);
void nativeCssInteraction_EnablePointer(bool enabled);
void nativeCssInteraction_CancelPointer(void);
bool nativeCssInteraction_Pointer(float u, float v,
                                  NativeCssPointerEvent event);
bool nativeCssInteraction_PointerPort(unsigned port);
#ifdef __cplusplus
}
#endif
