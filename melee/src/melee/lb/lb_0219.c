/**
 * @file lb_0219.c
 * @brief Data-driven animated screen flashes.
 *
 * Plays the colour animations stored in LbBf.dat over the background flash
 * overlay owned by lbbgflash.c.
 */

#include "lb_0219.h"

#include <placeholder.h>

#include "lb_013B.h"
#include "lbarchive.h"
#include "lbbf_assets.h"
#include "lbfile.h"
#include "lbheap.h"
#include "lbbgflash.h"
#include "types.h"
#include <dolphin/gx/GXStruct.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>

HSD_ObjAllocData lbl_804336A0;

void fn_800219E4(void* arg0)
{
    HSD_ObjFree(&lbl_804336A0, arg0);
}

typedef struct BgFlashUserData {
    u8 x0;
    u8 pad_01[3];
    ColorOverlay x4;
} BgFlashUserData;

HSD_GObj* lbl_804D63E0;
struct Fighter_804D653C_t* lbl_804D63DC;
f32 lbl_804D63D8;

void lbBgFlash_80021A10(f32 arg8)
{
    lbl_804D63D8 = arg8;
}

void lbBgFlash_80021A18(int arg0)
{
    HSD_GObj* gobj;
    BgFlashUserData* user_data;

#ifdef MELEE_NATIVE
    HSD_ObjAllocInit(&lbl_804336A0, sizeof(BgFlashUserData), 4);
#else
    HSD_ObjAllocInit(&lbl_804336A0, 0x84, 4);
#endif
    gobj = GObj_Create(0xE, 0xE, 0);
    if (gobj != NULL) {
        user_data = HSD_ObjAlloc(&lbl_804336A0);
        if (user_data != NULL) {
            GObj_InitUserData(gobj, 0xE, fn_800219E4, user_data);
            lbl_804D63E0 = gobj;
            lbl_804D63D8 = 1.0f;
            user_data->x0 = (u8) arg0;
#ifdef MELEE_NATIVE
            size_t length = lbFileGetSize("LbBf.dat");
            void* bytes = lbHeap_80015BD0(0, OSRoundUp32B(length));
            LbBfAssets* assets = lbHeap_80015BD0(0, sizeof(*assets));
            HSD_ASSERT(0, bytes != NULL && assets != NULL);
            lbFile_8001668C("LbBf.dat", bytes, &length);
            HSD_ASSERTREPORT(0, lbBfAssets_Import(bytes, length, assets),
                             "Invalid native flash DAT: %d at %lx",
                             assets->status, assets->error_offset);
            assets->owned_bytes = bytes;
            lbl_804D63DC = assets->entries;
#else
            lbArchive_LoadSymbols("LbBf.dat", &lbl_804D63DC,
                                  "lbBgFlashColAnimData", NULL);
#endif
            lbBgFlash_800208EC(6);
            fn_80021C1C();
            HSD_GObj_SetupProc(gobj, (HSD_GObjEvent) fn_80021B04, 1);
            return;
        }
        HSD_GObjFree(gobj);
    }
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
void fn_80021B04(HSD_GObj* gobj)
{
    BgFlashUserData* data = gobj->user_data;
    int was_active = data->x4.x7C_color_enable;
    GXColor color;
    f32 scale;

    PAD_STACK(4);

    fn_80021C80(gobj);
    if (data->x4.x7C_color_enable) {
        u8* bytes = (u8*) &color;
        scale = lbl_804D63D8;
        bytes[0] = (u8) ((f32) data->x4.x2C_hex.r * scale);
        // This is bytes[1] to bytes[3]
        ((u8*) (&color))[1] = (u8) (((f32) data->x4.x2C_hex.g) * scale);
        ((u8*) (&color))[2] = (u8) (((f32) data->x4.x2C_hex.b) * scale);
        ((u8*) (&color))[3] = (data->x4.x2C_hex.a * data->x0) / 255;
        lbBgFlash_InitState(&color);
        return;
    }
    if (was_active) {
        fn_800208B0(0);
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif

static void fn_80021C18(HSD_GObj* gobj, CommandInfo* cmd, int arg2) {}

void fn_80021C1C(void)
{
    BgFlashUserData* data = lbl_804D63E0->user_data;
    lb_80014498(&data->x4);
}

void lbBgFlash_80021C48(u32 arg0, u32 arg1)
{
    BgFlashUserData* data = lbl_804D63E0->user_data;
    lb_800144C8(&data->x4, lbl_804D63DC, arg0, arg1);
}

void fn_80021C80(HSD_GObj* gobj)
{
    struct {
        u8 unk0[4];
        ColorOverlay x4;
    }* user_data = gobj->user_data;

    while (lb_80014258(gobj, &user_data->x4, fn_80021C18)) {
        lb_80014498(&user_data->x4);
    }
}
