#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_pool.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/it/native_item_owner.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdemo.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ft_0C88.h>
#include <melee/pl/types.h>
#include <melee/lb/lbanim.h>
#include <melee/gm/native_match_snapshot_pool.h>
#include <melee/gm/native_rollback_state.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/lobj.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/fog.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma clang section data = ""
static uint8_t saved[2*1024*1024],invalid[2*1024*1024],before_payload[2*1024*1024];
static uint8_t oracle[64*1024*1024];
static NativeMatchPoolSlot before_slots[NATIVE_FIGHTER_POOL_CAPACITY];
static uint32_t before_free[NATIVE_FIGHTER_POOL_CAPACITY];
static unsigned checks,cases,unloads,new_life,normal_loads,demo_loads;
static bool retire_attach,retire_life,expect_panic;
static jmp_buf panic_jump;
static FigaTree tree={.frames=20};
static HSD_JObj root,alternate,endpoint;
static ftData data;
static struct Fighter_WaitAnimData animations[32];
static FighterPartsTable part_table={.parts_num=60};
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=tables;
HSD_ObjAllocData fighter_x2040_alloc_data;
u32 Fighter_804D64F8;
Event ftData_Table_Unk1[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnLoad[Ft_Kind_RuntimeCapacity],ftData_OnUserDataRemove[Ft_Kind_RuntimeCapacity];
u8 ftData_UnkBytePerCharacter[Ft_Kind_RuntimeCapacity];
MotionState ftData_803C52A0[14];MotionState* ftData_UnkMotionStates0[Ft_Kind_RuntimeCapacity];
static HSD_GObjEvent on_create_fighter[16];
void (*ftData_UnkDemoCallbacks0[Ft_Kind_RuntimeCapacity])(int,int*,int*);
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);assert(x);}}while(0)
void OSReport(char* format,...){(void)format;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason)
{if(expect_panic)longjmp(panic_jump,1);fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
bool nativeScriptRuntime_Idle(void){return true;}
bool nativeScriptFighter_RetireScene(void){return true;}
// This constructor/pool fixture owns no interpreted actor scene.
bool nativeScriptFighter_CanBeginScene(uint64_t e){return e!=0;}
bool nativeScriptFighter_BeginScene(uint64_t e){return e!=0;}
bool nativeScriptItem_CanBeginScene(uint64_t e){return e!=0;}
bool nativeScriptItem_BeginScene(uint64_t e){return e!=0;}
bool nativeScriptItem_RetireScene(void){return true;}
bool nativeScriptEffect_CanBeginScene(uint64_t e){return e!=0;}
bool nativeScriptEffect_BeginScene(uint64_t e){return e!=0;}
bool nativeScriptEffect_RetireScene(void){return true;}
bool nativeCallbackDispatch_Enter(void){return true;}
void nativeCallbackDispatch_Leave(void){}
const struct NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind){(void)kind;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Run(Fighter_GObj* object,uint32_t hook,const NativeScriptValue* args,uint32_t count,NativeScriptEventRecordIO* record,NativeScriptValue* result)
{(void)object;(void)hook;(void)args;(void)count;(void)record;(void)result;abort();}
bool nativeScriptFighter_HasHook(Fighter_GObj* object,uint32_t hook){(void)object;(void)hook;return false;}
bool nativeScriptFighter_IsAttached(Fighter_GObj* object){(void)object;return false;}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind kind){(void)kind;return NULL;}
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* object){(void)object;return NativeScriptVM_SourceRetired;}
bool nativeScriptFighter_View(Fighter_GObj* object,uint32_t generation,NativeScriptReceiverView* view){(void)object;(void)generation;(void)view;return false;}
void nativeScriptFighter_Detach(Fighter_GObj* object){(void)object;++unloads;}
bool nativeScriptFighter_Attach(Fighter_GObj* object){if(retire_attach){retire_attach=false;HSD_GObjFree(object);}return true;}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return NULL;}
bool nativeFighterRuntime_EnterLoad(Fighter_GObj* object){(void)object;return true;}
void nativeFighterRuntime_LeaveLoad(Fighter_GObj* object){(void)object;}
bool nativeCallbackSource_Alive(HSD_GObj* object,uint32_t generation,uint32_t source_class)
{
    CHECK(source_class==NativeCallbackSource_Fighter);
    NativeMatchPoolReference reference;if(!nativeMatchPool_Reference(&nativeGObjPool,object,&reference)||!object->user_data)return false;
    Fighter* fighter=object->user_data;return nativeFighterPool_Slot(fighter)!=NULL&&(uint32_t)fighter->x8_spawnNum==generation;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* object,uint32_t source_class,uint32_t* generation)
{CHECK(source_class==NativeCallbackSource_Fighter);*generation=(uint32_t)((Fighter*)object->user_data)->x8_spawnNum;return nativeCallbackSource_Alive(object,*generation,source_class);}
void HSD_ObjAllocInit(HSD_ObjAllocData* pool,size_t size,u32 align){CHECK(pool==&fighter_x2040_alloc_data&&size==32*sizeof(void*)&&align==4);}
void* HSD_ObjAlloc(HSD_ObjAllocData* pool){(void)pool;CHECK(false);return NULL;}
void HSD_ObjFree(HSD_ObjAllocData* pool,void* pointer){(void)pool;(void)pointer;CHECK(false);}
void ft_800852B0(void){}
void Fighter_LoadCommonData(void){}
void ft_8008549C(void){}
void ftCo_8009F4A4(void){}
void ftCo_800C8064(void){}
void ftCo_800C8F6C(void){}
void ftData_8008572C(FighterKind kind){CHECK(kind==Ft_Kind_Samus);}
void ftData_80085820(FighterKind kind,int color){(void)color;CHECK(kind==Ft_Kind_Samus);}
void Fighter_UnkInitLoad_80068914(Fighter_GObj* object,struct plAllocInfo* input)
{
    Fighter* fighter=object->user_data;NativeFighterPoolSlot* own=nativeFighterPool_Slot(fighter);CHECK(own);
    CHECK(fighter->dat_attrs_backup==own->attributes&&fighter->parts==NULL&&fighter->x203C.data==NULL);
    fighter->gobj=object;fighter->kind=input->internal_id;fighter->player_id=input->slot;
    fighter->x8_spawnNum=++Fighter_804D64F8;fighter->ft_data=&data;fighter->x24=animations;
}
void Fighter_UnkUpdateCostumeJoint_800686E4(Fighter_GObj* object){object->hsd_obj=&root;}
void ftData_80085B10(Fighter* fighter){CHECK(fighter->parts==NULL);fighter->x59C=fighter->x5A0=NULL;++normal_loads;}
void ftData_80085B98(Fighter* fighter,int first,int last){CHECK(fighter->parts==NULL&&first==0&&last==9);fighter->x59C=fighter->x5A0=NULL;++demo_loads;}
FigaTree* ftData_80085E50(Fighter* fighter,int motion){(void)fighter;(void)motion;return &tree;}
float lbAnim_8001E8F8(FigaTree* selected){CHECK(selected==&tree);return selected->frames;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter,Fighter_Part part){(void)fighter;return part;}
void ftParts_SetupParts(Fighter_GObj* object){Fighter* f=object->user_data;CHECK(f->parts==nativeFighterPool_Slot(f)->parts);f->parts[0].joint=&root;f->parts[139].joint=&endpoint;}
void ftAnim_80070308(Fighter_GObj* object){CHECK(((Fighter*)object->user_data)->x203C.data==NULL);}
void ftPartsPObjSetDefaultClass(void){}
void ftPartsPObjClearDefaultClass(void){}
void ft_800C85B8(Fighter_GObj* object){Fighter* f=object->user_data;CHECK(f->x203C.data==nativeFighterPool_Slot(f)->secondary_dobjs);}
void Fighter_80068E64(Fighter_GObj* object){(void)object;}
void ftParts_800749CC(Fighter_GObj* object){(void)object;}
void ftAnim_8007077C(Fighter_GObj* object){(void)object;}
void ftCo_8009CF84(Fighter* fighter){(void)fighter;}
void ftAnim_8006FE48(Fighter_GObj* object){((Fighter*)object->user_data)->x8AC_animSkeleton=&alternate;}
void Fighter_UnkUpdateVecFromBones_8006876C(Fighter* fighter){(void)fighter;}
void ftCo_8009F578(Fighter* fighter){(void)fighter;}
void efAsync_LoadSync(int bank){(void)bank;CHECK(false);}
void ftColl_8007B320(Fighter_GObj* object){(void)object;}
CmSubject* Camera_80029020(void){return NULL;}
void lbShadow_8000ED54(LbShadow* shadow,HSD_JObj* joint){(void)shadow;CHECK(joint==&root);}
bool Fighter_UnkProcessDeath_80068354(Fighter_GObj* object){++new_life;if(retire_life){retire_life=false;HSD_GObjFree(object);return false;}return true;}
int Player_GetFlagsBit3(int player){(void)player;return 0;}
#define EVENT(name) void name(Fighter_GObj* object){(void)object;}
EVENT(Fighter_8006A1BC) EVENT(Fighter_8006A360) EVENT(Fighter_8006ABA0)
EVENT(Fighter_Spaghetti_8006AD10) EVENT(Fighter_procUpdate) EVENT(Fighter_procMap)
EVENT(Fighter_8006C5F4) EVENT(Fighter_CallAcessoryCallbacks_8006C624) EVENT(Fighter_8006C80C)
EVENT(Fighter_UnkProcessGrab_8006CA5C) EVENT(Fighter_8006CB94) EVENT(Fighter_ProcessHit_8006D1EC)
EVENT(Fighter_8006D9AC) EVENT(Fighter_UnkCallCameraCallback_8006D9EC) EVENT(Fighter_8006DA4C)
EVENT(ftMh_MS_341_8014FE10) EVENT(ftCh_Init_80155FCC) EVENT(ftCo_800BFD04) EVENT(ftCo_800C61B0)
EVENT(ftCommon_8007D92C) EVENT(ftLib_800867E8)
void ftDrawCommon_80080E18(Fighter_GObj* object,int pass){(void)object;(void)pass;}
void nativeArticleResource_CancelCreator(Fighter_GObj* object,uint32_t generation){(void)object;(void)generation;}
void nativeArticleResource_CancelConstructingAttachment(Fighter_GObj* object,NativeMatchPoolReference gobj,NativeMatchPoolReference owner)
{CHECK(nativeMatchPool_Resolve(gobj)==object&&nativeMatchPool_Resolve(owner)==object->user_data);}
EVENT(ftColl_8007B8E8) EVENT(it_8026B7F8) EVENT(nativeFighterAttachment_Clear)
EVENT(ftColl_800765AC) EVENT(ft_80088C5C)
EVENT(efLib_DestroyAll)
void nativeItem_RemoveConstructingOwnerChildren(Fighter_GObj* object,NativeMatchPoolReference gobj,NativeMatchPoolReference owner)
{CHECK(nativeMatchPool_Resolve(gobj)==object&&nativeMatchPool_Resolve(owner)==object->user_data);}
void efAsync_QueueClear(void* pointer){(void)pointer;}
void Camera_800290D4(CmSubject* subject){(void)subject;}
void ftCo_UnloadDynamicBones(Fighter* fighter){(void)fighter;}
void lbShadow_8000EE8C(LbShadow* shadow){(void)shadow;}
void HSD_JObjRemoveAll(HSD_JObj* joint){CHECK(joint==&alternate||joint==NULL);}
void HSD_JObjUnref(HSD_JObj* joint){(void)joint;}
void ftData_800859A8(Fighter* fighter){(void)fighter;}
void HSD_LObjRemoveAll(HSD_LObj* light){(void)light;}
void Player_80031FB0(int player,s32 member,HSD_GObj* expected){(void)player;(void)member;CHECK(expected);}
void HSD_LObj_803668EC(HSD_LObj* light){(void)light;abort();}
HSD_CObj* HSD_CObjGetCurrent(void){abort();}
void HSD_LObjSetupInit(HSD_CObj* object){(void)object;abort();}
void HSD_JObjDispAll(HSD_JObj* object,Mtx matrix,u32 flags,u32 mode){(void)object;(void)matrix;(void)flags;(void)mode;abort();}
void HSD_FogSet(HSD_Fog* object){(void)object;abort();}
bool HSD_CObjSetCurrent(HSD_CObj* object){(void)object;abort();}
void HSD_CObjEndCurrent(void){abort();}

