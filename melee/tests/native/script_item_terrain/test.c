#include <melee/ft/native_script_binding_item_terrain.h>
#include <melee/ft/native_script_binding_item_collision.h>
#include <melee/ft/native_script_binding_item_lifecycle.h>
#include <melee/ft/native_script_binding_actions.h>
#include <melee/ft/native_script_effect_dispatch.h>
#include <melee/it/native_item_lifetime.h>
#include <melee/it/inlines.h>
#include <melee/it/item.h>
#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/itmaplib.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_3F14.h>
#include <melee/it/ithitbox.h>
#include <melee/lb/lbcollision.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <melee/ef/efsync.h>
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/generator.h>
#include <sysdolphin/baselib/psstructs.h>
#include <sysdolphin/baselib/psappsrt.h>
#include <math.h>
#include <dolphin.h>
#include <stdarg.h>
#include "controlled_setup.inc"

HSD_JObjInfo hsdJObj;
static HSD_ClassInfo joint_class;
static HSD_JObj joints[512];static FighterBone bones[4];
static ItemCommonData common;ItemCommonData* it_804D6D28=&common;
static bool floor_hit,air_floor_hit,motion,remove_on_write,replace_on_write;
static unsigned wall_bits,impacts,contacts,damage_resets,vm_calls,terrain_cases,reflections;
static HSD_GObj* selected;
static Item replacement_data;
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,
 uint32_t count,const NativeScriptValue* input,NativeScriptDiagnostic* d)
{
 (void)v;(void)d;
 if(count||s!=NativeScriptBinding_VelocityX||input->type!=NativeScriptType_F32)return false;
 Item* ip=((HSD_GObj*)c->source)->user_data;ip->x40_vel.x=input->as.f32;
 if(remove_on_write){remove_on_write=false;HSD_GObj* o=c->source;o->obj_kind=HSD_GOBJ_OBJ_NONE;o->hsd_obj=NULL;HSD_GObjFree(o);}
 if(replace_on_write){replace_on_write=false;ip->xDC8_word.flags.x1F=0;replacement_data=*ip;((HSD_GObj*)c->source)->user_data=&replacement_data;}
 return true;
}
static bool itemCallbackCurrent(HSD_GObj* object,uint32_t generation)
{return nativeCallbackSource_Alive(object,generation,NativeCallbackSource_Item);}
void HSD_JObjSetMtxDirtySub(HSD_JObj* node){CHECK(node);node->flags|=JOBJ_MTX_DIRTY;}
void(HSD_JObjSetMtxDirty)(HSD_JObj* node){HSD_JObjSetMtxDirtySub(node);}
void it_80276278(Item_GObj* o){(void)o;abort();}
bool mpColl_8004B108(CollData* c)
{c->cur_pos=(Vec3){10,20,30};c->env_flags=(floor_hit?Collide_FloorMask:0)|(wall_bits&1?Collide_LeftWallMask:0)|(wall_bits&2?Collide_RightWallMask:0)|(wall_bits&4?Collide_CeilingMask:0);c->floor.index=9;c->floor.normal=(Vec3){0,1,0};c->left_facing_wall.index=10;c->right_facing_wall.index=11;c->ceiling.index=12;return floor_hit;}
bool mpColl_800471F8(CollData* c)
{mpColl_8004B108(c);c->env_flags=(air_floor_hit?Collide_FloorMask:0)|(wall_bits&1?Collide_LeftWallMask:0)|(wall_bits&2?Collide_RightWallMask:0)|(wall_bits&4?Collide_CeilingMask:0);c->left_facing_wall.index=10;c->right_facing_wall.index=11;c->ceiling.index=12;return air_floor_hit;}
bool it_80277040(Item_GObj* o){(void)o;return motion;}
void it_80276FC4(Item_GObj* o,s32 flags){CHECK(o==selected&&flags);++impacts;}
bool it_80276D9C(Item_GObj* o,enum_t flags){CHECK(o==selected&&flags);++reflections;return true;}
void it_802734B4(Item_GObj* o){CHECK(o==selected);((Item*)o->user_data)->xDC8_word.flags.x1F=1;}
void lbColl_80008428(HitCapsule* h){CHECK(h);++contacts;}
void it_8027129C(Item_GObj* o,s32 i){CHECK(o==selected&&i>=0&&i<4);++damage_resets;}
s32 HSD_Randi(s32 n){CHECK(n>0);return 0;}
void Item_8026A8EC(Item_GObj* o){CHECK(o==selected);o->obj_kind=HSD_GOBJ_OBJ_NONE;o->hsd_obj=NULL;HSD_GObjFree(o);}
// Unselected families in the real split backends must never fake success.
MapCollData* mpLib_8004D164(void){abort();}
CollLine* mpGetGroundCollLine(void){abort();}
CollVtx* mpGetGroundCollVtx(void){abort();}
CollJoint* mpGetGroundCollJoint(void){abort();}
bool mpLib_80054ED8(int i){(void)i;abort();}
enum_t mpLineGetKind(int i){(void)i;abort();}
int mpJointFromLine(int i){(void)i;abort();}
void mpJointGetCb1(int i,mpLib_JointCollisionCallback* cb,void** data){(void)i;(void)cb;(void)data;abort();}
void mpJointGetCb2(int i,mpLib_JointCollisionCallback* cb,void** data){(void)i;(void)cb;(void)data;abort();}
void mpColl_80043558(CollData* c,int i){(void)c;(void)i;abort();}
bool mpGetSpeed(int i,Vec3* p,Vec3* v){(void)i;(void)p;(void)v;abort();}
bool it_8026EA20(HSD_GObj* o,Vec3* a,Vec3* b,Vec3* n,int* i,u32* f)
{(void)o;(void)a;(void)b;(void)n;(void)i;(void)f;abort();}
void it_802750F8(Item_GObj* o){(void)o;abort();}
void it_8027518C(Item_GObj* o){(void)o;abort();}
#include "native_ground.inc"

