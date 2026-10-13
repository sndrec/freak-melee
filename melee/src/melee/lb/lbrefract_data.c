#include "lbrefract_data.h"

#include <sysdolphin/baselib/dat.h>

bool lbRefractData_Import(const void* bytes, size_t size,
                          lbRefractData* output, f32* parameters,
                          size_t parameter_capacity)
{
    HSD_Dat dat;
    const u8* count;
    const char* external;
    u32 parameter_offset;
    u32 root;
    size_t parameter_count;
    size_t i;

    if (output == NULL || parameters == NULL ||
        HSD_DatOpen(&dat, bytes, size) != HSD_DAT_OK ||
        !HSD_DatFindRoot(&dat, "lbRefData", &root) ||
        (count = HSD_DatBytes(&dat, root, 1)) == NULL || *count == 0 ||
        HSD_DatResolve(&dat, root + 4, &parameter_offset, &external) !=
            HSD_DAT_REF_LOCAL)
    {
        return false;
    }

    parameter_count = (size_t) *count * 2;
    if (parameter_count > parameter_capacity ||
        HSD_DatBytes(&dat, parameter_offset,
                     parameter_count * sizeof(*parameters)) == NULL)
    {
        return false;
    }
    for (i = 0; i < parameter_count; ++i) {
        if (!HSD_DatReadF32(&dat,
                            parameter_offset + i * sizeof(*parameters),
                            &parameters[i]))
        {
            return false;
        }
    }

    output->count = *count;
    output->parameters = parameters;
    return true;
}
