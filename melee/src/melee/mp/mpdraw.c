#include "mplib.h"

#include <Runtime/platform.h>

#include <melee/lb/forward.h>
#include <sysdolphin/baselib/forward.h>

#include <math.h>
#include <placeholder.h>
#include <stdbool.h>
#include <stddef.h>

#include "forward.h"
#include "mpcoll.h"
#include "mpisland.h"
#include "types.h"
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXStruct.h>
#include <dolphin/gx/GXVert.h>
#include <dolphin/mtx.h>
#include <dolphin/types.h>
#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/db/db.h>
#include <melee/ft/ftlib.h>
#include <melee/gr/grdynamicattr.h>
#include <melee/gr/ground.h>
#include <melee/gr/stage.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/itCharItems.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/tev.h>
#include <sysdolphin/baselib/texp.h>

#include "mplib_internal.h"

/* 4D64D0 */ static s32 mpLib_804D64D0;
/* 4D64D4 */ static s32 mpLib_804D64D4;
/* 4D64D8 */ static s32 mpLib_804D64D8;
/* 4D64DC */ static s32 mpLib_804D64DC;
/* 4D64E0 */ static s32 mpLib_804D64E0;
/* 4D64E4 */ static s32 mpLib_804D64E4;
/* 458888 */ Vec3 mpLib_80458888[0x80];

static char lbl_803BF526[0x18] = "";

static HSD_Chan mpLib_803BF540 = {
    NULL,
    GX_COLOR0A0,
    0,
    { 0, 0, 0, 0 },
    { 0xFF, 0xA0, 0x30, 0xFF },
    0,
    GX_SRC_REG,
    GX_SRC_REG,
    GX_LIGHT_NULL,
    GX_DF_CLAMP,
    GX_AF_NONE,
    NULL,
};

static char lbl_803BF570[] = "B(%d,%d)-(%d,%d)\n";

void mpLib_SetupDraw(GXColor color)
{
    HSD_TevDesc spC;
    HSD_StateInitTev();
    spC.flags = 0;
    spC.stage = HSD_StateAssignTev();
    spC.coord = 0xFF;
    spC.map = 0xFF;
    spC.color = 4;
    spC.u.tevop.tevmode = 4;
    HSD_SetupTevStage(&spC);
    HSD_SetupPEMode(1, NULL);
    HSD_SetTevRegAll();
    HSD_StateSetNumTevStages();
    HSD_StateSetNumTexGens();
    HSD_StateSetNumChans(1);
    mpLib_803BF540.mat_color = color;
    HSD_SetupChannel(&mpLib_803BF540);
}

