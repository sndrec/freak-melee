#include "native_agent_internal.h"
#include <melee/ft/native_fighter_behavior_source.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>

namespace NativeAgent {
std::vector<std::string> example_names(const Context& context)
{
    std::vector<std::string> result;
    WIN32_FIND_DATAA file;
    HANDLE search = FindFirstFileA((context.installation + "\\ui\\*.fms").c_str(), &file);
    if (search == INVALID_HANDLE_VALUE) return result;
    do {
        if (!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) result.emplace_back(file.cFileName);
    } while (FindNextFileA(search, &file));
    FindClose(search);
    std::sort(result.begin(), result.end());
    return result;
}

bool read_example(const Context& context, const std::string& name, std::string& text)
{
    for (const auto& candidate : example_names(context))
        if (candidate == name)
            return read_text(context.installation + "\\ui\\" + candidate, text, NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY);
    return false;
}

void examples(const Context& context, yyjson_val* arguments, Reply& reply)
{
    auto* array = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "examples", array);
    unsigned offset = page_offset(arguments), limit = page_limit(arguments);
    size_t matching = 0;
    for (const auto& name : example_names(context)) {
        if (!matches(name, string(arguments, "query"))) continue;
        if (matching++ < offset || yyjson_mut_arr_size(array) >= limit) continue;
        auto* object = yyjson_mut_obj(reply.document);
        yyjson_mut_arr_add_val(array, object);
        add_string(reply, object, "example", name);
        add_string(reply, object, "uri", "freak-melee://examples/" + name);
    }
    yyjson_mut_obj_add_uint(reply.document, reply.result, "total", matching);
    if (matching > (size_t)offset + limit)
        yyjson_mut_obj_add_uint(reply.document, reply.result, "next_offset", offset + limit);
    else yyjson_mut_obj_add_null(reply.document, reply.result, "next_offset");
}

void example(const Context& context, yyjson_val* arguments, Reply& reply)
{
    const std::string name(string(arguments, "example"));
    std::string text;
    if (!read_example(context, name, text)) {
        reply.fail("unknown_example", "Choose a bundled source example returned by examples.list."); return;
    }
    add_string(reply, reply.result, "example", name);
    add_string(reply, reply.result, "uri", "freak-melee://examples/" + name);
    add_string(reply, reply.result, "text", text);
}
}
