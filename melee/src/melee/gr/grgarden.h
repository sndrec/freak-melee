#ifndef GALE01_202B6C
#define GALE01_202B6C

#include <Runtime/platform.h>

#include <melee/gr/forward.h>
#include <melee/it/forward.h>

/* 202B6C */ void grGarden_80202B6C(int);
/* 202B70 */ void grGarden_80202B70(void);
/* 202C24 */ void grGarden_80202C24(void);
/* 202C4C */ void grGarden_80202C4C(void);
/* 202C70 */ bool grGarden_80202C70(void);
/* 202C78 */ Ground_GObj* grGarden_80202C78(int gobj_id);
/* 202D60 */ void grGarden_80202D60(Ground_GObj*);
/* 202D8C */ bool grGarden_80202D8C(Ground_GObj*);
/* 202D94 */ void grGarden_80202D94(Ground_GObj*);
/* 202D98 */ void grGarden_80202D98(Ground_GObj*);
/* 202D9C */ void grGarden_80202D9C(Ground_GObj*);
/* 202DC8 */ void grGarden_80202DC8(Ground_GObj*);
/* 202E40 */ bool grGarden_80202E40(Ground_GObj*);
/* 202E48 */ void grGarden_80202E48(Ground_GObj*);
/* 202E68 */ void grGarden_80202E68(Ground_GObj*);
/* 202E6C */ void grGarden_80202E6C(Ground_GObj*);
/* 202F34 */ bool grGarden_80202F34(Ground_GObj*);
/* 202F3C */ void grGarden_80202F3C(Ground_GObj*);
/* 202F40 */ void grGarden_80202F40(Ground_GObj*);
/* 202F44 */ void grGarden_80202F44(Ground_GObj*);
/* 202F94 */ bool grGarden_80202F94(Ground_GObj*);
/* 202F9C */ void grGarden_80202F9C(Ground_GObj*);
/* 202FA0 */ void grGarden_80202FA0(Ground_GObj*);
/* 202FA4 */ void grGarden_80202FA4(Ground_GObj*);
/* 202FF4 */ bool grGarden_80202FF4(Ground_GObj*);
/* 202FFC */ void grGarden_80202FFC(Ground_GObj*);
/* 203000 */ void grGarden_80203000(Ground_GObj*);
/* 203004 */ void grGarden_InitCranky(Ground_GObj*);
/* 203090 */ bool grGarden_80203090(Ground_GObj*);
/* 203098 */ void grGarden_UpdateCranky(Ground_GObj*);
/* 2031A0 */ void grGarden_802031A0(Ground_GObj*);
/* 2031A4 */ void grGarden_InitKlaptrap(Ground_GObj*);
/* 203248 */ bool grGarden_80203248(Ground_GObj*);
/* 203250 */ void grGarden_UpdateKlaptrap(Ground_GObj*);
/* 203494 */ void grGarden_80203494(Ground_GObj*);
/* 203498 */ void grGarden_80203498(Item_GObj* item_gobj, Ground* gp);
/* 20349C */ bool grGarden_CheckWaterCollision(Ground_GObj* stage,
                                              HSD_GObj* player,
                                              Vec3* current_velocity);
/* 203560 */ void grGarden_SpawnWaterSplash(float scale, Vec3* position);
/* 2035C8 */ lbColl_80008D30_arg1* grGarden_802035C8(enum_t);
/* 2035D0 */ bool grGarden_802035D0(Vec3*, int arg, HSD_JObj* jobj);
/* 203624 */ float grGarden_80203624(void);
/* 3E52E0 */ extern StageData grGd_StageData;

#endif
