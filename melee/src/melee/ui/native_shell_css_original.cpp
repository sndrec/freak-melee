#include "native_shell_css_original.h"

#ifdef MELEE_NATIVE

#include <melee/gm/native_stock_content.h>
#include <melee/gm/native_content_store.h>
#include <melee/gm/native_match_launch.h>
#include <melee/ft/native_source_asset_facts.h>
#include <melee/gm/native_home.h>
#include <melee/mn/native_css_original.h>

#include <aurora/rmlui.hpp>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

Rml::ElementDocument* document;
bool editing;
bool collection_open;
int selected = -1;
int dragging = -1;
bool resizing;
int drag_start_x, drag_start_y;
int canvas_width, canvas_height;
NativeCssTile drag_origin{};
bool catalog_loaded;
int displayed_availability = -1;

struct InstalledPortrait {
    NativeContentId id;
    std::string title;
    std::vector<std::byte> pixels;
    unsigned width;
    unsigned height;
};
std::vector<InstalledPortrait> portraits;

std::string escape_markup(std::string_view value)
{
    std::string escaped;
    for (char ch : value) {
        switch (ch) {
        case '&': escaped += "&amp;"; break;
        case '<': escaped += "&lt;"; break;
        case '>': escaped += "&gt;"; break;
        case '"': escaped += "&quot;"; break;
        default: escaped += ch; break;
        }
    }
    return escaped;
}

bool decode_portrait(const uint8_t* png, size_t size,
                     InstalledPortrait& portrait)
{
    if (size > UINT32_MAX) {
        return false;
    }
    using Microsoft::WRL::ComPtr;
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapDecoder> decoder;
    ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr,
                                CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&factory))) ||
        FAILED(factory->CreateStream(&stream)) ||
        FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(png),
                                            UINT(size))) ||
        FAILED(factory->CreateDecoderFromStream(stream.Get(), nullptr,
                                                WICDecodeMetadataCacheOnLoad,
                                                &decoder)) ||
        FAILED(decoder->GetFrame(0, &frame)) ||
        FAILED(frame->GetSize(&portrait.width, &portrait.height)) ||
        portrait.width < 16 || portrait.width > 512 ||
        portrait.height < 16 || portrait.height > 512 ||
        FAILED(factory->CreateFormatConverter(&converter)) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA,
                                     WICBitmapDitherTypeNone, nullptr, 0,
                                     WICBitmapPaletteTypeCustom))) {
        return false;
    }
    portrait.pixels.resize(size_t(portrait.width) * portrait.height * 4);
    return SUCCEEDED(converter->CopyPixels(
        nullptr, portrait.width * 4, UINT(portrait.pixels.size()),
        reinterpret_cast<BYTE*>(portrait.pixels.data())));
}

const InstalledPortrait* find_portrait(const NativeContentId& id)
{
    for (const InstalledPortrait& portrait : portraits) {
        if (std::memcmp(&portrait.id, &id, sizeof(id)) == 0) {
            return &portrait;
        }
    }
    return nullptr;
}

void load_portraits()
{
    portraits.clear();
    char default_root[512];
    const char* root = nativeMatchLaunch_ContentRoot();
    if (root == nullptr) {
        if (!nativeContentStore_DefaultRoot(default_root,
                                            sizeof(default_root))) {
            return;
        }
        root = default_root;
    }
    NativeContentId ids[NATIVE_CSS_LAYOUT_MAX_TILES];
    size_t count = 0;
    if (!nativeContentStore_ListInstalled(root, ids,
                                          NATIVE_CSS_LAYOUT_MAX_TILES,
                                          &count)) {
        return;
    }
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) {
        return;
    }
    for (size_t index = 0; index < count; ++index) {
        uint8_t* package = nullptr;
        size_t package_size = 0;
        NativeFighterPackage metadata;
        NativeFighterPackageData data;
        bool valid = nativeContentStore_Read(
            root, &ids[index], &package, &package_size) &&
            nativeContentStore_ParseFighter(package, package_size,
                                            &metadata, &data);
        valid = valid && metadata.mode == NativeFighterFamily_Single && metadata.member_count == 1;
        if (valid) {
            NativeSourceAssetFacts facts;NativeScriptDiagnostic diagnostic = {};
            const NativeFighterMemberData& member = data.members[0];
            valid = member.script_text != nullptr && member.script_size != 0 &&
                nativeSourceAssets_Read(member.asset_record, member.asset_record_size, &facts, &diagnostic);
        }
        if (valid) {
            InstalledPortrait portrait{};
            portrait.id = ids[index];
            portrait.title = metadata.title;
            if (decode_portrait(data.portrait_png, data.portrait_size,
                                portrait)) {
                portraits.push_back(std::move(portrait));
            }
        }
        std::free(package);
    }
    if (SUCCEEDED(initialized)) {
        CoUninitialize();
    }
}

