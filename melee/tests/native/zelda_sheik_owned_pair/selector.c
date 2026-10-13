#include <melee/ft/types.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ef/native_model_effect.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GET_FIGHTER(g) ((Fighter*)(g)->user_data)
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"selector line%d: %s\n",__LINE__,#x); abort(); } } while(0)
static unsigned checks, cases;
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static HSD_GObj source, victim;
static Fighter* source_data;
static Fighter victim_data;
static ftData source_ft, victim_ft;
static ftCo_DatAttrs source_attributes, victim_attributes;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
static NativeFighterDefinition source_definition, victim_definition;
static bool authored_source, authored_victim;
static int selected, mutation, callback_count, transfer_count;
static FtMotionId seen_source, seen_victim;
static float seen_speed;

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (authored_source && kind == source_data->kind) return &source_definition;
    if (authored_victim && kind == victim_data.kind) return &victim_definition;
    return NULL;
}

bool nativeModelEffect_Snapshot(HSD_GObj* g, uint32_t* id)
{ (void)g; (void)id; return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g, uint32_t id)
{ (void)g; (void)id; return NULL; }

static int selector(HSD_GObj* g, int motion, int fallback)
{
    CHECK(g == &source && motion >= 219 && motion <= 222);
    CHECK(fallback == motion - 219 + 239);
    ++callback_count;
    if (mutation == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = &victim;
        DWORD previous;
        CHECK(VirtualProtect(source_data, sizeof(*source_data), PAGE_NOACCESS, &previous));
        source.user_data = (void*)1;
    } else if (mutation == 2) ++source_data->x8_spawnNum;
    return selected;
}

void ftCo_800DD398(Fighter_GObj* g, FtMotionId msid, FtMotionId victim_msid, float speed)
{
    CHECK(g == &source);
    ++transfer_count;
    seen_source = msid;
    seen_victim = victim_msid;
    seen_speed = speed;
}

#include "selector_bodies.h"

static void reset(FighterKind attacker, FighterKind target)
{
    DWORD previous;
    CHECK(VirtualProtect(source_data,sizeof(*source_data),PAGE_READWRITE,&previous));
    memset(source_data,0,sizeof(*source_data));
    memset(&victim_data,0,sizeof(victim_data));
    memset(&source,0,sizeof(source)); memset(&victim,0,sizeof(victim));
    source.user_data=source_data; victim.user_data=&victim_data;
    source.next=&victim; heads[HSD_GOBJ_PLINK_FIGHTER]=&source;
    source_data->kind=attacker; source_data->x8_spawnNum=(int32_t)UINT32_C(0x80000013);
    victim_data.kind=target; victim_data.x8_spawnNum=17;
    source_data->victim_gobj=&victim;
    source_data->ft_data=&source_ft; victim_data.ft_data=&victim_ft;
    source_ft.x0=&source_attributes; victim_ft.x0=&victim_attributes;
    source_attributes.weight_independent_throws_mask=0;
    victim_attributes.weight=80; common.x37C=0.01f;
    source_definition=(NativeFighterDefinition){0};
    victim_definition=(NativeFighterDefinition){0};
    authored_source=authored_victim=false;
    callback_count=transfer_count=mutation=0;
}

int main(void)
{
    source_data=VirtualAlloc(NULL,sizeof(*source_data),MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(source_data != NULL); HSD_GObjPLinkHead=heads;
    const FighterKind attackers[]={Ft_Kind_Koopa,Ft_Kind_GKoops,Ft_Kind_Mario};
    const FighterKind families[]={Ft_Kind_Peach,Ft_Kind_Zelda,Ft_Kind_Seak,Ft_Kind_Mario};
    for (unsigned a=0;a<3;++a) for (unsigned f=0;f<4;++f)
    for (int custom=0;custom<2;++custom) for (int ms=219;ms<=222;++ms)
    for (int independent=0;independent<2;++independent) {
        reset(attackers[a],custom?Ft_Kind_CustomFirst+1:families[f]);
        authored_victim=custom;
        victim_definition.source_kind=Ft_Kind_None;
        victim_definition.animation_source_kind=families[f];
        source_attributes.weight_independent_throws_mask=independent?(1U<<(ms-219)):0;
        ftCo_800DD4B0(&source,ms);
        int expected=(a<2 && ms==222 && f<2)?243:ms-219+239;
        CHECK(transfer_count==1 && callback_count==0);
        CHECK(seen_source==ms && seen_victim==expected);
        CHECK(seen_speed==(independent?1.0f:1.0f/(80.0f*common.x37C)));
        ++cases;
    }
    const int choices[]={0,243,ftCo_MS_Count-1,-1,ftCo_MS_Count};
    for (unsigned i=0;i<5;++i) for (int ms=219;ms<=222;++ms) {
        reset(Ft_Kind_Koopa,Ft_Kind_Zelda); authored_source=true;
        source_definition.on_throw_victim_motion=selector; selected=choices[i];
        ftCo_800DD4B0(&source,ms);
        CHECK(callback_count==1 && transfer_count==1);
        CHECK(seen_victim==(selected>=0 && selected<ftCo_MS_Count?selected:ms-219+239));
        ++cases;
    }
    for (int mode=1;mode<=2;++mode) {
        reset(Ft_Kind_CustomFirst,Ft_Kind_Zelda); authored_source=true;
        source_definition.on_throw_victim_motion=selector; selected=243; mutation=mode;
        ftCo_800DD4B0(&source,222);
        CHECK(callback_count==1 && transfer_count==0); ++cases;
    }
    DWORD previous;
    CHECK(VirtualProtect(source_data,sizeof(*source_data),PAGE_READWRITE,&previous));
    CHECK(VirtualFree(source_data,0,MEM_RELEASE));
    printf("{\"checks\":%u,\"cases\":%u,\"destructive\":2}\n",checks,cases);
    return 0;
}
