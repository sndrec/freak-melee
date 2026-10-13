#ifndef MELEE_CM_CAMERA_SUBJECTS_INTERNAL_H
#define MELEE_CM_CAMERA_SUBJECTS_INTERNAL_H

#include "types.h"

extern CmSubject* cm_804D6458;
extern CmSubject* cm_804D645C;
extern CmSubject* cm_804D6460;
extern CmSubject* cm_804D6468;

// Actual subject initialization/allocation list owner. Capacity requested by
// Camera_Init is retained; physical backing is stable across scene retirement.
void CameraSubjects_Init(int count);

#endif
