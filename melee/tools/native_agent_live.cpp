#include "native_agent_internal.h"
#include <melee/ft/native_workshop_connection.h>
#include <cstdlib>
#include <cstring>

namespace NativeAgent {
void running_instances(const Context&, yyjson_val*, Reply& reply)
{
    auto* list = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "instances", list);
    for (const auto& instance : NativeWorkshopConnection::instances()) {
        auto* row = yyjson_mut_obj(reply.document);
        add_string(reply, row, "instance", instance);
        yyjson_mut_obj_add_uint(reply.document, row, "process_id", std::strtoul(instance.c_str(), nullptr, 10));
        const std::string arguments = "{\"instance\":\"" + instance + "\"}";
        auto* doc = yyjson_read(arguments.data(), arguments.size(), 0);
        Reply endpoint(reply.document);
        bool available = doc && NativeWorkshopConnection::request(instance, "connection.describe", yyjson_doc_get_root(doc), endpoint, 500);
        yyjson_doc_free(doc);
        yyjson_mut_obj_add_bool(reply.document, row, "available", available);
        if (available) {
            for (const char* field : {"build_identity", "source_revision", "build", "executable"}) {
                const char* value = yyjson_mut_get_str(yyjson_mut_obj_get(endpoint.result, field));
                if (value) add_string(reply, row, field, value);
            }
            const char* identity = yyjson_mut_get_str(yyjson_mut_obj_get(endpoint.result, "build_identity"));
            yyjson_mut_obj_add_bool(reply.document, row, "build_matches", identity && !std::strcmp(identity, NativeAuthoring::build_identity()));
        } else {
            const char* code = yyjson_mut_get_str(yyjson_mut_obj_get(endpoint.result, "code"));
            if (code) add_string(reply, row, "connection_status", code);
        }
        yyjson_mut_arr_add_val(list, row);
    }
    add_string(reply, reply.result, "build_identity", NativeAuthoring::build_identity());
}
void live_command(const char* name, yyjson_val* arguments, Reply& reply)
{
    NativeWorkshopConnection::request(string(arguments, "instance"), name, arguments, reply,
        !std::strcmp(name, "session.scripts.check") || !std::strcmp(name, "session.build") ||
        !std::strcmp(name, "session.install") || !std::strcmp(name, "session.preview.refresh") ||
        !std::strcmp(name, "session.practice.start") || !std::strcmp(name, "session.open") ? 30000 : 10000);
}
}
