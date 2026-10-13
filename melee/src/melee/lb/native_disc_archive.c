#include "native_disc_archive.h"

#ifdef MELEE_NATIVE

#include <dolphin/dvd.h>
#include <melee/gm/native_content_store.h>

#include "lbfile.h"

#include <malloc.h>
#include <stdio.h>

bool nativeDiscArchive_Read(const char* filename, void** bytes, size_t* size)
{
    if (filename == NULL || bytes == NULL || size == NULL) {
        return false;
    }
    char disc_path[64];
    int length = snprintf(disc_path, sizeof(disc_path), "/%s",
                          lbFileGetFullName(filename));
    if (length <= 0 || length >= sizeof(disc_path)) {
        return false;
    }
    DVDFileInfo file;
    if (!DVDOpen(disc_path, &file)) {
        return false;
    }
    size_t file_size = file.length;
    size_t padded = (file_size + 31) & ~(size_t) 31;
    void* data = file_size >= 32 &&
                         file_size <= NATIVE_CONTENT_MAX_PACKAGE_BYTES
                     ? _aligned_malloc(padded, 32)
                     : NULL;
    bool ok = data != NULL &&
              DVDReadPrio(&file, data, padded, 0, 2) >= (s32) file_size;
    ok = DVDClose(&file) && ok;
    if (!ok) {
        _aligned_free(data);
        return false;
    }
    *bytes = data;
    *size = file_size;
    return true;
}

#endif