void mpLib_DrawEcbs(CollData* cd)
{
    GXColor spE0 = { 0xFF, 0xA0, 0x30, 0xFF };
    GXColor spDC = { 0x80, 0x50, 0x18, 0xFF };
    GXColor spD8 = { 0xFF, 0x5A, 0x20, 0xFF };
    GXColor spD4 = { 0x80, 0x2D, 0x10, 0xFF };
    GXColor spD0 = { 0x20, 0x20, 0xFF, 0xFF };
    GXColor spCC = { 0x10, 0x10, 0x80, 0xFF };
    GXColor spC8 = { 0xFF, 0xFF, 0xFF, 0x80 };
    GXColor spC4 = { 0xFF, 0xFF, 0xFF, 0x80 };

    mpLib_SetupDraw(spE0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32(cd->cur_pos.x + cd->ecb.top.x,
                   cd->cur_pos.y + cd->ecb.top.y, cd->cur_pos.z);
    GXPosition3f32(cd->cur_pos.x + cd->ecb.right.x,
                   cd->cur_pos.y + cd->ecb.right.y, cd->cur_pos.z);
    GXPosition3f32(cd->cur_pos.x + cd->ecb.bottom.x,
                   cd->cur_pos.y + cd->ecb.bottom.y, cd->cur_pos.z);
    GXPosition3f32(cd->cur_pos.x + cd->ecb.left.x,
                   cd->cur_pos.y + cd->ecb.left.y, cd->cur_pos.z);

    GXEnd();

    mpLib_SetupDraw(spDC);
    GXBegin(GX_LINES, GX_VTXFMT0, 4);

    GXPosition3f32(cd->cur_pos.x - 1.0F, cd->cur_pos.y, cd->cur_pos.z);
    GXPosition3f32(1.0F + cd->cur_pos.x, cd->cur_pos.y, cd->cur_pos.z);
    GXPosition3f32(cd->cur_pos.x, cd->cur_pos.y - 1.0F, cd->cur_pos.z);
    GXPosition3f32(cd->cur_pos.x, 1.0F + cd->cur_pos.y, cd->cur_pos.z);

    GXEnd();

    mpLib_SetupDraw(spD8);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32(cd->prev_pos.x + cd->prev_ecb.top.x,
                   cd->prev_pos.y + cd->prev_ecb.top.y, cd->prev_pos.z - 0.5F);
    GXPosition3f32(cd->prev_pos.x + cd->prev_ecb.right.x,
                   cd->prev_pos.y + cd->prev_ecb.right.y,
                   cd->prev_pos.z - 0.5F);
    GXPosition3f32(cd->prev_pos.x + cd->prev_ecb.bottom.x,
                   cd->prev_pos.y + cd->prev_ecb.bottom.y,
                   cd->prev_pos.z - 0.5F);
    GXPosition3f32(cd->prev_pos.x + cd->prev_ecb.left.x,
                   cd->prev_pos.y + cd->prev_ecb.left.y,
                   cd->prev_pos.z - 0.5F);

    GXEnd();

    mpLib_SetupDraw(spD4);
    GXBegin(GX_LINES, GX_VTXFMT0, 4);

    GXPosition3f32(cd->prev_pos.x - 1.0F, cd->prev_pos.y,
                   cd->prev_pos.z - 0.5F);
    GXPosition3f32(1.0F + cd->prev_pos.x, cd->prev_pos.y,
                   cd->prev_pos.z - 0.5F);
    GXPosition3f32(cd->prev_pos.x, cd->prev_pos.y - 1.0F,
                   cd->prev_pos.z - 0.5F);
    GXPosition3f32(cd->prev_pos.x, 1.0F + cd->prev_pos.y,
                   cd->prev_pos.z - 0.5F);

    GXEnd();

    mpLib_SetupDraw(spD0);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);

    GXPosition3f32(cd->last_pos.x + cd->xE4_ecb.top.x,
                   cd->last_pos.y + cd->xE4_ecb.top.y, cd->last_pos.z - 1.0F);
    GXPosition3f32(cd->last_pos.x + cd->xE4_ecb.right.x,
                   cd->last_pos.y + cd->xE4_ecb.right.y,
                   cd->last_pos.z - 1.0F);
    GXPosition3f32(cd->last_pos.x + cd->xE4_ecb.bottom.x,
                   cd->last_pos.y + cd->xE4_ecb.bottom.y,
                   cd->last_pos.z - 1.0F);
    GXPosition3f32(cd->last_pos.x + cd->xE4_ecb.left.x,
                   cd->last_pos.y + cd->xE4_ecb.left.y, cd->last_pos.z - 1.0F);

    GXEnd();

    mpLib_SetupDraw(spCC);
    GXBegin(GX_LINES, GX_VTXFMT0, 4);

    GXPosition3f32(cd->last_pos.x - 1.0F, cd->last_pos.y,
                   cd->last_pos.z - 1.0F);
    GXPosition3f32(1.0F + cd->last_pos.x, cd->last_pos.y,
                   cd->last_pos.z - 1.0F);
    GXPosition3f32(cd->last_pos.x, cd->last_pos.y - 1.0F,
                   cd->last_pos.z - 1.0F);
    GXPosition3f32(cd->last_pos.x, 1.0F + cd->last_pos.y,
                   cd->last_pos.z - 1.0F);

    GXEnd();

    mpLib_SetupDraw(spC8);
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);
    if (cd->x34_flags.b6) {
        GXPosition3f32(cd->x28_vec.x + cd->x64_ecb.top.x,
                       cd->x28_vec.y + cd->x64_ecb.top.y,
                       0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->x64_ecb.right.x,
                       cd->x28_vec.y + cd->x64_ecb.right.y,
                       0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->x64_ecb.bottom.x,
                       cd->x28_vec.y + cd->x64_ecb.bottom.y,
                       0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->x64_ecb.left.x,
                       cd->x28_vec.y + cd->x64_ecb.left.y,
                       0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->x64_ecb.top.x,
                       cd->x28_vec.y + cd->x64_ecb.top.y,
                       0.5F + cd->x28_vec.z);
    } else {
        GXPosition3f32(cd->x28_vec.x + cd->ecb.top.x,
                       cd->x28_vec.y + cd->ecb.top.y, 0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->ecb.right.x,
                       cd->x28_vec.y + cd->ecb.right.y, 0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->ecb.bottom.x,
                       cd->x28_vec.y + cd->ecb.bottom.y, 0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->ecb.left.x,
                       cd->x28_vec.y + cd->ecb.left.y, 0.5F + cd->x28_vec.z);
        GXPosition3f32(cd->x28_vec.x + cd->ecb.top.x,
                       cd->x28_vec.y + cd->ecb.top.y, 0.5F + cd->x28_vec.z);
    }
    GXEnd();

    mpLib_SetupDraw(spC4);
    GXBegin(GX_LINES, GX_VTXFMT0, 4U);

    GXPosition3f32(cd->x28_vec.x - 1.0F, cd->x28_vec.y, 0.5 + cd->x28_vec.z);
    GXPosition3f32(1.0F + cd->x28_vec.x, cd->x28_vec.y, 0.5 + cd->x28_vec.z);
    GXPosition3f32(cd->x28_vec.x, cd->x28_vec.y - 1.0F, 0.5 + cd->x28_vec.z);
    GXPosition3f32(cd->x28_vec.x, 1.0F + cd->x28_vec.y, 0.5 + cd->x28_vec.z);

    GXEnd();
}

