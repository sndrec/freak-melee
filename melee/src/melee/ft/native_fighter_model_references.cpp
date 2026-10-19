#include "native_fighter_model_references.h"

#include <algorithm>
#include <vector>

#include "native_asset_dat.h"

namespace {
    u32 pointer(const HSD_Dat& dat, u32 slot)
    {
        u32 value = 0;
        const char* external;
        return HSD_DatResolve(&dat, slot, &value, &external) ==
                       HSD_DAT_REF_LOCAL
                   ? value
                   : 0;
    }
    struct TextureSlot {
        u32 mesh, texture;
    };
    std::vector<TextureSlot> textures(const NativeFighterModelEditor& model)
    {
        std::vector<TextureSlot> result;
        for (unsigned i = 0; i < model.mesh_count; ++i) {
            u32 material = pointer(model.dat, model.meshes[i].offset + 8);
            for (u32 texture = material ? pointer(model.dat, material + 8) : 0,
                     n = 0;
                 texture && n < 8; ++n)
            {
                if (!HSD_DatBytes(&model.dat, texture, 84)) {
                    return {};
                }
                result.push_back({ model.meshes[i].offset, texture });
                texture = pointer(model.dat, texture + 4);
            }
        }
        return result;
    }
    struct Edit {
        u32 offset;
        uint16_t value;
        bool half;
    };
}
namespace {
    bool selector_fields(const HSD_Dat& dat, const char* root_name,
                         std::vector<Edit>* fields)
    {
        u32 root, model;
        const char* external;
        if (!HSD_DatFindRoot(&dat, root_name, &root)) {
            return false;
        }
        // Retail model data can start at archive offset zero without a
        // relocation. ftCharacterAssets_Import reads that record explicitly in
        // this case.
        auto kind = HSD_DatResolve(&dat, root + 8, &model, &external);
        if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) {
            return false;
        }
        u32 visibility = pointer(dat, model + 4),
            texture_rows = pointer(dat, model + 12), groups = 0, count = 0;
        if (!visibility || !texture_rows ||
            !HSD_DatReadU32(&dat, model, &groups) ||
            !HSD_DatReadU32(&dat, model + 8, &count) || groups > 11 ||
            count > 5)
        {
            return false;
        }
        unsigned bytes = HSD_DatNextTarget(&dat, visibility) - visibility,
                 rows = bytes / 16;
        if (bytes % 16 || !rows || rows > 8 ||
            !HSD_DatBytes(&dat, texture_rows, rows * 4))
        {
            return false;
        }
        for (unsigned row = 0; row < rows; ++row) {
            for (unsigned layer = 0; layer < 4; ++layer) {
                if (layer == 2) {
                    continue; // The separate metal-model draw list.
                }
                u32 lookup = pointer(dat, visibility + (row * 4 + layer) * 4);
                if (!lookup) {
                    continue;
                }
                if (!HSD_DatBytes(&dat, lookup, groups * 8)) {
                    return false;
                }
                for (unsigned g = 0; g < groups; ++g) {
                    u32 variants = 0;
                    HSD_DatReadU32(&dat, lookup + g * 8, &variants);
                    u32 list = pointer(dat, lookup + g * 8 + 4);
                    if (variants > 256 ||
                        !HSD_DatBytes(&dat, list, variants * 8))
                    {
                        return false;
                    }
                    for (unsigned v = 0; v < variants; ++v) {
                        u32 draws = 0;
                        HSD_DatReadU32(&dat, list + v * 8, &draws);
                        u32 indices = pointer(dat, list + v * 8 + 4);
                        const u8* ids = HSD_DatBytes(&dat, indices, draws);
                        if (draws > 256 || !ids) {
                            return false;
                        }
                        for (unsigned i = 0; i < draws; ++i) {
                            fields->push_back({ indices + i, ids[i], false });
                        }
                    }
                }
            }
            u32 indices = pointer(dat, texture_rows + row * 4);
            if (!indices) {
                continue;
            }
            if (!HSD_DatBytes(&dat, indices, count * 2)) {
                return false;
            }
            for (unsigned i = 0; i < count; ++i) {
                u16 value = 0;
                HSD_DatReadU16(&dat, indices + i * 2, &value);
                fields->push_back({ indices + i * 2, value, true });
            }
        }
        return true;
    }
}
bool nativeFighterModel_RemapSelectors(const NativeFighterModelEditor* before,
                                       const NativeFighterModelEditor* after,
                                       NativeFighterProject* project)
{
    if (!before || !after || !project || after->mesh_count > 124) {
        return false;
    }
    unsigned map[NATIVE_MODEL_MAX_MESHES];
    for (unsigned i = 0; i < before->mesh_count; ++i) {
        map[i] = UINT32_MAX;
        for (unsigned j = 0; j < after->mesh_count; ++j) {
            if (before->meshes[i].offset == after->meshes[j].offset) {
                map[i] = j;
            }
        }
    }
    auto old_textures = textures(*before), new_textures = textures(*after);
    HSD_Dat dat;
    std::vector<Edit> fields;
    if (HSD_DatOpen(&dat, project->fighter_dat, project->fighter_size) !=
            HSD_DAT_OK ||
        !selector_fields(dat, project->fighter_root, &fields))
    {
        return false;
    }
    for (auto& e : fields) {
        unsigned next = UINT32_MAX, old = e.value;
        if (e.half) {
            if (old < old_textures.size()) {
                for (unsigned i = 0; i < new_textures.size(); ++i) {
                    if (old_textures[old].mesh == new_textures[i].mesh &&
                        old_textures[old].texture == new_textures[i].texture)
                    {
                        next = i;
                    }
                }
            } else {
                next = old + new_textures.size() - old_textures.size();
            }
        } else {
            next = old < before->mesh_count
                       ? map[old]
                       : old + after->mesh_count - before->mesh_count;
        }
        if (next > (e.half ? UINT16_MAX : UINT8_MAX)) {
            return false;
        }
        e.value = (uint16_t) next;
    }
    // Aliased rows read their original values until collection is complete.
    for (const auto& e : fields) {
        uint8_t* p = project->fighter_dat + 32 + e.offset;
        if (e.half) {
            NativeAssetDat::be16(p, e.value);
        } else {
            *p = (uint8_t) e.value;
        }
    }
    return true;
}
bool nativeFighterModel_RestoreSelectors(NativeFighterProject* project,
                                         const void* saved, size_t bytes)
{
    HSD_Dat source, destination;
    std::vector<Edit> old_fields, new_fields;
    if (!project || HSD_DatOpen(&source, saved, bytes) != HSD_DAT_OK ||
        HSD_DatOpen(&destination, project->fighter_dat,
                    project->fighter_size) != HSD_DAT_OK ||
        !selector_fields(source, project->fighter_root, &old_fields) ||
        !selector_fields(destination, project->fighter_root, &new_fields) ||
        old_fields.size() != new_fields.size())
    {
        return false;
    }
    for (unsigned i = 0; i < old_fields.size(); ++i) {
        if (old_fields[i].half != new_fields[i].half) {
            return false;
        }
    }
    for (unsigned i = 0; i < old_fields.size(); ++i) {
        uint8_t* p = project->fighter_dat + 32 + new_fields[i].offset;
        if (old_fields[i].half) {
            NativeAssetDat::be16(p, old_fields[i].value);
        } else {
            *p = (uint8_t) old_fields[i].value;
        }
    }
    return true;
}