#include "parts.inc"
#include "family.inc"
#include "create.inc"
#include "demo.inc"
#include "unload.inc"

static void scene(void)
{
    CHECK(nativeGObjPool_RetireScene());HSD_GObjLibInitDataType config;HSD_GObj_803912E0(&config);config.gproc_pri_max=24;
    HSD_GObj_80391304(&config);CHECK(nativeFighterPool.lifetime==nativeGObjPool.lifetime);
    Fighter_FirstInitialize_80067A84();
}
static Fighter_GObj* create(bool demo)
{
    struct plAllocInfo normal={.internal_id=Ft_Kind_Samus,.slot=0};plAllocInfo2 display={.internal_id=Ft_Kind_Samus,.slot=0,.unk8=0};
    return demo?ftDemo_CreateFighter(&display):Fighter_Create(&normal);
}
static void snapshot_atomic_failure(size_t bytes,unsigned fault)
{
    memcpy(invalid,saved,bytes);NativeMatchPoolSnapshotHeader header;memcpy(&header,saved,sizeof(header));
    size_t payload=sizeof(header)+(header.capacity+header.free_count)*sizeof(uint32_t)+sizeof(uint32_t);
    if(fault==0){void* wrong=NULL;memcpy(invalid+payload+offsetof(Fighter,dat_attrs_backup),&wrong,sizeof(wrong));}
    if(fault==1){void* wrong=(void*)0x1234;memcpy(invalid+payload+offsetof(Fighter,parts),&wrong,sizeof(wrong));}
    if(fault==2){void* wrong=(void*)0x1234;memcpy(invalid+payload+offsetof(Fighter,x59C),&wrong,sizeof(wrong));}
    if(fault==3){u32 wrong=125;memcpy(invalid+payload+offsetof(Fighter,dobj_list.count),&wrong,sizeof(((Fighter*)0)->dobj_list.count));}
    size_t live_bytes=nativeFighterPool.capacity*nativeFighterPool.stride;
    memcpy(before_payload,nativeFighterPool.payload,live_bytes);memcpy(before_slots,nativeFighterPool.slots,sizeof(before_slots));
    memcpy(before_free,nativeFighterPool.free_order,sizeof(before_free));NativeMatchPool before=nativeFighterPool;
    CHECK(!nativeFighterPool_ValidateSnapshot(invalid,bytes));CHECK(!nativeFighterPool_Load(invalid,bytes));
    CHECK(memcmp(before_payload,nativeFighterPool.payload,live_bytes)==0&&memcmp(&before,&nativeFighterPool,sizeof(before))==0);
    CHECK(memcmp(before_slots,nativeFighterPool.slots,sizeof(before_slots))==0&&memcmp(before_free,nativeFighterPool.free_order,sizeof(before_free))==0);++cases;
}
int main(void)
{
    memset(ftData_UnkBytePerCharacter,0xFF,sizeof(ftData_UnkBytePerCharacter));tables[Ft_Kind_Samus]=&part_table;data.x14=animations;
    scene();Fighter_GObj* a=create(false),*b=create(true);CHECK(a&&b);
    Fighter* fp=a->user_data;NativeFighterPoolSlot* own=nativeFighterPool_Slot(fp);CHECK(own);
    CHECK(fp==&own->actor&&fp->parts==own->parts&&fp->dat_attrs_backup==own->attributes&&fp->x203C.data==own->secondary_dobjs);
    fp->dat_attrs=fp->dat_attrs_backup;fp->x2D0=fp->dat_attrs;
    own->attributes[1059]=0xAC;own->parts[139].flagsC=0x12345678;own->primary_dobjs[123]=(HSD_DObj*)&endpoint;
    own->secondary_dobjs[31]=(HSD_DObj*)&alternate;fp->native_module_state[255]=0xBA;
    size_t bytes;CHECK(nativeFighterPool_Save(saved,sizeof(saved),&bytes));CHECK(bytes==nativeMatchSnapshotPool_Bytes(&nativeFighterPool));
    for(unsigned fault=0;fault<4;++fault)snapshot_atomic_failure(bytes,fault);
    NativeMatchPoolReference old;CHECK(nativeMatchPool_Reference(&nativeFighterPool,fp,&old));
    NativeRollbackStateRegion region;CHECK(nativeRollbackState_GetGlobalRegions(&region,1)==1&&region.size<sizeof(oracle));
    CHECK((uintptr_t)nativeFighterPool.payload>=(uintptr_t)region.memory &&
        (uintptr_t)nativeFighterPool.payload+nativeFighterPool.capacity*nativeFighterPool.stride<=(uintptr_t)region.memory+region.size);
    memcpy(oracle,region.memory,region.size);
    HSD_GObjFree(a);CHECK(nativeMatchPool_Resolve(old)==NULL&&nativeFighterPool.used==1);
    Fighter_GObj* c=create(false);CHECK(c==a&&c->user_data==fp&&own->attributes[1059]==0&&own->parts[139].flagsC==0);
    CHECK(nativeFighterPool_Load(saved,bytes));
    CHECK(nativeMatchPool_Resolve(old)==fp&&own->attributes[1059]==0xAC&&own->parts[139].flagsC==0x12345678);
    CHECK(own->primary_dobjs[123]==(HSD_DObj*)&endpoint&&own->secondary_dobjs[31]==(HSD_DObj*)&alternate&&fp->native_module_state[255]==0xBA);
    memcpy(region.memory,oracle,region.size);CHECK(nativeFighterPool.used==2);++cases;
    expect_panic=true;if(setjmp(panic_jump)==0){Fighter_800679B0();CHECK(false);}expect_panic=false;
    CHECK(nativeFighterPool.used==2);++cases;
    HSD_GObjFree(a);HSD_GObjFree(b);CHECK(nativeFighterPool.used==0);
    retire_attach=true;CHECK(create(false)==NULL&&nativeFighterPool.used==0&&nativeGObjPool.used==0);++cases;
    retire_life=true;CHECK(create(true)==NULL&&nativeFighterPool.used==0&&nativeGObjPool.used==0);++cases;
    for(unsigned i=0;i<NATIVE_FIGHTER_POOL_CAPACITY;++i)CHECK(create((i&1)!=0));
    uint32_t gobjs=nativeGObjPool.used,gprocs=nativeGObjProcPool.used;
    CHECK(create(false)==NULL&&create(true)==NULL&&nativeFighterPool.used==64&&nativeGObjPool.used==gobjs&&nativeGObjProcPool.used==gprocs);++cases;
    CHECK(nativeFighterPool_Save(saved,sizeof(saved),&bytes));uint64_t epoch=nativeFighterPool.lifetime;
    scene();CHECK(nativeFighterPool.lifetime==epoch+1&&nativeFighterPool.used==0&&!nativeFighterPool_Load(saved,bytes));++cases;
    CHECK(nativeGObjPool_RetireScene());expect_panic=true;if(setjmp(panic_jump)==0){Fighter_FirstInitialize_80067A84();CHECK(false);}expect_panic=false;
    printf("{\"cases\":%u,\"checks\":%u,\"owner_slot_bytes\":%zu,\"actor_bytes\":%zu,\"full_snapshot_bytes\":%zu,\"normal_setups\":%u,\"demo_setups\":%u,\"global_oracle\":true}\n",
        cases,checks,sizeof(NativeFighterPoolSlot),sizeof(Fighter),bytes,normal_loads,demo_loads);
    return 0;
}
