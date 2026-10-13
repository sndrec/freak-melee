#include "plcommon_data.h"

#include "types.h"

#include <sysdolphin/baselib/dat.h>

#include <stddef.h>
#include <string.h>

bool plCommonData_Import(const void* bytes, size_t size,
                         pl_804D6470_t* output)
{
    HSD_Dat dat;
    const char* external;
    u32 data_offset;
    u32 root_offset;
    u32 source_offset;
    u32 value;

    if (output == NULL || sizeof(*output) != PL_COMMON_DATA_SIZE ||
        HSD_DatOpen(&dat, bytes, size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&dat, "plLoadCommonData", &root_offset) ||
        HSD_DatResolve(&dat, root_offset, &data_offset, &external) !=
            HSD_DAT_REF_LOCAL ||
        HSD_DatBytes(&dat, data_offset, sizeof(*output)) == NULL)
    {
        return false;
    }

    for (source_offset = 0; source_offset < sizeof(*output);
         source_offset += sizeof(value))
    {
        u8* destination = (u8*) output + source_offset;
        if (source_offset == offsetof(pl_804D6470_t, xC0)) {
            memcpy(destination, HSD_DatBytes(&dat, data_offset + source_offset,
                                             sizeof(output->xC0)),
                   sizeof(output->xC0));
        } else {
            if (!HSD_DatReadU32(&dat, data_offset + source_offset, &value)) {
                return false;
            }
            memcpy(destination, &value, sizeof(value));
        }
    }
    return true;
}
