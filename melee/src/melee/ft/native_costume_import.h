#ifndef MELEE_FT_NATIVE_COSTUME_IMPORT_H
#define MELEE_FT_NATIVE_COSTUME_IMPORT_H

#ifdef MELEE_NATIVE

#include "native_costume_package.h"

#ifdef __cplusplus
extern "C" {
#endif

// Validate an original-format DAT, then place its package in the installation's
// cache and collection. Call only at a menu transition, never during simulation.
bool nativeCostumeImport_Install(const char* root,
                                 const NativeCostumePackage* metadata,
                                 const void* dat, size_t dat_size,
                                 NativeContentId* id,
                                 NativeCostumeValidation* validation);

#ifdef __cplusplus
}
#endif

#endif
#endif