const char* stock_names[] = {
    "Dr. Mario", "Mario", "Luigi", "Bowser", "Peach", "Yoshi",
    "Donkey Kong", "Captain Falcon", "Ganondorf", "Falco", "Fox",
    "Ness", "Ice Climbers", "Kirby", "Samus", "Zelda", "Link",
    "Young Link", "Pichu", "Pikachu", "Jigglypuff", "Mewtwo",
    "Mr. Game &amp; Watch", "Marth", "Roy",
};

void set_status(const char* message)
{
    document->GetElementById("css-original-status")->SetInnerRML(message);
}

void portrait_geometry(unsigned index, const NativeCssTile& tile)
{
    Rml::Element* image = document->GetElementById(
        "css-original-portrait-" + std::to_string(index));
    if (image == nullptr) {
        return;
    }
    image->SetProperty(Rml::PropertyId::Left,
                       Rml::Property(tile.x * canvas_width / 1000.0f,
                                     Rml::Unit::PX));
    image->SetProperty(Rml::PropertyId::Top,
                       Rml::Property(tile.y * canvas_height / 1000.0f,
                                     Rml::Unit::PX));
    image->SetProperty(Rml::PropertyId::Width,
                       Rml::Property(tile.width * canvas_width / 1000.0f,
                                     Rml::Unit::PX));
    image->SetProperty(Rml::PropertyId::Height,
                       Rml::Property(tile.height * canvas_height / 1000.0f,
                                     Rml::Unit::PX));
}

void build_portraits()
{
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    std::string markup;
    if (layout != nullptr) {
        for (unsigned index = 0; index < layout->count; ++index) {
            const NativeCssTile& tile = layout->tiles[index];
            if (find_portrait(tile.fighter_package_id) == nullptr) {
                continue;
            }
            char hex[65];
            nativeContentStore_IdHex(&tile.fighter_package_id, hex);
            markup += "<img id=\"css-original-portrait-" +
                      std::to_string(index) + "\" class=\"css-original-portrait\" "
                      "src=\"fighter://" + hex + "\" />";
        }
    }
    document->GetElementById("css-original-portraits")->SetInnerRML(markup);
    if (layout != nullptr) {
        for (unsigned index = 0; index < layout->count; ++index) {
            portrait_geometry(index, layout->tiles[index]);
        }
    }
}

void handle_geometry(unsigned index, const NativeCssTile& tile)
{
    Rml::Element* handle = document->GetElementById(
        "css-original-handle-" + std::to_string(index));
    if (handle == nullptr) {
        return;
    }
    handle->SetProperty(Rml::PropertyId::Left,
                        Rml::Property(tile.x * canvas_width / 1000.0f,
                                      Rml::Unit::PX));
    handle->SetProperty(Rml::PropertyId::Top,
                        Rml::Property(tile.y * canvas_height / 1000.0f,
                                      Rml::Unit::PX));
    handle->SetProperty(Rml::PropertyId::Width,
                        Rml::Property(tile.width * canvas_width / 1000.0f,
                                      Rml::Unit::PX));
    handle->SetProperty(Rml::PropertyId::Height,
                        Rml::Property(tile.height * canvas_height / 1000.0f,
                                      Rml::Unit::PX));
    Rml::Element* resize = document->GetElementById(
        "css-original-resize-" + std::to_string(index));
    if (resize != nullptr) {
        resize->SetProperty(Rml::PropertyId::Left,
                            Rml::Property((tile.x + tile.width) *
                                              canvas_width / 1000.0f - 7.0f,
                                          Rml::Unit::PX));
        resize->SetProperty(Rml::PropertyId::Top,
                            Rml::Property((tile.y + tile.height) *
                                              canvas_height / 1000.0f - 7.0f,
                                          Rml::Unit::PX));
    }
}

void update_selected(void)
{
    document->GetElementById("css-original-remove")
        ->SetProperty("display", editing && selected >= 0 ? "block" : "none");
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    if (layout != nullptr) {
        for (unsigned index = 0; index < layout->count; ++index) {
            Rml::Element* handle = document->GetElementById(
                "css-original-handle-" + std::to_string(index));
            if (handle != nullptr) {
                handle->SetClass("selected", int(index) == selected);
            }
        }
    }
}

