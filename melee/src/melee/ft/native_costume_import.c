#include "native_costume_import.h"

#ifdef MELEE_NATIVE

#include <stdlib.h>

bool nativeCostumeImport_Install(const char* root,
                                 const NativeCostumePackage* metadata,
                                 const void* dat, size_t dat_size,
                                 NativeContentId* id,
                                 NativeCostumeValidation* validation)
{
    if (root == NULL || metadata == NULL || dat == NULL || id == NULL ||
        !nativeCostumePackage_Validate(metadata, dat, dat_size, root,
                                       validation)) {
        return false;
    }
    uint8_t* package = NULL;
    size_t package_size = 0;
    NativeContentId cached_id;
    bool result = nativeContentStore_BuildCostume(
                      metadata, dat, dat_size, &package, &package_size, id) &&
                  nativeContentStore_Cache(root, package, package_size,
                                           id, &cached_id) &&
                  nativeContentStore_Install(root, id);
    free(package);
    return result;
}

#endif
