#include "native_fighter_costume_compact.h"
#include "native_asset_dat.h"
#include "native_mesh_vertex_ids.h"
extern "C" {
#include <melee/ft/ftcostume_assets.h>
}
#include <cstdio>
#include <limits>
#include <unordered_set>

namespace {
struct Span { u32 begin, end, destination; bool pinned; u32 type; };
struct Root { u32 offset; HSD_DatImportType type; u32 index; };
void source_span(void* context, u32 offset, size_t size, u32 type)
{
    auto* spans = static_cast<std::vector<Span>*>(context);
    const bool pinned = type == DAT_JOINT || type == DAT_DOBJ || type == DAT_MOBJ ||
                        type == DAT_TOBJ || type == DAT_TEXANIM;
    spans->push_back({ offset, (u32)(offset + size), 0, pinned, type });
}
bool ends(const char* text, const char* suffix)
{
    const size_t n = std::strlen(text), m = std::strlen(suffix);
    return n >= m && std::memcmp(text + n-m, suffix, m) == 0;
}
HSD_DatImportType root_type(const NativeFighterProject& project, const char* name)
{
    if (std::strcmp(name, project.facts.costume_joint) == 0) return DAT_JOINT;
    if ((project.facts.costume_matanim[0] && std::strcmp(name, project.facts.costume_matanim) == 0) ||
        ends(name, "_matanim_joint")) return DAT_MATANIM_JOINT;
    if (ends(name, "_anim_joint")) return DAT_ANIMJOINT;
    if (ends(name, "_shapeanim_joint")) return DAT_SHAPE_JOINT;
    if (ends(name, "_joint")) return DAT_JOINT;
    return (HSD_DatImportType)0;
}
bool trace(const HSD_Dat& dat, const std::vector<Root>& roots,
             std::vector<Span>* spans, char* error, size_t error_size)
{
    std::vector<u8> arena(FT_COSTUME_IMPORT_ARENA_SIZE);
    u32 records = FT_COSTUME_IMPORT_RECORD_CAPACITY;
    HSD_DatImport import;
    for (const auto& root : roots) {
        const size_t before = spans ? spans->size() : 0;
        for (;;) {
            if (spans) spans->resize(before);
            HSD_DatImportInit(&import, &dat, arena.data(), arena.size(), records);
            if (spans) { import.source_range = source_span; import.source_context = spans; }
            if (HSD_DatImportGraph(&import, root.offset, root.type)) break;
            const bool record_limit = import.record_count == records;
            if (import.status == HSD_DAT_CAPACITY &&
                (record_limit || arena.size() < FT_COSTUME_IMPORT_MAX_ARENA_SIZE)) {
                if (record_limit) records *= 2;
                else arena.resize(arena.size() * 2);
                continue;
            }
            if (error && error_size) std::snprintf(error, error_size,
                "Costume graph failed validation (DAT status %u at 0x%lX; records %lu, arena %zu).",
                (unsigned)import.status, (unsigned long)import.error_offset,
                (unsigned long)import.record_count, import.arena_used);
            return false;
        }
    }
    return true;
}
const Span* containing(const std::vector<Span>& groups, u32 offset, size_t size)
{
    auto at = std::upper_bound(groups.begin(), groups.end(), offset,
        [](u32 offset, const Span& group) { return offset < group.begin; });
    if (at == groups.begin()) return nullptr;
    --at;
    return offset >= at->begin && offset < at->end && size <= at->end-offset ? &*at : nullptr;
}
bool mapped(const std::vector<Span>& groups, u32 offset, size_t size, u32* destination)
{
    const auto* group = containing(groups, offset, size);
    if (!group) return false;
    *destination = group->destination + (offset-group->begin);
    return true;
}
}

