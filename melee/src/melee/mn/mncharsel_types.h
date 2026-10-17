#pragma once
#include <Runtime/platform.h>
#include <sysdolphin/baselib/forward.h>

// Runtime objects shared by the stock CSS presentation and native interaction.
// The address-derived member names remain in the decompiled menu code.
typedef struct CSSCursorData {
    HSD_GObj* gobj;
    u8 x4; // Physical controller port.
    u8 x5; // 0/2: empty hand, 1: carrying, 3: disconnected.
    u8 x6; // Carried cookie's player slot.
    u8 x7;
    u16 x8;      // Hover feedback.
    u16 xA;      // Back-button hold timer.
    f32 xC, x10; // Hand position in menu world coordinates.
} CSSCursorData;
typedef struct CSSCharModel {
    HSD_GObj* gobj;
    u8 x4;        // Player slot.
    u8 x5;        // 0: placed; otherwise carrying controller port + 1.
    u8 x6;        // Displayed player/team color.
    u8 x7;        // Material animation tick.
    f32 x8, xC;   // Cookie target position.
    f32 x10, x14; // Cookie displayed position.
} CSSCharModel;