static inline GXColor mpLib_CopyColor(GXColor color)
{
    return color;
}

static const GXColor mpLib_804D80E0 = { 0xFF, 0x37, 0x37, 0x80 };
static const GXColor mpLib_804D80E4 = { 0x37, 0x37, 0xFF, 0x80 };

void mpLib_DrawSnapping(void)
{
    GXColor left_snap_color = mpLib_804D80E0;
    GXColor right_snap_color = mpLib_804D80E4;
    HSD_GObj* item_r28;
    HSD_GObj* ft_r27;
    bool var_r31;

    var_r31 = false;
    ft_r27 = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
    if (ft_r27 != NULL) {
        Mtx spDC;
        PAD_STACK(0x30);
        HSD_StateSetCullMode(0);
        GXClearVtxDesc();
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0U);
        HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(), spDC);
        GXSetCurrentMtx(0U);
        GXLoadPosMtxImm(spDC, 0);
        var_r31 = true;
        for (; ft_r27 != NULL; ft_r27 = ft_r27->next) {
            CollData* cd = ftLib_80086984(ft_r27);

            if (!cd->x35_flags.b0 || cd->x38 != mpColl_804D64AC) {
                continue;
            }

            mpLib_DrawEcbs(cd);

            if (cd->ledge_snap_x != 0.0 || cd->ledge_snap_y != 0.0) {
                float inner_x = cd->cur_pos.x + cd->ecb.right.x;
                float pos_y = cd->cur_pos.y;
                float f30 = 0.5F * cd->ledge_snap_height;

                mpLib_SetupDraw(right_snap_color);
                GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);
                GXPosition3f32(inner_x + cd->ledge_snap_x,
                               f30 + (pos_y + cd->ledge_snap_y),
                               cd->cur_pos.z);
                GXPosition3f32(inner_x + cd->ledge_snap_x,
                               (pos_y + cd->ledge_snap_y) - f30,
                               cd->cur_pos.z);
                GXPosition3f32(cd->cur_pos.x, (pos_y + cd->ledge_snap_y) - f30,
                               cd->cur_pos.z);
                GXPosition3f32(cd->cur_pos.x, f30 + (pos_y + cd->ledge_snap_y),
                               cd->cur_pos.z);
                GXPosition3f32(inner_x + cd->ledge_snap_x,
                               f30 + (pos_y + cd->ledge_snap_y),
                               cd->cur_pos.z);
                GXEnd();

                pos_y = cd->cur_pos.y;
                inner_x = cd->cur_pos.x + cd->ecb.left.x;
                mpLib_SetupDraw(mpLib_CopyColor(left_snap_color));
                GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);

                GXPosition3f32(inner_x - cd->ledge_snap_x,
                               f30 + (pos_y + cd->ledge_snap_y),
                               cd->cur_pos.z);
                GXPosition3f32(inner_x - cd->ledge_snap_x,
                               (pos_y + cd->ledge_snap_y) - f30,
                               cd->cur_pos.z);
                GXPosition3f32(cd->cur_pos.x, (pos_y + cd->ledge_snap_y) - f30,
                               cd->cur_pos.z);
                GXPosition3f32(cd->cur_pos.x, f30 + (pos_y + cd->ledge_snap_y),
                               cd->cur_pos.z);
                GXPosition3f32(inner_x - cd->ledge_snap_x,
                               f30 + (pos_y + cd->ledge_snap_y),
                               cd->cur_pos.z);

                GXEnd();
            }
        }
    }

    if ((item_r28 = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM]) != NULL) {
        if (!var_r31) {
            Mtx sp7C;
            PAD_STACK(0x34);
            GXSetCullMode(GX_CULL_NONE);
            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
            HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(), sp7C);
            GXSetCurrentMtx(0);
            GXLoadPosMtxImm(sp7C, 0);
        }

        for (; item_r28 != NULL; item_r28 = item_r28->next) {
            CollData* cd = &GET_ITEM(item_r28)->x378_itemColl;
            if (!cd->x35_flags.b0) {
                continue;
            }

            if (cd->x38 == mpColl_804D64AC) {
                mpLib_DrawEcbs(cd);
            }

            if (itGetKind(item_r28) == It_Kind_Link_HShot) {
                mpLib_DrawEcbs(&GET_ITEM(item_r28)
                                    ->xDD4_itemVar.linkhookshot.x0->coll_data);
            }
        }
    }
}

