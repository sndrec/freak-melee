#ifndef MELEE_IF_IFALL_STATE_INTERNAL_H
#define MELEE_IF_IFALL_STATE_INTERNAL_H
#include <sysdolphin/baselib/gobj.h>
#include <dolphin/mtx.h>
typedef struct IfAllState {
    HSD_GObj* gobj;
    HSD_GObj* gobj_2;
    HSD_JObj* x8;
    Vec3 xC;
    Vec3 x18[6];
    Vec3 x60[3];
    Vec3 x84[2];
} IfAllState;
extern IfAllState ifAll_804A0FD8;
extern u8 hud_hidden;
// Retained stock-indicator model followed by two live objects per player.
extern void* if_stock_indicators[13];
#endif
