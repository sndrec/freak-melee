#include "native_hub_costumes.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#include <melee/ft/native_costume_import.h>
#include <melee/ft/native_costume_catalog.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/native_home.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <algorithm>

namespace {
    struct Player {
        bool active;
        uint8_t character, color;
        NativeContentId fighter, outfit;
        char title[NATIVE_CONTENT_TITLE_BYTES];
    } players[4];
    int port = -1;
    unsigned first;
    char status[160];
    NativeHubPage return_page;
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    const char* names[] = {"Captain Falcon","Donkey Kong","Fox","Mr. Game & Watch","Kirby","Bowser","Link","Luigi","Mario","Marth","Mewtwo",
        "Ness","Peach","Pikachu","Ice Climbers","Jigglypuff","Samus","Yoshi","Zelda","Sheik","Falco","Young Link","Dr. Mario","Roy","Pichu","Ganondorf"};
    void message(const char* text) { std::snprintf(status,sizeof(status),"%s",text); }
    bool same_id(const NativeContentId* a,const NativeContentId* b) { return std::memcmp(a,b,sizeof(*a)) == 0; }
    bool compatible(const NativeCostumeEntry& entry)
    {
        if (port < 0 || !players[port].active) return false;
        const auto& p = players[port];
        return same_id(&entry.metadata.fighter_id,&p.fighter) && entry.metadata.character == p.character && entry.metadata.base_costume == p.color;
    }
    void player_title(Player& p)
    {
        NativeContentId empty = {};
        std::snprintf(p.title,sizeof(p.title),"%s",p.character < 26 ? names[p.character] : "Fighter");
        if (same_id(&p.fighter,&empty)) return;
        char path[512]; const char* root = nativeHubCatalog_Root(path); uint8_t* bytes = nullptr; size_t size = 0;
        NativeFighterPackage metadata;
        if (root && nativeContentStore_Read(root,&p.fighter,&bytes,&size) && nativeContentStore_ParseFighter(bytes,size,&metadata,nullptr))
            std::snprintf(p.title,sizeof(p.title),"%s",metadata.title);
        std::free(bytes);
    }
void import_costume(unsigned port)
{
    char default_root[512];
    const char* root = nativeHubCatalog_Root(default_root);
    if (!root) { message("Could not open the collection."); return; }
    NativeCostumePackage metadata = {};
    metadata.fighter_id = players[port].fighter;
    metadata.character = players[port].character;
    metadata.base_costume = players[port].color;
    metadata.member_count = 1;
    const NativeContentId empty = {};
    if (!same_id(&metadata.fighter_id, &empty)) {
        NativeFighterCostumeFamilyInfo family;
        if (!nativeFighterRuntime_CostumeInfoCached(&metadata.fighter_id, root, metadata.model_variant, &family)) {
            message("Could not read this fighter's outfit requirements.");
            return;
        }
        metadata.member_count = family.member_count;
        for (unsigned m = 0; m < metadata.member_count; ++m)
            std::memcpy(metadata.member_keys[m], family.members[m].key, NATIVE_FIGHTER_CODE_KEY_BYTES);
    }
    NativeCostumePackageData data = {};
    uint8_t* owned[NATIVE_FIGHTER_FAMILY_MEMBERS] = {};
    bool read = true;
    for (unsigned m = 0; read && m < metadata.member_count; ++m) {
        wchar_t path[32768] = {};
        wchar_t caption[128] = L"Choose outfit model";
        if (metadata.member_count > 1)
            swprintf_s(caption, L"Choose outfit model: %hs (%u of %u)",
                       metadata.member_keys[m], m + 1, metadata.member_count);
        OPENFILENAMEW picker = {};
        picker.lStructSize = sizeof(picker);
        picker.hwndOwner = GetActiveWindow();
        picker.lpstrTitle = caption;
        picker.lpstrFilter = L"Costume DAT\0*.dat\0All files\0*.*\0";
        picker.lpstrFile = path;
        picker.nMaxFile = sizeof(path) / sizeof(path[0]);
        picker.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
        if (!GetOpenFileNameW(&picker)) {
            message(CommDlgExtendedError() ? "Could not open the file picker." : "Import cancelled.");
            read = false;
            break;
        }
        if (m == 0) {
            const wchar_t* basename = wcsrchr(path, L'\\');
            basename = basename ? basename + 1 : path;
            const wchar_t* extension = wcsrchr(basename, L'.');
            int length = int((extension ? extension : basename + wcslen(basename)) - basename);
            int bytes = length > 0 ? WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                basename, length, metadata.title, NATIVE_CONTENT_TITLE_BYTES - 1, nullptr, nullptr) : 0;
            if (bytes <= 0) std::strcpy(metadata.title, "Imported outfit");
        }
        FILE* file = nullptr;
        if (_wfopen_s(&file, path, L"rb") || !file) {
            message("Could not open that DAT."); read = false; break;
        }
        long length = -1;
        if (std::fseek(file, 0, SEEK_END) == 0) length = std::ftell(file);
        read = length >= 32 && length <= NATIVE_CONTENT_MAX_ASSET_BYTES &&
               std::fseek(file, 0, SEEK_SET) == 0;
        if (read) {
            owned[m] = static_cast<uint8_t*>(std::malloc(size_t(length)));
            read = owned[m] && std::fread(owned[m], 1, size_t(length), file) == size_t(length);
            data.members[m] = {owned[m], size_t(length)};
        }
        std::fclose(file);
        if (!read) message("Could not read the outfit model.");
    }
    NativeContentId id;
    NativeCostumeValidation validation = {};
    bool installed = read && nativeCostumeImport_Install(root, &metadata, &data, &id, &validation);
    for (unsigned m = 0; m < metadata.member_count; ++m) std::free(owned[m]);
    if (!read) return;
    if (!installed) {
        message("Outfit models do not match this fighter family, or installation failed.");
        return;
    }
    nativeCostumeCatalog_Refresh();
    if (!nativeHome_SelectCostumePackage(port, &id)) {
        message("Installed. Return to CSS to select it.");
        return;
    }
    message("Outfit imported and selected.");
}

}
void nativeHubCostumes_Open()
{
    if (!nativeHome_IsSelection()) { nativeHub_Notice("Choose a fighter on character select to manage outfits."); return; }
    return_page = hub.page; first = 0; status[0] = 0;
    std::memset(players,0,sizeof(players)); port = -1;
    if (!nativeCostumeCatalog_Refresh()) message("Could not read the whole outfit collection.");
    nativeHub_Open(NativeHubPage_Costumes); nativeHubCostumes_Tick();
}
void nativeHubCostumes_Tick()
{
    if (hub.page != NativeHubPage_Costumes || !hub.visible) return;
    for (unsigned i=0;i<4;++i) {
        Player p = {}; p.active = nativeHome_CssPlayer(i,&p.character,&p.color);
        if (const auto* id = nativeHome_FighterPackage(i)) p.fighter = *id;
        if (const auto* id = nativeHome_CostumePackage(i)) p.outfit = *id;
        const auto& prev = players[i];
        if (p.active != prev.active || p.character != prev.character || p.color != prev.color || !same_id(&p.fighter,&prev.fighter)) {
            player_title(p); players[i] = p;
            if (int(i) == port) first = 0;
        } else players[i].outfit = p.outfit;
    }
    if (port < 0 || !players[port].active) {
        port = -1; for (unsigned i=0;i<4;++i) if (players[i].active) { port=int(i); break; }
        first = 0;
    }
}
bool nativeHubCostumes_Activate(HubAction action,int value)
{
    if (action == HubAction::Costumes) { nativeHubCostumes_Open(); return true; }
    if (hub.page != NativeHubPage_Costumes) return false;
    switch (action) {
    case HubAction::Back: nativeHub_ChangePage(return_page,true); return true;
    case HubAction::CostumePort: if (unsigned(value)<4 && players[value].active) { port=value; first=0; status[0]=0; } return true;
    case HubAction::CostumePage: first=unsigned(std::max(0,int(first)+value)); return true;
    case HubAction::CostumeDefault:
        if (port>=0 && !nativeHome_SelectCostumePackage(port,nullptr)) message("Could not select the default outfit.");
        else message("Default outfit selected.");
        return true;
    case HubAction::CostumeChoose: {
        size_t count; const auto* entries=nativeCostumeCatalog_Entries(&count);
        if (value<0 || size_t(value)>=count || !compatible(entries[value]) || !nativeHome_SelectCostumePackage(port,&entries[value].id)) message("That outfit is missing or incompatible.");
        else message("Outfit selected.");
        return true;
    }
    case HubAction::CostumeImport:
        if (port>=0 && players[port].active) import_costume(unsigned(port));
        return true;
    default: return false;
    }
}
void nativeHubCostumes_Render()
{
    nativeHub_Button("costume-close","< Back",40,402,110,28,HubAction::Back);
    nativeMenuPaint_Text(status,40,450,555,11,muted);
    bool any=false;
    for (unsigned i=0;i<4;++i) {
        char id[40], label[40]; std::snprintf(id,sizeof(id),"costume-port-%u",i); std::snprintf(label,sizeof(label),"P%u",i+1);
        nativeHub_Button(id,label,45+i*141,106,128,29,HubAction::CostumePort,int(i),int(i)==port,players[i].active);
        any |= players[i].active;
    }
    if (!any) { nativeHub_TextBlock("Select a fighter on character select to choose an outfit.",52,180,530,80,16,muted); return; }
    nativeMenuPaint_Text(players[port].title,51,159,529,19,gold,NativeMenuFont_MenuOption);
    NativeContentId empty = {};
    nativeHub_Button("costume-stock","Default outfit",51,192,531,27,HubAction::CostumeDefault,0,same_id(&players[port].outfit,&empty));
    size_t count; const auto* entries=nativeCostumeCatalog_Entries(&count);
    unsigned row=0,total=0;
    for (size_t i=0;i<count;++i) {
        if (!compatible(entries[i])) continue;
        if (total++<first || row>=4) continue;
        char id[40]; std::snprintf(id,sizeof(id),"costume-entry-%u",unsigned(i));
        nativeHub_Button(id,entries[i].metadata.title,51,226+row*33,531,27,HubAction::CostumeChoose,int(i),same_id(&players[port].outfit,&entries[i].id)); ++row;
    }
    if (!total) nativeMenuPaint_Text("No additional outfits for this fighter and color.",52,249,527,13,muted);
    nativeHub_Button("costume-prev","<",51,365,42,25,HubAction::CostumePage,-4,false,first>0);
    nativeHub_Button("costume-next",">",103,365,42,25,HubAction::CostumePage,4,false,first+4<total);
    nativeHub_Button("costume-import","Import outfit DAT...",325,365,257,25,HubAction::CostumeImport);
}
void nativeHubCostumes_Inspect()
{
    size_t count; nativeCostumeCatalog_Entries(&count);
    std::fprintf(stderr,"[hub-inspect] costumes page=%u port=%d count=%zu first=%u status=%s\n",unsigned(hub.page),port,count,first,status);
    for (unsigned i=0;i<4;++i) {
        char hex[65]; nativeContentStore_IdHex(&players[i].outfit,hex);
        std::fprintf(stderr,"[hub-inspect] costume-player port=%u active=%d character=%u color=%u title=%s outfit=%s\n",i,players[i].active,unsigned(players[i].character),unsigned(players[i].color),players[i].title,hex);
    }
}