bool nativeFighterModel_RemapMaterialAnimations(
    const NativeFighterModelEditor& before,
    const NativeFighterModelEditor& after, NativeAssetDat& costume)
{
    if (!before.project->facts.costume_matanim[0]) {
        return true;
    }
    u32 root;
    if (!HSD_DatFindRoot(&before.dat, before.project->facts.costume_matanim,
                         &root))
    {
        return false;
    }
    struct Pair {
        u32 mesh, material, animation;
    };
    std::vector<Pair> pairs;
    unsigned visited = 0;
    auto walk = [&](auto&& self, u32 joint, u32 animation,
                    unsigned depth) -> bool {
        if (depth > 512) {
            return false;
        }
        for (; joint && animation;
             joint = pointer(before.dat, joint + 12),
             animation = pointer(before.dat, animation + 4))
        {
            if (++visited > 512 || !HSD_DatBytes(&before.dat, joint, 64) ||
                !HSD_DatBytes(&before.dat, animation, 12))
            {
                return false;
            }
            u32 mat = pointer(before.dat, animation + 8);
            pairs.clear();
            for (unsigned i = 0; i < before.mesh_count; ++i) {
                if (before.joints[before.meshes[i].joint].offset == joint) {
                    pairs.push_back({ before.meshes[i].offset,
                        before.meshes[i].material<before.material_count
                            ?before.materials[before.meshes[i].material].offset:UINT32_MAX, mat });
                    if (mat) {
                        if (!HSD_DatBytes(&before.dat, mat, 16)) {
                            return false;
                        }
                        mat = pointer(before.dat, mat);
                    }
                }
            }
            if (pointer(before.dat, animation + 8)) {
                u32 first = 0, previous = 0;
                for (unsigned i = 0; i < after.mesh_count; ++i) {
                    if (after.joints[after.meshes[i].joint].offset == joint) {
                        u32 old = 0;
                        for (const auto& p : pairs) {
                            const u32 material=after.meshes[i].material<after.material_count
                                ?after.materials[after.meshes[i].material].offset:UINT32_MAX;
                            if (p.mesh == after.meshes[i].offset && p.material == material) {
                                old = p.animation;
                            }
                        }
                        u32 added = costume.allocate(16);
                        if (added == UINT32_MAX) {
                            return false;
                        }
                        if (old) {
                            costume.link(added + 4,
                                         pointer(before.dat, old + 4));
                            costume.link(added + 8,
                                         pointer(before.dat, old + 8));
                        }
                        if (previous) {
                            costume.link(previous, added);
                        } else {
                            first = added;
                        }
                        previous = added;
                    }
                }
                costume.link(animation + 8, first);
            }
            if (!self(self, pointer(before.dat, joint + 8),
                      pointer(before.dat, animation), depth + 1))
            {
                return false;
            }
        }
        return true;
    };
    u32 joint = 0;
    return HSD_DatFindRoot(&before.dat, before.root_name, &joint) &&
           walk(walk, joint, root, 0);
}
