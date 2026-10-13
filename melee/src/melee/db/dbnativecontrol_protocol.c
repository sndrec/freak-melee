#include "dbnativecontrol_protocol.h"

#include <stdio.h>
#include <string.h>

bool dbNativeControl_FormatChannel(
    char output[DB_NATIVE_CONTROL_CHANNEL_BYTES], const char* id)
{
    if (output == NULL) {
        return false;
    }
    if (id == NULL || id[0] == '\0') {
        memcpy(output, DB_NATIVE_CONTROL_CHANNEL,
               sizeof(DB_NATIVE_CONTROL_CHANNEL));
        return true;
    }
    size_t length = strnlen(id, DB_NATIVE_CONTROL_ID_BYTES + 1);
    if (length == 0 || length > DB_NATIVE_CONTROL_ID_BYTES) {
        return false;
    }
    for (size_t i = 0; i < length; ++i) {
        char c = id[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
        {
            return false;
        }
    }
    int written = snprintf(output, DB_NATIVE_CONTROL_CHANNEL_BYTES,
                           "%s-%s", DB_NATIVE_CONTROL_CHANNEL, id);
    return written > 0 && written < DB_NATIVE_CONTROL_CHANNEL_BYTES;
}
