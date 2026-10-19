#include <melee/ft/native_fighter_costume_compact.h>
#include <melee/ft/native_asset_dat.h>
extern "C" {
#include <melee/ft/ftcostume_assets.h>
#include <sysdolphin/baselib/jobj.h>
}
#include <cstdio>
#include <cstdlib>
#include <string>

static unsigned checks;
static void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
}

static void large_model(const std::vector<u8>& original, unsigned polygons, unsigned blocks,
                        bool admitted = true)
{
    NativeAssetDat dat;
    check(dat.open(original.data(), original.size()), "large source");
    dat.word(4, JOBJ_SKELETON_ROOT);
    const u32 mesh = dat.allocate(16), positions = dat.allocate(36);
    const u32 palette = dat.allocate(8), envelope = dat.allocate(16);
    dat.link(16, mesh); dat.link(palette, envelope);
    dat.link_local(envelope, 0); dat.word(envelope+4, 0x3F800000);
    dat.word(positions+12, 0x3F800000); dat.word(positions+28, 0x3F800000);
    const u32 display = dat.allocate(blocks*32);
    const u8 triangle[] = { 0x90, 0, 3, 0, 0, 0, 0, 0, 1, 0, 0, 2 };
    std::memcpy(dat.data.data()+display, triangle, sizeof(triangle));
    u32 previous = 0;
    for (unsigned i = 0; i < polygons; ++i) {
        const u32 polygon = dat.allocate(24), attrs = dat.allocate(72);
        dat.link(previous ? previous+4 : mesh+12, polygon); previous = polygon;
        dat.link(polygon+8, attrs); dat.half(polygon+12, POBJ_ENVELOPE);
        dat.half(polygon+14, (u16)blocks); dat.link(polygon+16, display); dat.link(polygon+20, palette);
        dat.word(attrs, GX_VA_PNMTXIDX); dat.word(attrs+4, GX_DIRECT);
        dat.word(attrs+24, GX_VA_POS); dat.word(attrs+28, GX_INDEX16);
        dat.word(attrs+32, GX_POS_XYZ); dat.word(attrs+36, GX_F32);
        dat.half(attrs+42, 12); dat.link(attrs+44, positions); dat.word(attrs+48, GX_VA_NULL);
    }
    std::vector<u8> bytes;
    check(dat.finish(&bytes), "large fixture archive");
    const auto before = bytes;
    std::vector<u8> small(FT_COSTUME_IMPORT_ARENA_SIZE);
    FtCostumeAssets fixed;
    check(!ftCostumeAssets_Import(bytes.data(), bytes.size(), "rig", nullptr,
        small.data(), small.size(), FT_COSTUME_IMPORT_RECORD_CAPACITY, &fixed) &&
        fixed.import.status == HSD_DAT_CAPACITY, "fixture exceeds old import capacity");
    FtCostumeAssets loaded;
    if (!admitted) {
        check(!ftCostumeAssets_Load(bytes.data(), bytes.size(), "rig", nullptr, &loaded) &&
            loaded.import.status == HSD_DAT_CAPACITY && !loaded.arena && !loaded.joint &&
            !loaded.import.arena && !loaded.import.records && bytes == before,
            "oversized workspace rejects with diagnostics and no published graph");
        NativeFighterProject project = {};
        std::snprintf(project.facts.costume_joint, sizeof(project.facts.costume_joint), "rig");
        char error[192] = {};
        check(!nativeFighterCostume_Compact(&project, &bytes, nullptr, error, sizeof(error)) &&
            bytes == before, "oversized compaction remains atomic");
        return;
    }
    check(ftCostumeAssets_Load(bytes.data(), bytes.size(), "rig", nullptr, &loaded), "grow model storage");
    check(loaded.arena == loaded.import.arena && loaded.import.dat == &loaded.dat && loaded.joint_count == 1,
        "stable graph ownership");
    unsigned count = 0;
    for (auto* polygon = loaded.joint->u.dobjdesc->pobjdesc; polygon; polygon = polygon->next) {
        check(polygon->n_display == blocks && polygon->display != loaded.dat.data+display &&
            !std::memcmp(polygon->display, triangle, sizeof(triangle)), "retained writable display");
        check(polygon->u.envelope_p[0][0].joint == loaded.joint, "root offset-zero alias");
        ++count;
    }
    check(count == polygons && bytes == before, "complete import keeps source immutable");
    std::printf("large polygons=%u records=%u arena_used=%zu arena_capacity=%zu\n",
        polygons, (unsigned)loaded.import.record_count, loaded.import.arena_used, loaded.import.arena_size);
    free(loaded.arena);
    NativeFighterProject project = {};
    std::snprintf(project.facts.costume_joint, sizeof(project.facts.costume_joint), "rig");
    NativeCostumeCompaction stats; char error[192] = {};
    check(nativeFighterCostume_Compact(&project, &bytes, &stats, error, sizeof(error)), error);
    check(stats.supported && ftCostumeAssets_Load(bytes.data(), bytes.size(), "rig", nullptr, &loaded),
        "large graph compaction and reload");
    free(loaded.arena);
    check(!ftCostumeAssets_Load(bytes.data(), bytes.size(), "missing", nullptr, &loaded) &&
        loaded.import.status == HSD_DAT_SYMBOL && !loaded.arena && !loaded.joint &&
        !loaded.import.arena && !loaded.import.records && !loaded.import.slots,
        "failed load retires partial graph");
}

