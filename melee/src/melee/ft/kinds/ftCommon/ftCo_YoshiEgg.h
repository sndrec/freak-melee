#ifndef MELEE_FT_COMMON_CAPTURE_ENCLOSURE_H
#define MELEE_FT_COMMON_CAPTURE_ENCLOSURE_H

#include <melee/ft/forward.h>
#include <melee/ft/native_capture_enclosure.h>
#include <sysdolphin/baselib/forward.h>

void ftCo_800BBC88(Fighter_GObj* gobj);
bool ftCo_CaptureEnclosure_Enter(Fighter_GObj* victim, Fighter_GObj* holder,
                                FtMotionId motion,
                                const NativeCaptureEnclosureSettings* settings,
                                HSD_Joint* model, float facing);
void ftCo_CaptureEnclosure_Anim(Fighter_GObj* gobj);
void ftCo_CaptureEnclosure_IASA(Fighter_GObj* gobj);
void ftCo_CaptureEnclosure_Phys(Fighter_GObj* gobj);
void ftCo_CaptureEnclosure_Coll(Fighter_GObj* gobj);
void ftCo_CaptureEnclosure_Damage(Fighter_GObj* gobj);

#endif
