#include <melee/ui/native_hub.h>
#include "native_shell_workshop_docs.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_editor.h"
#include "native_shell_workshop_model.h"
#include "native_shell_workshop_parameters.h"
#include "native_shell_workshop_agent.h"
#include <melee/gm/native_feedback.h>

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
#include <melee/ft/native_fighter_draft_access.h>
#include <melee/ft/native_fighter_snapshot.h>
#include <melee/ft/native_fighter_build.h>
#include <melee/ft/native_fighter_authoring_process.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_runtime.h>
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
#include <vector>

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
NativeFighterDraftLease active_lease{};
std::string saved_disk_revision;
NativeFighterProject* active_project;
unsigned active_member;
bool updating_member_selector;
bool member_opened[NATIVE_FIGHTER_FAMILY_MEMBERS];
bool project_loaded;
uint64_t project_generation;
bool attributes_dirty[NATIVE_FIGHTER_FAMILY_MEMBERS];
bool discard_pending;
bool package_open;
bool fighter_menu_open;
struct BuildRecord { NativeContentId id{}; std::string revision; bool valid = false; };
BuildRecord prepared_build, requested_preview, previous_preview;

void status(const char* id, const char* message);
void mark_family_saved(const std::string& revision);

void show_fighter_menu(bool show)
{
    fighter_menu_open = show;
    document->GetElementById("workshop-fighter-menu")->SetProperty("display", show ? "block" : "none");
    document->GetElementById("workshop-fighter-toggle")->SetClass("selected", show);
}

void close_project()
{
    ++project_generation;
    prepared_build = {}; requested_preview = {}; previous_preview = {};
    testing = false;
    preview_loading = false;
    while (nativeWorkshopDocs_Close()) {}
    document->GetElementById("workshop-panel")->SetClass("testing", false);
    nativeShellWorkshopEditor_Close();
    if (project_loaded) nativeFighterFamilyProject_Close(&active_family);
    nativeFighterDraft_Release(&active_lease);
    saved_disk_revision.clear();
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

} // namespace

void nativeShellWorkshop_RefreshAttributes()
{
    if (!project_loaded || !active_project) return;
    for (unsigned i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i) {
        double value;
        if (!nativeFighterProject_GetAttribute(active_project, i, &value)) continue;
        char text[48];
        const auto result = nativeFighterCommonAttributes[i].type == NativeAttribute_Float
            ? std::to_chars(text, text + sizeof(text) - 1, (float) value)
            : std::to_chars(text, text + sizeof(text) - 1, (long long) value);
        if (result.ec != std::errc{}) continue;
        *result.ptr = 0;
        input(attribute_id(i).c_str())->SetValue(text);
    }
}

bool nativeShellWorkshop_SelectMember(unsigned member, bool restart_preview)
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
    nativeShellWorkshop_RefreshAttributes();
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

