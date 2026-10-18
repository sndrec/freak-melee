#include "native_menu_skin.h"
#include "native_menu_paint.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dolphin/gx.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static u8 frame_border[48 * 32 / 2] ATTRIBUTE_ALIGN(32);
static unsigned references;
static bool loaded;

void nativeMenuSkin_Acquire(void)
{
    if (references++) return;
    char path[512];
    DWORD length = GetModuleFileNameA(NULL, path, sizeof(path));
    char* basename = length && length < sizeof(path) ? strrchr(path, '\\') : NULL;
    if (!basename) return;
    snprintf(basename, sizeof(path) - (basename - path), "\\ui\\skins\\menu-panel.bin");
    FILE* file = fopen(path, "rb");
    if (file) {
        loaded = fread(frame_border, 1, sizeof(frame_border), file) == sizeof(frame_border) &&
            fgetc(file) == EOF;
        fclose(file);
    }
    if (!loaded) fprintf(stderr, "Could not load menu panel artwork: %s\n", path);
}

void nativeMenuSkin_Release(void)
{
    if (!references || --references) return;
    loaded = false;
}

void nativeMenuSkin_Frame(float x, float y, float width, float height, u32 rgba)
{
    if (!loaded) return;
    GXTexObj texture;
    GXInitTexObj(&texture, frame_border, 48, 32, GX_TF_I4,
        GX_CLAMP, GX_CLAMP, GX_FALSE);
    GXLoadTexObj(&texture, GX_TEXMAP0);
    GXSetTevColor(GX_TEVREG0, (GXColor){ rgba >> 24, rgba >> 16, rgba >> 8, rgba });
    GXSetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0);
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_A0, GX_CA_ZERO);
    float edge = fminf(6, fminf(width, height) * .5f);
    const float px[] = { x, x + edge, x + width - edge, x + width };
    const float py[] = { y, y + edge, y + height - edge, y + height };
    const float u[] = { 0, 8.f / 48, 40.f / 48, 1 };
    const float v[] = { 0, 8.f / 32, 24.f / 32, 1 };
    GXBegin(GX_QUADS, GX_VTXFMT0, 36);
    for (unsigned row = 0; row < 3; ++row) for (unsigned col = 0; col < 3; ++col) {
        GXPosition3f32(px[col], -py[row], -5); GXTexCoord2f32(u[col], v[row]);
        GXPosition3f32(px[col+1], -py[row], -5); GXTexCoord2f32(u[col+1], v[row]);
        GXPosition3f32(px[col+1], -py[row+1], -5); GXTexCoord2f32(u[col+1], v[row+1]);
        GXPosition3f32(px[col], -py[row+1], -5); GXTexCoord2f32(u[col], v[row+1]);
    }
    GXEnd();
}

void nativeMenuSkin_Panel(float x, float y, float width, float height,
                          u32 border, u32 fill)
{
    nativeMenuPaint_Round(x + 1, y + 1, width - 2, height - 2, 5, fill);
    nativeMenuSkin_Frame(x, y, width, height, border);
}

void nativeMenuSkin_Button(const char* label, float x, float y, float width,
                           float height, float text_height, bool active, bool enabled)
{
    active = active && enabled;
    nativeMenuPaint_Gradient(x + 2, y + 2, width - 4, height - 4,
        active ? 0xFFE451FF : enabled ? 0x302A25F4 : 0x17151AF0,
        active ? 0xC1971FFF : enabled ? 0x0A090BF4 : 0x0A090BF0);
    nativeMenuSkin_Frame(x, y, width, height,
        active ? 0xFFF5A5FF : enabled ? 0xBDAA6EFF : 0x5B5660FF);
    nativeMenuPaint_Text(label, x + 7, y + (height - text_height) * .5f,
        width - 14, text_height,
        active ? 0x17121CFF : enabled ? 0xF2ECEDFF : 0x81788AFF,
        NativeMenuFont_MenuOption, true, !active);
}
