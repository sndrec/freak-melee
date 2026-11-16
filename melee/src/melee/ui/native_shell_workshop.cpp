#include <melee/ui/native_hub.h>
#include "native_shell_workshop_docs.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop_model.h"
#include "native_shell_workshop_parameters.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>

#include "native_shell.h"
#include "native_hub_packages.h"
#include "native_shell_css_original.h"
#include "native_hub_portrait.h"
#include "native_hub_catalog.h"

#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_family_project.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_gx_image.h>
#include <melee/gm/native_content_store.h>
#include <melee/gm/native_home.h>
#include <melee/gm/native_match_launch.h>
#include <melee/mn/native_css_original.h>

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>

#include <algorithm>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

enum { MAX_DRAFTS = 128, PATH_CAPACITY = 512 };

Rml::ElementDocument* document;
NativeFighterDraft drafts[MAX_DRAFTS];
size_t draft_count;
int selected = -1;
bool opened;
bool testing;
bool preview_loading;
NativeWorkshopOrigin return_destination;
NativeFighterFamilyProject active_family;
NativeFighterProject* active_project;
unsigned active_member;
bool updating_member_selector;
bool member_opened[NATIVE_FIGHTER_FAMILY_MEMBERS];
std::string attribute_text[NATIVE_FIGHTER_FAMILY_MEMBERS][NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT];
bool project_loaded;
bool attributes_dirty[NATIVE_FIGHTER_FAMILY_MEMBERS];
bool discard_pending;
bool package_open;
bool fighter_menu_open;
bool build_cached_candidate(bool save_project, bool persist_portrait,
                            bool require_built_script, NativeContentId* result);
void enter_draft_preview();

void status(const char* id, const char* message);
bool build_script(bool show_script);

void show_fighter_menu(bool show)
{
    fighter_menu_open = show;
    document->GetElementById("workshop-fighter-menu")->SetProperty("display", show ? "block" : "none");
    document->GetElementById("workshop-fighter-toggle")->SetClass("selected", show);
}

void close_project()
{
    testing = false;
    preview_loading = false;
    while (nativeWorkshopDocs_Close()) {}
    document->GetElementById("workshop-panel")->SetClass("testing", false);
    nativeShellWorkshopEditor_Close();
    if (project_loaded) nativeFighterFamilyProject_Close(&active_family);
    active_project = nullptr;
    active_member = 0;
    std::fill_n(member_opened, NATIVE_FIGHTER_FAMILY_MEMBERS, false);
    project_loaded = false;
    selected = -1;
    std::fill_n(attributes_dirty, NATIVE_FIGHTER_FAMILY_MEMBERS, false);
    discard_pending = false;
    package_open = false;
    show_fighter_menu(false);
}

bool confirm_discard()
{
    bool dirty = false;
    for (unsigned i = 0; project_loaded && i < active_family.member_count; ++i)
        dirty |= attributes_dirty[i] || nativeShellWorkshopEditor_HasUnsavedEdits(i);
    if (!dirty) return true;
    if (discard_pending) return true;
    discard_pending = true;
    status("workshop-editor-status", "Unsaved edits. Press Back again "
           "to discard them, or Save draft.");
    status("workshop-script-status", "Unsaved edits. Press Back again "
           "to discard them, or Save draft.");
    return false;
}

Rml::ElementFormControlInput* input(const char* id)
{
    return static_cast<Rml::ElementFormControlInput*>(
        document->GetElementById(id));
}

void status(const char* id, const char* message)
{
    document->GetElementById(id)->SetInnerRML(
        Rml::StringUtilities::EncodeRml(message));
}

void refresh_drafts()
{
    close_project();
    document->GetElementById("workshop-panel")->SetClass("library", true);
    draft_count = 0;
    document->GetElementById("workshop-editor-header")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-editor-context")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-library")
        ->SetProperty("display", "block");
    document->GetElementById("workshop-inspector")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-inspector")
        ->SetClass("workshop-package-open", false);
    document->GetElementById("workshop-package-toggle")
        ->SetClass("selected", false);
    for (const char* id : { "workshop-editor-browser",
                            "workshop-editor-inspector",
                            "workshop-editor-timeline",
                            "workshop-action-viewport",
                            "workshop-animation-viewport",
                            "workshop-script-inspector",
                            "workshop-script-pane",
                            "workshop-animation-pane",
                            "workshop-model-pane" })
        document->GetElementById(id)->SetProperty("display", "none");
    document->GetElementById("workshop-attributes")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-empty")
        ->SetProperty("display", "block");
    document->GetElementById("workshop-title")
        ->SetInnerRML("Select a draft");
    if (!nativeFighterFamilyProject_ListLocal(drafts, MAX_DRAFTS, &draft_count)) {
        status("workshop-library-status", "Could not read the local fighter drafts.");
        return;
    }
    std::string rows;
    for (size_t i = 0; i < draft_count; ++i) {
        rows += "<button id='workshop-draft-" + std::to_string(i) +
                "' class='workshop-draft'>" +
                Rml::StringUtilities::EncodeRml(drafts[i].title) + "</button>";
    }
    document->GetElementById("workshop-drafts")
        ->SetInnerRML(rows.empty() ?
            "<p class='workshop-muted'>No drafts yet.</p>" : rows);
    status("workshop-library-status", draft_count == MAX_DRAFTS
        ? "Showing the first 128 drafts." : "");
}

