#include <melee/it/itdraw.h>
#include <melee/it/article_model_layers.h>
#include <melee/cm/camera.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
unsigned render_observations,render_wrong_order,render_display_calls;
const HSD_Joint* render_descriptor;
bool render_replace_program,render_swapped;
void render_bad_draw(HSD_GObj* o,int pass){(void)o;(void)pass;abort();}
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor)
{
 HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
 ++render_observations;render_descriptor=descriptor;
 if(!object || object->render_cb!=it_8026EECC || object->gx_link!=6 || !nativeScriptItem_IsAttached(object))++render_wrong_order;
 for(unsigned i=0;i<32;++i)if(object->user_data==&spawn_items[i]){memset(&spawn_joints[i],0,sizeof(HSD_JObj));spawn_joints[i].scale=descriptor->scale;return &spawn_joints[i];}
 abort();
}
HSD_JObj* HSD_JObjAlloc(void){abort();}
void HSD_JObjDispAll(HSD_JObj* object,Mtx matrix,u32 flags,u32 mode)
{(void)matrix;(void)flags;(void)mode;if(!object)abort();++render_display_calls;}
int Camera_80031060(void){return 0;}
bool ftLib_80087074(HSD_GObj* o,Vec3* p){(void)o;(void)p;abort();}
bool ftLib_800868D4(HSD_GObj* o,HSD_GObj* item){(void)o;(void)item;abort();}
u32 it_8026ECE0(Item_GObj* o,u32 pass){(void)o;(void)pass;return 0;}
void it_8026EBC8(HSD_GObj* o,u16 n,const u8* p){(void)o;(void)n;(void)p;abort();}
void it_8026EC54(HSD_GObj* o,u16 n,const u8* p){(void)o;(void)n;(void)p;abort();}
void HSD_StateInvalidate(int flags){(void)flags;abort();}
void HSD_StateInitTev(void){abort();}
void HSD_ClearVtxDesc(void){abort();}
Article* nativeFighterRuntime_ArticleAssets(FighterKind k,ItemKind i){(void)k;(void)i;return NULL;}
#include "render_bodies.inc"
