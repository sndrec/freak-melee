#include <melee/ft/ftanim.h>
#include <melee/it/it_266F.h>
#include <stdlib.h>
#include "kernels.h"
#include <melee/ft/ftcoll.h>
#include <melee/ft/kinds/ftSamus/ftsamus.h>
#include <melee/pl/plbonuslib.h>

extern bool frames;
void record(int,HSD_GObj*,int,float,float);
static ftCommonData common={.x98=.5f,.attackhi3_stick_threshold_y=.5f,.xB0=-.5f,.x37C=.01f,.x348=6};
ftCommonData* p_ftCommonData=&common;
// Selected common leaves record their actual call arguments. Rendering and
// complete animation/motion processing are outside this console closure.
bool ftCo_800951D0(Fighter_GObj* o){record(2,o,0,0,0);return false;}
bool ftCo_80095254(Fighter_GObj* o){record(2,o,1,0,0);return false;}
bool ftCo_800952DC(Fighter_GObj* o){record(2,o,2,0,0);return false;}
bool fn_800D952C(Fighter_GObj* o){record(3,o,0,0,0);return true;}
bool fn_800D8EC8(Fighter_GObj* o){record(4,o,0,0,0);return false;}
bool fn_800D9558(Fighter_GObj* o){record(4,o,1,0,0);return false;}
bool fn_800D9228(Fighter_GObj* o){record(4,o,2,0,0);return false;}
bool fn_800D9930(Fighter_GObj* o){record(4,o,3,0,0);return false;}
void fn_800D949C(Fighter_GObj* o){record(5,o,0,0,0);}
void fn_800D9C64(Fighter_GObj* o){record(5,o,1,0,0);}
void ftCo_Fall_Enter(Fighter_GObj* o){record(6,o,0,0,0);}
void ft_8008A2BC(Fighter_GObj* o){record(7,o,0,0,0);}
bool ftAnim_IsFramesRemaining(Fighter_GObj* o){record(8,o,frames,0,0);return frames;}
void ftAnim_8006EBA4(Fighter_GObj* o){record(9,o,0,0,0);}
void ftAnim_SetAnimRate(Fighter_GObj* o,float rate){record(10,o,0,0,rate);}
void ftCommon_8007E2D0(Fighter* f,s16 mask,HSD_GObjEvent grab,HSD_GObjEvent pickup,void(*captured)(HSD_GObj*,HSD_GObj*))
{if(!grab||!pickup||!captured)abort();record(11,f->gobj,mask,0,0);f->grab_cb=grab;}
void ftCommon_8007E2F4(Fighter* f,s16 mask){record(12,f->gobj,mask,0,0);}
void ftCommon_8007EBAC(Fighter* f,u32 mode,u32 arg){if(arg)abort();record(13,f->gobj,mode,0,0);}
void ftCommon_8007E358(Fighter_GObj* o){record(14,o,0,0,0);}
void ftCommon_8007D5D4(Fighter* f){record(15,f->gobj,0,0,0);f->ground_or_air=GA_Air;}
void ftCommon_8007D7FC(Fighter* f){record(15,f->gobj,1,0,0);f->ground_or_air=GA_Ground;}
void ftCommon_UnlockECB(Fighter* f){record(16,f->gobj,0,0,0);}
void fn_800DAA40(Fighter_GObj* a,Fighter_GObj* b){if(!b)abort();record(17,a,0,0,0);}
void efAsync_Spawn(HSD_GObj* o,void* queue,u32 mode,u32 effect,HSD_JObj* joint,...)
{(void)queue;if(mode!=1||!joint)abort();record(18,o,effect,0,0);}
void ftCo_800DB368(Fighter* a,Fighter* b){if(!a||!b)abort();record(20,b->gobj,0,0,0);}
void ftColl_8007B7A4(Fighter_GObj* o,int n){record(21,o,n,0,0);}
void pl_80040614(int player,bool sub,float timer){(void)player;(void)sub;record(22,NULL,0,timer,0);}
void ft_80083C00(Fighter_GObj* o,HSD_GObjEvent cb){(void)cb;record(23,o,0,0,0);}
void HSD_JObjSetMtxDirtySub(HSD_JObj* o){(void)o;record(24,NULL,0,0,0);}
bool ftCo_800DF7F4(Fighter* f){(void)f;return false;}
bool ftCo_800DF844(Fighter* f){(void)f;return false;}
bool ftCo_800DF878(Fighter* f){(void)f;return false;}
// Callback pointers may be installed, but their later geometry/item processing
// is outside the selected scheduling proof and must never execute here.
void fn_800D8BFC(Fighter_GObj* o){(void)o;abort();}
void fn_800DAADC(Fighter_GObj* a,Fighter_GObj* b){(void)a;(void)b;abort();}
void fn_800DA054(Fighter_GObj* o){(void)o;abort();}
void fn_800DA190(Fighter_GObj* o){(void)o;abort();}
void fn_800DA4A0(Fighter_GObj* o){(void)o;abort();}
void fn_800DA490(Fighter_GObj* o){(void)o;abort();}
void fn_800DA668(Fighter_GObj* o){(void)o;abort();}
void fn_800DA678(Fighter_GObj* o){(void)o;abort();}
void ftCo_800DE508(Fighter_GObj* o){(void)o;abort();}
void it_802A7840(HSD_GObj* o){(void)o;abort();}
void it_802BAA94(Item_GObj* o){(void)o;abort();}
void it_802A7AAC(HSD_GObj* o){(void)o;abort();}
void it_802BAC3C(Fighter_GObj* o){(void)o;abort();}
void ftCo_800C3BE8(Fighter_GObj* o){(void)o;abort();}
void nativeCapture_EncloseCaught(Fighter_GObj* o,float r){(void)o;(void)r;abort();}
float ftYs_Init_8012BAC0(Fighter* f){(void)f;abort();}
void ftSs_Init_CreateThrowGrappleBeam(HSD_GObj* o,s32 id,float rate){(void)o;(void)id;(void)rate;abort();}
void ftColl_8007B62C(Fighter_GObj* o,int n){(void)o;(void)n;abort();}
FighterKind nativeFighterRuntime_AnimationFamily(Fighter* f){(void)f;abort();}

