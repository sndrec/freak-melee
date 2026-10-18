#include "native_fighter_metal_references.h"
#include "native_asset_dat.h"
#include "ftparts.h"

#include <cstdio>

namespace {
constexpr uint32_t missing = UINT32_MAX;
struct Draw { uint32_t joint, mesh; };
struct Copy { uint32_t source, count, destination; };

bool fail(char* error, size_t capacity, const char* message)
{
    if (error && capacity) std::snprintf(error, capacity, "%s", message);
    return false;
}

bool pointer(const HSD_Dat& dat, uint32_t slot, uint32_t& result)
{
    u32 at = 0; const char* external;
    auto kind = HSD_DatResolve(&dat, slot, &at, &external);
    result = kind == HSD_DAT_REF_NULL ? missing : at;
    return kind == HSD_DAT_REF_NULL || kind == HSD_DAT_REF_LOCAL;
}

bool draws(const HSD_Dat& dat, const uint32_t* joints, size_t count,
           std::vector<Draw>& output)
{
    for (size_t n = 0; n < count; ++n) {
        u32 flags; uint32_t mesh;
        if (!HSD_DatReadU32(&dat, joints[n] + 4, &flags) ||
            !HSD_DatBytes(&dat, joints[n], 64) ||
            (flags & ((1u << 12) | (1u << 17)))) return false;
        if (flags & ((1u << 5) | (1u << 14))) continue; // Particle/spline union.
        if (!pointer(dat, joints[n] + 16, mesh)) return false;
        for (unsigned bound = 0; mesh != missing; ++bound) {
            if (bound == 128 || output.size() == 32 ||
                !HSD_DatBytes(&dat, mesh, 16)) return false;
            output.push_back({joints[n], mesh});
            if (!pointer(dat, mesh + 4, mesh)) return false;
        }
    }
    return true;
}
}

