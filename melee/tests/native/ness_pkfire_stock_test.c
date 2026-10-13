#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/kinds/ftNess/ftnessspecials.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/fighter.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/it/kinds/itnesspkfire.h>
#include <melee/it/kinds/itnesspkfirepillar.h>
#include <melee/it/kinds/inlines.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* Bind)(const uint32_t*, unsigned, uint64_t);
enum { Motion=1, Animation, Joint, Origin, Spawn, Debug, State, Lifetime,
       Wait, Fall, GroundPhysics, AirPhysics, GroundCollision, AirCollision,
       Landing, StageHit, StageLand, Gravity, Reflect, ShieldBounce, Interact };
typedef struct NessEvent { int op,a,b; float x,y,z; } NessEvent;
typedef struct Trace { NessEvent events[64]; unsigned count; } Trace;
static Trace traces[2];
static unsigned lane, checks, failures;
static Fighter fighters[2];
static HSD_GObj owners[2], other_owners[2], objects[2][2];
static Item items[2][2];
static HSD_JObj joints[2][63], models[2][2];
static FighterBone bones[2][63];
static SpawnItem last_spawn[2];
static Article articles[2];
static ItemAttr common_attrs[2];
static itNessPKFirepillarAttributes fire_attrs, flame_attrs;
static ftNessAttributes fighter_attrs;
static const NativeFighterDefinition* module;
static const NativeFighterArticleLogic* fire_logic;
static const NativeFighterArticleLogic* flame_logic;
static bool allocation=true, remaining=true, contact=false;
#include "ness_raw_tuning.generated.h"