// Unselected setup presentation/held-item leaves must never run here.
bool itIsHeavy(Item_GObj* object){(void)object;abort();}
int itGetHoldKind(Item_GObj* object){(void)object;abort();}
void ftAnim_80070FB4(Fighter_GObj* object,s32 selector,s32 value)
{(void)object;(void)selector;(void)value;abort();}
void ftAnim_80070C48(Fighter_GObj* object,s32 selector)
{(void)object;(void)selector;abort();}
void ftAnim_80070CC4(Fighter_GObj* object,int selector)
{(void)object;(void)selector;abort();}
void ftAnim_800704F0(Fighter_GObj* object,int selector,float value)
{(void)object;(void)selector;(void)value;abort();}

// Outer throw tick/finish leaves are controlled; no donor special may run for
// an interpreted source or the native callback-policy comparison.
void ftCo_800DD724(Fighter_GObj* o){record(26,o,0,0,0);}
void ftCommon_8007D92C(Fighter_GObj* o){record(27,o,0,0,0);}
void ftCo_8009B56C(Fighter_GObj* o){(void)o;abort();}
void ftMt_SpecialN_Shoot(Fighter_GObj* o){(void)o;abort();}
void ftFx_Throw_Anim(Fighter_GObj* o){(void)o;abort();}

extern bool heavy_classification;
bool it_8026B2D8(Item_GObj* o){if(!o)abort();return heavy_classification;}
Item_GObj* ftpickupitem_800942A0(Fighter_GObj* o,u32 flags){(void)o;(void)flags;abort();}
void ftpickupitem_800948A8(Fighter_GObj* o,Item_GObj* item){(void)o;(void)item;abort();}
bool ftpickupitem_8009447C(HSD_GObj* o,Item_GObj* item){(void)o;(void)item;abort();}
void ftCo_80096D9C(Fighter_GObj* o){record(29,o,0,0,0);}
void ftDk_MS_341_800DF980(Fighter_GObj* o){(void)o;abort();}
void ftDk_MS_349_800E06D8(Fighter_GObj* o){(void)o;abort();}
