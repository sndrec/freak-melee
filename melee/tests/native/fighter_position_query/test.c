#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/inlines.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_fighter_target.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static HSD_GObj objects[3];
static Fighter fighters[3];
static unsigned checks,cases;
#define CHECK(x) do { ++checks; if(!(x)){fprintf(stderr,"case%u line%d: %s\n",cases,__LINE__,#x);exit(1);} } while(0)
#include "bodies.h"

static void query(uint32_t id,bool expected,const Vec3* position)
{
    ++cases;Vec3 result={-9876.5f,4321.25f,777.f},before=result;
    CHECK(nativeFighterTarget_Position(id,&result)==expected);
    CHECK(!memcmp(&result,expected?position:&before,sizeof(result)));
}
int main(void)
{
    void* poison=VirtualAlloc(NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_NOACCESS);CHECK(poison);
    memset(objects,0,sizeof(objects));memset(fighters,0,sizeof(fighters));
    const uint32_t ids[]={1,UINT32_C(0x80000001),UINT32_MAX};
    for(unsigned i=0;i<3;++i){objects[i].user_data=&fighters[i];objects[i].next=i<2?&objects[i+1]:NULL;
        fighters[i].x8_spawnNum=ids[i];fighters[i].cur_pos=(Vec3){-10.25f-i,20.5f+i,30.75f-i};
        fighters[i].parts=poison;fighters[i].x890_cameraBox=poison;}
    heads[HSD_GOBJ_PLINK_FIGHTER]=objects;HSD_GObjPLinkHead=heads;
    for(unsigned i=0;i<3;++i){query(ids[i],true,&fighters[i].cur_pos);++cases;CHECK(!nativeFighterTarget_Position(ids[i],NULL));}
    query(0,false,NULL);query(2,false,NULL);query(UINT32_C(0x80000000),false,NULL);
    // Every read observes current position, rather than a retained/camera sample.
    const Vec3 points[]={{0,0,0},{-0.0f,FLT_MIN,-FLT_MIN},{FLT_MAX,-FLT_MAX,16777217.f},{1.125f,-200.f,3000.5f}};
    for(unsigned actor=0;actor<3;++actor)for(unsigned p=0;p<4;++p){fighters[actor].cur_pos=points[p];query(ids[actor],true,&points[p]);}
    ++cases;objects[1].user_data=NULL;query(ids[1],false,NULL);query(ids[2],true,&fighters[2].cur_pos);objects[1].user_data=&fighters[1];
    // Recycling the same object/user-data addresses must not revive its old ID.
    fighters[1].x8_spawnNum=UINT32_C(0x80000002);query(ids[1],false,NULL);query(UINT32_C(0x80000002),true,&fighters[1].cur_pos);
    HSD_GObjPLinkHead=NULL;query(ids[0],false,NULL);query(0,false,NULL);
    HSD_GObjPLinkHead=heads;heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;query(ids[0],false,NULL);
    // The former endpoint can be wholly inaccessible when it is not registered.
    HSD_GObj* stale=VirtualAlloc(NULL,65536,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(stale);
    Fighter* stale_fighter=(Fighter*)((unsigned char*)stale+4096);CHECK(sizeof(*stale_fighter)<65536-4096);
    stale->user_data=stale_fighter;stale_fighter->x8_spawnNum=UINT32_C(0x81234567);stale_fighter->cur_pos=(Vec3){4,5,6};
    heads[HSD_GOBJ_PLINK_FIGHTER]=stale;query(UINT32_C(0x81234567),true,&stale_fighter->cur_pos);
    heads[HSD_GOBJ_PLINK_FIGHTER]=objects;unsigned long old;CHECK(VirtualProtect(stale,65536,PAGE_NOACCESS,&old));query(UINT32_C(0x81234567),false,NULL);
    // NULL output/zero ID short-circuit before an otherwise inaccessible registry.
    HSD_GObjPLinkHead=poison;++cases;CHECK(!nativeFighterTarget_Position(1,NULL));query(0,false,NULL);
    HSD_GObjPLinkHead=heads;CHECK(VirtualFree(stale,0,MEM_RELEASE));CHECK(VirtualFree(poison,0,MEM_RELEASE));
    printf("PASS %u live position cases/%u checks: actual registry/full-width IDs/currentcur_pos/unchanged failed output/null/recycle/unlinked PAGE_NOACCESS; no camera/bone path\n",cases,checks);
    return 0;
}