std::string attribute_id(size_t index) { return "workshop-attribute-" + std::to_string(index); }

void create_attribute_controls()
{
    std::string markup, group;
    for (size_t i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i) {
        const NativeFighterAttribute& field = nativeFighterCommonAttributes[i];
        if (group != field.group) {
            if (!group.empty()) markup += "</div>";
            group = field.group;
            markup += "<div class='workshop-attribute-group'><div class='eyebrow'>" + group + "</div>";
        }
        const std::string id = attribute_id(i);
        markup += "<label class='field-label' for='" + id + "' title='" +
            Rml::StringUtilities::EncodeRml(field.description) + "'>" +
            Rml::StringUtilities::EncodeRml(field.label) + "</label><input class='room-input' type='text' maxlength='32' id='" +
            id + "' title='" + field.name + "' />";
    }
    markup += "</div>";
    document->GetElementById("workshop-common-attributes")->SetInnerRML(markup);
}

bool family_script_changed()
{
    for (unsigned i = 0; project_loaded && i < active_family.member_count; ++i)
        if (member_opened[i] && nativeShellWorkshopEditor_ScriptChanged(i)) return true;
    return false;
}

bool family_has_unsaved_edits()
{
    for (unsigned i = 0; project_loaded && i < active_family.member_count; ++i)
        if (attributes_dirty[i] || nativeShellWorkshopEditor_HasUnsavedEdits(i)) return true;
    return false;
}

bool select_member(unsigned member, bool restart_preview = false)
{
    if (!project_loaded || member >= active_family.member_count) return false;
    // Editor restoration can immediately apply a pose. Select its actual actor
    // before restoring the incoming member's action/animation controls.
    const bool member_available = nativeHome_WorkshopSelectMember(member, restart_preview);
    if (member_opened[member]) {
        if (!nativeShellWorkshopEditor_Select(member)) return false;
    } else {
        nativeShellWorkshopEditor_Open(member, &active_family.members[member],
                                       active_family.paths[member]);
        member_opened[member] = true;
    }
    active_member = member;
    active_project = &active_family.members[member];
    for (int i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i)
        input(attribute_id(i).c_str())->SetValue(attribute_text[member][i]);
    auto* member_menu = static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("workshop-family-member"));
    const std::string member_value = std::to_string(member);
    if (member_menu->GetValue() != member_value) {
        updating_member_selector = true;
        member_menu->SetValue(member_value);
        updating_member_selector = false;
    }
    if (restart_preview && nativeHome_IsPractice()) {
        preview_loading = member_available && nativeHome_PracticeTransitioning();
        status("workshop-editor-status", !member_available
            ? "This family member is not available in the current preview. Its data remains editable."
            : preview_loading
            ? "Loading this member's preview..." : "Draft preview ready.");
    }
    return true;
}

void select_draft(size_t index)
{
    if (index >= draft_count) {
        return;
    }
    NativeFighterFamilyProject family;
    if (!nativeFighterFamilyProject_Open(drafts[index].path, &family)) {
        status("workshop-library-status", "Could not open that complete fighter draft.");
        return;
    }
    std::string values[NATIVE_FIGHTER_FAMILY_MEMBERS][NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT];
    bool valid = true;
    for (unsigned member = 0; member < family.member_count; ++member) {
        for (int i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i) {
            double value;
            if (!nativeFighterProject_GetAttribute(&family.members[member],
                    size_t(i), &value)) {
                valid = false;
                break;
            }
            char text[32];
            if (nativeFighterCommonAttributes[i].type == NativeAttribute_Float) {
                const auto formatted = std::to_chars(text, text + sizeof(text) - 1, float(value));
                *formatted.ptr = 0;
            } else std::snprintf(text, sizeof(text), "%.0f", value);
            values[member][i] = text;
        }
    }
    if (!valid) {
        nativeFighterFamilyProject_Close(&family);
        status("workshop-library-status", "Could not read that draft's member attributes.");
        return;
    }
    close_project();
    document->GetElementById("workshop-panel")->SetClass("library", false);
    active_family = family;
    active_member = 0;
    active_project = &active_family.members[0];
    project_loaded = true;
    for (unsigned member = 0; member < family.member_count; ++member)
        for (int i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i)
            attribute_text[member][i] = std::move(values[member][i]);
    for (int i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i)
        input(attribute_id(i).c_str())->SetValue(attribute_text[0][i]);
    auto* member_menu = static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("workshop-family-member"));
    std::string member_options;
    for (unsigned i = 0; i < family.member_count; ++i) {
        const char* name = nativeFighterProject_StockBaseName(family.members[i].base_kind);
        member_options += "<option value='" + std::to_string(i) + "'>" +
            Rml::StringUtilities::EncodeRml(name != nullptr ? name : family.keys[i]) +
            "</option>";
    }
    updating_member_selector = true;
    member_menu->RemoveAll();
    member_menu->SetInnerRML(member_options);
    member_menu->SetValue("0");
    updating_member_selector = false;
    member_menu->SetProperty("display", family.member_count > 1 ? "block" : "none");
    selected = (int) index;
    document->GetElementById("workshop-title")->SetInnerRML(
        Rml::StringUtilities::EncodeRml(drafts[index].title));
    document->GetElementById("workshop-empty")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-attributes")
        ->SetProperty("display", "block");
    for (size_t i = 0; i < draft_count; ++i) {
        document->GetElementById("workshop-draft-" + std::to_string(i))
            ->SetClass("selected", i == index);
    }
    status("workshop-editor-status", "");
    input("workshop-portrait-path")->SetValue("");
    status("workshop-library-status", "");
    document->GetElementById("workshop-editor-title")->SetInnerRML(
        Rml::StringUtilities::EncodeRml(drafts[index].title));
    document->GetElementById("workshop-library")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-inspector")
        ->SetProperty("display", "none");
    document->GetElementById("workshop-editor-header")
        ->SetProperty("display", "flex");
    document->GetElementById("workshop-editor-context")
        ->SetProperty("display", "flex");
    document->GetElementById("workshop-editor-browser")
        ->SetProperty("display", "block");
    document->GetElementById("workshop-editor-inspector")
        ->SetProperty("display", "block");
    document->GetElementById("workshop-editor-timeline")
        ->SetProperty("display", "block");
    document->GetElementById("workshop-mode-action")
        ->SetClass("selected", true);
    nativeShellWorkshopEditor_Open(0, active_project, active_family.paths[0]);
    member_opened[0] = true;
    nativeHome_WorkshopSelectMember(0, false);
    enter_draft_preview();
}

