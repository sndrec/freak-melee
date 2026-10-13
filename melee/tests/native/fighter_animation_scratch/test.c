#include <melee/gm/native_match_arena.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_script_fighter.h>
#include <melee/ft/native_fighter_pool.h>
#include <melee/lb/lbanim.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <sysdolphin/baselib/jobj.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Actors=8, Clips=4 };
HSD_ObjAllocData fighter_alloc_data,fighter_dat_attrs_alloc_data;
HSD_ObjAllocData fighter_parts_alloc_data,fighter_dobj_list_alloc_data,fighter_x2040_alloc_data;
#if EXPECT_LEGACY_BUFFERS
HSD_ObjAllocData fighter_x59C_alloc_data;
#endif
struct ftData_UnkCountStruct ftData_Table_Unk0[Ft_Kind_RuntimeCapacity],ftData_UnkIntPairs[Ft_Kind_RuntimeCapacity];
HSD_GObjEvent ftData_OnUserDataRemove[Ft_Kind_RuntimeCapacity];
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
static Fighter* fighters[Actors];static HSD_GObj objects[Actors];
NativeMatchPool nativeGObjPool;
static NativeMatchPoolSlot object_slots[Actors];
static uint32_t object_free_order[Actors];
static HSD_JObj joints[Actors][2],interpolation[Actors][2];
HSD_GObj* HSD_GObj_CurrentInvokedProcGObj;HSD_GObjProc* HSD_GObj_CurrentInvokedProc;
HSD_GObj* HSD_GObj_804D7818;struct _unk_gobj_struct HSD_GObj_DelayedProcInfo;
static struct Fighter_WaitAnimData rows[Clips],fallback_rows[Clips];
static FigaTree trees[Clips];static FigaTrack tracks[Clips][2];static s8 nodes[]={1,1,-1};
static ftData popo_data;
static Gm_PKind player_type=Gm_PKind_Human;
static unsigned checks,cases,allocated,freed_scratch,freed_other,attachments,removed_skeletons;
static void* guarded[Actors*2];static void* forbidden;
static HSD_JObj* expected_joints[2];static FigaTree* expected_tree;
static unsigned expected_part;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"line%u: %s\n",(unsigned)__LINE__,#x);assert(x);}}while(0)
void OSReport(char* format,...){(void)format;}
__declspec(noreturn) void __assert(char* file,u32 line,char* reason){fprintf(stderr,"%s:%u: %s\n",file,(unsigned)line,reason);abort();}
void* HSD_ObjAlloc(HSD_ObjAllocData* pool)
{
#if EXPECT_LEGACY_BUFFERS
    CHECK(pool==&fighter_x59C_alloc_data&&allocated<Actors*2);
    guarded[allocated]=VirtualAlloc(NULL,0x8000,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);
    CHECK(guarded[allocated]);return guarded[allocated++];
#else
    (void)pool;CHECK(false);return NULL;
#endif
}
void HSD_ObjFree(HSD_ObjAllocData* pool,void* pointer)
{
#if EXPECT_LEGACY_BUFFERS
    if(pool==&fighter_x59C_alloc_data){
        bool found=false;for(unsigned i=0;i<allocated;++i)if(guarded[i]==pointer)found=true;
        CHECK(found);++freed_scratch;return;
    }
#endif
    CHECK(pool==&fighter_alloc_data||pool==&fighter_dat_attrs_alloc_data||pool==&fighter_parts_alloc_data||pool==&fighter_dobj_list_alloc_data||pool==&fighter_x2040_alloc_data);
    (void)pointer;++freed_other;
}
void ftData_80085A14(FighterKind kind){CHECK(ftData_Table_Unk0[kind].count==Clips);}
Gm_PKind Player_GetPlayerSlotType(s32 slot){CHECK(slot>=0&&slot<Actors);return player_type;}
u32 ftParts_8007506C(FighterKind kind,int part){(void)kind;CHECK(part<2);return 0;}
void lbAnim_8001E6D8(HSD_JObj* joint,FigaTree* tree,FigaTrack* track,s8 frames)
{
    CHECK(expected_part<2&&joint==expected_joints[expected_part]&&tree==expected_tree&&frames==1);
    CHECK(track==&tree->tracks[expected_part]);++expected_part;++attachments;
}
// Complete actual unload body, with controlled owner cleanup primitives. Only
// scratch allocation/free and selection bodies are under investigation here.
bool nativeScriptFighter_IsAttached(Fighter_GObj* object){(void)object;return false;}
bool nativeScriptRuntime_Idle(void){return true;}
void nativeScriptFighter_Detach(Fighter_GObj* object){(void)object;}
void nativeArticleResource_CancelCreator(Fighter_GObj* object,uint32_t generation){(void)object;(void)generation;}
void nativeArticleResource_CancelConstructingAttachment(Fighter_GObj* object,NativeMatchPoolReference gobj,NativeMatchPoolReference owner)
{(void)object;(void)gobj;(void)owner;CHECK(false);}
void nativeItem_RemoveConstructingOwnerChildren(Fighter_GObj* object,NativeMatchPoolReference gobj,NativeMatchPoolReference owner)
{(void)object;(void)gobj;(void)owner;CHECK(false);}
void efLib_DestroyAll(Fighter_GObj* object){CHECK(object);}
void ftColl_8007B8E8(Fighter_GObj* object){(void)object;}
void efAsync_QueueClear(void* pointer){(void)pointer;}
void it_8026B7F8(HSD_GObj* object){(void)object;}
void Camera_800290D4(CmSubject* subject){(void)subject;}
void nativeFighterAttachment_Clear(Fighter_GObj* object){(void)object;}
void ftCo_UnloadDynamicBones(Fighter* fighter){(void)fighter;}
void ftColl_800765AC(Fighter_GObj* object){(void)object;}
void ft_80088C5C(Fighter_GObj* object){(void)object;}
void lbShadow_8000EE8C(LbShadow* shadow){(void)shadow;}
void HSD_JObjRemoveAll(HSD_JObj* joint){CHECK(joint!=NULL);++removed_skeletons;}
void HSD_JObjUnref(HSD_JObj* joint){(void)joint;}
void ftData_800859A8(Fighter* fighter){(void)fighter;}
void HSD_LObjRemoveAll(HSD_LObj* light){(void)light;}
void Player_80031FB0(int slot,s32 entity,HSD_GObj* expected){(void)slot;(void)entity;(void)expected;}

