#include "native_agent_tool.h"
#include "native_agent_internal.h"
#include <melee/gm/native_content_store.h>
#include <melee/ft/native_fighter_family_project.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace NativeAgent {
Reply::Reply(yyjson_mut_doc* owner) : document(owner), result(yyjson_mut_obj(owner)) {}

void Reply::fail(const char* code, const std::string& message)
{
    ok = false;
    result = yyjson_mut_obj(document);
    yyjson_mut_obj_add_strcpy(document, result, "code", code);
    yyjson_mut_obj_add_strcpy(document, result, "message", message.c_str());
}

void add_string(Reply& reply, yyjson_mut_val* object, const char* key, const std::string& value)
{
    yyjson_mut_obj_add_strcpy(reply.document, object, key, value.c_str());
}

const char* string(yyjson_val* args, const char* key)
{
    auto* field = yyjson_obj_get(args, key);
    const char* value = yyjson_get_str(field);
    return value && std::strlen(value) == yyjson_get_len(field) ? value : "";
}

unsigned page_offset(yyjson_val* args)
{
    return (unsigned)yyjson_get_uint(yyjson_obj_get(args, "offset"));
}

unsigned page_limit(yyjson_val* args)
{
    auto* value = yyjson_obj_get(args, "limit");
    return value ? (unsigned)yyjson_get_uint(value) : 50;
}

bool read_text(const std::string& path, std::string& text, size_t maximum)
{
    FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return false;
    long bytes;
    bool ok = !std::fseek(file, 0, SEEK_END) && (bytes = std::ftell(file)) >= 0 &&
        (size_t)bytes <= maximum && !std::fseek(file, 0, SEEK_SET);
    if (ok) {
        text.resize((size_t)bytes);
        ok = std::fread(text.data(), 1, text.size(), file) == text.size() &&
            text.find('\0') == std::string::npos;
    }
    ok = std::fclose(file) == 0 && ok;
    if (!ok) text.clear();
    return ok;
}

bool matches(const std::string& text, const std::string& query)
{
    return std::search(text.begin(), text.end(), query.begin(), query.end(),
        [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); }) != text.end();
}

bool initialize(Context& context, std::string& error)
{
    char executable[32768], content[MAX_PATH], projects[512];
    DWORD size = GetModuleFileNameA(nullptr, executable, sizeof(executable));
    if (!size || size >= sizeof(executable) ||
        !nativeContentStore_DefaultRoot(content, sizeof(content)) ||
        !nativeFighterFamilyProject_LocalRoot(projects, sizeof(projects))) {
        error = "Could not locate the installation or local authoring folders.";
        return false;
    }
    char* end = std::strrchr(executable, '\\');
    if (!end) { error = "Could not locate the authoring executable's folder."; return false; }
    *end = 0;
    context = {executable, content, projects};
    return true;
}

std::string serialize(yyjson_mut_doc* document)
{
    size_t size = 0;
    char* text = yyjson_mut_write(document, 0, &size);
    if (!text) return {};
    std::string result(text, size);
    std::free(text);
    return result;
}

static void print(yyjson_mut_doc* document)
{
    std::string text = serialize(document);
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fputc('\n', stdout);
    std::fflush(stdout);
}
}

int nativeAgentTool_Main(int argc, char** argv)
{
    using namespace NativeAgent;
    Context context;
    std::string error;
    if (!initialize(context, error)) { std::fprintf(stderr, "%s\n", error.c_str()); return 1; }
    if (argc == 1 && !std::strcmp(argv[0], "mcp")) return mcp(context);
    if (argc < 2 || std::strcmp(argv[0], "agent")) return 2;
    yyjson_mut_doc* document = yyjson_mut_doc_new(nullptr);
    if (!document) return 1;
    int code = 0;
    if (argc == 2 && !std::strcmp(argv[1], "list")) {
        yyjson_mut_doc_set_root(document, list_tools(document));
    } else if ((argc == 3 || argc == 4) && !std::strcmp(argv[1], "call")) {
        const char* json = argc == 4 ? argv[3] : "{}";
        auto* input = yyjson_read(json, std::strlen(json), YYJSON_READ_NOFLAG);
        Reply reply(document);
        if (!input) reply.fail("invalid_json", "Arguments must be one JSON object.");
        else call(context, argv[2], yyjson_doc_get_root(input), reply);
        auto* root = yyjson_mut_obj(document);
        yyjson_mut_obj_add_bool(document, root, "ok", reply.ok);
        yyjson_mut_obj_add_val(document, root, reply.ok ? "result" : "error", reply.result);
        yyjson_mut_doc_set_root(document, root);
        code = reply.ok ? 0 : 1;
        yyjson_doc_free(input);
    } else {
        std::fputs("Usage: native_content_tool agent list | agent call TOOL [JSON] | mcp\n", stderr);
        code = 2;
    }
    if (yyjson_mut_doc_get_root(document)) print(document);
    yyjson_mut_doc_free(document);
    return code;
}
