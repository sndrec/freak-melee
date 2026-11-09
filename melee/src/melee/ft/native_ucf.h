#ifndef MELEE_FT_NATIVE_UCF_H
#define MELEE_FT_NATIVE_UCF_H

#include <melee/ft/forward.h>

void nativeUcf_Sample(Fighter* fp);
bool nativeUcf_StickTravel(const Fighter* fp, bool horizontal, bool vertical,
                          int distance);
bool nativeUcf_AtStickRim(float x, float y);
bool nativeUcf_ShieldDropReady(const Fighter* fp);

#endif
