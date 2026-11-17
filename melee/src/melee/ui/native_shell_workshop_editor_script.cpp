#include <melee/ft/native_fighter_template.h>
#include "native_shell_workshop_editor_internal.h"

#ifdef MELEE_NATIVE

namespace WorkshopEditor {

std::string combined_script()
{
    if (session->script_loaded) {
        NativeFighterSources::set_source(session->script_files, session->script_file_index,
            static_cast<Rml::ElementFormControlTextArea*>(
                document->GetElementById("workshop-script-source"))->GetValue());
    }
    return NativeFighterSources::compose(session->script_files);
}


void show_script_files()
{
    std::string markup;
    for (size_t index = 0; index < NativeFighterSources::count(session->script_files); ++index) {
        markup += "<button id='workshop-script-file-" + number((unsigned) index) +
                  "' class='workshop-script-outline-row" +
                  (index == session->script_file_index ? " selected" : "") + "'>" +
                  Rml::StringUtilities::EncodeRml(
                      NativeFighterSources::name(session->script_files, index)) + "</button>";
    }
    document->GetElementById("workshop-script-files")->SetInnerRML(markup);
    set_text("workshop-script-file", session->script_file_index == 0
        ? "fighter_state.fms"
        : NativeFighterSources::name(session->script_files, session->script_file_index) + ".fms");
}

void select_script_file(size_t index)
{
    if (!session->script_loaded || index >= NativeFighterSources::count(session->script_files)) return;
    capture_script_view();
    combined_script();
    session->script_file_index = index;
    static_cast<Rml::ElementFormControlTextArea*>(
        document->GetElementById("workshop-script-source"))->SetValue(
            NativeFighterSources::source(session->script_files, index));
    show_script_files();
    nativeShellWorkshopEditor_RefreshScriptOutline();
    restore_script_view();
}

void show_script()
{
    if (session->script_loaded) return;
    if (!session->project || !NativeFighterSources::open_project(
            session->script_files, session->project_path, *session->project)) {
        set_text("workshop-script-status", session->script_files.error);
        return;
    }
    session->saved_script = NativeFighterSources::compose(session->script_files);
    const bool draft_loaded = session->script_files.origin == NativeFighterSources::Session::Draft;
    const bool template_loaded = session->script_files.origin == NativeFighterSources::Session::Template;
    session->script_files.parameters = nativeShellWorkshopParameters_Text();
    const bool import_valid = NativeFighterSources::import_parameters(session->script_files, *session->project);
    const bool imported_parameters = import_valid && session->script_files.imported_parameters;
    if (imported_parameters)
        nativeShellWorkshopParameters_SetText(document, session->script_files.parameters);
    session->script_file_index = NativeFighterSources::count(session->script_files) > 1 ? 1 : 0;
    Rml::ElementFormControlTextArea* source =
        static_cast<Rml::ElementFormControlTextArea*>(
            document->GetElementById("workshop-script-source"));
    source->SetValue(NativeFighterSources::source(session->script_files, session->script_file_index));
    show_script_files();
    set_text("workshop-script-status", !import_valid
        ? session->script_files.error
        : imported_parameters
        ? "Imported move values copied to this draft. Save and build behavior."
        : NativeFighterSources::loaded_from_files(session->script_files)
        ? "Separate move files and fighter state loaded."
        : draft_loaded
        ? "Saved script draft loaded. Build behavior before preview."
        : template_loaded
            ? "Fighter behavior template loaded. Build behavior before preview."
        : session->project != nullptr && session->project->script_size != 0
            ? "Saved validated source loaded."
            : "No behavior script in this draft yet.");
    session->script_loaded = true;
    nativeShellWorkshopEditor_RefreshScriptOutline();
}

bool run_script_tool(const char* output_path, const std::string& source,
                     bool publish, std::string* diagnostics, unsigned* error_line)
{
    *error_line = 0;diagnostics->clear();
    char executable[MAX_PATH];
    const DWORD path_length = GetModuleFileNameA(nullptr, executable,
                                                 sizeof(executable));
    if (path_length == 0 || path_length >= sizeof(executable)) { *diagnostics = "Could not locate the source validator.";return false;}
    char* basename = std::strrchr(executable, '\\');
    if (basename == nullptr ||
        size_t(basename - executable) + sizeof("\\native_content_tool.exe") >=
            sizeof(executable)) { *diagnostics = "Source validator path is too long.";return false;}
    std::strcpy(basename + 1, "native_content_tool.exe");
    if (GetFileAttributesA(executable) == INVALID_FILE_ATTRIBUTES) {
        *diagnostics = "The source validator is not installed beside Freak Melee.";
        return false;
    }
    char source_path[512], log_path[512];
    const DWORD process_id = GetCurrentProcessId();
    const int source_length = std::snprintf(source_path, sizeof(source_path),
        "%s\\behavior.editor.%lu.fms", output_path, process_id);
    const int log_length = std::snprintf(log_path, sizeof(log_path),
        "%s\\behavior.editor.%lu.log", output_path, process_id);
    if (source_length <= 0 || source_length >= sizeof(source_path) ||
        log_length <= 0 || log_length >= sizeof(log_path)) { *diagnostics = "Project path is too long.";return false;}
    FILE* file = std::fopen(source_path, "wb");
    const bool written = file != nullptr &&
        std::fwrite(source.data(), 1, source.size(), file) == source.size();
    const bool closed = file != nullptr && std::fclose(file) == 0;
    if (!written || !closed) {
        DeleteFileA(source_path);
        *diagnostics = "Could not stage the script source.";
        return false;
    }
    SECURITY_ATTRIBUTES inheritable = { sizeof(inheritable), nullptr, 1 };
    HANDLE log = CreateFileA(log_path, GENERIC_WRITE, FILE_SHARE_READ,
                             &inheritable, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
    if (log == INVALID_HANDLE_VALUE) {
        DeleteFileA(source_path);
        *diagnostics = "Could not create the validation log.";
        return false;
    }
    std::string command = "\"" + std::string(executable) + "\" " +
        (publish ? "script-build" : "source-check") + " \"" + output_path + "\" \"" + source_path + "\"";
    STARTUPINFOA startup = {};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = log;
    startup.hStdError = log;
    PROCESS_INFORMATION child = {};
    const BOOL started = CreateProcessA(executable, command.data(), nullptr,
        nullptr, 1, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child);
    if (started) {
        const DWORD wait = WaitForSingleObject(child.hProcess, 60000);
        if (wait == WAIT_TIMEOUT) {
            TerminateProcess(child.hProcess, 1);
            WaitForSingleObject(child.hProcess, INFINITE);
        }
        CloseHandle(child.hThread);
    }
    DWORD exit_code = 1;
    if (started) {
        GetExitCodeProcess(child.hProcess, &exit_code);
        CloseHandle(child.hProcess);
    }
    CloseHandle(log);
    char captured[4097] = {};
    FILE* result = std::fopen(log_path, "rb");
    if (result != nullptr) {
        const size_t count = std::fread(captured, 1, 4096, result);
        captured[count] = '\0';
        std::fclose(result);
    }
    DeleteFileA(source_path);
    DeleteFileA(log_path);
    if (!started || exit_code != 0) {
        diagnostics->assign(captured[0] != '\0' ? captured : "Source validation failed or timed out.");
        const std::string prefix = std::string(source_path) + ":";
        const size_t error = diagnostics->find(prefix);
        if (error != std::string::npos) {
            unsigned column = 0;int consumed = 0;
            if (std::sscanf(diagnostics->c_str() + error + prefix.size(), "%u:%u: %n", error_line, &column, &consumed) == 2)
                diagnostics->erase(0, error + prefix.size() + (size_t)consumed);
        } else {
            unsigned column = 0;int consumed = 0;
            if (std::sscanf(diagnostics->c_str(), "%u:%u: %n", error_line, &column, &consumed) == 2)
                diagnostics->erase(0, (size_t)consumed);
        }
        return false;
    }
    return true;
}

bool validate_script()
{
    if (session->project == nullptr) return false;
    const std::string source = combined_script();
    if (source.empty() || source.size() > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY) {
        set_text("workshop-script-status", "Enter a script up to " NATIVE_FIGHTER_BEHAVIOR_SOURCE_LIMIT_TEXT ".");
        set_text("workshop-script-problems", "Enter a script up to " NATIVE_FIGHTER_BEHAVIOR_SOURCE_LIMIT_TEXT ".");
        return false;
    }
    const std::string parameter_source = nativeShellWorkshopParameters_Text();
    NativeFighterParameters parameters;unsigned error_line = 0;std::string diagnostics;
    bool valid = nativeFighterParameters_Parse(parameter_source.data(), parameter_source.size(), &parameters, &error_line);
    if (valid) valid = run_script_tool(session->project_path, source, false, &diagnostics, &error_line);
    else { diagnostics = "Invalid fighter parameter on line " + number(error_line) + ".";error_line = 0; }
    size_t error_file = session->script_file_index;
    if (!valid && error_line != 0 && NativeFighterSources::locate_line(
            session->script_files, error_line, &error_file, &error_line)) select_script_file(error_file);
    set_text("workshop-script-status", valid ? "Source and fighter assets validated."
        : NativeFighterSources::name(session->script_files, error_file) + ":" + number(error_line) + ": " + diagnostics);
    if (valid) set_text("workshop-script-problems", "No problems found.");
    else if (error_line != 0) {
        const std::string label = "Line " + number(error_line) + " · " + diagnostics;
        document->GetElementById("workshop-script-problems")->SetInnerRML(
            "<button id='workshop-script-problem-line-" + number(error_line) + "'>" +
            Rml::StringUtilities::EncodeRml(label) + "</button>");
    } else set_text("workshop-script-problems", diagnostics);
    return valid;
}

void jump_to_script_line(unsigned line)
{
    if (line == 0) return;
    Rml::ElementFormControlTextArea* source =
        static_cast<Rml::ElementFormControlTextArea*>(
            document->GetElementById("workshop-script-source"));
    const std::string text = source->GetValue();
    size_t offset = 0;
    for (unsigned current = 1; current < line && offset < text.size();
         ++current) {
        const size_t next = text.find('\n', offset);
        if (next == std::string::npos) return;
        offset = next + 1;
    }
    source->Focus(true);
    source->SetSelectionRange((int) offset, (int) offset);
}


} // namespace WorkshopEditor

using namespace WorkshopEditor;

void nativeShellWorkshopEditor_RefreshScriptOutline()
{
    if (!session->script_loaded || document == nullptr) return;
    const std::string composed = combined_script();
    nativeShellWorkshopParameters_Refresh(document, composed);
    const std::string& state = NativeFighterSources::source(session->script_files, 0);
    std::string variables;
    size_t state_start = 0;
    unsigned state_line = 1;
    while (state_start < state.size()) {
        const size_t end = state.find('\n', state_start);
        const std::string line = state.substr(state_start,
            (end == std::string::npos ? state.size() : end) - state_start);
        if (line.compare(0, 11, "state bool ") == 0 ||
            line.compare(0, 12, "state float ") == 0) {
            variables += "<div class='workshop-script-variable'>"
                "<button id='workshop-script-variable-" +
                number(state_line) + "' class='workshop-script-outline-row'>" +
                Rml::StringUtilities::EncodeRml(line) + "</button>"
                "<button id='workshop-script-variable-remove-" +
                number(state_line) + "' title='Remove this variable'>×</button>"
                "</div>";
        }
        if (end == std::string::npos) break;
        state_start = end + 1;
        ++state_line;
    }
    document->GetElementById("workshop-script-variables")->SetInnerRML(
        variables.empty() ? "<span class='workshop-muted'>No variables</span>" :
                            variables);
    const std::string source =
        static_cast<Rml::ElementFormControlTextArea*>(
            document->GetElementById("workshop-script-source"))->GetValue();
    const unsigned lines = 1u + (unsigned) std::count(source.begin(),
                                                      source.end(), '\n');
    std::string line_numbers;
    for (unsigned line = 1; line <= lines; ++line) {
        if (line != 1) line_numbers += "<br/>";
        line_numbers += number(line);
    }
    document->GetElementById("workshop-script-lines")
        ->SetInnerRML(line_numbers);
    std::string markup;
    size_t start = 0;
    unsigned line_number = 1;
    while (start < source.size()) {
        const size_t end = source.find('\n', start);
        size_t length = (end == std::string::npos ? source.size() : end) - start;
        if (length != 0 && source[start + length - 1] == '\r') --length;
        const std::string line = source.substr(start, length);
        const bool move = line.size() > 6 &&
            line.compare(0, 5, "move ") == 0 && line.back() == ':';
        const bool handler = line.size() > 8 &&
            line.compare(0, 7, "    on ") == 0 && line.back() == ':';
        if (move || handler) {
            const std::string title = move
                ? line.substr(5, line.size() - 6)
                : line.substr(7, line.size() - 8);
            markup += "<button id='workshop-script-line-" +
                      number(line_number) +
                      "' class='workshop-script-outline-row" +
                      (handler ? " handler" : "") + "'>" +
                      Rml::StringUtilities::EncodeRml(
                          move ? title : "on " + title) + "</button>";
        }
        if (end == std::string::npos) break;
        start = end + 1;
        ++line_number;
    }
    document->GetElementById("workshop-script-outline")->SetInnerRML(
        markup.empty() ? "<span class='workshop-muted'>No move blocks yet.</span>"
                       : markup);
    set_text("workshop-script-context",
             std::string(nativeShellWorkshopEditor_ScriptChanged(selected_member)
                 ? "Unvalidated draft" : "Validated source") +
             " · Original commands and hitboxes are edited in Action Editor.");
}

bool nativeShellWorkshopEditor_PreviewSource(unsigned member, std::string* source,
                                             std::string* parameters)
{
    if (member >= 2 || source == nullptr || parameters == nullptr ||
        sessions[member].project == nullptr || !sessions[member].script_loaded)
        return false;
    if (member == selected_member) {
        *source = combined_script();
        *parameters = nativeShellWorkshopParameters_Text();
    } else {
        *source = NativeFighterSources::compose(sessions[member].script_files);
        *parameters = sessions[member].parameter_source;
    }
    return !source->empty() && source->size() <= NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY;
}

bool nativeShellWorkshopEditor_BuildScript(const char* output_path)
{
    if (session->project == nullptr || output_path == nullptr) return false;
    if (!nativeShellWorkshopParameters_Save(document, session->project, output_path)) return false;
    const std::string source = combined_script();
    if (source.empty() || source.size() > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY) {
        set_text("workshop-script-status", "Enter a script up to " NATIVE_FIGHTER_BEHAVIOR_SOURCE_LIMIT_TEXT ".");return false;
    }
    std::string diagnostics;unsigned line = 0;
    set_text("workshop-script-status", "Validating source and fighter assets...");
    if (!run_script_tool(output_path, source, true, &diagnostics, &line)) {
        size_t file = session->script_file_index;
        if (line != 0 && NativeFighterSources::locate_line(session->script_files, line, &file, &line)) select_script_file(file);
        const std::string message = diagnostics.empty() ? "Could not validate this source candidate." : diagnostics;
        set_text("workshop-script-status", line == 0 ? message : NativeFighterSources::name(session->script_files, file) + ":" + number(line) + ": " + message);
        if (line == 0) set_text("workshop-script-problems", message);
        else document->GetElementById("workshop-script-problems")->SetInnerRML(
            "<button id='workshop-script-problem-line-" + number(line) + "'>" + Rml::StringUtilities::EncodeRml(message) + "</button>");
        return false;
    }
    char draft_path[512];
    const int draft_length = std::snprintf(draft_path, sizeof(draft_path), "%s\\behavior.draft.fms", output_path);
    if (draft_length > 0 && draft_length < sizeof(draft_path)) DeleteFileA(draft_path);
    set_text("workshop-script-status", "Source validated and saved to this draft. Apply a preview to test it.");
    set_text("workshop-script-problems", "No problems found.");return true;
}

#endif
