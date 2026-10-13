#include "native_shell_costumes.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include <melee/ft/native_costume_import.h>
#include <melee/gm/native_content_store.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <string>

namespace {

enum { MAX_COSTUMES = 512 };

struct CostumeEntry {
    NativeContentId id;
    NativeCostumePackage metadata;
};

Rml::ElementDocument* document;
CostumeEntry catalog[MAX_COSTUMES];
size_t catalog_count;
bool selection_active;
int section_visible = -1;
int open_port = -1;
uint8_t shown_characters[4];
uint8_t shown_colors[4];
NativeContentId shown_packages[4];
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
    wchar_t path[32768] = {};
    OPENFILENAMEW picker = {};
    picker.lStructSize = sizeof(picker);
    picker.hwndOwner = GetActiveWindow();
    picker.lpstrFilter = L"Costume DAT\0*.dat\0All files\0*.*\0";
    picker.lpstrFile = path;
    picker.nMaxFile = sizeof(path) / sizeof(path[0]);
    picker.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                   OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&picker)) {
        if (CommDlgExtendedError() != 0) {
            set_status("Could not open the file picker.");
        }
        return;
    }

    const wchar_t* basename = wcsrchr(path, L'\\');
    basename = basename == nullptr ? path : basename + 1;
    const wchar_t* extension = wcsrchr(basename, L'.');
    const int title_length = int((extension == nullptr ?
                                  basename + wcslen(basename) : extension) -
                                 basename);
    NativeCostumePackage metadata = {};
    metadata.character = shown_characters[port];
    metadata.base_costume = shown_colors[port];
    const int title_bytes = title_length > 0 ? WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, basename, title_length,
        metadata.title, NATIVE_CONTENT_TITLE_BYTES - 1, nullptr, nullptr) : 0;
    if (title_bytes <= 0) {
        std::strcpy(metadata.title, "Imported costume");
    } else {
        metadata.title[title_bytes] = '\0';
    }

    FILE* file = nullptr;
    if (_wfopen_s(&file, path, L"rb") != 0 || file == nullptr) {
        set_status("Could not open that DAT.");
        return;
    }
    long length = -1;
    if (std::fseek(file, 0, SEEK_END) == 0) {
        length = std::ftell(file);
    }
    if (length < 32 || length > NATIVE_CONTENT_MAX_PACKAGE_BYTES - 76 ||
        std::fseek(file, 0, SEEK_SET) != 0) {
        std::fclose(file);
        set_status("DAT is empty or too large.");
        return;
    }
    uint8_t* dat = static_cast<uint8_t*>(std::malloc(size_t(length)));
    const bool read = dat != nullptr &&
                      std::fread(dat, 1, size_t(length), file) ==
                          size_t(length);
    std::fclose(file);
    if (!read) {
        std::free(dat);
        set_status("Could not read the DAT.");
        return;
    }

    char default_root[512];
    const char* root = content_root(default_root);
    NativeContentId id;
    NativeCostumeValidation validation;
    const bool installed = root != nullptr && nativeCostumeImport_Install(
        root, &metadata, dat, size_t(length), &id, &validation);
    std::free(dat);
    if (!installed) {
        set_status("DAT does not match this fighter and color, or installation failed.");
        return;
    }
    load_catalog();
    if (!nativeHome_SelectCostumePackage(port, &id)) {
        set_status("Installed. Return to CSS to select it.");
        return;
    }
    set_status("Costume imported and selected.");
}

void load_catalog()
{
    catalog_count = 0;
    char default_root[512];
    const char* root = content_root(default_root);
    if (root == nullptr) {
        return;
    }
    NativeContentId ids[MAX_COSTUMES];
    size_t count = 0;
    if (!nativeContentStore_ListInstalled(root, ids, MAX_COSTUMES, &count)) {
        document->GetElementById("costume-status")
            ->SetInnerRML("Costume collection is too large to show.");
    }
    for (size_t index = 0; index < count; ++index) {
        uint8_t* package = nullptr;
        size_t package_size = 0;
        NativeCostumePackage metadata;
        const uint8_t* dat;
        size_t dat_size;
        const bool valid = nativeContentStore_Read(
            root, &ids[index], &package, &package_size) &&
            nativeContentStore_ParseCostume(package, package_size, &metadata,
                                            &dat, &dat_size);
        std::free(package);
        if (!valid) {
            continue;
        }
        catalog[catalog_count++] = { ids[index], metadata };
    }
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
        if (event.GetType() != "click") {
            return;
        }
        const Rml::String& id = event.GetCurrentElement()->GetId();
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
             const NativeContentId packages[4], const bool players[4])
{
    bool any = false;
    for (int port = 0; port < 4; ++port) {
        if (!players[port]) {
            continue;
        }
        any = true;
    }
    document->GetElementById("costume-section")
        ->SetProperty("display", any ? "block" : "none");
    section_visible = int(any);
    if (!any) {
        open_port = -1;
        return;
    }

    std::string markup;
    for (unsigned port = 0; port < 4; ++port) {
        if (!players[port] ||
            characters[port] >= sizeof(character_names) /
                                    sizeof(character_names[0])) {
            continue;
        }
        const char* chosen = "Stock";
        for (size_t index = 0; index < catalog_count; ++index) {
            if (same_id(&packages[port], &catalog[index].id)) {
                chosen = catalog[index].metadata.title;
                break;
            }
        }
        markup += "<button id=\"costume-port-" + std::to_string(port) +
                  "\"" + (open_port == int(port) ? " class=\"active\"" : "") +
                  ">P" + std::to_string(port + 1) + " " +
                  character_names[characters[port]] + " · " +
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
             ">Stock costume</button>";
    for (size_t index = 0; index < catalog_count; ++index) {
        const NativeCostumePackage* entry = &catalog[index].metadata;
        if (entry->character != characters[open_port] ||
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
}

void nativeShellCostumes_RefreshInstalled(void)
{
    selection_active = false;
}

void nativeShellCostumes_Update(void)
{
    if (document == nullptr) {
        return;
    }
    const bool active = nativeHome_IsSelection();
    if (!active) {
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
    bool players[4] = {};
    bool changed = false;
    for (unsigned port = 0; port < 4; ++port) {
        players[port] = nativeHome_CssPlayer(port, &characters[port],
                                             &colors[port]);
        const NativeContentId* selected = nativeHome_CostumePackage(port);
        if (selected != nullptr) {
            packages[port] = *selected;
        }
        changed |= players[port] != shown_players[port] ||
                   characters[port] != shown_characters[port] ||
                   colors[port] != shown_colors[port] ||
                   !same_id(&packages[port], &shown_packages[port]);
    }
    if (changed) {
        std::memcpy(shown_players, players, sizeof(players));
        std::memcpy(shown_characters, characters, sizeof(characters));
        std::memcpy(shown_colors, colors, sizeof(colors));
        std::memcpy(shown_packages, packages, sizeof(packages));
        rebuild(characters, colors, packages, players);
    }
}

#endif
