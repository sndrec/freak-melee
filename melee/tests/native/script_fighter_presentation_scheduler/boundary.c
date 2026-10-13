#include "boundary.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftwaitanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <sysdolphin/baselib/random.h>
#include <melee/ft/ftdrawcommon.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftdynamics.h>
#include <melee/ft/kinds/ftCommon/ftCo_DemoCallback0.h>
#include <melee/ft/ftCo_800C7070.h>
#include <melee/ft/ftCo_800C70D0.h>
#include <melee/ft/ftCo_800C7178.h>
#include <melee/ft/ftCo_800C7220.h>
#include <melee/ft/ftCo_800C739C.h>
#include <melee/ft/ftCo_800C7434.h>
#include <melee/ft/ft_0C88.h>
#include <melee/ft/kinds/ftCommon/ftCo_09F4.h>
#include <melee/ft/kinds/ftCommon/ftCo_09F7.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/ef/efasync.h>
#include <melee/ef/efsync.h>
#include <melee/ft/ft_0BEC.h>
#include <melee/ft/ft_0BF0.h>
#include <melee/ft/ft_0BEF.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_script_runtime.h>
#include <melee/ft/native_script_actor_access.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_AppealS.h>
#include <melee/ft/kinds/ftFox/types.h>
#include <melee/ft/kinds/ftMario/ftmariospecialn.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/ft/kinds/ftDrMario/ftdrmario.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftZelda/ftzelda.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itfoxblaster.h>
#include <melee/it/kinds/itdrmariopill.h>
#include <melee/it/kinds/itnessbat.h>
#include <melee/lb/lb_013B.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lb_00F9.h>
#include <melee/lb/lbshadow.h>
#include <melee/cm/camera.h>
#include <melee/pl/types.h>
#include <melee/pl/plbonuslib.h>
#include <melee/db/db.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/fog.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#pragma clang section data = ""
unsigned presentation_bookkeeping,presentation_common_motion,presentation_stock_calls,presentation_reset_calls,presentation_overlay_calls,presentation_loop_calls,presentation_demo_tail;
unsigned presentation_retire_mode,presentation_retire_at,presentation_callback_writes;
int presentation_overlay_branch;bool presentation_source_package;
static uint64_t old_layout,old_parameters,old_epoch;static Fighter* mutated;
static HSD_JObj roots[64];static unsigned count;static ftData data;static ftCo_DatAttrs common;static struct itPickup pickup;static Vec2 extra;static CmSubject camera;
static MotionState motions[ftCo_MS_Count];static struct Fighter_WaitAnimData anim;static Fighter_804D653C_t colors[128];
Fighter_804D653C_t* Fighter_804D653C=colors;Fighter_804D653C_t* Fighter_804D6538;DbLKind DbLevel;
MotionState ftData_803C52A0[14];MotionState* ftData_UnkMotionStates0[Ft_Kind_RuntimeCapacity];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];MotionState* ftData_CharacterStateTables[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnLoad[Ft_Kind_RuntimeCapacity],ftData_UnkMotionStates4[Ft_Kind_RuntimeCapacity];
u8 ftData_UnkBytePerCharacter[Ft_Kind_RuntimeCapacity];
void (*ftData_UnkDemoCallbacks0[Ft_Kind_RuntimeCapacity])(int,enum_t*,enum_t*);
static void stock(HSD_GObj* object){(void)object;++presentation_stock_calls;}
static const NativeSourcePackage package;
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;return presentation_source_package?&package:NULL;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return NULL;}
FighterKind nativeFighterRuntime_BehaviorKind(FighterKind kind){return kind;}
void presentation_reset(void){presentation_restore();presentation_bookkeeping=presentation_common_motion=presentation_stock_calls=presentation_reset_calls=presentation_overlay_calls=presentation_loop_calls=presentation_demo_tail=presentation_callback_writes=0;presentation_retire_mode=presentation_retire_at=0;presentation_overlay_branch=0;}
void presentation_restore(void){if(!mutated)return;presentation_bank.storage_layout.identity=old_layout;presentation_bank.storage_parameters.identity=old_parameters;nativeGObjPool.lifetime=old_epoch;mutated->kind=Ft_Kind_Mario;mutated->ft_data=&data;mutated=NULL;}
NativeScriptValue presentation_state(HSD_GObj* object,const char* name,const NativeScriptValue* input){NativeScriptReceiverView view;CHECK(nativeScriptFighter_View(object,((Fighter*)object->user_data)->x8_spawnNum,&view));NativeScriptDiagnostic diagnostic;
 for(unsigned i=0;i<view.program->value_count;++i)if(!view.program->values[i].owner&&nativeScriptSource_SpanEquals(view.program->tree,view.program->values[i].name,name)){NativeScriptValue out;if(input)CHECK(nativeScriptStorage_Write(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,input,&diagnostic));CHECK(nativeScriptStorage_Read(view.storage,NATIVE_SCRIPT_STATE_SEMANTIC|i,NULL,0,&out,&diagnostic));return out;}CHECK(false);return(NativeScriptValue){0};}
