#ifndef MELEE_NATIVE_ASSET_DAT_H
#define MELEE_NATIVE_ASSET_DAT_H
#include <melee/ft/native_fighter_project.h>
extern "C" {
#include <sysdolphin/baselib/dat.h>
}
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
// Mutable asset transaction. Existing offsets stay stable; finish rebuilds
// relocation/root tables and validates the resulting archive before
// publishing.
struct NativeAssetDat {
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
    struct Root { uint32_t offset; std::string name; };
    std::vector<Root> added_roots;

    bool add_root(const char* name, uint32_t offset)
    {
        if (!name || !*name || std::strlen(name) > 127 || offset >= data.size()) return false;
        for (unsigned external = 0; external < 2; ++external)
            for (u32 i = 0; i < (external ? original.extern_count : original.root_count); ++i) {
                u32 at; const char* symbol;
                if (!HSD_DatSymbol(&original, external != 0, i, &at, &symbol) ||
                    !std::strcmp(name, symbol)) return false;
            }
        for (const auto& r : added_roots) if (r.name == name) return false;
        added_roots.push_back({offset, name});
        return true;
    }

    bool open(const uint8_t* bytes, size_t byte_count)
    {
        source = bytes;
        source_size = byte_count;
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

    // DAT offset zero is a valid local target when the pointer has a relocation.
    // Use this for resolved source references; link(..., 0) writes a null pointer.
    void link_local(uint32_t slot, uint32_t target)
    {
        word(slot, target);
        relocations.push_back(slot);
        detached_relocations.erase(
            std::remove(detached_relocations.begin(), detached_relocations.end(), slot),
            detached_relocations.end());
    }

    bool finish(std::vector<uint8_t>* result,
                 size_t max_output = NATIVE_CONTENT_MAX_ASSET_BYTES)
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
        size_t tail = source_size - tail_start;
        for (const auto& r : added_roots) tail += 8 + r.name.size() + 1;
        const size_t size = 32 + data.size() + relocations.size() * 4 + tail;
        if (size > UINT32_MAX || size > max_output) {
            return false;
        }
        result->resize(size);
        uint8_t* output = result->data();
        std::memcpy(output, source, 32);
        be32(output, (uint32_t) size);
        be32(output + 4, (uint32_t) data.size());
        be32(output + 8, (uint32_t) relocations.size());
        be32(output + 12, original.root_count + (uint32_t)added_roots.size());
        std::memcpy(output + 32, data.data(), data.size());
        uint8_t* table = output + 32 + data.size();
        for (size_t i = 0; i < relocations.size(); ++i) {
            be32(table + i * 4, relocations[i]);
        }
        uint8_t* footer = table + relocations.size() * 4;
        const size_t root_bytes = original.root_count * 8;
        const size_t external_bytes = original.extern_count * 8;
        std::memcpy(footer, original.roots, root_bytes);
        size_t name_at = original.strings_size;
        for (size_t i = 0; i < added_roots.size(); ++i) {
            be32(footer + root_bytes + i * 8, added_roots[i].offset);
            be32(footer + root_bytes + i * 8 + 4, (uint32_t)name_at);
            name_at += added_roots[i].name.size() + 1;
        }
        footer += root_bytes + added_roots.size() * 8;
        std::memcpy(footer, original.externs, external_bytes);
        footer += external_bytes;
        std::memcpy(footer, original.strings, original.strings_size);
        footer += original.strings_size;
        for (const auto& r : added_roots) {
            std::memcpy(footer, r.name.c_str(), r.name.size() + 1);
            footer += r.name.size() + 1;
        }
        HSD_Dat verified;
        return HSD_DatOpen(&verified, output, size) == HSD_DAT_OK;
    }
};

#endif