#ifdef MUST_MATCH
#define UNINITIALIZED(x) x
#else
#define UNINITIALIZED(x) void
#endif

static UNINITIALIZED(int)
    mpLib_DrawMatchingLines(int value, int flag, const GXColor* color)
{
    CollLine* line_r31;
    int count_r28;
    int total_r27;
    CollLine* line_r6;
    int i;

    count_r28 = 0;
    total_r27 = mpLib_804D64B4->line_count;
    line_r6 = groundCollLine;
    for (i = 0; i < total_r27; i++) {
        if (line_r6->flags & LINE_FLAG_ENABLED &&
            !(line_r6->flags & LINE_FLAG_HIDDEN) &&
            value == (line_r6->x0->lo_flags & flag))
        {
            count_r28 += 1;
        }
        line_r6 += 1;
    }

    if (count_r28 == 0) {
        return;
    }

    line_r31 = groundCollLine;
    mpLib_SetupDraw(*color);
    GXBegin(GX_QUADS, GX_VTXFMT0, count_r28 * 4);
    for (i = 0; i < total_r27; i++) {
        if (line_r31->flags & LINE_FLAG_ENABLED &&
            !(line_r31->flags & LINE_FLAG_HIDDEN))
        {
            if (value == (line_r31->x0->lo_flags & flag)) {
                PAD_STACK(8);
                GXPosition3f32(groundCollVtx[line_r31->x0->v0_idx].pos.x,
                               groundCollVtx[line_r31->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r31->x0->v1_idx].pos.x,
                               groundCollVtx[line_r31->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r31->x0->v1_idx].pos.x,
                               groundCollVtx[line_r31->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r31->x0->v0_idx].pos.x,
                               groundCollVtx[line_r31->x0->v0_idx].pos.y,
                               -25.0F);
            }
        }
        line_r31 += 1;
    }
    GXEnd();
}

static const GXColor mpLib_804D80F0 = { 0xFF, 0x40, 0x40, 0xFF };
static const GXColor mpLib_804D80F4 = { 0x40, 0x40, 0xFF, 0xFF };
static const GXColor mpLib_804D80F8 = { 0xFF, 0x40, 0xFF, 0xFF };
static const GXColor mpLib_804D80FC = { 0x80, 0x80, 0x80, 0xFF };
static const GXColor mpLib_804D8100 = { 0x80, 0x80, 0x80, 0xFF };

