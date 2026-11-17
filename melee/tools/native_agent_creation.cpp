#include "native_agent_internal.h"
#include <melee/ft/native_fighter_draft_creation.h>
#include <melee/ft/native_fighter_draft_access.h>
#include <aurora/dvd.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace NativeAgent {
namespace {
std::string destination(const Context& context, yyjson_val* args)
{
    const char* supplied = string(args, "output");
    if (*supplied) return supplied;
    unsigned char random[16]; char hex[33];
    if (BCryptGenRandom(nullptr, random, sizeof(random), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) return {};
    for (unsigned i = 0; i < sizeof(random); ++i) std::snprintf(hex + i * 2, 3, "%02x", random[i]);
    return context.projects_root + "\\fighter-" + hex;
}
void created(const std::string& path, Reply& reply)
{
    NativeFighterFamilyProjectInfo info{};
    std::string revision, error;
    NativeDraftAccess::Lease lease;
    if (!lease.acquire(path.c_str(), error) || !nativeFighterFamilyProject_ReadInfo(path.c_str(), &info) ||
        !NativeDraftAccess::revision(path, revision, error)) {
        reply.fail("created_draft_unavailable", "Draft creation completed but its inspection failed: " + path + ". " + error); return;
    }
    add_string(reply, reply.result, "project", info.paths[0]);
    add_string(reply, reply.result, "title", info.members[0].title);
    add_string(reply, reply.result, "revision", revision);
    add_string(reply, reply.result, "agent_guide", std::string(info.paths[0]) + "\\AGENTS.md");
    add_string(reply, reply.result, "status", "created_draft");
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
    auto* members = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "members", members);
    for (unsigned i = 0; i < info.member_count; ++i) {
        auto* member = yyjson_mut_obj(reply.document); yyjson_mut_arr_append(members, member);
        add_string(reply, member, "key", info.keys[i]);
        add_string(reply, member, "path", info.paths[i]);
    }
}
}
void clone_stock(const Context& context, yyjson_val* args, Reply& reply)
{
    const auto kind = (FighterKind) yyjson_get_uint(yyjson_obj_get(args, "fighter_id"));
    NativeFighterStockLayout layout;
    if (!nativeFighterDraft_StockLayout(kind, &layout)) {
        reply.fail("unsupported_stock_fighter", "Choose a fighter_id returned by stock.list."); return;
    }
    const auto output = destination(context, args);
    if (output.empty()) { reply.fail("path_unavailable", "Could not choose a local draft path."); return; }
    std::string iso = string(args, "iso");
    if (iso.empty()) {
        const auto user = context.projects_root.substr(0, context.projects_root.find_last_of("\\/"));
        read_text(user + "\\iso-path.txt", iso, 4096);
        while (!iso.empty() && (iso.back() == '\n' || iso.back() == '\r')) iso.pop_back();
    }
    if (iso.empty() || !aurora_dvd_open(iso.c_str())) {
        reply.fail("iso_unavailable", "Choose a readable Melee ISO or configure one in the client."); return;
    }
    char error[1024];
    const bool ok = nativeFighterDraft_CreateStock(output.c_str(), string(args, "title"), kind, error, sizeof(error));
    aurora_dvd_close();
    if (!ok) { reply.fail("clone_failed", error); return; }
    created(output, reply);
}
void clone_installed(const Context& context, yyjson_val* args, Reply& reply)
{
    const std::string root = *string(args, "content_root") ? string(args, "content_root") : context.content_root;
    NativeContentId id;
    uint8_t* bytes = nullptr; size_t size = 0;
    if (!nativeContentStore_IdFromHex(string(args, "package_id"), &id) ||
        !nativeContentStore_IsInstalled(root.c_str(), &id) ||
        !nativeContentStore_Read(root.c_str(), &id, &bytes, &size)) {
        reply.fail("installed_fighter_unavailable", "Choose an installed fighter package_id from fighters.installed."); return;
    }
    const auto output = destination(context, args);
    char error[1024];
    bool ok = !output.empty() && nativeFighterDraft_CreateFromPackage(output.c_str(), string(args, "title"), bytes, size, error, sizeof(error));
    std::free(bytes);
    if (!ok) { reply.fail("clone_failed", output.empty() ? "Could not choose a local draft path." : error); return; }
    created(output, reply);
}
void import_fighter(const Context& context, yyjson_val* args, Reply& reply)
{
    FILE* file = std::fopen(string(args, "package"), "rb");
    long length = 0;
    bool ok = file && !std::fseek(file, 0, SEEK_END) && (length = std::ftell(file)) > 0 &&
        (size_t)length <= NATIVE_CONTENT_MAX_PACKAGE_BYTES && !std::fseek(file, 0, SEEK_SET);
    std::vector<uint8_t> bytes(ok ? (size_t)length : 0);
    if (ok) ok = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    if (file) ok = std::fclose(file) == 0 && ok;
    NativeFighterPackage metadata; NativeFighterPackageData data;
    if (!ok || !nativeContentStore_ParseFighter(bytes.data(), bytes.size(), &metadata, &data)) {
        reply.fail("invalid_fighter_package", "Choose a readable complete fighter .fmc package."); return;
    }
    const char* title = *string(args, "title") ? string(args, "title") : metadata.title;
    const auto output = destination(context, args);
    char error[1024];
    ok = !output.empty() && nativeFighterDraft_CreateFromPackage(output.c_str(), title, bytes.data(), bytes.size(), error, sizeof(error));
    if (!ok) { reply.fail("import_failed", output.empty() ? "Could not choose a local draft path." : error); return; }
    created(output, reply);
}
void rename_project(const Context&, yyjson_val* args, Reply& reply)
{
    NativeFighterRenameDetails details;
    const auto result = nativeFighterProject_Rename(string(args, "project"), string(args, "title"), string(args, "revision"), &details);
    const char* codes[] = {"ok", "draft_busy", "revision_conflict", "invalid_title", "rename_failed"};
    if (result != NativeFighterRename_Success) { reply.fail(codes[result], details.error); return; }
    const std::string revision = details.revision;
    add_string(reply, reply.result, "revision", revision);
    add_string(reply, reply.result, "title", string(args, "title"));
    add_string(reply, reply.result, "project", string(args, "project"));
    add_string(reply, reply.result, "status", "saved_draft");
    yyjson_mut_obj_add_bool(reply.document, reply.result, "changed", revision != string(args, "revision"));
    yyjson_mut_obj_add_bool(reply.document, reply.result, "saved", true);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "installed", false);
}
}
