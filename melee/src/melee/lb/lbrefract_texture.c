#include "lbrefract.h"
#include "types.h"
#include <dolphin/gx/GXTexture.h>

static void lbRefract_WriteTexCoordIA8(lbRefract_CallbackData* data, u32 row,
                                    u32 col, u8 r, u8 g, u8 intensity, u8 alpha)
{
    u32 r_ = row >> 2;
    u32 c_ = col >> 2;
    u8(*base)[2] =
        (void*) ((u8*) data->buffer + c_ * data->row_stride + r_ * 32);
    row &= 3;
    col &= 3;
    base[row + col * 4][0] = alpha;
    base[row + col * 4][1] = intensity;
}

static void fn_80021F70(lbRefract_CallbackData* data, u32 row,
                        u32 col, u8 r, u8 g, u8 b, u8 alpha)
{
    u8* base;
    s32 offset;
    u32 r_ = row >> 2;
    u32 c_ = col >> 2;
    base = (u8*) ((u8*) data->buffer + c_ * data->row_stride + r_ * 32);
    row &= 3;
    col &= 3;
    offset = (row + col * 4);
    u16 pixel = (b / 8) | ((g * 8 & 0x7E0 & ~0xF800) | ((r << 8) & 0xF800));
    base[offset * 2] = (u8) (pixel >> 8);
    base[offset * 2 + 1] = (u8) pixel;
}

static void fn_80021FB4(lbRefract_CallbackData* data, u32 row,
                        u32 col, u8 arg6, u8 arg7, u8 arg8, u8 arg9)
{
    u32 r_ = row >> 2;
    u32 c_ = col >> 2;
    u8(*base)[2] =
        (void*) ((u8*) data->buffer + c_ * data->row_stride + r_ * 64);
    row &= 3;
    col &= 3;
    base[row + col * 4][0] = arg9;
    base[row + col * 4][1] = arg6;
    base[row + col * 4 + 16][0] = arg7;
    base[row + col * 4 + 16][1] = arg8;
}

static void fn_80021FF8(lbRefract_CallbackData* data, u32 row,
                        u32 col, u32* arg3, u32* arg4, u32* arg5, u32* arg6)
{
    u8* base;
    int offset;

    {
        u32 r = row >> 2;
        u32 c = col >> 2;
        base = (u8*) data->buffer + c * data->row_stride + r * 32;
    }
    row &= 3;
    col &= 3;
    offset = (row + col * 4) * 2;
    if (arg3 != NULL) {
        *arg3 = 0xFF;
    }
    if (arg4 != NULL) {
        *arg4 = 0xFF;
    }
    if (arg5 != NULL) {
        *arg5 = base[offset + 1];
    }
    if (arg6 != NULL) {
        *arg6 = base[offset];
    }
}

static void fn_8002206C(lbRefract_CallbackData* data, u32 row,
                        u32 col, u32* arg3, u32* arg4, u32* arg5, u32* arg6)
{
    u8* base;
    s32 offset;
    u16 pixel;
    s32 val;

    {
        u32 r = row & ~3;
        u32 c = col & ~3;
        base = (u8*) ((u8*) data->buffer + c / 4 * data->row_stride + r * 8);
    }
    row &= 3;
    col &= 3;
    offset = (row + col * 4) * 2;
    pixel = ((u16) base[offset] << 8) | base[offset + 1];
    if (arg6 != NULL) {
        *arg6 = 0xFF;
    }
    if (arg3 != NULL) {
        if (((pixel >> 8) & 0xF8) | (pixel & 0x8000)) {
            val = 7;
        } else {
            val = 0;
        }
        *arg3 = val;
    }
    if (arg4 != NULL) {
        if (((pixel >> 3) & 0xFC) | (pixel & 0x400)) {
            val = 3;
        } else {
            val = 0;
        }
        *arg4 = val;
    }
    if (arg5 != NULL) {
        if (pixel & 0xF8) {
            val = 7;
        } else {
            val = 0;
        }
        *arg5 = val;
    }
}

static void lbRefract_ReadTexCoordRGBA8(lbRefract_CallbackData* data, u32 row,
                                    u32 col, u32* out_r, u32* out_g,
                                    u32* out_b, u32* out_a)
{
    int offset;
    u8* base;

    {
        u32 r = row >> 2;
        u32 c = col >> 2;
        base = (u8*) data->buffer + c * data->row_stride + r * 64;
    }
    row &= 3;
    col &= 3;
    offset = (row + col * 4) * 2;

    if (out_a != NULL) {
        *out_a = base[offset];
    }
    if (out_r != NULL) {
        *out_r = base[offset + 1];
    }
    if (out_g != NULL) {
        *out_g = base[offset + 0x20];
    }
    if (out_b != NULL) {
        *out_b = base[offset + 0x21];
    }
}

/// @brief Initialize refraction callback data for a texture buffer.
/// @param data Callback data to initialize.
/// @param buffer Base address of texture buffer.
/// @param format Texture format (3=IA8, 4=RGB565, 6=RGBA8).
/// @param width Texture width in pixels.
/// @param height Texture height in pixels.
/// @return 0 on success, -1 if format is unsupported.
int lbRefract_8002219C(lbRefract_CallbackData* data, void* buffer,
                         s32 format, u32 width, u32 height)
{
    data->buffer = buffer;
    data->format = format;
    data->width = width;
    data->height = height;
    data->buffer_size = GXGetTexBufferSize(width, height, format, 0, 0);
    switch (format) {
    case GX_TF_IA8:
        data->callback0 = lbRefract_WriteTexCoordIA8;
        data->callback1 = fn_80021FF8;
        data->row_stride = (width * 8) & 0xFFFFFFE0;
        break;
    case GX_TF_RGB565:
        data->callback0 = fn_80021F70;
        data->callback1 = fn_8002206C;
        data->row_stride = (width * 8) & 0xFFFFFFE0;
        break;
    case GX_TF_RGBA8:
        data->callback0 = fn_80021FB4;
        data->callback1 = lbRefract_ReadTexCoordRGBA8;
        data->row_stride = (width * 16) & 0xFFFFFFC0;
        break;
    case GX_TF_RGB5A3:
    default:
        return -1;
    }
    return 0;
}

