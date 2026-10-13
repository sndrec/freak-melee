#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/forward.h>
#include <melee/gm/native_stock_content.h>
#include <melee/mn/native_css_layout.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

int main(void)
{
    NativeCssLayout layout;
    nativeCssLayout_Default(&layout);
    require(layout.count == 25 && nativeCssLayout_Validate(&layout),
            "default layout has the original 25 fighter tiles\n");
    NativeCssTile mario = layout.tiles[1];
    require(mario.stock_character_id ==
                nativeStockContent_CharacterId(CKind_Mario),
            "default layout keeps original roster order\n");

    NativeCssTile custom = {
        .x = 800,
        .y = 700,
        .width = 100,
        .height = 100,
    };
    custom.fighter_package_id.bytes[0] = 0x91;
    require(nativeCssLayout_Add(&layout, &custom) && layout.count == 26 &&
                !nativeCssLayout_Add(&layout, &custom),
            "one tile per stable fighter identity\n");
    custom.x = 950;
    require(!nativeCssLayout_Update(&layout, 25, &custom),
            "reject a tile extending past the canvas\n");
    custom.x = 720;
    custom.width = 180;
    require(nativeCssLayout_Update(&layout, 25, &custom) &&
                nativeCssLayout_Remove(&layout, 0) &&
                layout.count == 25 && nativeCssLayout_Validate(&layout),
            "move, resize, and remove without changing fighter identities\n");

    char directory[MAX_PATH];
    DWORD length = GetTempPathA(sizeof(directory), directory);
    require(length > 0 && length < sizeof(directory), "temporary directory\n");
    char path[MAX_PATH];
    int written = snprintf(path, sizeof(path), "%sFreakMeleeCss-%lu-%lu.bin",
                           directory, GetCurrentProcessId(), GetTickCount());
    require(written > 0 && written < sizeof(path), "temporary layout path\n");

    NativeCssLayout loaded;
    bool missing = false;
    require(!nativeCssLayout_LoadFile(&loaded, path, &missing) && missing,
            "missing installation layout is distinct from corruption\n");
    require(nativeCssLayout_SaveFile(&layout, path),
            "save installation layout\n");
    require(nativeCssLayout_LoadFile(&loaded, path, &missing) && !missing &&
                loaded.count == layout.count &&
                memcmp(loaded.tiles, layout.tiles,
                       layout.count * sizeof(layout.tiles[0])) == 0,
            "restart restores positions and stable identities\n");

    NativeContentId replacement = custom.fighter_package_id;
    replacement.bytes[0] = 0x92;
    require(nativeCssLayout_ReplaceFighter(&loaded,
                &custom.fighter_package_id, &replacement) &&
                loaded.tiles[24].fighter_package_id.bytes[0] == 0x92 &&
                loaded.tiles[24].x == 720 && loaded.tiles[24].width == 180 &&
                nativeCssLayout_SaveFile(&loaded, path),
            "fighter update retains edited CSS placement\n");
    NativeCssLayout reloaded;
    require(nativeCssLayout_LoadFile(&reloaded, path, &missing) &&
                reloaded.tiles[24].fighter_package_id.bytes[0] == 0x92,
            "updated fighter identity survives restart\n");

    HANDLE file = CreateFileA(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);
    require(file != INVALID_HANDLE_VALUE, "open layout for corruption test\n");
    require(SetFilePointer(file, 24, NULL, FILE_BEGIN) == 24,
            "seek into saved tile\n");
    DWORD changed = 0;
    const char bad = 'X';
    require(WriteFile(file, &bad, 1, &changed, NULL) && changed == 1 &&
                CloseHandle(file),
            "corrupt saved layout\n");
    NativeCssLayout preserved = loaded;
    require(!nativeCssLayout_LoadFile(&loaded, path, &missing) && !missing &&
                memcmp(&loaded, &preserved, sizeof(loaded)) == 0,
            "damaged tile does not replace the active layout\n");
    require(DeleteFileA(path), "remove test layout\n");
    puts("installation CSS layout persistence passed");
    return 0;
}
