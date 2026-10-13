// Actual generated Chef/food callbacks versus unchanged frozen originals.
// Allocation, animation evaluator, world matrix, stage mask and paper draw are
// controlled boundaries. This does not exercise an installed fighter or pixels.
#include "behavior.generated.h"
#include "parameters.h"
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/item.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/it_2725.h>
#include <melee/it/itgroundcoll.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/random.h>
#include <stdio.h>
#include <stdlib.h>

static Fighter fighters[2]; static Item items[2];
static HSD_GObj owners[2], objects[2], *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static FighterBone parts[2][53]; static HSD_JObj models[2], anchor[2];
static Article data[2]; static itGamewatchchefAttributes food;
static ftGameWatchAttributes tuning;
static const NativeFighterDefinition* definition;
static NativeFighterDefinition base; static MotionState motions[2];
static unsigned family, checks, failures, pairs, fighter_pairs, food_pairs, edited_pairs;
static unsigned variants_seen;
static bool allocation_failure, animation_remaining, ground_result, air_result;
static int stage_flags, result[2]; static unsigned random_seed[2];
typedef struct ChefEvent { char operation; int a,b; float x,y,z,w; } ChefEvent;
static ChefEvent events[2][128]; static unsigned event_count[2];
static void check(bool value, const char* reason)
{ ++checks; if (!value && failures++ < 12) fprintf(stderr,"%s (family%u pair%u)\n",reason,family,pairs); }
static void event(char operation,int a,int b,float x,float y,float z,float w)
{ check(event_count[family]<128,"bounded events"); events[family][event_count[family]++]=(ChefEvent){operation,a,b,x,y,z,w}; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { j->flags |= JOBJ_MTX_DIRTY; }
void __assert(char* file,u32 line,char* expression) { fprintf(stderr,"HSD assertion %s:%d %s\n",file,line,expression);abort(); }
static int state_int(const char* name)
{
    for (unsigned i=0;i<definition->state_field_count;++i) if(!strcmp(name,definition->state_fields[i].name)) {
        int32_t value=0; memcpy(&value,fighters[1].native_module_state+script_state_base+definition->state_fields[i].offset,4); return value;
    }
    abort();
}
static void set_state(const char* name,int value)
{
    for(unsigned i=0;i<definition->state_field_count;++i) if(!strcmp(name,definition->state_fields[i].name)) {
        memcpy(fighters[1].native_module_state+script_state_base+definition->state_fields[i].offset,&value,definition->state_fields[i].type==NativeFighterState_Bool?1:4);return;
    }
    abort();
}
static float param_float(const char* name)
{ for(unsigned i=0;i<FIXTURE_PARAMETER_COUNT;++i) if(!strcmp(name,fixture_parameter_names[i])) return script_parameters[i].f;abort(); }
static void param(const char* name,float value)
{ for(unsigned i=0;i<FIXTURE_PARAMETER_COUNT;++i) if(!strcmp(name,fixture_parameter_names[i])) { script_parameters[i].f=value;return; } abort(); }
void Fighter_ChangeMotionState(Fighter_GObj* g, FtMotionId motion, MotionFlags flags,
    float frame,float rate,float blend,Fighter_GObj* other)
{ check(g==&owners[family] && blend==0 && other==NULL,"motion args"); fighters[family].motion_id=motion;fighters[family].cur_anim_frame=frame;event('M',motion,flags,frame,rate,0,0); }
void ftAnim_8006EBA4(Fighter_GObj* g) { (void)g;event('A',0,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g) { (void)g;event('Q',0,0,0,0,0,0);return animation_remaining; }
void ft_8008A2BC(Fighter_GObj* g) { (void)g;fighters[family].motion_id=ftCo_MS_Wait;event('W',0,0,0,0,0,0); }
void ftCo_Fall_Enter(Fighter_GObj* g) { (void)g;fighters[family].motion_id=ftCo_MS_Fall;event('F',0,0,0,0,0,0); }
void ft_80084F3C(Fighter_GObj* g) { (void)g;event('P',0,0,0,0,0,0); }
void ft_80084EEC(Fighter_GObj* g) { (void)g;event('P',1,0,0,0,0,0); }
bool ft_800827A0(Fighter_GObj* g) { (void)g;event('G',0,0,0,0,0,0);return ground_result; }
GroundOrAir ft_80081D0C(Fighter_GObj* g) { (void)g;event('L',0,0,0,0,0,0);return air_result; }
static GroundOrAir air_collision(Fighter_GObj* g) { return ft_80081D0C(g); }
void ftCommon_8007D5D4(Fighter* f) { f->ground_or_air=GA_Air;event('T',1,0,0,0,0,0); }
void ftCommon_8007D7FC(Fighter* f) { f->ground_or_air=GA_Ground;event('T',0,0,0,0,0,0); }
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part part) { (void)f;check(part==FtPart_LThumbNb,"semantic thumb");return 17; }
static unsigned bone_count(Fighter* f) { (void)f;return 53; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* output)
{ check(joint==&anchor[family] && offset!=NULL,"world joint+offset");event('B',0,0,offset->x,offset->y,offset->z,0); *output=(Vec3){20+offset->x,-7+offset->y,3+offset->z}; }
s32 HSD_Randi(s32 bound)
{ check(bound>0,"positive RNG bound");event('R',bound,0,0,0,0,0);random_seed[family]=random_seed[family]*1664525u+1013904223u;return (random_seed[family]>>8)%(unsigned)bound; }
#include "random_original.h"
void it_8026BB68(HSD_GObj* owner,Vec3* output) { check(owner==&owners[family],"constructor owner");*output=fighters[family].cur_pos; }
static Item_GObj* allocate(SpawnItem* spawn)
{
    event('C',spawn->kind,spawn->x40,spawn->prev_pos.x,spawn->prev_pos.y,spawn->prev_pos.z,spawn->facing_dir);
    event('c',spawn->x44_flag.b0,spawn->x0_parent_gobj==&owners[family]&&spawn->x4_parent_gobj2==&owners[family],spawn->pos.x,spawn->pos.y,spawn->pos.z,spawn->x3C_damage);
    check(spawn->vel.x==0&&spawn->vel.y==0&&spawn->vel.z==0,"zero creation velocity");
    if(allocation_failure)return NULL;
    items[family].owner=&owners[family];items[family].pos=spawn->pos;items[family].x40_vel=spawn->vel;items[family].facing_dir=spawn->facing_dir;return &objects[family];
}
Item_GObj* Item_80268B18(SpawnItem* spawn) { return allocate(spawn); }
static int32_t create(Fighter_GObj* owner,ItemKind kind,Vec3* pos,Vec3* vel,float facing,bool center,uint32_t group,float value,bool collision)
{
    SpawnItem spawn={0};spawn.kind=kind;spawn.prev_pos=spawn.pos=*pos;spawn.vel=*vel;spawn.facing_dir=facing;spawn.x0_parent_gobj=spawn.x4_parent_gobj2=owner;spawn.x40=group;spawn.x44_flag.b0=collision;
    if(center)it_8026BB68(owner,&spawn.pos);
    Item_GObj* object=allocate(&spawn);if(!object)return -1;
    check(value>=0 && value<5 && value==(int)value,"bounded exact variant");
    variants_seen|=1u<<(unsigned)value;
    items[1].xDD4_itemVar.native_article.creation_value=value;
    definition->article_logics[0].on_created(object);return items[1].x1C;
}
void it_80275158(Item_GObj* g,float life) { items[family].xD44_lifeTimer=life;event('l',0,0,life,0,0,0);(void)g; }
void Item_80268E5C(Item_GObj* g,enum_t state,Item_StateChangeFlags flags)
{ (void)g;items[family].msid=state;event('S',state,flags,0,0,0,0); }
void it_8026B3A8(Item_GObj* g) { (void)g;items[family].xDC8_word.flags.x15=0;event('m',0,0,0,0,0,0); }
void db_80225DD8(HSD_GObj* g,HSD_GObj* owner) { (void)g;check(owner==&owners[family],"debug owner");event('D',0,0,0,0,0,0); }
void it_80274594(Item_GObj* g) { (void)g;event('I',0,0,0,0,0,0); }
void it_8027CE64(Item_GObj* g,HSD_GObj* owner,itGamewatchModelVisibility* lists)
{ (void)owner;(void)lists;it_80274594(g); } // Paper rendering/colors explicitly excluded.
s32 it_8026DAA8(Item_GObj* g) { (void)g;event('K',stage_flags,0,0,0,0,0);return stage_flags; }
void it_8026BB20(Item_GObj* g) { (void)g;models[family].flags&=~JOBJ_HIDDEN;event('V',1,0,0,0,0,0); }
void it_8026BB44(Item_GObj* g) { (void)g;models[family].flags|=JOBJ_HIDDEN;event('V',0,0,0,0,0,0); }
static float joint_read(Item_GObj* g,unsigned model,const char* path,unsigned channel)
{ check(g==&objects[family]&&model==0&&!*path&&channel==3,"joint read");return models[family].rotate.x; }
static void joint_write(Item_GObj* g,unsigned model,const char* path,unsigned channel,float value)
{ check(g==&objects[family]&&model==0&&!*path,"joint write");if(channel==3)HSD_JObjSetRotationX(&models[family],value);else {check(channel==10,"recursive visibility channel");if(value)it_8026BB20(g);else it_8026BB44(g);} }
bool itColl_BounceOffShield(Item_GObj* g) { (void)g;event('h',0,0,0,0,0,0);return stage_flags&1; }
bool it_80273030(Item_GObj* g) { (void)g;items[family].facing_dir = -items[family].facing_dir;event('f',0,0,0,0,0,0); return false; }
bool it_8026B894(Item_GObj* g,HSD_GObj* other) { (void)g;(void)other;event('i',0,0,0,0,0,0); return false; }
static bool interact(Item_GObj* g,HSD_GObj* other) { it_8026B894(g,other);return false; }
#include "original.h"

static void reset(void)
{
    memset(fighters,0,sizeof(fighters));memset(items,0,sizeof(items));memset(owners,0,sizeof(owners));memset(objects,0,sizeof(objects));memset(models,0,sizeof(models));memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));
    for(unsigned i=0;i<2;++i) {
        owners[i].user_data=&fighters[i];objects[i].user_data=&items[i];objects[i].hsd_obj=&models[i];fighters[i].gobj=&owners[i];fighters[i].kind=Ft_Kind_CustomFirst;fighters[i].x8_spawnNum=(int32_t)0xFEDCBA98u;
        fighters[i].dat_attrs=&tuning;fighters[i].parts=parts[i];parts[i][17].joint=&anchor[i];fighters[i].cur_pos=(Vec3){4,5,6};fighters[i].facing_dir=1;
        items[i].xC4_article_data=&data[i];data[i].x4_specialAttributes=&food;items[i].x1C=0x80000001u;items[i].xD5C=29;items[i].xD44_lifeTimer=19;items[i].xDC8_word.flags.x15=1;items[i].owner=&owners[i];random_seed[i]=71;result[i]=0;
    }
    allocation_failure=false;animation_remaining=true;ground_result=true;air_result=false;stage_flags=0;
}
static void select(unsigned value) { family=value;heads[HSD_GOBJ_PLINK_FIGHTER]=&owners[value];heads[HSD_GOBJ_PLINK_ITEM]=&objects[value]; }
static void equal(void)
{
    ++pairs;check(result[0]==result[1],"return result");check(event_count[0]==event_count[1],"event count");
    for(unsigned i=0;i<event_count[0]&&i<event_count[1];++i) {
        ChefEvent* a=&events[0][i];ChefEvent* b=&events[1][i];check(a->operation==b->operation&&a->a==b->a&&a->b==b->b&&a->x==b->x&&a->y==b->y&&a->z==b->z&&a->w==b->w,"ordered event");
    }
    check(fighters[0].motion_id==fighters[1].motion_id&&fighters[0].ground_or_air==fighters[1].ground_or_air&&fighters[0].cur_anim_frame==fighters[1].cur_anim_frame,"fighter transition");
    check(!memcmp(fighters[0].cmd_vars,fighters[1].cmd_vars,sizeof(fighters[0].cmd_vars)),"commands");
    check(fighters[0].self_vel.y==fighters[1].self_vel.y,"vertical fighter velocity");
    check(fighters[0].mv.gw.SpecialN.maxSausage==state_int("chef_spawned"),"spawn count");
    check(fighters[0].mv.gw.SpecialN.isChefLoopDisable==state_int("chef_loop_disabled"),"release latch");
    check(fighters[0].u.gw.x2240_chefVar1==state_int("chef_latest")&&fighters[0].u.gw.x2244_chefVar2==state_int("chef_previous"),"history");
    check((fighters[0].accessory4_cb!=NULL)==(fighters[1].accessory4_cb!=NULL),"accessory binding");
    check(random_seed[0]==random_seed[1],"RNG consumption");
    int32_t variant;memcpy(&variant,items[1].xDD4_itemVar.native_article.state,4);
    check(items[0].xDD4_itemVar.gamewatchchef.variant==variant,"food variant storage");
    check(items[0].msid==items[1].msid&&items[0].xD44_lifeTimer==items[1].xD44_lifeTimer,"food state+life");
    check(!memcmp(&items[0].x40_vel,&items[1].x40_vel,sizeof(Vec3)),"food velocity+preservedZ");
    check(items[0].xD5C==items[1].xD5C&&items[0].xDC8_word.flags.x15==items[1].xDC8_word.flags.x15,"food setup flags");
    check(models[0].rotate.x==models[1].rotate.x&&models[0].flags==models[1].flags,"root rotation+visibility");
    check(models[0].rotate.y==models[1].rotate.y && items[0].facing_dir==items[1].facing_dir,"reflection rotation+facing");
}
static void initial(int count,bool disabled,int latest,int previous)
{
    fighters[0].mv.gw.SpecialN.maxSausage=count;fighters[0].mv.gw.SpecialN.isChefLoopDisable=disabled;fighters[0].u.gw.x2240_chefVar1=latest;fighters[0].u.gw.x2244_chefVar2=previous;
    set_state("chef_spawned",count);set_state("chef_loop_disabled",disabled);set_state("chef_latest",latest);set_state("chef_previous",previous);
    for(unsigned i=0;i<2;++i) { fighters[i].accessory4_cb=i?script_fn_ChefCreate:ftGw_SpecialN_CreateSausage;fighters[i].cur_anim_frame=9.5f; }
}
static void fighter_matrix(void)
{
    float limits[]={-1,0,0.5f,1,3,5,5.5f};
    for(unsigned air=0;air<2;++air) for(unsigned flags=0;flags<32;++flags) for(unsigned cap=0;cap<7;++cap) for(int count=0;count<=6;++count) {
        reset();initial(count,flags&1,1,3);tuning.x1C_GAMEWATCH_CHEF_MAX=limits[cap];param("chef_max_food",limits[cap]);animation_remaining=!(flags&2);
        for(unsigned i=0;i<2;++i) { fighters[i].cmd_vars[1]=flags&4;fighters[i].cmd_vars[2]=flags&8;fighters[i].input.held_buttons[0]=(flags&16)?HSD_PAD_B:0;fighters[i].input.pressed_buttons=(flags&8)?HSD_PAD_B:0; }
        select(0);if(air)ftGw_SpecialAirN_Anim(&owners[0]);else ftGw_SpecialN_Anim(&owners[0]);
        select(1);if(air)authored_air_animation_n(&owners[1]);else authored_ground_animation_n(&owners[1]);equal();++fighter_pairs;
        reset();initial(count,flags&1,1,3);for(unsigned i=0;i<2;++i) { fighters[i].cmd_vars[1]=flags&4;fighters[i].input.held_buttons[0]=(flags&16)?HSD_PAD_B:0;fighters[i].input.pressed_buttons=(flags&8)?HSD_PAD_B:0; }
        select(0);if(air)ftGw_SpecialAirN_IASA(&owners[0]);else ftGw_SpecialN_IASA(&owners[0]);
        select(1);if(air)authored_air_interrupt_n(&owners[1]);else authored_ground_interrupt_n(&owners[1]);equal();++fighter_pairs;
    }
    for(unsigned cmd=0;cmd<3;++cmd) for(int history=-1;history<6;++history) for(int previous=-1;previous<6;++previous) for(unsigned fail=0;fail<2;++fail) for(unsigned sign=0;sign<2;++sign) {
        reset();initial(0,false,history,previous);random_seed[0]=random_seed[1]=71u+(unsigned)(history+1)+7u*(unsigned)(previous+1)+cmd;allocation_failure=fail;tuning.x1C_GAMEWATCH_CHEF_MAX=5;param("chef_max_food",5);
        for(unsigned i=0;i<2;++i) {fighters[i].cmd_vars[0]=cmd;fighters[i].facing_dir=sign?-1:1;}
        select(0);ftGw_SpecialN_CreateSausage(&owners[0]);select(1);script_fn_ChefCreate(&owners[1]);equal();++fighter_pairs;
    }
    for(unsigned air=0;air<2;++air) for(unsigned kind=0;kind<3;++kind) for(unsigned outcome=0;outcome<2;++outcome) {
        reset();initial(4,true,2,3);ground_result=outcome;air_result=outcome;
        for(unsigned i=0;i<2;++i){fighters[i].cmd_vars[0]=4;fighters[i].cmd_vars[1]=5;fighters[i].cmd_vars[2]=6;fighters[i].self_vel.y=17;}
        select(0);if(!kind){if(air)ftGw_SpecialAirN_Enter(&owners[0]);else ftGw_SpecialN_Enter(&owners[0]);}else if(kind==1){if(air)ftGw_SpecialAirN_Coll(&owners[0]);else ftGw_SpecialN_Coll(&owners[0]);}else {if(air)ftGw_SpecialAirN_Phys(&owners[0]);else ftGw_SpecialN_Phys(&owners[0]);}
        select(1);if(!kind){if(air)authored_air_n(&owners[1]);else authored_ground_n(&owners[1]);}else if(kind==1){if(air)authored_air_collision_n(&owners[1]);else authored_ground_collision_n(&owners[1]);}else {if(air)authored_air_physics_n(&owners[1]);else authored_ground_physics_n(&owners[1]);}equal();++fighter_pairs;
    }
    // Exhausted/fractional cap must clear the cue without RNG/allocation; count
    // and history advance before a failed constructor only when under budget.
    for(unsigned cmd=0;cmd<3;++cmd) for(unsigned cap=0;cap<7;++cap) for(int count=0;count<=6;++count) for(unsigned fail=0;fail<2;++fail) {
        reset();initial(count,false,1,3);allocation_failure=fail;tuning.x1C_GAMEWATCH_CHEF_MAX=limits[cap];param("chef_max_food",limits[cap]);
        for(unsigned i=0;i<2;++i)fighters[i].cmd_vars[0]=cmd;
        select(0);ftGw_SpecialN_CreateSausage(&owners[0]);select(1);script_fn_ChefCreate(&owners[1]);equal();++fighter_pairs;
    }
}
static void food_matrix(void)
{
    float life[]={0,0.5f,1,1.5f,2,3,4,9,60};float speeds[]={-7,-3,-0.5f,0,0.5f,3,7};
    for(unsigned variant=0;variant<5;++variant) for(unsigned state=0;state<2;++state) for(unsigned phase=0;phase<3;++phase) for(unsigned l=0;l<9;++l) for(unsigned v=0;v<7;++v) for(unsigned mask=0;mask<16;++mask) {
        reset();initial(0,false,0,0);stage_flags=mask;
        for(unsigned i=0;i<2;++i){items[i].xD44_lifeTimer=life[l];items[i].x40_vel=(Vec3){speeds[v],speeds[(v+3)%7],6};items[i].msid=state;models[i].rotate.x=-0.25f;models[i].flags=JOBJ_HIDDEN;}
        items[0].xDD4_itemVar.gamewatchchef.variant=variant;memcpy(items[1].xDD4_itemVar.native_article.state,&variant,4);
        select(0);if(phase==0)result[0]=it_803F79E0[state].animated(&objects[0]);else if(phase==1)it_803F79E0[state].physics_updated(&objects[0]);else result[0]=it_803F79E0[state].collided(&objects[0]);
        select(1);ItemStateTable* states=definition->article_logics[0].logic->states;if(phase==0)result[1]=states[state].animated(&objects[1]);else if(phase==1)states[state].physics_updated(&objects[1]);else result[1]=states[state].collided(&objects[1]);equal();++food_pairs;
    }
    for(unsigned kind=0;kind<7;++kind) for(unsigned mask=0;mask<2;++mask) {
        reset();initial(0,false,0,0);stage_flags=mask;for(unsigned i=0;i<2;++i){items[i].x40_vel=(Vec3){8,-6,3};items[i].xD44_lifeTimer=51;}
        select(0);switch(kind){case 0:result[0]=itGameWatchChef_Logic112_DmgDealt(&objects[0]);break;case 1:result[0]=it_2725_Logic112_Clanked(&objects[0]);break;case 2:result[0]=it_2725_Logic112_HitShield(&objects[0]);break;case 3:result[0]=it_2725_Logic112_Absorbed(&objects[0]);break;case 4:result[0]=itGameWatchChef_Logic112_ShieldBounced(&objects[0]);break;case 5:result[0]=itGameWatchChef_Logic112_Reflected(&objects[0]);break;default:itGameWatchChef_Logic112_EvtUnk(&objects[0],&objects[0]);}
        select(1);ItemLogicTable* logic=definition->article_logics[0].logic;switch(kind){case 0:result[1]=logic->dmg_dealt(&objects[1]);break;case 1:result[1]=logic->clanked(&objects[1]);break;case 2:result[1]=logic->hit_shield(&objects[1]);break;case 3:result[1]=logic->absorbed(&objects[1]);break;case 4:result[1]=logic->shield_bounced(&objects[1]);break;case 5:result[1]=logic->reflected(&objects[1]);break;default:logic->evt_unk(&objects[1],&objects[1]);}equal();++food_pairs;
    }
}
static void edited_matrix(void)
{
    float gravities[]={-.5f,0,.125f,.5f};float terminals[]={-1,0,1,4};
    float speeds[]={-5,-4,-1,-0.0f,0,1,4,5};
    for(unsigned variant=0;variant<5;++variant) for(unsigned g=0;g<4;++g) for(unsigned t=0;t<4;++t) for(unsigned v=0;v<8;++v) {
        reset();initial(0,false,0,0);char name[48];
        food.entries[variant].gravity=gravities[g];food.entries[variant].terminal_velocity=terminals[t];
        snprintf(name,sizeof(name),"food_%u_gravity",variant);param(name,gravities[g]);
        snprintf(name,sizeof(name),"food_%u_terminal",variant);param(name,terminals[t]);
        for(unsigned i=0;i<2;++i)items[i].x40_vel=(Vec3){2,speeds[v],9};
        items[0].xDD4_itemVar.gamewatchchef.variant=variant;memcpy(items[1].xDD4_itemVar.native_article.state,&variant,4);
        select(0);itGamewatchchef_UnkMotion0_Phys(&objects[0]);
        select(1);definition->article_logics[0].logic->states[0].physics_updated(&objects[1]);equal();++edited_pairs;
    }
    for(unsigned air=0;air<2;++air) for(unsigned frame=0;frame<5;++frame) {
        reset();initial(1,false,1,3);tuning.x18_GAMEWATCH_CHEF_LOOPFRAME=(float)frame*.5f-1;param("chef_loop_frame",tuning.x18_GAMEWATCH_CHEF_LOOPFRAME);
        for(unsigned i=0;i<2;++i)fighters[i].cmd_vars[1]=fighters[i].input.pressed_buttons=HSD_PAD_B;
        tuning.x1C_GAMEWATCH_CHEF_MAX=7;param("chef_max_food",7);
        select(0);if(air)ftGw_SpecialAirN_IASA(&owners[0]);else ftGw_SpecialN_IASA(&owners[0]);
        select(1);if(air)authored_air_interrupt_n(&owners[1]);else authored_ground_interrupt_n(&owners[1]);equal();++edited_pairs;
    }
}
int main(void)
{
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.change_motion_state=Fighter_ChangeMotionState,.start_animation=ftAnim_8006EBA4,.animation_frames_remaining=ftAnim_IsFramesRemaining,.finish_ground_action=ft_8008A2BC,.enter_fall=ftCo_Fall_Enter,.ground_friction=ft_80084F3C,.air_gravity_and_friction=ft_80084EEC,.special_ground_collision=ft_800827A0,.air_collision=air_collision,.ground_to_air=ftCommon_8007D5D4,.air_to_ground=ftCommon_8007D7FC,.bone_count=bone_count,.joint_position=lb_8000B1CC,.random_int_excluding=random_int_excluding,.create_article_at=create,.item_set_lifetime=it_80275158,.item_reflect=itReflectItemAndUpdateRotation,.item_shield_bounce=itColl_BounceOffShield,.item_interact=interact,.item_joint_value=joint_read,.item_set_joint_value=joint_write,.item_sweep_stage_flags=it_8026DAA8,.abs_float=fabsf,.item_inherit_owner_scale=it_80274594,.item_use_manual_lifetime=it_8026B3A8,.item_change_state_flags=Item_80268E5C,.item_set_debug_owner=db_80225DD8};
    base.source_kind=Ft_Kind_GameWatch;base.joint_count=53;
    base.motion_state_first=353;base.motion_state_count=2;base.motion_states=motions;base.special_ground_state[0]=353;base.special_air_state[0]=354;
    definition=FreakMelee_GetFighterModule(&base,&host);check(definition!=NULL,"generated definition accepted");if(!definition)return 2;
    check(FreakMelee_SetFighterParameters(fixture_parameter_bits,FIXTURE_PARAMETER_COUNT,FIXTURE_PARAMETER_SCHEMA),"owned parameters bound");
    check((uint32_t)script_parameters[4].i==transition_flags,"original exact transition flags");
    tuning.x18_GAMEWATCH_CHEF_LOOPFRAME=16;
    food.wall_bounce_velocity_multiplier=.8f;food.active_lifetime=60;food.inactive_lifetime=16;
    for(unsigned i=0;i<5;++i) {
        const char* names[]={"horizontal","vertical","gravity","terminal","angular"};
        float values[5];char name[48];
        for(unsigned j=0;j<5;++j){snprintf(name,sizeof(name),"food_%u_%s",i,names[j]);values[j]=param_float(name);}
        food.entries[i]=(itGamewatchchefAttrEntry){values[0],values[1],values[2],values[3],values[4]};
    }
    fighter_matrix();food_matrix();edited_matrix();
    check(variants_seen==31,"all five RNG food variants reached");
    printf("Chef source: %u checks, %u failures, %u pairs; fighter%u food%u edited%u\n",checks,failures,pairs,fighter_pairs,food_pairs,edited_pairs);
    return failures?1:0;
}
