#include "native_fighter_pose.h"
#include <stddef.h>

bool nativeFighterPose_PathValid(const char* path)
{
    if (path == NULL) return false;
    unsigned bytes = 0, depth = 0;
    while (path[bytes]) {
        if (++depth > 12) return false;
        unsigned ordinal = 0, digits = 0;
        while (bytes < 64 && path[bytes] >= '0' && path[bytes] <= '9') {
            ordinal = ordinal * 10 + (unsigned)(path[bytes++] - '0');
            if (++digits > 3 || ordinal > 255) return false;
        }
        if (!digits || bytes >= 64) return false;
        if (path[bytes] == 0) return true;
        if (path[bytes++] != '/' || bytes >= 64 || path[bytes] == 0) return false;
    }
    return true;
}