void create_stock_clone()
{
    const std::string title = input("workshop-new-name")->GetValue();
    if (title.empty() || title.find_first_not_of(" \t\r\n") ==
                             std::string::npos) {
        status("workshop-library-status", "Give the new fighter a name.");
        return;
    }
    const std::string base_name = static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("workshop-new-base"))->GetValue();
    FighterKind base_kind = Ft_Kind_None;
    for (int index = Ft_Kind_Mario; index < Ft_Kind_StockCount; ++index) {
        const FighterKind kind = static_cast<FighterKind>(index);
        const char* name = nativeFighterProject_StockBaseName(kind);
        if (name != nullptr && base_name == name) {
            base_kind = kind;
            break;
        }
    }
    if (base_kind == Ft_Kind_None ||
        nativeFighterRuntime_FindStockDefinition(base_kind) == nullptr) {
        status("workshop-library-status", "Choose a supported fighter to clone.");
        return;
    }
    char path[PATH_CAPACITY];
    bool created=nativeHubCatalog_CreateStockDraft(base_kind,title.c_str(),path);
    if (!created) {
        status("workshop-library-status", "Could not clone the fighter from the game disc.");
        return;
    }
    refresh_drafts();
    for (size_t i = 0; i < draft_count; ++i) {
        if (std::strcmp(drafts[i].path, path) == 0) {
            select_draft(i);
            break;
        }
    }
    input("workshop-new-name")->SetValue("");
    status("workshop-library-status", "Fighter clone saved as a separate draft.");
}

bool save_draft()
{
    if (!project_loaded || selected < 0 ||
        size_t(selected) >= draft_count) {
        return false;
    }
    const unsigned previous_member = active_member;
    bool saved = true;
    for (unsigned member = 0; member < active_family.member_count; ++member) {
        if (!select_member(member)) { saved = false; break; }
        const char* path = active_family.paths[member];
        char asset_error[256] = {};
        if (!nativeFighterProject_SaveAssets(path, active_project,
                                              asset_error, sizeof(asset_error)) ||
            !nativeShellWorkshopParameters_Save(document, active_project, path) ||
            !nativeShellWorkshopEditor_SaveScriptDraft()) {
            if (*asset_error) std::fprintf(stderr, "[workshop-assets] %s\n", asset_error);
            saved = false;
            break;
        }
        nativeShellWorkshopEditor_MarkAssetsSaved();
        nativeShellWorkshopModel_MarkSaved(member);
        attributes_dirty[member] = false;
    }
    if (saved) {
        select_member(previous_member);
        discard_pending = false;
        status("workshop-editor-status", family_script_changed()
            ? "Draft saved. Build behavior before preview." : "Draft saved.");
    } else {
        status("workshop-editor-status", "Could not save this member. Its edits are retained.");
    }
    return saved;
}

void browse_portrait()
{
    char path[PATH_CAPACITY] = {};
    OPENFILENAMEA picker = {};
    picker.lStructSize = sizeof(picker);
    picker.hwndOwner = GetActiveWindow();
    picker.lpstrFilter = "PNG images (*.png)\0*.png\0All files (*.*)\0*.*\0";
    picker.lpstrFile = path;
    picker.nMaxFile = sizeof(path);
    picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
                   OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&picker)) {
        input("workshop-portrait-path")->SetValue(path);
    }
}

