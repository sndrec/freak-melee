#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct NativeFileIdentity {
    uint32_t volume, index_hi, index_lo, created_hi, created_lo;
    bool valid;
} NativeFileIdentity;
typedef enum NativeFileExisting {
    NATIVE_FILE_PRESERVE, NATIVE_FILE_REPLACE
} NativeFileExisting;
typedef enum NativeFileResult {
    NATIVE_FILE_PUBLISHED, NATIVE_FILE_INVALID, NATIVE_FILE_UNAVAILABLE, NATIVE_FILE_FAILED
} NativeFileResult;
// Publish a complete file by renaming its open handle in the destination directory.
NativeFileResult nativeFile_PublishWide(const wchar_t* path, const void* bytes, size_t size,
                                        NativeFileExisting existing, NativeFileIdentity* identity);
bool nativeFile_PublishLocal(const char* path, const void* bytes, size_t size,
                             NativeFileExisting existing, NativeFileIdentity* identity);
void nativeFile_RemoveOwned(const char* path, const NativeFileIdentity* identity);
#ifdef __cplusplus
}
#endif
