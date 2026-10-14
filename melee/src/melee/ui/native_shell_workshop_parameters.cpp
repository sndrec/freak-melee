#include "native_shell_workshop_parameters.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_project.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlInput.h>
#include <RmlUi/Core/Elements/ElementFormControlSelect.h>
#include <melee/ft/native_script_source.h>
#include <vector>
#include <map>

#include <cstdio>
#include <charconv>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
void status(Rml::ElementDocument* document, const std::string& message)
{
    document->GetElementById("workshop-parameters-status")
        ->SetInnerRML(Rml::StringUtilities::EncodeRml(message));
}

struct ParameterRow { std::string name, type, default_value, group; };
std::vector<ParameterRow> rows;
std::string parameter_text;
Rml::ElementDocument* panel_document;
std::map<std::string, bool> collapsed;
bool rebuilding;

std::string literal(const NativeFighterParameter& field)
{
    char text[48];
    if (field.type == NativeFighterParameter_Float) {
        float value; std::memcpy(&value, &field.bits, sizeof(value));
        const auto formatted = std::to_chars(text, text + sizeof(text) - 1, value);
        *formatted.ptr = 0;
    } else if (field.type == NativeFighterParameter_Int)
        std::snprintf(text, sizeof(text), "%d", (int32_t)field.bits);
    else std::strcpy(text, field.bits ? "true" : "false");
    return text;
}

void render_rows()
{
    NativeFighterParameters overrides;
    if (!nativeFighterParameters_Parse(parameter_text.data(), parameter_text.size(), &overrides, nullptr)) return;
    std::string markup, group;
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (group != row.group) {
            if (!group.empty()) markup += "</div>";
            group = row.group;
            markup += "<button id='workshop-param-group-" + std::to_string(i) +
                "' class='workshop-param-group'>" + (collapsed[group] ? "+ " : "- ") +
                Rml::StringUtilities::EncodeRml(group) + "</button><div>";
        }
        if (collapsed[group]) continue;
        std::string value = row.default_value;
        bool overridden = false;
        for (unsigned f = 0; f < overrides.count; ++f)
            if (row.name == overrides.fields[f].name) { value = literal(overrides.fields[f]); overridden = true; break; }
        const std::string id = "workshop-param-value-" + std::to_string(i);
        markup += "<div class='workshop-parameter-row'><label for='" + id + "' title='attrs." + row.name + "'>" +
            Rml::StringUtilities::EncodeRml(row.name) + "<small>" + row.type + (overridden ? " · override" : " · default") + "</small></label>";
        if (row.type == "bool")
            markup += "<select id='" + id + "' class='room-input'><option value='false'" + (value == "false" ? " selected='selected'" : "") +
                ">false</option><option value='true'" + (value == "true" ? " selected='selected'" : "") + ">true</option></select>";
        else markup += "<input id='" + id + "' class='room-input' type='text' value='" + Rml::StringUtilities::EncodeRml(value) + "' />";
        markup += "<button id='workshop-param-reset-" + std::to_string(i) + "' title='Use the source declaration default'>Reset</button></div>";
    }
    if (!group.empty()) markup += "</div>";
    rebuilding = true;
    panel_document->GetElementById("workshop-parameters-list")->SetInnerRML(markup.empty()
        ? "<p class='workshop-muted'>Declare a typed parameter in Fighter state or an entity to expose it here.</p>" : markup);
    rebuilding = false;
}