static const GXColor mpLib_FloorColor = { 0xC0, 0xC0, 0xC0, 0xFF };
static const GXColor mpLib_CeilingColor = { 0xC0, 0x80, 0x80, 0xFF };
static const GXColor mpLib_RightWallColor = { 0x80, 0xC0, 0x80, 0xFF };
static const GXColor mpLib_LeftWallColor = { 0x80, 0x80, 0xC0, 0xFF };
static const GXColor mpLib_DynamicFloorColor = { 0x60, 0x60, 0x60, 0xFF };
static const GXColor mpLib_DynamicCeilingColor = { 0x60, 0x40, 0x40, 0xFF };
static const GXColor mpLib_DynamicRightWallColor = { 0x40, 0x60, 0x40, 0xFF };
static const GXColor mpLib_DynamicLeftWallColor = { 0x40, 0x40, 0x60, 0xFF };

void mpLib_80059554(void)
{
    CollLine* line_r7;
    CollLine* line_r4;
    CollVtx* v0_r6;
    CollVtx* v1_r4;
    int total_r29;
    int count_r30;
    MapCollData* coll_data_r31;
    int i;
    UNUSED u8 pad[8];
    GXColor spB4;
    GXColor spB0;
    GXColor spAC;
    GXColor spA8;

    coll_data_r31 = mpLib_804D64B4;

    total_r29 = 0;
    count_r30 = coll_data_r31->floor_count;
    line_r4 = &groundCollLine[coll_data_r31->floor_start];

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4++;
    }

    if (total_r29 != 0) {
        mpLib_SetupDraw(mpLib_FloorColor);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->floor_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }

    total_r29 = 0;
    count_r30 = coll_data_r31->ceiling_count;
    line_r4 = &groundCollLine[coll_data_r31->ceiling_start];

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }

    if (total_r29 != 0) {
        mpLib_SetupDraw(mpLib_CeilingColor);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->ceiling_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }

    total_r29 = 0;
    count_r30 = coll_data_r31->right_wall_count;
    line_r4 = &groundCollLine[coll_data_r31->right_wall_start];

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }

    if (total_r29 != 0) {
        mpLib_SetupDraw(mpLib_RightWallColor);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->right_wall_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }

    total_r29 = 0;
    count_r30 = coll_data_r31->left_wall_count;
    line_r4 = &groundCollLine[coll_data_r31->left_wall_start];

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }

    if (total_r29 != 0) {
        mpLib_SetupDraw(mpLib_LeftWallColor);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->left_wall_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }

    count_r30 = coll_data_r31->dynamic_count;
    total_r29 = 0;
    spB4 = mpLib_DynamicFloorColor;
    spB0 = mpLib_DynamicCeilingColor;
    spAC = mpLib_DynamicRightWallColor;
    spA8 = mpLib_DynamicLeftWallColor;
    line_r4 = &groundCollLine[coll_data_r31->dynamic_start];

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            line_r4->flags & CollLine_Floor &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }

    if (total_r29 != 0) {
        mpLib_SetupDraw(spB4);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->dynamic_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                line_r7->flags & CollLine_Floor &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }
    line_r4 = &groundCollLine[coll_data_r31->dynamic_start];
    total_r29 = 0;

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            line_r4->flags & CollLine_Ceiling &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }

    if (total_r29 != 0) {
        mpLib_SetupDraw(spB0);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->dynamic_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                line_r7->flags & CollLine_Ceiling &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }
    line_r4 = &groundCollLine[coll_data_r31->dynamic_start];
    total_r29 = 0;

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            line_r4->flags & CollLine_RightWall &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }
    if (total_r29 != 0) {
        mpLib_SetupDraw(spAC);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->dynamic_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                line_r7->flags & CollLine_RightWall &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }
    line_r4 = &groundCollLine[coll_data_r31->dynamic_start];
    total_r29 = 0;

    for (i = 0; i < count_r30; i++) {
        if (line_r4->flags & LINE_FLAG_ENABLED &&
            line_r4->flags & CollLine_LeftWall &&
            !(line_r4->flags & LINE_FLAG_HIDDEN))
        {
            total_r29 += 1;
        }
        line_r4 += 1;
    }
    if (total_r29 != 0) {
        mpLib_SetupDraw(spA8);
        GXBegin(GX_QUADS, GX_VTXFMT0, (total_r29 * 4) & 0xFFFC);
        line_r7 = &groundCollLine[coll_data_r31->dynamic_start];

        for (i = 0; i < count_r30; i++) {
            if (line_r7->flags & LINE_FLAG_ENABLED &&
                line_r7->flags & CollLine_LeftWall &&
                !(line_r7->flags & LINE_FLAG_HIDDEN))
            {
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v1_idx].pos.x,
                               groundCollVtx[line_r7->x0->v1_idx].pos.y,
                               -25.0F);
                GXPosition3f32(groundCollVtx[line_r7->x0->v0_idx].pos.x,
                               groundCollVtx[line_r7->x0->v0_idx].pos.y,
                               -25.0F);
            }
            line_r7++;
        }
        GXEnd();
    }
}