#include "selectors.inc"
#include "tracks.inc"
#include "unload.inc"

int main(void)
{
    forbidden=VirtualAlloc(NULL,0x8000,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);CHECK(forbidden);
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),object_slots,object_free_order,
        Actors,sizeof(HSD_GObj),_Alignof(HSD_GObj),1));
    CHECK(nativeMatchScene_Begin());
    CHECK(nativeFighterPool_BeginScene(1));
    for(unsigned i=0;i<Clips;++i){trees[i]=(FigaTree){.nodes=nodes,.tracks=tracks[i],.frames=20+(float)i};rows[i].x14=&trees[i];fallback_rows[i].x14=&trees[(i+1)%Clips];}
    popo_data.xC=fallback_rows;gFtDataList[Ft_Kind_Popo]=&popo_data;
    for(unsigned a=0;a<Actors;++a){
        NativeMatchPoolReference reference;NativeFighterPoolSlot* own=nativeMatchPool_Allocate(&nativeFighterPool,&reference);CHECK(own);
        NativeMatchPoolReference object_reference;
        HSD_GObj* object=nativeMatchPool_Allocate(&nativeGObjPool,&object_reference);CHECK(object);
        Fighter* f=fighters[a]=&own->actor;object->user_data=f;f->gobj=object;
        f->kind=a%2?Ft_Kind_Nana:Ft_Kind_Popo;f->player_id=(u8)a;f->x24=rows;f->parts=own->parts;
        f->dat_attrs_backup=own->attributes;f->dobj_list.data=own->primary_dobjs;f->x203C.data=own->secondary_dobjs;
        ftData_Table_Unk0[f->kind].count=Clips;ftData_UnkIntPairs[f->kind].count=Clips;
        ftData_UnkIntPairs[f->kind].data=(void*)&rows[0];
        if(a<4)ftData_80085B10(f);else ftData_80085B98(f,0,Clips-1);
#if EXPECT_LEGACY_BUFFERS
        CHECK(f->x59C&&f->x5A0);
#else
        CHECK(f->x59C==NULL&&f->x5A0==NULL);
#endif
        CHECK(f->x58C==Clips&&f->x5A4==NULL&&f->x5A8==NULL);
        // Independent poison pointers prove native selectors/track setup do not
        // dereference archive scratch regardless of allocation/removal policy.
        struct Fighter_x59C_t* owned0=f->x59C,*owned1=f->x5A0;
        f->x59C=f->x5A0=forbidden;f->x8AC_animSkeleton=&interpolation[a][0];
        for(unsigned part=0;part<2;++part){f->parts[part].joint=&joints[a][part];f->parts[part].x4_jobj2=&interpolation[a][part];f->parts[part].flags_b1=true;}
        for(unsigned motion=0;motion<Clips;++motion)for(unsigned blend=0;blend<2;++blend){
            ftData_80085CD8(f,f,(int)motion);CHECK(f->x590==rows[motion].x14&&f->x5A4==rows[motion].x14);
            CHECK(ftData_80085E50(f,(int)motion)==rows[motion].x14);
            Fighter saved=*f;f->x590=NULL;f->x5A4=NULL;*f=saved;
            expected_tree=rows[motion].x14;expected_part=0;
            expected_joints[0]=blend?f->parts[0].x4_jobj2:f->parts[0].joint;
            expected_joints[1]=blend?f->parts[1].x4_jobj2:f->parts[1].joint;
            ftAnim_8006F4C8(f,blend!=0,f->x590);CHECK(expected_part==2);
            CHECK(f->x8AC_animSkeleton==&interpolation[a][0]);++cases;
        }
        CHECK(ftData_80085E50(f,Clips)==NULL);FigaTree* previous=f->x590;
        ftData_80085CD8(f,f,Clips);CHECK(f->x590==previous);
        f->x59C=owned0;f->x5A0=owned1;
        Fighter_Unload_8006DABC(f);
        CHECK(nativeMatchPool_Free(&nativeGObjPool,object_reference));++cases;
    }
    // Native Nana fallback resolves to Popo's resident table; demo keeps its own
    // null entry. Neither choice reads the inaccessible archive scratch.
    Fighter nana={.kind=Ft_Kind_Nana,.player_id=0,.x58C=Clips,.x24=rows,.x59C=forbidden,.x5A0=forbidden};
    rows[2].x14=NULL;player_type=Gm_PKind_Cpu;
    CHECK(ftData_80085E50(&nana,2)==fallback_rows[2].x14);
    ftData_80085CD8(&nana,&nana,2);CHECK(nana.x590==fallback_rows[2].x14);
    player_type=Gm_PKind_Demo;CHECK(ftData_80085E50(&nana,2)==NULL);
    ftData_80085CD8(&nana,&nana,2);CHECK(nana.x590==NULL);++cases;
    CHECK(allocated==(EXPECT_LEGACY_BUFFERS?Actors*2:0)&&freed_scratch==allocated);
    CHECK(freed_other==0&&removed_skeletons==Actors&&nativeFighterPool.used==0);
    CHECK(nativeFighterPool_RetireScene());
    for(unsigned i=0;i<allocated;++i)CHECK(VirtualFree(guarded[i],0,MEM_RELEASE));
    CHECK(VirtualFree(forbidden,0,MEM_RELEASE));
    printf("{\"cases\":%u,\"checks\":%u,\"scratch_allocations\":%u,\"scratch_frees\":%u,\"track_attachments\":%u,\"interpolation_cleanup_calls\":%u,\"no_access_payloads\":true}\n",
        cases,checks,allocated,freed_scratch,attachments,removed_skeletons);
    return 0;
}
