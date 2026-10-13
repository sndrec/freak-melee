/* Constructor-only comparison. Engine boundaries are explicit below; the
 * real owner backend and source registry are linked, not always-true mocks. */
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftPeach/types.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftPeach/ftpeachspeciallw.h>
#include <melee/ef/efsync.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_article.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itpeachturnip.h>
#include <melee/it/itCharItems.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Position, Random, Spawn, Lifetime, Attach, Appearance, Change, Advance,
       Scale, Pickup, Effect, Remove };
typedef struct CompositionEvent { int op, arg; float x,y,z; } CompositionEvent;
static CompositionEvent events[64], expected[64];
static unsigned count, expected_count, checks, pairs;
static HSD_GObj owner, other, object, previous, *heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static Fighter fighter, foreign;
static FighterBone bones[MAX_FT_PARTS];
static HSD_JObj joint;
static Item item;
static ftPe_DatAttrs tuning;
static ftData fighter_data;
static struct ftData_x8 attachment_data;
static Article article;
static ItemStateArray appearances;
static union { itPeachTurnipAttributes attr; unsigned char storage[88]; } faces;
static ItemLogicTable logic;
static FighterPartsTable rig;
static FighterPartsTable* rigs[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=rigs;
static const NativeFighterDefinition* definition;
static unsigned draws[3], draw_count, draw_cursor;
static bool scripted, allocation_failure, redirect_owner;
static uint32_t handle;
static SpawnItem observed;
static unsigned script_state_base;
static int32_t script_state_get_int(Fighter*,unsigned);
#define CHECK(x) do { ++checks; if(!(x)){fprintf(stderr,"pair %u line %d: %s\n",pairs,__LINE__,#x);exit(1);} }while(0)
static void record(int op,int arg,float x,float y,float z)
{ CHECK(count<64); events[count++]=(CompositionEvent){op,arg,x,y,z}; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==fighter.kind ? definition : NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation)
{ (void)g;*generation=0;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation)
{ (void)g;(void)generation;return NULL; }
void __assert(char* file,u32 line,char* text)
{ fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,text);exit(1); }
s32 HSD_Randi(s32 bound)
{ CHECK(bound>0&&draw_cursor<draw_count&&draws[draw_cursor]<(unsigned)bound); int v=draws[draw_cursor++];record(Random,bound,v,0,0);return v; }
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* pos)
{ CHECK(j==&joint&&offset==NULL);*pos=(Vec3){11,22,33};record(Position,109,pos->x,pos->y,pos->z); }
static unsigned bone_count(Fighter* f){CHECK(f==&fighter);return 114;}
void it_80275158(Item_GObj* g,f32 lifetime)
{ CHECK(g==&object);item.xD44_lifeTimer=lifetime;record(Lifetime,0,lifetime,0,0); }
void it_8026BD0C(Item_GObj* g){CHECK(g==&object);item.xDCD_flag.b2=true;}
void it_8026BCF4(Item_GObj* g){CHECK(g==&object);item.xDCD_flag.b2=false;}
void it_80273670(Item_GObj* g,int descriptor,f32 frame)
{ CHECK(g==&object);record(Appearance,descriptor,frame,0,0); }
void Item_80268E5C(HSD_GObj* g,enum_t state,Item_StateChangeFlags flags)
{ CHECK(g==&object);item.msid=state;record(Change,flags,state,0,0); }
static void change(HSD_GObj* g,int state){Item_80268E5C(g,state,ITEM_ANIM_UPDATE);}
void Item_802694CC(HSD_GObj* g){CHECK(g==&object);record(Advance,0,0,0,0);}
/* Explicit engine AB54 boundary, already covered by the separate unchanged
 * actual-body owner/attachment fixtures. Its callback runs before publication. */
