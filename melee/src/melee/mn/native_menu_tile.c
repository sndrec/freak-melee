#include "native_menu_tile.h"
#include "native_menu_paint.h"
#include <melee/ft/native_gx_image.h>
#include <SDL3/SDL_filesystem.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned references;
static u8* frame;
static u16 frame_width, frame_height;

void nativeMenuTile_Acquire(void)
{
    if (references++) return;
    char path[560]; snprintf(path, sizeof(path), "%sui/portraits/tile-frame.png", SDL_GetBasePath());
    FILE* file = fopen(path, "rb");
    if (!file) return;
    long size = 0; u8* bytes = NULL;
    bool ok = !fseek(file, 0, SEEK_END) && (size = ftell(file)) > 0 && size <= 65536 && !fseek(file, 0, SEEK_SET);
    if (ok) { bytes = malloc(size); ok = bytes && fread(bytes, 1, size, file) == size; }
    fclose(file);
    if (ok) frame = nativeGxImage_Decode(bytes, (unsigned)size, &frame_width, &frame_height);
    free(bytes);
}

void nativeMenuTile_Release(void)
{
    if (!references || --references) return;
    nativeGxImage_Free(frame); frame = NULL;
}

unsigned nativeMenuTile_PortraitRect(const char* title, float x, float y,
                                     float width, float height, NativeMenuTilePortrait* portrait)
{
    char lines[2][128];
    unsigned count = nativeMenuFont_LayoutTileName(title, width*58/64, height*9/56, lines);
    if (portrait) *portrait = (NativeMenuTilePortrait){x+width*3/64, y+height*3/56,
        width*58/64, height*(count == 2 ? 31 : 38)/56};
    return count;
}

void nativeMenuTile_Draw(void* pixels, unsigned image_width, unsigned image_height,
                         const char* title, float x, float y, float width, float height)
{
    if (width <= 0 || height <= 0 || !title) return;
    NativeMenuTilePortrait photo;
    unsigned count = nativeMenuTile_PortraitRect(title, x, y, width, height, &photo);
    float top = count == 2 ? 34 : 41;
    // The stock portrait is transparent. Preserve that background, including
    // behind Game & Watch's black silhouette; only the name strip is opaque.
    nativeMenuPaint_Rect(x+width*3/64, y+height*top/56, width*58/64, height*(53-top)/56, 0x180000FF);
    if (pixels) nativeMenuPaint_Image(pixels, image_width, image_height,
        photo.x, photo.y, photo.width, photo.height, NativeMenuImage_Cover);
    char lines[2][128];
    nativeMenuFont_LayoutTileName(title, width*58/64, height*9/56, lines);
    for (unsigned i = 0; i < count; ++i)
        nativeMenuPaint_Text(lines[i], x+width*3/64, y+height*(top+1+i*9)/56,
            width*58/64, height*9/56, 0xF5EFEAFF, NativeMenuFont_Tile, true, true);
    if (frame) nativeMenuPaint_Image(frame, frame_width, frame_height,
        x, y, width, height, NativeMenuImage_Stretch);
}
