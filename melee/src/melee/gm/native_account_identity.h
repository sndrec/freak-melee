#pragma once
#include <stdbool.h>
#include <string.h>

static inline bool nativeAccountIdentity_Valid(const char* id, const char* handle)
{
    if (!*id && !*handle) return true;
    if (strlen(id) != 43) return false;
    size_t length = strlen(handle);
    if (length < 3 || length > 24) return false;
    for (unsigned i = 0; i < 43; ++i)
        if (!(id[i] >= 'a' && id[i] <= 'z') && !(id[i] >= 'A' && id[i] <= 'Z') &&
            !(id[i] >= '0' && id[i] <= '9') && id[i] != '_' && id[i] != '-') return false;
    for (size_t i = 0; i < length; ++i)
        if (!(handle[i] >= 'a' && handle[i] <= 'z') && !(handle[i] >= '0' && handle[i] <= '9') && handle[i] != '_') return false;
    return true;
}
