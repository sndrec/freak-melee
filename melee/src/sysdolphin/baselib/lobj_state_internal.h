#ifndef HSD_LOBJ_STATE_INTERNAL_H
#define HSD_LOBJ_STATE_INTERNAL_H
#include "lobj.h"
#include "list.h"
extern HSD_LObjInfo* light_default_class;
extern HSD_SList* light_current_list;
extern HSD_LObj* light_active[MAX_GXLIGHT];
extern s32 light_active_count;
extern s32 light_diffuse_mask,light_specular_mask,light_attenuation_mask,light_alpha_mask;
#endif
