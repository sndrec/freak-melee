#ifndef MELEE_NATIVE_FIGHTER_COSTUME_DAT_H
#define MELEE_NATIVE_FIGHTER_COSTUME_DAT_H
#include <melee/ft/native_fighter_project.h>
extern "C" {
#include <sysdolphin/baselib/dat.h>
}
#include <algorithm>
#include <cstring>
#include <vector>
// Mutable costume transaction. Existing offsets stay stable; finish rebuilds
// relocation/root tables and validates the resulting archive before
// publishing.
struct NativeCostumeDat {
    static void be32(uint8_t* p, uint32_t n)
    {
        p[0] = n >> 24;
        p[1] = n >> 16;
        p[2] = n >> 8;
        p[3] = n;
    }
    static void be16(uint8_t* p, uint16_t n)
    {
        p[0] = n >> 8;
        p[1] = n;
    }
    static uint32_t read32(const uint8_t* p)
    {
        return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 |
               uint32_t(p[2]) << 8 | p[3];
    }
    HSD_Dat original = {};
    const uint8_t* source = nullptr;
    size_t source_size = 0;
    std::vector<uint8_t> data;
    std::vector<uint32_t> relocations;
    std::vector<uint32_t> detached_relocations;

    bool open(const NativeFighterProject* project)
    {
        source = project->costume_dat;
        source_size = project->costume_size;
        if (HSD_DatOpen(&original, source, source_size) != HSD_DAT_OK) {
            return false;
        }
        data.assign(original.data, original.data + original.data_size);
        for (u32 i = 0; i < original.relocation_count; ++i) {
            u32 slot;
            HSD_DatRelocation(&original, i, &slot);
            relocations.push_back(slot);
        }
        return true;
    }

    uint32_t allocate(size_t size, size_t alignment = 4)
    {
        size_t start = (data.size() + alignment - 1) & ~(alignment - 1);
        if (start > UINT32_MAX || size > UINT32_MAX - start) {
            return UINT32_MAX;
        }
        data.resize(start + size, 0);
        return (uint32_t) start;
    }

    void word(uint32_t offset, uint32_t value)
    {
        be32(data.data() + offset, value);
    }

    void half(uint32_t offset, uint16_t value)
    {
        be16(data.data() + offset, value);
    }

    void link(uint32_t slot, uint32_t target)
    {
        word(slot, target);
        if (target) {
            relocations.push_back(slot);
        } else {
            detached_relocations.push_back(slot);
        }
    }

    bool finish(std::vector<uint8_t>* result)
    {
        std::sort(relocations.begin(), relocations.end());
        relocations.erase(std::unique(relocations.begin(), relocations.end()),
                          relocations.end());
        relocations.erase(
            std::remove_if(relocations.begin(), relocations.end(),
                           [&](uint32_t slot) {
                               return std::find(detached_relocations.begin(),
                                                detached_relocations.end(),
                                                slot) !=
                                          detached_relocations.end() &&
                                      read32(data.data() + slot) == 0;
                           }),
            relocations.end());
        const size_t tail_start =
            32 + original.data_size + (size_t) original.relocation_count * 4;
        const size_t tail = source_size - tail_start;
        const size_t size = 32 + data.size() + relocations.size() * 4 + tail;
        if (size > UINT32_MAX || size > NATIVE_CONTENT_MAX_PACKAGE_BYTES) {
            return false;
        }
        result->resize(size);
        uint8_t* output = result->data();
        std::memcpy(output, source, 32);
        be32(output, (uint32_t) size);
        be32(output + 4, (uint32_t) data.size());
        be32(output + 8, (uint32_t) relocations.size());
        std::memcpy(output + 32, data.data(), data.size());
        uint8_t* table = output + 32 + data.size();
        for (size_t i = 0; i < relocations.size(); ++i) {
            be32(table + i * 4, relocations[i]);
        }
        std::memcpy(table + relocations.size() * 4, source + tail_start, tail);
        HSD_Dat verified;
        return HSD_DatOpen(&verified, output, size) == HSD_DAT_OK;
    }
};

#endif
