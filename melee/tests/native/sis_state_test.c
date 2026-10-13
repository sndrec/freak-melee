#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/sislib_data.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

void* HSD_SisLib_Alloc(s32 size) { return calloc(1, size); }
void HSD_SisLib_Free(void* memory) { free(memory); }
SIS* HSD_SisLib_804D1124[5];
u8 HSD_SisLib_8040CB00[0x240];

static u8 archive_bytes[129];
static unsigned archive_allocations;
void* HSD_MemAlloc(ssize_t size) { ++archive_allocations; return malloc(size); }
void HSD_Free(void* memory) { --archive_allocations; free(memory); }
size_t lbFileGetSize(const char* path) { return sizeof(archive_bytes); }
void lbFile_8001668C(const char* path, void* dst, size_t* size)
{
    memcpy(dst, archive_bytes, sizeof(archive_bytes));
    *size = sizeof(archive_bytes);
}
void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%u: %s\n", file, line, expression);
    abort();
}
static void put_word(u32 slot, u32 value)
{
    u8* p = archive_bytes + slot;
    p[0] = value >> 24; p[1] = value >> 16;
    p[2] = value >> 8; p[3] = value;
}
static void load_text(HSD_Text* text)
{
    put_word(0, sizeof(archive_bytes)); put_word(4, 64);
    put_word(8, 4); put_word(12, 1);
    put_word(32, 16); put_word(36, 24); put_word(40, 26); put_word(44, 48);
    for (u32 i = 0; i < 4; ++i) put_word(96 + i * 4, i * 4);
    memcpy(archive_bytes + 120, "SIS_Test", 9);
    const u8 caller[] = { 9, 0, 0, 0, 48, 26, 0 };
    memcpy(archive_bytes + 32 + 26, caller, sizeof(caller));
    archive_bytes[32 + 48] = 0x20; archive_bytes[32 + 49] = 1;
    HSD_SisLib_ResetData();
    HSD_SisLib_803A62A0(0, "fixture", "SIS_Test");
    require(archive_allocations == 2, "native SIS storage ownership");
    u8** table = (u8**) HSD_SisLib_804D1124[0];
    f32 width, height;
    HSD_SisLib_803A8134(table[2], text, &width, &height);
    require(width == 24.0F && height == 16.0F && text->x6C == 0,
            "archive-relative call and native return");
    u8 copy[sizeof(caller)];
    memcpy(copy, table[2], sizeof(copy));
    HSD_SisLib_803A8134(copy, text, &width, &height);
    require(width == 24.0F, "copied text retains archive-relative call");
    HSD_SisLib_803A62A0(0, "fixture", "SIS_Test");
    require(archive_allocations == 2, "replacing SIS releases old storage");
    HSD_SisLib_FreeData(0);
    require(archive_allocations == 0 && HSD_SisLib_804D1124[0] == NULL,
            "free SIS ownership");
}

static void measure_text(HSD_Text* text)
{
    TextGlyphTexture custom_widths = { 0 };
    SIS font = { 0 };
    custom_widths.data[0] = 3; custom_widths.data[1] = 5;
    font.textures = &custom_widths;
    HSD_SisLib_804D1124[0] = &font;
    HSD_SisLib_8040CB00[2] = 2; HSD_SisLib_8040CB00[3] = 4;
    text->x80.x = text->x80.y = 0.5F;
    text->x78.x = text->x78.y = 0;
    text->kerning = 1;
    text->active_color = (GXColor) { 12, 34, 56, 78 };
    HSD_SisLib_803A7684(text, NULL, 2);
    u16 saved_size = text->x6C;
    f32 width, height;
    // Scale 1x2, standard and custom glyphs, restore scale, half-width space.
    u8 encoded[] = { 14, 1, 0, 2, 0, 0x20, 1, 0x40, 0, 15, 26, 0 };
    HSD_SisLib_803A8134(encoded, text, &width, &height);
    require(width == 62.0F && height == 64.0F,
            "encoded scale and native standard/custom kerning lookup");
    require(text->x80.x == 0.5F && text->x80.y == 0.5F &&
                text->x6C == saved_size, "measurement preserves state");
    text->active_color.r = 0;
    HSD_SisLib_803A7F0C(text, 2);
    require(text->active_color.r == 12, "measurement preserves prior stack");
    text->kerning = 0;
    u8 spacing[] = { 10, 0xfd, 0xc0, 0, 0, 0x20, 1, 11, 0 };
    HSD_SisLib_803A8134(spacing, text, &width, &height);
    require(width == 14.875F && height == 16.0F && text->x78.x == 0,
            "encoded negative spacing and restore");
    HSD_SisLib_804D1124[0] = NULL;
}

