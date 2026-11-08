#include "native_menu_portrait.h"
#include <melee/ft/native_gx_image.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_stock_content.h>
#include <melee/pl/player.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_source_resources.h>
#include <SDL3/SDL_filesystem.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void nativeMenuPortrait_Free(NativeMenuPortrait* portrait)
{
    if (!portrait) {
        return;
    }
    nativeGxImage_Free(portrait->pixels);
    memset(portrait, 0, sizeof(*portrait));
}
bool nativeMenuPortrait_Load(NativeMenuPortrait* result,
                             const NativeContentId* package, uint64_t stock_id, NativeMenuPortraitUse use)
{
    if (!result) {
        return false;
    }
    nativeMenuPortrait_Free(result);
    NativeContentId empty = { 0 };
    unsigned char* bytes = NULL;
    size_t size = 0;
    if (package && memcmp(package, &empty, sizeof(empty))) {
        char default_root[512];
        const char* root = nativeMatchLaunch_ContentRoot();
        if (!root &&
            nativeContentStore_DefaultRoot(default_root, sizeof(default_root)))
        {
            root = default_root;
        }
        NativeFighterPackage metadata;
        NativeFighterPackageData data;
        if (!root || !nativeContentStore_Read(root, package, &bytes, &size) ||
            !nativeContentStore_ParseFighter(bytes, size, &metadata, &data))
        {
            free(bytes);
            snprintf(result->title, sizeof(result->title), "Custom fighter");
            return false;
        }
        snprintf(result->title, sizeof(result->title), "%s", metadata.title);
        const uint8_t* picture=data.portrait_png; size_t picture_size=data.portrait_size;
        NativeSourceResources resources;
        if (use==NativeMenuPortrait_PlayerCard && nativeSourceResources_Read(data.members[0].resources,
                data.members[0].resource_size,&resources,NULL,0)) {
            const NativeSourceResourceEntry* card=nativeSourceResources_Find(&resources,"ui/player-card.png",NativeSourceResource_PNG);
            if (card) { picture=card->bytes; picture_size=card->size; }
        }
        result->pixels = nativeGxImage_Decode(picture,
                                              (unsigned) picture_size,
                                              &result->width, &result->height);
        free(bytes);
    } else {
        uint8_t character;
        if (!nativeStockContent_CharacterFromId(stock_id, &character)) {
            return false;
        }
        FighterKind kind = Player_800325C8(character, false);
        const char* name = nativeFighterProject_StockBaseName(kind);
        snprintf(result->title, sizeof(result->title), "%s",
                 name ? name : "Fighter");
        char filename[560];
        snprintf(filename, sizeof(filename), "%sui/portraits/%s-%u.png",
                 SDL_GetBasePath(),use==NativeMenuPortrait_PlayerCard?"card":"stock",kind);
        FILE* file = fopen(filename, "rb");
        if (!file) {
            return false;
        }
        long length;
        bool ok = !fseek(file, 0, SEEK_END) && (length = ftell(file)) > 0 &&
                  length <= 4 * 1024 * 1024 && !fseek(file, 0, SEEK_SET);
        bytes = ok ? malloc(length) : NULL;
        ok = bytes && fread(bytes, 1, length, file) == (size_t) length;
        ok = fclose(file) == 0 && ok;
        if (ok) {
            result->pixels = nativeGxImage_Decode(
                bytes, (unsigned) length, &result->width, &result->height);
        }
        free(bytes);
    }
    return result->pixels != NULL;
}