namespace {

void select_draft(size_t index)
{
    if (index >= draft_count) {
        return;
    }
    NativeDraftAccess::Lease candidate_lease;
    std::string lease_error;
    if (!candidate_lease.acquire(drafts[index].path, lease_error)) {
        status("workshop-library-status", lease_error.c_str()); return;
    }
    std::string disk_revision;
    if (!NativeDraftAccess::revision(drafts[index].path, disk_revision, lease_error)) {
        status("workshop-library-status", lease_error.c_str()); return;
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
    std::string opened_revision;
    valid = valid && NativeDraftAccess::revision(drafts[index].path, opened_revision, lease_error) &&
        opened_revision == disk_revision;
    if (!valid) {
        nativeFighterFamilyProject_Close(&family);
        status("workshop-library-status", "Could not read that draft's member attributes.");
        return;
    }
    close_project();
    active_lease = candidate_lease.value;
    saved_disk_revision = std::move(disk_revision);
    candidate_lease.value = {};
    document->GetElementById("workshop-panel")->SetClass("library", false);
    active_family = family;
    active_member = 0;
    active_project = &active_family.members[0];
    project_loaded = true;
    for (int i = 0; i < NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT; ++i)
        input(attribute_id(i).c_str())->SetValue(values[0][i]);
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
    nativeShellWorkshop_RequestPreview(false);
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

} // namespace

bool nativeShellWorkshop_Save(std::string* error)
{
    if (!project_loaded || selected < 0 ||
        size_t(selected) >= draft_count) {
        if (error) *error = "Open a fighter draft first.";
        return false;
    }
    const unsigned previous_member = active_member;
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    std::string message;
    bool ready = true;
    for (unsigned member = 0; member < active_family.member_count; ++member) {
        if (!nativeShellWorkshop_SelectMember(member) ||
            !(sources[member] = nativeShellWorkshopEditor_CaptureSources())) {
            message = "Could not capture this member's editable source files.";
            ready = false;
            break;
        }
    }
    nativeShellWorkshop_SelectMember(previous_member);
    std::string after;
    const bool saved = ready && NativeFighterSnapshot::save(active_family, sources,
        saved_disk_revision, after, message);
    if (saved) {
        mark_family_saved(after);
        status("workshop-editor-status", family_script_changed()
            ? "Draft saved. Build behavior before preview." : "Draft saved.");
    } else {
        status("workshop-editor-status", message.c_str());
    }
    if (error) *error = message;
    return saved;
}

bool nativeShellWorkshop_Check(NativeAuthoring::Reply& report)
{
    if (!project_loaded) report.fail("no_draft", "Open a fighter draft first.");
    else {
        NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
        sources[active_member] = nativeShellWorkshopEditor_CaptureSources();
        if (!sources[active_member]) report.fail("invalid_source_set", "Could not capture the current editable source files.");
        else NativeFighterAuthoringProcess::check(active_family, sources, active_member, report);
    }
    nativeShellWorkshopEditor_PresentValidation(report);
    return report.ok && yyjson_mut_get_bool(yyjson_mut_obj_get(report.result, "accepted"));
}

namespace {

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

std::string content_root()
{
    const char* root = nativeMatchLaunch_ContentRoot();
    char fallback[PATH_CAPACITY];
    if (!root && nativeContentStore_DefaultRoot(fallback, sizeof(fallback))) root = fallback;
    return root ? root : "";
}

bool capture_family(NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS])
{
    const unsigned before = active_member;
    bool ready = true;
    for (unsigned i = 0; i < active_family.member_count; ++i) {
        if (!nativeShellWorkshop_SelectMember(i) || !(sources[i] = nativeShellWorkshopEditor_CaptureSources())) { ready = false; break; }
    }
    nativeShellWorkshop_SelectMember(before);
    return ready;
}

void mark_family_saved(const std::string& revision)
{
    saved_disk_revision = revision;
    const unsigned before = active_member;
    for (unsigned i = 0; i < active_family.member_count; ++i) {
        nativeShellWorkshop_SelectMember(i);
        nativeShellWorkshopEditor_MarkAssetsSaved();
        nativeShellWorkshopModel_MarkSaved(i);
        attributes_dirty[i] = false;
    }
    nativeShellWorkshop_SelectMember(before);
    discard_pending = false;
}

bool prepare_build(NativeAuthoring::Reply& report)
{
    if (!project_loaded) { report.fail("no_draft", "Open a fighter draft first."); return false; }
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    if (!capture_family(sources)) { report.fail("source_unavailable", "Could not capture every member's editable source."); return false; }
    std::string image = input("workshop-portrait-path")->GetValue();
    if (image.empty()) {
        image = std::string(active_family.paths[0]) + "/portrait.png";
        if (GetFileAttributesA(image.c_str()) == INVALID_FILE_ATTRIBUTES) {
            char executable[32768]; DWORD length = GetModuleFileNameA(nullptr, executable, sizeof(executable));
            char* end = length && length < sizeof(executable) ? std::strrchr(executable, '\\') : nullptr;
            if (!end) { report.fail("portrait_unavailable", "Could not locate the installed default portrait."); return false; }
            image.assign(executable, end + 1); image += "ui/workshop_draft_portrait.png";
        }
    }
    uint8_t* portrait = nullptr; size_t size = 0;
    if (!read_portrait(image.c_str(), &portrait, &size)) { report.fail("portrait_unavailable", "Choose a readable portrait PNG."); return false; }
    const std::string root = content_root();
    NativeFighterAuthoringProcess::build(active_family, sources, portrait, size, root.c_str(), report);
    std::free(portrait);
    if (!report.ok) {
        const char* message = yyjson_mut_get_str(yyjson_mut_obj_get(report.result, "message"));
        status("workshop-editor-status", message ? message : "Could not build this fighter.");
        return false;
    }
    if (!yyjson_mut_get_bool(yyjson_mut_obj_get(report.result, "accepted"))) {
        size_t index, count; yyjson_mut_val* member;
        yyjson_mut_arr_foreach(yyjson_mut_obj_get(report.result, "member_reports"), index, count, member) {
            if (yyjson_mut_get_bool(yyjson_mut_obj_get(member, "accepted"))) continue;
            const char* key = yyjson_mut_get_str(yyjson_mut_obj_get(member, "key"));
            for (unsigned i = 0; key && i < active_family.member_count; ++i) if (!std::strcmp(key, active_family.keys[i])) {
                nativeShellWorkshop_SelectMember(i);
                NativeAuthoring::Reply problems(report.document); problems.result = member;
                problems.ok = !yyjson_mut_obj_get(member, "code");
                nativeShellWorkshopEditor_PresentValidation(problems); break;
            }
            break;
        }
        yyjson_mut_obj_add_bool(report.document, report.result, "committed", false);
        status("workshop-editor-status", "Fix the reported problems before building this fighter."); return false;
    }
    const char* id = yyjson_mut_get_str(yyjson_mut_obj_get(report.result, "package_id"));
    const char* revision = yyjson_mut_get_str(yyjson_mut_obj_get(report.result, "fighter_revision"));
    if (!id || !revision || !nativeContentStore_IdFromHex(id, &prepared_build.id)) {
        report.fail("build_unavailable", "The builder did not return a complete package identity."); return false;
    }
    prepared_build.revision = revision; prepared_build.valid = true;
    return true;
}

} // namespace

bool nativeShellWorkshop_Build(NativeAuthoring::Reply& report)
{
    if (!prepare_build(report)) return false;
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    for (unsigned i = 0; i < active_family.member_count; ++i) sources[i] = nativeShellWorkshopEditor_SourceSession(i);
    std::string after, error;
    const std::string root = content_root();
    if (!NativeFighterBuild::accept(active_family, sources, root.c_str(), prepared_build.id, saved_disk_revision, after, error)) {
        report.fail("accept_failed", error); status("workshop-editor-status", error.c_str()); return false;
    }
    mark_family_saved(after);
    nativeShellWorkshopEditor_RefreshScriptOutline();
    yyjson_mut_obj_remove_str(report.result, "saved");
    yyjson_mut_obj_add_bool(report.document, report.result, "saved", true);
    NativeAuthoring::add_string(report, report.result, "status", "accepted_behavior");
    status("workshop-editor-status", "Behavior built and saved for the whole fighter."); return true;
}

bool nativeShellWorkshop_Install(NativeAuthoring::Reply& report)
{
    if (!nativeShellWorkshop_Build(report)) return false;
    const std::string root = content_root();
    std::string current, error;
    if (!NativeDraftAccess::revision(active_family.paths[0], current, error) || current != saved_disk_revision) {
        report.fail("revision_conflict", "Draft changed outside the Workshop. Reopen before installing."); return false;
    }
    NativeContentId replaced{};
    if (!nativeFighterProject_Install(active_family.paths[0], root.c_str(), &prepared_build.id, &replaced)) {
        report.fail("install_failed", "Could not install this complete fighter package."); return false;
    }
    const NativeContentId empty{};
    if (std::memcmp(&replaced, &empty, sizeof(empty))) {
        nativeHome_ReplaceInstalledFighter(&replaced, &prepared_build.id);
        nativeCssOriginal_ReloadInstalledFighters();
    }
    nativeShellCssOriginal_RefreshInstalledFighters();
    yyjson_mut_obj_remove_str(report.result, "installed");
    yyjson_mut_obj_add_bool(report.document, report.result, "installed", true);
    yyjson_mut_obj_remove_str(report.result, "status");
    NativeAuthoring::add_string(report, report.result, "status", "installed");
    status("workshop-editor-status", std::memcmp(&replaced, &empty, sizeof(empty))
        ? "Installed. Character-select tiles now use this build."
        : "Installed. Add this fighter to character select with Edit CSS."); return true;
}

bool nativeShellWorkshop_RequestPreview(bool live, NativeAuthoring::Reply* output)
{
    auto* owner = output ? nullptr : yyjson_mut_doc_new(nullptr);
    if (!output && !owner) return false;
    NativeAuthoring::Reply local(output ? output->document : owner);
    auto& report = output ? *output : local;
    bool entered = false;
    if (!opened || !project_loaded || !nativeHome_CanOpenWorkshop()) report.fail("preview_unavailable", "Return to an available home scene before previewing this fighter.");
    else if (prepare_build(report)) {
        entered = nativeHome_IsSelection() ? nativeHome_RequestAuthoringPractice(&prepared_build.id, live)
            : nativeHome_IsPractice() && nativeHome_RequestPreview(&prepared_build.id, live);
        if (!entered) report.fail("preview_unavailable", "Could not request this fighter's practice scene.");
    }
    if (entered) {
        previous_preview = requested_preview; requested_preview = prepared_build;
        testing = live; preview_loading = true;
        if (live) { while (nativeWorkshopDocs_Close()) {} nativeShell_Blur(); }
        document->GetElementById("workshop-panel")->SetClass("testing", live);
        nativeShellWorkshopEditor_SetTesting(live);
        document->GetElementById("workshop-inspector")->SetProperty("display", !live && package_open ? "block" : "none");
        NativeAuthoring::add_string(report, report.result, "status", live ? "practice_loading" : "preview_loading");
        status("workshop-editor-status", live ? "Starting live practice test..." : "Loading the draft preview...");
    }
    if (owner) yyjson_mut_doc_free(owner);
    return entered;
}

bool nativeShellWorkshop_StopTest(NativeAuthoring::Reply& report)
{
    if (!opened || !testing || !nativeHome_ReturnToWorkshopPreview()) {
        report.fail("practice_unavailable", "Wait for this Workshop's practice test before returning to the editor."); return false;
    }
    testing = false; preview_loading = true;
    document->GetElementById("workshop-panel")->SetClass("testing", false);
    nativeShellWorkshopEditor_SetTesting(false);
    document->GetElementById("workshop-inspector")->SetProperty("display", package_open ? "block" : "none");
    NativeAuthoring::add_string(report, report.result, "status", "preview_loading");
    status("workshop-editor-status", "Returning to the draft preview..."); return true;
}

void nativeShellWorkshop_Artifacts(NativeAuthoring::Reply& report)
{
    const std::string root = content_root();
    NativeAuthoring::add_string(report, report.result, "content_root", root);
    char projects[PATH_CAPACITY];
    if (nativeFighterFamilyProject_LocalRoot(projects, sizeof(projects))) NativeAuthoring::add_string(report, report.result, "projects_root", projects);
    if (!project_loaded) return;
    NativeFighterSources::Session* sources[NATIVE_FIGHTER_FAMILY_MEMBERS]{};
    bool complete = true;
    for (unsigned i = 0; i < active_family.member_count; ++i) { sources[i] = nativeShellWorkshopEditor_SourceSession(i); complete &= sources[i] != nullptr; }
    std::string current, error;
    if (complete && NativeFighterBuild::revision(active_family, sources, current, error)) NativeAuthoring::add_string(report, report.result, "fighter_revision", current);
    else yyjson_mut_obj_add_null(report.document, report.result, "fighter_revision");
    bool accepted = complete;
    for (unsigned i = 0; i < active_family.member_count; ++i) accepted &= !nativeShellWorkshopEditor_ScriptChanged(i);
    yyjson_mut_obj_add_bool(report.document, report.result, "accepted_behavior_matches_sources", accepted);
    auto identity = [&](const char* name, const NativeContentId* id) {
        if (!id) { yyjson_mut_obj_add_null(report.document, report.result, name); return; }
        char hex[65]; nativeContentStore_IdHex(id, hex); NativeAuthoring::add_string(report, report.result, name, hex);
    };
    identity("built_package_id", prepared_build.valid ? &prepared_build.id : nullptr);
    identity("requested_preview_package_id", requested_preview.valid ? &requested_preview.id : nullptr);
    NativeHomePracticeWatch watch{};
    bool loaded = !nativeHome_PracticeTransitioning() && nativeHome_PracticeWatch(0, &watch);
    const NativeContentId empty{};
    loaded = loaded && std::memcmp(&watch.package_id, &empty, sizeof(empty));
    identity("loaded_preview_package_id", loaded ? &watch.package_id : nullptr);
    const BuildRecord* actual = nullptr;
    for (const auto* record : {&requested_preview, &previous_preview, &prepared_build})
        if (loaded && record->valid && !std::memcmp(&record->id, &watch.package_id, sizeof(record->id))) { actual = record; break; }
    if (actual) NativeAuthoring::add_string(report, report.result, "preview_fighter_revision", actual->revision);
    else yyjson_mut_obj_add_null(report.document, report.result, "preview_fighter_revision");
    yyjson_mut_obj_add_bool(report.document, report.result, "preview_ready", actual != nullptr);
    yyjson_mut_obj_add_bool(report.document, report.result, "preview_matches_fighter", actual && complete && actual->revision == current);
    FILE* receipt = std::fopen((std::string(active_family.paths[0]) + "/installed.id").c_str(), "rb");
    char hex[66]{}; NativeContentId installed{}; bool found = false;
    if (receipt) { size_t size = std::fread(hex, 1, sizeof(hex), receipt); std::fclose(receipt); if (size == 65 && hex[64] == '\n') { hex[64] = 0; found = nativeContentStore_IdFromHex(hex, &installed); } }
    identity("installed_package_id", found && nativeContentStore_IsInstalled(root.c_str(), &installed) ? &installed : nullptr);
}

namespace {

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
                if (value == "0" || value == "1") nativeShellWorkshop_SelectMember(unsigned(value[0] - '0'), true);
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
        if (event.GetType() == "click" && nativeWorkshopAgent_Click(id.c_str())) {
            if (id == "workshop-agent-show") show_fighter_menu(false);
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
        if (id == "workshop-script-build") {
            auto* owner = yyjson_mut_doc_new(nullptr);
            if (owner) { NativeAuthoring::Reply report(owner); nativeShellWorkshop_Build(report); yyjson_mut_doc_free(owner); }
            return;
        }
        if (id == "workshop-return-editor") {
            auto* owner = yyjson_mut_doc_new(nullptr);
            if (owner) { NativeAuthoring::Reply report(owner); nativeShellWorkshop_StopTest(report); yyjson_mut_doc_free(owner); }
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
            nativeShellWorkshop_Save();
        } else if (id == "workshop-portrait-browse") {
            browse_portrait();
        } else if (id == "workshop-stock-icon-browse" || id == "workshop-emblem-browse") {
            browse_hud_image(id == "workshop-emblem-browse");
        } else if (id == "workshop-fighter-import-package") {
            import_fighter();
        } else if (id == "workshop-portrait-edit") {
            if (project_loaded && nativeShellWorkshop_Save()) nativeShellWorkshop_OpenPortrait();
        } else if (id == "workshop-test-character") {
            nativeShellWorkshop_RequestPreview(true);
        } else if (id == "workshop-install") {
            auto* owner = yyjson_mut_doc_new(nullptr);
            if (owner) { NativeAuthoring::Reply report(owner); nativeShellWorkshop_Install(report); yyjson_mut_doc_free(owner); }
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
    auto* owner=yyjson_mut_doc_new(nullptr);if(!owner)return false;
    auto* args=yyjson_mut_obj(owner);yyjson_mut_doc_set_root(owner,args);
    auto* changes=yyjson_mut_arr(owner);auto* change=yyjson_mut_obj(owner);
    yyjson_mut_obj_add_val(owner,args,"changes",changes);yyjson_mut_arr_append(changes,change);
    yyjson_mut_obj_add_str(owner,args,"summary",target?"Import HUD emblem":"Import stock icon");
    yyjson_mut_obj_add_str(owner,change,"operation","import");yyjson_mut_obj_add_str(owner,change,"name",target?"ui/emblem.png":"ui/stock-icon.png");
    yyjson_mut_obj_add_str(owner,change,"kind","png");yyjson_mut_obj_add_strcpy(owner,change,"path",file);
    auto* request=yyjson_mut_doc_imut_copy(owner,nullptr);
    NativeAuthoring::Reply reply(owner);
    if(request)nativeShellWorkshopEditor_Resources("edit",yyjson_doc_get_root(request),reply);
    else reply.fail("allocation_failed","Could not capture this image import.");
    if(!reply.ok)status("workshop-editor-status","Could not import this PNG. The previous HUD image is retained.");
    const bool ok=reply.ok;yyjson_doc_free(request);yyjson_mut_doc_free(owner);return ok;
}

extern "C" bool nativeShellWorkshop_ImportFighter(const char* file)
{
    if (!opened || testing || !project_loaded || !file || !*file) return false;
    if (draft_count >= MAX_DRAFTS) {
        status("workshop-editor-status", "The Workshop draft library is full.");
        return false;
    }
    if (!nativeShellWorkshop_Save()) return false;
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
    nativeWorkshopAgent_Init(shell);
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
            !nativeShellWorkshop_Save()) return;
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

bool nativeShellWorkshop_OpenPortrait()
{
    if (!opened || !project_loaded || family_has_unsaved_edits() || nativeHubPortrait_Requested()) return false;
    char path[PATH_CAPACITY];
    std::snprintf(path, sizeof(path), "%s", active_family.paths[0]);
    const unsigned member = active_member;
    nativeShellWorkshop_Close();
    if (opened) return false;
    nativeHubPortrait_Open(path, member, true);
    return nativeHubPortrait_Requested();
}

bool nativeShellWorkshop_IsTesting()
{
    return opened && testing;
}

NativeWorkshopLiveState nativeShellWorkshop_LiveState()
{
    return {project_loaded ? &active_family : nullptr, active_member, project_generation,
        opened, testing, preview_loading,
        !nativeFeedback_IsOpen() && !testing && !preview_loading && nativeHome_CanOpenWorkshop() &&
        (nativeHome_IsSelection() || nativeHome_IsPractice()), family_has_unsaved_edits(), saved_disk_revision.c_str()};
}

#endif