bool nativeFighterCostume_Compact(const NativeFighterProject* project, std::vector<uint8_t>* bytes,
                                  NativeCostumeCompaction* output, char* error, size_t error_size)
{
    NativeCostumeCompaction stats;
    if (!project || !bytes) return false;
    stats.before = stats.after = bytes->size();
    auto unsupported = [&](const char* reason) {
        std::snprintf(stats.reason, sizeof(stats.reason), "%s", reason);
        if (output) *output = stats;
        if (bytes->size() > NATIVE_CONTENT_MAX_PACKAGE_BYTES) {
            if (error && error_size) std::snprintf(error, error_size,
                "The retained costume exceeds the package limit: %s", reason);
            return false;
        }
        return true;
    };
    HSD_Dat dat;
    if (HSD_DatOpen(&dat, bytes->data(), bytes->size()) != HSD_DAT_OK) return false;
    if (dat.extern_count) return unsupported("The costume has external symbol chains.");
    std::vector<Root> roots;
    u32 identity_index = UINT32_MAX;
    for (u32 i = 0; i < dat.root_count; ++i) {
        u32 offset; const char* name;
        if (!HSD_DatSymbol(&dat, false, i, &offset, &name)) return false;
        if (std::strcmp(name, NATIVE_MESH_VERTEX_IDS_ROOT) == 0) { identity_index = i; continue; }
        const auto type = root_type(*project, name);
        if (!type) {
            char reason[160]; std::snprintf(reason, sizeof(reason), "Unknown costume root: %.128s", name);
            return unsupported(reason);
        }
        roots.push_back({ offset, type, i });
    }
    if (roots.empty()) return false;
    std::vector<Span> spans;
    if (!trace(dat, roots, &spans, error, error_size)) return false;
    std::vector<NativeMeshVertexIdRow> vertex_ids;
    if (!nativeMeshVertexIds_Read(dat, &vertex_ids)) {
        if (error && error_size) std::snprintf(error, error_size, "Invalid mesh vertex identities.");
        return false;
    }
    std::unordered_set<u32> live_polygons;
    for (const auto& span : spans) if (span.type == DAT_POBJ) live_polygons.insert(span.begin);
    vertex_ids.erase(std::remove_if(vertex_ids.begin(), vertex_ids.end(),
        [&](const auto& row) { return !live_polygons.count(row.polygon); }), vertex_ids.end());
    std::sort(spans.begin(), spans.end(), [](const auto& a, const auto& b) {
        return a.begin == b.begin ? a.end < b.end : a.begin < b.begin;
    });
    std::vector<Span> groups;
    for (const auto& span : spans) {
        if (span.begin >= span.end || span.end > dat.data_size) return false;
        // Overlap retains interior aliases and strided arrays as one object.
        // Adjacent independent objects may move separately.
        if (!groups.empty() && span.begin < groups.back().end) {
            groups.back().end = std::max(groups.back().end, span.end);
            groups.back().pinned |= span.pinned;
        } else groups.push_back(span);
    }
    if (groups.empty()) return false;
    // Preserve an existing allocation at zero. New placements start above it:
    // zero is also the null word in DAT descriptors and authoring metadata.
    if (groups.front().begin == 0) groups.front().pinned = true;
    // Every pointer retained in a live range must have a traced target. Legacy
    // fields not read by the importer can point at opaque source data; retain
    // the original archive intact instead of inferring that data's extent.
    for (u32 i = 0; i < dat.relocation_count; ++i) {
        u32 slot, target;
        HSD_DatRelocation(&dat, i, &slot);
        if (!containing(groups, slot, 1)) continue;
        if (!containing(groups, slot, 4) || !HSD_DatReadU32(&dat, slot, &target)) return false;
        if (!containing(groups, target, 1)) {
            char reason[160]; std::snprintf(reason, sizeof(reason),
                "Untraced source data at 0x%lX (reference 0x%lX).", (unsigned long)target, (unsigned long)slot);
            return unsupported(reason);
        }
    }
    std::vector<Span> pins;
    for (const auto& group : groups) if (group.pinned) pins.push_back(group);
    size_t cursor = 1, pin = 0, extent = 0;
    for (auto& group : groups) {
        const size_t size = group.end-group.begin;
        stats.live += size;
        if (group.pinned) {
            group.destination = group.begin; stats.pinned += size;
        } else {
            for (;;) {
                while (pin < pins.size() && cursor >= pins[pin].end) ++pin;
                const size_t limit = pin < pins.size() ? pins[pin].begin : UINT32_MAX;
                size_t candidate = (cursor & ~size_t(31)) | (group.begin & 31);
                if (candidate < cursor) candidate += 32;
                if (candidate <= limit && size <= limit-candidate) {
                    group.destination = (u32)candidate; cursor = candidate + size; break;
                }
                if (pin == pins.size()) return false;
                cursor = pins[pin++].end;
            }
        }
        extent = std::max(extent, size_t(group.destination) + size);
    }
    extent = (extent + 3) & ~size_t(3);
    if (extent > dat.data_size) return false;
    std::vector<u8> data(extent, 0);
    for (const auto& group : groups)
        std::memcpy(data.data() + group.destination, dat.data + group.begin, group.end-group.begin);
    std::vector<u32> relocations;
    for (u32 i = 0; i < dat.relocation_count; ++i) {
        u32 slot, target, new_slot, new_target;
        HSD_DatRelocation(&dat, i, &slot);
        if (!containing(groups, slot, 1)) continue;
        if (!mapped(groups, slot, 4, &new_slot) || !HSD_DatReadU32(&dat, slot, &target) ||
            !mapped(groups, target, 1, &new_target)) return false;
        NativeAssetDat::be32(data.data() + new_slot, new_target);
        relocations.push_back(new_slot);
    }
    u32 identity_header = 0;
    if (identity_index != UINT32_MAX) {
        for (auto& row : vertex_ids) {
            u32 destination;
            if (!mapped(groups, row.polygon, 24, &destination)) return false;
            row.polygon = (uint32_t)destination;
        }
        NativeAssetDat extension;
        extension.data.swap(data);
        identity_header = extension.allocate(8);
        if (identity_header == UINT32_MAX || !nativeMeshVertexIds_Store(extension, identity_header, vertex_ids)) return false;
        stats.live += extension.data.size()-extent;
        extension.data.swap(data);
        relocations.insert(relocations.end(), extension.relocations.begin(), extension.relocations.end());
    }
    std::sort(relocations.begin(), relocations.end());
    const size_t tail_start = 32 + dat.data_size + size_t(dat.relocation_count) * 4;
    const size_t tail_size = bytes->size()-tail_start;
    const size_t total = 32 + data.size() + relocations.size()*4 + tail_size;
    if (total > UINT32_MAX || total > NATIVE_CONTENT_MAX_PACKAGE_BYTES) return false;
    std::vector<u8> result(total);
    std::memcpy(result.data(), bytes->data(), 32);
    NativeAssetDat::be32(result.data(), (u32)total);
    NativeAssetDat::be32(result.data()+4, (u32)data.size());
    NativeAssetDat::be32(result.data()+8, (u32)relocations.size());
    std::memcpy(result.data()+32, data.data(), data.size());
    u8* table = result.data()+32+data.size();
    for (unsigned i = 0; i < relocations.size(); ++i) NativeAssetDat::be32(table+i*4, relocations[i]);
    u8* tail = table+relocations.size()*4;
    std::memcpy(tail, bytes->data()+tail_start, tail_size);
    for (unsigned i = 0; i < roots.size(); ++i) {
        u32 offset;
        if (!mapped(groups, roots[i].offset, 1, &offset)) return false;
        roots[i].offset = offset;
        NativeAssetDat::be32(tail+roots[i].index*8, offset);
    }
    if (identity_index != UINT32_MAX) NativeAssetDat::be32(tail+identity_index*8, identity_header);
    HSD_Dat verified;
    if (HSD_DatOpen(&verified, result.data(), result.size()) != HSD_DAT_OK ||
        !trace(verified, roots, nullptr, error, error_size) ||
        !nativeMeshVertexIds_Read(verified, &vertex_ids)) return false;
    stats.supported = true; stats.after = result.size();
    stats.groups = (unsigned)groups.size(); stats.relocations = (unsigned)relocations.size();
    if (output) for (const auto& group : groups)
        stats.ranges.push_back({ (uint32_t)group.begin, (uint32_t)group.destination,
                                 (uint32_t)(group.end-group.begin) });
    bytes->swap(result);
    if (output) *output = stats;
    return true;
}
