#include "lobj_state_internal.h"
// Actual source lighting controls, not a snapshot mirror.
HSD_LObjInfo* light_default_class;
HSD_SList* light_current_list;
HSD_LObj* light_active[MAX_GXLIGHT];
s32 light_active_count;
s32 light_diffuse_mask,light_specular_mask,light_attenuation_mask,light_alpha_mask;
