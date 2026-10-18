#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_asset_dat.h>
#include <melee/ft/native_fighter_costume_compact.h>
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/dat_texture.h>
}
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
unsigned checks;
void check(bool value, const char* label, unsigned line = __builtin_LINE())
{
    ++checks;
    if (!value) throw std::runtime_error(std::string(label) + " at line " + std::to_string(line));
}
bool close(float a, float b) { return std::isfinite(a) && std::isfinite(b) && std::fabs(a-b) < 0.00001f; }
bool remap(const NativeCostumeCompaction& stats, u32 source, size_t size, u32* result)
{
    auto at = std::upper_bound(stats.ranges.begin(), stats.ranges.end(), source,
        [](u32 source, const auto& range) { return source < range.source; });
    if (at == stats.ranges.begin()) return false;
    --at;
    if (source < at->source || source-at->source >= at->size || size > at->size-(source-at->source)) return false;
    *result = at->destination + (source-at->source); return true;
}
void compact_payloads(const std::vector<u8>& original, const std::vector<u8>& result,
                       const NativeCostumeCompaction& stats, const char* main_root)
{
    HSD_Dat a, b;
    check(HSD_DatOpen(&a, original.data(), original.size()) == HSD_DAT_OK, "uncompacted archive");
    check(HSD_DatOpen(&b, result.data(), result.size()) == HSD_DAT_OK, "compacted archive");
    std::vector<u32> pointers;
    for (u32 i = 0; i < a.relocation_count; ++i) {
        u32 slot, target, new_slot, new_target, actual; const char* external;
        HSD_DatRelocation(&a, i, &slot);
        if (!remap(stats, slot, 1, &new_slot)) continue;
        check(remap(stats, slot, 4, &new_slot), "whole pointer field");
        check(HSD_DatReadU32(&a, slot, &target) && remap(stats, target, 1, &new_target), "pointer target");
        check(HSD_DatResolve(&b, new_slot, &actual, &external) == HSD_DAT_REF_LOCAL && actual == new_target,
              "relocated pointer/alias");
        pointers.push_back(slot);
    }
    std::sort(pointers.begin(), pointers.end());
    for (const auto& range : stats.ranges) {
        auto pointer = std::lower_bound(pointers.begin(), pointers.end(), range.source);
        for (u32 i = 0; i < range.size;) {
            if (pointer != pointers.end() && *pointer == range.source+i) { i += 4; ++pointer; continue; }
            check(a.data[range.source+i] == b.data[range.destination+i], "unchanged non-pointer bytes"); ++i;
        }
    }
    auto payload = [&](const void* data, size_t size, const char* label) {
        if (!size) return;
        const size_t source = (const u8*)data-a.data;
        u32 destination;
        check(source < a.data_size && remap(stats, (u32)source, size, &destination), label);
        const u8* bytes = HSD_DatBytes(&b, destination, size);
        check(bytes && std::memcmp(data, bytes, size) == 0, label);
    };
    std::vector<u8> arena(FT_COSTUME_IMPORT_ARENA_SIZE);
    for (u32 i = 0; i < a.root_count; ++i) {
        u32 offset, after_offset, expected; const char* name;
        check(HSD_DatSymbol(&a, false, i, &offset, &name), "original root symbol");
        check(remap(stats, offset, 1, &expected) && HSD_DatFindRoot(&b, name, &after_offset) &&
              after_offset == expected, "root symbol identity");
        HSD_DatImportType type = DAT_JOINT;
        const std::string symbol(name);
        if (symbol.size() >= 14 && symbol.compare(symbol.size()-14, 14, "_matanim_joint") == 0) type = DAT_MATANIM_JOINT;
        else if (symbol.size() >= 11 && symbol.compare(symbol.size()-11, 11, "_anim_joint") == 0 && symbol != main_root) type = DAT_ANIMJOINT;
        HSD_DatImport graph;
        check(HSD_DatImportInit(&graph, &a, arena.data(), arena.size(), FT_COSTUME_IMPORT_RECORD_CAPACITY) == HSD_DAT_OK,
              "independent payload graph");
        check(HSD_DatImportGraph(&graph, offset, type) != nullptr, "independent original graph");
        for (u32 record = 0; record < graph.record_count; ++record) {
            const auto& node = graph.records[record];
            if (node.type == DAT_IMAGE) {
                HSD_ImageDesc image; size_t bytes;
                check(HSD_DatDecodeImage(&a, node.offset, &image, &bytes) == HSD_DAT_OK, "image extent");
                payload(image.image_ptr, bytes, "all texture pixels and mip levels");
            } else if (node.type == DAT_PALETTE) {
                const auto* palette = static_cast<const HSD_TlutDesc*>(node.object);
                payload(palette->lut, size_t(palette->n_entries)*2, "all palette entries");
            } else if (node.type == DAT_FOBJ) {
                const auto* fobj = static_cast<const HSD_FObjDesc*>(node.object);
                payload(fobj->ad, fobj->length, "complete compressed animation stream");
            } else if (node.type == DAT_VTXDESC) {
                const auto* attrs = static_cast<const HSD_VtxDescList*>(node.object);
                for (u32 attr = 0; attr < node.count && attrs[attr].attr != GX_VA_NULL; ++attr)
                    if (attrs[attr].attr_type > GX_DIRECT)
                        payload(attrs[attr].vertex, attrs[attr].vertex_size, "rendered indexed vertex buffer");
            }
        }
    }
}
std::vector<u8> extras(const HSD_Dat& dat, const NativeSkinPart& part, const NativeSkinCorner& corner)
{
    std::vector<u8> bytes;
    u32 cursor = corner.source;
    for (const auto& attr : part.attributes) {
        const u8* data = dat.data + cursor;
        cursor += attr.display_bytes;
        if (attr.attr == 0 || attr.attr == 9 || attr.attr == 10 || attr.attr == 25) continue;
        bytes.push_back((u8)attr.attr);
        if (attr.attr >= 1 && attr.attr <= 8) {
            check(data[0] == 60 || data[0] == 30 + corner.matrix*3, "texture binding follows position");
            bytes.push_back(data[0] == 60); continue;
        }
        if (attr.type != 1) {
            const unsigned index = attr.type == 2 ? data[0] : unsigned(data[0]) << 8 | data[1];
            data = HSD_DatBytes(&dat, attr.array + index * attr.stride, attr.width);
        }
        check(data != nullptr, "attribute array");
        bytes.insert(bytes.end(), data, data + attr.width);
    }
    return bytes;
}
void compare(const NativeFighterModelEditor& a, const NativeSkinMesh& before,
               const NativeFighterModelEditor& b, const NativeSkinMesh& after, bool unchanged = true)
{
    if (unchanged) check(before.parts.size() == after.parts.size(), "polygon boundaries");
    check(before.vertices.size() == after.vertices.size(), "vertex identities");
    struct Entry { const NativeSkinPart* part; const NativeSkinCorner* corner; };
    std::vector<Entry> previous, current;
    for (const auto& part : before.parts) for (unsigned index : part.triangles)
        previous.push_back({ &part, &part.corners[index] });
    for (const auto& part : after.parts) for (unsigned index : part.triangles)
        current.push_back({ &part, &part.corners[index] });
    check(previous.size() == current.size(), "triangle count");
    for (unsigned t = 0; t < previous.size(); ++t) {
        const auto& old = *previous[t].part; const auto& now = *current[t].part;
        const auto& ca = *previous[t].corner; const auto& cb = *current[t].corner;
        const auto& va = before.vertices[ca.vertex]; const auto& vb = after.vertices[cb.vertex];
        check((old.flags & ~0x3000) == (now.flags & ~0x3000), "polygon render flags");
        check(ca.vertex == cb.vertex, "stable editing identity");
        for (unsigned axis = 0; axis < 3; ++axis)
            check(close(va.position[axis], vb.position[axis]), "bind position");
        for (unsigned n = 0; n < 9; ++n)
            check(close(ca.normal[n], cb.normal[n]), "normal/tangent/binormal");
        if (unchanged) check(nativeSkinWeights_Equal(&va.skin, &vb.skin), "unchanged weights");
        check(extras(a.dat, old, ca) == extras(b.dat, now, cb), "UV/color/texture attributes");
    }
}
unsigned paint_cases(NativeFighterProject& original, const NativeFighterModelEditor& model)
{
    unsigned cases = 0;
    bool types[3] = {};
    for (unsigned mesh = 0; mesh < model.mesh_count; ++mesh) {
        NativeSkinMesh old;
        check(nativeSkinMesh_Read(model, mesh, &old, nullptr, 0), "brush source");
        const unsigned kind = (old.parts[0].flags >> 12) & 3;
        if (types[kind]) continue;
        unsigned target = 0;
        for (; target < model.joint_count; ++target) {
            float matrix[3][4];
            if ((model.joints[target].flags & 1) &&
                nativeSkinMesh_InverseBind(model, target, matrix) &&
                nativeSkinWeights_Get(&old.vertices[0].skin, (u16)target) < .8f) break;
        }
        if (target == model.joint_count) continue;
        types[kind] = true;
        NativeFighterProject painted = original;
        painted.costume_dat = (u8*)std::malloc(original.costume_size);
        check(painted.costume_dat != nullptr, "brush costume copy");
        std::memcpy(painted.costume_dat, original.costume_dat, original.costume_size);
        NativeSkinBrushPoint point;
        std::memcpy(point.position, old.vertices[0].position, sizeof(point.position));
        unsigned affected = 0; char error[192] = {};
        const bool ok = nativeFighterSkin_Paint(&painted, mesh, target, &point, 1,
                                                .01f, .2f, false, &affected, error, sizeof(error));
        if (!ok) { std::free(painted.costume_dat); throw std::runtime_error("brush mesh " + std::to_string(mesh) + ": " + error); }
        check(affected > 0, "brush affected vertices");
        auto edited = std::make_unique<NativeFighterModelEditor>();
        check(nativeFighterModel_Open(edited.get(), &painted), "painted model");
        NativeSkinMesh now;
        check(nativeSkinMesh_Read(*edited, mesh, &now, nullptr, 0), "painted weights");
        compare(model, old, *edited, now, false);
        check(close(nativeSkinWeights_Get(&now.vertices[0].skin, (u16)target),
                    nativeSkinWeights_Get(&old.vertices[0].skin, (u16)target) + .2f), "brush strength");
        std::free(painted.costume_dat); ++cases;
    }
    return cases;
}

