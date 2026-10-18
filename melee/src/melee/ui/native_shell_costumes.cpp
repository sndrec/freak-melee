#include "native_shell_costumes.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <melee/ft/native_costume_import.h>
#include <melee/ft/native_costume_catalog.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/gm/native_content_store.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>

namespace {

Rml::ElementDocument* document;
const NativeCostumeEntry* catalog;
size_t catalog_count;
bool selection_active;
bool opened;
int section_visible = -1;
int open_port = -1;
uint8_t shown_characters[4];
uint8_t shown_colors[4];
NativeContentId shown_packages[4];
NativeContentId shown_fighters[4];
bool shown_players[4];

const char* character_names[] = {
    "Captain Falcon", "Donkey Kong", "Fox", "Mr. Game & Watch",
    "Kirby", "Bowser", "Link", "Luigi", "Mario", "Marth", "Mewtwo",
    "Ness", "Peach", "Pikachu", "Ice Climbers", "Jigglypuff", "Samus",
    "Yoshi", "Zelda", "Sheik", "Falco", "Young Link", "Dr. Mario",
    "Roy", "Pichu", "Ganondorf",
};

std::string escape_rml(const char* value)
{
    std::string escaped;
    for (; *value != '\0'; ++value) {
        switch (*value) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        default: escaped += *value; break;
        }
    }
    return escaped;
}

bool same_id(const NativeContentId* a, const NativeContentId* b)
{
    return std::memcmp(a, b, sizeof(*a)) == 0;
}

const char* content_root(char path[512])
{
    const char* root = nativeMatchLaunch_ContentRoot();
    if (root != nullptr) {
        return root;
    }
    return nativeContentStore_DefaultRoot(path, 512) ? path : nullptr;
}

void set_status(const char* message)
{
    document->GetElementById("costume-status")->SetInnerRML(message);
}

void load_catalog();

void import_costume(unsigned port)
{
    char default_root[512];
    const char* root = content_root(default_root);
    if (!root) { set_status("Could not open the collection."); return; }
    NativeCostumePackage metadata = {};
    metadata.fighter_id = shown_fighters[port];
    metadata.character = shown_characters[port];
    metadata.base_costume = shown_colors[port];
    metadata.member_count = 1;
    const NativeContentId empty = {};
    if (!same_id(&metadata.fighter_id, &empty)) {
        NativeFighterCostumeFamilyInfo family;
        if (!nativeFighterRuntime_CostumeInfoCached(&metadata.fighter_id, root, &family)) {
            set_status("Could not read this fighter's outfit requirements.");
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
            set_status(CommDlgExtendedError() ? "Could not open the file picker." : "Import cancelled.");
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
            set_status("Could not open that DAT."); read = false; break;
        }
        long length = -1;
        if (std::fseek(file, 0, SEEK_END) == 0) length = std::ftell(file);
        read = length >= 32 && length < NATIVE_CONTENT_MAX_PACKAGE_BYTES &&
               std::fseek(file, 0, SEEK_SET) == 0;
        if (read) {
            owned[m] = static_cast<uint8_t*>(std::malloc(size_t(length)));
            read = owned[m] && std::fread(owned[m], 1, size_t(length), file) == size_t(length);
            data.members[m] = {owned[m], size_t(length)};
        }
        std::fclose(file);
        if (!read) set_status("Could not read the outfit model.");
    }
    NativeContentId id;
    NativeCostumeValidation validation = {};
    bool installed = read && nativeCostumeImport_Install(root, &metadata, &data, &id, &validation);
    for (unsigned m = 0; m < metadata.member_count; ++m) std::free(owned[m]);
    if (!read) return;
    if (!installed) {
        set_status("Outfit models do not match this fighter family, or installation failed.");
        return;
    }
    load_catalog();
    if (!nativeHome_SelectCostumePackage(port, &id)) {
        set_status("Installed. Return to CSS to select it.");
        return;
    }
    set_status("Outfit imported and selected.");
}

void load_catalog()
{
    if (!nativeCostumeCatalog_Refresh()) set_status("Could not read the whole outfit collection.");
    catalog = nativeCostumeCatalog_Entries(&catalog_count);
}

bool parse_index(const Rml::String& id, const char* prefix, unsigned* index)
{
    const size_t length = std::strlen(prefix);
    return id.compare(0, length, prefix) == 0 &&
           std::sscanf(id.c_str() + length, "%u", index) == 1;
}

