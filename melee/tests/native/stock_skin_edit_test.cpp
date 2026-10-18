#include <melee/ft/native_fighter_skin_mesh.h>
#include <melee/ft/native_asset_dat.h>
extern "C" {
#include <melee/ft/ftcostume_assets.h>
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

}

int main(int argc, char** argv)
{
    try {
        check(argc == 3 || argc == 4, "Usage: stock_skin_edit_test costume.dat root [roundtrip.dat]");
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
        if (argc == 4) {
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
        std::printf("PASS: meshes=%u checks=%u brushes=%u original=%zu rewritten=%zu\n",
                     before->mesh_count, checks, brush_cases, source.size(), result.size());
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what()); return 1;
    }
}
