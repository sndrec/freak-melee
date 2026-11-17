#include "native_shell_workshop_editor_internal.h"
#include <melee/ft/native_authoring_reply.h>

void nativeShellWorkshopEditor_PresentValidation(NativeAuthoring::Reply& report)
{
    using namespace WorkshopEditor;
    if (!session->project) return;
    if (!report.ok) {
        const char* message = yyjson_mut_get_str(yyjson_mut_obj_get(report.result, "message"));
        set_text("workshop-script-status", message ? message : "Validation could not finish.");
        set_text("workshop-script-problems", message ? message : "Validation could not finish.");
        return;
    }
    if (yyjson_mut_get_bool(yyjson_mut_obj_get(report.result, "accepted"))) {
        set_text("workshop-script-status", "Current source and fighter assets validated.");
        set_text("workshop-script-problems", "No problems found.");
        return;
    }
    std::string markup;
    size_t index, count; yyjson_mut_val* diagnostic;
    auto* diagnostics = yyjson_mut_obj_get(report.result, "diagnostics");
    yyjson_mut_arr_foreach(diagnostics, index, count, diagnostic) {
        const char* path = yyjson_mut_get_str(yyjson_mut_obj_get(diagnostic, "file"));
        const char* message = yyjson_mut_get_str(yyjson_mut_obj_get(diagnostic, "message"));
        unsigned line = (unsigned)yyjson_mut_get_uint(yyjson_mut_obj_get(diagnostic, "line"));
        std::string relative = path ? path : "";
        std::replace(relative.begin(), relative.end(), '\\', '/');
        size_t file = 0;
        for (; file < session->script_files.files.size(); ++file) {
            auto candidate = session->script_files.files[file].path.substr(session->script_files.root.size() + 1);
            std::replace(candidate.begin(), candidate.end(), '\\', '/');
            if (candidate == relative) break;
        }
        std::string label = relative.empty() ? "" : relative + (line ? ':' + number(line) : "") + ": ";
        label += message ? message : "Validation failed.";
        label = Rml::StringUtilities::EncodeRml(label);
        if (line && file < session->script_files.files.size())
            markup += "<button id='workshop-script-problem-" + std::to_string(file) + '-' + number(line) + "'>" + label + "</button>";
        else markup += "<div>" + label + "</div>";
    }
    auto total = yyjson_mut_get_uint(yyjson_mut_obj_get(report.result, "diagnostic_count"));
    set_text("workshop-script-status", std::to_string(total) + (total == 1 ? " problem found." : " problems found."));
    document->GetElementById("workshop-script-problems")->SetInnerRML(markup.empty() ? "Validation did not accept this draft." : markup);
}
