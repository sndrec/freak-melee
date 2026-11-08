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
    if (!placing) return;
    float l,t,r,b; nativeCssOriginal_TileArea(&l,&t,&r,&b);
    float left = std::clamp(hub.pointer_x - tile.width * .32f, l, r - tile.width * .64f);
    float top = std::clamp(hub.pointer_y - tile.height * .24f, t, b - tile.height * .48f);
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
    nativeMenuSkin_Panel(16,445,608,32,0xC2BEC7FF,0x09080EF5);
    nativeMenuPaint_Text("Place your fighter. A or click to drop.",28,454,440,12,0xE5C361FF,NativeMenuFont_MenuOption);
    float l,t,r,b; nativeCssOriginal_TileArea(&l,&t,&r,&b);
    nativeHub_Region("setup-place-area",l,t,r-l,b-t,HubAction::StartupPlaceTile);
    nativeHub_Button("setup-back","< Back",514,449,98,25,HubAction::StartupBack);
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
bool nativeHubStartupStamp_IsPlacing(void) { return placing; }
