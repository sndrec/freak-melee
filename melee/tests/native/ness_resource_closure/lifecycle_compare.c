#include "behavior.generated.h"
#include <melee/ft/kinds/ftNess/ftness.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftNess/ftnessattacks4.h>
#include <melee/ft/kinds/ftNess/ftnessspecialn.h>
#include <melee/ft/kinds/ftNess/ftnessspecialhi.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/it/kinds/itnessbat.h>
#include <melee/it/kinds/itnessyoyo.h>
#include <melee/it/kinds/itnesspkflash.h>
#include <melee/it/kinds/itnesspkthunderball.h>
#include <melee/it/kinds/itnesspkthundertrail.h>
#include <melee/it/it_26B1.h>
#include <melee/it/item.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/lb/lb_00B0.h>
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>

static Fighter fighters[2]; static HSD_GObj owners[2];
static Item items[2][10]; static HSD_GObj objects[2][10];
static ItemLink links[2][20]; static HSD_GObj nodes[2][20];
static bool removed[2][10];
static unsigned family, checks, failures;
typedef struct LifeEvent { unsigned operation, object, flags; } LifeEvent;
static LifeEvent events[2][32]; static unsigned counts[2];
static const NativeFighterDefinition* definition;
static bool creation_success;
static unsigned constructor_calls[2];
static FighterBone bones[2][43];
static HSD_JObj anchor;
enum { Flash, Thunder, Bat, Yoyo, Trail };
static void check(bool valid, const char* name)
{ ++checks; if(!valid){if(failures++<12)fprintf(stderr,"%s\n",name);} }
static void event(unsigned op,unsigned id)
{
    unsigned flags=0;
    for(unsigned i=0;i<10;++i)if(items[family][i].owner)flags|=1u<<i;
    events[family][counts[family]++]=(LifeEvent){op,id,flags};
}
static unsigned item_index(HSD_GObj* object)
{ for(unsigned i=0;i<10;++i)if(object==&objects[family][i])return i;abort(); }
static unsigned offset(const char* name)
{ for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;abort(); }
static void set(const char* name,int32_t value)
{ memcpy(fighters[1].native_module_state+offset(name),&value,4); }
static int32_t get(const char* name)
{ int32_t value;memcpy(&value,fighters[1].native_module_state+offset(name),4);return value; }
void Item_8026A8EC(Item_GObj* object)
{ unsigned i=item_index(object);event(2,i);removed[family][i]=true; }
void HSD_GObjFree(HSD_GObj* object)
{ for(unsigned i=0;i<20;++i)if(object==&nodes[family][i]){event(1,i);return;}abort(); }
void efLib_DestroyAll(HSD_GObj* object)
{ check(object==&owners[family],"effects owner");event(3,0); }
void ftParts_80074A4C(HSD_GObj* object,int group,int value)
{ check(object==&owners[family]&&group==0&&value==0,"death visibility");event(5,0); }
static void rotation(Fighter* fighter,int part,float value)
{ check(fighter==&fighters[family]&&part==0&&value==0,"root rotation contract");event(4,0); }
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId id,MotionFlags flags,float start,float rate,float blend,Fighter_GObj* donor)
{ check(object==&owners[0]&&id==341&&!flags&&start==0&&rate==1&&blend==0&&!donor,"original smash common entry");fighters[0].motion_id=id; }
void ftAnim_8006EBA4(Fighter_GObj* object)
{ check(object==&owners[0],"original smash animation setup"); }
void lb_8000B1CC(HSD_JObj* joint,Vec3* local,Vec3* position)
{ check(joint==&anchor&&!local,"raw42 anchor query");*position=(Vec3){3.5f,-4.0f,0}; }
Item_GObj* it_802AD478(Item_GObj* object,Vec3* position,Fighter_Part part,float facing)
{ check(object==&owners[0]&&part==42&&position->x==3.5f&&position->y==-4.0f&&facing==-1,"original Bat constructor inputs");++constructor_calls[0];return creation_success?&objects[0][Bat]:NULL; }
HSD_GObj* it_802BE9D8(HSD_GObj* object,float facing,Vec3* position,s32 motion)
{ check(object==&owners[0]&&motion==345&&facing==-1&&position->x==3.5f&&position->y==-4.0f&&position->z==0,"original YoYo constructor inputs");++constructor_calls[0];return creation_success?&objects[0][Yoyo]:NULL; }
#define ftPartSetRotX rotation
#define GET_ITEM(gobj) ((Item*)(gobj)->user_data)
#define GET_FIGHTER(gobj) ((Fighter*)(gobj)->user_data)
#include "original.h"
#undef ftPartSetRotX

