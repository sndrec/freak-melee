#ifndef MELEE_FT_NATIVE_COSTUME_IMPORT_H
#define MELEE_FT_NATIVE_COSTUME_IMPORT_H

#ifdef MELEE_NATIVE

#include "native_costume_package.h"

#ifdef __cplusplus
extern "C" {
#endif

// Validate every member's DAT, then place the outfit in the installation's
// cache and collection. Call only at a menu transition, never during simulation.
bool nativeCostumeImport_Install(const char* root,
                                 const NativeCostumePackage* metadata,
                                 const NativeCostumePackageData* data,
                                 NativeContentId* id,
                                 NativeCostumeValidation* validation);

#ifdef __cplusplus
}
#endif

#endif
#endif
