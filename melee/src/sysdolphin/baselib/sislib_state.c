#include "sislib.h"
#include "sislib.static.h"
#include <string.h>

// Formatting scalars retain SIS byte order. Return addresses are native and
// process-local; they are never serialized to an archive.

void HSD_SisLib_803A7684(HSD_Text* text, const u8* cursor, u8 flags)
{
    switch (flags & 0x7F) {
    case 1: {
        u8* old_buf;
        u32 count;
        int idx;
        int new_x6E;
        int old_x6E;

        old_x6E = text->x6E;
        if (old_x6E < (s32) (text->x6C + 5)) {
            new_x6E = old_x6E + 0x10;
            old_buf = (u8*) text->string_buffer;
            text->string_buffer = HSD_SisLib_Alloc(new_x6E);
            text->x6E = (u16) new_x6E;
            count = old_x6E;
            for (idx = 0; (u32) idx < count; idx++) {
                text->string_buffer[idx] = old_buf[idx];
            }
            while (idx < (s32) text->x6E) {
                text->string_buffer[idx] = 0;
                idx += 1;
            }
            HSD_SisLib_Free(old_buf);
        }
        text->string_buffer[text->x6C++] =
            (u8) ((s32) (256.0F * text->x78.x) >> 8);
        text->string_buffer[text->x6C++] = (u8) (256.0F * text->x78.x);
        text->string_buffer[text->x6C++] =
            (u8) ((s32) (256.0F * text->x78.y) >> 8);
        text->string_buffer[text->x6C++] = (u8) (256.0F * text->x78.y);
        text->string_buffer[text->x6C++] = flags;
        return;
    }
    case 2: {
        int old_x6E;
        u32 count;
        int idx;
        u8* old_buf;
        int new_x6E;

        old_x6E = text->x6E;
        if (old_x6E < (s32) (text->x6C + 4)) {
            new_x6E = old_x6E + 0x10;
            old_buf = (u8*) text->string_buffer;
            text->string_buffer = HSD_SisLib_Alloc(new_x6E);
            text->x6E = (u16) new_x6E;
            count = old_x6E;
            for (idx = 0; (u32) idx < count; idx++) {
                text->string_buffer[idx] = old_buf[idx];
            }
            while (idx < (s32) text->x6E) {
                text->string_buffer[idx] = 0;
                idx += 1;
            }
            HSD_SisLib_Free(old_buf);
        }
        text->string_buffer[text->x6C++] = text->active_color.r;
        text->string_buffer[text->x6C++] = text->active_color.g;
        text->string_buffer[text->x6C++] = text->active_color.b;
        text->string_buffer[text->x6C++] = flags;
        return;
    }
    case 3: {
        int old_x6E;
        u32 count;
        int idx;
        u8* old_buf;
        int new_x6E;

        old_x6E = text->x6E;
        if (old_x6E < (s32) (text->x6C + 5)) {
            new_x6E = old_x6E + 0x10;
            old_buf = (u8*) text->string_buffer;
            text->string_buffer = HSD_SisLib_Alloc(new_x6E);
            text->x6E = (u16) new_x6E;
            count = old_x6E;
            for (idx = 0; (u32) idx < count; idx++) {
                text->string_buffer[idx] = old_buf[idx];
            }
            while (idx < (s32) text->x6E) {
                text->string_buffer[idx] = 0;
                idx += 1;
            }
            HSD_SisLib_Free(old_buf);
        }
        text->string_buffer[text->x6C++] =
            (u8) ((s32) (256.0F * text->x80.x) >> 8);
        text->string_buffer[text->x6C++] = (u8) (256.0F * text->x80.x);
        text->string_buffer[text->x6C++] =
            (u8) ((s32) (256.0F * text->x80.y) >> 8);
        text->string_buffer[text->x6C++] = (u8) (256.0F * text->x80.y);
        text->string_buffer[text->x6C++] = flags;
        return;
    }
    case 4: {
        int old_x6E;
        u32 count;
        int idx;
        u8* old_buf;
        int new_x6E;

        old_x6E = text->x6E;
        if (old_x6E < (s32) (text->x6C + 2)) {
            new_x6E = old_x6E + 0x10;
            old_buf = (u8*) text->string_buffer;
            text->string_buffer = HSD_SisLib_Alloc(new_x6E);
            text->x6E = (u16) new_x6E;
            count = old_x6E;
            for (idx = 0; (u32) idx < count; idx++) {
                text->string_buffer[idx] = old_buf[idx];
            }
            while (idx < (s32) text->x6E) {
                text->string_buffer[idx] = 0;
                idx += 1;
            }
            HSD_SisLib_Free(old_buf);
        }
        text->string_buffer[text->x6C++] = text->alignment;
        text->string_buffer[text->x6C++] = flags;
        return;
    }
    case 5: {
        int new_x6E;
        u32 count;
        int idx;
        u8* old_buf;
        int old_x6E;

        old_x6E = text->x6E;
        if (old_x6E < (s32) (text->x6C + sizeof(cursor) + 1)) {
            new_x6E = old_x6E + 0x10;
            old_buf = (u8*) text->string_buffer;
            text->string_buffer = HSD_SisLib_Alloc(new_x6E);
            text->x6E = (u16) new_x6E;
            count = old_x6E;
            for (idx = 0; (u32) idx < count; idx++) {
                text->string_buffer[idx] = old_buf[idx];
            }
            while (idx < (s32) text->x6E) {
                text->string_buffer[idx] = 0;
                idx += 1;
            }
            HSD_SisLib_Free(old_buf);
        }
        memcpy(text->string_buffer + text->x6C, &cursor, sizeof(cursor));
        text->x6C += sizeof(cursor);
        text->string_buffer[text->x6C++] = flags;
    }
    }
}