bool parse_index(const Rml::String& id, const char* prefix, unsigned* index)
{
    const size_t size = std::char_traits<char>::length(prefix);
    return id.compare(0, size, prefix) == 0 &&
           std::sscanf(id.c_str() + size, "%u", index) == 1;
}

void build_handles(void);
void build_stock(void);

void drag_handle(Rml::Event& event, unsigned index, bool resize)
{
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    if (!editing || layout == nullptr || index >= layout->count ||
        canvas_width <= 0 || canvas_height <= 0) {
        return;
    }
    const Rml::String& type = event.GetType();
    if (type == "dragstart") {
        dragging = int(index);
        resizing = resize;
        drag_start_x = event.GetParameter<int>("mouse_x", 0);
        drag_start_y = event.GetParameter<int>("mouse_y", 0);
        drag_origin = layout->tiles[index];
        selected = int(index);
        update_selected();
        return;
    }
    if (dragging != int(index) || resizing != resize) {
        return;
    }
    const int dx = (event.GetParameter<int>("mouse_x", drag_start_x) -
                    drag_start_x) * 1000 / canvas_width;
    const int dy = (event.GetParameter<int>("mouse_y", drag_start_y) -
                    drag_start_y) * 1000 / canvas_height;
    NativeCssTile tile = drag_origin;
    if (resize) {
        tile.width = uint16_t(std::clamp(
            int(tile.width) + dx, int(NATIVE_CSS_LAYOUT_MIN_TILE_SIZE),
            int(1000 - tile.x)));
        tile.height = uint16_t(std::clamp(
            int(tile.height) + dy, int(NATIVE_CSS_LAYOUT_MIN_TILE_SIZE),
            int(1000 - tile.y)));
    } else {
        tile.x = uint16_t(std::clamp(int(tile.x) + dx, 0,
                                     int(1000 - tile.width)));
        tile.y = uint16_t(std::clamp(int(tile.y) + dy, 0,
                                     int(1000 - tile.height)));
    }
    if (nativeCssOriginal_PreviewTile(index, &tile)) {
        handle_geometry(index, tile);
        portrait_geometry(index, tile);
    }
    if (type == "dragend") {
        if (!nativeCssOriginal_UpdateTile(index, &tile)) {
            nativeCssOriginal_CancelPreview();
            handle_geometry(index, drag_origin);
            portrait_geometry(index, drag_origin);
            set_status("Couldn't save the layout.");
        }
        dragging = -1;
    }
}

class CssOriginalEvents final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        const Rml::String& id = event.GetCurrentElement()->GetId();
        unsigned index = 0;
        const bool corner = parse_index(id, "css-original-resize-", &index);
        const bool handle = !corner &&
            parse_index(id, "css-original-handle-", &index);
        const Rml::String& type = event.GetType();
        if (type == "dragstart" || type == "drag" || type == "dragend") {
            if (corner || handle) {
                drag_handle(event, index, corner);
                if (corner) {
                    event.StopPropagation();
                }
            }
            return;
        }
        if (type != "click") {
            return;
        }
        if (handle) {
            selected = int(index);
            update_selected();
        } else if (corner) {
            event.StopPropagation();
        } else if (id == "css-original-back") {
            nativeShellCssOriginal_Close();
        } else if (id == "css-original-edit") {
            if (!editing && !nativeCssOriginal_CanEdit()) {
                set_status("Layout unavailable; repair its saved file first.");
                return;
            }
            editing = !editing;
            collection_open = false;
            selected = -1;
            document->GetElementById("css-original-edit")
                ->SetClass("active", editing);
            document->GetElementById("css-original-tools")
                ->SetProperty("display", editing ? "block" : "none");
            document->GetElementById("css-original-overlay")
                ->SetProperty("display", editing ? "block" : "none");
            if (editing) {
                load_portraits();
                build_portraits();
                build_handles();
            }
            update_selected();
        } else if (id == "css-original-remove") {
            if (selected >= 0 &&
                nativeCssOriginal_RemoveTile(unsigned(selected))) {
                selected = -1;
                build_handles();
                build_stock();
                build_portraits();
                set_status("");
            }
        } else if (id == "css-original-collection") {
            collection_open = !collection_open;
            document->GetElementById("css-original-stock")
                ->SetProperty("display", collection_open ? "block" : "none");
            build_stock();
        } else if (parse_index(id, "css-original-stock-", &index)) {
            if (nativeCssOriginal_AddStock(index)) {
                collection_open = false;
                document->GetElementById("css-original-stock")
                    ->SetProperty("display", "none");
                build_handles();
                build_portraits();
            }
        } else if (parse_index(id, "css-original-fighter-", &index) &&
                   index < portraits.size()) {
            if (nativeCssOriginal_AddInstalled(&portraits[index].id)) {
                collection_open = false;
                document->GetElementById("css-original-stock")
                    ->SetProperty("display", "none");
                build_handles();
                build_portraits();
                set_status("Portrait placed.");
            }
        } else if (id == "css-original-reset") {
            if (nativeCssOriginal_Reset()) {
                selected = -1;
                build_handles();
                build_stock();
                build_portraits();
                set_status("");
            }
        }
    }
} events;