void mpLib_80059E60(void)
{
    Mtx sp104;
    PAD_STACK(0x30);

    HSD_LObjSetupInit(HSD_CObjGetCurrent());
    GXSetCullMode(GX_CULL_NONE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0U);
    HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(), sp104);
    GXSetCurrentMtx(0U);
    GXLoadPosMtxImm(sp104, 0U);
    if (Camera_80030B50()) {
        // terrain draw
        struct {
            mpIsland_Palette palette;
            u8 pad[4];
            GXColor line_color;
            GXColor basic_color;
        } sp28;
        GXColor* line_color;
        mpIsland_PaletteEntry* entry;
        sp28.palette = mpIsland_TerrainPalette;

        entry = sp28.palette.x0;
        line_color = &sp28.line_color;

        while (entry->kind != -1) {
            *line_color = entry->color;
            mpLib_DrawMatchingLines(entry->kind, 0xFF, line_color);
            entry++;
        }

        sp28.basic_color = mpLib_804D8100;
        mpLib_DrawMatchingLines(mp_Terrain_Basic, 0xFF, &sp28.basic_color);
    } else if (Camera_80030B7C()) {
        // platform/ledge draw
        struct {
            u8 pad[0x10];
            GXColor ledge;
            GXColor platform;
            GXColor ledge_platform;
            GXColor none;
        } colors;
        colors.ledge = mpLib_804D80F0;
        mpLib_DrawMatchingLines(LINE_FLAG_LEDGE, LINE_FLAG_LEDGE,
                                &colors.ledge);
        colors.platform = mpLib_804D80F4;
        mpLib_DrawMatchingLines(LINE_FLAG_PLATFORM, LINE_FLAG_PLATFORM,
                                &colors.platform);
        colors.ledge_platform = mpLib_804D80F8;
        mpLib_DrawMatchingLines(LINE_FLAG_LEDGE | LINE_FLAG_PLATFORM,
                                LINE_FLAG_LEDGE | LINE_FLAG_PLATFORM,
                                &colors.ledge_platform);
        colors.none = mpLib_804D80FC;
        mpLib_DrawMatchingLines(0, LINE_FLAG_LEDGE | LINE_FLAG_PLATFORM,
                                &colors.none);
    } else {
        mpLib_80059554();
    }
}

void mpLib_DrawCrosses(s16* idx, int len, GXColor arg2)
{
    Vec3 sp34;
    int i;
    Vec3* vtx;
    int idx_i;
    int out_count;

    for (idx_i = 0, out_count = 0;
         idx_i < len && out_count < (signed) ARRAY_SIZE(mpLib_80458888);
         idx_i++)
    {
        if (Ground_801C2D24(idx[idx_i], &sp34)) {
            mpLib_80458888[out_count] = sp34;
            out_count += 1;
        }
    }

    if (!out_count) {
        return;
    }

    mpLib_SetupDraw(arg2);
    GXBegin(GX_LINES, GX_VTXFMT0, out_count * 6);
    for (i = 0; i < out_count; i++) {
        GXPosition3f32(mpLib_80458888[i].x - 3.0F, mpLib_80458888[i].y,
                       mpLib_80458888[i].z);
        GXPosition3f32(3.0F + mpLib_80458888[i].x, mpLib_80458888[i].y,
                       mpLib_80458888[i].z);
        GXPosition3f32(mpLib_80458888[i].x, mpLib_80458888[i].y - 3.0F,
                       mpLib_80458888[i].z);
        GXPosition3f32(mpLib_80458888[i].x, 3.0F + mpLib_80458888[i].y,
                       mpLib_80458888[i].z);
        GXPosition3f32(mpLib_80458888[i].x, mpLib_80458888[i].y,
                       mpLib_80458888[i].z - 3.0F);
        GXPosition3f32(mpLib_80458888[i].x, mpLib_80458888[i].y,
                       3.0F + mpLib_80458888[i].z);
    }
    GXEnd();
}

