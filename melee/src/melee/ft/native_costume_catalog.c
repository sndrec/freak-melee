#include "native_costume_catalog.h"
#include <melee/gm/native_match_launch.h>
#include <stdlib.h>
#include <string.h>

static NativeCostumeEntry entries[NATIVE_COSTUME_CATALOG_CAPACITY];
static size_t count;

static int compare(const void* a, const void* b)
{
    const NativeCostumeEntry* left = a;
    const NativeCostumeEntry* right = b;
    int order = strcmp(left->metadata.title, right->metadata.title);
    return order ? order : memcmp(&left->id, &right->id, sizeof(left->id));
}

bool nativeCostumeCatalog_Refresh(void)
{
    count = 0;
    char default_root[512];
    const char* root = nativeMatchLaunch_ContentRoot();
    if (!root) {
        if (!nativeContentStore_DefaultRoot(default_root, sizeof(default_root))) return false;
        root = default_root;
    }
    NativeContentId ids[NATIVE_COSTUME_CATALOG_CAPACITY];
    size_t installed = 0;
    bool complete = nativeContentStore_ListInstalled(root, ids, NATIVE_COSTUME_CATALOG_CAPACITY, &installed);
    for (size_t i = 0; i < installed; ++i) {
        uint8_t* bytes = NULL;
        size_t size;
        NativeCostumePackage metadata;
        NativeCostumePackageData data;
        if (nativeContentStore_Read(root, &ids[i], &bytes, &size) &&
            nativeContentStore_ParseCostume(bytes, size, &metadata, &data))
            entries[count++] = (NativeCostumeEntry){ids[i], metadata};
        free(bytes);
    }
    qsort(entries, count, sizeof(*entries), compare);
    return complete;
}

const NativeCostumeEntry* nativeCostumeCatalog_Entries(size_t* size)
{
    *size = count;
    return entries;
}
