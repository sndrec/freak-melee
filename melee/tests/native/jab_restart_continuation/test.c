#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack1.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned checks,pairs,safety;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x);exit(1);}}while(0)
enum{Policy=1,Jab,Rapid,Normal,Pickup,Motion};
typedef struct RestartEvent{int kind,motion,flags;}RestartEvent;
static RestartEvent events[8],expected_events[8];static unsigned count,expected_count;
static Fighter fighter;static HSD_GObj object,*heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static NativeFighterDefinition definition,replacement;static const NativeFighterDefinition* current;
static bool decision,picked;static int invalidation;
static void record(int kind,int motion,int flags){CHECK(count<8);events[count++]=(RestartEvent){kind,motion,flags};}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind){(void)kind;return current;}
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*id){(void)g;*id=0;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj*g,uint32_t id){(void)g;(void)id;return NULL;}
bool ftCo_Attack11_EnterMotion(Fighter_GObj*g,FtMotionId motion){CHECK(g==&object);record(Jab,motion,0);return true;}
static void doAttack12Rapid(Fighter_GObj*g){CHECK(g==&object);record(Rapid,ftCo_MS_Attack11,0);}
static void doAttack12Normal(Fighter_GObj*g){CHECK(g==&object);record(Normal,ftCo_MS_Attack12,0);}
bool ftpickupitem_80094790(Fighter_GObj*g){CHECK(g==&object);record(Pickup,0,0);return picked;}
void Fighter_ChangeMotionState(Fighter_GObj*g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj*donor){CHECK(g==&object&&frame==0&&rate==1&&blend==0&&!donor);record(Motion,motion,(int)flags);fighter.motion_id=motion;}
static bool restart(Fighter_GObj*g,int32_t motion){CHECK(g==&object);record(Policy,motion,decision);switch(invalidation){
    case 1:heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=(void*)1;break;
    case 2:fighter.x8_spawnNum++;break;
    case 3:fighter.kind=Ft_Kind_Donkey;break;
    case 4:current=&replacement;break;
    case 5:heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;object.user_data=NULL;break;
    }return decision;
}
#include "old.inc"
#include "new.inc"

static void init(FighterKind kind,uint32_t generation){memset(&fighter,0,sizeof fighter);memset(&object,0,sizeof object);memset(heads,0,sizeof heads);memset(&definition,0,sizeof definition);current=NULL;
    object.user_data=&fighter;object.classifier=HSD_GOBJ_CLASS_FIGHTER;heads[HSD_GOBJ_PLINK_FIGHTER]=&object;fighter.gobj=&object;fighter.kind=kind;fighter.x8_spawnNum=(s32)generation;fighter.allow_interrupt=true;fighter.x2218_b1=true;fighter.motion_id=14;count=0;invalidation=0;decision=false;picked=false;}
static int expected_motion,expected_allow,expected_b1;
static void save(void){expected_count=count;memcpy(expected_events,events,count*sizeof(*events));expected_motion=fighter.motion_id;expected_allow=fighter.allow_interrupt;expected_b1=fighter.x2218_b1;}
static void compare(void){CHECK(count==expected_count);for(unsigned i=0;i<count;++i){CHECK(events[i].kind==expected_events[i].kind);CHECK(events[i].motion==expected_events[i].motion);CHECK(events[i].flags==expected_events[i].flags);}CHECK(fighter.motion_id==expected_motion);CHECK(fighter.allow_interrupt==expected_allow);CHECK(fighter.x2218_b1==expected_b1);++pairs;}
int main(void){
    const FighterKind kinds[]={Ft_Kind_Mario,Ft_Kind_Mars,Ft_Kind_Pikachu,Ft_Kind_Pichu};
    const uint32_t generations[]={1,UINT32_C(0x80000000),UINT32_C(0xFEDCBA98),UINT32_MAX};
    for(unsigned k=0;k<4;++k)for(unsigned uid=0;uid<4;++uid)for(unsigned route=0;route<2;++route)for(unsigned policy=0;policy<3;++policy)for(unsigned result=0;result<2;++result)for(unsigned pickup=0;pickup<2;++pickup){
        init(kinds[k],generations[uid]);current=policy?&definition:NULL;definition.on_jab_restart=policy==2?restart:NULL;decision=result;picked=pickup;if(route)old_doAttack13(&object);else old_doAttack12(&object);save();
        init(kinds[k],generations[uid]);current=policy?&definition:NULL;definition.on_jab_restart=policy==2?restart:NULL;decision=result;picked=pickup;if(route)new_doAttack13(&object);else new_doAttack12(&object);compare();
    }
    for(unsigned uid=0;uid<4;++uid)for(unsigned route=0;route<2;++route)for(unsigned result=0;result<2;++result)for(unsigned mode=1;mode<=5;++mode){
        init(Ft_Kind_Mario,generations[uid]);current=&definition;definition.on_jab_restart=restart;decision=result;invalidation=mode;if(route)new_doAttack13(&object);else new_doAttack12(&object);
        CHECK(count==1&&events[0].kind==Policy&&events[0].motion==(route?ftCo_MS_Attack13:ftCo_MS_Attack12));CHECK(fighter.allow_interrupt&&fighter.x2218_b1&&fighter.motion_id==14);++safety;
    }
    /* Source absent from registry while its userdata remains readable. These
     * engine entrypoints require nonnull userdata on initial dispatch; null or
     * poisoned initial userdata is not falsely claimed as a supported caller. */
    for(unsigned route=0;route<2;++route)for(unsigned absent=0;absent<2;++absent){init(Ft_Kind_Mario,UINT32_C(0xFEDCBA98));current=&definition;definition.on_jab_restart=restart;
        if(absent)HSD_GObjPLinkHead=NULL;else heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
        if(route)new_doAttack13(&object);else new_doAttack12(&object);CHECK(count==0&&fighter.allow_interrupt&&fighter.motion_id==14);++safety;HSD_GObjPLinkHead=heads;}
    printf("PASS %u original/current pairs, %u checks, %u source safety cases\n",pairs,checks,safety);return 0;
}