static const GXColor mpLib_804D8128 = { 0x7D, 0x7D, 0xFF, 0xFF };
static const GXColor mpLib_804D812C = { 0x7D, 0xFF, 0x80, 0xFF };
static const GXColor mpLib_804D8130 = { 0xFF, 0xFF, 0x80, 0xFF };
static const GXColor mpLib_804D8134 = { 0xFF, 0x40, 0x40, 0xFF };
static const GXColor mpLib_804D8138 = { 0xFF, 0x40, 0xC0, 0xFF };
static const GXColor mpLib_804D813C = { 0xFF, 0xFF, 0xFF, 0xFF };

enum {
    mpLib_EnemySpawnVtxIds = 0x16,
    mpLib_TrophySpawnVtxIds = 0x66,
    mpLib_ExitVtxIds = 0x80,
};

static s16 mpLib_ItemSpawnVtxIds[] = {
    0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A,
    0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93, 0,    0x20, 0x21,
    0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D,
    0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45,
    0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51,
    0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D,
    0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
    0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1,
    0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED,
    0xEE, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0,    0x99, 0x9A, 0x9B, 0x9C,
    0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8,
    0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4,
    0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0,
    0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6,
};

char mpLib_803BF6E0[] =
    "map coll under=%d upper=%d left=%d right=%d bbox=%d\n";

static s16 mpLib_SpawnVtxIds[4] = { 0, 1, 2, 3 };
SDATA s16 mpLib_RespawnVtxIds[6] = { 4, 5, 6, 7, 0, 0 };

void mpLib_DrawSpecialPoints(void)
{
    PAD_STACK(40);
    mpLib_DrawCrosses(mpLib_SpawnVtxIds, 0x04, mpLib_804D8128);
    mpLib_DrawCrosses(mpLib_RespawnVtxIds, 0x04, mpLib_804D812C);
    mpLib_DrawCrosses(mpLib_ItemSpawnVtxIds, 0x15, mpLib_804D8130);
    mpLib_DrawCrosses(&mpLib_ItemSpawnVtxIds[mpLib_EnemySpawnVtxIds], 0x50,
                      mpLib_804D8134);
    mpLib_DrawCrosses(&mpLib_ItemSpawnVtxIds[mpLib_TrophySpawnVtxIds], 0x19,
                      mpLib_804D8138);
    mpLib_DrawCrosses(&mpLib_ItemSpawnVtxIds[mpLib_ExitVtxIds], 0x2E,
                      mpLib_804D813C);
}

void mpLib_8005A2DC(void)
{
    mpLib_DrawSnapping();
    mpLib_80059E60();
    if (mpLib_804D64D0 == 0) {
        mpLib_804D64D0 = 1;
        OSReport(mpLib_803BF6E0, mpLib_804D64D4, mpLib_804D64D8,
                 mpLib_804D64DC, mpLib_804D64E0, mpLib_804D64E4);
    }
    HSD_StateInvalidate(-1);
}

static const GXColor mpLib_804D8140 = { 0xFF, 0xFF, 0xC0, 0xFF };
static const GXColor mpLib_804D8144 = { 0xFF, 0xFF, 0xFF, 0x80 };
static const GXColor mpLib_804D8148 = { 0x80, 0xC0, 0xC0, 0xFF };
static const GXColor mpLib_804D814C = { 0xC0, 0xFF, 0xFF, 0x80 };
static const GXColor mpLib_804D8150[2] = {
    { 0x40, 0x40, 0xFF, 0x00 },
    { 0, 0, 0, 0 },
};

