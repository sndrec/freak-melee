#include "native_fighter_gltf_source.h"
#include "native_fighter_gltf_import.h"

#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

namespace {
constexpr size_t SourceLimit = 64 * 1024 * 1024;
constexpr size_t ImageLimit = 8 * 1024 * 1024;
constexpr size_t ParserLimit = 192 * 1024 * 1024;

struct alignas(std::max_align_t) Allocation { size_t size; };
void* allocate(void* context, size_t size)
{
    auto& used = *static_cast<size_t*>(context);
    if (size > ParserLimit - used || sizeof(Allocation) > ParserLimit - used - size)
        return nullptr;
    auto* block = static_cast<Allocation*>(std::malloc(sizeof(Allocation) + size));
    if (!block) return nullptr;
    block->size = size + sizeof(Allocation);
    used += block->size;
    return block + 1;
}

void release(void* context, void* ptr)
{
    if (!ptr) return;
    auto* block = static_cast<Allocation*>(ptr) - 1;
    *static_cast<size_t*>(context) -= block->size;
    std::free(block);
}

std::wstring wide_path(const char* text)
{
    if (!text || !*text || strnlen(text, 4097) > 4096) return {};
    int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
    if (!count) return {};
    std::wstring wide(count, L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, wide.data(), count)) return {};
    wide.pop_back();
    return wide;
}

struct File {
    HANDLE handle = INVALID_HANDLE_VALUE;
    ~File() { if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }

    bool open(const std::filesystem::path& path, DWORD flags)
    {
        handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                             OPEN_EXISTING, flags, nullptr);
        return handle != INVALID_HANDLE_VALUE && GetFileType(handle) == FILE_TYPE_DISK;
    }
    std::wstring final_path() const
    {
        DWORD capacity = GetFinalPathNameByHandleW(handle, nullptr, 0, FILE_NAME_NORMALIZED);
        if (!capacity || capacity > 32768) return {};
        std::wstring path(capacity, L'\0');
        DWORD size = GetFinalPathNameByHandleW(handle, path.data(), capacity, FILE_NAME_NORMALIZED);
        if (!size || size >= capacity) return {};
        path.resize(size);
        return path;
    }
    size_t size(size_t maximum) const
    {
        LARGE_INTEGER length;
        if (!GetFileSizeEx(handle, &length) || length.QuadPart <= 0 ||
            static_cast<uint64_t>(length.QuadPart) > maximum) return 0;
        return static_cast<size_t>(length.QuadPart);
    }
    bool read(void* bytes, size_t length) const
    {
        DWORD got = 0;
        return ReadFile(handle, bytes, static_cast<DWORD>(length), &got, nullptr) && got == length;
    }
};

