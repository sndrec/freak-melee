/* Unchanged four Chain solvers versus generated resource policy. Resource
 * channels and stage results are controlled, recorded engine boundaries. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftSeak/types.h>
#include <melee/ft/kinds/ftSeak/forward.h>
#include <melee/ft/kinds/ftSeak/ftseakspecials.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/kinds/itseakchain.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/itCharItems.h>
#include <melee/it/item.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/mp/mpcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>

static unsigned checks,pairs;
#define CHECK(v) do { ++checks;if(!(v)){fprintf(stderr,"line%d pair%u %s\n",__LINE__,pairs,#v);exit(1);} }while(0)
static Item item;static Fighter fighter;static HSD_GObj object,creator,*heads[32];
HSD_GObj**HSD_GObjPLinkHead=heads;
static Article article;static itSeakChain_Attrs attributes;static ItemLink links[30];
static Vec3 anchor;static unsigned count,contact_mode,probe_count,sound_count;
typedef struct Probe { unsigned node;Vec3 last,current; } Probe;
static Probe probes[128];
bool nativeModelEffect_Snapshot(HSD_GObj*g,uint32_t*id){(void)g;*id=0;return false;}
EF_Effect*nativeModelEffect_Resolve(HSD_GObj*g,uint32_t id){(void)g;(void)id;return NULL;}
static unsigned link_index(const ItemLink*link){CHECK(link>=links&&link<links+count);return (unsigned)(link-links);}
bool mpColl_80048844(CollData*coll)
{
    unsigned node=0;while(node<count&&coll!=&links[node].coll_data)++node;CHECK(node<count&&probe_count<128);
    probes[probe_count++]=(Probe){node,coll->last_pos,coll->cur_pos};
    coll->env_flags=contact_mode==1?1:contact_mode==2?0x8000:contact_mode==3?0x10000:0;
    if(contact_mode){coll->cur_pos.x+=.05f;coll->cur_pos.y+=.075f;}
    return contact_mode!=0;
}
int lbAudioAx_800237A8(enum_t id,int volume,int pan)
{CHECK(id==270149&&volume==127&&pan==64);++sound_count;return 3;}
static float resource_read(Item_GObj*g,unsigned index,NativeArticleResourceFloat channel)
{
    CHECK(g==&object&&index<count);Vec3*value;
    if(channel<3)value=&links[index].pos;
    else if(channel<6){value=&links[index].vel;channel-=3;}
    else if(channel<9){value=&links[index].coll_data.cur_pos;channel-=6;}
    else if(channel<12){value=&links[index].coll_data.last_pos;channel-=9;}
    else{CHECK(false);return 0;}
    return channel==0?value->x:channel==1?value->y:value->z;
}
static bool resource_write(Item_GObj*g,unsigned index,NativeArticleResourceFloat channel,float value)
{
    CHECK(g==&object&&index<count&&isfinite(value));Vec3*target;
    if(channel<3)target=&links[index].pos;
    else if(channel<6){target=&links[index].vel;channel-=3;}
    else if(channel<9){target=&links[index].coll_data.cur_pos;channel-=6;}
    else if(channel<12){target=&links[index].coll_data.last_pos;channel-=9;}
    else{CHECK(false);return false;}
    if(channel==0)target->x=value;else if(channel==1)target->y=value;else target->z=value;return true;
}
static int32_t resource_int(Item_GObj*g,unsigned index,NativeArticleResourceInt channel)
{CHECK(g==&object&&index<count&&channel==NativeArticleResource_EnvironmentFlags);return links[index].coll_data.env_flags;}
static bool resource_sweep(Item_GObj*g,unsigned index){CHECK(g==&object&&index<count);return mpColl_80048844(&links[index].coll_data);}
static bool resource_draw(Item_GObj*g,unsigned index,bool visible){CHECK(g==&object&&index<count);(void)visible;return true;}
static HSD_GObj* retained_creator(Item_GObj*g){CHECK(g==&object);return &creator;}

#include "generated.inc"
#include "parameters.inc"
#include "article_names.inc"
#include "article_solver_actual.inc"
#include "article_solver_native64_control.inc"

static unsigned field(const char*name){for(unsigned i=0;i<sizeof(scripted_article_2_state_fields)/sizeof(*scripted_article_2_state_fields);++i)if(!strcmp(name,scripted_article_2_state_fields[i].name))return scripted_article_2_state_fields[i].offset;CHECK(false);return 0;}
static void set_int(const char*name,int32_t value){memcpy(item.xDD4_itemVar.native_article.state+field(name),&value,4);}
static int32_t get_int(const char*name){int32_t value;memcpy(&value,item.xDD4_itemVar.native_article.state+field(name),4);return value;}
static void set_float(const char*name,unsigned index,float value){memcpy(item.xDD4_itemVar.native_article.state+field(name)+4*index,&value,4);}
static float get_float(const char*name,unsigned index){float value;memcpy(&value,item.xDD4_itemVar.native_article.state+field(name)+4*index,4);return value;}
static void set_bool(const char*name,unsigned index,bool value){item.xDD4_itemVar.native_article.state[field(name)+index]=value;}
static bool get_bool(const char*name,unsigned index){return item.xDD4_itemVar.native_article.state[field(name)+index]!=0;}
static void reset(bool generated,unsigned first,unsigned scenario)
{
    memset(&item,0,sizeof item);memset(&fighter,0,sizeof fighter);memset(&object,0,sizeof object);memset(&creator,0,sizeof creator);memset(heads,0,sizeof heads);memset(links,0,sizeof links);
    object.user_data=&item;object.classifier=HSD_GOBJ_CLASS_ITEM;heads[HSD_GOBJ_PLINK_ITEM]=&object;item.x1C=UINT32_C(0x80000001);item.entity=&object;item.kind=It_Kind_Seak_Chain;item.owner=&creator;item.xC4_article_data=&article;article.x4_specialAttributes=&attributes;
    creator.user_data=&fighter;creator.classifier=HSD_GOBJ_CLASS_FIGHTER;heads[HSD_GOBJ_PLINK_FIGHTER]=&creator;fighter.x8_spawnNum=UINT32_C(0xfedcba98);fighter.kind=generated?Ft_Kind_CustomFirst:Ft_Kind_Seak;item.facing_dir=scenario&1?-1:1;
    fighter.input.lstick[0]=(Vec2){scenario&2?.7f:.1f,scenario&4?-.8f:.1f};fighter.u.sk.lstick_delta=(Vec3){scenario&2?.6f:-.4f,scenario&4?.9f:-.6f,0};fighter.mv.sk.specials.x1C=scenario&1?3:0;
    script_state_set_float(&fighter,156,fighter.u.sk.lstick_delta.x);script_state_set_float(&fighter,160,fighter.u.sk.lstick_delta.y);script_state_set_int(&fighter,52,fighter.mv.sk.specials.x1C);
    anchor=(Vec3){scenario&8?12.f:2.5f,1,.2f};
    for(unsigned i=0;i<count;++i){links[i].prev=i+1<count?&links[i+1]:NULL;links[i].next=i?&links[i-1]:NULL;links[i].pos=(Vec3){i*.37f,(float)(i%3)*.3f,.2f};links[i].vel=(Vec3){(i%4-1.5f)*.21f,(i%3-1.f)*.17f,.03f};links[i].coll_data.cur_pos=(Vec3){links[i].pos.x-.1f,links[i].pos.y-.2f,.2f};links[i].coll_data.last_pos=(Vec3){-1,-2,.2f};links[i].x2C_b0=i>=first;links[i].x2C_b1=(scenario&2)!=0;links[i].x2C_b2=(scenario&4)!=0;}
    if(generated){item.native_article_author_kind=fighter.kind;item.xDD4_itemVar.native_article.attachment_owner=&creator;item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;set_int("tip",count-1);set_int("previous_rehit",scenario&1);set_int("previous_wall_flags",scenario&2?0x8000:0);set_float("anchor_x",0,anchor.x);set_float("anchor_y",0,anchor.y);set_float("anchor_z",0,anchor.z);for(unsigned i=0;i<count;++i){set_bool("active",i,links[i].x2C_b0);set_bool("touching",i,links[i].x2C_b1);set_bool("sound_latched",i,links[i].x2C_b2);}for(unsigned i=0;i<15;++i){set_float("history_x",i,.025f*i);set_float("history_y",i,-.03f*i);}}
    else{item.xDD4_itemVar.seakchain.x0=&links[count-1];item.xDD4_itemVar.seakchain.x4=links;item.xDD4_itemVar.seakchain.parent_gobj=&creator;item.xDD4_itemVar.seakchain.x18=scenario&1;item.xDD4_itemVar.seakchain.x10=scenario&2?0x8000:0;for(unsigned i=0;i<15;++i)item.xDD4_itemVar.seakchain.history[i]=(Vec3){.025f*i,-.03f*i,0};}
    probe_count=sound_count=0;
}
static ItemLink expected_links[30];static Probe expected_probes[128];static unsigned expected_probe_count,expected_sound_count;static Vec3 expected_history[15];static int expected_wall,expected_rehit;static float maximum_difference;
static void eq(float a,float b){float d=fabsf(a-b);if(d>maximum_difference)maximum_difference=d;if(!(d<=2e-6f*fmaxf(1,fmaxf(fabsf(a),fabsf(b)))))fprintf(stderr,"float expected=%.9g actual=%.9g delta=%.9g\n",a,b,d);CHECK(d<=2e-6f*fmaxf(1,fmaxf(fabsf(a),fabsf(b))));}
static void eqvec(Vec3 a,Vec3 b){eq(a.x,b.x);eq(a.y,b.y);eq(a.z,b.z);}
static void save(void){memcpy(expected_links,links,sizeof links);memcpy(expected_probes,probes,sizeof probes);memcpy(expected_history,item.xDD4_itemVar.seakchain.history,sizeof expected_history);expected_wall=item.xDD4_itemVar.seakchain.x10;expected_rehit=item.xDD4_itemVar.seakchain.x18;expected_probe_count=probe_count;expected_sound_count=sound_count;}
static void compare(bool control)
{
    CHECK(expected_probe_count==probe_count&&expected_sound_count==sound_count);
    for(unsigned i=0;i<probe_count;++i){CHECK(expected_probes[i].node==probes[i].node);eqvec(expected_probes[i].last,probes[i].last);eqvec(expected_probes[i].current,probes[i].current);}
    for(unsigned i=0;i<count;++i){eqvec(expected_links[i].pos,links[i].pos);eqvec(expected_links[i].vel,links[i].vel);eqvec(expected_links[i].coll_data.last_pos,links[i].coll_data.last_pos);eqvec(expected_links[i].coll_data.cur_pos,links[i].coll_data.cur_pos);CHECK(expected_links[i].coll_data.env_flags==links[i].coll_data.env_flags);CHECK(expected_links[i].x2C_b0==get_bool("active",i)&&expected_links[i].x2C_b1==get_bool("touching",i)&&expected_links[i].x2C_b2==get_bool("sound_latched",i));}
    if(control){for(unsigned i=0;i<15;++i){eq(expected_history[i].x,get_float("history_x",i));eq(expected_history[i].y,get_float("history_y",i));}CHECK(expected_wall==get_int("previous_wall_flags")&&expected_rehit==get_int("previous_rehit"));}++pairs;
}