void import_fighter()
{
    if (!project_loaded) return;
    char file[PATH_CAPACITY] = {};
    OPENFILENAMEA picker = {};
    picker.lStructSize = sizeof(picker);
    picker.hwndOwner = GetActiveWindow();
    picker.lpstrTitle = "Import fighter as a new draft";
    picker.lpstrFilter = "Freak Melee fighter packages (*.fmc)\0*.fmc\0All files (*.*)\0*.*\0";
    picker.lpstrFile = file;
    picker.nMaxFile = sizeof(file);
    picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&picker)) nativeShellWorkshop_ImportFighter(file);
}

void browse_hud_image(unsigned target)
{
    char file[PATH_CAPACITY] = {};
    OPENFILENAMEA picker = {};
    picker.lStructSize = sizeof(picker);
    picker.hwndOwner = GetActiveWindow();
    picker.lpstrTitle = target ? "Import HUD emblem" : "Import stock icon";
    picker.lpstrFilter = "PNG image (*.png)\0*.png\0";
    picker.lpstrFile = file;
    picker.nMaxFile = sizeof(file);
    picker.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&picker)) nativeShellWorkshop_ImportHudImage(target, file);
}

bool read_portrait(const char* path, uint8_t** bytes, size_t* size)
{
    FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        return false;
    }
    long length = 0;
    bool valid = std::fseek(file, 0, SEEK_END) == 0 &&
                 (length = std::ftell(file)) >= 45 &&
                 length <= 1024 * 1024 &&
                 std::fseek(file, 0, SEEK_SET) == 0;
    uint8_t* data = valid ? static_cast<uint8_t*>(std::malloc(size_t(length)))
                          : nullptr;
    bool loaded = data != nullptr &&
                  std::fread(data, 1, size_t(length), file) == size_t(length);
    loaded = std::fclose(file) == 0 && loaded;
    if (!loaded) {
        std::free(data);
        return false;
    }
    *bytes = data;
    *size = size_t(length);
    return true;
}

bool build_cached_candidate(bool save_project, bool persist_portrait,
                            bool require_built_script, NativeContentId* result)
{
    if (selected < 0 || size_t(selected) >= draft_count) {
        return false;
    }
    if (require_built_script && family_script_changed()) {
        status("workshop-editor-status", "Build the changed behavior script "
               "before preview or install.");
        return false;
    }
    const std::string chosen = input("workshop-portrait-path")->GetValue();
    const char* path = drafts[selected].path;
    char saved_portrait[PATH_CAPACITY];
    int length = std::snprintf(saved_portrait, sizeof(saved_portrait),
                               "%s\\portrait.png", path);
    if (length <= 0 || length >= sizeof(saved_portrait)) {
        status("workshop-editor-status", "Project path is too long.");
        return false;
    }
    char fallback_portrait[MAX_PATH] = {};
    if (!persist_portrait && chosen.empty() &&
        GetFileAttributesA(saved_portrait) == INVALID_FILE_ATTRIBUTES) {
        DWORD length = GetModuleFileNameA(nullptr, fallback_portrait,
                                          sizeof(fallback_portrait));
        char* basename = length > 0 && length < sizeof(fallback_portrait)
            ? std::strrchr(fallback_portrait, '\\') : nullptr;
        if (basename != nullptr &&
            size_t(basename - fallback_portrait) +
                sizeof("\\ui\\workshop_draft_portrait.png") <=
                    sizeof(fallback_portrait))
            std::strcpy(basename, "\\ui\\workshop_draft_portrait.png");
        else fallback_portrait[0] = '\0';
    }
    const char* portrait_path = !fallback_portrait[0]
        ? (chosen.empty() ? saved_portrait : chosen.c_str())
        : fallback_portrait;
    uint8_t* portrait = nullptr;
    size_t portrait_size = 0;
    uint8_t* package = nullptr;
    size_t package_size = 0;
    NativeContentId id;
    char default_root[PATH_CAPACITY];
    const char* root = nativeMatchLaunch_ContentRoot();
    if (root == nullptr && nativeContentStore_DefaultRoot(
                               default_root, sizeof(default_root))) {
        root = default_root;
    }
    bool opened_project = project_loaded && root != nullptr &&
                          read_portrait(portrait_path, &portrait,
                                        &portrait_size);
    bool built = false;
    std::string preview_source[NATIVE_FIGHTER_FAMILY_MEMBERS];
    std::string preview_parameters[NATIVE_FIGHTER_FAMILY_MEMBERS];
    if (opened_project) {
        bool valid = !save_project || save_draft();
        NativeFighterFamilyProject candidate = active_family;
        const unsigned previous_member = active_member;
        for (unsigned member = 0; valid && !require_built_script &&
             member < candidate.member_count; ++member) {
            valid = select_member(member) &&
                nativeShellWorkshopEditor_PreviewSource(member, &preview_source[member],
                                                        &preview_parameters[member]);
            if (valid) {
                NativeFighterProject& part = candidate.members[member];
                part.script_text = (uint8_t*) preview_source[member].data();
                part.script_size = preview_source[member].size();
                part.parameter_text = (uint8_t*) preview_parameters[member].data();
                part.parameter_size = preview_parameters[member].size();
            }
        }
        select_member(previous_member);
        built = valid && nativeFighterFamilyProject_Build(
                             &candidate, portrait, portrait_size,
                             &package, &package_size, &id);
    }
    if (!built) {
        std::free(portrait);
        status("workshop-editor-status",
               active_family.member_count > 1
                   ? "Could not prepare both source members. Validate each member's behavior and owned assets; the current preview is retained."
                   : "Could not prepare this source fighter. Validate its behavior and assets, and check the portrait.");
        return false;
    }
    NativeFighterPackage metadata;
    NativeFighterPackageData data;
    bool cached = nativeContentStore_ParseFighter(package, package_size,
                                                  &metadata, &data) &&
                  metadata.member_count == active_family.member_count &&
                  nativeContentStore_Cache(root, package, package_size,
                                           nullptr, &id);
    bool valid = cached && nativeFighterRuntime_ValidateCached(
                               &id, root);
    if (valid && persist_portrait && !chosen.empty() &&
        _stricmp(chosen.c_str(), saved_portrait) != 0) {
        valid = CopyFileA(chosen.c_str(), saved_portrait, 0) != 0;
    }
    std::free(package);
    std::free(portrait);
    if (!valid) {
        status("workshop-editor-status", cached && !valid
            ? "Source fighter validation failed. Validate its behavior, parameter values and owned assets."
            : "Could not build this fighter.");
        return false;
    }
    *result = id;
    return true;
}