/// blast zones, camera bounds, etc
void mpLib_DrawZones(void)
{
    u8 _3[0x4];
    Mtx sp7C;
    u8 _2[0x38];
    GXColor sp40;
    u8 _5[0x8];
    GXColor sp34;
    GXColor sp20;
    CmSubject* var_r30;

    f32 left;
    f32 right;
    f32 bottom;
    f32 top;

    HSD_LObjSetupInit(HSD_CObjGetCurrent());
    GXSetCullMode(GX_CULL_NONE);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
    HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(), sp7C);
    GXSetCurrentMtx(0);
    GXLoadPosMtxImm(sp7C, 0);

    left = Stage_GetBlastZoneLeftOffset();
    right = Stage_GetBlastZoneRightOffset();
    bottom = Stage_GetBlastZoneBottomOffset();
    top = Stage_GetBlastZoneTopOffset();

    sp40 = mpLib_804D8144;
    mpLib_SetupDraw(mpLib_804D8140);
    GXSetZMode(1, GX_LEQUAL, 0);
    GXSetLineWidth(0x10, GX_TO_ZERO);

    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);

    GXPosition3f32(left, bottom, 0.0F);
    GXPosition3f32(right, bottom, 0.0F);
    GXPosition3f32(right, top, 0.0F);
    GXPosition3f32(left, top, 0.0F);
    GXPosition3f32(left, bottom, 0.0F);

    GXEnd();

    mpLib_SetupDraw(sp40);
    GXSetZMode(1, GX_GREATER, 0);
    GXSetLineWidth(0x10, GX_TO_ZERO);

    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);

    GXPosition3f32(left, bottom, 0.0F);
    GXPosition3f32(right, bottom, 0.0F);
    GXPosition3f32(right, top, 0.0F);
    GXPosition3f32(left, top, 0.0F);
    GXPosition3f32(left, bottom, 0.0F);

    GXEnd();

    left = Stage_GetCamBoundsLeftOffset();
    right = Stage_GetCamBoundsRightOffset();
    bottom = Stage_GetCamBoundsBottomOffset();
    top = Stage_GetCamBoundsTopOffset();

    sp34 = mpLib_804D814C;
    mpLib_SetupDraw(mpLib_804D8148);

    GXSetZMode(1, GX_LEQUAL, 0);
    GXSetLineWidth(0x10, GX_TO_ZERO);
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);

    sp20 = sp34;

    GXPosition3f32(left, bottom, 0.0F);
    GXPosition3f32(right, bottom, 0.0F);
    GXPosition3f32(right, top, 0.0F);
    GXPosition3f32(left, top, 0.0F);
    GXPosition3f32(left, bottom, 0.0F);

    GXEnd();

    mpLib_SetupDraw(sp20);

    GXSetZMode(1, GX_GREATER, 0);
    GXSetLineWidth(0x10, GX_TO_ZERO);
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);

    GXPosition3f32(left, bottom, 0.0F);
    GXPosition3f32(right, bottom, 0.0F);
    GXPosition3f32(right, top, 0.0F);
    GXPosition3f32(left, top, 0.0F);
    GXPosition3f32(left, bottom, 0.0F);

    GXEnd();

    mpLib_SetupDraw(mpLib_804D8150[0]);

    GXSetZMode(0, GX_LEQUAL, 0);
    GXSetLineWidth(0x10, GX_TO_ZERO);

    var_r30 = cm_804D6468;
    while (var_r30 != NULL) {
        if (Camera_8002928C(var_r30)) {
            GXBegin(GX_LINESTRIP, GX_VTXFMT0, 5);

            GXPosition3f32(var_r30->pos.x + var_r30->ext.h.x,
                           var_r30->pos.y + var_r30->ext.v.x, 0.0F);
            GXPosition3f32(var_r30->pos.x + var_r30->ext.h.y,
                           var_r30->pos.y + var_r30->ext.v.x, 0.0F);
            GXPosition3f32(var_r30->pos.x + var_r30->ext.h.y,
                           var_r30->pos.y + var_r30->ext.v.y, 0.0F);
            GXPosition3f32(var_r30->pos.x + var_r30->ext.h.x,
                           var_r30->pos.y + var_r30->ext.v.y, 0.0F);
            GXPosition3f32(var_r30->pos.x + var_r30->ext.h.x,
                           var_r30->pos.y + var_r30->ext.v.x, 0.0F);

            GXEnd();
        }
        var_r30 = var_r30->prev;
    }
    HSD_StateInvalidate(-1);
}