static HSD_Generator generator;static HSD_psAppSRT appsrt;
static EF_Effect effects_native[12];static HSD_GObj effect_objects[12];static HSD_JObj effect_joints[12];
static bool generator_fail,appsrt_fail;static unsigned model_count,effect_calls;
HSD_Generator* hsd_8039F05C(s32 link,s32 bank_id,s32 id)
{CHECK(link==0&&bank_id==id/1000);if(generator_fail)return NULL;memset(&generator,0,sizeof(generator));memset(&appsrt,0,sizeof(appsrt));generator.appsrt=appsrt_fail?NULL:&appsrt;return&generator;}
HSD_psAppSRT* psAddGeneratorAppSRT_begin(HSD_Generator* g,s32 kind)
{CHECK(g==&generator&&kind==1);return appsrt_fail?NULL:&appsrt;}
void hsd_8039D4DC(HSD_Generator* g){CHECK(g==&generator);}
EF_Effect* efLib_Create_Attach_Pos(u32 id,HSD_GObj* parent,Vec3* pos)
{CHECK(id==0x1E||id==0x1F);if(model_count==12)return NULL;unsigned n=model_count++;effects_native[n]=(EF_Effect){.gobj=&effect_objects[n],.parent_gobj=parent};effect_objects[n]=(HSD_GObj){.hsd_obj=&effect_joints[n]};effect_joints[n]=(HSD_JObj){.translate=*pos};return&effects_native[n];}
f32 HSD_Randf(void){return 0.25f;}
void efLib_Cb_SetOffset_FromParams(EF_Effect* e){(void)e;abort();}
#include "native_facing.inc"
EF_SpawnResult efSync_Spawn(s32 gfx_id,HSD_GObj* gobj,...)
{
 ++effect_calls;EF_SpawnResult spawned={0};Vec3* va_vec3;Vec3 translate;float f32_1;
 float rand_f32,rand_rot_y,rand_rot_x,rand_param_x,rand_param_y;HSD_JObj* jobj_1;
 va_list vlist;va_start(vlist,gobj);switch(gfx_id){
#include "native_effect_cases.inc"
 default:abort();}va_end(vlist);return spawned;
}
void efAsync_Spawn(HSD_GObj* o,void* p,u32 m,u32 id,HSD_JObj* j,...)
{(void)o;(void)p;(void)m;(void)id;(void)j;abort();}
void lb_8000B1CC(HSD_JObj* j,Vec3* o,Vec3* p){(void)j;(void)o;(void)p;abort();}
void lb_800119DC(Vec3* p,int n,float x,float y,float z){(void)p;(void)n;(void)x;(void)y;(void)z;abort();}
void ftColl_8007AEF8(Fighter_GObj* o){(void)o;abort();}
void ftColl_CreateReflectHit(Fighter_GObj* o,ReflectDesc* r,HSD_GObjEvent cb){(void)o;(void)r;(void)cb;abort();}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp,Fighter_Part p){(void)fp;(void)p;abort();}

