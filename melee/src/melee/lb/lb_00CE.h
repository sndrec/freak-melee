#ifndef GALE01_00CE50
#define GALE01_00CE50

#include <Runtime/platform.h>

// Original game approximations; keep separate from the host C math library.
float lbExpf(float);
float lbPowf(float, float);

/* 00CF74 */ s32 powi(s32, s32);
/* 00D008 */ float lb_8000D008(float, float);
/* 00D148 */ s32 lb_8000D148(float, float, float, float, float, float, float);

#endif