int main(void)
{
    HSD_Text text = { 0 };
    // Exactly full stacks must not examine a byte past their final flag.
    char full[3] = { 4, 4, (char) 0x85 };
    text.string_buffer = full;
    text.x6C = text.x6E = 2;
    HSD_SisLib_803A7F0C(&text, 4);
    require(text.alignment == 4 && text.x6C == 0 &&
                (u8) full[2] == 0x85, "exact-capacity stack pop");
    require(HSD_SisLib_803A7F0C(&text, 5) == NULL, "empty stack");

    text.string_buffer = HSD_SisLib_Alloc(16);
    text.x6E = 16;
    text.x78.x = -2.5F; text.x78.y = 3.25F;
    text.x80.x = 0.5390625F; text.x80.y = 2.0F;
    text.active_color = (GXColor) { 17, 128, 249, 42 };
    text.alignment = 2;
    HSD_SisLib_803A7684(&text, NULL, 0x81);
    HSD_SisLib_803A7684(&text, NULL, 2);
    HSD_SisLib_803A7684(&text, NULL, 0x83);
    HSD_SisLib_803A7684(&text, NULL, 4);
    require(text.x6C == 16, "format stack size");

    // This address cannot survive a 32-bit integer or pointer slot.
    const u8* outer = (const u8*) (uintptr_t) UINT64_C(0x123456789abc);
    const u8* inner = (const u8*) (uintptr_t) UINT64_C(0xfedcba987654);
    HSD_SisLib_803A7684(&text, outer, 5);
    HSD_SisLib_803A7684(&text, inner, 0x85);
    require(text.x6E >= text.x6C, "stack growth capacity");
    u16 size = text.x6C;
    require(HSD_SisLib_803A7F0C(&text, 5) == inner && text.x6C == size,
            "different flag restores without removal");
    require(HSD_SisLib_803A7F0C(&text, 0x85) == inner,
            "nested native return address");

    // Remove a format entry below a return address; surviving entries must
    // shift together and retain their exact pointer bytes.
    text.x80.x = text.x80.y = 0;
    HSD_SisLib_803A7F0C(&text, 0x83);
    require(text.x80.x == 0.5390625F && text.x80.y == 2.0F,
            "unsigned big-endian scale restore");
    require(HSD_SisLib_803A7F0C(&text, 5) == outer,
            "native return address after middle removal");
    text.alignment = 0;
    HSD_SisLib_803A7F0C(&text, 4);
    require(text.alignment == 2, "alignment restore");
    text.active_color = (GXColor) { 0, 0, 0, 99 };
    HSD_SisLib_803A7F0C(&text, 2);
    require(text.active_color.r == 17 && text.active_color.g == 128 &&
                text.active_color.b == 249 && text.active_color.a == 99,
            "RGB restore preserves alpha");
    text.x78.x = text.x78.y = 0;
    HSD_SisLib_803A7F0C(&text, 0x81);
    require(text.x78.x == -2.5F && text.x78.y == 3.25F && text.x6C == 0,
            "signed big-endian spacing restore");
    measure_text(&text);
    load_text(&text);
    HSD_SisLib_Free(text.string_buffer);
    puts("Native SIS formatting state passed");
    return 0;
}
