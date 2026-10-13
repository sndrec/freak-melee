#ifndef HSD_NATIVE_WORKSHOP_GIZMO_H
#define HSD_NATIVE_WORKSHOP_GIZMO_H
#include "forward.h"
#include <dolphin/mtx.h>
#ifdef __cplusplus
extern "C" {
#endif
void nativeWorkshopGizmo_Clear(void);
void nativeWorkshopGizmo_Set(HSD_JObj* owner, const float center[3],
                             const float axes[3][3], float size,
                             unsigned operation, unsigned selected);
void nativeWorkshopGizmo_Draw(HSD_JObj* owner, Mtx view);
#ifdef __cplusplus
}
#endif
#endif
