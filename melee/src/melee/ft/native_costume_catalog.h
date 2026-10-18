#ifndef MELEE_FT_NATIVE_COSTUME_CATALOG_H
#define MELEE_FT_NATIVE_COSTUME_CATALOG_H
#include <melee/gm/native_content_store.h>
#ifdef __cplusplus
extern "C" {
#endif
enum { NATIVE_COSTUME_CATALOG_CAPACITY = 512 };
typedef struct NativeCostumeEntry {
    NativeContentId id;
    NativeCostumePackage metadata;
} NativeCostumeEntry;
// Refresh at a collection or menu transition. Entries contain copied metadata.
bool nativeCostumeCatalog_Refresh(void);
const NativeCostumeEntry* nativeCostumeCatalog_Entries(size_t* count);
#ifdef __cplusplus
}
#endif
#endif
