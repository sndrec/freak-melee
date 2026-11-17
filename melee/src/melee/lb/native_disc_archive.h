#ifndef MELEE_LB_NATIVE_DISC_ARCHIVE_H
#define MELEE_LB_NATIVE_DISC_ARCHIVE_H

#ifdef MELEE_NATIVE

#include <stdbool.h>
#include <stddef.h>

// Read one archive from the currently open game disc. Release the returned
// buffer with _aligned_free after the caller has copied or imported its bytes.
#ifdef __cplusplus
extern "C" {
#endif
bool nativeDiscArchive_Read(const char* filename, void** bytes, size_t* size);
#ifdef __cplusplus
}
#endif

#endif
#endif