const u8* HSD_SisLib_803A7F0C(HSD_Text* text, s32 flags)
{
    s8 entry;
    s32 flag_hi;
    s32 entry_flags;
    s32 target_type;
    const u8* result;
    s32 remove_size;
    s32 pos;

    flag_hi = flags & 0x80;
    target_type = flags & 0x7F;
    pos = (s32) text->x6C - 1;
    result = NULL;
    remove_size = 0;
    while (pos >= 0) {
        entry = text->string_buffer[pos];
        entry_flags = entry & 0x80;
        switch ((u8) entry & 0x7F) { /* irregular */
        case 1:
            pos -= 4;
            if (target_type == 1) {
                text->x78.x =
                    (f32) (s16) HSD_SisLib_ReadU16(text->string_buffer + pos) / 256.0F;
                text->x78.y =
                    (f32) (s16) HSD_SisLib_ReadU16(text->string_buffer + pos + 2) / 256.0F;
                if (flag_hi == entry_flags) {
                    remove_size = 5;
                }
                goto done;
            }
            break;
        case 2:
            pos -= 3;
            if (target_type == 2) {
                text->active_color.r = text->string_buffer[pos];
                text->active_color.g = text->string_buffer[pos + 1];
                text->active_color.b = text->string_buffer[pos + 2];
                if (flag_hi == entry_flags) {
                    remove_size = 4;
                }
                goto done;
            }
            break;
        case 3:
            pos -= 4;
            if (target_type == 3) {
                text->x80.x =
                    (f32) HSD_SisLib_ReadU16(text->string_buffer + pos) / 256.0F;
                text->x80.y =
                    (f32) HSD_SisLib_ReadU16(text->string_buffer + pos + 2) / 256.0F;
                if (flag_hi == entry_flags) {
                    remove_size = 5;
                }
                goto done;
            }
            break;
        case 4:
            pos -= 1;
            if (target_type == 4) {
                text->alignment = text->string_buffer[pos];
                if (flag_hi == entry_flags) {
                    remove_size = 2;
                }
                goto done;
            }
            break;
        case 5:
            pos -= sizeof(result);
            if (target_type == 5) {
                memcpy(&result, text->string_buffer + pos, sizeof(result));
                if (flag_hi == entry_flags) {
                    remove_size = sizeof(result) + 1;
                }
                goto done;
            }
            break;
        }
        pos -= 1;
    }
done:
    if (remove_size != 0) {
        while ((pos + remove_size) < (s32) text->x6C) {
            text->string_buffer[pos] = text->string_buffer[pos + remove_size];
            pos += 1;
        }
        while (pos < (s32) text->x6C) {
            text->string_buffer[pos] = 0;
            pos += 1;
        }
        text->x6C -= remove_size;
    }
    return result;
}

