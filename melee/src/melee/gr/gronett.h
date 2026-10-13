#ifndef GALE01_1E3734
#define GALE01_1E3734

#include <melee/gr/forward.h>
#include <melee/it/forward.h>
#include <melee/lb/forward.h>
#include <melee/sc/forward.h>
#include <sysdolphin/baselib/forward.h>

#include <dolphin/mtx.h>

/* 1E3734 */ void grOnett_801E3734(int);
/* 1E3738 */ void grOnett_801E3738(void);
/* 1E37C4 */ void grOnett_801E37C4(void);
/* 1E37C8 */ void grOnett_801E37C8(void);
/* 1E37EC */ bool grOnett_801E37EC(void);
/* 1E37F4 */ HSD_GObj* grOnett_801E37F4(int);
/* 1E38DC */ void grOnett_801E38DC(Ground_GObj*);
/* 1E3920 */ bool grOnett_801E3920(Ground_GObj*);
/* 1E3928 */ void grOnett_801E3928(Ground_GObj*);
/* 1E392C */ void grOnett_801E392C(Ground_GObj*);
/* 1E3930 */ void grOnett_SetupAwningCollision(Ground_GObj*);
/* 1E3A34 */ void grOnett_InitAwnings(Ground_GObj*);
/* 1E3C58 */ bool grOnett_801E3C58(Ground_GObj*);
/* 1E3C60 */ void grOnett_UpdateAwnings(Ground_GObj*);
/* 1E3CE0 */ void grOnett_801E3CE0(Ground_GObj*);
/* 1E3CE4 */ void grOnett_InitBuilding(Ground_GObj*);
/* 1E3D98 */ bool grOnett_801E3D98(Ground_GObj*);
/* 1E3DA0 */ void grOnett_UpdateBuilding(Ground_GObj*);
/* 1E40E0 */ void grOnett_801E40E0(Ground_GObj*);
/* 1E41B0 */ void grOnett_StartTraffic(Ground_GObj*);
/* 1E41C8 */ void grOnett_InitTraffic(Ground_GObj*);
/* 1E43D8 */ bool grOnett_801E43D8(Ground_GObj*);
/* 1E43E0 */ void grOnett_UpdateTraffic(Ground_GObj*);
/* 1E502C */ void grOnett_801E502C(Ground_GObj*);
/* 1E5030 */ void grOnett_OnTrafficCarHit(Item_GObj*, Ground*);
/* 1E5140 */ void grOnett_PlayPassingCarSfx(s32 idx);
/* 1E5194 */ void grOnett_PlayTrafficSfx(Ground*, s32, s32);
/* 1E5214 */ void grOnett_UpdateAwningPhysics(Ground_GObj*);
/* 1E5538 */ void grOnett_UpdateAmbientEffect(Ground_GObj*);
/* 1E56FC */ DynamicModelDesc* grOnett_GetHazardIndicatorModel(void);
/* 1E5760 */ lbColl_80008D30_arg1* grOnett_801E5760(enum_t);
/* 1E5768 */ bool grOnett_801E5768(Vec3*, int, HSD_JObj*);
/* 3E2858 */ extern StageData grOt_StageData;

#endif
