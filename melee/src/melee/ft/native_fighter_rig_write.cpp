#include "native_fighter_rig.h"
#include "native_asset_dat.h"
#include <cstdio>
#include <cstdlib>

extern "C" bool nativeFighterRig_Write(NativeFighterProject* project,
    const NativeFighterRig* source, char* error, size_t capacity)
{
    auto fail = [&](const char* message) {
        if (error && capacity) std::snprintf(error, capacity, "%s", message);
        return false;
    };
    if (!project || !source) return fail("Bone-ID writing needs a draft and rig.");
    NativeFighterRig rig = *source;
    if (!nativeFighterRig_Index(&rig, error, capacity)) return false;
    NativeAssetDat dat;
    if (!dat.open(project->fighter_dat, project->fighter_size)) return fail("Could not open the fighter DAT.");
    NativeFighterRig before;
    // Read the original count, which can differ during an insertion/removal.
    u32 root, table = 0, names = 0;
    if (HSD_DatFindRoot(&dat.original, NATIVE_RIG_ROOT, &root)) {
        u32 count; const char* external;
        if (!HSD_DatReadU32(&dat.original, root + 8, &count) ||
            !nativeFighterRig_Read(&dat.original, (unsigned)count, &before, error, capacity) ||
            HSD_DatResolve(&dat.original, root + 16, &table, &external) != HSD_DAT_REF_LOCAL ||
            !HSD_DatBytes(&dat.original, table, MAX_FT_PARTS * 12))
            return fail("The existing bone-ID allocation is invalid.");
        // The writer owns a full fixed-capacity name slab immediately after
        // its entry table. Existing class names and authoring labels stay put.
        names = table + MAX_FT_PARTS * 12;
        if (!HSD_DatBytes(&dat.original, names, MAX_FT_PARTS * 64))
            return fail("The bone-name allocation is truncated.");
    } else {
        root = dat.allocate(20);
        table = dat.allocate(MAX_FT_PARTS * (12 + 64));
        if (root == UINT32_MAX || table == UINT32_MAX || !dat.add_root(NATIVE_RIG_ROOT, root))
            return fail("Could not allocate the bone-ID table.");
        names = table + MAX_FT_PARTS * 12;
    }
    dat.word(root, 0x52494742); dat.word(root + 4, rig.next_id);
    dat.word(root + 8, rig.count); dat.word(root + 12, rig.mapped);
    dat.link_local(root + 16, table);
    for (unsigned i = 0; i < MAX_FT_PARTS; ++i) {
        const uint32_t at = (uint32_t)table + i * 12;
        dat.word(at, rig.ids[i]);
        dat.half(at + 4, (uint16_t)i);
        dat.half(at + 6, rig.flags[i]);
        std::memset(dat.data.data() + names + i * 64, 0, 64);
        if (rig.ids[i] != UINT32_MAX && *rig.names[i]) {
            std::memcpy(dat.data.data() + names + i * 64, rig.names[i], std::strlen(rig.names[i]));
            dat.link_local(at + 8, (uint32_t)names + i * 64);
        } else dat.link(at + 8, 0);
    }
    std::vector<uint8_t> bytes;
    if (!dat.finish(&bytes)) return fail("Could not finish the fighter's bone-ID table.");
    HSD_Dat verified; NativeFighterRig admitted;
    if (HSD_DatOpen(&verified, bytes.data(), bytes.size()) != HSD_DAT_OK ||
        !nativeFighterRig_Read(&verified, rig.count, &admitted, error, capacity)) return false;
    auto* copy = (uint8_t*)std::malloc(bytes.size());
    if (!copy) return fail("Could not allocate the updated fighter DAT.");
    std::memcpy(copy, bytes.data(), bytes.size());
    std::free(project->fighter_dat); project->fighter_dat = copy; project->fighter_size = bytes.size();
    return true;
}
