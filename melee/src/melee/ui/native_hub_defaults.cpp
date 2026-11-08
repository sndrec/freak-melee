#include "native_hub_defaults.h"
#include "native_shell_css_original.h"
#include <melee/gm/native_home.h>
#include <melee/mn/native_css_original.h>
#include <melee/mn/native_menu_scene.h>
#include <dolphin/gx.h>
extern "C" {
#include <sysdolphin/baselib/state.h>
}
#include <SDL3/SDL.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
    enum class DefaultsMode { Closed, Confirm, Building, Failed };
    struct Defaults {
        DefaultsMode mode;
        NativeCssLayout stock, candidate;
        FighterKind kinds[25];
        NativeContentId baseline[Ft_Kind_StockCount];
        unsigned index, completed;
        char error[192], current[64];
    } defaults;

    bool registry_path(char path[560])
    {
        char buffer[512]; const char* root = nativeHubCatalog_Root(buffer);
        return root && std::snprintf(path, 560, "%s/default-roster.txt", root) < 560;
    }
    void load_registry()
    {
        char path[560]; if (!registry_path(path)) return;
        FILE* file = std::fopen(path, "rb"); if (!file) return;
        unsigned kind; char hex[65];
        while (std::fscanf(file, "%u %64s", &kind, hex) == 2)
            if (kind < Ft_Kind_StockCount) nativeContentStore_IdFromHex(hex, &defaults.baseline[kind]);
        std::fclose(file);
    }
    bool save_registry()
    {
        char path[560], temporary[568]; if (!registry_path(path)) return false;
        std::snprintf(temporary, sizeof(temporary), "%s.tmp", path);
        FILE* file = std::fopen(temporary, "wb"); if (!file) return false;
        bool ok = true; const NativeContentId zero{};
        for (unsigned kind = 0; kind < Ft_Kind_StockCount; ++kind) {
            if (!std::memcmp(&defaults.baseline[kind], &zero, sizeof(zero))) continue;
            char hex[65]; nativeContentStore_IdHex(&defaults.baseline[kind], hex);
            ok = std::fprintf(file, "%u %s\n", kind, hex) > 0 && ok;
        }
        ok = std::fclose(file) == 0 && ok;
        return ok && MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    bool prepare(FighterKind kind, NativeContentId* result)
    {
        char root_buffer[512]; const char* root = nativeHubCatalog_Root(root_buffer);
        if (!root) return false;
        NativeContentId& baseline = defaults.baseline[kind];
        uint8_t* packet = nullptr; size_t bytes = 0;
        NativeFighterPackage metadata{}; NativeFighterPackageData spans{};
        bool cached = nativeContentStore_Read(root, &baseline, &packet, &bytes) &&
            nativeContentStore_ParseFighter(packet, bytes, &metadata, &spans);
        if (cached) {
            for (unsigned i = 0; i < nativeHubFighterCount; ++i) {
                const auto& fighter = nativeHubFighters[i]; NativeFighterFamilyProjectInfo info{};
                if (fighter.installed && !std::memcmp(&fighter.id, &baseline, sizeof(baseline)) &&
                    fighter.draft[0] && nativeFighterFamilyProject_ReadInfo(fighter.draft, &info)) {
                    std::free(packet); *result = baseline; return true;
                }
            }
            char projects[512], draft[560];
            bool ok = nativeFighterFamilyProject_LocalRoot(projects, sizeof(projects)) &&
                std::snprintf(draft, sizeof(draft), "%s/default-%u-%016llx", projects, unsigned(kind),
                    static_cast<unsigned long long>(SDL_GetTicksNS())) < sizeof(draft) &&
                nativeFighterFamilyProject_CreateFromPackage(draft, metadata.title, packet, bytes) &&
                nativeFighterProject_Install(draft, root, &baseline, nullptr);
            std::free(packet);
            if (ok) *result = baseline;
            return ok;
        }
        std::free(packet);
        char draft[512];
        if (!nativeHubCatalog_CreateStockDraft(kind, nativeFighterProject_StockBaseName(kind), draft)) return false;
        const char* tile_name = kind == Ft_Kind_Donkey ? "D K" : kind == Ft_Kind_Captain ? "C. Falcon" : "";
        if ((*tile_name && !nativeFighterProject_SetTileName(draft, tile_name)) ||
            !nativeHubCatalog_PublishDraft(draft, NativeHubPublish::Batch, result)) return false;
        baseline = *result;
        return save_registry();
    }
    void failed(const char* message)
    {
        defaults.mode = DefaultsMode::Failed;
        std::snprintf(defaults.error, sizeof(defaults.error), "%s", message);
        nativeHubCatalog_Load();
        nativeShellCssOriginal_RefreshInstalledFighters();
    }
    void reference(float x, float y, bool filled)
    {
        float u0, v0, u1, v1;
        if (!nativeMenuScene_Project(x, y, &u0, &v0) || !nativeMenuScene_Project(x+205, y+153.75f, &u1, &v1)) return;
        float viewport[6], projection[7]; u32 scissor[4];
        GXGetViewportv(viewport); GXGetProjectionv(projection);
        GXGetScissor(&scissor[0], &scissor[1], &scissor[2], &scissor[3]);
        float left = viewport[0]+u0*viewport[2], top = viewport[1]+v0*viewport[3];
        float width = (u1-u0)*viewport[2], height = (v1-v0)*viewport[3];
        GXSetViewport(left,top,width,height,viewport[4],viewport[5]);
        GXSetScissor(unsigned(left),unsigned(top),unsigned(width),unsigned(height));
        nativeCssOriginal_DrawReference(filled);
        GXSetViewport(viewport[0],viewport[1],viewport[2],viewport[3],viewport[4],viewport[5]);
        GXSetScissor(scissor[0],scissor[1],scissor[2],scissor[3]); GXSetProjectionv(projection);
        Mtx view; Mtx44 p; nativeMenuScene_View(false,false,view,p); nativeMenuPaint_Begin(view);
    }
}