class ParameterEvents final : public Rml::EventListener {
    void ProcessEvent(Rml::Event& event) override
    {
        if (rebuilding) return;
        auto* target = event.GetTargetElement();
        while (target && target->GetId().empty()) target = target->GetParentNode();
        if (!target) return;
        const std::string id = target->GetId();
        const bool value = id.compare(0, 21, "workshop-param-value-") == 0;
        const bool reset = id.compare(0, 21, "workshop-param-reset-") == 0;
        const bool group = id.compare(0, 21, "workshop-param-group-") == 0;
        if ((!value && !reset && !group) ||
            (value ? event.GetType() != "change" : event.GetType() != "click")) return;
        char* end = nullptr; const size_t index = std::strtoul(id.c_str()+21, &end, 10);
        if (*end || index >= rows.size()) return;
        if (group) { collapsed[rows[index].group] = !collapsed[rows[index].group]; render_rows(); return; }
        const auto& row = rows[index];
        std::string next;
        NativeFighterParameters overrides;
        if (!nativeFighterParameters_Parse(parameter_text.data(), parameter_text.size(), &overrides, nullptr)) return;
        for (unsigned i = 0; i < overrides.count; ++i) {
            const auto& field = overrides.fields[i];
            if (row.name == field.name) continue;
            next += std::string(field.type == NativeFighterParameter_Float ? "float " : field.type == NativeFighterParameter_Int ? "int " : "bool ") +
                field.name + " = " + literal(field) + "\n";
        }
        if (!reset) next += row.type + " " + row.name + " = " +
            static_cast<Rml::ElementFormControl*>(target)->GetValue() + "\n";
        NativeFighterParameters checked;
        if (!nativeFighterParameters_Parse(next.data(), next.size(), &checked, nullptr)) {
            status(panel_document, "Enter a valid " + row.type + " value for " + row.name + "."); return;
        }
        parameter_text = std::move(next);
        status(panel_document, "Edited parameter values. Save or Test Character to apply.");
        render_rows();
    }
} parameter_events;

}

void nativeShellWorkshopParameters_Open(Rml::ElementDocument* document,
                                       NativeFighterProject* project)
{
    if (document == nullptr || project == nullptr) return;
    panel_document = document;
    static bool attached = false;
    if (!attached) {
        document->GetElementById("workshop-parameters-list")->AddEventListener("change", &parameter_events);
        document->GetElementById("workshop-parameters-list")->AddEventListener("click", &parameter_events);
        attached = true;
    }
    nativeShellWorkshopParameters_SetText(document, project->parameter_text != nullptr
        ? std::string((const char*)project->parameter_text, project->parameter_size) : "");
    status(document, "Values are read when the fighter loads.");
}

void nativeShellWorkshopParameters_SetText(Rml::ElementDocument* document, const std::string& text)
{
    panel_document = document; parameter_text = text; rows.clear();
}

const std::string& nativeShellWorkshopParameters_Text() { return parameter_text; }

void nativeShellWorkshopParameters_Refresh(Rml::ElementDocument* document, const std::string& source)
{
    panel_document = document;
    static NativeScriptSourceNode nodes[16384];
    NativeScriptSourceTree tree = {};
    tree.source = source.data(); tree.source_bytes = (uint32_t)source.size();
    tree.nodes = nodes; tree.capacity = 16384;
    NativeScriptDiagnostic diagnostic = {};
    if (!nativeScriptSource_Parse(&tree, &diagnostic)) return;
    std::vector<ParameterRow> next;
    for (uint32_t i = 0; i < tree.count; ++i) {
        const auto& node = nodes[i];
        if (node.kind != NativeScriptSource_Parameter) continue;
        ParameterRow row;
        row.name.assign(tree.source + node.name.offset, node.name.length);
        row.type = node.value_type == NativeScriptType_F32 ? "float" : node.value_type == NativeScriptType_Bool ? "bool" : "int";
        row.default_value.assign(tree.source + node.value.offset, node.value.length);
        // The declaration span includes its '=' token. Display the value
        // itself so numeric inputs and boolean dropdowns receive literals.
        const size_t value_start = row.default_value.find_first_not_of(" \t=");
        row.default_value.erase(0, value_start == std::string::npos
                                      ? row.default_value.size() : value_start);
        row.group = "Fighter";
        for (uint32_t parent = node.parent; parent != 0; parent = nodes[parent].parent) {
            if (nodes[parent].kind == NativeScriptSource_Article || nodes[parent].kind == NativeScriptSource_Item || nodes[parent].kind == NativeScriptSource_Effect) {
                row.group.assign(tree.source + nodes[parent].name.offset, nodes[parent].name.length);
                row.name = row.group + "." + row.name; break;
            }
        }
        next.push_back(std::move(row));
    }
    rows = std::move(next); render_rows();
}

bool nativeShellWorkshopParameters_Dirty(Rml::ElementDocument* document,
                                        const NativeFighterProject* project)
{
    if (document == nullptr || project == nullptr) return false;
    const std::string source = nativeShellWorkshopParameters_Text();
    return source.size() != project->parameter_size ||
        (!source.empty() && std::memcmp(source.data(), project->parameter_text,
                                       source.size()) != 0);
}

