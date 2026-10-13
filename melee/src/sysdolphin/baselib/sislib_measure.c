#include "sislib.h"
#include "sislib_data.h"
#include "sislib.static.h"

void HSD_SisLib_803A8134(void* cursor, HSD_Text* text, f32* out_width,
                         f32* out_height)
{
    SIS* sis;
    TextGlyphTexture* glyph_tex = NULL;
    u8* default_kerning = HSD_SisLib_8040CB00;
    f32 line_height;
    f32 saved_scale_x;
    f32 saved_scale_y;
    f32 saved_spacing;
    u16 scale_val;
    u16 kern_enabled;
    u16 saved_x6C;
    u16 glyph_code;
    s32 kern_width;
    s32 clear_idx;
    const u8* pop_result;
    u8 opcode;
    const TextKerning* kern_data;
    saved_scale_x = text->x80.x;
    saved_scale_y = text->x80.y;
    saved_spacing = text->x78.x;
    kern_enabled = text->kerning;
    sis = HSD_SisLib_804D1124[text->font_idx];
    saved_x6C = text->x6C;
    if (sis != NULL) {
        glyph_tex = sis->textures;
    }
    *out_width = 0.0F;
    *out_height = 32.0F * text->x80.y;
loop_3:
    opcode = *(u8*) cursor;
    switch (opcode) {
    case 0:
        pop_result = HSD_SisLib_803A7F0C(text, 0x85);
        if (pop_result != NULL) {
            cursor = (u8*) (pop_result + 4);
            goto block_33;
        }
        break;
    case 1:
    case 2:
    case 3:
    case 7:
        break;
    case 9:
        HSD_SisLib_803A7684(text, (u8*) cursor, 0x85U);
        /* fallthrough */
    case 8:
        cursor = (u8*) HSD_SisLib_Jump(text->font_idx, (u8*) cursor + 1) - 1;
        goto block_33;
    case 14:
        HSD_SisLib_803A7684(text, (u8*) cursor, 0x83U);
        text->x80.x = (f32) HSD_SisLib_ReadU16(((u8*) cursor + 1)) / 256.0F;
        scale_val = HSD_SisLib_ReadU16(((u8*) cursor + 3));
        cursor = (u8*) cursor + 4;
        text->x80.y = (f32) scale_val / 256.0F;
        goto block_33;
    case 15:
        HSD_SisLib_803A7F0C(text, 0x83);
        goto block_33;
    case 6:
        cursor = (u8*) cursor + 4;
        goto block_33;
    case 10:
        if ((text->alloc_data == NULL) || (kern_enabled == 0)) {
            HSD_SisLib_803A7684(text, (u8*) cursor, 0x81U);
            text->x78.x = (f32) (s16) HSD_SisLib_ReadU16(((u8*) cursor + 1)) / 256.0F;
        }
        cursor = (u8*) cursor + 4;
        goto block_33;
    case 11:
        if ((text->alloc_data == NULL) || (kern_enabled == 0)) {
            HSD_SisLib_803A7F0C(text, 0x81);
        }
        goto block_33;
    case 12:
        cursor = (u8*) cursor + 3;
        goto block_33;
    case 5:
        cursor = (u8*) cursor + 2;
        goto block_33;
    case 22:
        kern_enabled = 1;
        goto block_33;
    case 23:
        kern_enabled = 0;
        goto block_33;
    case 26:
        *out_width += text->x80.x * (16.0F + text->x78.x);
        if (*out_height < (32.0F * text->x80.y)) {
            *out_height = 32.0F * text->x80.y;
        }
        goto block_33;
    default:
        if (opcode >= 0x20U) {
            *out_width += text->x80.x * (32.0F + text->x78.x);
            if (kern_enabled != 0) {
                glyph_code = HSD_SisLib_ReadU16(cursor);
                if (glyph_code < 0x4000U) {
                    kern_data = (const TextKerning*) (default_kerning +
                        (((glyph_code - 0x2000) * 2) & 0x1FFFE));
                } else {
                    kern_data = (const TextKerning*) (glyph_tex->data +
                        (((glyph_code - 0x4000) * 2) & 0x1FFFE));
                }
                kern_width = kern_data->left + kern_data->right - 2;
                *out_width = -((text->x80.x * (f32) kern_width) - *out_width);
            }
            if (*out_height < (32.0F * text->x80.y)) {
                *out_height = 32.0F * text->x80.y;
            }
            cursor = (u8*) cursor + 1;
        }
        goto block_33;
    }
    goto block_done;
block_33:
    cursor = (u8*) cursor + 1;
    goto loop_3;
block_done:
    *out_width -= text->x78.x;
    text->x80.x = saved_scale_x;
    text->x80.y = saved_scale_y;
    text->x78.x = saved_spacing;
    text->x6C = saved_x6C;
    opcode = 0;
    clear_idx = text->x6C;
    while (clear_idx < (s32) text->x6E) {
        text->string_buffer[clear_idx] = opcode;
        clear_idx += 1;
    }
}

