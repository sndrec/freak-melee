#include <melee/ft/ftcommon.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0C88.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/ftchangeparam.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftmetal.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/kinds/ftCommon/types.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ef/native_model_effect.h>
#include <melee/pl/player.h>
#include <melee/pl/types.h>
#include <melee/it/it_26B1.h>
#include <melee/it/types.h>
#include <melee/pl/plbonuslib.h>
#include <melee/it/kinds/itrabbitc.h>
#include <melee/gm/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/debug.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#undef NDEBUG
#include <assert.h>

static unsigned checks, pairs, lifetime_cases, preconditions, item_cases;
#define CHECK(x) do { ++checks; assert(x); } while(0)
typedef struct Trace { unsigned event; intptr_t a,b,c; } Trace;
static Trace trace[64]; static unsigned count, mutation_at;
static unsigned mutation, victim;
static bool recursive_rejected;
static HSD_GObj objects[2], items[3];
static Item* item_data[3]; static Item replacement_item;
static ItemLogicTable item_logic[3], replacement_logic;
static unsigned item_mutation, item_victim;
static Fighter* fighters[2]; static Fighter saved[2];
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static ftData data[2]; static ftData_x8 attributes[2];
static NativeFighterDefinition definitions[2], replacement;
static const NativeFighterDefinition* active[2];
static ftCommonData common; ftCommonData* p_ftCommonData=&common;
StaticPlayer player_slots[Gm_Player_NumMax];
typedef struct _ftMapping { s8 internal_id; s8 extra_internal_id; s8 has_transformation; } ftMapping;
static ftMapping ftMapping_list[ChKind_RuntimeCapacity];
static u32 serial;
Fighter_ItemEvent ftData_OnItemPickupExt[Ft_Kind_RuntimeCapacity];
static intptr_t object_index(HSD_GObj* object)
{ return object==&objects[0]?0:object==&objects[1]?1:object==&items[0]?2:object==&items[1]?3:4; }
static void record(unsigned event,intptr_t a,intptr_t b,intptr_t c)
{
    CHECK(count<64);trace[count++]=(Trace){event,a,b,c};
    if(count!=mutation_at) return;
    if(item_mutation) {
        HSD_GObj* object=&items[item_victim];Item* item=item_data[item_victim];
        switch(item_mutation) {
        case 1: // Registry retirement; leave a dangling destination deliberately.
            if(item_victim==0) heads[HSD_GOBJ_PLINK_ITEM]=&items[1];
            else items[item_victim-1].next=object->next;
            { DWORD old;CHECK(VirtualProtect(item,sizeof(*item),PAGE_NOACCESS,&old)); }
            break;
        case 2: ++item->x1C;break;
        case 3: item->kind=It_Kind_Capsule;break;
        case 4: replacement_item=*item;object->user_data=&replacement_item;break;
        case 5: item->xB8_itemLogicTable=&replacement_logic;break;
        case 6: // A hook can replace/clear the source binding without removing item.
            if(item_victim==0) fighters[0]->x197C=NULL;
            else if(item_victim==1) fighters[0]->x1980=NULL;
            else fighters[0]->item_gobj=NULL;
            break;
        case 7: item->native_removal_in_progress=true;break;
        case 8: item->owner=&objects[1];break; // Healthy attachment ownership update.
        case 9: // Destination callback discarded the already copied binding.
            if(item_victim==0) fighters[1]->x197C=NULL;
            else if(item_victim==1) fighters[1]->x1980=NULL;
            else fighters[1]->item_gobj=NULL;
            break;
        default: assert(!"unknown item mutation");
        }
        return;
    }
    HSD_GObj* object=&objects[victim];Fighter* fighter=fighters[victim];
    switch(mutation) {
    case 1: // Retire then protect userdata; actual registry must prevent reads.
        if(victim==0) heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[1];else objects[0].next=NULL;
        { DWORD old;CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_NOACCESS,&old)); }
        break;
    case 2: ++fighter->x8_spawnNum;break; // same address, new generation
    case 3: fighter->kind=Ft_Kind_Mario;break;
    case 4: active[victim]=&replacement;break;
    case 5: object->user_data=&saved[victim];break;
    case 6: fighter->ft_data=&data[1-victim];break;
    case 7: recursive_rejected=!ftCommon_ActivatePartner(&objects[1],NULL);break;
    default: assert(!"unknown mutation");
    }
}
#define EVENT(id,a,b,c) record(id,(intptr_t)(a),(intptr_t)(b),(intptr_t)(c))
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if(kind==Ft_Kind_CustomFirst) return active[0];
    if(kind==Ft_Kind_CustomFirst+1) return active[1];
    return NULL;
}
bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* generation)
{ (void)object;(void)generation;assert(!"unexpected model query");return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t generation)
{ (void)object;(void)generation;assert(!"unexpected model query");return NULL; }
void __assert(char* file,u32 line,char* message)
{ fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,message);abort(); }
void OSReport(char* fmt,...) { va_list a;va_start(a,fmt);vfprintf(stderr,fmt,a);va_end(a); }
u32 Fighter_NewSpawn_80068E40(void) { return ++serial; }
u32 ftLib_80087460(Fighter_GObj* object)
{ CHECK(object->user_data);return ((Fighter*)object->user_data)->x8_spawnNum; }
void Player_LoadPlayerCoords(s32 slot,Vec3* p) { CHECK(slot==0);*p=(Vec3){11,22,33}; }
float Player_GetFacingDirection(s32 slot) { CHECK(slot==0);return -1; }
s32 Player_GetDamage(s32 slot) { CHECK(slot==0);return 5; }
float ftCommon_800804EC(Fighter* fighter) { (void)fighter;return 3; }
void Player_SetHPByIndex(s32 slot,s32 index,s32 percent) { EVENT(1,slot,index,percent); }
void ftColl_ApplyStarInvincibility(Fighter* f,s32 duration) { EVENT(2,f==fighters[1],duration,0); }
void ftCo_800C0358(Fighter* s,Fighter* d,s32 id) { CHECK(s==fighters[0]&&d==fighters[1]);EVENT(3,id,0,0); }
void ft_800880D8(Fighter* f) { EVENT(4,f==fighters[0],0,0); }
void ftCo_800C0200(Fighter* f,int color) { EVENT(5,f==fighters[1],color,0); }
void ftColl_8007B760(Fighter_GObj* g,int frames) { EVENT(6,object_index(g),frames,0); }
void ftLib_SetScale(HSD_GObj* g,float scale) { ((Fighter*)g->user_data)->x34_scale.y=scale;EVENT(7,object_index(g),0,0); }
void ftCo_800C8348(Fighter_GObj* g,int timer,int health) { EVENT(8,object_index(g),timer,health); }
void ftCo_800C8540(Fighter_GObj* g) { EVENT(9,object_index(g),0,0); }
void ftCo_800C88D4(Fighter_GObj* g,int timer,bool hidden) { EVENT(10,object_index(g),timer,hidden); }
void ftCo_800C8A64(Fighter_GObj* g) { EVENT(11,object_index(g),0,0); }
void it_8026B9A8(Item_GObj* item,HSD_GObj* g,Fighter_Part part)
{ CHECK(item->user_data);((Item*)item->user_data)->owner=g;EVENT(12,object_index(item),object_index(g),part); }
void it_802950D4(Item_GObj* item,bool hide) { EVENT(13,object_index(item),hide,0); }
void ftCommon_8007F948(Fighter_GObj* g,Fighter_GObj* item,s32 timer)
{ Fighter* f=g->user_data;f->x197C=item;f->x2014=timer;EVENT(14,object_index(g),object_index(item),timer); }
void ftCommon_8007FA00(Fighter_GObj* g) { EVENT(15,object_index(g),0,0); }
void ftCommon_8007FE84(Fighter_GObj* g,Fighter_GObj* item,s32 timer,float health)
{ Fighter* f=g->user_data;f->x1980=item;f->x2018=timer;f->x201C=common.x6F8;f->x2024=health;f->x2020=f->x2021=0;EVENT(16,object_index(g),object_index(item),timer); }
void ftCommon_8007FDA0(Fighter_GObj* g) { EVENT(17,object_index(g),0,0); }
void ftCo_800D105C(Fighter_GObj* g) { EVENT(18,object_index(g),0,0); }
void ftCamera_80076064(Fighter* f) { EVENT(19,f==fighters[1],0,0); }
void ft_800849EC(Fighter* s,Fighter* d) { CHECK(s==fighters[0]&&d==fighters[1]);EVENT(20,0,0,0); }
void ft_80081C88(Fighter_GObj* g,float scale) { (void)scale;EVENT(21,object_index(g),0,0); }
static void pickup(Fighter_GObj* g,bool a) { EVENT(22,object_index(g),a,0); }
void pl_8003E17C(int player,int role,Item_GObj* item)
{
    CHECK(player==0);CHECK(role==0);CHECK(item==&items[2]);
    // The actual counter's first item operation is itGetKind; prove that its
    // input still resolves before simulating that userdata read.
    uint32_t generation;
    CHECK(nativeCallbackSource_Snapshot(item,NativeCallbackSource_Item,&generation));
    CHECK(((Item*)item->user_data)->kind==It_Kind_L_Gun);
}
void un_80322314(void) { EVENT(23,0,0,0); }
void ftCo_800BFD04(Fighter_GObj* g) { ((Fighter*)g->user_data)->x221F_b3=true;EVENT(24,object_index(g),0,0); }
static void finish(Fighter_GObj* g) { ((Fighter*)g->user_data)->motion_id=356;EVENT(25,object_index(g),0,0); }

