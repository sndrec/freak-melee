#include "behavior.generated.h"
#include <Runtime/platform.h>
#include <melee/ft/ftlib.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itnessbat.h>
#include <stdio.h>
#include <stdlib.h>

static Fighter fighter[2]; static Item item[2];
static HSD_GObj owner[2], object[2];
static unsigned family, checks, failures, event_count[2];
typedef struct BatEvent { int operation, value, commands, manual; } BatEvent;
static BatEvent events[2][4]; static SpawnItem descriptor;
static bool succeeds;
static void check(bool value,const char* label)
{ ++checks;if(!value&&failures++<12)fprintf(stderr,"%s\n",label); }
static void record_event(int operation,int value)
{ Item* ip=&item[family];events[family][event_count[family]++]=(BatEvent){operation,value,ip->xDAC_itcmd_var0 | (ip->xDB0_itcmd_var1<<1) | (ip->xDB4_itcmd_var2<<2) | (ip->xDB8_itcmd_var3<<3),ip->xDCC_flag.b3}; }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{ descriptor=*spawn;return succeeds?&object[0]:NULL; }
void ftLib_80086644(Fighter_GObj* gobj,Vec3* pos)
{ check(gobj==&owner[0],"position owner");*pos=fighter[0].cur_pos; }
float ftLib_800865C0(HSD_GObj* gobj)
{ check(gobj==&owner[0],"facing owner");return fighter[0].facing_dir; }
void it_80275158(Item_GObj* gobj,float lifetime)
{ check(gobj==&object[family],"lifetime item");item[family].xD44_lifeTimer=lifetime;record_event(1,(int)lifetime); }
void Item_8026AB54(HSD_GObj* gobj,HSD_GObj* parent,Fighter_Part part)
{ check(gobj==&object[family]&&parent==&owner[family],"attachment owner/item");record_event(2,part); }
void Item_80268E5C(HSD_GObj* gobj,enum_t state,Item_StateChangeFlags flags)
{ check(gobj==&object[family]&&flags==ITEM_ANIM_UPDATE,"pickup flags/item");item[family].msid=state;record_event(3,state); }
void Item_802694CC(HSD_GObj* gobj)
{ check(gobj==&object[family],"advance item");record_event(4,0); }
#define GET_ITEM(gobj) ((Item*)(gobj)->user_data)
#include "original.h"
static bool alive(HSD_GObj* gobj,uint32_t generation,uint32_t kind)
{ return kind==NativeCallbackSource_Item&&gobj==&object[1]&&gobj->user_data==&item[1]&&generation==item[1].x1C; }
static HSD_GObj* creator(Item_GObj* gobj)
{ return gobj==&object[1]&&item[1].xDD4_itemVar.native_article.creator_spawn_id==fighter[1].x8_spawnNum?&owner[1]:NULL; }
static unsigned bone_count(Fighter* fp){check(fp==&fighter[1],"attachment bone owner");return 68;}
static void manual(Item_GObj* gobj){check(gobj==&object[1],"manual item");item[1].xDCC_flag.b3=false;}
static float scale(Fighter_GObj* gobj){(void)gobj;return 1;}
static bool incapacitated(Fighter_GObj* gobj){(void)gobj;return false;}
static bool interact(Item_GObj* gobj,HSD_GObj* other){(void)gobj;(void)other;return false;}
static void joint(Item_GObj* gobj,unsigned index,const char* path,unsigned channel,float value){(void)gobj;(void)index;(void)path;(void)channel;(void)value;}
static void remove_item(Item_GObj* gobj){(void)gobj;}
static void initialize(void)
{
    memset(item,0,sizeof(item));memset(fighter,0,sizeof(fighter));memset(event_count,0,sizeof(event_count));
    for(unsigned f=0;f<2;++f){
        owner[f]=(HSD_GObj){.user_data=&fighter[f],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        object[f]=(HSD_GObj){.user_data=&item[f]};item[f].entity=&object[f];item[f].owner=&owner[f];
        item[f].x1C=0x80000010;fighter[f].x8_spawnNum=0x80000099;
        fighter[f].cur_pos=(Vec3){7,-3,9};fighter[f].facing_dir=-1;
        item[f].xDCC_flag.b3=true;item[f].xD44_lifeTimer=333;
        item[f].xDD4_itemVar.native_article.attachment_owner=&owner[f];
        item[f].xDD4_itemVar.native_article.creator_spawn_id=fighter[f].x8_spawnNum;
    }
}
int main(void)
{
    NativeFighterHost host={.callback_source_alive=alive,.item_set_lifetime=it_80275158,
        .fighter_root_scale_y=scale,.fighter_incapacitated=incapacitated,.item_interact=interact,
        .item_set_joint_value=joint,.item_remove=remove_item,.item_advance_animation=Item_802694CC,
        .item_creator=creator,.item_use_manual_lifetime=manual,.item_change_state_flags=Item_80268E5C,
        .item_attach_to_owner=Item_8026AB54,.bone_count=bone_count};
    MotionState motion={0};NativeFighterDefinition base={.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    const NativeFighterDefinition* def=FreakMelee_GetFighterModule(&base,&host);if(!def)return 2;
    script_parameters[0].i=42;script_parameters[1].f=1200;script_parameters[2].i=341;
    const NativeFighterArticleLogic* logic=&def->article_logics[0];
    for(unsigned mode=0;mode<2;++mode)for(unsigned success=0;success<2;++success){
        initialize();succeeds=success;family=0;
        Item_GObj* result=mode?it_802AD590(&owner[0],42):it_802AD478(&owner[0],&fighter[0].cur_pos,42,-1);
        check((result!=NULL)==success,"constructor failure propagation");
        check(descriptor.kind==It_Kind_Ness_Bat&&descriptor.x0_parent_gobj==&owner[0]&&descriptor.x4_parent_gobj2==&owner[0],"Bat source kind/parent pair");
        check(descriptor.pos.x==7&&descriptor.pos.y==-3&&descriptor.pos.z==0&&!memcmp(&descriptor.prev_pos,&descriptor.pos,sizeof(Vec3)),"Bat flattened current/previous position");
        check(descriptor.vel.x==0&&descriptor.vel.y==0&&descriptor.vel.z==0&&descriptor.facing_dir==-1&&!descriptor.x44_flag.b0&&descriptor.x40==0&&descriptor.x3C_damage==0,"Bat initial collision/velocity/group/damage");
        family=1;item[1].xDD4_itemVar.native_article.creation_value=mode;
        if(success)logic->on_created(&object[1]);
        check(item[0].xDAC_itcmd_var0==item[1].xDAC_itcmd_var0&&item[0].xDB0_itcmd_var1==item[1].xDB0_itcmd_var1&&item[0].xDB4_itcmd_var2==item[1].xDB4_itcmd_var2&&item[0].xDB8_itcmd_var3==item[1].xDB8_itcmd_var3,"four command modes");
        check(item[0].xDCC_flag.b3==item[1].xDCC_flag.b3&&item[0].xD44_lifeTimer==item[1].xD44_lifeTimer,"manual lifetime only normal mode");
        check(event_count[0]==event_count[1]&&!memcmp(events[0],events[1],event_count[0]*sizeof(BatEvent)),"ordered lifetime before attachment");
    }
    for(unsigned bits=0;bits<16;++bits){
        initialize();for(unsigned f=0;f<2;++f){item[f].xDAC_itcmd_var0=bits&1;item[f].xDB0_itcmd_var1=(bits>>1)&1;item[f].xDB4_itcmd_var2=(bits>>2)&1;item[f].xDB8_itcmd_var3=(bits>>3)&1;}
        family=0;it_2725_Logic58_PickedUp(&object[0]);family=1;logic->logic->picked_up(&object[1]);
        check(item[0].msid==item[1].msid&&event_count[0]==event_count[1]&&!memcmp(events[0],events[1],event_count[0]*sizeof(BatEvent)),"pickup mode selection then animation advance");
    }
    printf("Ness Bat construction/init: %u checks, %u failures\n",checks,failures);return failures?1:0;
}


