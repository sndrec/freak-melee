// Actual baseline stock bodies are linked by ness_flash_stock_compare.py.
// Engine boundaries are explicit fixture mocks; no simulation/game is launched.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef near
#undef far
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/kinds/ftNess/ftnessspecialn.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/it/inlines.h>
#include <melee/it/item.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/itgroundcoll.h>
#include <melee/it/kinds/itnesspkflash.h>
#include <melee/it/kinds/itnesspkflashexplode.h>
#include <melee/it/kinds/inlines.h>
#include <math.h>
#include <melee/lb/lb_00B0.h>
#include <melee/cm/camera.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
typedef int (__cdecl* Bind)(const uint32_t*,unsigned,uint64_t);
enum { Motion=1,Start,GroundPhysics,FallPhysics,Deaccel,GroundTest,AirTest,
       ToAir,ToGround,Rotate,Wait,Fall,Freefall,Create,ItemMotion,Life,Manual,
       ChildVisible,Debug,Disable,Advance,Damage,Stage,StopSound,Sound,Quake,Rumble };
typedef struct FlashEvent { int op,a,b; float x,y,z,w; } FlashEvent;
static FlashEvent events[2][96]; static unsigned counts[2],lane,checks,failures;
static Fighter fighters[2]; static HSD_GObj owners[2],others[2],objects[2][2];
static FighterBone bones[2][63]; static HSD_JObj joints[2][63],models[2][2],children[2][2];
static Item items[2][2]; static Article articles[2]; static ItemAttr common_attrs[2];
static ftNessAttributes attrs; static itFlashAttributes flash_attrs; static itFlashExplAttributes explosion_attrs;
static SpawnItem spawns[2]; static bool remaining,contact,allocation=true;
static const NativeFighterDefinition* module;
static const NativeFighterArticleLogic *flash_logic,*explosion_logic;
#include "ness_flash_raw.generated.h"