bool nativeFighterModel_RemapMetalSelectors(
    const HSD_Dat& source, const char* fighter_root,
    const uint32_t* before, size_t before_count,
    const uint32_t* after, size_t after_count, NativeAssetDat& candidate,
    char* error, size_t capacity)
{
    if (error && capacity) std::snprintf(error, capacity, "Could not remap the metal visibility tables.");
    std::vector<Draw> old_draws, new_draws;
    if ((!before && before_count) || (!after && after_count) ||
        before_count > MAX_FT_PARTS || after_count > MAX_FT_PARTS ||
        !draws(source, before, before_count, old_draws) ||
        !draws(source, after, after_count, new_draws))
        return fail(error, capacity, "The metal model has an unsupported draw hierarchy.");
    uint16_t map[32];
    for (unsigned i = 0; i < old_draws.size(); ++i) {
        map[i] = UINT16_MAX;
        for (unsigned j = 0; j < new_draws.size(); ++j)
            if (old_draws[i].joint == new_draws[j].joint &&
                old_draws[i].mesh == new_draws[j].mesh) map[i] = (uint16_t)j;
    }
    u32 root, groups; uint32_t model, visibility;
    if (!HSD_DatFindRoot(&source, fighter_root, &root) ||
        !pointer(source, root + 8, model))
        return fail(error, capacity, "The fighter's model visibility data is unavailable.");
    if (model == missing) model = 0; // Retail model record at offset zero.
    if (!HSD_DatReadU32(&source, model, &groups) || groups > 11 ||
        !pointer(source, model + 4, visibility) || visibility == missing)
        return fail(error, capacity, "The fighter's model visibility table is invalid.");
    const uint32_t end = HSD_DatNextTarget(&source, visibility);
    if (end < visibility || (end - visibility) % 16 ||
        end == visibility || (end - visibility) / 16 > 8 ||
        !HSD_DatBytes(&source, visibility, end - visibility) ||
        candidate.data.size() < end)
        return fail(error, capacity, "The fighter's costume visibility rows are invalid.");

    std::vector<Copy> lookups, variants, indices;
    for (uint32_t row = visibility; row < end; row += 16) {
        uint32_t lookup;
        if (!pointer(source, row + 8, lookup)) return false;
        if (lookup == missing) continue;
        if (!HSD_DatBytes(&source, lookup, groups * 8)) return false;
        // Read the original selectors. Costume remapping may already have
        // changed aliased bytes in the candidate. Detach only metal's view.
        bool changed = false;
        for (unsigned g = 0; g < groups; ++g) {
            u32 count; uint32_t list;
            if (!HSD_DatReadU32(&source, lookup + g * 8, &count) || count > 256 ||
                !pointer(source, lookup + g * 8 + 4, list) ||
                (count && (list == missing || !HSD_DatBytes(&source, list, count * 8))))
                return fail(error, capacity, "A metal visibility group is invalid.");
            for (unsigned v = 0; v < count; ++v) {
                u32 draws; uint32_t ids;
                if (!HSD_DatReadU32(&source, list + v * 8, &draws) || draws > 256 ||
                    !pointer(source, list + v * 8 + 4, ids) ||
                    (draws && (ids == missing || !HSD_DatBytes(&source, ids, draws) ||
                               size_t(ids) + draws > candidate.data.size())))
                    return fail(error, capacity, "A metal visibility selection is invalid.");
                for (unsigned i = 0; i < draws; ++i) {
                    unsigned old = source.data[ids + i];
                    if (old >= old_draws.size() || map[old] == UINT16_MAX)
                        return fail(error, capacity, "A metal visibility selection refers to a removed mesh.");
                    changed |= candidate.data[ids + i] != map[old];
                }
            }
        }
        if (!changed) continue;
        auto copied = std::find_if(lookups.begin(), lookups.end(),
            [&](const Copy& c) { return c.source == lookup; });
        if (copied != lookups.end()) {
            candidate.link_local(row + 8, copied->destination);
            continue;
        }
        uint32_t destination = candidate.allocate(groups * 8);
        if (destination == missing) return false;
        lookups.push_back({lookup, groups, destination});
        candidate.link_local(row + 8, destination);
        for (unsigned g = 0; g < groups; ++g) {
            u32 count; uint32_t list;
            HSD_DatReadU32(&source, lookup + g * 8, &count);
            pointer(source, lookup + g * 8 + 4, list);
            candidate.word(destination + g * 8, count);
            if (!count) { candidate.link(destination + g * 8 + 4, 0); continue; }
            auto found = std::find_if(variants.begin(), variants.end(),
                [&](const Copy& c) { return c.source == list && c.count == count; });
            if (found != variants.end()) {
                candidate.link_local(destination + g * 8 + 4, found->destination);
                continue;
            }
            uint32_t out = candidate.allocate(count * 8);
            if (out == missing) return false;
            variants.push_back({list, count, out});
            candidate.link_local(destination + g * 8 + 4, out);
            for (unsigned v = 0; v < count; ++v) {
                u32 draws; uint32_t ids;
                HSD_DatReadU32(&source, list + v * 8, &draws);
                pointer(source, list + v * 8 + 4, ids);
                candidate.word(out + v * 8, draws);
                if (!draws) { candidate.link(out + v * 8 + 4, 0); continue; }
                auto existing = std::find_if(indices.begin(), indices.end(),
                    [&](const Copy& c) { return c.source == ids && c.count == draws; });
                uint32_t target;
                if (existing != indices.end()) target = existing->destination;
                else {
                    target = candidate.allocate(draws, 1);
                    if (target == missing) return false;
                    for (unsigned i = 0; i < draws; ++i)
                        candidate.data[target + i] = (uint8_t)map[source.data[ids + i]];
                    indices.push_back({ids, draws, target});
                }
                candidate.link_local(out + v * 8 + 4, target);
            }
        }
    }
    if (error && capacity) error[0] = 0;
    return true;
}
