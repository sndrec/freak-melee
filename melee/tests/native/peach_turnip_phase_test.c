#include <melee/ft/fighter.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftPeach/ftpeachspeciallw.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/itpeachturnip.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
enum { State,Appearance,Bounce,Shield,Spin,Interact };
typedef struct PhaseEvent {int op,arg;float value;} PhaseEvent;
static PhaseEvent events[16],expected[16];static unsigned count,expected_count,checks,pairs;
static HSD_GObj object,source,reference,*heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static Item item;static Fighter fighter;static Article article;static ItemStateArray appearances;
static ItemAttr attr;static ItemCommonData common;ItemCommonData* it_804D6D28=&common;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"pair %u line %d: %s\n",pairs,__LINE__,#x);exit(1);}}while(0)
static void record(int op,int arg,float value){CHECK(count<16);events[count++]=(PhaseEvent){op,arg,value};}
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation){(void)g;*generation=0;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation){(void)g;(void)generation;return NULL;}
void __assert(char* file,u32 line,char* text){fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,text);exit(1);}
void Item_80268E5C(HSD_GObj* g,enum_t state,Item_StateChangeFlags flags){CHECK(g==&object);item.msid=state;record(State,flags,state);}
static void change(HSD_GObj* g,int state){Item_80268E5C(g,state,ITEM_ANIM_UPDATE);}
void it_80273670(Item_GObj* g,int descriptor,float frame){CHECK(g==&object);record(Appearance,descriptor,frame);}
void itColl_BounceOffVictim(Item_GObj* g){CHECK(g==&object);record(Bounce,0,0);item.x40_vel.x=-item.x40_vel.x;}
bool itColl_BounceOffShield(Item_GObj* g){CHECK(g==&object);record(Shield,0,0);return true;}
int it_80274658(Item_GObj* g,float value){CHECK(g==&object);record(Spin,0,value);return 0;}
bool it_8026B894(HSD_GObj* g,HSD_GObj* ref){CHECK(g==&object);record(Interact,ref==&source,0);if(item.owner==ref){item.owner=NULL;return true;}return false;}
static HSD_GObj* creator(HSD_GObj* g){CHECK(g==&object);return nativeCallbackSource_Alive(item.xDD4_itemVar.native_article.attachment_owner,item.xDD4_itemVar.native_article.creator_spawn_id,NativeCallbackSource_Fighter)?item.xDD4_itemVar.native_article.attachment_owner:NULL;}
#include "generated.inc"
#include "turnip_phase_actual.inc"
static const NativeFighterDefinition* definition;
static void reset(bool scripted,int state,float timer,float velocity,float gravity,float terminal,float multiplier,int ref)
{
    memset(&item,0,sizeof item);memset(&fighter,0,sizeof fighter);memset(heads,0,sizeof heads);count=0;
    object=(HSD_GObj){.user_data=&item};source=(HSD_GObj){.user_data=&fighter};
    heads[HSD_GOBJ_PLINK_ITEM]=&object;heads[HSD_GOBJ_PLINK_FIGHTER]=&source;
    item.entity=&object;item.x1C=0xFEDCBA98u;fighter.x8_spawnNum=0x81230001;
    item.kind=It_Kind_Peach_Turnip;item.owner=ref==2?&reference:&source;item.msid=state;
    item.xD44_lifeTimer=timer;item.xD48_halfLifeTimer=35;item.x40_vel=(Vec3){4,velocity,2};item.xC70=multiplier;item.facing_dir=-1;
    item.xCC_item_attr=&attr;attr.x10_fall_speed=gravity;attr.x14_fall_speed_max=terminal;common.x68_float=.75f;
    item.xC4_article_data=&article;article.xC_itemStates=&appearances;appearances.count=1;fighter.u.pe.veg_gobj=&object;
    if(scripted){scripted_article_set_state_int(&item,0,7);scripted_article_set_state_int(&item,4,30);item.xDD4_itemVar.native_article.attachment_owner=&source;item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;}
    else{item.xDD4_itemVar.peachturnip.xDD8=7;item.xDD4_itemVar.peachturnip.xDDC_damage=30;item.xDD4_itemVar.peachturnip.xDE4_owner=&source;}
    script_state_set_int(&fighter,script_state_base+4,123);
}
static void save(void){expected_count=count;memcpy(expected,events,sizeof events);}
static void compare(void){CHECK(count==expected_count&&!memcmp(events,expected,count*sizeof(*events)));++pairs;}
static bool equal_float(float a,float b){return memcmp(&a,&b,4)==0;}
int main(void)
{
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.item_creator=creator,.item_change_state_flags=Item_80268E5C,.item_change_state=change,
        .item_set_appearance=it_80273670,.item_expire_tick=it_802751D8,.item_fall=it_80272860,.item_gravity=Item_ApplyFallingPhysics,
        .item_reflect=it_80273030,.item_bounce=itColl_BounceOffVictim,.item_shield_bounce=itColl_BounceOffShield,.item_interact=it_8026B894};
    NativeFighterDefinition base={0};definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==1);
    const ItemLogicTable* logic=definition->article_logics[0].logic;CHECK(!logic->thrown&&!logic->states[2].collided&&!logic->states[3].collided);
    const float timers[]={-2,0,.1f,1,1.1f,35};
    for(unsigned state=0;state<5;state++)for(unsigned t=0;t<6;t++){
        reset(false,state,timers[t],0,0,0,1,0);bool wanted=state==1?itPeachturnip_UnkMotion1_Anim(&object):(state==2||state==3)?itPeachturnip_UnkMotion3_Anim(&object):itPeachturnip_UnkMotion4_Anim(&object);float timer=item.xD44_lifeTimer;save();
        reset(true,state,timers[t],0,0,0,1,0);bool got=logic->states[state].animated(&object);compare();CHECK(got==wanted&&equal_float(timer,item.xD44_lifeTimer));
    }
    const float velocities[]={-10,-2,-.0f,0,2,10},gravity[]={-.2f,0,.2f},terminal[]={0,2,9};
    for(unsigned state=0;state<5;state++)for(unsigned v=0;v<6;v++)for(unsigned g=0;g<3;g++)for(unsigned m=0;m<3;m++){
        reset(false,state,35,velocities[v],gravity[g],terminal[m],1,0);
        if(state==1)itPeachturnip_UnkMotion1_Phys(&object);else if(state==2||state==3)itPeachturnip_UnkMotion3_Phys(&object);else itPeachturnip_UnkMotion4_Phys(&object);
        float wanted=item.x40_vel.y;save();reset(true,state,35,velocities[v],gravity[g],terminal[m],1,0);logic->states[state].physics_updated(&object);compare();CHECK(equal_float(wanted,item.x40_vel.y));
    }
    for(unsigned operation=0;operation<6;operation++)for(unsigned value=0;value<4;value++){
        float multiplier=value*.75f;reset(false,2,8,3,.2f,2,multiplier,0);bool wanted=false;
        if(operation==0)itPeachTurnip_Logic56_Dropped(&object);else if(operation==1)wanted=itPeachTurnip_Logic56_DmgDealt(&object);else if(operation==2)wanted=itPeachTurnip_Logic56_Clanked(&object);else if(operation==3)wanted=itPeachTurnip_Logic56_Reflected(&object);else if(operation==4)wanted=itPeachTurnip_Logic56_HitShield(&object);else wanted=itPeachTurnip_Logic56_ShieldBounced(&object);
        Vec3 velocity=item.x40_vel;float timer=item.xD44_lifeTimer,facing=item.facing_dir;int state=item.msid;save();reset(true,2,8,3,.2f,2,multiplier,0);bool got=false;
        if(operation==0)logic->dropped(&object);else if(operation==1)got=logic->dmg_dealt(&object);else if(operation==2)got=logic->clanked(&object);else if(operation==3)got=logic->reflected(&object);else if(operation==4)got=logic->hit_shield(&object);else got=logic->shield_bounced(&object);
        compare();CHECK(got==wanted&&!memcmp(&velocity,&item.x40_vel,sizeof velocity)&&timer==item.xD44_lifeTimer&&facing==item.facing_dir&&state==item.msid);
    }
    for(unsigned ref=0;ref<3;ref++){
        HSD_GObj* other=ref==0?NULL:ref==1?&source:&reference;
        reset(false,2,8,3,.2f,2,1,ref);itPeachTurnip_Logic56_EvtUnk(&object,other);bool forgotten=item.xDD4_itemVar.peachturnip.xDE4_owner==NULL;HSD_GObj* owner=item.owner;save();
        reset(true,2,8,3,.2f,2,1,ref);logic->evt_unk(&object,other);compare();CHECK((item.xDD4_itemVar.native_article.attachment_owner==NULL)==forgotten&&item.owner==owner);
    }
    for(unsigned present=0;present<2;present++){
        reset(false,2,8,3,.2f,2,1,0);if(!present)item.xDD4_itemVar.peachturnip.xDE4_owner=NULL;
        itPeachTurnip_Logic56_Destroyed(&object);bool cleared=fighter.u.pe.veg_gobj==NULL;save();
        reset(true,2,8,3,.2f,2,1,0);if(!present)item.xDD4_itemVar.native_article.attachment_owner=NULL;
        logic->destroyed(&object);compare();CHECK((script_state_get_int(&fighter,script_state_base+4)==-1)==cleared);
    }
    printf("Turnip available phases: %u actual-original/generated pairs, %u checks. Thrown/stage response/resource chain excluded.\n",pairs,checks);return 0;
}