void __assert(char* file,u32 line,char* message)
{ fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,message);abort(); }
static void unused_host(void) { fputs("Unexpected host operation\n",stderr);abort(); }
static void record(int op,int a,int b,float x,float y,float z)
{
    Trace* trace=&traces[lane];
    if(trace->count==64)abort();
    trace->events[trace->count++]=(NessEvent){op,a,b,x,y,z};
}
static void check(bool valid,const char* name)
{ ++checks;if(!valid){++failures;fprintf(stderr,"%s differs\n",name);} }
static void check_float(float a,float b,const char* name)
{ check(memcmp(&a,&b,4)==0,name); }
static void compare_trace(void)
{
    check(traces[0].count==traces[1].count &&
          memcmp(traces[0].events,traces[1].events,traces[0].count*sizeof(NessEvent))==0,
          "ordered engine calls");
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags|=JOBJ_MTX_DIRTY; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{ (void)offset;*result=joint->translate;record(Joint,(int)(joint-joints[lane]),0,result->x,result->y,result->z); }
void it_8026BB68(HSD_GObj* owner,Vec3* result)
{ *result=((Fighter*)owner->user_data)->cur_pos;record(Origin,0,0,result->x,result->y,result->z); }
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,
                               float start,float speed,float blend,Fighter_GObj* donor)
{ (void)donor;((Fighter*)object->user_data)->motion_id=motion;record(Motion,motion,flags,start,speed,blend); }
void ftAnim_8006EBA4(Fighter_GObj* object)
{ (void)object;record(Animation,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* object) { (void)object;return remaining; }
void ft_8008A2BC(Fighter_GObj* object) { (void)object;record(Wait,0,0,0,0,0); }
void ftCo_Fall_Enter(Fighter_GObj* object) { (void)object;record(Fall,0,0,0,0,0); }
void ft_80084F3C(Fighter_GObj* object) { (void)object;record(GroundPhysics,0,0,0,0,0); }
void ft_80084EEC(Fighter_GObj* object) { (void)object;record(AirPhysics,0,0,0,0,0); }
bool ft_800827A0(Fighter_GObj* object)
{ (void)object;record(GroundCollision,0,0,0,0,0);return contact; }
GroundOrAir ft_80081D0C(Fighter_GObj* object)
{ (void)object;record(AirCollision,0,0,0,0,0);return contact?GA_Air:GA_Ground; }
void ftCo_LandingFallSpecial_Enter(Fighter_GObj* object,bool keep,float lag)
{ (void)object;record(Landing,keep,0,lag,0,0); }
HSD_GObj* Item_80268B18(SpawnItem* spawn)
{
    last_spawn[lane]=*spawn;
    record(Spawn,spawn->kind,spawn->x44_flag.b0,spawn->vel.x,spawn->vel.y,spawn->vel.z);
    if(!allocation)return NULL;
    unsigned index=spawn->kind==It_Kind_Ness_PKFire_Flame;
    Item* item=&items[lane][index];
    item->kind=spawn->kind;item->pos=spawn->prev_pos;item->x40_vel=spawn->vel;
    item->facing_dir=spawn->facing_dir;item->owner=spawn->x0_parent_gobj;
    return &objects[lane][index];
}
void db_80225DD8(HSD_GObj* object,HSD_GObj* owner)
{ (void)object;record(Debug,owner==&owners[lane],0,0,0,0); }
void Item_80268E5C(HSD_GObj* object,enum_t state,Item_StateChangeFlags flags)
{ ((Item*)object->user_data)->msid=state;record(State,state,flags,0,0,0); }
void it_80275158(HSD_GObj* object,float life)
{ Item* item=object->user_data;item->xD44_lifeTimer=life;item->xD48_halfLifeTimer=life*.5f;record(Lifetime,0,0,life,0,0); }
bool it_8026E058(HSD_GObj* object)
{ (void)object;record(StageHit,0,0,0,0,0);return contact; }
void it_8026E414(HSD_GObj* object,HSD_GObjEvent callback)
{ record(StageLand,0,0,0,0,0);if(contact){((Item*)object->user_data)->ground_or_air=GA_Ground;callback(object);} }
void it_80273454(HSD_GObj* object)
{ ((Item*)object->user_data)->x40_vel=(Vec3){0}; }
void it_8026D82C(HSD_GObj* object) { (void)object;unused_host(); }
void it_80272860(HSD_GObj* object,float gravity,float terminal)
{ (void)object;record(Gravity,0,0,gravity,terminal,0); }
bool it_80273030(HSD_GObj* object)
{ (void)object;record(Reflect,0,0,0,0,0);return contact; }
bool itColl_BounceOffShield(HSD_GObj* object)
{ (void)object;record(ShieldBounce,0,0,0,0,0);return contact; }
bool it_8026B894(HSD_GObj* object,HSD_GObj* other)
{ (void)object;record(Interact,other==&owners[lane],0,0,0,0);return false; }
static unsigned bone_count(Fighter* fighter) { (void)fighter;return 63; }
static Fighter_GObj* item_creator(Item_GObj* object)
{ if(object!=&objects[lane][0]&&object!=&objects[lane][1])abort();return &owners[lane]; }
static float joint_value(HSD_GObj* object,unsigned slot,const char* path,unsigned channel)
{ if(slot||*path||channel!=5)abort();return ((HSD_JObj*)object->hsd_obj)->rotate.z; }
static void set_joint_value(HSD_GObj* object,unsigned slot,const char* path,unsigned channel,float value)
{ if(slot||*path||channel!=5)abort();HSD_JObjSetRotationZ((HSD_JObj*)object->hsd_obj,value); }
static void set_scale(HSD_GObj* object,float value)
{ HSD_JObjSetScale((HSD_JObj*)object->hsd_obj,&(Vec3){value,value,value}); }
static int32_t create_article(HSD_GObj* owner,ItemKind kind,Vec3* position,Vec3* velocity,
                              float facing,bool origin,uint32_t group,float value,bool initial_collision)
{
    SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawn(&spawn,owner,position,facing);
    spawn.vel=*velocity;spawn.x40=group;spawn.x44_flag.b0=initial_collision;if(origin)it_8026BB68(owner,&spawn.pos);
    HSD_GObj* object=Item_80268B18(&spawn);if(!object)return -1;
    Item* item=object->user_data;item->xDD4_itemVar.native_article.creation_value=value;
    fire_logic->on_created(object);return item->x1C;
}
static int32_t create_child(HSD_GObj* parent,ItemKind kind,Vec3* position,float facing)
{
    Item* source=parent->user_data;
    SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawnOnPlane(&spawn,source->owner,position,facing);
    spawn.x4_parent_gobj2=parent;
    HSD_GObj* object=Item_80268B18(&spawn);if(!object)return -1;
    flame_logic->on_created(object);return ((Item*)object->user_data)->x1C;
}
static void compare_spawn(void)
{
    SpawnItem* a=&last_spawn[0];SpawnItem* b=&last_spawn[1];
    check(a->kind==b->kind,"spawn kind");
    check(memcmp(&a->pos,&b->pos,sizeof(Vec3))==0,"spawn current position");
    check(memcmp(&a->prev_pos,&b->prev_pos,sizeof(Vec3))==0,"spawn previous position");
    check(memcmp(&a->vel,&b->vel,sizeof(Vec3))==0,"spawn velocity before construction");
    check_float(a->facing_dir,b->facing_dir,"spawn facing");
    check(a->x40==b->x40 && a->x3C_damage==b->x3C_damage,"spawn group/damage");
    check(a->x44_flag.b0==b->x44_flag.b0,"spawn collision flag");
    check((a->x0_parent_gobj==&owners[0])==(b->x0_parent_gobj==&owners[1]) &&
          (a->x0_parent_gobj==&other_owners[0])==(b->x0_parent_gobj==&other_owners[1]) &&
          (a->x0_parent_gobj==NULL)==(b->x0_parent_gobj==NULL),"spawn damage owner");
    check((a->x4_parent_gobj2==&objects[0][0])==(b->x4_parent_gobj2==&objects[1][0]),"spawn secondary parent");
}
static void compare_item(unsigned index)
{
    Item* a=&items[0][index];Item* b=&items[1][index];
    check_float(a->xD44_lifeTimer,b->xD44_lifeTimer,"life");
    check_float(a->xD48_halfLifeTimer,b->xD48_halfLifeTimer,"half life");
    check(a->ground_or_air==b->ground_or_air,"ground/air");
    check(a->xDC8_word.flags.x1F==b->xDC8_word.flags.x1F,"landing flag preserved");
    check(memcmp(&a->x40_vel,&b->x40_vel,sizeof(Vec3))==0,"item velocity");
    check(memcmp(&models[0][index].scale,&models[1][index].scale,sizeof(Vec3))==0,"model scale");
    check_float(models[0][index].rotate.z,models[1][index].rotate.z,"model rotation");
}
static void reset(float facing)
{
    memset(traces,0,sizeof(traces));memset(fighters,0,sizeof(fighters));
    memset(items,0,sizeof(items));memset(models,0,sizeof(models));memset(last_spawn,0,sizeof(last_spawn));
    for(unsigned i=0;i<2;++i){
        owners[i].user_data=&fighters[i];fighters[i].dat_attrs=&fighter_attrs;
        fighters[i].parts=bones[i];fighters[i].facing_dir=facing;
        fighters[i].cur_pos=(Vec3){-13,8,4};
        for(unsigned j=0;j<63;++j){bones[i][j].joint=&joints[i][j];joints[i][j].translate=(Vec3){j*.25f,12+j*.5f,-4};}
        for(unsigned j=0;j<2;++j){objects[i][j].user_data=&items[i][j];objects[i][j].hsd_obj=&models[i][j];
            models[i][j].scale=(Vec3){1,1,1};items[i][j].owner=&owners[i];items[i][j].entity=&objects[i][j];
            items[i][j].xC4_article_data=&articles[j];items[i][j].xCC_item_attr=&common_attrs[j];items[i][j].x1C=j+1;
            items[i][j].x40_vel=(Vec3){.75f,-.3f,2};items[i][j].pos=(Vec3){7,11,-3};
            items[i][j].facing_dir=facing;items[i][j].xDC8_word.flags.x1F=1;}
    }
}
static char* read_file(const char* path,size_t* size)
{ FILE* file=fopen(path,"rb");if(!file)abort();fseek(file,0,SEEK_END);*size=ftell(file);rewind(file);char* text=malloc(*size+1);if(!text)abort();if(fread(text,1,*size,file)!=*size)abort();text[*size]=0;fclose(file);return text; }
int main(int argc,char** argv)
{
    if(argc!=3)return 2;
    size_t size;char* text=read_file(argv[2],&size);NativeFighterParameters parameters;unsigned line;
    if(!nativeFighterParameters_Parse(text,size,&parameters,&line))return 2;free(text);
    bool spawn_joint_found=false;
    for(unsigned i=0;i<parameters.count;++i)if(strcmp(parameters.fields[i].name,"fire_spawn_joint")==0){
        check(parameters.fields[i].bits==(uint32_t)FtPart_R2ndNa,"authored direct joint ordinal");spawn_joint_found=true;
    }
    check(spawn_joint_found,"authored direct joint parameter exists");
    HMODULE library=LoadLibraryA(argv[1]);if(!library)return 2;
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");
    Bind bind=(Bind)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)bits[i]=parameters.fields[i].bits;
    if(!entry||!bind||!bind(bits,parameters.count,parameters.schema_hash))return 2;
    MotionState motions[36]={0};NativeFighterDefinition base={.code_key="ness_clone",.source_kind=Ft_Kind_Ness,.animation_source_kind=Ft_Kind_Ness,.fighter_root="ftDataNess",.costume_joint="PlyNess5K_Share_joint",.costume_matanim="PlyNess5K_Share_matanim_joint",.action_count=326,.demo_count=14,.joint_count=63,.motion_state_first=341,.motion_state_count=36,.motion_states=motions};
    base.special_ground_state[NativeFighterSpecial_Side]=356;base.special_air_state[NativeFighterSpecial_Side]=357;
    base.special_ground_state[NativeFighterSpecial_Down]=367;base.special_air_state[NativeFighterSpecial_Down]=372;
    NativeFighterHost host={0};
#include "ness_required_host.generated.h"
    host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;
    host.joint_position=lb_8000B1CC;host.bone_count=bone_count;host.create_article_at=create_article;
    host.item_creator=item_creator;
    host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.finish_ground_action=ft_8008A2BC;
    host.enter_fall=ftCo_Fall_Enter;host.ground_friction=ft_80084F3C;host.air_gravity_and_friction=ft_80084EEC;
    host.special_ground_collision=ft_800827A0;host.air_collision=ft_80081D0C;host.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter;
    host.cos_float=cosf;host.sin_float=sinf;host.item_change_state_flags=Item_80268E5C;
    host.item_set_lifetime=it_80275158;host.item_set_debug_owner=db_80225DD8;
    host.item_joint_value=joint_value;host.item_set_joint_value=set_joint_value;host.item_set_uniform_scale=set_scale;
    host.item_fall=it_80272860;host.item_reflect=it_80273030;host.item_shield_bounce=itColl_BounceOffShield;
    host.item_interact=it_8026B894;
    host.create_item_at_position=create_child;host.item_hits_stage=it_8026E058;
    host.item_land_on_stage=it_8026E414;
    module=entry(&base,&host);if(!module)return 2;
    for(unsigned i=0;i<module->article_logic_count;++i){const NativeFighterArticleLogic* logic=&module->article_logics[i];if(logic->kind==It_Kind_Ness_PKFire)fire_logic=logic;if(logic->kind==It_Kind_Ness_PKFire_Flame)flame_logic=logic;}
    if(!fire_logic||!flame_logic)return 2;
    memcpy(&fighter_attrs.x20_PKFIRE_AERIAL_LAUNCH_TRAJECTORY,raw_tuning,7*sizeof(float));
    fire_attrs=(itNessPKFirepillarAttributes){raw_tuning[7],raw_tuning[8],0};
    flame_attrs=(itNessPKFirepillarAttributes){raw_tuning[9],raw_tuning[10],raw_tuning[11]};
    articles[0].x4_specialAttributes=&fire_attrs;articles[1].x4_specialAttributes=&flame_attrs;
    common_attrs[0].x60_scale=1;common_attrs[1].x60_scale=1.25f;
    common_attrs[1].x10_fall_speed=.08f;common_attrs[1].x14_fall_speed_max=1.9f;
    for(unsigned air=0;air<2;++air)for(unsigned face=0;face<2;++face)for(unsigned success=0;success<2;++success){
        allocation=success;reset(face?-1:1);
        for(unsigned i=0;i<2;++i){fighters[i].ground_or_air=air?GA_Air:GA_Ground;fighters[i].throw_flags=255;fighters[i].cmd_vars[0]=77;}
        lane=0;if(air)ftNs_SpecialAirS_Enter(&owners[0]);else ftNs_SpecialS_Enter(&owners[0]);
        lane=1;if(air)module->special_air_s(&owners[1]);else module->special_s(&owners[1]);compare_trace();
        check(fighters[0].throw_flags==fighters[1].throw_flags && fighters[1].throw_flags==0,"entry cue reset");
        check(fighters[0].cmd_vars[0]==fighters[1].cmd_vars[0] && fighters[1].cmd_vars[0]==0,"entry command reset");
        for(unsigned i=0;i<2;++i)fighters[i].throw_flags_b0=1;memset(traces,0,sizeof(traces));
        lane=0;fighters[0].accessory4_cb(&owners[0]);lane=1;fighters[1].accessory4_cb(&owners[1]);
        compare_trace();compare_spawn();if(success)compare_item(0);
        check(!fighters[0].throw_flags_b0&&!fighters[1].throw_flags_b0,"spawn cue consumed");
        memset(traces,0,sizeof(traces));lane=0;fighters[0].accessory4_cb(&owners[0]);lane=1;fighters[1].accessory4_cb(&owners[1]);compare_trace();check(!traces[1].count,"no repeated cue launch");
    }
    for(unsigned end=0;end<2;++end)for(unsigned collision=0;collision<2;++collision){
        remaining=!end;contact=collision;reset(1);
        lane=0;ftNs_SpecialS_Anim(&owners[0]);ftNs_SpecialAirS_Anim(&owners[0]);ftNs_SpecialS_Phys(&owners[0]);ftNs_SpecialAirS_Phys(&owners[0]);ftNs_SpecialS_Coll(&owners[0]);ftNs_SpecialAirS_Coll(&owners[0]);
        lane=1;const MotionState* ground=&module->motion_states[15];const MotionState* air=&module->motion_states[16];ground->anim_cb(&owners[1]);air->anim_cb(&owners[1]);ground->phys_cb(&owners[1]);air->phys_cb(&owners[1]);ground->coll_cb(&owners[1]);air->coll_cb(&owners[1]);compare_trace();
    }
    for(unsigned kind=0;kind<2;++kind)for(int life=-2;life<=204;++life){
        reset(1);for(unsigned i=0;i<2;++i)items[i][kind].xD44_lifeTimer=life*.5f;
        lane=0;bool a=kind?itNesspkfirepillar_UnkMotion0_Anim(&objects[0][1]):itNesspkfire_UnkMotion0_Anim(&objects[0][0]);
        lane=1;bool b=(kind?flame_logic:fire_logic)->logic->states[0].animated(&objects[1][kind]);check(a==b,"animation removal decision");compare_item(kind);
    }
    for(unsigned ground=0;ground<2;++ground)for(unsigned flag=0;flag<2;++flag)for(unsigned collision=0;collision<2;++collision){
        reset(-1);contact=collision;for(unsigned i=0;i<2;++i){items[i][1].ground_or_air=ground?GA_Ground:GA_Air;items[i][1].xDC8_word.flags.x1F=flag;}
        lane=0;itNesspkfirepillar_UnkMotion0_Phys(&objects[0][1]);bool a=itNesspkfirepillar_UnkMotion0_Coll(&objects[0][1]);
        lane=1;flame_logic->logic->states[0].physics_updated(&objects[1][1]);bool b=flame_logic->logic->states[0].collided(&objects[1][1]);check(a==b,"flame collision decision");compare_trace();compare_item(1);
    }
    for(int damage=0;damage<=20;++damage)for(int life=-1;life<=100;life+=7){
        reset(1);for(unsigned i=0;i<2;++i){items[i][1].xC9C=damage;items[i][1].xCA0=999;items[i][1].xD44_lifeTimer=life+.25f;}
        lane=0;bool a=itNesspkfirepillar_Logic24_DmgReceived(&objects[0][1]);lane=1;bool b=flame_logic->logic->dmg_received(&objects[1][1]);check(a==b,"damage decision");compare_item(1);
    }
    allocation=true;
    for(unsigned clank=0;clank<2;++clank)for(unsigned face=0;face<2;++face)for(unsigned success=0;success<2;++success)for(unsigned owner=0;owner<3;++owner){
        allocation=success;reset(face?-1:1);for(unsigned i=0;i<2;++i)items[i][0].owner=owner==0?&owners[i]:owner==1?&other_owners[i]:NULL;
        lane=0;bool a=clank?it_2725_Logic23_Clanked(&objects[0][0]):it_2725_Logic23_DmgDealt(&objects[0][0]);
        lane=1;bool b=clank?fire_logic->logic->clanked(&objects[1][0]):fire_logic->logic->dmg_dealt(&objects[1][0]);check(a==b,"flame spawn removal decision");compare_trace();compare_spawn();if(success)compare_item(1);
    }
    for(unsigned response=0;response<2;++response){
        contact=response;reset(-1);for(unsigned i=0;i<2;++i)models[i][0].rotate.z=.375f;
        lane=0;bool a=itNesspkfire_UnkMotion0_Coll(&objects[0][0]);lane=1;bool b=fire_logic->logic->states[0].collided(&objects[1][0]);check(a==b,"projectile stage removal decision");compare_trace();
        memset(traces,0,sizeof(traces));lane=0;a=it_2725_Logic23_ShieldBounced(&objects[0][0]);lane=1;b=fire_logic->logic->shield_bounced(&objects[1][0]);check(a==b,"shield bounce decision");compare_trace();compare_item(0);
        memset(traces,0,sizeof(traces));lane=0;a=itNessPKFire_Logic23_Reflected(&objects[0][0]);lane=1;b=fire_logic->logic->reflected(&objects[1][0]);check(a==b,"reflect decision");compare_trace();
        lane=0;a=itNessPKFire_Logic23_Absorbed(&objects[0][0]);lane=1;b=fire_logic->logic->absorbed(&objects[1][0]);check(a==b,"absorb decision");
        lane=0;a=itNessPKFire_Logic23_HitShield(&objects[0][0]);lane=1;b=fire_logic->logic->hit_shield(&objects[1][0]);check(a==b,"shield removal decision");
        memset(traces,0,sizeof(traces));lane=0;it_802AA474(&objects[0][0],&owners[0]);itNesspkfirepillar_Logic24_EvtUnk(&objects[0][1],&owners[0]);lane=1;fire_logic->logic->evt_unk(&objects[1][0],&owners[1]);flame_logic->logic->evt_unk(&objects[1][1],&owners[1]);compare_trace();
    }
    printf("Ness PK Fire stock comparison: %u checks, %u failures\n",checks,failures);
    FreeLibrary(library);return failures?1:0;
}