void presentation_destroy(void* p){Fighter* fp=p;nativeScriptFighter_Detach(fp->gobj);NativeFighterPoolSlot* slot=nativeFighterPool_Slot(fp);if(slot){NativeMatchPoolReference ref;CHECK(nativeMatchPool_Reference(&nativeFighterPool,slot,&ref));CHECK(nativeMatchPool_Free(&nativeFighterPool,ref));}}
void Fighter_Unload_8006DABC(void* p){presentation_destroy(p);}
static void initialize(HSD_GObj* object,FighterKind kind){Fighter* fp=object->user_data;fp->gobj=object;fp->kind=kind;fp->behavior_kind=kind;fp->x8_spawnNum=++count;fp->ft_data=&data;fp->x18=ftCo_MS_Count;fp->x1C_actionStateList=motions;fp->x1C_actionStateList[ftCo_MS_AppealSL].anim_id=1;fp->x34_scale=(Vec3){1,1,1};fp->facing_dir=1;fp->x890_cameraBox=&camera;object->hsd_obj=&roots[(count-1)%64];data.x0=&common;data.x40=&pickup;data.x50=&extra;gFtDataList[kind]=&data;ftData_UnkBytePerCharacter[kind]=0xff;colors[0x6A].unk5=1;ftData_UnkMotionStates4[kind]=stock;ftData_OnLoad[kind]=stock;}
HSD_GObj* presentation_create(FighterKind kind,bool attach)
{
 HSD_GObj* object=GObj_Create(HSD_GOBJ_CLASS_FIGHTER,HSD_GOBJ_PLINK_FIGHTER,0);CHECK(object);
 NativeMatchPoolReference reference;NativeFighterPoolSlot* slot=nativeMatchPool_Allocate(&nativeFighterPool,&reference);CHECK(slot);
 GObj_InitUserData(object,4,presentation_destroy,&slot->actor);initialize(object,kind);
 if(attach)CHECK(nativeScriptFighter_Attach(object));
 return object;
}
#include "actual_context.h"
bool nativeScriptBindings_Read(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,NativeScriptValue* out,NativeScriptDiagnostic* d){(void)v;(void)d;if(n||s!=NativeScriptBinding_PositionX||!nativeScriptBindings_SourceAlive(c))return false;*out=(NativeScriptValue){.type=NativeScriptType_F32,.as.f32=((Fighter*)((HSD_GObj*)c->source)->user_data)->cur_pos.x};return true;}
bool nativeScriptBindings_Write(NativeScriptRuntimeContext* c,uint32_t s,const NativeScriptValue* v,uint32_t n,const NativeScriptValue* in,NativeScriptDiagnostic* d){(void)v;(void)d;if(n||s!=NativeScriptBinding_PositionX||in->type!=NativeScriptType_F32||!nativeScriptBindings_SourceAlive(c))return false;Fighter* fp=((HSD_GObj*)c->source)->user_data;fp->cur_pos.x=in->as.f32;++presentation_callback_writes;
 if(presentation_retire_mode==7&&presentation_callback_writes==presentation_retire_at){if(d)snprintf(d->message,sizeof(d->message),"Controlled native position failure");return false;}
 if(presentation_retire_mode==8&&presentation_callback_writes==presentation_retire_at){HSD_GObjFree(c->source);return true;}
 if(presentation_retire_mode&&presentation_callback_writes==presentation_retire_at){mutated=fp;old_layout=presentation_bank.storage_layout.identity;old_parameters=presentation_bank.storage_parameters.identity;old_epoch=nativeGObjPool.lifetime;
  switch(presentation_retire_mode){case 1:++fp->x8_spawnNum;break;case 2:++presentation_bank.storage_layout.identity;break;case 3:++presentation_bank.storage_parameters.identity;break;case 4:++nativeGObjPool.lifetime;break;case 5:fp->kind=Ft_Kind_Luigi;break;case 6:fp->ft_data=NULL;break;default:abort();}}
 return true;}
