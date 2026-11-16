#include "native_source_hud.h"
#include "native_source_package.h"
#include "native_fighter_runtime.h"
#include "native_gx_image.h"
#include "types.h"
#include <melee/pl/player.h>
#include <sysdolphin/baselib/tobj.h>
#include <stdio.h>
#include <string.h>

bool nativeSourceHud_Prepare(NativeSourceHud* hud,
    const NativeSourceResources* resources, char* error, size_t capacity)
{
    static const char* const names[] = {"ui/stock-icon.png", "ui/emblem.png"};
    for (unsigned i = 0; i < NativeSourceHud_Count; ++i) {
        const NativeSourceResourceEntry* entry = nativeSourceResources_Find(
            resources, names[i], NativeSourceResource_PNG);
        if (!entry) continue;
        NativeSourceHudImage* image = &hud->images[i];
        image->image.image_ptr = nativeGxImage_Decode(entry->bytes, entry->size,
                                            &image->image.width, &image->image.height);
        image->image.format = GX_TF_RGBA8;
        if (!image->image.image_ptr) {
            if (error && capacity) snprintf(error, capacity, "Could not decode %s.", names[i]);
            nativeSourceHud_Release(hud);
            return false;
        }
    }
    return true;
}

void nativeSourceHud_Release(NativeSourceHud* hud)
{
    for (unsigned i = 0; i < NativeSourceHud_Count; ++i)
        nativeGxImage_Free(hud->images[i].image.image_ptr);
    memset(hud, 0, sizeof(*hud));
}

void nativeSourceHud_Apply(unsigned player, HSD_TObj* texture,
                            NativeSourceHudTarget target)
{
    if (!texture || target >= NativeSourceHud_Count) return;
    Fighter_GObj* actor = Player_GetEntityAtIndex(player, 0);
    if (!actor) return;
    const NativeSourcePackage* package = nativeFighterRuntime_SourcePackage(GET_FIGHTER(actor)->kind);
    if (!package) return;
    const NativeSourceHudImage* image = &package->hud.images[target];
    if (!image->image.image_ptr) return;
    texture->imagedesc = (HSD_ImageDesc*)&image->image;
    // The original stock icon is indexed. The replacement owns RGBA pixels.
    texture->tlut = NULL;
}