void enter_draft_preview()
{
    NativeContentId id;
    if (!build_cached_candidate(false, false, false, &id)) {
        status("workshop-editor-status", "Could not build the draft for the "
               "practice preview. Check its assets and portrait.");
        return;
    }
    const bool entering = nativeHome_IsSelection()
        ? nativeHome_RequestAuthoringPractice(&id, false)
        : nativeHome_IsPractice() && nativeHome_RequestPreview(&id, false);
    preview_loading = entering;
    status("workshop-editor-status", entering
        ? "Loading the draft fighter into practice..."
        : "Could not open the draft's practice preview.");
}

void preview_draft()
{
    NativeContentId id;
    if (!build_cached_candidate(false, false, false, &id)) {
        return;
    }
    const bool entering = nativeHome_IsSelection()
        ? nativeHome_RequestAuthoringPractice(&id, true)
        : nativeHome_RequestPreview(&id, true);
    if (!entering) {
        status("workshop-editor-status", "Could not start practice preview.");
        return;
    }
    while (nativeWorkshopDocs_Close()) {}
    testing = true;
    preview_loading = true;
    nativeShell_Blur();
    document->GetElementById("workshop-panel")->SetClass("testing", true);
    document->GetElementById("workshop-inspector")
        ->SetProperty("display", "none");
    nativeShellWorkshopEditor_SetTesting(true);
    status("workshop-test-note", "Live practice with current draft behavior");
    status("workshop-editor-status", "Starting live practice test...");
}

void install_draft()
{
    if (family_script_changed() && !build_script(false)) return;
    NativeContentId id;
    if (!build_cached_candidate(true, true, true, &id)) {
        return;
    }
    const char* root = nativeMatchLaunch_ContentRoot();
    char default_root[PATH_CAPACITY];
    if (root == nullptr && nativeContentStore_DefaultRoot(
                               default_root, sizeof(default_root))) {
        root = default_root;
    }
    NativeContentId replaced = {};
    if (root == nullptr || !nativeFighterProject_Install(
                               drafts[selected].path, root, &id, &replaced)) {
        status("workshop-editor-status", "Could not install this fighter.");
        return;
    }
    const NativeContentId empty = {};
    if (std::memcmp(&replaced, &empty, sizeof(empty)) != 0) {
        nativeHome_ReplaceInstalledFighter(&replaced, &id);
        nativeCssOriginal_ReloadInstalledFighters();
    }
    nativeShellCssOriginal_RefreshInstalledFighters();
    status("workshop-editor-status", std::memcmp(&replaced, &empty, sizeof(empty))
        ? "Installed. Your character-select tiles now use this build."
        : "Installed. Add this fighter to character select with Edit CSS.");
}

bool build_script(bool show_script)
{
    if (!project_loaded || selected < 0 ||
        size_t(selected) >= draft_count) return false;
    const unsigned previous_member = active_member;
    if (!save_draft()) return false;
    for (unsigned member = 0; member < active_family.member_count; ++member) {
        if (!select_member(member) ||
            !nativeShellWorkshopEditor_BuildScript(active_family.paths[member])) {
            status("workshop-editor-status", "Could not build this fighter's script. Open Scripting for details.");
            return false;
        }
    }
    NativeFighterFamilyProject replacement;
    if (!nativeFighterFamilyProject_Open(drafts[selected].path, &replacement)) {
        status("workshop-editor-status", "Could not reopen the saved draft. Your current edits are still open.");
        return false;
    }
    // Keep editor project addresses and DAT allocations stable. Building
    // behavior must not discard either member's model/animation undo history.
    for (unsigned member = 0; member < active_family.member_count; ++member) {
        NativeFighterProject& current = active_family.members[member];
        NativeFighterProject& built = replacement.members[member];
        std::swap(current.asset_record, built.asset_record);
        std::swap(current.asset_record_size, built.asset_record_size);
        std::swap(current.resources, built.resources);
        std::swap(current.resource_size, built.resource_size);
        current.facts = built.facts;
        std::swap(current.script_text, built.script_text);
        std::swap(current.script_size, built.script_size);
    }
    nativeFighterFamilyProject_Close(&replacement);
    select_member(previous_member);
    nativeShellWorkshopEditor_RefreshScriptOutline();
    if (show_script) nativeShellWorkshopEditor_Click("workshop-mode-script");
    return true;
}

