#include "native_shell_workshop_parameters.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_fighter_project.h>
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/ElementFormControlTextArea.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {
void status(Rml::ElementDocument* document, const std::string& message)
{
    document->GetElementById("workshop-parameters-status")
        ->SetInnerRML(Rml::StringUtilities::EncodeRml(message));
}

Rml::ElementFormControlTextArea* editor(Rml::ElementDocument* document)
{
    return static_cast<Rml::ElementFormControlTextArea*>(
        document->GetElementById("workshop-parameters-source"));
}
}

void nativeShellWorkshopParameters_Open(Rml::ElementDocument* document,
                                       NativeFighterProject* project)
{
    if (document == nullptr || project == nullptr) return;
    editor(document)->SetValue(project->parameter_text != nullptr
        ? std::string((const char*) project->parameter_text,
                      project->parameter_size) : "");
    status(document, "Values are read when the fighter loads.");
}

bool nativeShellWorkshopParameters_Dirty(Rml::ElementDocument* document,
                                        const NativeFighterProject* project)
{
    if (document == nullptr || project == nullptr) return false;
    const std::string source = editor(document)->GetValue();
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
    std::string values = editor(document)->GetValue();
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
    editor(document)->SetValue(values);
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
    const std::string source = editor(document)->GetValue();
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
