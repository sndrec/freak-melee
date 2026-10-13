#ifndef MELEE_FT_NATIVE_SOURCE_RESOURCES_H
#define MELEE_FT_NATIVE_SOURCE_RESOURCES_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
    NATIVE_SOURCE_RESOURCE_ENTRIES=64,
    NATIVE_SOURCE_RESOURCE_NAME_BYTES=128,
    NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES=8*1024*1024,
    NATIVE_SOURCE_RESOURCE_BLOB_BYTES=4*1024*1024,
    NATIVE_SOURCE_RESOURCE_PNG_BYTES=262144,
    NATIVE_SOURCE_RESOURCE_PNG_DIMENSION=1024,
};
typedef enum NativeSourceResourceKind {
    NativeSourceResource_Blob=1,
    NativeSourceResource_PNG=2,
} NativeSourceResourceKind;
typedef struct NativeSourceResourceInput {
    const char* name;
    const void* bytes;
    size_t size;
    NativeSourceResourceKind kind;
} NativeSourceResourceInput;
typedef struct NativeSourceResourceEntry {
    const char* name;
    const uint8_t* bytes;
    uint32_t size;
    NativeSourceResourceKind kind;
    uint16_t width, height; // PNG IHDR dimensions; zero for opaque blobs.
} NativeSourceResourceEntry;
typedef struct NativeSourceResources {
    const uint8_t* bytes;
    size_t size;
    uint32_t count;
    NativeSourceResourceEntry entries[NATIVE_SOURCE_RESOURCE_ENTRIES];
} NativeSourceResources;

#ifdef __cplusplus
extern "C" {
#endif
// Exact, case-sensitive logical names. Names are safe relative paths; aliases
// differing only in ASCII case cannot coexist in one bundle.
bool nativeSourceResources_NameValid(const char* name);
// No allocation or file access. Write sorts the directory without changing its
// inputs. Invalid calls preserve destination/written/output views. Diagnostics
// are optional caller-owned storage and must not overlap inputs or outputs.
bool nativeSourceResources_Measure(const NativeSourceResourceInput*,uint32_t count,
    size_t* bytes,char* error,size_t error_capacity);
bool nativeSourceResources_Write(const NativeSourceResourceInput*,uint32_t count,
    void* output,size_t capacity,size_t* written,char* error,size_t error_capacity);
// Every view/name/payload borrows the caller's immutable pinned bundle bytes.
// NULL+0 is an optional absent bundle and succeeds with a wholly zero view.
bool nativeSourceResources_Read(const void* bytes,size_t size,
    NativeSourceResources* output,char* error,size_t error_capacity);
const NativeSourceResourceEntry* nativeSourceResources_Find(
    const NativeSourceResources*,const char* name,NativeSourceResourceKind);
// PNG validation covers framing, CRC, ordering and bounded legal IHDR fields.
// Pixel decompression/decoder success is a separate mandatory activation check.
bool nativeSourceResources_PNG(const void* bytes,size_t size,
    uint16_t* width,uint16_t* height,char* error,size_t error_capacity);
#ifdef __cplusplus
}
#endif
#endif