void Item_8026AB54(Item_GObj* g,HSD_GObj* source,Fighter_Part part)
{
    CHECK(g==&object&&source==&owner&&part==109);
    CHECK(fighter.item_gobj==&previous);
    if(scripted) CHECK(script_state_get_int(&fighter,script_state_base+4)==77);
    else CHECK(fighter.u.pe.veg_gobj==&previous);
    record(Attach,part,0,0,0);item.owner=source;item.xDC4=part;
    if(logic.picked_up) logic.picked_up(g);
    if(redirect_owner)item.owner=&other;
}
void it_80274594(Item_GObj* g)
{
    CHECK(g==&object);Fighter* current=item.owner->user_data;
    item.scl=current->x34_scale.y;record(Scale,0,item.scl,0,0);
}
void ftpickupitem_80094818(HSD_GObj* g,bool visible)
{
    CHECK(g==&owner&&!visible&&fighter.item_gobj==&object&&!fighter.death2_cb&&!fighter.take_dmg_cb);
    if(scripted)CHECK(script_state_get_int(&fighter,script_state_base+4)==(int32_t)item.x1C);
    else CHECK(fighter.u.pe.veg_gobj==&object);
    record(Pickup,0,0,0,0);
}
void db_80225DD8(Item_GObj* g, Fighter_GObj* holder)
{ (void)g; (void)holder; CHECK(false); } /* This producer does not request the extra notification. */
EF_SpawnResult efSync_Spawn(s32 effect,HSD_GObj* g,...)
{
    CHECK(effect==1234&&g==&owner&&!fighter.death2_cb&&!fighter.take_dmg_cb);va_list args;va_start(args,g);Vec3* pos=va_arg(args,Vec3*);va_end(args);
    CHECK(pos->x==1&&pos->y==2&&pos->z==3);record(Effect,effect,pos->x,pos->y,pos->z);return(EF_SpawnResult){0};
}
void ftPe_Init_OnDeath2(HSD_GObj* g)
{ (void)g;CHECK(false); } /* Registration only: global cleanup is out of scope. */
void Item_8026A8EC(Item_GObj* g)
{ CHECK(g==&object);record(Remove,0,0,0,0);if(logic.destroyed)logic.destroyed(g);heads[HSD_GOBJ_PLINK_ITEM]=NULL; }
void it_802BD45C(Item_GObj* g){Item_8026A8EC(g);}
ItemKind itGetKind(Item_GObj* g){CHECK(g==&object);return item.kind;}
static HSD_GObj* allocate(SpawnItem* spawn)
{
    observed=*spawn;record(Spawn,spawn->kind,spawn->pos.x,spawn->pos.y,spawn->pos.z);
    CHECK(spawn->prev_pos.x==spawn->pos.x&&spawn->prev_pos.y==spawn->pos.y&&spawn->prev_pos.z==spawn->pos.z);
    CHECK(spawn->x0_parent_gobj==&owner&&spawn->x4_parent_gobj2==&owner&&spawn->x44_flag.b0);
    CHECK(spawn->vel.x==0&&spawn->vel.y==0&&spawn->vel.z==0&&spawn->x3C_damage==0&&spawn->x40==0&&spawn->facing_dir==fighter.facing_dir);
    if(allocation_failure)return NULL;
    heads[HSD_GOBJ_PLINK_ITEM]=&object;item.kind=spawn->kind;item.owner=&owner;item.entity=&object;item.x1C=handle;
    nativeItem_CaptureCreator(&object,&owner);return &object;
}
HSD_GObj* Item_80268B18(SpawnItem* spawn){return allocate(spawn);}
HSD_GObj* Item_CreateEngine(SpawnItem* spawn)
{ CHECK(spawn->kind==It_Kind_BombHei||spawn->kind==It_Kind_Dosei||spawn->kind==It_Kind_Sword);return allocate(spawn); }
Fighter_GObj* nativeArticle_Creator(Item_GObj* g)
{ return nativeItem_Creator(g); } /* Existing authored lookup, scoped stable identity. */
static int32_t create_article(Fighter_GObj* source,ItemKind kind,Vec3* pos,Vec3* velocity,float facing,bool origin,uint32_t group,float value,bool initialize);
static bool ref_exists(HSD_GObj* source,uint32_t id,ItemKind kind)
{ return source==&owner&&kind==It_Kind_Peach_Turnip&&nativeCallbackSource_Alive(&object,id,NativeCallbackSource_Item)&&item.kind==kind&&nativeArticle_Creator(&object)==source; }
static bool remove_owned(HSD_GObj* source,s32 id)
{ if(!ref_exists(source,(uint32_t)id,It_Kind_Peach_Turnip))return false;Item_8026A8EC(&object);return true; }