static bool alive(HSD_GObj* object,uint32_t generation,uint32_t kind)
{
    if(kind==NativeCallbackSource_Fighter)return object==&owners[1]&&object->user_data==&fighters[1]&&fighters[1].x8_spawnNum==generation;
    for(unsigned i=0;i<10;++i)if(object==&objects[1][i])return !removed[1][i]&&object->user_data==&items[1][i]&&items[1][i].x1C==generation;
    return false;
}
static uint32_t owner_id(Item_GObj* object)
{ return ((Item*)object->user_data)->owner ? fighters[1].x8_spawnNum : 0; }
static bool target(uint32_t id,uint32_t name)
{
    check(id==fighters[1].x8_spawnNum&&name==script_callback_exports[0].name_hash,"recipient full identity/export hash");
    script_callback_exports[0].callback(&owners[1]);return true;
}
static void release(Item_GObj* object)
{ check(object==&objects[1][Yoyo],"release collection owner");for(int i=19;i>=0;--i)HSD_GObjFree(&nodes[1][i]); }
static bool signal(HSD_GObj* source,uint32_t handle,uint32_t hash,float value)
{
    check(source==&owners[1]||source==&objects[1][Yoyo]||source==&objects[1][Thunder],"signal actual caller");
    for(unsigned i=0;i<10;++i)if(items[1][i].x1C==handle&&!removed[1][i]) {
        unsigned index=i<Trail?i:Trail;
        const NativeFighterArticleLogic* logic=&definition->article_logics[index];
        for(unsigned s=0;s<logic->signal_count;++s)if(logic->signals[s].name_hash==hash){logic->signals[s].callback(&objects[1][i],value);return true;}
    }
    return false;
}
static bool bind_item(Fighter_GObj* object,uint32_t handle,bool auxiliary)
{ check(object==&owners[1]&&auxiliary,"auxiliary publication");for(unsigned i=0;i<10;++i)if(items[1][i].x1C==handle){fighters[1].x1984_heldItemSpec=&objects[1][i];return true;}return false; }
static bool clear_item(Fighter_GObj* object,bool auxiliary)
{ check(object==&owners[1]&&auxiliary,"auxiliary clear");fighters[1].x1984_heldItemSpec=NULL;return true; }
static unsigned bone_count(Fighter* fighter)
{ check(fighter==&fighters[1],"bone count source");return 43; }
static int32_t create_article(Fighter_GObj* object,ItemKind kind,Vec3* position,Vec3* velocity,float facing,bool origin,uint32_t group,float value,bool initial_collision)
{
    check(object==&owners[1]&&position->x==3.5f&&position->y==-4&&position->z==0&&facing==-1&&origin&&!group,"authored constructor origin/owner");
    check(velocity->x==0&&velocity->y==0&&velocity->z==0,"constructor zero velocity");
    check((kind==It_Kind_Ness_Bat&&value==0&&!initial_collision)||(kind==It_Kind_Ness_Yoyo&&value==345&&initial_collision),"constructor mode/collision selection");
    ++constructor_calls[1];return creation_success?items[1][kind==It_Kind_Ness_Bat?Bat:Yoyo].x1C:-1;
}
static void copy_attrs(Fighter_GObj* object){check(object==&owners[1],"attribute copy owner");event(6,0);}
static void sentinel(HSD_GObj* object){(void)object;}
static void initialize(unsigned mask,int motion,unsigned reflected,unsigned pause)
{
    memset(fighters,0,sizeof(fighters));memset(items,0,sizeof(items));memset(removed,0,sizeof(removed));memset(counts,0,sizeof(counts));
    for(unsigned f=0;f<2;++f){
        owners[f]=(HSD_GObj){.user_data=&fighters[f],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        fighters[f].x8_spawnNum=0x80000099u;fighters[f].motion_id=motion;
        fighters[f].death2_cb=sentinel;fighters[f].take_dmg_cb=sentinel;fighters[f].u.ns.pkthunder_gfx=true;
        for(unsigned i=0;i<10;++i){
            objects[f][i]=(HSD_GObj){.user_data=&items[f][i]};items[f][i].entity=&objects[f][i];
            items[f][i].x1C=0x80000010u+i;items[f][i].owner=&owners[f];items[f][i].xDC8_word.flags.x13=true;
            items[f][i].xDD4_itemVar.native_article.attachment_owner=&owners[f];
            items[f][i].xDD4_itemVar.native_article.creator_spawn_id=fighters[f].x8_spawnNum;
        }
        items[f][Yoyo].xDC8_word.flags.x3=pause&1;items[f][Yoyo].xDC8_word.flags.x7=(pause>>1)&1;
        for(unsigned i=0;i<20;++i){nodes[f][i]=(HSD_GObj){0};links[f][i].gobj=&nodes[f][i];links[f][i].next=i?&links[f][i-1]:NULL;}
    }
    fighters[0].u.ns.pkflash_gobj=mask&1?&objects[0][Flash]:NULL;
    fighters[0].u.ns.pkthunder_gobj=mask&2?&objects[0][Thunder]:NULL;
    fighters[0].u.ns.bat_gobj=mask&4?&objects[0][Bat]:NULL;
    fighters[0].u.ns.yoyo_gobj=mask&8?&objects[0][Yoyo]:NULL;
    items[0][Bat].xDD4_itemVar.nessbat.x0=&owners[0];items[0][Yoyo].xDD4_itemVar.nessyoyo.x8=&links[0][19];
    items[0][Thunder].xDD4_itemVar.pkthunder.xF00=reflected;
    for(unsigned i=0;i<6;++i)items[0][Thunder].xDD4_itemVar.pkthunder.xDD4[i]=&objects[0][Trail+i];
    set("flash_handle",mask&1?items[1][Flash].x1C:-1);set("thunder_ball_handle",mask&2?items[1][Thunder].x1C:-1);
    set("bat_handle",mask&4?items[1][Bat].x1C:-1);set("yoyo_handle",mask&8?items[1][Yoyo].x1C:-1);
    fighters[1].native_module_state[offset("thunder_effects_active")]=true;
    const NativeFighterArticleLogic* ball=&definition->article_logics[Thunder];
    for(unsigned i=0;i<ball->state_field_count;++i){const NativeFighterStateField* field=&ball->state_fields[i];
        if(!strcmp(field->name,"reflected"))items[1][Thunder].xDD4_itemVar.native_article.state[field->offset]=reflected;
        if(!strcmp(field->name,"trail_handles"))for(unsigned j=0;j<6;++j)memcpy(items[1][Thunder].xDD4_itemVar.native_article.state+field->offset+4*j,&items[1][Trail+j].x1C,4);
    }
}
static void compare(void)
{
    check(!fighters[0].u.ns.pkflash_gobj&&get("flash_handle")==-1,"Flash handle");
    check(!fighters[0].u.ns.pkthunder_gobj&&get("thunder_ball_handle")==-1,"Thunder handle");
    check(!fighters[0].u.ns.bat_gobj&&get("bat_handle")==-1,"Bat handle");
    check(!fighters[0].u.ns.yoyo_gobj&&get("yoyo_handle")==-1,"YoYo handle");
    check(fighters[0].u.ns.pkthunder_gfx==fighters[1].native_module_state[offset("thunder_effects_active")],"GFX flag");
    check(!fighters[0].take_dmg_cb&&!fighters[1].take_dmg_cb&&!fighters[0].death2_cb&&!fighters[1].death2_cb,"callbacks clear");
    for(unsigned i=0;i<10;++i){check((items[0][i].owner!=NULL)==(items[1][i].owner!=NULL),"resource owner");check(items[0][i].xDC8_word.flags.x13==items[1][i].xDC8_word.flags.x13,"owner-bound flag");check(removed[0][i]==removed[1][i],"removed item");}
    check(items[0][Yoyo].xDC8_word.flags.x3==items[1][Yoyo].xDC8_word.flags.x3,"pause clear");
    check(items[0][Yoyo].xDC8_word.flags.x5==items[1][Yoyo].xDC8_word.flags.x5,"resume x7->x5");
    check(counts[0]==counts[1]&&!memcmp(events[0],events[1],counts[0]*sizeof(LifeEvent)),"ordered releases/owners/effects/root");
}
int main(void)
{
    NativeFighterHost host={.copy_ext_attributes=copy_attrs,.reload_ext_attributes=copy_attrs,
        .set_visibility_group=ftParts_80074A4C,.callback_source_alive=alive,.signal_article=signal,
        .destroy_effects=efLib_DestroyAll,.set_root_rotation_x=rotation,.bind_fighter_item=bind_item,
        .clear_fighter_item=clear_item,.target_call=target,.item_pause=it_8026B724,.item_resume=it_8026B73C,
        .item_remove=Item_8026A8EC,.item_current_owner=owner_id,.article_resource_release=release,
        .create_article_at=create_article,.bone_count=bone_count,.joint_position=lb_8000B1CC};
    MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataNess",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    definition=FreakMelee_GetFighterModule(&base,&host);if(!definition)return 2;
    script_parameters[0].i=358;script_parameters[1].i=366;script_parameters[2].i=6;
    script_parameters[3].i=42;script_parameters[5].i=0;script_parameters[6].i=1;
    for(unsigned mask=0;mask<16;++mask)for(int m=357;m<=367;++m)for(unsigned r=0;r<2;++r)for(unsigned p=0;p<4;++p){
        initialize(mask,m,r,p);family=0;ftNs_Init_OnDamage(&owners[0]);family=1;script_fn_resource_damage(&owners[1]);compare();
    }
    initialize(15,361,0,3);family=0;ftNs_Init_OnDeath(&owners[0]);family=1;definition->on_death(&owners[1]);
    check(get("bat_handle")==-1&&get("yoyo_handle")==-1&&!fighters[1].native_module_state[offset("thunder_effects_active")],"death reset");
    check(counts[0]==1&&counts[1]==1&&!memcmp(events[0],events[1],sizeof(LifeEvent)),"death hides only; no cleanup");
    family=1;definition->on_load(&owners[1]);check(get("flash_handle")==-1&&get("thunder_ball_handle")==-1,"load initialized sentinels");
    for(unsigned success=0;success<2;++success)for(unsigned callback_mask=0;callback_mask<4;++callback_mask)for(unsigned kind=0;kind<2;++kind){
        initialize(0,kind?345:341,0,0);creation_success=success;
        memset(constructor_calls,0,sizeof(constructor_calls));
        for(unsigned f=0;f<2;++f){
            fighters[f].cur_pos=(Vec3){3.5f,-4.0f,9};fighters[f].facing_dir=-1;
            fighters[f].death2_cb=callback_mask&1?sentinel:NULL;fighters[f].take_dmg_cb=callback_mask&2?sentinel:NULL;
            fighters[f].parts=bones[f];bones[f][42].joint=&anchor;
            fighters[f].x1984_heldItemSpec=&objects[f][Flash];
        }
        family=0;if(kind)ftNs_AttackHi4_YoyoCreateItem(&owners[0]);else ftNs_AttackS4_Enter(&owners[0]);
        family=1;if(kind)script_fn_yoyo_create(&owners[1]);else script_fn_bat_create(&owners[1],false);
        check(constructor_calls[0]==1&&constructor_calls[1]==1,"single construction evaluation");
        check(get(kind?"yoyo_handle":"bat_handle")== (success?(int32_t)items[1][kind?Yoyo:Bat].x1C:-1),"constructor full identity publication");
        check((fighters[0].death2_cb==NULL)==(fighters[1].death2_cb==NULL)&&
              (fighters[0].death2_cb==sentinel)==(fighters[1].death2_cb==sentinel),"constructor death callback ownership");
        check((fighters[0].take_dmg_cb==NULL)==(fighters[1].take_dmg_cb==NULL)&&
              (fighters[0].take_dmg_cb==sentinel)==(fighters[1].take_dmg_cb==sentinel),"constructor damage callback ownership");
        if(kind){
            check((fighters[0].x1984_heldItemSpec!=NULL)==(fighters[1].x1984_heldItemSpec!=NULL),"YoYo auxiliary success/failure publication");
            check(fighters[0].pre_hitlag_cb&&fighters[1].pre_hitlag_cb&&fighters[0].post_hitlag_cb&&fighters[1].post_hitlag_cb,"YoYo failed construction retains hitlag callbacks");
        }
    }
    printf("Ness ordered lifecycle: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