#include "actual_bodies.h"

static void initialize(unsigned flags,bool authored)
{
    for(unsigned i=0;i<2;++i) {
        DWORD old;CHECK(VirtualProtect(fighters[i],sizeof(Fighter),PAGE_READWRITE,&old));
        memset(fighters[i],0,sizeof(Fighter));memset(&objects[i],0,sizeof(HSD_GObj));
        memset(&definitions[i],0,sizeof(definitions[i]));
        Fighter* f=fighters[i];f->gobj=&objects[i];f->player_id=0;f->x221F_b4=i;
        f->x8_spawnNum=0x80000001+i;f->kind=authored?Ft_Kind_CustomFirst+i:i?Ft_Kind_Zelda:Ft_Kind_Seak;
        f->ft_data=&data[i];data[i].x8=&attributes[i];attributes[i].x10=3;attributes[i].x12=4;
        f->x34_scale=(Vec3){1,1.25f,1};f->cur_pos=(Vec3){2,3,4};f->prev_pos=(Vec3){5,6,7};
        f->pos_delta=(Vec3){8,9,10};f->self_vel=(Vec3){11,12,13};f->gr_vel=14;f->facing_dir=-1;
        f->dmg.x1830_percent=27.75f;f->shield_health=34;f->x1968_jumpsUsed=2;
        f->ground_or_air=flags&128?GA_Air:GA_Ground;
        f->x670_timer_lstick_tilt_x=31;f->x671_timer_lstick_tilt_y=39;
        memset(&f->input,0x25,sizeof(f->input));
        memset(f->native_module_state,i?0xA7:0x3C,sizeof(f->native_module_state));
        objects[i].user_data=f;objects[i].next=i?NULL:&objects[1];
        definitions[i].source_kind=Ft_Kind_None;definitions[i].on_form_activated=finish;
        active[i]=authored?&definitions[i]:NULL;
    }
    Fighter* s=fighters[0];s->x221D_b6=flags&1;s->x1990=flags&2?55:0;
    for(unsigned i=0;i<3;++i) {
        DWORD old;CHECK(VirtualProtect(item_data[i],sizeof(Item),PAGE_READWRITE,&old));
        memset(item_data[i],0,sizeof(Item));memset(&items[i],0,sizeof(HSD_GObj));
        item_data[i]->x1C=0x90000001+i;item_data[i]->kind=It_Kind_L_Gun;
        item_data[i]->xB8_itemLogicTable=&item_logic[i];item_data[i]->owner=&objects[0];
        items[i].user_data=item_data[i];items[i].next=i<2?&items[i+1]:NULL;
    }
    heads[HSD_GOBJ_PLINK_ITEM]=&items[0];
    ftData_OnItemPickupExt[fighters[1]->kind]=pickup;
    s->is_metal=(flags&4)!=0;s->metal_timer=56;s->metal_health=57;
    s->x2226_b4=(flags&8)!=0;s->x2226_b7=(flags&16)!=0;s->x2030=58;
    s->x197C=flags&32?&items[0]:NULL;s->x1980=flags&64?&items[1]:NULL;
    s->item_gobj=flags&256?&items[2]:NULL;s->x2014=59;s->x2018=60;s->x2024=61;
    s->x198C=7;objects[1].user_data=fighters[1];
    memset(player_slots,0,sizeof(player_slots));player_slots[0].player_character=ChKind_CustomFirst;
    player_slots[0].player_entity[0]=&objects[0];player_slots[0].player_entity[1]=&objects[1];
    player_slots[0].transformed[0]=0;player_slots[0].transformed[1]=1;
    ftMapping_list[ChKind_CustomFirst]=(ftMapping){Ft_Kind_CustomFirst,Ft_Kind_CustomFirst+1,true};
    heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[0];count=mutation_at=mutation=0;serial=0x80001000;
    common.x260_startShieldHealth=60;recursive_rejected=false;
    item_mutation=0;
}
static void parity(void)
{
    Trace expected_trace[64];unsigned expected_count;u8 roles[2];
    for(unsigned authored=0;authored<2;++authored) for(unsigned flags=0;flags<512;++flags) {
        initialize(flags,authored);original_transfer(&objects[0],finish);
        saved[0]=*fighters[0];saved[1]=*fighters[1];expected_count=count;
        memcpy(expected_trace,trace,sizeof(trace));roles[0]=player_slots[0].transformed[0];roles[1]=player_slots[0].transformed[1];
        initialize(flags,authored);CHECK(ftCommon_ActivatePartner(&objects[0],authored?NULL:finish));
        CHECK(memcmp(fighters[0],&saved[0],sizeof(Fighter))==0);
        CHECK(memcmp(fighters[1],&saved[1],sizeof(Fighter))==0);
        CHECK(count==expected_count);CHECK(memcmp(trace,expected_trace,count*sizeof(Trace))==0);
        CHECK(player_slots[0].transformed[0]==roles[0]&&player_slots[0].transformed[1]==roles[1]);
        CHECK(fighters[0]->x198C==2);CHECK(fighters[1]->x8_spawnNum==0x80001001);
        for(unsigned i=0;i<256;++i) { CHECK(fighters[0]->native_module_state[i]==0x3C);CHECK(fighters[1]->native_module_state[i]==0xA7); }
        ++pairs;
    }
}
static void failures(void)
{
    for(unsigned failure=0;failure<15;++failure) {
        initialize(511,true);HSD_GObj* caller=&objects[0];HSD_GObjEvent callback=NULL;
        switch(failure) {
        case 0: ftMapping_list[ChKind_CustomFirst].has_transformation=false;break;
        case 1: player_slots[0].player_entity[1]=NULL;break;
        case 2: player_slots[0].player_entity[1]=&objects[0];break;
        case 3: definitions[1].on_form_activated=NULL;break;
        case 4: callback=finish;break;
        case 5: active[0]=NULL;break;
        case 6: active[1]=NULL;break;
        case 7: caller=&objects[1];break;
        case 8: fighters[1]->player_id=1;break;
        case 9: definitions[1].source_kind=Ft_Kind_Zelda;break;
        case 10: for(unsigned i=0;i<NATIVE_CALLBACK_DISPATCH_LIMIT;++i) CHECK(nativeCallbackDispatch_Enter());break;
        case 11: active[0]=active[1]=NULL;break; // Retail needs an explicit finish.
        case 12: ftMapping_list[ChKind_CustomFirst].extra_internal_id=Ft_Kind_CustomFirst;break;
        case 13: player_slots[0].player_character=ChKind_RuntimeCapacity;break;
        case 14: heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[1];break;
        }
        saved[0]=*fighters[0];saved[1]=*fighters[1];
        CHECK(!ftCommon_ActivatePartner(caller,callback));CHECK(count==0);
        CHECK(memcmp(fighters[0],&saved[0],sizeof(Fighter))==0&&memcmp(fighters[1],&saved[1],sizeof(Fighter))==0);
        if(failure==10) for(unsigned i=0;i<NATIVE_CALLBACK_DISPATCH_LIMIT;++i) nativeCallbackDispatch_Leave();
        ++preconditions;
    }
    initialize(511,true);CHECK(ftCommon_ActivatePartner(&objects[0],NULL));unsigned full=count;
    for(unsigned at=1;at<=full;++at) for(victim=0;victim<2;++victim) for(unsigned mode=1;mode<=6;++mode) {
        initialize(511,true);mutation_at=at;mutation=mode;
        CHECK(!ftCommon_ActivatePartner(&objects[0],NULL));CHECK(count==at);++lifetime_cases;
        // Failed path must release transient reentrancy and dispatch state.
        initialize(511,true);CHECK(ftCommon_ActivatePartner(&objects[0],NULL));
    }
    initialize(511,true);mutation_at=full;mutation=7;
    CHECK(ftCommon_ActivatePartner(&objects[0],NULL));CHECK(recursive_rejected);++lifetime_cases;
    // Cover the mutually exclusive normal-color and visible-cloak return sites.
    const unsigned alternate_flags[]={510,495};
    const unsigned alternate_events[]={5,10};
    for(unsigned branch=0;branch<2;++branch) {
        initialize(alternate_flags[branch],true);CHECK(ftCommon_ActivatePartner(&objects[0],NULL));
        unsigned at=0;
        for(unsigned i=0;i<count;++i) if(trace[i].event==alternate_events[branch]) at=i+1;
        CHECK(at!=0);
        for(victim=0;victim<2;++victim) for(unsigned mode=1;mode<=6;++mode) {
            initialize(alternate_flags[branch],true);mutation_at=at;mutation=mode;
            CHECK(!ftCommon_ActivatePartner(&objects[0],NULL));CHECK(count==at);++lifetime_cases;
            initialize(511,true);CHECK(ftCommon_ActivatePartner(&objects[0],NULL));
        }
    }
}
static void destructive_items(void)
{
    const unsigned events[]={14,16,22};
    for(item_victim=0;item_victim<3;++item_victim) {
        initialize(511,true);CHECK(ftCommon_ActivatePartner(&objects[0],NULL));
        unsigned at=0;
        for(unsigned i=0;i<count;++i) if(trace[i].event==events[item_victim]) at=i+1;
        CHECK(at!=0);
        for(unsigned mode=1;mode<=9;++mode) {
            if(mode==8) continue;
            initialize(511,true);mutation_at=at;item_mutation=mode;
            CHECK(!ftCommon_ActivatePartner(&objects[0],NULL));CHECK(count==at);++item_cases;
            initialize(511,true);CHECK(ftCommon_ActivatePartner(&objects[0],NULL));
        }
        // Ownership updates are expected during transfer, not identity changes.
        initialize(511,true);mutation_at=at;item_mutation=8;
        CHECK(ftCommon_ActivatePartner(&objects[0],NULL));++item_cases;
    }
    // Retired/unregistered or currently removing input cannot start a transfer.
    for(item_victim=0;item_victim<3;++item_victim) for(unsigned mode=0;mode<2;++mode) {
        initialize(511,true);
        if(mode) item_data[item_victim]->native_removal_in_progress=true;
        else if(item_victim==0) heads[HSD_GOBJ_PLINK_ITEM]=&items[1];
        else items[item_victim-1].next=items[item_victim].next;
        saved[0]=*fighters[0];saved[1]=*fighters[1];
        CHECK(!ftCommon_ActivatePartner(&objects[0],NULL));CHECK(count==0);
        CHECK(memcmp(fighters[0],&saved[0],sizeof(Fighter))==0&&memcmp(fighters[1],&saved[1],sizeof(Fighter))==0);
        ++item_cases;
    }
}
int main(void)
{
    for(unsigned i=0;i<2;++i) { fighters[i]=VirtualAlloc(NULL,sizeof(Fighter),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(fighters[i]); }
    for(unsigned i=0;i<3;++i) { item_data[i]=VirtualAlloc(NULL,sizeof(Item),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);CHECK(item_data[i]); }
    parity();failures();destructive_items();
    printf("{\"checks\":%u,\"pairs\":%u,\"lifetime_cases\":%u,\"preconditions\":%u,\"item_cases\":%u}\n",checks,pairs,lifetime_cases,preconditions,item_cases);
    return 0;
}
