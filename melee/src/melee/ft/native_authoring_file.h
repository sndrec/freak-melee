#pragma once
#include "native_authoring_reply.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace NativeAuthoring {
bool read_file(const char* path, size_t limit, std::vector<uint8_t>& bytes);
// Creates a new destination only after all bytes have been written and flushed.
bool export_file(const char* path, const void* bytes, size_t size, Reply& reply);
}