class WorkshopEvents final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        Rml::Element* target = event.GetTargetElement();
        while (target != nullptr && target->GetId().empty()) {
            target = target->GetParentNode();
        }
        if (target == nullptr) {
            return;
        }
        const Rml::String id = target->GetId();
        if (event.GetType() == "change" || event.GetType() == "input") {
            if (id == "workshop-family-member") {
                if (updating_member_selector) return;
                const std::string value = static_cast<Rml::ElementFormControlSelect*>(target)->GetValue();
                if (value == "0" || value == "1") select_member(unsigned(value[0] - '0'), true);
            } else if (id == "workshop-action-search") {
                nativeShellWorkshopEditor_FilterActions();
            } else if (id == "workshop-animation-search") {
                nativeShellWorkshopEditor_FilterClips();
            } else if (id == "workshop-script-source") {
                discard_pending = false;
                nativeShellWorkshopEditor_RefreshScriptOutline();
            } else {
                discard_pending = false;
            }
            return;
        }
        if (project_loaded && id.compare(0, 19, "workshop-attribute-") == 0 &&
            (event.GetType() == "keydown" || event.GetType() == "blur")) {
            const unsigned index = std::strtoul(id.c_str() + 19, nullptr, 10);
            if (index >= NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT) return;
            auto* control = input(id.c_str());
            auto& saved = attribute_text[active_member][index];
            if (event.GetType() == "blur") {
                control->SetValue(saved);
            } else if (event.GetParameter<int>("key_identifier", 0) ==
                       Rml::Input::KI_RETURN) {
                const auto value = control->GetValue();
                char* end;
                const double number = std::strtod(value.c_str(), &end);
                if (value != saved && end != value.c_str() && *end == '\0' &&
                    nativeFighterProject_SetAttribute(active_project, index, number)) {
                    saved = value;
                    attributes_dirty[active_member] = true;
                    discard_pending = false;
                    status("workshop-editor-status", "Attribute updated.");
                } else if (value != saved) {
                    status("workshop-editor-status", "Enter a valid attribute value.");
                }
                control->Blur();
                event.StopPropagation();
            }
            return;
        }
        if (event.GetType() == "keydown") {
            const int key = event.GetParameter<int>("key_identifier", 0);
            if (key == Rml::Input::KI_ESCAPE && fighter_menu_open) {
                show_fighter_menu(false);
                event.StopPropagation();
                return;
            }
            if (key == Rml::Input::KI_ESCAPE &&
                (nativeWorkshopDocs_Close() || nativeShellWorkshopEditor_Shortcut(key))) {
                event.StopPropagation();
            } else if (key == Rml::Input::KI_ESCAPE && opened) {
                if (testing) {
                    if (!nativeHome_ReturnToWorkshopPreview()) {
                        status("workshop-editor-status",
                               "Wait for live practice before returning to the editor.");
                        event.StopPropagation();
                        return;
                    }
                    testing = false;
                    preview_loading = true;
                    status("workshop-editor-status", "Returning to the draft preview...");
                    document->GetElementById("workshop-panel")
                        ->SetClass("testing", false);
                    nativeShellWorkshopEditor_SetTesting(false);
                    document->GetElementById("workshop-inspector")
                        ->SetProperty("display", package_open ? "block" : "none");
                } else nativeShellWorkshop_Close();
                event.StopPropagation();
            } else if (key == Rml::Input::KI_RETURN &&
                       id == "workshop-new-name") {
                create_stock_clone();
                event.StopPropagation();
            } else if ((key == Rml::Input::KI_RETURN ||
                        key == Rml::Input::KI_SPACE) &&
                       target->GetTagName() == "button") {
                target->Click();
                event.StopPropagation();
            } else if (target->GetTagName() != "input" &&
                       target->GetTagName() != "textarea" &&
                       !event.GetParameter<int>("ctrl_key", 0) &&
                       !event.GetParameter<int>("alt_key", 0) &&
                       nativeShellWorkshopEditor_Shortcut(key)) {
                event.StopPropagation();
            }
            return;
        }
        if (id == "workshop-script-validate") {
            if (save_draft()) nativeShellWorkshopEditor_Click(id.c_str());
            return;
        }
        if (id == "workshop-script-build") {
            build_script(true);
            return;
        }
        if (id == "workshop-return-editor") {
            if (!nativeHome_ReturnToWorkshopPreview()) {
                status("workshop-editor-status",
                       "Wait for live practice before returning to the editor.");
                return;
            }
            testing = false;
            preview_loading = true;
            status("workshop-editor-status", "Returning to the draft preview...");
            document->GetElementById("workshop-panel")->SetClass("testing", false);
            nativeShellWorkshopEditor_SetTesting(false);
            document->GetElementById("workshop-inspector")
                ->SetProperty("display", package_open ? "block" : "none");
            return;
        }
        if (id != "workshop-back-drafts" && id != "workshop-close")
            discard_pending = false;
        if (id == "workshop-fighter-toggle") {
            show_fighter_menu(!fighter_menu_open);
            return;
        }
        if (id != "workshop-portrait-path" && id != "workshop-portrait-browse")
            show_fighter_menu(false);
        if (nativeShellWorkshopEditor_ScrubTimeline(id.c_str(),
                event.GetParameter<int>("mouse_x", 0))) return;
        if (nativeShellWorkshopEditor_Click(
                id.c_str(), event.GetParameter<int>("shift_key", 0) != 0))
            return;
        if (id == "workshop-close") {
            nativeShellWorkshop_Close();
        } else if (id == "workshop-back-drafts") {
            if (return_destination == NativeWorkshopOrigin::Hub) nativeShellWorkshop_Close();
            else if (confirm_discard()) refresh_drafts();
        } else if (id == "workshop-package-toggle") {
            package_open = !package_open;
            document->GetElementById("workshop-inspector")
                ->SetProperty("display", package_open ? "block" : "none");
            document->GetElementById("workshop-inspector")
                ->SetClass("workshop-package-open", package_open);
            document->GetElementById("workshop-package-toggle")
                ->SetClass("selected", package_open);
        } else if (id == "workshop-create") {
            create_stock_clone();
        } else if (id == "workshop-save-top") {
            save_draft();
        } else if (id == "workshop-portrait-browse") {
            browse_portrait();
        } else if (id == "workshop-stock-icon-browse" || id == "workshop-emblem-browse") {
            browse_hud_image(id == "workshop-emblem-browse");
        } else if (id == "workshop-fighter-import-package") {
            import_fighter();
        } else if (id == "workshop-portrait-edit") {
            if (!project_loaded || !save_draft()) return;
            char path[PATH_CAPACITY]; std::snprintf(path,sizeof(path),"%s",active_family.paths[0]);
            unsigned member=active_member;
            nativeShellWorkshop_Close();
            if (!opened) nativeHubPortrait_Open(path,member,true);
        } else if (id == "workshop-test-character") {
            preview_draft();
        } else if (id == "workshop-install") {
            install_draft();
        } else if (id.compare(0, 15, "workshop-draft-") == 0) {
            char* end = nullptr;
            long index = std::strtol(id.c_str() + 15, &end, 10);
            if (*end == '\0' && index >= 0) {
                select_draft((size_t) index);
            }
        }
    }
} events;

} // namespace