void zero_reference(const std::vector<u8>& bytes)
{
    NativeAssetDat dat;
    check(dat.open(bytes.data(), bytes.size()), "zero-reference transaction");
    const u32 slot = dat.allocate(4);
    std::vector<u8> result;
    for (unsigned test = 0; test < 3; ++test) {
        if (test != 1) dat.link_local(slot, 0);
        else dat.link(slot, 0);
        check(dat.finish(&result), "zero-reference archive");
        HSD_Dat verified; u32 target; const char* external;
        check(HSD_DatOpen(&verified, result.data(), result.size()) == HSD_DAT_OK, "zero-reference view");
        check(HSD_DatResolve(&verified, slot, &target, &external) ==
            (test == 1 ? HSD_DAT_REF_NULL : HSD_DAT_REF_LOCAL), "local-zero/null transitions");
    }
}

void opaque_cases()
{
    std::vector<u8> file(164, 0);
    auto word = [&](size_t at, u32 value) { NativeAssetDat::be32(file.data()+at, value); };
    word(0, (u32)file.size()); word(4, 108); word(8, 3); word(12, 1);
    word(32+16, 64); word(32+64+8, 80); word(32+80+16, 104);
    word(32+108, 16); word(32+112, 72); word(32+116, 96);
    std::memcpy(file.data()+32+104, "keep", 4);
    std::memcpy(file.data()+160, "rig", 4);
    NativeFighterProject project = {};
    std::snprintf(project.facts.costume_joint, sizeof(project.facts.costume_joint), "rig");
    NativeCostumeCompaction stats;
    const auto original = file;
    check(nativeFighterCostume_Compact(&project, &file, &stats, nullptr, 0) && !stats.supported &&
          file == original && std::strstr(stats.reason, "Untraced"), "opaque legacy data retained");
    std::memcpy(file.data()+160, "raw", 4);
    const auto unknown = file;
    check(nativeFighterCostume_Compact(&project, &file, &stats, nullptr, 0) && !stats.supported &&
          file == unknown && std::strstr(stats.reason, "Unknown"), "unknown root retained");
    file = original; word(32+16, 109);
    const auto invalid = file;
    check(!nativeFighterCostume_Compact(&project, &file, &stats, nullptr, 0) && file == invalid,
          "invalid source fails atomically");
    file.assign(175, 0);
    word(0, (u32)file.size()); word(4, 108); word(8, 2); word(12, 1); word(16, 1);
    word(32+16, 64); word(32+64+8, 80); word(32+80+16, UINT32_MAX);
    word(140, 16); word(144, 72); word(156, 96); word(160, 4);
    std::memcpy(file.data()+164, "rig\0shared", 11);
    const auto external = file;
    check(nativeFighterCostume_Compact(&project, &file, &stats, nullptr, 0) && !stats.supported &&
          file == external && std::strstr(stats.reason, "external"), "external chain retained");
}

