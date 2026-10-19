#include "lbrefract.h"
#include <sysdolphin/baselib/pobj.h>

s32 lbRefract_PObjLoad(HSD_PObj* pobj, HSD_PObjDesc* desc)
{
    u8* display;
    s32 offset;
    s32 total_bytes;
    u8* ptr;
    s32 hi;
    s32 copied;
    s32 count;
    s32 ret;
    HSD_VtxDescList* verts;
    s32 last_offset;
    s32 pnmtx_offset;
    s32 stride;

    ret = hsdPObj.load(pobj, desc);
    if (ret != 0) {
        return ret;
    }

    if (pobj_type(pobj) != POBJ_ENVELOPE) {
        return 0;
    }

    verts = pobj->verts;
    stride = 0;
    pnmtx_offset = -1;
    last_offset = -1;

    while (verts != NULL && verts->attr != GX_VA_NULL) {
        switch (verts->attr) {
        case GX_VA_TEX0MTXIDX:
        case GX_VA_TEX1MTXIDX:
        case GX_VA_TEX2MTXIDX:
        case GX_VA_TEX3MTXIDX:
        case GX_VA_TEX4MTXIDX:
        case GX_VA_TEX5MTXIDX:
        case GX_VA_TEX6MTXIDX:
        case GX_VA_TEX7MTXIDX:
            last_offset = stride;
            stride++;
            break;
        case GX_VA_PNMTXIDX:
            pnmtx_offset = stride;
            stride++;
            break;
        case GX_VA_TEX0:
        case GX_VA_TEX1:
        case GX_VA_TEX2:
        case GX_VA_TEX3:
        case GX_VA_TEX4:
        case GX_VA_TEX5:
        case GX_VA_TEX6:
        case GX_VA_TEX7:
        case GX_VA_POS:
        case GX_VA_NRM:
            if (verts->attr_type == GX_INDEX16) {
                stride += 2;
            } else {
                stride++;
            }
            break;
        case GX_VA_CLR0:
        case GX_VA_CLR1:
            switch (verts->attr_type) {
            case GX_INDEX16:
                stride += 2;
                break;
            case GX_INDEX8:
                stride++;
                break;
            default:
                switch (verts->comp_type) {
                case GX_RGB565:
                case GX_RGBA4:
                    stride += 2;
                    break;
                case GX_RGB8:
                case GX_RGBA6:
                    stride += 3;
                    break;
                case GX_RGBX8:
                    stride += 4;
                    break;
                default:
                    break;
                }
                break;
            }
            break;
        default:
            break;
        }
        verts++;
    }

    if (pnmtx_offset == -1 || last_offset < 2) {
        return 0;
    }

    display = pobj->display;
    offset = 0;
    total_bytes = pobj->n_display << 5;
    while (offset < total_bytes) {
        if ((display[offset++] & 0xF8) == 0) {
            break;
        }

        if (total_bytes - offset < 2) return -1;
        ptr = display + offset;
        hi = ptr[0];
        count = hi << 8;
        count |= ptr[1];
        offset += 2;

        if (stride <= 0 || count > (total_bytes - offset) / stride) return -1;
        for (copied = 0; copied < count; copied++) {
            display[offset + last_offset] = display[offset + pnmtx_offset];
            offset += stride;
        }
    }

    // Aurora copies CPU display-list bytes into its FIFO at submission.

    return 0;
}