void build_handles(void)
{
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    if (layout == nullptr) {
        return;
    }
    std::string markup;
    for (unsigned index = 0; index < layout->count; ++index) {
        uint8_t character;
        if (!nativeStockContent_CharacterFromId(
                layout->tiles[index].stock_character_id, &character) &&
            find_portrait(layout->tiles[index].fighter_package_id) == nullptr) {
            continue;
        }
        markup += "<div id=\"css-original-handle-" + std::to_string(index) +
                  "\" class=\"css-original-handle\"></div>";
        markup += "<div id=\"css-original-resize-" + std::to_string(index) +
                  "\" class=\"css-original-resize\"></div>";
    }
    document->GetElementById("css-original-handles")->SetInnerRML(markup);
    for (unsigned index = 0; index < layout->count; ++index) {
        for (const char* prefix : { "css-original-handle-",
                                    "css-original-resize-" }) {
            Rml::Element* element = document->GetElementById(
                std::string(prefix) + std::to_string(index));
            if (element == nullptr) {
                continue;
            }
            for (const char* type : { "click", "dragstart", "drag", "dragend" }) {
                element->AddEventListener(type, &events);
            }
        }
        handle_geometry(index, layout->tiles[index]);
    }
    update_selected();
}

void build_stock(void)
{
    if (!editing || !collection_open) {
        return;
    }
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    if (layout == nullptr) {
        return;
    }
    NativeCssLayout stock;
    nativeCssLayout_Default(&stock);
    std::string markup;
    for (unsigned icon = 0; icon < stock.count; ++icon) {
        bool present = false;
        for (unsigned tile = 0; tile < layout->count; ++tile) {
            present |= layout->tiles[tile].stock_character_id ==
                       stock.tiles[icon].stock_character_id;
        }
        if (!present) {
            markup += "<button id=\"css-original-stock-" +
                      std::to_string(icon) + "\"><span>" +
                      stock_names[icon] + "</span></button>";
        }
    }
    for (unsigned fighter = 0; fighter < portraits.size(); ++fighter) {
        bool present = false;
        for (unsigned tile = 0; tile < layout->count; ++tile) {
            present |= std::memcmp(&layout->tiles[tile].fighter_package_id,
                                   &portraits[fighter].id,
                                   sizeof(NativeContentId)) == 0;
        }
        if (!present) {
            markup += "<button id=\"css-original-fighter-" +
                      std::to_string(fighter) + "\"><span>" +
                      escape_markup(portraits[fighter].title) +
                      "</span></button>";
        }
    }
    if (markup.empty()) {
        markup = "<p>All installed fighters are on the screen.</p>";
    }
    document->GetElementById("css-original-stock")->SetInnerRML(markup);
    for (unsigned icon = 0; icon < stock.count; ++icon) {
        Rml::Element* button = document->GetElementById(
            "css-original-stock-" + std::to_string(icon));
        if (button != nullptr) {
            button->AddEventListener("click", &events);
        }
    }
    for (unsigned fighter = 0; fighter < portraits.size(); ++fighter) {
        Rml::Element* button = document->GetElementById(
            "css-original-fighter-" + std::to_string(fighter));
        if (button != nullptr) {
            button->AddEventListener("click", &events);
        }
    }
}

} // namespace