int main()
{
    std::vector<u8> original(108, 0);
    NativeAssetDat::be32(original.data(), (u32)original.size());
    NativeAssetDat::be32(original.data()+4, 64);
    NativeAssetDat::be32(original.data()+12, 1);
    for (unsigned axis = 0; axis < 3; ++axis)
        NativeAssetDat::be32(original.data()+32+32+axis*4, 0x3F800000);
    std::memcpy(original.data()+104, "rig", 4);
    NativeFighterProject project = {};
    std::snprintf(project.facts.costume_joint, sizeof(project.facts.costume_joint), "rig");
    NativeAssetDat dat;
    check(dat.open(original.data(), original.size()), "small source");
    check(dat.allocate(NATIVE_CONTENT_MAX_PACKAGE_BYTES + 4096) != UINT32_MAX, "dead allocation");
    std::vector<u8> staged = { 1, 2, 3 };
    check(!dat.finish(&staged) && staged == std::vector<u8>({1, 2, 3}), "ordinary package cap is atomic");
    check(dat.finish(&staged, UINT32_MAX) && staged.size() > NATIVE_CONTENT_MAX_PACKAGE_BYTES,
          "temporary costume may exceed package cap");
    NativeCostumeCompaction stats;
    char error[192] = {};
    check(nativeFighterCostume_Compact(&project, &staged, &stats, error, sizeof(error)), error);
    check(stats.supported && stats.before > NATIVE_CONTENT_MAX_PACKAGE_BYTES && stats.after == original.size(),
          "reclaim before applying final package cap");
    check(staged == original, "all dead storage removed with identity intact");
    check(dat.finish(&staged, UINT32_MAX), "second oversized staging");
    std::memcpy(staged.data()+staged.size()-4, "raw", 4);
    const auto unsupported = staged;
    check(!nativeFighterCostume_Compact(&project, &staged, &stats, error, sizeof(error)) &&
          !stats.supported && staged == unsupported, "oversized opaque archive is retained and rejected");
    large_model(original, 1536, 1);
    large_model(original, 1, 65535);
    large_model(original, 33, 65535, false);
    FtCostumeAssets invalid;
    check(!ftCostumeAssets_Load(original.data(), 0, "rig", nullptr, &invalid) &&
        invalid.import.status == HSD_DAT_HEADER && !invalid.arena && !invalid.joint,
        "malformed archive preserves import diagnostic");
    check(!ftCostumeAssets_Load(original.data(), original.size(), nullptr, nullptr, &invalid) &&
        invalid.import.status == HSD_DAT_SYMBOL && !invalid.arena && !invalid.joint,
        "missing root argument rejects cleanly");
    std::printf("PASS: %u capacity, atomicity and dead-storage checks\n", checks);
    return 0;
}
