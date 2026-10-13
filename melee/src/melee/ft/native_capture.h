#ifndef MELEE_FT_NATIVE_CAPTURE_H
#define MELEE_FT_NATIVE_CAPTURE_H

#include <melee/ft/forward.h>

void nativeCapture_EncloseCaught(Fighter_GObj* holder, float radius);

bool nativeCapture_ReleaseIntoEnclosure(Fighter_GObj* holder);

#endif