extern "C" bool nativeShellWorkshop_ImportHudImage(unsigned target, const char* file)
{
    if (!opened || testing || !project_loaded || target > 1 || !file || !*file) return false;
    uint8_t* bytes = nullptr; size_t size = 0;
    uint16_t width = 0, height = 0;
    bool valid = read_portrait(file, &bytes, &size) &&
        nativeSourceResources_PNG(bytes, size, &width, &height, nullptr, 0);
    unsigned char* pixels = valid ? nativeGxImage_Decode(bytes, unsigned(size), &width, &height) : nullptr;
    valid = valid && pixels;
    nativeGxImage_Free(pixels);
    const char* name = target ? "ui/emblem.png" : "ui/stock-icon.png";
    if (!valid || !save_draft() || !nativeFighterProject_SetResource(
            active_family.paths[active_member], name, NativeSourceResource_PNG, bytes, size)) {
        std::free(bytes);
        status("workshop-editor-status", "Could not import this PNG. The previous HUD image is retained.");
        return false;
    }
    std::free(bytes);
    NativeFighterProject saved{};
    if (!nativeFighterProject_Open(active_family.paths[active_member], &saved)) {
        status("workshop-editor-status", "Image saved, but could not reopen the draft. Reopen it before building.");
        return false;
    }
    std::swap(active_project->resources, saved.resources);
    std::swap(active_project->resource_size, saved.resource_size);
    nativeFighterProject_Close(&saved);
    status("workshop-editor-status", target ? "HUD emblem imported. Build & Install to use it in matches."
                                          : "Stock icon imported. Build & Install to use it in matches.");
    return true;
}

extern "C" bool nativeShellWorkshop_ImportFighter(const char* file)
{
    if (!opened || testing || !project_loaded || !file || !*file) return false;
    if (draft_count >= MAX_DRAFTS) {
        status("workshop-editor-status", "The Workshop draft library is full.");
        return false;
    }
    if (!save_draft()) return false;
    char draft[PATH_CAPACITY];
    if (!nativeHubCatalog_ImportFighter(file, draft)) {
        status("workshop-editor-status", "Could not import this fighter package. Your current draft is still open.");
        return false;
    }
    refresh_drafts();
    for (size_t i = 0; i < draft_count; ++i) {
        if (std::strcmp(drafts[i].path, draft)) continue;
        select_draft(i);
        status("workshop-editor-status", "Fighter imported as a new draft.");
        return true;
    }
    status("workshop-library-status", "The fighter was imported, but its draft could not be opened.");
    return false;
}

bool nativeShellWorkshop_RefreshPreview()
{
    if (testing || !nativeHome_IsPractice()) return false;
    NativeContentId id;
    if (!build_cached_candidate(false, false, false, &id) ||
        !nativeHome_RequestPreview(&id, false)) return false;
    preview_loading = true;
    status("workshop-editor-status", "Refreshing the draft preview...");
    return true;
}