class CostumeEvents final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        if (event.GetType() == "keydown") {
            if (event.GetParameter<int>("key_identifier",0) == Rml::Input::KI_ESCAPE) {
                nativeShellCostumes_Close();
                event.StopPropagation();
            }
            return;
        }
        if (event.GetType() != "click") {
            return;
        }
        const Rml::String& id = event.GetCurrentElement()->GetId();
        if (id == "costume-close") {
            nativeShellCostumes_Close();
            return;
        }
        unsigned index;
        if (parse_index(id, "costume-port-", &index) && index < 4) {
            open_port = open_port == int(index) ? -1 : int(index);
        } else if (id == "costume-stock" && open_port >= 0) {
            if (!nativeHome_SelectCostumePackage(open_port, nullptr)) {
                document->GetElementById("costume-status")
                    ->SetInnerRML("Could not select the stock costume.");
            }
        } else if (id == "costume-import" && open_port >= 0 &&
                   shown_players[open_port]) {
            import_costume(unsigned(open_port));
        } else if (parse_index(id, "costume-entry-", &index) &&
                   index < catalog_count && open_port >= 0) {
            if (!nativeHome_SelectCostumePackage(open_port,
                                                 &catalog[index].id)) {
                document->GetElementById("costume-status")
                    ->SetInnerRML("Costume is missing or incompatible.");
            } else {
                document->GetElementById("costume-status")->SetInnerRML("");
            }
        } else {
            return;
        }
        // A click is a menu transition; rebuild the small fixed catalog view.
        std::memset(shown_players, 0, sizeof(shown_players));
        nativeShellCostumes_Update();
    }
} events;

void rebuild(const uint8_t characters[4], const uint8_t colors[4],
             const NativeContentId packages[4], const NativeContentId fighters[4], const bool players[4])
{
    bool any = false;
    for (int port = 0; port < 4; ++port) {
        if (!players[port]) {
            continue;
        }
        any = true;
    }
    document->GetElementById("costume-section")
        ->SetProperty("display", "block");
    section_visible = int(any);
    if (!any) {
        open_port = -1;
        document->GetElementById("costume-players")->SetInnerRML(
            "Select a fighter on character select to choose an outfit.");
        document->GetElementById("costume-choices")->SetProperty("display","none");
        return;
    }

    std::string markup;
    for (unsigned port = 0; port < 4; ++port) {
        if (!players[port]) {
            continue;
        }
        char fighter_name[NATIVE_CONTENT_TITLE_BYTES] = "Fighter";
        const NativeContentId empty = {};
        if (same_id(&fighters[port], &empty)) {
            if (characters[port] < sizeof(character_names) / sizeof(character_names[0]))
                std::snprintf(fighter_name, sizeof(fighter_name), "%s", character_names[characters[port]]);
        } else {
            char default_root[512];
            const char* root = content_root(default_root);
            uint8_t* bytes = nullptr;
            size_t size;
            NativeFighterPackage family;
            NativeFighterPackageData data;
            if (root && nativeContentStore_Read(root, &fighters[port], &bytes, &size) &&
                nativeContentStore_ParseFighter(bytes, size, &family, &data))
                std::memcpy(fighter_name, family.title, sizeof(fighter_name));
            std::free(bytes);
        }
        const char* chosen = "Default";
        for (size_t index = 0; index < catalog_count; ++index) {
            if (same_id(&packages[port], &catalog[index].id)) {
                chosen = catalog[index].metadata.title;
                break;
            }
        }
        markup += "<button id=\"costume-port-" + std::to_string(port) +
                  "\"" + (open_port == int(port) ? " class=\"active\"" : "") +
                  ">P" + std::to_string(port + 1) + " " +
                  escape_rml(fighter_name) + " / " +
                  escape_rml(chosen) + "</button>";
    }
    document->GetElementById("costume-players")->SetInnerRML(markup);
    for (unsigned port = 0; port < 4; ++port) {
        Rml::Element* button = document->GetElementById(
            "costume-port-" + std::to_string(port));
        if (button != nullptr) {
            button->AddEventListener("click", &events);
        }
    }

    Rml::Element* choices = document->GetElementById("costume-choices");
    if (open_port < 0 || !players[open_port]) {
        choices->SetProperty("display", "none");
        return;
    }
    choices->SetProperty("display", "block");
    const NativeContentId empty = {};
    markup = std::string("<button id=\"costume-stock\"") +
             (same_id(&packages[open_port], &empty)
                  ? " class=\"selected\""
                  : "") +
             ">Default outfit</button>";
    for (size_t index = 0; index < catalog_count; ++index) {
        const NativeCostumePackage* entry = &catalog[index].metadata;
        if (!same_id(&entry->fighter_id, &fighters[open_port]) ||
            entry->character != characters[open_port] ||
            entry->base_costume != colors[open_port]) {
            continue;
        }
        markup += "<button id=\"costume-entry-" + std::to_string(index) +
                  "\"" +
                  (same_id(&packages[open_port], &catalog[index].id)
                       ? " class=\"selected\""
                       : "") +
                  ">" + escape_rml(entry->title) + "</button>";
    }
    markup += "<button id=\"costume-import\">Import DAT for P" +
              std::to_string(open_port + 1) + "</button>";
    choices->SetInnerRML(markup);
    choices->GetOwnerDocument()->GetElementById("costume-stock")
        ->AddEventListener("click", &events);
    document->GetElementById("costume-import")
        ->AddEventListener("click", &events);
    for (size_t index = 0; index < catalog_count; ++index) {
        Rml::Element* button = document->GetElementById(
            "costume-entry-" + std::to_string(index));
        if (button != nullptr) {
            button->AddEventListener("click", &events);
        }
    }
}

} // namespace

