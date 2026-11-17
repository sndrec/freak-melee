#include "native_agent_project.h"
#include <melee/ft/native_fighter_parameters.h>
#include <cstring>

namespace NativeAgent {
void parameter_source(const Context&, yyjson_val* arguments, Reply& reply)
{
    auto project = open_project(arguments, reply);
    NativeFighterSources::Session files;
    if (!project || !load_sources(*project, files, reply)) return;
    if (!source_status(files, project->family.members[project->member], reply)) return;
    add_string(reply, reply.result, "text", files.parameters);
    NativeFighterParameters parameters;
    unsigned line = 0;
    bool valid = nativeFighterParameters_Parse(files.parameters.data(), files.parameters.size(), &parameters, &line);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "valid", valid);
    auto* fields = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "fields", fields);
    if (!valid) {
        yyjson_mut_obj_add_uint(reply.document, reply.result, "error_line", line);
        return;
    }
    for (unsigned i = 0; i < parameters.count; ++i) {
        const auto& parameter = parameters.fields[i];
        auto* object = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_add_val(fields, object);
        add_string(reply, object, "name", parameter.name);
        const char* types[] = {"float", "integer", "boolean"};
        add_string(reply, object, "type", types[parameter.type]);
        if (parameter.type == NativeFighterParameter_Float) {
            float value;
            std::memcpy(&value, &parameter.bits, sizeof(value));
            yyjson_mut_obj_add_real(reply.document, object, "value", value);
        } else if (parameter.type == NativeFighterParameter_Int)
            yyjson_mut_obj_add_int(reply.document, object, "value", (int32_t)parameter.bits);
        else yyjson_mut_obj_add_bool(reply.document, object, "value", parameter.bits != 0);
    }
}
}