void nativeShellWorkshop_Open(const char* draft_path, NativeWorkshopOrigin origin)
{
    if (!nativeHome_IsSelection() && !nativeHome_IsPractice()) {
        nativeHub_Notice("Return home to edit a fighter.");
        return;
    }
    if (!nativeHome_CanOpenWorkshop()) {
        nativeHub_Notice("Return to character select outside Quick Play to edit a fighter.");
        return;
    }
    if (opened && !confirm_discard()) return;
    nativeShellCssOriginal_Close();
    refresh_drafts();
    return_destination = origin;
    opened = true;
    nativeHome_SetAuthoring(true);
    document->GetElementById("workshop-panel")->SetProperty("display", "block");
    document->GetElementById("workshop-close")->SetInnerRML(origin == NativeWorkshopOrigin::Collection ? "&lt; BACK TO PACKAGES" : "&lt; BACK");
    document->GetElementById("workshop-back-drafts")->SetAttribute("title",
        origin == NativeWorkshopOrigin::Hub ? "Back to Workshop" : "Fighter drafts");
    input("workshop-new-name")->Focus(true);
    if (draft_path != nullptr) {
        for (size_t i = 0; i < draft_count; ++i)
            if (std::strcmp(drafts[i].path, draft_path) == 0) { select_draft(i); return; }
        status("workshop-library-status", "That draft is no longer available.");
    }
}

void nativeShellWorkshop_Init(Rml::ElementDocument* shell)
{
    document = shell;
    create_attribute_controls();
    auto* base_select = static_cast<Rml::ElementFormControlSelect*>(
        document->GetElementById("workshop-new-base"));
    for (int index = Ft_Kind_Mario; index < Ft_Kind_StockCount; ++index) {
        const FighterKind kind = static_cast<FighterKind>(index);
        const char* name = nativeFighterProject_StockBaseName(kind);
        if (name != nullptr &&
            nativeFighterRuntime_FindStockDefinition(kind) != nullptr)
            base_select->Add(name, name);
    }
    opened = false;
    testing = false;
    project_loaded = false;
    nativeShellWorkshopEditor_Init(shell);
    document->GetElementById("workshop-panel")
        ->AddEventListener("click", &events);
    document->GetElementById("workshop-panel")
        ->AddEventListener("keydown", &events);
    document->GetElementById("workshop-action-search")
        ->AddEventListener("change", &events);
    document->GetElementById("workshop-action-search")
        ->AddEventListener("input", &events);
    document->GetElementById("workshop-animation-search")
        ->AddEventListener("change", &events);
    document->GetElementById("workshop-animation-search")
        ->AddEventListener("input", &events);
    document->GetElementById("workshop-script-source")
        ->AddEventListener("change", &events);
    document->GetElementById("workshop-script-source")
        ->AddEventListener("input", &events);
    document->GetElementById("workshop-family-member")
        ->AddEventListener("change", &events);
    for (size_t index = 0; index < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++index)
        document->GetElementById(attribute_id(index))->AddEventListener("blur", &events);
}

void nativeShellWorkshop_Update()
{
    if (opened && !testing) nativeShellWorkshopEditor_Update();
    if (opened && project_loaded) {
        if (preview_loading && !nativeHome_PracticeTransitioning()) {
            preview_loading = false;
            if (nativeHome_IsPractice()) {
                status("workshop-editor-status", testing
                    ? "Live practice ready." : "Draft preview ready.");
            } else {
                testing = false;
                document->GetElementById("workshop-panel")->SetClass("testing", false);
                document->GetElementById("workshop-inspector")
                    ->SetProperty("display", package_open ? "block" : "none");
                nativeShellWorkshopEditor_SetTesting(false);
                status("workshop-editor-status", "Could not load the draft preview.");
            }
        }
        static int last_practice = -1;
        const int practice = nativeHome_IsPractice() ? 1 : 0;
        if (practice != last_practice) {
            last_practice = practice;
            status("workshop-preview-state", practice
                ? "live practice" : "enter practice to preview");
        }
    }
    if (opened && !nativeHome_IsSelection() && !nativeHome_IsPractice() &&
        !nativeHome_PracticeTransitioning()) {
        if (project_loaded &&
            family_has_unsaved_edits() &&
            !save_draft()) return;
        nativeShellWorkshop_Close();
    }
}

void nativeShellWorkshop_Close()
{
    if (!opened || nativeHome_PracticeTransitioning()) {
        return;
    }
    if (!confirm_discard()) return;
    opened = false;
    close_project();
    nativeHome_SetAuthoring(false);
    document->GetElementById("workshop-panel")
        ->SetProperty("display", "none");
    nativeShell_Blur();
    if (return_destination == NativeWorkshopOrigin::Collection && (nativeHome_IsSelection() || nativeHome_IsPractice())) {
        nativeHubPackages_Open();
    }
}

bool nativeShellWorkshop_IsOpen()
{
    return opened;
}

bool nativeShellWorkshop_IsTesting()
{
    return opened && testing;
}

#endif