#include "generated.inc"
#include "parameters.inc"
#include "peach_composition_actual.inc"

static int32_t create_article(Fighter_GObj* source,ItemKind kind,Vec3* pos,Vec3* velocity,float facing,bool origin,uint32_t group,float value,bool initialize)
{
    CHECK(source==&owner&&kind==It_Kind_Peach_Turnip&&!origin&&group==0&&value==0&&initialize);
    SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawn(&spawn,source,pos,facing);spawn.vel=*velocity;
    if(!allocate(&spawn))return -1;
    item.native_article_author_kind=fighter.kind;definition->article_logics[0].on_created(&object);
    return (int32_t)handle;
}
static int32_t field(const char* name)
{ for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;CHECK(false);return -1; }
static void reset(bool script,int rare_count,int face_count,unsigned first,unsigned second,unsigned third,bool fail,bool redirect,float scale,float facing)
{
    scripted=script;allocation_failure=fail;redirect_owner=redirect;count=draw_cursor=0;draw_count=3;
    draws[0]=first;draws[1]=second;draws[2]=third;
    memset(&fighter,0,sizeof fighter);memset(&foreign,0,sizeof foreign);memset(&item,0,sizeof item);memset(heads,0,sizeof heads);
    memset(&logic,0,sizeof logic);memset(&tuning,0,sizeof tuning);memset(&faces,0,sizeof faces);
    owner=(HSD_GObj){.user_data=&fighter};other=(HSD_GObj){.user_data=&foreign};object=(HSD_GObj){.user_data=&item,.hsd_obj=&joint};
    heads[HSD_GOBJ_PLINK_FIGHTER]=&owner;owner.next=&other;
    fighter.kind=Ft_Kind_CustomFirst;fighter.x8_spawnNum=0xF0000001;fighter.parts=bones;bones[109].joint=&joint;
    fighter.dat_attrs=&tuning;fighter.ft_data=&fighter_data;fighter_data.x8=&attachment_data;attachment_data.x10=109;
    fighter.facing_dir=facing;fighter.cur_pos=(Vec3){1,2,3};fighter.x34_scale.y=scale;foreign.x34_scale.y=.375f;
    fighter.item_gobj=&previous;fighter.u.pe.veg_gobj=&previous;fighter.throw_flags_b0=1;
    tuning.x14=128;tuning.speciallw_item_table_count=rare_count;
    int weights[]={2,3,1}, kinds[]={It_Kind_BombHei,It_Kind_Dosei,It_Kind_Sword};
    for(unsigned i=0;i<3;i++){tuning.speciallw_item_table[i].randi_max=weights[i];tuning.speciallw_item_table[i].kind=kinds[i];}
    int fw[]={35,6,5,3,3,4,1,1},fd[]={2,2,2,2,2,6,12,30};
    faces.attr.x0_lifetime=140;faces.attr.x4_length=face_count;
    for(unsigned i=0;i<8;i++){faces.attr.x8[i].x0_odds=fw[i];faces.attr.x8[i].x4_damage=fd[i];}
    article.x4_specialAttributes=&faces.attr;article.xC_itemStates=&appearances;appearances.count=1;
    item.xC4_article_data=&article;item.xB8_itemLogicTable=&logic;item.scl=1.25f;item.throw_scale=1;item.native_article_author_kind=Ft_Kind_None;
    if(script){logic.picked_up=definition->article_logics[0].logic->picked_up;logic.destroyed=definition->article_logics[0].logic->destroyed;}
    else{logic.picked_up=itPeachTurnip_Logic56_PickedUp;logic.destroyed=itPeachTurnip_Logic56_Destroyed;}
    script_state_set_int(&fighter,field("vegetable_handle"),77);
    script_parameters[1].i=rare_count;script_parameters[11].i=face_count;
    rig.parts_num=114;rigs[fighter.kind]=&rig;
}
static void compare_trace(void)
{
    if(count!=expected_count||memcmp(events,expected,count*sizeof(*events))){
        for(unsigned i=0;i<count||i<expected_count;i++)fprintf(stderr,"%u old %d,%d,%g new %d,%d,%g\n",i,expected[i].op,expected[i].arg,expected[i].x,events[i].op,events[i].arg,events[i].x);
    }
    CHECK(count==expected_count&&!memcmp(events,expected,count*sizeof(*events)));++pairs;
}
static void constructor_pair(unsigned rc,unsigned fc,unsigned first,unsigned second,bool fail,bool redirect,float scale,float facing)
{
    reset(false,rc,fc,first,second,0,fail,redirect,scale,facing);
    spawnVeg(&owner);expected_count=count;memcpy(expected,events,sizeof events);
    ItemKind kind=item.kind;float scl=item.scl,remember=item.throw_scale,life=item.xD44_lifeTimer;int msid=item.msid;
    int face=item.xDD4_itemVar.peachturnip.xDD8,damage=item.xDD4_itemVar.peachturnip.xDDC_damage;
    bool sounds=item.xDCD_flag.b2,held=fighter.item_gobj==&object,published=fighter.u.pe.veg_gobj==&object,callbacks=fighter.death2_cb!=NULL;
    unsigned consumed=draw_cursor;
    reset(true,rc,fc,first,second,0,fail,redirect,scale,facing);
    definition->on_tick(&owner);compare_trace();
    CHECK(draw_cursor==consumed&&!fighter.throw_flags_b0&&item.kind==kind&&item.scl==scl&&item.throw_scale==remember&&item.xD44_lifeTimer==life&&item.msid==msid&&item.xDCD_flag.b2==sounds);
    CHECK((fighter.item_gobj==&object)==held&&(script_state_get_int(&fighter,field("vegetable_handle"))==(int32_t)handle)==published&&(fighter.death2_cb!=NULL)==callbacks&&(fighter.take_dmg_cb!=NULL)==callbacks);
    if(held&&kind==It_Kind_Peach_Turnip){CHECK(scripted_article_state_int(&item,0)==face&&scripted_article_state_int(&item,4)==damage&&scripted_article_state_bool(&item,8));}
}
int main(void)
{
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.random_int=HSD_Randi,.bone_count=bone_count,.joint_position=lb_8000B1CC,
        .create_article_at=create_article,.create_engine_item=nativeItem_CreateEngine,.attach_created_item=nativeItem_Attach,
        .inherit_created_item_scale=nativeItem_InheritOwnerScale,.bind_fighter_item=nativeItem_BindFighter,.clear_fighter_item=nativeItem_ClearBinding,
        .held_item_id=nativeItem_HeldId,.held_item_kind=itGetKind,.notify_item_pickup=ftpickupitem_80094818,.spawn_effect=efSync_Spawn,
        .item_set_lifetime=it_80275158,.item_set_appearance=it_80273670,.item_change_state=change,.item_advance_animation=Item_802694CC,
        .item_creator=nativeArticle_Creator,.article_ref_exists=ref_exists,.remove_owned_article=remove_owned};
    MotionState state={0};NativeFighterDefinition base={.motion_states=&state,.motion_state_count=1};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->state_field_count==5&&definition->article_logic_count==1);
    uint32_t values[28];for(unsigned i=0;i<28;i++)values[i]=fixture_parameter_bits[i];
    CHECK(FreakMelee_SetFighterParameters(values,28,fixture_parameter_schema));
    unsigned face_sums[]={35,41,46,49,52,56,57,58};
    const uint32_t handles[]={0,UINT32_C(0x80000001),UINT32_C(0xFEDCBA98),UINT32_C(0xFFFFFFFE)};
    for(unsigned h=0;h<4;h++)for(unsigned rc=1;rc<=3;rc++)for(unsigned fc=1;fc<=8;fc++)for(unsigned fail=0;fail<2;fail++)for(unsigned redirect=0;redirect<2;redirect++)for(unsigned facing=0;facing<2;facing++){
        handle=handles[h];
        unsigned rare_sum=rc==1?2:rc==2?5:6;
        for(unsigned draw=0;draw<=rare_sum;draw++){
            unsigned first=draw==rare_sum?1:0, second=draw==rare_sum?0:draw;
            /* Turnip draws the face as its second draw; rare items use no face. */
            unsigned last=first?face_sums[fc-1]-1:0;
            constructor_pair(rc,fc,first,first?last:second,fail,redirect,facing?2:.75f,facing?-1:1);
        }
    }
    /* Every face draw, not only each cumulative boundary. */
    handle=UINT32_C(0xFEDCBA98);
    for(unsigned fc=1;fc<=8;fc++)for(unsigned draw=0;draw<face_sums[fc-1];draw++)for(unsigned redirect=0;redirect<2;redirect++)
        constructor_pair(3,fc,127,draw,false,redirect,1.625f,-1);
    /* Exact rare gate bound and probability, with caller's two-draw order. */
    for(unsigned rc=1;rc<=3;rc++)for(unsigned first=0;first<128;first++){
        unsigned bound=rc==1?2:rc==2?5:6;
        for(unsigned second=0;second<(first==0?bound:1);second++){
            reset(false,rc,8,first,second,0,false,false,1,1);ItemKind expected_kind=getVeg(&owner);
            expected_count=count;memcpy(expected,events,sizeof events);unsigned consumed=draw_cursor;
            reset(true,rc,8,first,second,0,false,false,1,1);script_fn_choose_vegetable(&owner);compare_trace();
            CHECK(script_state_get_int(&fighter,field("vegetable_kind"))==expected_kind&&draw_cursor==consumed);
        }
    }
    /* Cue absence does not call constructor or overwrite an earlier binding. */
    reset(false,3,8,0,0,0,false,false,1,1);fighter.throw_flags_b0=0;spawnVeg(&owner);CHECK(count==0);
    reset(true,3,8,0,0,0,false,false,1,1);fighter.throw_flags_b0=0;definition->on_tick(&owner);
    CHECK(count==0&&fighter.item_gobj==&previous&&script_state_get_int(&fighter,field("vegetable_handle"))==77);++pairs;
    /* Repeated pickup and destruction use the actual callbacks; authored
     * motion phases and unrelated Peach resources are deliberately absent. */
    for(unsigned repeated=0;repeated<2;repeated++){
        reset(false,3,8,1,57,0,false,false,1,1);spawnVeg(&owner);count=0;
        item.xDD4_itemVar.peachturnip.xDD4.b0=repeated;itPeachTurnip_Logic56_PickedUp(&object);
        expected_count=count;memcpy(expected,events,sizeof events);int expected_state=item.msid;
        reset(true,3,8,1,57,0,false,false,1,1);definition->on_tick(&owner);count=0;
        scripted_article_set_state_bool(&item,8,repeated);definition->article_logics[0].logic->picked_up(&object);compare_trace();CHECK(item.msid==expected_state);
    }
    for(unsigned held=0;held<2;held++)for(unsigned draw=0;draw<=6;draw++){
        unsigned first=draw==6?1:0,second=draw==6?57:draw;
        reset(false,3,8,first,second,0,false,false,1,1);spawnVeg(&owner);count=0;if(!held)fighter.item_gobj=&previous;
        ftPe_SpecialLw_8011CFA0(&owner);expected_count=count;memcpy(expected,events,sizeof events);bool forgotten=fighter.u.pe.veg_gobj==NULL;
        reset(true,3,8,first,second,0,false,false,1,1);definition->on_tick(&owner);count=0;if(!held)fighter.item_gobj=&previous;
        script_fn_interrupt_vegetable(&owner);compare_trace();CHECK((script_state_get_int(&fighter,field("vegetable_handle"))==-1)==forgotten);
    }
    printf("Peach constructor composition: %u actual-original/generated pairs, %u checks passed. Engine allocation/AB54/geometry are explicit boundaries; no gameplay/article phase acceptance.\n",pairs,checks);
    return 0;
}