bool nativeShellWorkshopParameters_Import(Rml::ElementDocument* document,
                                         const NativeFighterProject* project,
                                         std::string* state_source)
{
    if (document == nullptr || project == nullptr || state_source == nullptr ||
        (state_source->compare(0, 10, "attribute ") != 0 &&
         state_source->find("\nattribute ") == std::string::npos &&
         state_source->find("\n    attribute ") == std::string::npos)) return false;
    char *converted = nullptr, *parameters = nullptr;
    size_t converted_size = 0, parameter_size = 0;
    unsigned error_line = 0;
    if (!nativeFighterParameters_Import(project->fighter_dat,
        project->fighter_size, project->fighter_root, project->base_kind, state_source->data(),
        state_source->size(), &converted, &converted_size, &parameters,
        &parameter_size, &error_line)) {
        status(document, "Could not copy imported attributes at line " +
               std::to_string(error_line) + ". Original source retained.");
        return false;
    }
    std::string values = nativeShellWorkshopParameters_Text();
    if (!values.empty() && values.back() != '\n') values += '\n';
    values.append(parameters, parameter_size);
    NativeFighterParameters checked;
    if (!nativeFighterParameters_Parse(values.data(), values.size(), &checked,
                                       &error_line)) {
        std::free(converted);
        std::free(parameters);
        status(document, "Imported parameters conflict with existing values. Original source retained.");
        return false;
    }
    state_source->assign(converted, converted_size);
    nativeShellWorkshopParameters_SetText(document, values);
    std::free(converted);
    std::free(parameters);
    status(document, "Stock values copied into this draft. Save and build behavior.");
    return true;
}

bool nativeShellWorkshopParameters_Save(Rml::ElementDocument* document,
                                       NativeFighterProject* project,
                                       const char* project_path)
{
    if (document == nullptr || project == nullptr || project_path == nullptr)
        return false;
    if (!nativeShellWorkshopParameters_Dirty(document, project)) return true;
    const std::string source = nativeShellWorkshopParameters_Text();
    NativeFighterParameters next;
    unsigned error_line = 0;
    if (!nativeFighterParameters_Parse(source.data(), source.size(), &next,
                                       &error_line)) {
        status(document, "Invalid parameter on line " +
              std::to_string(error_line) +
              ". Use float/int/bool Name = value.");
        return false;
    }
    NativeFighterParameters previous;
    const bool previous_valid = nativeFighterParameters_Parse(
        project->parameter_text != nullptr
            ? (const char*) project->parameter_text : "",
        project->parameter_size, &previous, nullptr);
    uint8_t* copied = (uint8_t*) std::malloc(source.size() + 1);
    if (copied == nullptr) {
        status(document, "Could not allocate parameter data.");
        return false;
    }
    std::memcpy(copied, source.data(), source.size());
    char path[512], temporary[512];
    const int length = std::snprintf(path, sizeof(path),
        "%s\\fighter_parameters.fmp", project_path);
    const int temporary_length = std::snprintf(temporary, sizeof(temporary),
        "%s.tmp", path);
    if (length <= 0 || length >= sizeof(path) ||
        temporary_length <= 0 || temporary_length >= sizeof(temporary)) {
        std::free(copied);
        status(document, "Project path is too long.");
        return false;
    }
    FILE* output = std::fopen(temporary, "wb");
    if (output == nullptr) {
        std::free(copied);
        status(document, "Could not save parameters.");
        return false;
    }
    const bool written = std::fwrite(source.data(), 1, source.size(), output)
        == source.size();
    const bool closed = std::fclose(output) == 0;
    if (!written || !closed || !MoveFileExA(temporary, path,
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        DeleteFileA(temporary);
        std::free(copied);
        status(document, "Could not save parameters.");
        return false;
    }
    std::free(project->parameter_text);
    project->parameter_text = copied;
    project->parameter_size = source.size();
    status(document, !previous_valid || previous.schema_hash != next.schema_hash
        ? "Saved. Build behavior after changing names or types."
        : "Saved. Values apply on the next fighter load; no build needed.");
    return true;
}

#endif
