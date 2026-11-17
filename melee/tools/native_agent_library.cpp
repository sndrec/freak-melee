#include "native_agent_internal.h"
#include <melee/ft/native_fighter_draft_creation.h>
#include <melee/ft/native_fighter_template.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace NativeAgent {
namespace {
void page(Reply& r, size_t total, unsigned offset, unsigned limit)
{
    yyjson_mut_obj_add_uint(r.document, r.result, "total", total);
    if (offset + limit < total) yyjson_mut_obj_add_uint(r.document, r.result, "next_offset", offset + limit);
    else yyjson_mut_obj_add_null(r.document, r.result, "next_offset");
}
}
void stock_fighters(const Context&, yyjson_val* args, Reply& reply)
{
    auto* rows = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "fighters", rows);
    size_t total = 0;
    for (unsigned i = 0; i < Ft_Kind_StockCount; ++i) {
        NativeFighterStockLayout layout;
        const auto kind = (FighterKind)i;
        if (!nativeFighterDraft_StockLayout(kind, &layout)) continue;
        const char* name = nativeFighterProject_StockBaseName(kind);
        if (!matches(name, string(args, "query"))) continue;
        if (total++ < page_offset(args) || yyjson_mut_arr_size(rows) >= page_limit(args)) continue;
        auto* row = yyjson_mut_obj(reply.document); yyjson_mut_arr_append(rows, row);
        yyjson_mut_obj_add_uint(reply.document, row, "fighter_id", i);
        add_string(reply, row, "name", name);
        yyjson_mut_obj_add_uint(reply.document, row, "member_count", layout.partner == Ft_Kind_None ? 1 : 2);
        add_string(reply, row, "primary", nativeFighterProject_StockBaseName(layout.primary));
        if (layout.partner != Ft_Kind_None) add_string(reply, row, "partner", nativeFighterProject_StockBaseName(layout.partner));
        const char* modes[] = {"single", "alternate", "companion"};
        add_string(reply, row, "mode", modes[layout.mode]);
        char* source = nullptr; size_t size = 0;
        bool available = nativeFighterTemplate_Read(layout.primary, &source, &size);
        std::free(source);
        if (available && layout.partner != Ft_Kind_None) {
            source = nullptr;
            available = nativeFighterTemplate_Read(layout.partner, &source, &size);
            std::free(source);
        }
        yyjson_mut_obj_add_bool(reply.document, row, "template_available", available);
    }
    page(reply, total, page_offset(args), page_limit(args));
}
void installed_fighters(const Context& context, yyjson_val* args, Reply& reply)
{
    const std::string root = *string(args, "content_root") ? string(args, "content_root") : context.content_root;
    std::vector<NativeContentId> ids(256);
    size_t count;
    bool ok;
    while (!(ok = nativeContentStore_ListInstalled(root.c_str(), ids.data(), ids.size(), &count)) && ids.size() < 65536) ids.resize(ids.size() * 2);
    if (!ok) { reply.fail("library_unavailable", "Could not enumerate the installed library within its 65536-package inspection limit."); return; }
    ids.resize(count);
    std::sort(ids.begin(), ids.end(), [](const auto& a, const auto& b) { return std::memcmp(&a, &b, sizeof(a)) < 0; });
    auto* rows = yyjson_mut_arr(reply.document);
    yyjson_mut_obj_add_val(reply.document, reply.result, "fighters", rows);
    size_t total = 0;
    for (const auto& id : ids) {
        uint8_t* bytes = nullptr; size_t size = 0;
        if (!nativeContentStore_Read(root.c_str(), &id, &bytes, &size)) {
            reply.fail("package_unavailable", "An installed package could not be read. Retry after the library operation finishes."); return;
        }
        NativeFighterPackage meta; NativeFighterPackageData data;
        bool fighter = nativeContentStore_ParseFighter(bytes, size, &meta, &data);
        char hex[65]; nativeContentStore_IdHex(&id, hex);
        if (fighter && (matches(meta.title, string(args, "query")) || matches(hex, string(args, "query")))) {
            if (total++ >= page_offset(args) && yyjson_mut_arr_size(rows) < page_limit(args)) {
                auto* row = yyjson_mut_obj(reply.document); yyjson_mut_arr_append(rows, row);
                add_string(reply, row, "package_id", hex);
                add_string(reply, row, "title", meta.title);
                add_string(reply, row, "tile_name", meta.tile_name);
                yyjson_mut_obj_add_uint(reply.document, row, "member_count", meta.member_count);
            }
        }
        std::free(bytes);
    }
    add_string(reply, reply.result, "content_root", root);
    page(reply, total, page_offset(args), page_limit(args));
}
}