static void set(HSD_GObj* o,const char* name,int64_t value,bool boolean)
{NativeScriptValue v=boolean?(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=value!=0}:(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=value};field(o,name,&v);}
static HSD_GObj* actor(void)
{
 HSD_GObj* o=item();selected=o;Item* ip=o->user_data;
 o->obj_kind=HSD_GObj_JObjKind;o->hsd_obj=&joints[item_count];HSD_JObj* j=o->hsd_obj;
 *j=(HSD_JObj){.scale={1,1,1}};HSD_CLASS_METHOD(j)=&joint_class;
 ip->xCC_item_attr=&attributes;ip->x378_itemColl.x0_gobj=o;ip->xC0C=(itECB){0};
 ip->xC0C.top=1;ip->xC0C.bottom=-1;ip->xC0C.right=1;ip->xC0C.left=-1;
 ip->scl=ip->facing_dir=1;ip->x40_vel=(Vec3){0.001f,-0.002f,0};ip->xC30=-1;
 ip->xDC8_word.flags.x16=1;ip->xDAA_flag.b2=1;
 floor_hit=air_floor_hit=false;motion=false;wall_bits=0;contacts=damage_resets=impacts=reflections=0;
 remove_on_write=replace_on_write=false;return o;
}
static void close_actor(HSD_GObj* o)
{o->obj_kind=HSD_GOBJ_OBJ_NONE;o->hsd_obj=NULL;HSD_GObjFree(o);}
static void oracle_event(HSD_GObj* o)
{NativeScriptValue result;CHECK(nativeScriptItem_RunHook(o,floor_hit?NativeScriptItemHook_StageLanded:NativeScriptItemHook_StageLeft,NULL,0,&result)==NativeScriptVM_Ok);}
static CmSubject camera_subject;
static unsigned camera_releases;
void Camera_800290D4(CmSubject* subject)
{CHECK(subject==&camera_subject);++camera_releases;}
static void camera_release_pairs(void)
{
 for(unsigned mode=0;mode<4;++mode)for(unsigned attached=0;attached<2;++attached){
  HSD_GObj* o=actor();Item* ip=o->user_data;ip->xDCD_flag.b01=mode;ip->x520_cameraBox=attached?&camera_subject:NULL;
  Item initial=*ip;camera_releases=0;Item_80267454(o);Item expected=*ip;unsigned expected_releases=camera_releases;
  *ip=initial;camera_releases=0;NativeScriptRuntimeContext c=context(o);NativeScriptValue out;
  CHECK(nativeScriptBindingItemLifecycle_Invoke(&c,NativeScriptBinding_ItemReleaseCamera,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected))&&camera_releases==expected_releases);
  CHECK(camera_releases==((mode&&attached)?1u:0u));++comparisons;close_actor(o);
 }
}
static void native_pairs(void)
{
 for(unsigned kind=0;kind<5;++kind)for(unsigned floor=0;floor<2;++floor)for(unsigned wall=0;wall<8;++wall){
  HSD_GObj* o=actor();Item* ip=o->user_data;floor_hit=air_floor_hit=floor;wall_bits=wall;
  Item original=*ip;
  if(kind==0)it_8026D62C(o,oracle_event);else if(kind==3)it_8026E15C(o,oracle_event);else if(kind==4)it_8026E414(o,oracle_event);else it_8026E248(o,oracle_event);
  Item expected=*ip;unsigned reset_count=contacts,impact_count=impacts;
  *ip=original;contacts=damage_resets=impacts=0;set(o,"left",0,false);set(o,"landed",0,false);
  NativeScriptRuntimeContext c=context(o);NativeScriptValue out={0},arg={.type=NativeScriptType_Bool,.as.u64=kind==3};
  uint32_t semantic=kind==4?NativeScriptBinding_ItemLandOnStage:kind==0?NativeScriptBinding_ItemFollowGround:kind==1?NativeScriptBinding_ItemBounceToGround:NativeScriptBinding_ItemBounceToGroundWithContactReset;
  CHECK(nativeScriptBindingItemTerrain_Invoke(&c,semantic,&arg,kind==2||kind==3?1:0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected)));
  CHECK(contacts==reset_count&&impacts==impact_count);
  if(kind<4)CHECK(contacts==((kind==3&&floor)?4u:0u));
  else CHECK(field(o,"landed",NULL).as.i64==(floor?1:0)&&field(o,"left",NULL).as.i64==0);++comparisons;++terrain_cases;close_actor(o);
 }
}
static void authored(void)
{
 NativeScriptValue out;
 HSD_GObj* o=actor();CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok);
 CHECK(field(o,"left",NULL).as.i64==1&&field(o,"completed",NULL).as.i64==1);++vm_calls;close_actor(o);
 for(unsigned reset=0;reset<2;++reset){o=actor();floor_hit=air_floor_hit=true;set(o,"reset",reset,true);
  CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Created,NULL,0,&out)==NativeScriptVM_Ok);
  CHECK(field(o,"landed",NULL).as.i64==1&&field(o,"completed",NULL).as.i64==1&&contacts==(reset?4u:0u));++vm_calls;close_actor(o);}
 o=actor();floor_hit=air_floor_hit=true;CHECK(nativeScriptItem_BindCallback(o,NativeScriptItemCallback_Accessory,nativeScriptItem_CallbackFunction(o,NativeScriptItemCallback_Accessory)));
 CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Accessory,NULL,0,&out)==NativeScriptVM_Ok);CHECK(contacts==0&&field(o,"landed",NULL).as.i64==1);++vm_calls;close_actor(o);
 o=actor();air_floor_hit=true;set(o,"nested",1,true);CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok);
 CHECK(field(o,"left",NULL).as.i64==1&&field(o,"landed",NULL).as.i64==1&&!field(o,"nested",NULL).as.u64&&field(o,"completed",NULL).as.i64==1);++vm_calls;close_actor(o);
 o=actor();floor_hit=motion=true;((Item*)o->user_data)->xD5C=2;
 CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Ok);
 CHECK(field(o,"air",NULL).as.i64==1&&((Item*)o->user_data)->xDC8_word.flags.x1F);++vm_calls;close_actor(o);
 o=actor();set(o,"mode",2,false);CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_Error);CHECK(nativeScriptRuntime_Idle());++vm_calls;close_actor(o);
 o=actor();set(o,"mode",1,false);remove_on_write=true;CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Spawned,NULL,0,&out)==NativeScriptVM_SourceRetired);CHECK(nativeScriptRuntime_Idle());++vm_calls;
 o=actor();floor_hit=motion=true;((Item*)o->user_data)->xD5C=2;replace_on_write=true;Item* original=o->user_data;
 NativeScriptRuntimeContext c=context(o);out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
 CHECK(!nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemFollowGround,NULL,0,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!original->xDC8_word.flags.x1F&&!replacement_data.xDC8_word.flags.x1F);
 o->user_data=original;close_actor(o);
 o=actor();c=context(o);NativeScriptValue wrong={.type=NativeScriptType_I32,.as.i64=1};
 CHECK(!nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemBounceToGroundWithContactReset,&wrong,1,&out,&diagnostic));close_actor(o);
}
static void oracle_contact(HSD_GObj* o)
{NativeScriptValue result;CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_StageContact,NULL,0,&result)==NativeScriptVM_Ok);}
static void contact_responses(void)
{
 for(unsigned form=0;form<3;++form)for(unsigned floor=0;floor<2;++floor)for(unsigned wall=0;wall<8;++wall){
  HSD_GObj* o=actor();Item* ip=o->user_data;floor_hit=air_floor_hit=floor;wall_bits=wall;
  Item original=*ip;
  if(form==2)it_8026E5A0(o,oracle_contact);else it_8026E71C(o,oracle_contact);
  Item expected=*ip;unsigned reset_count=contacts,impact_count=impacts,reflection_count=reflections;
  int64_t called=field(o,"contact",NULL).as.i64;
  *ip=original;contacts=impacts=reflections=0;set(o,"contact",0,false);
  NativeScriptRuntimeContext c=context(o);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99};
  NativeScriptValue arg={.type=NativeScriptType_Bool,.as.u64=form==2};
  CHECK(nativeScriptBindingItemTerrain_Invoke(&c,form?NativeScriptBinding_ItemRespondToStageWithImpactResponse:NativeScriptBinding_ItemRespondToStage,&arg,form?1:0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected)));
  CHECK(contacts==reset_count&&impacts==impact_count&&reflections==reflection_count);
  CHECK(field(o,"contact",NULL).as.i64==called&&called==(floor||wall?1:0));
  ++comparisons;++terrain_cases;close_actor(o);
 }
 for(unsigned impact=0;impact<2;++impact){
  HSD_GObj* o=actor();air_floor_hit=true;set(o,"reset",impact,true);NativeScriptValue out;
  CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Thrown,NULL,0,&out)==NativeScriptVM_Ok);
  CHECK(field(o,"contact",NULL).as.i64==1&&field(o,"completed",NULL).as.i64==1);++vm_calls;close_actor(o);
  for(unsigned failure=0;failure<3;++failure){
   o=actor();air_floor_hit=true;set(o,"reset",impact,true);set(o,"mode",failure?1:2,false);
   remove_on_write=failure==1;replace_on_write=failure==2;Item* original=o->user_data;
   NativeScriptRuntimeContext c=context(o);NativeScriptValue arg={.type=NativeScriptType_Bool,.as.u64=impact};
   out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};
   CHECK(!nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemRespondToStageWithImpactResponse,&arg,1,&out,&diagnostic));
   CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&nativeScriptRuntime_Idle());
   if(failure!=1){o->user_data=original;close_actor(o);}++terrain_cases;
  }
 }
 HSD_GObj* o=actor();NativeScriptRuntimeContext c=context(o);NativeScriptValue out={.type=NativeScriptType_U64,.as.u64=99},arg={.type=NativeScriptType_I32,.as.i64=1};
 CHECK(!nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemRespondToStageWithImpactResponse,&arg,1,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99);close_actor(o);
}
static void common_operations(void)
{
 NativeScriptValue out;
 const int32_t facing_values[]={INT32_MIN,-2,-1,0,1,2,INT32_MAX};
 for(unsigned n=0;n<256;++n){
  HSD_GObj* o=actor();Item* ip=o->user_data;
  ip->xC0C=(itECB){.top=(float)n,.bottom=-(float)n,.right=(float)n/3,.left=-(float)n/7};
  ip->scl=(float)((int)n-128)/9;ip->facing_dir=n&1?-1:1;
  ip->x378_itemColl.x34_flags.b0=n&1;ip->x378_itemColl.facing_dir=123;
  Item original=*ip;it_80275D5C(o,&ip->xC0C);Item expected=*ip;*ip=original;
  NativeScriptRuntimeContext c=context(o);
  CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemResetDefaultEcb,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected)));++comparisons;
  int32_t direction=facing_values[n%7];original=*ip;mpCollSetFacingDir(&ip->x378_itemColl,direction);expected=*ip;*ip=original;
  NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=direction};
  CHECK(nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,&value,1,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected)));++comparisons;
  ip->x524_cmd.u=(void*)(uintptr_t)(n+1);ip->x524_cmd.loop_count=n;ip->x524_cmd.timer=(float)n;
  original=*ip;ip->x524_cmd.u=NULL;expected=*ip;*ip=original;
  CHECK(nativeScriptBindingItemLifecycle_Invoke(&c,NativeScriptBinding_ItemClearAnimationCommands,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected)));++comparisons;close_actor(o);
 }
 for(unsigned n=0;n<64;++n){
  HSD_GObj* o=actor();Item* ip=o->user_data;floor_hit=n&1;wall_bits=n>>1;ip->xDC8_word.flags.x1F=(n>>4)&1;ip->xD5C=(n>>5)&1;
  Item original=*ip;it_8026E8C4(o,oracle_event,oracle_event);Item expected=*ip;*ip=original;
  set(o,"left",0,false);set(o,"landed",0,false);NativeScriptRuntimeContext c=context(o);
  CHECK(nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemSlideOnGround,NULL,0,&out,&diagnostic));
  CHECK(out.type==NativeScriptType_Void&&!memcmp(ip,&expected,sizeof(expected)));++comparisons;++terrain_cases;close_actor(o);
 }
 for(unsigned floor=0;floor<2;++floor){
  HSD_GObj* o=actor();floor_hit=floor;
  o->obj_kind=HSD_GOBJ_OBJ_NONE;o->hsd_obj=NULL;
  CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Dropped,NULL,0,&out)==NativeScriptVM_Ok);
  CHECK(field(o,floor?"landed":"left",NULL).as.i64==1&&field(o,"completed",NULL).as.i64==1);++vm_calls;close_actor(o);
 }
 HSD_GObj* o=actor();Item* ip=o->user_data;ip->x524_cmd.u=(void*)1;set(o,"mode",INT32_MIN,false);
 CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_PickedUp,NULL,0,&out)==NativeScriptVM_Ok);
 CHECK(ip->x378_itemColl.facing_dir==1&&!ip->x524_cmd.u&&field(o,"completed",NULL).as.i64==1);++vm_calls;close_actor(o);
 o=actor();NativeScriptRuntimeContext c=context(o);NativeScriptValue value={.type=NativeScriptType_I32,.as.i64=INT64_MAX};
 out=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=99};Item before=*(Item*)o->user_data;
 CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,&value,1,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(o->user_data,&before,sizeof(before)));
 value=(NativeScriptValue){.type=NativeScriptType_Bool,.as.u64=1};
 CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,&value,1,&out,&diagnostic));
 CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,NULL,0,&value,1,&out,&diagnostic));
 value=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=1};
 CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,NULL,0,&value,1,&out,&diagnostic));
 value=(NativeScriptValue){.type=NativeScriptType_U64,.as.u64=UINT32_MAX};
 CHECK(!nativeScriptBindings_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,NULL,0,&value,1,&out,&diagnostic));
 CHECK(out.type==NativeScriptType_U64&&out.as.u64==99&&!memcmp(o->user_data,&before,sizeof(before)));
 ((Item*)o->user_data)->x378_itemColl.x0_gobj=NULL;
 CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemResetDefaultEcb,NULL,0,&out,&diagnostic));
 CHECK(!nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemSlideOnGround,NULL,0,&out,&diagnostic));close_actor(o);
 o=actor();c=context(o);((Item*)o->user_data)->x378_itemColl.x34_flags.b1234=1;
 value=(NativeScriptValue){.type=NativeScriptType_I32,.as.i64=1};
 CHECK(!nativeScriptBindingItemCollision_Invoke(&c,NativeScriptBinding_ItemSetCollisionFacing,&value,1,&out,&diagnostic));
 CHECK(!nativeScriptBindingItemTerrain_Invoke(&c,NativeScriptBinding_ItemSlideOnGround,NULL,0,&out,&diagnostic));close_actor(o);
 o=actor();set(o,"mode",1,false);remove_on_write=true;
 CHECK(nativeScriptItem_RunHook(o,NativeScriptItemHook_Dropped,NULL,0,&out)==NativeScriptVM_SourceRetired);CHECK(nativeScriptRuntime_Idle());++vm_calls;
}
static void effect_vm(void)
{
 HSD_GObj* f=fighter(true,777);Fighter* fp=f->user_data;fp->parts=bones;f->obj_kind=HSD_GObj_JObjKind;f->hsd_obj=&joints[400];
 target_package.assets.parts.part_to_joint=target_package.assets.part_to_joint;
 target_package.assets.parts.joint_to_part=target_package.assets.joint_to_part;
 for(unsigned i=0;i<4;++i)bones[i].joint=&joints[400+i];
 uint32_t helper=UINT32_MAX;for(unsigned i=0;i<bank.declarations.function_count;++i)if(!functions[i].owner&&nativeScriptSource_SpanEquals(&bank.tree,functions[i].name,"Effect"))helper=i;CHECK(helper!=UINT32_MAX);
 const float scalars[]={-2,-0.0f,0,2};
 for(unsigned id=0;id<2;++id)for(unsigned n=0;n<4;++n){
  NativeScriptValue args[5]={{.type=NativeScriptType_I32,.as.i64=id?0x4CF:0x3F1},{.type=NativeScriptType_F32,.as.f32=12.5f},{.type=NativeScriptType_F32,.as.f32=-4},{.type=NativeScriptType_F32,.as.f32=7},{.type=NativeScriptType_F32,.as.f32=scalars[n]}},out;
  model_count=0;generator_fail=appsrt_fail=false;
  CHECK(nativeScriptFighter_Helper(f,777,helper,args,5,&out,&diagnostic)==NativeScriptVM_Ok);
  if(!id){CHECK(appsrt.translate.x==12.5f&&appsrt.translate.y==-4&&appsrt.translate.z==7);CHECK(appsrt.rot.y==(-scalars[n]<0?-M_PI_2_F:M_PI_2_F));}
  else{CHECK(model_count==12);for(unsigned i=0;i<12;++i)CHECK(effect_joints[i].scale.x==scalars[n]&&effect_joints[i].scale.y==scalars[n]&&effect_joints[i].scale.z==scalars[n]);}
  ++vm_calls;
 }
 NativeScriptRuntimeContext c=context(f);NativeScriptValue args[5]={{.type=NativeScriptType_I32,.as.i64=0x3F1},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32},{.type=NativeScriptType_F32,.as.f32=1}},out;
 generator_fail=true;CHECK(nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsSpawnWithScalarAtPosition,args,5,&out,&diagnostic));
 generator_fail=false;appsrt_fail=true;CHECK(nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsSpawnWithScalarAtPosition,args,5,&out,&diagnostic));appsrt_fail=false;
 unsigned before=effect_calls;args[0].as.i64=0x3F7;CHECK(!nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsSpawnWithScalarAtPosition,args,5,&out,&diagnostic));
 args[0].as.i64=0x4D9;f->hsd_obj=NULL;CHECK(!nativeScriptBindingFeedback_Invoke(&c,NativeScriptBinding_EffectsSpawnWithScalarAtPosition,args,5,&out,&diagnostic));CHECK(effect_calls==before);
 f->obj_kind=HSD_GOBJ_OBJ_NONE;HSD_GObjFree(f);
}
static void frontend(void)
{
 const char* calls[]={"item.bounce_to_ground(1)","item.bounce_to_ground(false, true)","item.follow_ground(1)","item.set_collision_facing(true)","item.set_collision_facing(1.0)","item.slide_on_ground(1)","item.reset_default_ecb(1)","item.clear_animation_commands(1)","item.respond_to_stage(1)","item.respond_to_stage(1.0)","item.respond_to_stage(true, false)"};
 for(unsigned i=0;i<sizeof(calls)/sizeof(*calls);++i){
  char text[512];snprintf(text,sizeof(text),"fighter:\n    behavior independent\narticle Probe states 1:\n    state 0 anim -1:\n        on collision:\n            %s\n            return false\n",calls[i]);
  NativeScriptItemAsset asset={.declaration="Probe",.kind=It_Kind_Mario_Fire,.article=&article};NativeScriptItemMetadataContext metadata={.assets=&asset,.asset_count=1};
  NativeScriptMoveLayout layout={.common_count=341,.first=341,.count=1};NativeScriptCompileRequest request={.source=text,.source_bytes=(uint32_t)strlen(text),.motion_layout=&layout,.metadata=nativeScriptItemMetadata_Validate,.metadata_context=&metadata};
  const NativeScriptCompileBank* accepted=NULL;CHECK(!nativeScriptCompile_Build(&bank,&request,&accepted,&diagnostic)&&!accepted);
 }
}
int main(void)
{
 prepare();*HSD_CLASS_INFO(&hsdJObj)=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj)}};
 joint_class=(HSD_ClassInfo){.head={.flags=1,.obj_size=sizeof(HSD_JObj),.parent=HSD_CLASS_INFO(&hsdJObj)}};
 attributes.x58=0;attributes.x5c=0.1f;common.xCC=1;common.x74_float=30;
 HSD_GObjLibInitDataType setup={.p_link_max=32,.gx_link_max=8,.gproc_pri_max=31};HSD_GObj_80391304(&setup);
 native_pairs();camera_release_pairs();authored();contact_responses();common_operations();effect_vm();CHECK(nativeScriptRuntime_Idle());frontend();
 printf("{\"status\":\"PASS terrain and position scalar\",\"checks\":%u,\"native_pairs\":%u,\"terrain_cases\":%u,\"vm_calls\":%u,\"native_effect_calls\":%u}\n",checks,comparisons,terrain_cases,vm_calls,effect_calls);return 0;
}