void lb_80014498(ColorOverlay* overlay){(void)overlay;++presentation_reset_calls;}
bool lb_800144C8(ColorOverlay* overlay,struct Fighter_804D653C_t* descriptor,int id,int mode){(void)overlay;(void)descriptor;(void)id;(void)mode;++presentation_overlay_calls;return true;}
bool ftCo_800C53E4(Fighter* fp){(void)fp;return 1;}
bool ftCo_800BFFD0(Fighter* fp,enum_t id,bool mode){(void)fp;(void)id;(void)mode;++presentation_overlay_calls;return true;}
static unsigned overlay_frames;
bool lb_80014258(HSD_GObj* object,void* overlay,void(*callback)(HSD_GObj*,CommandInfo*,int)){(void)callback;++presentation_loop_calls;Fighter* fp=object->user_data;if(overlay!=&fp->x488)return false;bool answer=overlay_frames++%2==0;return answer;}
static void ft_800BFF34(HSD_GObj* g,CommandInfo* c,int n){(void)g;(void)c;(void)n;abort();}
static void ft_800BFF70(HSD_GObj* g,CommandInfo* c,int n){(void)g;(void)c;(void)n;abort();}
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,u32 flags,float frame,float rate,float blend,HSD_GObj* other){(void)flags;(void)frame;(void)rate;(void)blend;(void)other;++presentation_common_motion;((Fighter*)object->user_data)->motion_id=motion;nativeScriptFighter_SelectMotion(object,motion);}
void pl_80040120(int player,int companion){(void)player;(void)companion;++presentation_bookkeeping;}
struct Fighter_WaitAnimData* ftData_80085FD4(Fighter* fp,int clip){(void)fp;(void)clip;return &anim;}
void ftCl_Init_80149318(HSD_GObj* g){stock(g);}void ftDr_Init_80149910(HSD_GObj* g){stock(g);}void ftPe_Init_8011B93C(HSD_GObj* g){stock(g);}void ftZd_Init_801395C8(HSD_GObj* g){stock(g);}void ftKb_SpecialN_800F5D04(HSD_GObj* g,bool value){(void)value;stock(g);}
void lb_8000B1CC(HSD_JObj* j,Vec3* local,Vec3* out){(void)j;(void)local;(void)out;abort();}void lb_800119DC(Vec3* p,int n,float a,float b,float c){(void)p;(void)n;(void)a;(void)b;(void)c;abort();}
// Asset/model initialization is controlled. It establishes real native pool
// identity; attachment, source Load/NewLife/Death and event execution are real.
void Fighter_UnkInitLoad_80068914(Fighter_GObj* g,struct plAllocInfo* p){initialize(g,p->internal_id);}
void ftData_8008572C(FighterKind k){(void)k;}void ftData_80085820(FighterKind k,int c){(void)k;(void)c;}
void ftData_80085B98(Fighter* fp,enum_t a,enum_t b){(void)fp;(void)a;(void)b;}
void efAsync_LoadSync(int bank){(void)bank;}
#define INIT(name) void name(Fighter_GObj* g){(void)g;}
INIT(Fighter_UnkUpdateCostumeJoint_800686E4) INIT(ftParts_SetupParts) INIT(ftAnim_80070308)
INIT(ftCo_800C884C) INIT(Fighter_80068E64) INIT(ftParts_800749CC) INIT(ftAnim_8007077C) INIT(ftAnim_8006FE48)
#define FDATA(name) void name(Fighter* fp){(void)fp;}
FDATA(ftParts_80074E58) FDATA(ftCo_8009CF84) FDATA(Fighter_UnkUpdateVecFromBones_8006876C) FDATA(ftCo_8009F578)
INIT(ftColl_8007B320)
CmSubject* Camera_80029020(void){return &camera;}
void lbShadow_8000ED54(LbShadow* value,HSD_JObj* joint){(void)joint;(void)value;}
void ftDrawCommon_80080E18(HSD_GObj* g,int pass){(void)g;(void)pass;abort();}
INIT(Fighter_8006A360) INIT(Fighter_procUpdate) INIT(Fighter_procMap) INIT(Fighter_8006C80C) INIT(Fighter_8006D9AC)
bool Fighter_UnkProcessDeath_80068354(Fighter_GObj* g){Fighter* fp=g->user_data;CHECK(nativeScriptFighter_IsAttached(g));++fp->x8_spawnNum;nativeScriptFighter_NewLife(g,fp->x8_spawnNum);return nativeScriptFighter_Run(g,NativeScriptFighter_Death,NULL,0, NULL,NULL)==NativeScriptVM_Ok;}
void ftLib_800867E8(Fighter_GObj* g){CHECK(nativeScriptFighter_Status(g)==NativeScriptVM_Ok);++presentation_demo_tail;}
void ftCo_800BEFD4(Fighter_GObj* g){(void)g;abort();}
void ftCo_800C6150(Fighter_GObj* g){(void)g;abort();}void ftCo_800C7070(Fighter_GObj* g){(void)g;abort();}void ftCo_800C70D0(Fighter_GObj* g){(void)g;abort();}
void ftCo_800C7178(Fighter_GObj* g){(void)g;abort();}void ftCo_800C7220(Fighter_GObj* g){(void)g;abort();}void ftCo_800C7294(Fighter_GObj* g){(void)g;abort();}
void ftCo_800C7308(Fighter_GObj* g){(void)g;abort();}void ftCo_800C739C(Fighter_GObj* g){(void)g;abort();}void ftCo_800C7434(Fighter_GObj* g){(void)g;abort();}
void HSD_JObjRemoveAll(HSD_JObj* root){(void)root;}
void HSD_LObjRemoveAll(HSD_LObj* o){(void)o;abort();}void HSD_LObj_803668EC(HSD_LObj* o){(void)o;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}void HSD_LObjSetupInit(HSD_CObj* o){(void)o;abort();}
void HSD_JObjDispAll(HSD_JObj* o,Mtx m,u32 f,u32 mode){(void)o;(void)m;(void)f;(void)mode;abort();}
void HSD_FogSet(HSD_Fog* o){(void)o;abort();}bool HSD_CObjSetCurrent(HSD_CObj* o){(void)o;abort();}void HSD_CObjEndCurrent(void){abort();}
const EF_ModelRecipe* efSync_ModelRecipe(s32 id){(void)id;return NULL;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp,enum_t part){(void)fp;(void)part;abort();}
Item_GObj* it_802AE994(Fighter_GObj* g,Fighter_Part p,ItemKind k){(void)g;(void)p;(void)k;abort();}
Item_GObj* it_802AD590(Fighter_GObj* g,Fighter_Part p){(void)g;(void)p;abort();}
void it_8026BAE8(Item_GObj* g,float scale){(void)g;(void)scale;abort();}
int ftMr_SpecialN_VitaminRandom(Fighter_GObj* g){(void)g;abort();}
Item_GObj* itDrMarioPill_802C09C4(Fighter_GObj* g,Vec3* v,s32 color,ItemKind k,s32 mode,Fighter_Part p,f32 f){(void)g;(void)v;(void)color;(void)k;(void)mode;(void)p;(void)f;abort();}
void OSReport(char* format,...){va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);}
void __assert(char* file,u32 line,char* text){(void)file;(void)line;(void)text;abort();}
void HSD_Panic(char* file,u32 line,char* text){(void)file;(void)line;(void)text;abort();}
// Decision/continuation fixture: animation data, RNG, physics and pose leaves
// are controlled. Actual schedulers and ordinary interpreted hooks run below.
bool policy_frames,policy_jump_sound,policy_multi;
unsigned policy_jump_tail;
unsigned policy_restarts,policy_wait_changes,policy_rng_calls,policy_jumps;
static struct ftCommonData policy_common;
struct ftCommonData* p_ftCommonData=&policy_common;
bool ftAnim_IsFramesRemaining(Fighter_GObj* g){(void)g;return policy_frames;}
void ftCo_8008A6D8(Fighter_GObj* g,s32 animation){CHECK(animation==((Fighter*)g->user_data)->anim_id);++policy_restarts;}
s32 HSD_Randi(s32 maximum){CHECK(maximum==100);++policy_rng_calls;return 0;}
void ftData_80085CD8(Fighter* a,Fighter* b,enum_t animation){CHECK(a==b&&animation==1);++policy_wait_changes;}
void ftCo_8009E7B4(Fighter* f,u8 (*blend)[2]){(void)f;(void)blend;}
void ftAnim_8006EBE8(Fighter_GObj* g,f32 frame,f32 rate,f32 blend){(void)g;(void)frame;(void)rate;(void)blend;abort();}
void ftAnim_8006EBA4(Fighter_GObj* g){(void)g;}
void ftCommon_8007D5D4(Fighter* f){(void)f;}
void ftCo_JumpAerial_Begin(Fighter_GObj* g,FtMotionId motion,Vec3* speed,bool reset,bool use,bool sound){
 (void)g;CHECK(motion==(policy_multi?ftCo_MS_JumpAerialF:ftCo_MS_ItemScrewAir)&&reset==!policy_multi&&!use);
 CHECK(speed->x==0&&speed->y==(policy_multi?2:0));policy_jump_sound=sound;++policy_jumps;
}
s32 ftCo_800D7268(Fighter* f){(void)f;return 0;}
void ft_800CB6EC(Fighter* f,s32 value){(void)f;CHECK(value==5);++policy_jump_tail;}
#include "actual_presentation.h"
#include "reference_common_policies.h"
