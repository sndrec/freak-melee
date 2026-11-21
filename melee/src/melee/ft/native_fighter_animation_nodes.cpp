#include "native_fighter_animation_nodes.h"
#include "native_asset_dat.h"
#include <melee/lb/lbanim_dat.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
constexpr unsigned max_nodes = 512, max_tracks = 4096, max_roots = 1024;
struct Span { uint32_t source, size, destination; };
struct Root { uint32_t source, destination; const char* name; unsigned map; };

bool fail(char* error, size_t capacity, const char* message)
{
    if (error && capacity) std::snprintf(error, capacity, "%s", message);
    return false;
}
uint32_t allocate(std::vector<uint8_t>& data, size_t size)
{
    const size_t at = (data.size() + 3) & ~size_t(3);
    if (at > NATIVE_CONTENT_MAX_ASSET_BYTES ||
        size > NATIVE_CONTENT_MAX_ASSET_BYTES - at) return UINT32_MAX;
    data.resize(at + size, 0);
    return (uint32_t)at;
}
void pointer(std::vector<uint8_t>& data, std::vector<uint32_t>& relocations,
             uint32_t slot, uint32_t target)
{
    NativeAssetDat::be32(data.data() + slot, target);
    relocations.push_back(slot); // Includes the valid local target zero.
}

bool remap(const HSD_Dat& dat, uint32_t source, unsigned old_count,
           const uint16_t* order, unsigned count, std::vector<uint8_t>& data,
           std::vector<uint32_t>& relocations, std::vector<Span>& streams,
           uint32_t* result)
{
    FigaTree tree;
    std::vector<FigaTrack> tracks(max_tracks);
    u32 nodes;
    if (lbAnim_DecodeFigaTree(&dat, source, &tree, tracks.data(), max_tracks,
                             &nodes) != HSD_DAT_OK || nodes != old_count)
        return false;
    unsigned starts[max_nodes + 1] = {};
    for (unsigned i = 0; i < old_count; ++i)
        starts[i + 1] = starts[i] + (uint8_t)tree.nodes[i];
    unsigned kept_tracks = 0;
    for (unsigned i = 0; i < count; ++i)
        if (order[i] != UINT16_MAX)
            kept_tracks += (uint8_t)tree.nodes[order[i]];
    const uint32_t root = allocate(data, 20);
    const uint32_t counts = allocate(data, count + 1);
    // A zero-track tree still requires a valid local tracks pointer.
    const uint32_t table = allocate(data, std::max(1u, kept_tracks * 12));
    if (root == UINT32_MAX || counts == UINT32_MAX || table == UINT32_MAX)
        return false;
    std::memcpy(data.data() + root, dat.data + source, 12);
    pointer(data, relocations, root + 12, counts);
    pointer(data, relocations, root + 16, table);
    data[counts + count] = 255;
    u32 old_table;
    const char* external;
    if (HSD_DatResolve(&dat, source + 16, &old_table, &external) != HSD_DAT_REF_LOCAL)
        return false;
    unsigned target_track = 0;
    for (unsigned j = 0; j < count; ++j) {
        const unsigned old = order[j];
        if (old == UINT16_MAX) continue;
        const unsigned n = (uint8_t)tree.nodes[old];
        data[counts + j] = (uint8_t)n;
        for (unsigned t = starts[old]; t < starts[old + 1]; ++t) {
            const auto& track = tracks[t];
            const uint32_t from = (uint32_t)(track.ad_head - dat.data);
            auto span = std::find_if(streams.begin(), streams.end(),
                [&](const Span& s) { return s.source == from && s.size == track.length; });
            uint32_t stream;
            if (span == streams.end()) {
                stream = allocate(data, std::max(1u, (unsigned)track.length));
                if (stream == UINT32_MAX) return false;
                std::memcpy(data.data() + stream, track.ad_head, track.length);
                streams.push_back({from, track.length, stream});
            } else stream = span->destination;
            const uint32_t at = table + target_track++ * 12;
            std::memcpy(data.data() + at, dat.data + old_table + t * 12, 8);
            pointer(data, relocations, at + 8, stream);
        }
    }
    *result = root;
    return true;
}
}

