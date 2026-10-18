#include <melee/ft/native_fighter_costume_compact.h>
#include <melee/ft/native_asset_dat.h>
#include <cstdio>
#include <cstdlib>
#include <string>

static unsigned checks;
static void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", label); std::exit(1); }
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
    std::printf("PASS: %u capacity, atomicity and dead-storage checks\n", checks);
    return 0;
}