bool nativeHubDefaults_Activate(HubAction action)
{
    auto& s = defaults;
    if (action == HubAction::DefaultsOpen && s.mode == DefaultsMode::Closed) {
        if (!nativeHome_IsSelection() || !nativeCssOriginal_CanEdit()) return true;
        s = Defaults{};
        if (!nativeCssOriginal_StockRoster(&s.stock, s.kinds)) return true;
        load_registry();
        nativeHubCatalog_Load(); nativeHub_ChangePage(hub.page);
        s.mode = DefaultsMode::Confirm; return true;
    }
    if (s.mode == DefaultsMode::Closed) return false;
    if (action == HubAction::DefaultsConfirm && s.mode != DefaultsMode::Building) {
        s.index = s.completed = 0; s.candidate = NativeCssLayout{};
        s.error[0] = 0; s.mode = DefaultsMode::Building; return true;
    }
    if (action == HubAction::Back || action == HubAction::DefaultsCancel) {
        nativeHubCatalog_Load(); nativeShellCssOriginal_RefreshInstalledFighters();
        s.mode = DefaultsMode::Closed; return true;
    }
    return true;
}

void nativeHubDefaults_Tick()
{
    auto& s = defaults; if (s.mode != DefaultsMode::Building) return;
    while (s.index < s.stock.count) {
        FighterKind kind = s.kinds[s.index];
        const NativeCssTile& original = s.stock.tiles[s.index++];
        if (kind == Ft_Kind_Kirby) continue;
        if (kind < 0 || kind >= Ft_Kind_StockCount) { failed("Could not read the default roster."); return; }
        std::snprintf(s.current, sizeof(s.current), "%s", nativeFighterProject_StockBaseName(kind));
        NativeCssTile tile = original; tile.stock_character_id = 0;
        if (!prepare(kind, &tile.fighter_package_id) || !nativeCssLayout_Add(&s.candidate, &tile)) {
            char message[192]; std::snprintf(message,sizeof(message),"Could not finish %s. Your CSS is unchanged. Completed fighters are in the Workshop.",s.current);
            failed(message); return;
        }
        ++s.completed;
        return;
    }
    if (s.completed != 24 || !save_registry() || !nativeCssOriginal_ReplaceLayout(&s.candidate)) {
        failed("Could not save the default layout. Your previous CSS is unchanged."); return;
    }
    nativeShellCssOriginal_RefreshInstalledFighters(); nativeHubCatalog_Load();
    hub.selected = nativeHubFighterCount ? 0 : -1;
    s.mode = DefaultsMode::Closed;
    nativeHub_Notice("Melee roster added. Every fighter is editable.");
}

void nativeHubDefaults_Render()
{
    const auto& s = defaults; if (s.mode == DefaultsMode::Closed) return;
    hub.region_count = 0;
    nativeMenuPaint_Rect(0,0,640,480,0x000000D0);
    nativeHub_Panel(45,105,550,337);
    nativeMenuPaint_Text("Populate the Character Select Screen",72,126,500,18,0xE5C361FF,NativeMenuFont_MenuOption,true);
    nativeMenuPaint_Text("with the default SSBM roster?",72,152,500,18,0xE5C361FF,NativeMenuFont_MenuOption,true);
    reference(73,190,false); reference(362,190,true);
    nativeMenuPaint_Line(300,263,341,263,2,0xE5C361FF);
    nativeMenuPaint_Line(330,253,341,263,2,0xE5C361FF); nativeMenuPaint_Line(330,273,341,263,2,0xE5C361FF);
    bool busy = s.mode == DefaultsMode::Building;
    char status[192]; std::snprintf(status,sizeof(status),"%u / 24   %s",s.completed,s.current);
    nativeMenuPaint_Text(busy?status:s.mode==DefaultsMode::Failed?s.error:"24 editable fighters. Existing fighters stay in the Workshop.",73,361,494,11,0xEEE9EDFF);
    nativeHub_Button("defaults-cancel", "< Back",73,401,115,28,HubAction::DefaultsCancel);
    nativeHub_Button("defaults-confirm",busy?"Adding fighters...":s.mode==DefaultsMode::Failed?"Try again":"Add Melee roster",350,397,216,35,
        HubAction::DefaultsConfirm,0,true,!busy);
}

void nativeHubDefaults_Shutdown() { defaults = Defaults{}; }
bool nativeHubDefaults_IsOpen() { return defaults.mode != DefaultsMode::Closed; }
void nativeHubDefaults_Inspect()
{
    std::fprintf(stderr,"[hub-inspect] defaults mode=%u completed=%u tiles=%u current=%s error=%s\n",
        unsigned(defaults.mode),defaults.completed,defaults.candidate.count,defaults.current,defaults.error);
}
