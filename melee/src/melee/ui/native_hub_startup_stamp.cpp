#include "native_hub_startup_stamp.h"
#include "native_hub_startup.h"
#include "native_hub_hand.h"
#include <melee/ft/native_gx_image.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_menu_skin.h>
#include <melee/mn/native_menu_tile.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    unsigned char* png;
    unsigned char* pixels;
    unsigned short width, height;
    unsigned bytes, index;
    NativeCssTile tile;
    bool placing;
}

void nativeHubStartupStamp_Unload()
{
    if (placing) nativeCssOriginal_CancelPreview();
    placing = false;
    std::free(png); png = nullptr; bytes = 0;
    nativeGxImage_Free(pixels); pixels = nullptr;
}

bool nativeHubStartupStamp_Load()
{
    nativeHubStartupStamp_Unload();
    char path[560]; std::snprintf(path, sizeof(path), "%s/portrait.png", nativeHubStartup.draft);
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    long size = 0;
    bool ok = !std::fseek(file, 0, SEEK_END) && (size = std::ftell(file)) > 0 && size <= 1024 * 1024 && !std::fseek(file, 0, SEEK_SET);
    if (ok) {
        png = static_cast<unsigned char*>(std::malloc(size));
        ok = png && std::fread(png, 1, size, file) == size;
    }
    std::fclose(file);
    if (ok) {
        bytes = unsigned(size);
        pixels = nativeGxImage_Decode(png, bytes, &width, &height);
        ok = pixels != nullptr;
    }
    if (!ok) nativeHubStartupStamp_Unload();
    return ok;
}

void nativeHubStartupStamp_RenderName()
{
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    nativeMenuPaint_Text("Make your stamp", 292, 107, 310, 31, gold, NativeMenuFont_MenuCaption);
    nativeMenuTile_Draw(pixels, width, height, nativeHubStartup.tile_name, 73, 178, 210, 210*56.f/64);
    nativeMenuPaint_Text("Short tile name", 338, 210, 260, 19, muted, NativeMenuFont_MenuOption);
    nativeHub_Field("setup-tile-name", nativeHubStartup.tile_name, 338, 245, 260, 38, HubAction::StartupTileName);
    nativeMenuPaint_Text("Printed beneath the portrait on character select.", 338, 298, 260, 10, muted);
    const char* name = nativeHubStartup.tile_name;
    bool nonempty = *name && std::strspn(name, " \t\r\n") != std::strlen(name);
    nativeHub_Button("setup-place-next", "Place on character select", 324, 415, 278, 39,
        HubAction::StartupNext, 0, true, nonempty);
}

bool nativeHubStartupStamp_BeginPlacement()
{
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    if (!layout || !nativeCssOriginal_CanEdit() || !png) return false;
    index = layout->count;
    tile = NativeCssTile{};
    tile.fighter_package_id = nativeHubStartup.fighter_id;
    tile.x = 450; tile.y = 280; tile.width = 92; tile.height = 120;
    if (!nativeCssOriginal_PreviewTile(index, &tile)) return false;
    nativeCssOriginal_SetPortrait(index, png, bytes, nativeHubStartup.tile_name);
    hub.pointer_x = (tile.x + tile.width * .5f) * .64f;
    hub.pointer_y = (tile.y + tile.height * .5f) * .48f;
    placing = true;
    return true;
}

void nativeHubStartupStamp_Move()
{
    if (!placing || !nativeHub_Hover(24, 94, 592, 292)) return;
    float left = std::clamp(hub.pointer_x - tile.width * .32f, 24.f, 616 - tile.width * .64f);
    float top = std::clamp(hub.pointer_y - tile.height * .24f, 94.f, 386 - tile.height * .48f);
    tile.x = uint16_t(std::lround(left / .64f));
    tile.y = uint16_t(std::lround(top / .48f));
    nativeCssOriginal_PreviewTile(index, &tile);
}

bool nativeHubStartupStamp_Place()
{
    if (!placing) return false;
    nativeHubStartupStamp_Move();
    if (!nativeCssOriginal_CommitPreview()) return false;
    placing = false;
    return true;
}

void nativeHubStartupStamp_CancelPlacement()
{
    if (placing) nativeCssOriginal_CancelPreview();
    placing = false;
}

void nativeHubStartupStamp_DrawCss(Mtx view)
{
    if (!placing || !hub.visible || !nativeHubStartup.active || nativeHubStartup.step != NativeStartup_Placement) return;
    float left, right, top, bottom;
    nativeCssOriginal_Viewport(&left, &right, &top, &bottom);
    Mtx canvas = {{(right-left)/640,0,0,left}, {0,(top-bottom)/480,0,top}, {0,0,1,5}}, model;
    C_MTXConcat(view, canvas, model);
    hub.region_count = 0;
    nativeMenuPaint_Begin(model);
    nativeMenuSkin_Panel(28, 400, 584, 61, 0xC2BEC7FF, 0x09080EF5);
    nativeMenuPaint_Text("Place your fighter", 42, 411, 350, 17, 0xE5C361FF, NativeMenuFont_MenuOption);
    nativeMenuPaint_Text("Move the hand. Press A or click to drop the tile.", 42, 438, 430, 10, 0xEEE9EDFF);
    nativeHub_Region("setup-place-area", 24, 94, 592, 292, HubAction::StartupPlaceTile);
    nativeHub_Button("setup-back", "< Back", 499, 418, 98, 29, HubAction::StartupBack);
    nativeHubHand_Draw(hub.pointer_x, hub.pointer_y, true, model);
    nativeMenuPaint_Begin(model);
    if (nativeHubStartup.error[0]) nativeMenuPaint_Text(nativeHubStartup.error, 42, 470, 550, 8, 0xFFAE92FF);
    nativeMenuPaint_End();
}

void nativeHubStartupStamp_Inspect()
{
    std::fprintf(stderr, "[hub-inspect] startup-stamp placing=%d index=%u xy=%u,%u size=%u,%u name=%s draft=%s\n",
        placing, index, tile.x, tile.y, tile.width, tile.height, nativeHubStartup.tile_name, nativeHubStartup.draft);
}
