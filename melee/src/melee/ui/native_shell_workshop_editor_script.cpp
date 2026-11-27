#include "native_shell_workshop_editor_internal.h"
#include <melee/ft/native_script_source.h>
#include <melee/ft/native_source_bank.h>

#include <cctype>

#ifdef MELEE_NATIVE

namespace WorkshopEditor {

static bool updating_execute_options;

static bool execute_functions(std::vector<std::string>& functions)
{
    show_script();
    if (!session->script_loaded) return false;
    const std::string source = combined_script();
    if (source.size() > NATIVE_SOURCE_SOURCE_BYTES) return false;
    std::vector<NativeScriptSourceNode> nodes(NATIVE_SOURCE_SYNTAX_NODES);
    NativeScriptSourceTree tree = { source.data(), (uint32_t)source.size(), nodes.data(),
                                   0, (uint32_t)nodes.size() };
    NativeScriptDiagnostic diagnostic{};
    if (!nativeScriptSource_Parse(&tree, &diagnostic)) return false;
    for (unsigned i = 1; i < tree.count; ++i) {
        const auto& node = nodes[i];
        if (node.kind != NativeScriptSource_Function || node.name.length > FT_ACTION_SCRIPT_NAME_MAX)
            continue;
        unsigned owner = node.parent;
        while (owner != UINT32_MAX && nodes[owner].kind != NativeScriptSource_Article &&
               nodes[owner].kind != NativeScriptSource_Item && nodes[owner].kind != NativeScriptSource_Effect)
            owner = nodes[owner].parent;
        if (owner != UINT32_MAX || node.value.length < 2) continue;
        const char* signature = source.data() + node.value.offset;
        unsigned next = 1;
        while (next < node.value.length && std::isspace((unsigned char)signature[next])) ++next;
        if (next < node.value.length && signature[next] == ')')
            functions.push_back(source.substr(node.name.offset, node.name.length));
    }
    std::sort(functions.begin(), functions.end());
    functions.erase(std::unique(functions.begin(), functions.end()), functions.end());
    return true;
}

void show_execute_script_picker()
{
    std::vector<std::string> functions;
    const bool parsed = execute_functions(functions);
    std::string markup = "<button id='workshop-event-categories'>‹ Categories</button>"
                         "<div class='event-category'>Execute Script</div>";
    if (!parsed)
        markup += "<div class='workshop-muted'>Fix the script syntax to list functions.</div>";
    else if (functions.empty())
        markup += "<div class='workshop-muted'>Add a fighter function that takes no arguments in Scripting.</div>";
    for (const auto& function : functions)
        markup += "<button id='workshop-execute-create-" + function + "'>" + function + "</button>";
    auto* menu = document->GetElementById("workshop-add-event-menu");
    menu->SetInnerRML(markup);
    menu->SetProperty("display", "block");
    menu->SetScrollTop(0);
}

void show_execute_script_fields(uint32_t source)
{
    char name[FT_ACTION_SCRIPT_NAME_MAX + 1];
    if (!nativeFighterAction_GetScript(session->project, (unsigned)session->selected_action, source, name))
        return;
    std::vector<std::string> functions;
    const bool parsed = execute_functions(functions);
    const bool missing = std::find(functions.begin(), functions.end(), name) == functions.end();
    std::string markup;
    if (missing) markup = std::string("<option value='") + name + "' selected='selected'>" + name + "</option>";
    for (const auto& function : functions)
        markup += "<option value='" + function + "'" +
            (function == name ? " selected='selected'" : "") + ">" + function + "</option>";
    updating_execute_options = true;
    document->GetElementById("workshop-execute-function")->SetInnerRML(markup);
    updating_execute_options = false;
    document->GetElementById("workshop-execute-script-fields")->SetProperty("display", "block");
    set_text("workshop-action-detail", !parsed ? "Fix the script syntax to list functions." :
        missing ? "This function is missing or needs arguments." : "Runs this function at the event’s frame.");
}

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

void show_action_script(int motion, const std::string& action)
{
    if (!session->script_loaded) return;
    combined_script();
    if (motion < 0) {
        set_text("workshop-script-status", action + " · No motion found in the live preview.");
        return;
    }
    const std::string handler = "on state_" + number((unsigned) motion) + "_";
    for (size_t index = 0; index < NativeFighterSources::count(session->script_files); ++index) {
        const std::string source = NativeFighterSources::source(session->script_files, index);
        const size_t offset = source.find("\n    " + handler);
        if (offset == std::string::npos) continue;
        select_script_file(index);
        jump_to_script_line((unsigned) std::count(source.begin(), source.begin() + offset + 1, '\n') + 1);
        set_text("workshop-script-status", action + " · Motion " + number((unsigned) motion));
        return;
    }
    set_text("workshop-script-status", action + " · Motion " + number((unsigned) motion) +
        ". For owned motions, an action block can define " + handler + "animation:. See Language → Common motion policies.");
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

void nativeShellWorkshopEditor_ApplyExecuteFunction(const char* function)
{
    using namespace WorkshopEditor;
    if (updating_execute_options || !*function || !session->project ||
        session->selected_action < 0 || session->selected_event < 0 ||
        (size_t) session->selected_event >= session->event_count) return;
    const auto& event = session->events[session->selected_event];
    if (event.kind != NativeFighterActionEvent_ExecuteScript) return;
    char name[FT_ACTION_SCRIPT_NAME_MAX + 1];
    if (!nativeFighterAction_GetScript(session->project, (unsigned)session->selected_action,
                                      event.source_offset, name) || !std::strcmp(name, function)) return;

    auto* doc = yyjson_mut_doc_new(nullptr);
    if (!doc) return;
    auto* args = yyjson_mut_obj(doc);
    auto* properties = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, args);
    char event_id[24];
    std::snprintf(event_id, sizeof(event_id), "event-%08x", event.source_offset);
    yyjson_mut_obj_add_uint(doc, args, "action_id", (unsigned)session->selected_action);
    yyjson_mut_obj_add_str(doc, args, "operation", "update");
    yyjson_mut_obj_add_strcpy(doc, args, "event_id", event_id);
    yyjson_mut_obj_add_str(doc, args, "summary", "Change script function");
    yyjson_mut_obj_add_strcpy(doc, properties, "function", function);
    yyjson_mut_obj_add_val(doc, args, "properties", properties);
    auto* input = yyjson_mut_doc_imut_copy(doc, nullptr);
    NativeAuthoring::Reply reply(doc);
    if (input) nativeShellWorkshopEditor_Actions("edit", yyjson_doc_get_root(input), reply);
    if (!reply.ok) {
        const char* message = yyjson_mut_get_str(yyjson_mut_obj_get(reply.result, "message"));
        set_text("workshop-editor-status", message ? message : "Could not change the script function.");
        show_event();
    }
    yyjson_doc_free(input);
    yyjson_mut_doc_free(doc);
}

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

#endif
