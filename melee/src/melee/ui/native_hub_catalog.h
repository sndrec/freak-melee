#pragma once
#include <melee/ft/native_fighter_family_project.h>
enum {
    NATIVE_HUB_FIGHTERS = 256
};
struct NativeHubFighter {
    NativeContentId id;
    char title[64];
    char draft[512];
    unsigned char* pixels;
    unsigned short width, height;
    bool installed;
};
extern NativeHubFighter nativeHubFighters[NATIVE_HUB_FIGHTERS];
extern unsigned nativeHubFighterCount;
bool nativeHubCatalog_Load(void);
void nativeHubCatalog_Clear(void);
bool nativeHubCatalog_CreateStock(FighterKind kind, const char* title,
                                  char draft[512]);
bool nativeHubCatalog_Copy(unsigned index, const char* title, char draft[512]);
bool nativeHubCatalog_MakeEditable(unsigned index, char draft[512]);
bool nativeHubCatalog_Rename(unsigned index, const char* title);
const char* nativeHubCatalog_Root(char buffer[512]);