size_t stress_edits(const NativeFighterProject& original, unsigned iterations)
{
    if (!iterations) return 0;
    check(iterations >= 32 && iterations <= 10000, "stress iteration count");
    auto model = std::make_unique<NativeFighterModelEditor>();
    check(nativeFighterModel_Open(model.get(), const_cast<NativeFighterProject*>(&original)), "stress model");
    NativeSkinMesh skin;
    unsigned mesh = 0, target = model->joint_count;
    for (; mesh < model->mesh_count; ++mesh) {
        check(nativeSkinMesh_Read(*model, mesh, &skin, nullptr, 0), "stress source");
        if (skin.vertices.size() < 4) continue;
        for (target = 0; target < model->joint_count; ++target) {
            float inverse[3][4];
            if ((model->joints[target].flags & 1) && nativeSkinMesh_InverseBind(*model, target, inverse) &&
                nativeSkinWeights_Get(&skin.vertices[0].skin, (u16)target) == 0) break;
        }
        if (target < model->joint_count) break;
    }
    check(mesh < model->mesh_count, "stress mesh and alternate bone");
    NativeFighterProject edited = original;
    edited.costume_dat = (u8*)std::malloc(original.costume_size);
    check(edited.costume_dat != nullptr, "stress costume copy");
    std::memcpy(edited.costume_dat, original.costume_dat, original.costume_size);
    NativeSkinBrushPoint point;
    std::memcpy(point.position, skin.vertices[0].position, sizeof(point.position));
    size_t warmup_max = 0, maximum = 0;
    for (unsigned i = 0; i < iterations; ++i) {
        unsigned affected; char error[192] = {};
        check(nativeFighterSkin_Paint(&edited, mesh, target, &point, 1, .01f, .2f, (i&1) != 0,
                                       &affected, error, sizeof(error)), error);
        check(affected > 0, "stress stroke changes weights");
        maximum = std::max(maximum, edited.costume_size);
        if (i < 16) warmup_max = std::max(warmup_max, edited.costume_size);
        else check(edited.costume_size <= warmup_max, "storage stays bounded after warmup");
        auto current = std::make_unique<NativeFighterModelEditor>();
        check(nativeFighterModel_Open(current.get(), &edited), "stress edited model");
        NativeSkinMesh now;
        check(nativeSkinMesh_Read(*current, mesh, &now, nullptr, 0), "stress edited weights");
        check(close(nativeSkinWeights_Get(&now.vertices[0].skin, (u16)target), i&1 ? 0 : .2f), "stress saved strength");
        for (unsigned joint = 0; joint < model->joint_count; ++joint)
            check(model->joints[joint].offset == current->joints[joint].offset, "stress bone identity");
    }
    std::printf("STRESS: iterations=%u original=%zu max=%zu final=%zu warmup_max=%zu\n",
                 iterations, original.costume_size, maximum, edited.costume_size, warmup_max);
    std::free(edited.costume_dat);
    return maximum;
}

}