extern "C" bool nativeFighterAnimation_RemapNodes(
    const void* source, size_t source_size,
    const NativeFighterAnimationNodeMap* maps, unsigned map_count, void** output,
    size_t* output_size, char* error, size_t error_size)
{
    if (!source || source_size > NATIVE_CONTENT_MAX_ASSET_BYTES ||
        !output || !output_size || *output || *output_size ||
        !maps || !map_count || map_count > max_roots)
        return fail(error, error_size, "Animation remapping needs a bounded node map and empty output.");
    for (unsigned m = 0; m < map_count; ++m) {
        const auto& map = maps[m];
        if (!map.old_count || map.old_count > max_nodes || !map.new_count ||
            map.new_count > max_nodes || !map.new_to_old)
            return fail(error, error_size, "Animation node map counts are outside their bounds.");
        bool seen[max_nodes] = {};
        for (unsigned i = 0; i < map.new_count; ++i) {
            const unsigned old = map.new_to_old[i];
            if (old == UINT16_MAX) continue;
            if (old >= map.old_count || seen[old])
                return fail(error, error_size, "Animation node map contains an invalid or repeated source node.");
            seen[old] = true;
        }
    }
    HSD_Dat dat;
    if (HSD_DatOpen(&dat, source, source_size) != HSD_DAT_OK ||
        dat.root_count != map_count || dat.extern_count)
        return fail(error, error_size, "Animation DAT is malformed or has external dependencies.");
    std::vector<uint8_t> data;
    std::vector<uint32_t> relocations;
    std::vector<Span> streams;
    std::vector<Root> roots;
    for (u32 i = 0; i < dat.root_count; ++i) {
        u32 at;
        const char* name;
        if (!HSD_DatSymbol(&dat, false, i, &at, &name))
            return fail(error, error_size, "Animation root is invalid.");
        for (const Root& r : roots)
            if (!std::strcmp(r.name, name))
                return fail(error, error_size, "Animation root names must be unique.");
        uint32_t target;
        auto alias = std::find_if(roots.begin(), roots.end(),
            [&](const Root& r) { return r.source == at; });
        const auto& map = maps[i];
        if (alias != roots.end()) {
            const auto& prior = maps[alias->map];
            if (map.old_count != prior.old_count || map.new_count != prior.new_count ||
                std::memcmp(map.new_to_old, prior.new_to_old, map.new_count * sizeof(uint16_t)))
                return fail(error, error_size, "Aliased animation roots need the same node map.");
            target = alias->destination;
        } else if (!remap(dat, (uint32_t)at, map.old_count, map.new_to_old, map.new_count,
                        data, relocations, streams, &target))
            return fail(error, error_size, "Animation tree is malformed or has a different source node count.");
        roots.push_back({(uint32_t)at, target, name, (unsigned)i});
    }
    allocate(data, 0);
    std::sort(relocations.begin(), relocations.end());
    const size_t strings_at = 32 + data.size() + relocations.size() * 4 + roots.size() * 8;
    size_t total = strings_at;
    for (const Root& r : roots) total += std::strlen(r.name) + 1;
    if (total > NATIVE_CONTENT_MAX_ASSET_BYTES)
        return fail(error, error_size, "Remapped animation exceeds the asset size limit.");
    std::vector<uint8_t> bytes(total, 0);
    auto word = [&](size_t at, uint32_t value) { NativeAssetDat::be32(bytes.data() + at, value); };
    std::memcpy(bytes.data(), source, 32);
    word(0, (uint32_t)total);
    word(4, (uint32_t)data.size());
    word(8, (uint32_t)relocations.size());
    word(12, (uint32_t)roots.size());
    word(16, 0);
    std::memcpy(bytes.data() + 32, data.data(), data.size());
    size_t at = 32 + data.size();
    for (uint32_t slot : relocations) { word(at, slot); at += 4; }
    size_t text = strings_at;
    for (const Root& r : roots) {
        word(at, r.destination);
        word(at + 4, (uint32_t)(text - strings_at));
        const size_t length = std::strlen(r.name) + 1;
        std::memcpy(bytes.data() + text, r.name, length);
        text += length;
        at += 8;
    }
    HSD_Dat verified;
    if (HSD_DatOpen(&verified, bytes.data(), bytes.size()) != HSD_DAT_OK)
        return fail(error, error_size, "Remapped animation DAT did not validate.");
    void* copy = std::malloc(total);
    if (!copy) return fail(error, error_size, "Could not allocate the remapped animation.");
    std::memcpy(copy, bytes.data(), total);
    *output = copy;
    *output_size = total;
    if (error && error_size) error[0] = 0;
    return true;
}
