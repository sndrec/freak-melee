#pragma once
#include "native_css_participants.h"
#include "mncharsel_types.h"
#include <melee/mn/forward.h>
#include <melee/sc/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativeCssCards_Begin(CSSData* css, CSSDoorsData* doors, HSD_JObj* menu,
                          const StaticModelDesc* source);
void nativeCssCards_End(void);
void nativeCssCards_Update(void);
bool nativeCssCards_Active(void);
unsigned nativeCssCards_DisplayPort(unsigned physical_port);
const NativeCssParticipant* nativeCssCards_Hit(float x, float y);
void nativeCssCards_Carry(unsigned port, const NativeContentId* package,
                          float x, float y);
void nativeCssCards_InvalidateArt(void);
void nativeCssCards_Inspect(void);
#ifdef __cplusplus
}
#endif