void nativeShellCostumes_Init(Rml::ElementDocument* shell)
{
    document = shell;
    section_visible = -1;
    opened = false;
    document->GetElementById("costume-close")->AddEventListener("click", &events);
    document->GetElementById("costume-panel")->AddEventListener("keydown", &events);
}

void nativeShellCostumes_Open(void)
{
    if (!document || !nativeHome_IsSelection()) return;
    selection_active = false;
    section_visible = -1;
    opened = true;
    document->GetElementById("costume-panel")->SetProperty("display","flex");
    document->GetElementById("costume-close")->Focus(true);
    nativeShellCostumes_Update();
}

void nativeShellCostumes_Close(void)
{
    opened = false;
    if (document) document->GetElementById("costume-panel")->SetProperty("display","none");
}

extern "C" bool nativeShellCostumes_IsOpen(void) { return opened; }

void nativeShellCostumes_RefreshInstalled(void)
{
    nativeCostumeCatalog_Refresh();
    selection_active = false;
    section_visible = -1;
}

void nativeShellCostumes_Update(void)
{
    if (document == nullptr || !opened) {
        return;
    }
    const bool active = nativeHome_IsSelection();
    if (!active) {
        nativeShellCostumes_Close();
        selection_active = false;
        open_port = -1;
        if (section_visible != 0) {
            section_visible = 0;
            document->GetElementById("costume-section")
                ->SetProperty("display", "none");
        }
        return;
    }
    if (!selection_active) {
        selection_active = true;
        load_catalog();
        std::memset(shown_players, 0, sizeof(shown_players));
    }
    uint8_t characters[4] = {};
    uint8_t colors[4] = {};
    NativeContentId packages[4] = {};
    NativeContentId fighters[4] = {};
    bool players[4] = {};
    bool changed = section_visible < 0;
    for (unsigned port = 0; port < 4; ++port) {
        players[port] = nativeHome_CssPlayer(port, &characters[port],
                                             &colors[port]);
        const NativeContentId* fighter = nativeHome_FighterPackage(port);
        if (fighter) fighters[port] = *fighter;
        const NativeContentId* selected = nativeHome_CostumePackage(port);
        if (selected != nullptr) {
            packages[port] = *selected;
        }
        changed |= players[port] != shown_players[port] ||
                   characters[port] != shown_characters[port] ||
                   colors[port] != shown_colors[port] ||
                   !same_id(&packages[port], &shown_packages[port]) ||
                   !same_id(&fighters[port], &shown_fighters[port]);
    }
    if (changed) {
        std::memcpy(shown_players, players, sizeof(players));
        std::memcpy(shown_characters, characters, sizeof(characters));
        std::memcpy(shown_colors, colors, sizeof(colors));
        std::memcpy(shown_packages, packages, sizeof(packages));
        std::memcpy(shown_fighters, fighters, sizeof(fighters));
        rebuild(characters, colors, packages, fighters, players);
    }
}

#endif
