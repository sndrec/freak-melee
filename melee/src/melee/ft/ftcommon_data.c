#include <sysdolphin/baselib/jobj.h>
#include "types.h"

ftCommonData* p_ftCommonData;

float ftCommon_GetModelScale(Fighter* fp)
{
    return fp->x34_scale.y * fp->co_attrs.model_scaling;
}

void ftCommon_8007F6A4(Fighter* fp, HSD_JObj* jobj)
{
    Vec3 scale;
    float val = 1.0f / fp->ft_data->x0->model_scaling;
    scale.z = val;
    scale.y = val;
    scale.x = val;
    HSD_JObjSetScale(jobj, &scale);
}