void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags|=JOBJ_MTX_DIRTY; }
static void unexpected(void) { fputs("Unexpected fixture boundary\n",stderr); abort(); }
void __assert(char* file,u32 line,char* message) { fprintf(stderr,"%s:%lu %s\n",file,line,message); abort(); }
static void record_event(int op,int a,int b,float x,float y,float z,float w)
{ if(counts[lane]==96)abort();events[lane][counts[lane]++]=(FlashEvent){op,a,b,x,y,z,w}; }
static void check(bool valid,const char* message)
{ ++checks;if(!valid){if(failures<20)fprintf(stderr,"%s differs (check %u)\n",message,checks);++failures;} }
static void eqf(float a,float b,const char* message) { check(memcmp(&a,&b,4)==0,message); }
static const NativeFighterStateField* field(const NativeFighterStateField* fields,unsigned count,const char* name)
{ for(unsigned i=0;i<count;++i)if(!strcmp(fields[i].name,name))return &fields[i];unexpected();return NULL; }
static void set_f(const char* name,int32_t value)
{ const NativeFighterStateField* f=field(module->state_fields,module->state_field_count,name);memcpy(fighters[1].native_module_state+f->offset,&value,4); }
static int32_t get_f(const char* name)
{ const NativeFighterStateField* f=field(module->state_fields,module->state_field_count,name);int32_t value;memcpy(&value,fighters[1].native_module_state+f->offset,4);return value; }
static void set_i(unsigned index,const char* name,uint32_t value)
{ const NativeFighterArticleLogic* l=index?explosion_logic:flash_logic;const NativeFighterStateField* f=field(l->state_fields,l->state_field_count,name);memcpy(items[1][index].xDD4_itemVar.native_article.state+f->offset,&value,f->stride); }
static uint32_t get_i(unsigned index,const char* name)
{ const NativeFighterArticleLogic* l=index?explosion_logic:flash_logic;const NativeFighterStateField* f=field(l->state_fields,l->state_field_count,name);uint32_t value=0;memcpy(&value,items[1][index].xDD4_itemVar.native_article.state+f->offset,f->stride);return value; }
static uint32_t bits(float v) { uint32_t b;memcpy(&b,&v,4);return b; }
static void compare_trace(void)
{ check(counts[0]==counts[1],"FlashEvent count");if(counts[0]==counts[1])for(unsigned i=0;i<counts[0];++i)check(!memcmp(&events[0][i],&events[1][i],sizeof(FlashEvent)),"ordered engine FlashEvent"); }
static void compare_spawn(void)
{
    SpawnItem* a=&spawns[0];SpawnItem* b=&spawns[1];
    check(a->kind==b->kind,"spawn kind");
    check(!memcmp(&a->pos,&b->pos,sizeof(Vec3)),"spawn origin");
    check(!memcmp(&a->prev_pos,&b->prev_pos,sizeof(Vec3)),"spawn initial position");
    check(!memcmp(&a->vel,&b->vel,sizeof(Vec3)),"spawn initial velocity");
    eqf(a->facing_dir,b->facing_dir,"spawn facing");eqf(a->x3C_damage,b->x3C_damage,"spawn damage");
    check(a->x40==b->x40&&a->x44_flag.b0==b->x44_flag.b0,"spawn group/flags");
    check((a->x0_parent_gobj==&owners[0])==(b->x0_parent_gobj==&owners[1]),"spawn damage parent");
    check((a->x4_parent_gobj2==&objects[0][0])==(b->x4_parent_gobj2==&objects[1][0]),"spawn secondary item parent");
    check((a->x4_parent_gobj2==&owners[0])==(b->x4_parent_gobj2==&owners[1]),"spawn secondary fighter parent");
}
static void compare_fighter(void)
{
    compare_trace(); Fighter* a=&fighters[0];Fighter* b=&fighters[1];
    check(a->motion_id==b->motion_id,"fighter motion");eqf(a->cur_anim_frame,b->cur_anim_frame,"fighter animation frame");
    check(a->ground_or_air==b->ground_or_air,"fighter ground/air");check(!memcmp(&a->self_vel,&b->self_vel,sizeof(Vec3)),"fighter velocity");
    check(!memcmp(a->cmd_vars,b->cmd_vars,sizeof(a->cmd_vars)),"fighter commands");check(a->x1968_jumpsUsed==b->x1968_jumpsUsed,"jump count");
    check((a->u.ns.pkflash_gobj!=NULL)==(get_f("flash_handle")!=-1),"flash handle presence");
    check((a->death2_cb!=NULL)==(b->death2_cb!=NULL)&&(a->take_dmg_cb!=NULL)==(b->take_dmg_cb!=NULL),"dynamic cleanup bindings");
    check(a->mv.ns.specialn.frames_to_loop_charge_ground==get_f("flash_ground_loop_delay"),"ground loop timer");
    check(a->mv.ns.specialn.frames_to_loop_charge_air==get_f("flash_air_loop_delay"),"air loop timer");
    check(a->mv.ns.specialn.falling_acceleration_delay==get_f("flash_gravity_delay"),"gravity timer");
    check(a->mv.ns.specialn.charge_release_delay==get_f("flash_release_delay"),"release timer");
}
static void compare_item(unsigned index)
{
    Item* a=&items[0][index];Item* b=&items[1][index];
    check(a->msid==b->msid,"item motion");eqf(a->xD44_lifeTimer,b->xD44_lifeTimer,"item lifetime");eqf(a->xD48_halfLifeTimer,b->xD48_halfLifeTimer,"item half lifetime");
    check(!memcmp(&a->x40_vel,&b->x40_vel,sizeof(Vec3)),"item velocity");check(!memcmp(&models[0][index].scale,&models[1][index].scale,sizeof(Vec3)),"item root model scale");
    check(!memcmp(&models[0][index].rotate,&models[1][index].rotate,sizeof(Vec3)),"item root rotation");
    check(a->xDC8_word.flags.x13==b->xDC8_word.flags.x13,"item owner-bound flag");
    check(a->xDAC_itcmd_var0==b->xDAC_itcmd_var0&&a->xDB4_itcmd_var2==b->xDB4_itcmd_var2,"item commands");
    if(index){check(bits(a->xDD4_itemVar.pkflushexplode.xDD4)==get_i(1,"charge"),"explosion charge");
        check(bits(a->xDD4_itemVar.pkflushexplode.xDD8)==get_i(1,"initial_radius"),"initial radius");
        eqf(a->x5D4_hitboxes[0].hit.scale,b->x5D4_hitboxes[0].hit.scale,"explosion hitbox radius");}
    else{check(bits(a->xDD4_itemVar.pkflush.xDD8_PKFlash)==get_i(0,"charge"),"flash charge");
        check((a->xDD4_itemVar.pkflush.xDDC_PKFlash!=0)==(get_i(0,"reflected")!=0),"reflection state");
        check((a->xDD4_itemVar.pkflush.xDE0_PKFlash_Owner!=NULL)==(get_i(0,"controller_enabled")!=0),"controller retention");}
}
void Fighter_ChangeMotionState(HSD_GObj* object,enum_t motion,MotionFlags flags,float start,float rate,float blend,HSD_GObj* donor)
{ (void)donor;Fighter* f=object->user_data;f->motion_id=motion;f->cur_anim_frame=start;record_event(Motion,motion,flags,start,rate,blend,0); }
void ftAnim_8006EBA4(HSD_GObj* object) { (void)object;record_event(Start,0,0,0,0,0,0); }
bool ftAnim_IsFramesRemaining(HSD_GObj* object) { (void)object;return remaining; }
void ft_80084F3C(HSD_GObj* object) { (void)object;record_event(GroundPhysics,0,0,0,0,0,0); }
void ftCommon_Fall(Fighter* f,float gravity,float terminal)
{ f->self_vel.y-=gravity;if(f->self_vel.y < -terminal)f->self_vel.y=-terminal;record_event(FallPhysics,0,0,gravity,terminal,0,0); }
void ftCommon_CalcSelfAccel_Deaccel(Fighter* f,float friction) { (void)f;record_event(Deaccel,0,0,friction,0,0,0); }
bool ft_80082708(HSD_GObj* object) { (void)object;record_event(GroundTest,0,0,0,0,0,0);return contact; }
GroundOrAir ft_80081D0C(HSD_GObj* object) { (void)object;record_event(AirTest,0,0,0,0,0,0);return contact; }
void ftCommon_8007D5D4(Fighter* f) { f->ground_or_air=GA_Air;record_event(ToAir,0,0,0,0,0,0); }
void ftCommon_8007D7FC(Fighter* f) { f->ground_or_air=GA_Ground;record_event(ToGround,0,0,0,0,0,0); }
void ftPartSetRotX(Fighter* f,int joint,float angle) { f->parts[joint].joint->rotate.x=angle;record_event(Rotate,joint,0,angle,0,0,0); }
void ft_8008A2BC(HSD_GObj* object) { (void)object;record_event(Wait,0,0,0,0,0,0); }
void ftCo_Fall_Enter(HSD_GObj* object) { (void)object;record_event(Fall,0,0,0,0,0,0); }
void ftCo_80096900(HSD_GObj* object,int a,int b,bool interrupt,float mobility,float lag)
{ (void)object;record_event(Freefall,a,b,mobility,lag,interrupt,0); }
void lb_8000B1CC(HSD_JObj* joint,Vec3* relative,Vec3* out) { if(relative)unexpected();*out=joint->translate; }
void it_8026BB68(HSD_GObj* owner,Vec3* out) { *out=((Fighter*)owner->user_data)->cur_pos; }
void ftLib_800865D8(HSD_GObj* owner,float* x,float* y) { Fighter* f=owner->user_data;*x=f->input.lstick[0].x;*y=f->input.lstick[0].y; }
void Item_80268E5C(HSD_GObj* object,enum_t motion,Item_StateChangeFlags flags)
{ ((Item*)object->user_data)->msid=motion;record_event(ItemMotion,motion,flags,0,0,0,0); }
void it_80275158(HSD_GObj* object,float life)
{ Item* item=object->user_data;item->xD44_lifeTimer=life;item->xD48_halfLifeTimer=life*.5f;record_event(Life,0,0,life,0,0,0); }
void it_8026B3A8(HSD_GObj* object) { (void)object;record_event(Manual,0,0,0,0,0,0); }
void it_80272940(HSD_GObj* object) { (void)object;record_event(ChildVisible,1,0,0,0,0,0); }
void db_80225DD8(HSD_GObj* object,HSD_GObj* owner) { (void)object;record_event(Debug,owner==&owners[lane],0,0,0,0,0); }
void it_802725D4(HSD_GObj* object) { (void)object;record_event(Disable,0,0,0,0,0,0); }
bool it_80273130(HSD_GObj* object) { return Item_TickLifetime(object->user_data); }
bool it_80272C6C(Item_GObj* object) { (void)object;return remaining; }
bool lb_8000B09C(HSD_JObj* joint) { (void)joint;return remaining; }
void Item_802694CC(HSD_GObj* object) { (void)object;record_event(Advance,0,0,0,0,0,0); }
void it_80272460(HitCapsule* hit,u32 damage,HSD_GObj* object) { (void)hit;(void)object;record_event(Damage,damage,0,0,0,0,0); }
bool it_8026DA08(HSD_GObj* object) { (void)object;record_event(Stage,0,0,0,0,0,0);return contact; }
void Item_8026B074(Item* item) { (void)item;record_event(StopSound,0,0,0,0,0,0); }
void Item_8026AE84(Item* item,enum_t sound,u8 pan,u8 volume) { (void)item;record_event(Sound,sound,pan,volume,0,0,0); }
void Camera_RequestQuake(CmQuakeKind kind,Vec3* position) { record_event(Quake,kind,0,position->x,position->y,position->z,0); }
void it_80273598(HSD_GObj* object,s32 id,s32 frames) { (void)object;record_event(Rumble,id,frames,0,0,0,0); }
bool it_8026B894(HSD_GObj* object,HSD_GObj* other) { (void)object;(void)other;return false; }
bool ftKb_SpecialNNs_800FEC78(HSD_GObj* object) { (void)object;unexpected();return false; }
void ftKb_SpecialNNs_800FECBC(HSD_GObj* object) { (void)object;unexpected(); }
void ftNs_Init_OnDamage(HSD_GObj* object)
{ // Flash-only retained resources: compare the ordered Flash + unconditional root reset.
    Fighter* f=object->user_data;
    if(f->u.ns.yoyo_gobj||f->u.ns.pkthunder_gobj||f->u.ns.bat_gobj)unexpected();
    ftNs_SpecialN_ItemPKFlushSetNULL(object);ftPartSetRotX(f,0,0);
}
static void play_sound(Item_GObj* object,s32 sound,u8 pan,u8 volume) { Item_8026AE84(object->user_data,sound,pan,volume); }
static void child_visible(HSD_GObj* object,bool value) { if(!value)unexpected();it_80272940(object); }
static void scale(HSD_GObj* object,float value) { HSD_JObjSetScale(object->hsd_obj,&(Vec3){value,value,value}); }
static float joint_value(HSD_GObj* object,unsigned slot,const char* path,unsigned channel)
{ if(slot||*path||channel!=4)unexpected();return ((HSD_JObj*)object->hsd_obj)->rotate.y; }
static void joint_write(HSD_GObj* object,unsigned slot,const char* path,unsigned channel,float value)
{ if(slot||*path||channel!=4)unexpected();HSD_JObjSetRotationY(object->hsd_obj,value); }
static Fighter_GObj* creator(HSD_GObj* object)
{ Item* i=object->user_data;return i->xDD4_itemVar.native_article.attachment_owner==&owners[1]&&i->xDD4_itemVar.native_article.creator_spawn_id==fighters[1].x8_spawnNum?&owners[1]:NULL; }
static uint32_t current_owner(HSD_GObj* object)
{ Item* i=object->user_data;return i->owner==&owners[lane]?fighters[lane].x8_spawnNum:0; }
static bool alive(HSD_GObj* object,uint32_t generation,uint32_t cls)
{
    if(cls==NativeCallbackSource_Fighter)return object==&owners[1]&&generation==fighters[1].x8_spawnNum;
    if(cls==NativeCallbackSource_Item)for(unsigned j=0;j<2;++j)if(object==&objects[1][j])return generation==items[1][j].x1C;
    return false;
}
static unsigned bone_count(Fighter* f) { (void)f;return 63; }
static bool owned(HSD_GObj* owner,s32 id)
{ return owner==&owners[1]&&id!=-1&&items[1][0].x1C==(uint32_t)id&&items[1][0].owner==owner; }
static bool ref_bool(HSD_GObj* caller,uint32_t id,ItemKind kind,unsigned f,int32_t index)
{ if(caller!=&owners[1]||id!=items[1][0].x1C||kind!=It_Kind_Ness_PKFlush||index||f!=3)unexpected();return get_i(0,"stage_stopped")!=0; }
static bool signal_article(HSD_GObj* caller,uint32_t id,uint32_t name,float value)
{ (void)caller;if(id!=items[1][0].x1C)return false;for(unsigned i=0;i<flash_logic->signal_count;++i)if(flash_logic->signals[i].name_hash==name){flash_logic->signals[i].callback(&objects[1][0],value);return true;}return false; }
HSD_GObj* Item_80268B18(SpawnItem* spawn)
{
    spawns[lane]=*spawn;record_event(Create,spawn->kind,spawn->x40,spawn->prev_pos.x,spawn->prev_pos.y,spawn->prev_pos.z,spawn->facing_dir);
    if(!allocation)return NULL;unsigned index=spawn->kind==It_Kind_Ness_PKFlush?0:1;
    Item* i=&items[lane][index];i->kind=spawn->kind;i->pos=spawn->pos;i->x40_vel=spawn->vel;i->facing_dir=spawn->facing_dir;i->owner=spawn->x0_parent_gobj;
    return &objects[lane][index];
}
static int32_t create_article(HSD_GObj* owner,ItemKind kind,Vec3* position,Vec3* velocity,float facing,bool origin,uint32_t group,float value,bool initial_collision)
{
    SpawnItem spawn={0};spawn.kind=kind;Item_InitSpawn(&spawn,owner,position,facing);spawn.vel=*velocity;spawn.x40=group;spawn.x44_flag.b0=initial_collision;
    if(origin)it_8026BB68(owner,&spawn.pos);HSD_GObj* object=Item_80268B18(&spawn);if(!object)return -1;
    Item* i=object->user_data;i->xDD4_itemVar.native_article.attachment_owner=owner;i->xDD4_itemVar.native_article.creator_spawn_id=fighters[1].x8_spawnNum;i->xDD4_itemVar.native_article.creation_value=value;
    flash_logic->on_created(object);return i->x1C;
}
static int32_t create_child(HSD_GObj* parent,ItemKind kind,Vec3* position,float facing,uint32_t owner,uint32_t group,float value,unsigned backlink)
{
    if(backlink)unexpected();SpawnItem spawn={0};spawn.kind=kind;spawn.pos=spawn.prev_pos=*position;spawn.facing_dir=facing;
    spawn.x0_parent_gobj=owner==fighters[1].x8_spawnNum?&owners[1]:NULL;spawn.x4_parent_gobj2=parent;spawn.x40=group;spawn.x44_flag.b0=1;
    HSD_GObj* object=Item_80268B18(&spawn);if(!object)return -1;Item* i=object->user_data;
    i->xDD4_itemVar.native_article.attachment_owner=&owners[1];i->xDD4_itemVar.native_article.creator_spawn_id=fighters[1].x8_spawnNum;i->xDD4_itemVar.native_article.creation_value=value;
    explosion_logic->on_created(object);return i->x1C;
}
static void counters(int ground,int air,int gravity,int release)
{
    fighters[0].mv.ns.specialn.frames_to_loop_charge_ground=ground;set_f("flash_ground_loop_delay",ground);
    fighters[0].mv.ns.specialn.frames_to_loop_charge_air=air;set_f("flash_air_loop_delay",air);
    fighters[0].mv.ns.specialn.falling_acceleration_delay=gravity;set_f("flash_gravity_delay",gravity);
    fighters[0].mv.ns.specialn.charge_release_delay=release;set_f("flash_release_delay",release);
}
static void reset(unsigned motion,float facing,bool linked)
{
    memset(fighters,0,sizeof(fighters));memset(items,0,sizeof(items));memset(models,0,sizeof(models));memset(events,0,sizeof(events));memset(counts,0,sizeof(counts));
    for(unsigned l=0;l<2;++l){Fighter* f=&fighters[l];owners[l].user_data=f;f->x8_spawnNum=0xF1234567;f->parts=bones[l];f->dat_attrs=&attrs;f->motion_id=motion;f->ground_or_air=motion>=352?GA_Air:GA_Ground;
        f->facing_dir=facing;f->self_vel=(Vec3){2.25f,-.25f,0};f->cur_pos=(Vec3){11,22,3};f->cur_anim_frame=4.5f;f->co_attrs.max_jumps=2;f->co_attrs.terminal_velocity=2.5f;f->co_attrs.aerial_friction=.03f;f->x34_scale.y=1.25f;
        for(unsigned j=0;j<63;++j){bones[l][j].joint=&joints[l][j];joints[l][j].translate=(Vec3){j*.25f,j*.5f,3};}
        for(unsigned j=0;j<2;++j){objects[l][j].user_data=&items[l][j];objects[l][j].hsd_obj=&models[l][j];models[l][j].child=&children[l][j];models[l][j].scale=(Vec3){1,1,1};
            items[l][j].entity=&objects[l][j];items[l][j].x1C=0xFEDCBA98+j;items[l][j].kind=j?It_Kind_Ness_PKFlush_Explode:It_Kind_Ness_PKFlush;items[l][j].owner=&owners[l];items[l][j].xC4_article_data=&articles[j];items[l][j].xCC_item_attr=&common_attrs[j];
            items[l][j].x40_vel=(Vec3){.75f,-.25f,2};items[l][j].facing_dir=facing;items[l][j].pos=(Vec3){7,9,0};items[l][j].xD44_lifeTimer=20;if(l==1){items[l][j].xDD4_itemVar.native_article.attachment_owner=&owners[l];items[l][j].xDD4_itemVar.native_article.creator_spawn_id=f->x8_spawnNum;}}}
    items[0][0].xDD4_itemVar.pkflush.xDE0_PKFlash_Owner=&owners[0];set_i(0,"controller_enabled",1);
    fighters[0].u.ns.pkflash_gobj=linked?&objects[0][0]:NULL;set_f("flash_handle",linked?(int32_t)items[1][0].x1C:-1);counters(2,3,1,1);
}
static char* read_file(const char* path,size_t* size)
{ FILE* file=fopen(path,"rb");if(!file)abort();fseek(file,0,SEEK_END);*size=ftell(file);rewind(file);char* text=malloc(*size+1);if(!text||fread(text,1,*size,file)!=*size)abort();text[*size]=0;fclose(file);return text; }
int main(int argc,char** argv)
{
    if(argc!=3)return 2;size_t size;char* text=read_file(argv[2],&size);NativeFighterParameters parameters;unsigned line;
    if(!nativeFighterParameters_Parse(text,size,&parameters,&line))return 2;free(text);
    HMODULE library=LoadLibraryA(argv[1]);if(!library)return 2;Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");Bind bind=(Bind)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    uint32_t values[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)values[i]=parameters.fields[i].bits;if(!entry||!bind||!bind(values,parameters.count,parameters.schema_hash))return 2;
    MotionState motions[36]={0};NativeFighterDefinition base={.code_key="ness_clone",.source_kind=Ft_Kind_Ness,.animation_source_kind=Ft_Kind_Ness,.action_count=326,.joint_count=63,.motion_state_first=341,.motion_state_count=36,.motion_states=motions};
    base.special_ground_state[0]=348;base.special_air_state[0]=352;base.special_ground_state[1]=356;base.special_air_state[1]=357;base.special_ground_state[3]=367;base.special_air_state[3]=372;
    NativeFighterHost host={0};
#include "ness_flash_required_host.generated.h"
    host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.ground_friction=ft_80084F3C;
    host.fall=ftCommon_Fall;host.air_decel=ftCommon_CalcSelfAccel_Deaccel;host.ground_collision=ft_80082708;host.air_collision=ft_80081D0C;host.ground_to_air=ftCommon_8007D5D4;host.air_to_ground=ftCommon_8007D7FC;
    host.set_root_rotation_x=ftPartSetRotX;host.finish_ground_action=ft_8008A2BC;host.enter_fall=ftCo_Fall_Enter;host.enter_freefall=ftCo_80096900;host.bone_count=bone_count;host.joint_position=lb_8000B1CC;
    host.create_article_at=create_article;host.item_create_child=create_child;host.item_creator=creator;host.item_current_owner=current_owner;host.owned_article_exists=owned;host.article_ref_bool=ref_bool;host.signal_article=signal_article;host.callback_source_alive=alive;
    host.item_change_state_flags=Item_80268E5C;host.item_set_lifetime=it_80275158;host.item_use_manual_lifetime=it_8026B3A8;host.item_set_child_visible=child_visible;host.item_set_debug_owner=db_80225DD8;host.item_set_uniform_scale=scale;
    host.item_expire_tick=it_80273130;host.model_animation_active=lb_8000B09C;host.item_joint_value=joint_value;host.item_set_joint_value=joint_write;host.item_disable_hitboxes=it_802725D4;host.item_stage_collision_land=it_8026DA08;
    host.item_stop_loop_sound=Item_8026B074;host.item_play_sound=play_sound;host.camera_quake=Camera_RequestQuake;host.item_rumble=it_80273598;host.item_advance_animation=Item_802694CC;host.item_set_hitbox_damage=it_80272460;
    host.cos_float=cosf;host.sin_float=sinf;host.abs_float=fabsf;
    module=entry(&base,&host);if(!module)return 2;
    for(unsigned i=0;i<module->article_logic_count;++i){if(module->article_logics[i].kind==It_Kind_Ness_PKFlush)flash_logic=&module->article_logics[i];if(module->article_logics[i].kind==It_Kind_Ness_PKFlush_Explode)explosion_logic=&module->article_logics[i];}
    if(!flash_logic||!explosion_logic)return 2;
    for(unsigned i=0;i<sizeof(raw_attrs)/sizeof(raw_attrs[0]);++i)memcpy((unsigned char*)&attrs+raw_attrs[i].offset,&raw_attrs[i].bits,4);
    memcpy(&flash_attrs,flash_raw,sizeof(flash_raw));memcpy(&explosion_attrs,explosion_raw,sizeof(explosion_raw));articles[0].x4_specialAttributes=&flash_attrs;articles[1].x4_specialAttributes=&explosion_attrs;
    HSD_GObjEvent anim[]={ftNs_SpecialNStart_Anim,ftNs_SpecialNRelease_Anim,ftNs_SpecialNRelease_Anim,ftNs_SpecialNEnd_Anim,ftNs_SpecialAirNStart_Anim,ftNs_SpecialAirNRelease_Anim,ftNs_SpecialAirNRelease_Anim,ftNs_SpecialAirNEnd_Anim};
    HSD_GObjEvent interrupt[]={ftNs_SpecialNStart_IASA,ftNs_SpecialNRelease_IASA,ftNs_SpecialNRelease_IASA,ftNs_SpecialNEnd_IASA,ftNs_SpecialAirNStart_IASA,ftNs_SpecialAirNRelease_IASA,ftNs_SpecialAirNRelease_IASA,ftNs_SpecialAirNEnd_IASA};
    HSD_GObjEvent phys[]={ftNs_SpecialNStart_Phys,ftNs_SpecialNRelease_Phys,ftNs_SpecialNRelease_Phys,ftNs_SpecialNEnd_Phys,ftNs_SpecialAirNStart_Phys,ftNs_SpecialAirNRelease_Phys,ftNs_SpecialAirNRelease_Phys,ftNs_SpecialAirNEnd_Phys};
    HSD_GObjEvent coll[]={ftNs_SpecialNStart_Coll,ftNs_SpecialNRelease_Coll,ftNs_SpecialNRelease_Coll,ftNs_SpecialNEnd_Coll,ftNs_SpecialAirNStart_Coll,ftNs_SpecialAirNRelease_Coll,ftNs_SpecialAirNRelease_Coll,ftNs_SpecialAirNEnd_Coll};
    for(unsigned air=0;air<2;++air)for(unsigned face=0;face<2;++face){reset(348+4*air,face?-1:1,true);lane=0;(air?ftNs_SpecialAirNStart_Enter:ftNs_SpecialNStart_Enter)(&owners[0]);lane=1;(air?module->special_air_n:module->special_n)(&owners[1]);compare_fighter();}
    for(unsigned state=0;state<8;++state)for(unsigned linked=0;linked<2;++linked)for(unsigned end=0;end<2;++end)for(unsigned foreign=0;foreign<2;++foreign)for(unsigned stopped=0;stopped<2;++stopped){
        reset(348+state,1,linked);remaining=!end;allocation=linked;items[0][0].msid=items[1][0].msid=stopped?2:0;set_i(0,"stage_stopped",stopped);if(foreign)for(unsigned l=0;l<2;++l)items[l][0].owner=&others[l];
        lane=0;anim[state](&owners[0]);lane=1;module->motion_states[7+state].anim_cb(&owners[1]);compare_fighter();if((state==0||state==4)&&end&&allocation)compare_item(0);
    }
    for(unsigned state=0;state<8;++state)for(int delay=-1;delay<=2;++delay)for(unsigned held=0;held<2;++held)for(unsigned touch=0;touch<2;++touch){
        reset(348+state,-1,true);contact=touch;counters(1,1,delay,delay);for(unsigned l=0;l<2;++l)fighters[l].input.held_buttons[0]=held?HSD_PAD_B:0;
        lane=0;interrupt[state](&owners[0]);phys[state](&owners[0]);coll[state](&owners[0]);lane=1;module->motion_states[7+state].input_cb(&owners[1]);module->motion_states[7+state].phys_cb(&owners[1]);module->motion_states[7+state].coll_cb(&owners[1]);compare_fighter();
    }
    for(unsigned state=0;state<3;++state)for(unsigned reflected=0;reflected<2;++reflected)for(unsigned held=0;held<2;++held)for(unsigned foreign=0;foreign<2;++foreign)for(int life=-1;life<4;++life){
        reset(349,1,held);remaining=(life&1)!=0;for(unsigned l=0;l<2;++l){items[l][0].msid=state;items[l][0].xD44_lifeTimer=life+.25f;if(foreign)items[l][0].owner=&others[l];}
        items[0][0].xDD4_itemVar.pkflush.xDDC_PKFlash=reflected;items[0][0].xDD4_itemVar.pkflush.xDD8_PKFlash=3.25f;set_i(0,"reflected",reflected);set_i(0,"charge",bits(3.25f));
        lane=0;bool a=it_803F6B70[state].animated(&objects[0][0]);lane=1;bool b=flash_logic->logic->states[state].animated(&objects[1][0]);check(a==b,"flash removal decision");compare_trace();compare_item(0);
    }
    for(unsigned state=0;state<3;++state)for(unsigned face=0;face<2;++face)for(int stick=-5;stick<=5;++stick){
        reset(349,face?-1:1,true);for(unsigned l=0;l<2;++l)fighters[l].input.lstick[0].x=stick*.125f;
        lane=0;it_803F6B70[state].physics_updated(&objects[0][0]);lane=1;flash_logic->logic->states[state].physics_updated(&objects[1][0]);compare_item(0);
    }
    for(unsigned mask=0;mask<16;++mask)for(unsigned direction=0;direction<4;++direction){
        reset(349,1,true);for(unsigned l=0;l<2;++l){items[l][0].x378_itemColl.env_flags=(mask&1?Collide_FloorMask:0)|(mask&2?Collide_CeilingMask:0)|(mask&4?Collide_LeftWallMask:0)|(mask&8?Collide_RightWallMask:0);items[l][0].x40_vel.x=direction&1?1:-1;items[l][0].x40_vel.y=direction&2?1:-1;}
        lane=0;bool a=itNesspkflash_UnkMotion0_Coll(&objects[0][0]);lane=1;bool b=flash_logic->logic->states[0].collided(&objects[1][0]);check(a==b,"stage return");compare_trace();compare_item(0);
    }
    for(unsigned face=0;face<2;++face){reset(349,face?-1:1,true);lane=0;bool a=itNesspkflash_Logic102_Reflected(&objects[0][0]);lane=1;bool b=flash_logic->logic->reflected(&objects[1][0]);check(a==b,"reflection return");compare_item(0);}
    for(unsigned charge=0;charge<=20;++charge)for(unsigned enabled=0;enabled<2;++enabled)for(unsigned configured=0;configured<2;++configured){
        reset(349,1,true);remaining=(charge&1)!=0;float value=charge*flash_attrs.x4_FLASH_HITBOX_SIZE_MUL/20;
        for(unsigned l=0;l<2;++l){items[l][1].x5D4_hitboxes[0].hit.state=enabled?HitCapsule_Enabled:HitCapsule_Disabled;items[l][1].x5D4_hitboxes[0].hit.scale=2.75f;items[l][1].xDB4_itcmd_var2=configured;}
        items[0][1].xDD4_itemVar.pkflushexplode.xDD4=value;set_i(1,"charge",bits(value));
        lane=0;bool a=itNessPKFlashExplode_UnkMotion0_Anim(&objects[0][1]);itNessPKFlashExplode_UnkMotion0_Phys(&objects[0][1]);lane=1;bool b=explosion_logic->logic->states[0].animated(&objects[1][1]);explosion_logic->logic->states[0].physics_updated(&objects[1][1]);check(a==b,"explosion removal");compare_trace();compare_item(1);
    }
    // Constructor success/failure, raw spawn joint and origin split.
    for(unsigned air=0;air<2;++air)for(unsigned face=0;face<2;++face)for(unsigned alloc=0;alloc<2;++alloc){
        reset(348+4*air,face?-1:1,false);remaining=false;allocation=alloc;
        lane=0;anim[4*air](&owners[0]);lane=1;module->motion_states[7+4*air].anim_cb(&owners[1]);compare_fighter();compare_spawn();if(alloc)compare_item(0);
    }
    // Release creates its explosion only at the exact command threshold.
    for(int command=(int)flash_attrs.x28_FLASH_EXPLOSION_DELAY-4;command<=(int)flash_attrs.x28_FLASH_EXPLOSION_DELAY+1;++command)for(unsigned alloc=0;alloc<2;++alloc){
        reset(349,1,true);allocation=alloc;remaining=true;
        for(unsigned l=0;l<2;++l)items[l][0].xDAC_itcmd_var0=command;
        items[0][0].xDD4_itemVar.pkflush.xDD8_PKFlash=77;set_i(0,"charge",bits(77));
        lane=0;bool a=itNesspkflash_UnkMotion1_Anim(&objects[0][0]);lane=1;bool b=flash_logic->logic->states[1].animated(&objects[1][0]);
        check(a==b,"child creation removal decision");compare_trace();compare_item(0);if(a)compare_spawn();if(alloc&&a)compare_item(1);
    }
    // Explosion constructor covers all presentation thresholds and allocation failure.
    for(unsigned charge=0;charge<=40;++charge)for(unsigned alloc=0;alloc<2;++alloc){
        reset(349,-1,true);allocation=alloc;remaining=true;float value=charge*explosion_attrs.x0_FLASH_EXPL_HITBOX_SIZE_MUL/40;
        lane=0;HSD_GObj* a=it_802AF940(&owners[0],&objects[0][0],&items[0][0].pos,It_Kind_Ness_PKFlush_Explode,-1,value);
        lane=1;int32_t b=create_child(&objects[1][0],It_Kind_Ness_PKFlush_Explode,&items[1][0].pos,-1,fighters[1].x8_spawnNum,0,value,0);
        check((a!=NULL)==(b!=-1),"explosion allocation");compare_trace();compare_spawn();if(alloc)compare_item(1);
    }
    // Explicit Flash-only damage/death cleanup and normal destruction.
    for(unsigned foreign=0;foreign<2;++foreign){
        reset(349,1,true);allocation=true;remaining=false;
        lane=0;anim[0](&owners[0]);lane=1;module->motion_states[7].anim_cb(&owners[1]);
        memset(counts,0,sizeof(counts));if(foreign)for(unsigned l=0;l<2;++l)items[l][0].owner=&others[l];
        lane=0;it_2725_Logic102_Destroyed(&objects[0][0]);lane=1;flash_logic->logic->destroyed(&objects[1][0]);compare_fighter();compare_item(0);
        reset(349,1,false);lane=0;ftNs_SpecialNStart_Anim(&owners[0]);lane=1;module->motion_states[7].anim_cb(&owners[1]);
        memset(counts,0,sizeof(counts));lane=0;ftNs_Init_OnDamage(&owners[0]);lane=1;fighters[1].take_dmg_cb(&owners[1]);compare_fighter();compare_item(0);
    }
    printf("Ness Flash stock comparison: %u checks, %u failures\n",checks,failures);FreeLibrary(library);return failures?1:0;
}
