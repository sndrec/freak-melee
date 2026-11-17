#include "native_shell_workshop_editor_internal.h"
#include "native_shell_workshop.h"
#include "native_shell_workshop_model_internal.h"
#include <melee/ft/native_attribute_authoring.h>
#include <cmath>

void nativeShellWorkshopEditor_Attributes(const char* method, yyjson_val* args,
                                         NativeAuthoring::Reply& reply)
{
    using namespace WorkshopEditor;
    if (!session->project) {
        reply.fail("no_draft", "Open a fighter draft first."); return;
    }
    if (!std::strcmp(method, "list")) {
        NativeAttributeAuthoring::list(*session->project, args, reply); return;
    }
    std::vector<uint8_t> before(session->project->fighter_dat,
                                session->project->fighter_dat + session->project->fighter_size);
    if (!NativeAttributeAuthoring::edit(*session->project, args, reply)) return;
    if (yyjson_mut_get_bool(yyjson_mut_obj_get(reply.result, "changed"))) {
        const char* summary = yyjson_get_str(yyjson_obj_get(args, "summary"));
        if (session->mode == WorkshopMode::Model) {
            WorkshopModelInternal::record_asset({}, before);
            if (summary) WorkshopModelInternal::active->changes.back().summary = summary;
            WorkshopModelInternal::buttons();
        } else {
            record_edit(false, before.data());
            session->action_dirty = true;
            if (summary && !session->edit_history.empty()) session->edit_history.back().summary = summary;
            update_history_buttons();
        }
    }
    nativeShellWorkshop_RefreshAttributes();
}

namespace WorkshopEditor {
bool apply_attribute(const Rml::String& id)
{
    const unsigned index = (unsigned) std::strtoul(id.c_str() + 19, nullptr, 10);
    if (!session->project || index >= NATIVE_FIGHTER_COMMON_ATTRIBUTE_COUNT) return false;
    const std::string entered = field(id.c_str())->GetValue();
    char* end;
    const double number = std::strtod(entered.c_str(), &end);
    if (entered.empty() || *end || !std::isfinite(number)) {
        set_text("workshop-editor-status", "Enter a finite attribute value.");
        nativeShellWorkshop_RefreshAttributes(); return true;
    }
    auto* owner = yyjson_mut_doc_new(nullptr);
    if (!owner) return false;
    auto* args = yyjson_mut_obj(owner);
    yyjson_mut_doc_set_root(owner, args);
    auto* changes = yyjson_mut_arr(owner);
    auto* change = yyjson_mut_obj(owner);
    yyjson_mut_obj_add_val(owner, args, "changes", changes);
    yyjson_mut_arr_append(changes, change);
    yyjson_mut_obj_add_str(owner, change, "name", nativeFighterCommonAttributes[index].name);
    yyjson_mut_obj_add_real(owner, change, "value", number);
    yyjson_mut_obj_add_str(owner, args, "summary", "Edit fighter attribute");
    auto* request = yyjson_mut_doc_imut_copy(owner, nullptr);
    NativeAuthoring::Reply reply(owner);
    if (request) nativeShellWorkshopEditor_Attributes("edit", yyjson_doc_get_root(request), reply);
    else reply.fail("allocation_failed", "Could not capture this attribute edit.");
    set_text("workshop-editor-status", reply.ok ? "Attribute updated."
        : "Enter a value within this attribute's type and range.");
    if (!reply.ok) nativeShellWorkshop_RefreshAttributes();
    yyjson_doc_free(request);
    yyjson_mut_doc_free(owner);
    return true;
}
}