int main(int argc, char** argv)
{
    try {
        check(argc >= 3 && argc <= 5, "Usage: stock_skin_edit_test costume.dat root [roundtrip.dat [stress-count]]");
        opaque_cases();
        FILE* file = std::fopen(argv[1], "rb"); check(file != nullptr, "open costume");
        std::fseek(file, 0, SEEK_END); const long size = std::ftell(file); std::rewind(file);
        check(size > 0, "costume size");
        std::vector<u8> source((size_t)size);
        check(std::fread(source.data(), 1, source.size(), file) == source.size(), "read costume");
        std::fclose(file);
        zero_reference(source);
        NativeFighterProject project = {};
        project.costume_dat = source.data(); project.costume_size = source.size();
        std::snprintf(project.facts.costume_joint, sizeof(project.facts.costume_joint), "%s", argv[2]);
        auto before = std::make_unique<NativeFighterModelEditor>();
        check(nativeFighterModel_Open(before.get(), &project), "original model");
        NativeAssetDat transaction;
        check(transaction.open(source.data(), source.size()), "open transaction");
        for (unsigned mesh = 0; mesh < before->mesh_count; ++mesh) {
            NativeSkinMesh skin; char error[192] = {};
            if (!nativeSkinMesh_Read(*before, mesh, &skin, error, sizeof(error)))
                throw std::runtime_error("read mesh " + std::to_string(mesh) + ": " + error);
            if (!nativeSkinMesh_Write(transaction, *before, mesh, skin))
                throw std::runtime_error("write mesh " + std::to_string(mesh));
        }
        std::vector<u8> result;
        check(transaction.finish(&result), "archive round trip");
        const auto uncompacted = result;
        NativeCostumeCompaction compaction;
        char compact_error[192] = {};
        check(nativeFighterCostume_Compact(&project, &result, &compaction, compact_error, sizeof(compact_error)), compact_error);
        if (!compaction.supported) throw std::runtime_error(std::string("compaction unsupported: ") + compaction.reason);
        compact_payloads(uncompacted, result, compaction, argv[2]);
        const auto once = result;
        check(nativeFighterCostume_Compact(&project, &result, nullptr, compact_error, sizeof(compact_error)), "second compaction");
        check(result == once, "idempotent compaction");
        if (argc >= 4) {
            file = std::fopen(argv[3], "wb"); check(file != nullptr, "open result");
            check(std::fwrite(result.data(), 1, result.size(), file) == result.size(), "write result"); std::fclose(file);
        }
        std::vector<u8> arena(FT_COSTUME_IMPORT_ARENA_SIZE);
        FtCostumeAssets imported;
        if (!ftCostumeAssets_Import(result.data(), result.size(), argv[2], nullptr,
            arena.data(), arena.size(), FT_COSTUME_IMPORT_RECORD_CAPACITY, &imported))
            throw std::runtime_error("game costume importer status " + std::to_string(imported.import.status) +
                                     " offset " + std::to_string(imported.import.error_offset));
        NativeFighterProject rewritten = project;
        rewritten.costume_dat = result.data(); rewritten.costume_size = result.size();
        auto after = std::make_unique<NativeFighterModelEditor>();
        check(nativeFighterModel_Open(after.get(), &rewritten), "rewritten model");
        check(before->mesh_count == after->mesh_count && before->joint_count == after->joint_count &&
              before->material_count == after->material_count, "rig and material counts");
        for (unsigned i = 0; i < before->joint_count; ++i)
            check(before->joints[i].offset == after->joints[i].offset, "stable bone identity");
        for (unsigned i = 0; i < before->material_count; ++i)
            check(before->materials[i].offset == after->materials[i].offset, "stable material identity");
        for (unsigned mesh = 0; mesh < before->mesh_count; ++mesh) {
            check(before->meshes[mesh].offset == after->meshes[mesh].offset &&
                  before->meshes[mesh].joint == after->meshes[mesh].joint &&
                  before->meshes[mesh].material == after->meshes[mesh].material, "mesh ownership/material");
            NativeSkinMesh old, now;
            check(nativeSkinMesh_Read(*before, mesh, &old, nullptr, 0), "read original mesh");
            check(nativeSkinMesh_Read(*after, mesh, &now, nullptr, 0), "read rewritten mesh");
            compare(*before, old, *after, now);
        }
        const unsigned brush_cases = paint_cases(project, *before);
        if (argc == 5) {
            char* end; const unsigned count = (unsigned)std::strtoul(argv[4], &end, 10);
            check(*end == 0, "stress count");
            stress_edits(project, count);
        }
        std::printf("PASS: meshes=%u checks=%u brushes=%u original=%zu rewritten=%zu compact_before=%zu compact_after=%zu pinned=%zu\n",
                     before->mesh_count, checks, brush_cases, source.size(), result.size(), compaction.before, compaction.after, compaction.pinned);
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what()); return 1;
    }
}
