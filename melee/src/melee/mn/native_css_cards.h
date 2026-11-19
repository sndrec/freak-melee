#pragma once
#include "native_css_participants.h"
#include <dolphin/mtx.h>
#include "mncharsel_types.h"
#include <melee/mn/forward.h>
#include <melee/sc/forward.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativeCssCards_Begin(CSSData* css, CSSDoorsData* doors, HSD_JObj* menu,
                          const StaticModelDesc* source);
void nativeCssCards_End(void);
void nativeCssCards_SetRulesText(HSD_Text* stock, HSD_Text* lobby);
void nativeCssCards_Update(void);
bool nativeCssCards_Active(void);
unsigned nativeCssCards_DisplayPort(unsigned physical_port);
bool nativeCssCards_HasLocalCard(unsigned physical_port);
bool nativeCssCards_RemotePort(const char* peer_id, unsigned seat, unsigned* display);
const NativeCssParticipant* nativeCssCards_Hit(float x, float y);
bool nativeCssCards_ChangeSeat(float x, float y, unsigned physical_port);
void nativeCssCards_Carry(unsigned port, const NativeContentId* package,
                          float x, float y);
void nativeCssCards_InvalidateArt(void);
void nativeCssCards_Inspect(void);
typedef struct NativeCssCardPreview {
    float x, y, width, height;
} NativeCssCardPreview;
// Original P1 card artwork on a native menu canvas. Returns its portrait area.
bool nativeCssCards_DrawPreview(Mtx canvas, float x, float y, float width,
                                 NativeCssCardPreview* portrait);
#ifdef __cplusplus
}
#endif
