#pragma once

#include "../../../extern/cgltf/cgltf.h"
#include <cstdint>
#include <memory>
#include <vector>

struct NativeGltfSource {
    cgltf_data* data = nullptr;
    std::unique_ptr<uint8_t[]> document;
    size_t allocated = 0;

    ~NativeGltfSource() { cgltf_free(data); }
    bool open(const char* path, char* error, size_t capacity);
};

bool nativeGltf_ReadImage(const char* document_path, const cgltf_image& image,
                          std::vector<uint8_t>& encoded);
