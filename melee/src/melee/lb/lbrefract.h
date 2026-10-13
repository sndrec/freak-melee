#ifndef GALE01_021CE8
#define GALE01_021CE8

#include <Runtime/platform.h>
#include "forward.h"

#include <sysdolphin/baselib/forward.h>

extern HSD_DObjInfo lbRefractDObjInfo;
extern HSD_PObjInfo lbRefractPObjInfo;

int lbRefract_8002219C(lbRefract_CallbackData*, void*, s32, u32, u32);

/* 0222A4 */ void lbRefract_800222A4(void);
/* 02247C */ void lbRefract_8002247C(HSD_CObj*);
/* 022560 */ void lbRefract_80022560(void);
/* 0225D4 */ void lbRefract_800225D4(void);
/* 0226A8 */ s32 lbRefract_PObjLoad(HSD_PObj* pobj, HSD_PObjDesc* desc);
/* 022998 */ void lbRefract_80022998(HSD_MObj*, u32 rendermode, s32);
/* 022BB8 */ void lbRefract_80022BB8(void);
/* 022BD0 */ void lbRefSetUnuse(void);

#endif