bool open_dependency(File& file, const char* document, const char* uri)
{
    if (!uri || strnlen(uri, 4097) > 4096) return false;
    std::string decoded(uri);
    size_t n = cgltf_decode_uri(decoded.data());
    decoded.resize(n);
    if (decoded.find('\0') != std::string::npos) return false;
    std::wstring name = wide_path(decoded.c_str());
    if (name.empty() || name.front() == L'/' || name.front() == L'\\' ||
        name.find_first_of(L":<>\"|?*") != name.npos) return false;
    for (auto ch : name) if (ch < 32) return false;
    std::filesystem::path base = std::filesystem::path(wide_path(document)).parent_path();
    File directory;
    if (!directory.open(base.empty() ? std::filesystem::path(L".") : base,
                        FILE_FLAG_BACKUP_SEMANTICS)) return false;
    auto root = directory.final_path();
    if (root.empty()) return false;
    if (root.back() != L'\\') root += L'\\';
    std::filesystem::path target(root);
    auto relative = std::filesystem::path(name).lexically_normal();
    if (relative.empty() || relative.has_root_path() || !relative.has_filename()) return false;
    for (const auto& component : relative) {
        const auto& part = component.native();
        if (part.empty() || part.back() == L'.' || part.back() == L' ') return false;
        target /= component;
        DWORD attrs = GetFileAttributesW(target.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    }
    if (!file.open(target, FILE_FLAG_OPEN_REPARSE_POINT)) return false;
    BY_HANDLE_FILE_INFORMATION info;
    if (!GetFileInformationByHandle(file.handle, &info) ||
        (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return false;
    // Compare the opened handle before reading, including the directory separator.
    return file.final_path().compare(0, root.size(), root) == 0;
}

bool base64_size(const char* uri, const char*& payload, size_t& bytes)
{
    const char* comma = std::strchr(uri, ',');
    if (!comma || comma - uri < 7 || std::memcmp(comma - 7, ";base64", 7)) return false;
    payload = comma + 1;
    size_t chars = std::strlen(payload);
    if (!chars || chars % 4) return false;
    size_t padding = (payload[chars-1] == '=') + (payload[chars-2] == '=');
    bytes = chars / 4 * 3 - padding;
    return true;
}

bool fits_view(const cgltf_buffer_view* view, size_t offset, size_t count,
               size_t stride, size_t element)
{
    return view && count && element && stride >= element && offset <= view->size &&
           element <= view->size - offset &&
           count - 1 <= (view->size - offset - element) / stride;
}

bool valid_views(const cgltf_data& data)
{
    if (data.nodes_count > 4096 || data.meshes_count > NATIVE_GLTF_MAX_PRIMITIVES) return false;
    size_t primitives = 0;
    size_t index_budget = SourceLimit;
    for (size_t i = 0; i < data.meshes_count; ++i) {
        const auto& mesh = data.meshes[i];
        if (mesh.primitives_count > NATIVE_GLTF_MAX_PRIMITIVES - primitives) return false;
        primitives += mesh.primitives_count;
        for (size_t j = 0; j < mesh.primitives_count; ++j) {
            const auto* indices = mesh.primitives[j].indices;
            if (indices) {
                if (indices->count > index_budget) return false;
                index_budget -= indices->count;
            }
        }
    }
    for (size_t i = 0; i < data.buffer_views_count; ++i) {
        const auto& v = data.buffer_views[i];
        if (!v.buffer || !v.buffer->data || v.offset > v.buffer->size ||
            v.size > v.buffer->size - v.offset) return false;
        if (v.has_meshopt_compression) {
            const auto& m = v.meshopt_compression;
            if (!m.buffer || m.offset > m.buffer->size || m.size > m.buffer->size - m.offset ||
                !m.stride || v.size % m.stride || v.size / m.stride != m.count) return false;
        }
    }
    for (size_t i = 0; i < data.accessors_count; ++i) {
        const auto& a = data.accessors[i];
        size_t element = cgltf_calc_size(a.type, a.component_type);
        if (!element || !a.count || a.count > SourceLimit / element ||
            (a.buffer_view && !fits_view(a.buffer_view, a.offset, a.count, a.stride, element))) return false;
        if (a.is_sparse) {
            const auto& s = a.sparse;
            size_t stride = cgltf_component_size(s.indices_component_type);
            if (s.count > a.count || s.count > index_budget ||
                !fits_view(s.indices_buffer_view, s.indices_byte_offset,
                s.count, stride, stride) || !fits_view(s.values_buffer_view, s.values_byte_offset,
                s.count, element, element)) return false;
            index_budget -= s.count;
        }
    }
    return true;
}
} // namespace

bool NativeGltfSource::open(const char* path, char* error, size_t capacity)
{
    auto fail = [&](const char* message) {
        if (error && capacity) std::snprintf(error, capacity, "%s", message);
        return false;
    };
    File source;
    if (!source.open(wide_path(path), FILE_ATTRIBUTE_NORMAL)) return fail("Could not open the selected model.");
    size_t size = source.size(SourceLimit);
    if (!size) return fail("Select a nonempty glTF or GLB file up to 64 MiB.");
    document.reset(new(std::nothrow) uint8_t[size]);
    cgltf_options options{};
    options.memory = {allocate, release, &allocated};
    if (!document || !source.read(document.get(), size) ||
        cgltf_parse(&options, document.get(), size, &data) != cgltf_result_success)
        return fail("Could not read the glTF document within the import memory limit.");
    size_t remaining = SourceLimit;
    for (size_t i = 0; i < data->buffers_count; ++i) {
        auto& b = data->buffers[i];
        if (!b.size || b.size > remaining) return fail("Model buffers exceed the 64 MiB import limit.");
        remaining -= b.size;
        if (!b.uri) {
            if (i || !data->bin || b.size > data->bin_size) return fail("The GLB binary buffer is incomplete.");
        } else if (!std::strncmp(b.uri, "data:", 5)) {
            const char* payload;
            size_t actual;
            if (!base64_size(b.uri, payload, actual) || actual != b.size)
                return fail("An embedded model buffer differs from its declared size.");
        } else {
            File external;
            if (!open_dependency(external, path, b.uri) || external.size(SourceLimit) < b.size)
                return fail("Model buffers must be readable files inside the selected model's folder.");
            b.data = allocate(&allocated, b.size);
            b.data_free_method = cgltf_data_free_method_memory_free;
            if (!b.data || !external.read(b.data, b.size)) return fail("Could not read a model buffer.");
        }
    }
    if (cgltf_load_buffers(&options, data, nullptr) != cgltf_result_success ||
        !valid_views(*data) || cgltf_validate(data) != cgltf_result_success)
        return fail("The selected model has invalid buffers or accessors.");
    return true;
}

bool nativeGltf_ReadImage(const char* path, const cgltf_image& image, std::vector<uint8_t>& bytes)
{
    if (image.buffer_view) {
        const auto& v = *image.buffer_view;
        if (!v.size || v.size > ImageLimit) return false;
        const uint8_t* data = cgltf_buffer_view_data(&v);
        if (!data) return false;
        bytes.assign(data, data + v.size);
        return true;
    }
    if (!image.uri) return false;
    if (!std::strncmp(image.uri, "data:", 5)) {
        const char* payload;
        size_t size;
        if (!base64_size(image.uri, payload, size) || size > ImageLimit) return false;
        cgltf_options options{};
        void* decoded = nullptr;
        if (cgltf_load_buffer_base64(&options, size, payload, &decoded) != cgltf_result_success) return false;
        auto* data = static_cast<const uint8_t*>(decoded);
        bytes.assign(data, data + size);
        std::free(decoded);
        return true;
    }
    File external;
    if (!open_dependency(external, path, image.uri)) return false;
    size_t size = external.size(ImageLimit);
    if (!size) return false;
    bytes.resize(size);
    return external.read(bytes.data(), size);
}
#endif
