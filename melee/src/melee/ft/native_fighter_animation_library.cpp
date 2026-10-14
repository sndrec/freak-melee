#include "native_asset_dat.h"
#include "native_fighter_animation_editor.h"
#include "native_fighter_model_editor.h"
#ifdef MELEE_NATIVE
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {
    void word(std::vector<uint8_t>& bytes, unsigned at, uint32_t value)
    {
        NativeAssetDat::be32(bytes.data() + at, value);
    }
    void floating(std::vector<uint8_t>& bytes, unsigned at, float value)
    {
        uint32_t bits;
        std::memcpy(&bits, &value, 4);
        word(bytes, at, bits);
    }
    bool clip_name(unsigned target, const char* name, char output[128])
    {
        if (!name || !*name || std::strlen(name) > 63) {
            return false;
        }
        for (const char* p = name; *p; ++p) {
            if (!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
                  (*p >= '0' && *p <= '9') || *p == '_'))
            {
                return false;
            }
        }
        return std::snprintf(output, 128, "Freak_ACTION_%s_%u_figatree", name,
                             target) < 128;
    }
    bool publish(NativeFighterProject* project, unsigned target,
                 const char* name, const std::vector<uint8_t>& clip)
    {
        if (!project || target >= project->action_count) {
            return false;
        }
        NativeAssetDat dat;
        if (!dat.open(project->fighter_dat, project->fighter_size)) {
            return false;
        }
        u32 root, table;
        const char* external;
        if (!HSD_DatFindRoot(&dat.original, project->fighter_root, &root) ||
            HSD_DatResolve(&dat.original, root + 12, &table, &external) !=
                HSD_DAT_REF_LOCAL ||
            !HSD_DatBytes(&dat.original, table, project->action_count * 24))
        {
            return false;
        }
        uint32_t string = dat.allocate(std::strlen(name) + 1);
        if (string == UINT32_MAX) {
            return false;
        }
        std::memcpy(dat.data.data() + string, name, std::strlen(name) + 1);
        size_t offset = (project->animation_size + 31) & ~size_t(31);
        if (offset + clip.size() > NATIVE_CONTENT_MAX_PACKAGE_BYTES) {
            return false;
        }
        std::vector<uint8_t> bank(project->animation_dat,
                                  project->animation_dat +
                                      project->animation_size);
        bank.resize(offset, 0xff);
        bank.insert(bank.end(), clip.begin(), clip.end());
        dat.link(table + target * 24, string);
        dat.word(table + target * 24 + 4, (uint32_t) offset);
        dat.word(table + target * 24 + 8, (uint32_t) clip.size());
        std::vector<uint8_t> fighter;
        if (!dat.finish(&fighter)) {
            return false;
        }
        NativeFighterProject candidate = *project;
        candidate.fighter_dat = fighter.data();
        candidate.fighter_size = fighter.size();
        candidate.animation_dat = bank.data();
        candidate.animation_size = bank.size();
        NativeFighterAnimationInfo info;
        if (!nativeFighterAnimation_Info(
                &candidate, { NativeFighterAnimation_Action, target }, &info))
        {
            return false;
        }
        uint8_t* f = (uint8_t*) std::malloc(fighter.size());
        uint8_t* b = (uint8_t*) std::malloc(bank.size());
        if (!f || !b) {
            std::free(f);
            std::free(b);
            return false;
        }
        std::memcpy(f, fighter.data(), fighter.size());
        std::memcpy(b, bank.data(), bank.size());
        std::free(project->fighter_dat);
        std::free(project->animation_dat);
        project->fighter_dat = f;
        project->fighter_size = fighter.size();
        project->animation_dat = b;
        project->animation_size = bank.size();
        return true;
    }
    std::vector<uint8_t> named_clip(const HSD_Dat& original, const char* name)
    {
        if (original.root_count != 1 || original.extern_count) {
            return {};
        }
        size_t body = 32 + original.data_size + original.relocation_count * 4;
        const uint8_t* bytes = original.data - 32;
        std::vector<uint8_t> result(bytes, bytes + body);
        result.resize(body + 8 + std::strlen(name) + 1);
        NativeAssetDat::be32(result.data() + body,
                             NativeAssetDat::read32(bytes + body));
        NativeAssetDat::be32(result.data() + body + 4, 0);
        std::memcpy(result.data() + body + 8, name, std::strlen(name) + 1);
        word(result, 0, (uint32_t) result.size());
        return result;
    }
}
extern "C" bool
nativeFighterAnimation_Duplicate(NativeFighterProject* p,
                                 NativeFighterAnimationClip source,
                                 unsigned target, const char* name)
{
    char symbol[128];
    NativeFighterAnimationInfo info;
    HSD_Dat dat;
    if (!clip_name(target, name, symbol) ||
        !nativeFighterAnimation_Info(p, source, &info) ||
        HSD_DatOpen(&dat, p->animation_dat + info.clip_offset,
                    info.clip_size) != HSD_DAT_OK)
    {
        return false;
    }
    auto clip = named_clip(dat, symbol);
    return !clip.empty() && publish(p, target, symbol, clip);
}
extern "C" bool nativeFighterAnimation_CreateRest(NativeFighterProject* p,
                                                  unsigned target,
                                                  const char* name,
                                                  unsigned frames)
{
    char symbol[128];
    if (!p || frames < 1 || frames > 1000 || !clip_name(target, name, symbol))
    {
        return false;
    }
    auto* model = (NativeFighterModelEditor*) std::calloc(
        1, sizeof(NativeFighterModelEditor));
    if (!model || !nativeFighterModel_Open(model, p)) {
        std::free(model);
        return false;
    }
    const unsigned count = model->joint_count;
    std::vector<uint8_t> data(20 + count + 1, 0);
    word(data, 0, 1);
    floating(data, 8, (float) frames);
    word(data, 12, 20);
    for (unsigned i = 4; i < count; ++i) {
        data[20 + i] = 9;
    }
    data[20 + count] = 255;
    data.resize((data.size() + 3) & ~size_t(3));
    const unsigned tracks = (unsigned) data.size();
    word(data, 16, tracks);
    data.resize(data.size() + (count > 4 ? count - 4 : 0) * 9 * 12);
    std::vector<uint32_t> relocs = { 12, 16 };
    unsigned t = 0;
    const unsigned channels[] = { 1, 2, 3, 5, 6, 7, 8, 9, 10 };
    for (unsigned j = 4; j < count; ++j) {
        for (unsigned c = 0; c < 9; ++c) {
            const auto& bone = model->joints[j];
            float value = c < 3   ? bone.rotation[c]
                          : c < 6 ? bone.position[c - 3]
                                  : bone.scale[c - 6];
            unsigned at = tracks + t++ * 12;
            unsigned stream = (unsigned) data.size();
            for (unsigned key = 0; key < 2; ++key) {
                data.push_back(2);
                uint32_t bits;
                std::memcpy(&bits, &value, 4);
                for (unsigned k = 0; k < 4; ++k) {
                    data.push_back((uint8_t) (bits >> (k * 8)));
                }
                unsigned wait = key ? 0 : frames;
                do {
                    uint8_t b = wait & 127;
                    wait >>= 7;
                    data.push_back(b | (wait ? 128 : 0));
                } while (wait);
            }
            NativeAssetDat::be16(data.data() + at,
                                 (uint16_t) (data.size() - stream));
            data[at + 4] = (uint8_t) channels[c];
            word(data, at + 8, stream);
            relocs.push_back(at + 8);
        }
    }
    std::free(model);
    data.resize((data.size() + 3) & ~size_t(3));
    std::vector<uint8_t> clip(32);
    clip.insert(clip.end(), data.begin(), data.end());
    for (uint32_t slot : relocs) {
        size_t at = clip.size();
        clip.resize(at + 4);
        word(clip, (unsigned) at, slot);
    }
    size_t at = clip.size();
    clip.resize(at + 8 + std::strlen(symbol) + 1, 0);
    std::memcpy(clip.data() + at + 8, symbol, std::strlen(symbol) + 1);
    word(clip, 0, (uint32_t) clip.size());
    word(clip, 4, (uint32_t) data.size());
    word(clip, 8, (uint32_t) relocs.size());
    word(clip, 12, 1);
    return publish(p, target, symbol, clip);
}
extern "C" bool
nativeFighterAnimation_RestoreDescriptors(NativeFighterProject* p,
                                          const void* saved, size_t bytes)
{
    NativeAssetDat destination;
    HSD_Dat source;
    u32 root, old_root;
    const char* external;
    if (!p || !destination.open(p->fighter_dat, p->fighter_size) ||
        HSD_DatOpen(&source, saved, bytes) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&destination.original, p->fighter_root, &root) ||
        !HSD_DatFindRoot(&source, p->fighter_root, &old_root))
    {
        return false;
    }
    unsigned counts[] = { p->action_count, p->demo_count };
    for (unsigned bank = 0; bank < 2; ++bank) {
        if (!counts[bank]) {
            continue;
        }
        u32 table, old_table;
        unsigned field = bank ? 20 : 12;
        if (HSD_DatResolve(&destination.original, root + field, &table,
                           &external) != HSD_DAT_REF_LOCAL ||
            HSD_DatResolve(&source, old_root + field, &old_table, &external) !=
                HSD_DAT_REF_LOCAL ||
            !HSD_DatBytes(&destination.original, table, counts[bank] * 24) ||
            !HSD_DatBytes(&source, old_table, counts[bank] * 24))
        {
            return false;
        }
        for (unsigned i = 0; i < counts[bank]; ++i) {
            u32 record = table + i * 24, old_record = old_table + i * 24,
                string;
            HSD_DatReferenceKind kind =
                HSD_DatResolve(&source, old_record, &string, &external);
            if (kind == HSD_DAT_REF_NULL) {
                destination.link(record, 0);
            } else if (kind == HSD_DAT_REF_LOCAL) {
                const char* name =
                    (const char*) HSD_DatBytes(&source, string, 1);
                if (!name || !std::memchr(name, 0, source.data_size - string))
                {
                    return false;
                }
                u32 current;
                const char* existing = nullptr;
                if (HSD_DatResolve(&destination.original, record, &current,
                                   &external) == HSD_DAT_REF_LOCAL)
                {
                    existing = (const char*) HSD_DatBytes(
                        &destination.original, current, 1);
                }
                if (!existing ||
                    !std::memchr(existing, 0,
                                 destination.original.data_size - current) ||
                    std::strcmp(name, existing))
                {
                    u32 added = destination.allocate(std::strlen(name) + 1);
                    if (added == UINT32_MAX) {
                        return false;
                    }
                    std::memcpy(destination.data.data() + added, name,
                                std::strlen(name) + 1);
                    destination.link(record, added);
                }
            } else {
                return false;
            }
            destination.word(record + 4, NativeAssetDat::read32(
                                             source.data + old_record + 4));
            destination.word(record + 8, NativeAssetDat::read32(
                                             source.data + old_record + 8));
        }
    }
    std::vector<uint8_t> result;
    if (!destination.finish(&result)) {
        return false;
    }
    auto* copy = (uint8_t*) std::malloc(result.size());
    if (!copy) {
        return false;
    }
    std::memcpy(copy, result.data(), result.size());
    std::free(p->fighter_dat);
    p->fighter_dat = copy;
    p->fighter_size = result.size();
    return true;
}
#endif
