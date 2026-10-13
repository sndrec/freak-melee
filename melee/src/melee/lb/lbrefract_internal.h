#ifndef MELEE_LB_REFRACT_INTERNAL_H
#define MELEE_LB_REFRACT_INTERNAL_H
#include <sysdolphin/baselib/tobj.h>

typedef struct {
    int refractionUserCount;
    void* image_ptr;
    HSD_ImageDesc* imagedesc;
    HSD_TObj** tobj_list;
    Mtx texture_mtx;
} lbRefract_State;

extern lbRefract_State lbRefractState;
#endif
