#pragma once
#include <sysdolphin/baselib/forward.h>
typedef struct MnCamera {
    HSD_CObjDesc* rest;
    bool input_disabled;
} MnCamera;
// Camera GObj userdata points to MnCamera, owned by its scene.
void mnCamera_Tick(HSD_GObj* object);
