#include "native_hub_catalog.h"
#include "native_shell_css_original.h"
#include <melee/ft/native_gx_image.h>
#include <melee/ft/native_fighter_template.h>
#include <melee/mn/native_css_original.h>
#include <melee/gm/native_match_launch.h>
#include <melee/gm/native_home.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_timer.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

NativeHubFighter nativeHubFighters[NATIVE_HUB_FIGHTERS];
unsigned nativeHubFighterCount;

const char* nativeHubCatalog_Root(char buffer[512])
{
    const char* root = nativeMatchLaunch_ContentRoot();
    return root                                          ? root
           : nativeContentStore_DefaultRoot(buffer, 512) ? buffer
                                                         : nullptr;
}
static bool read_file(const char* path, unsigned char** bytes, size_t* size)
{
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        return false;
    }
    long length = 0;
    bool ok = std::fseek(f, 0, SEEK_END) == 0 &&
              (length = std::ftell(f)) > 0 && length <= 16 * 1024 * 1024 &&
              std::fseek(f, 0, SEEK_SET) == 0;
    *bytes = ok ? static_cast<unsigned char*>(std::malloc(length)) : nullptr;
    ok = *bytes && std::fread(*bytes, 1, length, f) == size_t(length);
    ok = std::fclose(f) == 0 && ok;
    if (!ok) {
        std::free(*bytes);
        *bytes = nullptr;
        return false;
    }
    *size = length;
    return true;
}
void nativeHubCatalog_Clear(void)
{
    for (unsigned i = 0; i < nativeHubFighterCount; ++i) {
        nativeGxImage_Free(nativeHubFighters[i].pixels);
    }
    std::memset(nativeHubFighters, 0, sizeof(nativeHubFighters));
    nativeHubFighterCount = 0;
}
bool nativeHubCatalog_Load(void)
{
    nativeHubCatalog_Clear();
    char root_buffer[512];
    const char* root = nativeHubCatalog_Root(root_buffer);
    NativeContentId ids[NATIVE_HUB_FIGHTERS];
    size_t count = 0;
    if (!root || !nativeContentStore_ListInstalled(
                     root, ids, NATIVE_HUB_FIGHTERS, &count))
    {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        uint8_t* bytes = nullptr;
        size_t size = 0;
        NativeFighterPackage meta;
        NativeFighterPackageData data;
        if (!nativeContentStore_Read(root, &ids[i], &bytes, &size)) {
            continue;
        }
        if (nativeContentStore_ParseFighter(bytes, size, &meta, &data)) {
            NativeHubFighter& f = nativeHubFighters[nativeHubFighterCount++];
            f.id = ids[i];
            f.installed = true;
            std::snprintf(f.title, sizeof(f.title), "%s", meta.title);
            f.pixels = nativeGxImage_Decode(data.portrait_png,
                                            unsigned(data.portrait_size),
                                            &f.width, &f.height);
        }
        std::free(bytes);
    }
    NativeFighterDraft drafts[128];
    size_t draft_count = 0;
    if (!nativeFighterFamilyProject_ListLocal(drafts, 128, &draft_count)) {
        return false;
    }
    for (size_t i = 0; i < draft_count; ++i) {
        NativeHubFighter* entry = nullptr;
        char path[560], hex[66]{};
        std::snprintf(path, sizeof(path), "%s/installed.id", drafts[i].path);
        FILE* marker = std::fopen(path, "rb");
        NativeContentId id{};
        if (marker) {
            size_t n = std::fread(hex, 1, 65, marker);
            std::fclose(marker);
            hex[64] = 0;
            if (n == 65 && nativeContentStore_IdFromHex(hex, &id)) {
                for (unsigned j = 0; j < nativeHubFighterCount; ++j) {
                    if (!std::memcmp(&nativeHubFighters[j].id, &id,
                                     sizeof(id)))
                    {
                        entry = &nativeHubFighters[j];
                        break;
                    }
                }
            }
        }
        if (!entry) {
            if (nativeHubFighterCount == NATIVE_HUB_FIGHTERS) {
                continue;
            }
            entry = &nativeHubFighters[nativeHubFighterCount++];
            std::snprintf(entry->title, sizeof(entry->title), "%s",
                          drafts[i].title);
            std::snprintf(path, sizeof(path), "%s/portrait.png",
                          drafts[i].path);
            unsigned char* png = nullptr;
            size_t size = 0;
            if (read_file(path, &png, &size)) {
                entry->pixels = nativeGxImage_Decode(
                    png, unsigned(size), &entry->width, &entry->height);
            }
            std::free(png);
        }
        std::snprintf(entry->draft, sizeof(entry->draft), "%s",
                      drafts[i].path);
    }
    return true;
}
static bool new_path(char path[512])
{
    char root[512];
    if (!nativeFighterFamilyProject_LocalRoot(root, sizeof(root))) {
        return false;
    }
    int length = std::snprintf(path, 512, "%s/fighter-%016llx", root,
                               (unsigned long long) SDL_GetTicksNS());
    return length > 0 && length < 512;
}
static bool install_draft(const char* path, bool place_on_css)
{
    char png_path[560];
    std::snprintf(png_path, sizeof(png_path), "%s/portrait.png", path);
    unsigned char* portrait = nullptr;
    size_t portrait_size = 0;
    if (!read_file(png_path, &portrait, &portrait_size)) {
        std::snprintf(png_path, sizeof(png_path),
                      "%sui/workshop_draft_portrait.png", SDL_GetBasePath());
        if (!read_file(png_path, &portrait, &portrait_size)) {
            return false;
        }
        FILE* out = nullptr;
        std::snprintf(png_path, sizeof(png_path), "%s/portrait.png", path);
        out = std::fopen(png_path, "wb");
        bool saved = out && std::fwrite(portrait, 1, portrait_size, out) ==
                                portrait_size;
        if (out) {
            saved = std::fclose(out) == 0 && saved;
        }
        if (!saved) {
            std::free(portrait);
            return false;
        }
    }
    NativeFighterFamilyProject family{};
    bool ok = nativeFighterFamilyProject_Open(path, &family);
    uint8_t* packet = nullptr;
    size_t size = 0;
    NativeContentId id{}, replaced{};
    char root_buffer[512];
    const char* root = nativeHubCatalog_Root(root_buffer);
    ok = ok && root &&
         nativeFighterFamilyProject_Build(&family, portrait, portrait_size,
                                          &packet, &size, &id) &&
         nativeContentStore_Cache(root, packet, size, &id, &id) &&
         nativeFighterProject_Install(path, root, &id, &replaced);
    std::free(portrait);
    std::free(packet);
    nativeFighterFamilyProject_Close(&family);
    if (ok) {
        const NativeContentId zero{};
        if (std::memcmp(&replaced, &zero, sizeof(zero))) {
            nativeHome_ReplaceInstalledFighter(&replaced, &id);
        }
        nativeCssOriginal_ReloadInstalledFighters();
        const NativeCssLayout* layout = nativeCssOriginal_Layout();
        bool present = false;
        for (unsigned i = 0; layout && i < layout->count; ++i) {
            if (!std::memcmp(&layout->tiles[i].fighter_package_id, &id,
                             sizeof(id)))
            {
                present = true;
            }
        }
        if (place_on_css && !present) {
            nativeCssOriginal_AddInstalled(&id);
        }
        nativeShellCssOriginal_RefreshInstalledFighters();
    }
    return ok;
}
bool nativeHubCatalog_CreateStock(FighterKind kind, const char* title,
                                  char draft[512])
{
    NativeFighterFamilyMode mode = NativeFighterFamily_Single;
    FighterKind partner = Ft_Kind_None;
    if (kind == Ft_Kind_Zelda || kind == Ft_Kind_Seak) {
        mode = NativeFighterFamily_AlternateForm;
        partner = kind == Ft_Kind_Zelda ? Ft_Kind_Seak : Ft_Kind_Zelda;
    }
    if (kind == Ft_Kind_Popo || kind == Ft_Kind_Nana) {
        mode = NativeFighterFamily_SimultaneousCompanion;
        kind = Ft_Kind_Popo;
        partner = Ft_Kind_Nana;
    }
    if (!new_path(draft) || !nativeFighterFamilyProject_CreateStockFromDisc(
                                draft, title, mode, kind, partner))
    {
        return false;
    }
    if (!nativeFighterTemplate_Apply(draft)) {
        return false;
    }
    if (partner != Ft_Kind_None) {
        char partner_path[560];
        std::snprintf(partner_path, sizeof(partner_path), "%s/partner", draft);
        if (!nativeFighterTemplate_Apply(partner_path)) {
            return false;
        }
    }
    if (!install_draft(draft, true)) {
        return false;
    }
    NativeFighterFamilyProjectInfo info{};
    return nativeFighterFamilyProject_ReadInfo(draft, &info);
}
bool nativeHubCatalog_Copy(unsigned index, const char* title, char draft[512])
{
    if (index >= nativeHubFighterCount || !nativeHubFighters[index].installed)
    {
        return false;
    }
    char buffer[512];
    const char* root = nativeHubCatalog_Root(buffer);
    uint8_t* packet = nullptr;
    size_t size = 0;
    bool ok = root && new_path(draft) &&
              nativeContentStore_Read(root, &nativeHubFighters[index].id,
                                      &packet, &size) &&
              nativeFighterFamilyProject_CreateFromPackage(draft, title,
                                                           packet, size) &&
              install_draft(draft, true);
    std::free(packet);
    return ok;
}
bool nativeHubCatalog_MakeEditable(unsigned index, char draft[512])
{
    if (index >= nativeHubFighterCount) {
        return false;
    }
    const NativeHubFighter& f = nativeHubFighters[index];
    if (*f.draft) {
        std::snprintf(draft, 512, "%s", f.draft);
        return true;
    }
    char root_buffer[512];
    const char* root = nativeHubCatalog_Root(root_buffer);
    uint8_t* packet = nullptr;
    size_t size = 0;
    bool ok = root && new_path(draft) &&
              nativeContentStore_Read(root, &f.id, &packet, &size) &&
              nativeFighterFamilyProject_CreateFromPackage(draft, f.title,
                                                           packet, size) &&
              nativeFighterProject_Install(draft, root, &f.id, nullptr);
    std::free(packet);
    return ok;
}
bool nativeHubCatalog_Rename(unsigned index, const char* title)
{
    if (!title || !*title || std::strlen(title) >= 64) {
        return false;
    }
    char draft[512];
    if (!nativeHubCatalog_MakeEditable(index, draft)) {
        return false;
    }
    NativeFighterProjectInfo original{};
    if (!nativeFighterProject_ReadInfo(draft, &original) ||
        !nativeFighterProject_Rename(draft, title))
    {
        return false;
    }
    if (install_draft(draft, false)) {
        return true;
    }
    nativeFighterProject_Rename(draft, original.title);
    return false;
}