void nativeShellCssOriginal_Init(Rml::ElementDocument* shell)
{
    document = shell;
    portraits.clear();
    catalog_loaded = false;
    displayed_availability = -1;
    aurora::rmlui::register_texture_provider(
        "fighter", [](std::string_view source)
            -> std::optional<aurora::rmlui::RuntimeTexture> {
            constexpr std::string_view prefix = "fighter://";
            if (!source.starts_with(prefix)) {
                return std::nullopt;
            }
            NativeContentId id;
            const std::string hex(source.substr(prefix.size()));
            if (!nativeContentStore_IdFromHex(hex.c_str(), &id)) {
                return std::nullopt;
            }
            const InstalledPortrait* portrait = find_portrait(id);
            if (portrait == nullptr) {
                return std::nullopt;
            }
            return aurora::rmlui::RuntimeTexture{
                portrait->width, portrait->height, portrait->pixels,
                false, false,
            };
        });
    for (const char* id : { "css-original-edit", "css-original-back",
                            "css-original-remove",
                            "css-original-collection", "css-original-reset" }) {
        document->GetElementById(id)->AddEventListener("click", &events);
    }
}

void nativeShellCssOriginal_Update(void)
{
    if (document == nullptr) {
        return;
    }
    const bool available = nativeHome_IsSelection() &&
                           nativeCssOriginal_Layout() != nullptr;
    if (available && !catalog_loaded) {
        load_portraits();
        catalog_loaded = true;
        build_portraits();
    } else if (!available && catalog_loaded) {
        document->GetElementById("css-original-portraits")->SetInnerRML("");
        portraits.clear();
        catalog_loaded = false;
    }
    if (available) {
        const NativeCssLayout* layout = nativeCssOriginal_Layout();
        for (unsigned index = 0; index < layout->count; ++index) {
            Rml::Element* portrait = document->GetElementById(
                "css-original-portrait-" + std::to_string(index));
            if (portrait == nullptr) {
                continue;
            }
            for (unsigned port = 0; port < 4; ++port) {
                const NativeContentId* selected =
                    nativeHome_FighterPackage(port);
                portrait->SetClass(
                    "selected-p" + std::to_string(port + 1),
                    selected != nullptr &&
                        std::memcmp(selected,
                                    &layout->tiles[index].fighter_package_id,
                                    sizeof(*selected)) == 0);
            }
        }
    }
    // RmlUI property assignment dirties layout even when the value is unchanged.
    // Apply visibility on selection/match transitions, not on every game frame.
    if (displayed_availability != int(available)) {
        displayed_availability = int(available);
        document->GetElementById("css-original-portraits")
            ->SetProperty("display", available ? "block" : "none");
        document->GetElementById("css-original-edit")
            ->SetProperty("display", available ? "block" : "none");
    }
    if (!available && editing) {
        nativeShellCssOriginal_Close();
    }
}

void nativeShellCssOriginal_RefreshInstalledFighters(void)
{
    if (document == nullptr) return;
    document->GetElementById("css-original-portraits")->SetInnerRML("");
    portraits.clear();
    catalog_loaded = false;
}

void nativeShellCssOriginal_Resize(int left, int top, int width, int height)
{
    canvas_width = width;
    canvas_height = height;
    if (document == nullptr) {
        return;
    }
    Rml::Element* panel = document->GetElementById("css-original-overlay");
    Rml::Element* portrait_panel = document->GetElementById(
        "css-original-portraits");
    for (Rml::Element* element : { panel, portrait_panel }) {
        element->SetProperty(Rml::PropertyId::Left,
                             Rml::Property(float(left), Rml::Unit::PX));
        element->SetProperty(Rml::PropertyId::Top,
                             Rml::Property(float(top), Rml::Unit::PX));
        element->SetProperty(Rml::PropertyId::Width,
                             Rml::Property(float(width), Rml::Unit::PX));
        element->SetProperty(Rml::PropertyId::Height,
                             Rml::Property(float(height), Rml::Unit::PX));
    }
    const NativeCssLayout* layout = nativeCssOriginal_Layout();
    if (layout != nullptr) {
        for (unsigned index = 0; index < layout->count; ++index) {
            portrait_geometry(index, layout->tiles[index]);
        }
    }
    if (editing) {
        if (layout != nullptr) {
            for (unsigned index = 0; index < layout->count; ++index) {
                handle_geometry(index, layout->tiles[index]);
            }
        }
    }
}

void nativeShellCssOriginal_Close(void)
{
    editing = false;
    collection_open = false;
    selected = -1;
    dragging = -1;
    nativeCssOriginal_CancelPreview();
    if (document != nullptr) {
        document->GetElementById("css-original-edit")
            ->SetClass("active", false);
        document->GetElementById("css-original-tools")
            ->SetProperty("display", "none");
        document->GetElementById("css-original-overlay")
            ->SetProperty("display", "none");
        document->GetElementById("css-original-stock")
            ->SetProperty("display", "none");
    }
}

bool nativeShellCssOriginal_IsEditing(void)
{
    return editing;
}

#endif
