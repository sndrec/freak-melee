#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/kinds/ftCommon/ftCo_AirCatch.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftSamus/types.h>
#include <melee/ft/kinds/ftSamus/forward.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/lb/lb_00B0.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "aircatch.generated.c"

static Fighter fighters[2];
static HSD_GObj objects[2], tether[2];
static Item items[2];
static Article article;
static struct TetherAttributes tether_attrs;
static ftLk_DatAttrs fighter_attrs;
static FighterBone parts[2][MAX_FT_PARTS];
static HSD_JObj thumbs[2];
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static const NativeFighterDefinition* definition;
static unsigned checks, cases, scenario, current;
static bool create_fail, blocked, finished, request_fast, collision_result, hit_aux;
static bool tether_live[2];
static unsigned capture_mask[2];
static float max_difference;
static FighterKind family;
enum { Change=1, Capture, Create, Launch, Retract, Remove, Sound, Freefall,
       CheckFast, Fast, Fall, Drift, Clamp, Air, Collision, Landing,
       HitSweep, HitAux, HitThreshold, HitBounce, BasicLanding };
static unsigned event_totals[BasicLanding + 1];
typedef struct Event { int id; float value[6]; } TraceEvent;
static TraceEvent events[2][32];
static unsigned event_count[2];
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"scenario%u line%u: %s\n",scenario,__LINE__,#x); abort(); } } while(0)
static unsigned side(HSD_GObj* g) { return g == &objects[1] || g == &tether[1]; }
static unsigned fside(Fighter* f) { return f == &fighters[1]; }
static void event(unsigned s,int id,float a,float b,float c,float d,float e,float f)
{ CHECK(event_count[s]<32); events[s][event_count[s]++]=(TraceEvent){id,{a,b,c,d,e,f}}; }
static void same(float a,float b)
{ CHECK(isfinite(a) && isfinite(b)); float delta=fabsf(a-b); if(delta>max_difference)max_difference=delta; CHECK(delta<=0.000001f); }
static void unused_host(void) { fprintf(stderr,"unused Host reached scenario%u\n",scenario); abort(); }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return current==1 && kind==family ? definition : NULL; }
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* other)
{ CHECK(!other); unsigned s=side(g); event(s,Change,motion,flags,frame,rate,blend,0); fighters[s].motion_id=motion; }
void ftCommon_8007E2F4(Fighter* f,s16 mask)
{ unsigned s=fside(f);capture_mask[s]=(unsigned)mask;event(s,Capture,mask,0,0,0,0,0); }
void ftCommon_ClampSelfVelX(Fighter* f,float maximum)
{ event(fside(f),Clamp,maximum,0,0,0,0,0); if(f->self_vel.x>maximum)f->self_vel.x=maximum;else if(f->self_vel.x< -maximum)f->self_vel.x= -maximum; }
void ftCommon_8007D5D4(Fighter* f) { event(fside(f),Air,0,0,0,0,0,0);f->ground_or_air=GA_Air; }
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part bone)
{ CHECK(f==&fighters[0] || f==&fighters[1]); CHECK(bone==FtPart_RThumbNb);return 11; }
static unsigned bone_count(Fighter* f) { CHECK(f==&fighters[1]);return MAX_FT_PARTS; }
void lb_8000B1CC(HSD_JObj* j,Vec3* offset,Vec3* out)
{ CHECK(!offset && (j==&thumbs[0] || j==&thumbs[1]));*out=(Vec3){j->mtx[0][3],j->mtx[1][3],j->mtx[2][3]}; }
void HSD_JObjSetupMatrixSub(HSD_JObj* j) { j->flags &= ~JOBJ_MTX_DIRTY; }
Item_GObj* it_802A2BA4(Fighter_GObj* g,Vec3* p,float facing,s32 kind)
{ unsigned s=side(g);CHECK(kind==fighter_attrs.xBC);event(s,Create,p->x,p->y,p->z,facing,kind,0);tether_live[s]=!create_fail;return create_fail?NULL:&tether[s]; }
static s32 spawn(Fighter_GObj* g,ItemKind kind,Fighter_Part bone)
{ CHECK(bone==FtPart_RThumbNb);Vec3 p;lb_8000B1CC(&thumbs[1],NULL,&p);CHECK(kind==fighter_attrs.xBC);return it_802A2BA4(g,&p,fighters[1].facing_dir,kind)?(s32)items[1].x1C:-1; }
void it_802A7AF0(Fighter_GObj* g) { (void)g;abort(); }
void it_802A7AAC(Fighter_GObj* g) { (void)g;abort(); }
void it_802A7B34(Fighter_GObj* g) { (void)g;abort(); }
void it_802A2B10(Item_GObj* g)
{ unsigned s=side(g);CHECK(tether_live[s]);event(s,Remove,0,0,0,0,0,0);tether_live[s]=false;if(s)script_fn_Tether_Unbind(&objects[1],(int32_t)items[1].x1C);else {fighters[0].u.lk.xC=NULL;fighters[0].accessory2_cb=NULL;fighters[0].death1_cb=NULL;fighters[0].accessory3_cb=NULL;} }
void it_802A78B8(Item_GObj* g,Vec3* p) { event(side(g),Launch,p->x,p->y,p->z,0,0,0); }
void it_802A77DC(Item_GObj* g) { event(side(g),Retract,0,0,0,0,0,0); }
static bool ref(HSD_GObj* g,uint32_t id,ItemKind kind)
{ CHECK(g==&objects[1] && kind==fighter_attrs.xBC);return id==items[1].x1C && tether_live[1]; }
static uint32_t hash(const char* s) { uint32_t h=2166136261u;for(;*s;++s)h=(h^(uint8_t)*s)*16777619u;return h; }
static bool signal(HSD_GObj* g,uint32_t id,uint32_t name,float payload)
{
    CHECK(g==&objects[1] && id==items[1].x1C && tether_live[1]);
    if(name==hash("constructor_post_pickup")) { CHECK(payload==0);return true; }
    if(name==hash("extend_air")) { Vec3 p={tether_attrs.pos_x_0*fighters[1].facing_dir+payload,0,0};it_802A78B8(&tether[1],&p); }
    else if(name==hash("retract")) { CHECK(payload==0);it_802A77DC(&tether[1]); }
    else { CHECK(name==hash("remove") && payload==0);it_802A2B10(&tether[1]); }
    return true;
}
bool mpCheckAllRemap(Vec3* p,int* line,u32* flags,Vec3* normal,int skip,int only,float x0,float y0,float x1,float y1)
{ CHECK(!p && !line && !flags && !normal && skip==-1 && only==-1);event(current,Collision,x0,y0,x1,y1,0,0);return blocked; }
void ft_PlaySFX(Fighter* f,enum_t sound,u8 volume,u8 pan)
{ event(fside(f),Sound,sound,volume,pan,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g) { CHECK(g==&objects[current]);return !finished; }
void ftCo_80096900(Fighter_GObj* g,int a,int b,bool allow,float mobility,float lag)
{ event(side(g),Freefall,a,b,allow,mobility,lag,0);fighters[side(g)].motion_id=35; }
bool ftCommon_CheckFallFast(Fighter* f)
{ event(fside(f),CheckFast,0,0,0,0,0,0);if(request_fast)f->fall_fast=true;return request_fast; }
void ftCommon_FallFast(Fighter* f) { event(fside(f),Fast,0,0,0,0,0,0);f->self_vel.y= -7; }
void ftCommon_Fall(Fighter* f,float gravity,float terminal)
{ event(fside(f),Fall,gravity,terminal,0,0,0,0);f->self_vel.y=fmaxf(f->self_vel.y-gravity,-terminal); }
void ftCommon_CalcSelfAccel_Drift(Fighter* f) { event(fside(f),Drift,0,0,0,0,0,0);f->x74_self_accel.x=0.03125f; }
GroundOrAir ft_80081D0C(Fighter_GObj* g) { event(side(g),Collision,0,0,0,0,0,0);return (GroundOrAir)collision_result; }
static GroundOrAir air_collision(Fighter_GObj* g) { return (GroundOrAir)ft_80081D0C(g); }
void ftCo_Landing_Enter(Fighter_GObj* g,FtMotionId motion,bool allow,MotionFlags flags,float frame,float rate)
{ event(side(g),Landing,motion,allow,flags,frame,rate,0);fighters[side(g)].motion_id=motion; }
bool mpColl_800471F8(CollData* c)
{ unsigned s=c==&fighters[1].coll_data;event(s,HitSweep,0,0,0,0,0,0);c->cur_pos.x+=0.5f;return collision_result; }
bool ft_80081A00(Fighter_GObj* g) { event(side(g),HitAux,0,0,0,0,0,0);return hit_aux; }
float ftCo_800D0EC8(Fighter* f) { event(fside(f),HitThreshold,0,0,0,0,0,0);return 0.5f; }
void ft_8008A2BC(Fighter_GObj* g) { event(side(g),HitBounce,0,0,0,0,0,0); }
void ftCo_Landing_Enter_Basic(Fighter_GObj* g) { event(side(g),BasicLanding,0,0,0,0,0,0); }
// Link/CLink-only matrix: unexpected Samus branches fail instead of being delegated.
Item_GObj* it_802B7C18(Fighter_GObj* g,Vec3* p,float face) { (void)g;(void)p;(void)face;abort(); }
void it_802BAC80(Fighter_GObj* g) { (void)g;abort(); }
void it_802BAC3C(Fighter_GObj* g) { (void)g;abort(); }
void it_802BACC4(Fighter_GObj* g) { (void)g;abort(); }
void it_802B7B84(Item_GObj* g) { (void)g;abort(); }
void it_802BAAE4(Item_GObj* g,Vec3* p) { (void)g;(void)p;abort(); }
void it_802BAA58(Item_GObj* g) { (void)g;abort(); }
#include "aircatch_originals.h"

static unsigned offset(const char* name)
{ for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;CHECK(false);return 0; }
static void integer(const char* name,int value) { memcpy(fighters[1].native_module_state+offset(name),&value,4); }
static void scalar(const char* name,float value) { memcpy(fighters[1].native_module_state+offset(name),&value,4); }
static float read_scalar(const char* name) { float value;memcpy(&value,fighters[1].native_module_state+offset(name),4);return value; }
static int read_int(const char* name) { int value;memcpy(&value,fighters[1].native_module_state+offset(name),4);return value; }
static void reset(float counter)
{
    ++scenario;memset(fighters,0,sizeof(fighters));memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));
    memset(capture_mask,0,sizeof(capture_mask));create_fail=blocked=finished=request_fast=collision_result=hit_aux=false;
    for(unsigned s=0;s<2;++s){objects[s].user_data=&fighters[s];objects[s].next=s?NULL:&objects[1];fighters[s].x8_spawnNum=0x80000003u+s;
        fighters[s].kind=family;fighters[s].motion_id=360;fighters[s].ground_or_air=GA_Air;fighters[s].dat_attrs=&fighter_attrs;
        fighters[s].parts=parts[s];parts[s][11].joint=&thumbs[s];thumbs[s].mtx[0][3]=11;thumbs[s].mtx[1][3]=13;thumbs[s].mtx[2][3]=17;
        fighters[s].facing_dir=1;fighters[s].x34_scale=(Vec3){1,1,1};fighters[s].x2EC=4.25f;fighters[s].coll_data.cur_pos=(Vec3){19,23,29};
        fighters[s].cur_pos=(Vec3){31,37,41};fighters[s].pos_delta=(Vec3){0.75f,-2,3};fighters[s].self_vel=(Vec3){1,-1,2};
        fighters[s].x74_self_accel=(Vec3){3,4,5};fighters[s].co_attrs.gravity=0.125f;fighters[s].co_attrs.terminal_velocity=3.5f;fighters[s].co_attrs.air_drift_max=1.5f;
        tether[s].user_data=&items[s];items[s].x1C=0x80000203u;items[s].xC4_article_data=&article;tether_live[s]=true;
    }
    heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[0];fighters[0].u.lk.xC=&tether[0];fighters[0].mv.co.aircatch.x0=counter;
    integer("tether_handle",(int32_t)items[1].x1C);integer("tether_attachment_part",11);scalar("tether_counter",counter);
}
static void compare(bool counter,bool timers)
{
    ++cases;CHECK(event_count[0]==event_count[1]);
    for(unsigned e=0;e<event_count[0];++e){CHECK(events[0][e].id==events[1][e].id);++event_totals[events[0][e].id];for(unsigned v=0;v<6;++v)same(events[0][e].value[v],events[1][e].value[v]);}
    CHECK(fighters[0].motion_id==fighters[1].motion_id);CHECK(fighters[0].ground_or_air==fighters[1].ground_or_air);
    CHECK(fighters[0].fall_fast==fighters[1].fall_fast);CHECK(fighters[0].used_tether==fighters[1].used_tether);
    CHECK(capture_mask[0]==capture_mask[1]);CHECK(tether_live[0]==tether_live[1]);
    Vec3* a[]={&fighters[0].self_vel,&fighters[0].x74_self_accel,&fighters[0].cur_pos,&fighters[0].coll_data.cur_pos,&fighters[0].coll_data.last_pos};
    Vec3* b[]={&fighters[1].self_vel,&fighters[1].x74_self_accel,&fighters[1].cur_pos,&fighters[1].coll_data.cur_pos,&fighters[1].coll_data.last_pos};
    for(unsigned i=0;i<5;++i){same(a[i]->x,b[i]->x);same(a[i]->y,b[i]->y);same(a[i]->z,b[i]->z);}
    if(counter)same(fighters[0].mv.co.aircatch.x0,read_scalar("tether_counter"));
    if(timers){CHECK(fighters[0].mv.co.aircatchhit.x0==read_int("tether_hit_timer"));CHECK(fighters[0].mv.co.aircatchhit.x4==read_int("tether_hit_aux"));}
    CHECK((fighters[0].accessory2_cb!=NULL)==(fighters[1].accessory2_cb!=NULL));
    CHECK((fighters[0].accessory3_cb!=NULL)==(fighters[1].accessory3_cb!=NULL));CHECK((fighters[0].death1_cb!=NULL)==(fighters[1].death1_cb!=NULL));
}
static void matrix(void)
{
    // Whole original input decision table, including grounded no-entry but consumed input.
    for(unsigned bits=0;bits<256;++bits){reset(3);
        for(unsigned s=0;s<2;++s){fighters[s].used_tether=bits&1;fighters[s].accessory2_cb=(bits&2)?it_802A7AF0:NULL;
            fighters[s].death1_cb=(bits&4)?it_802A7AAC:NULL;fighters[s].accessory3_cb=(bits&8)?it_802A7B34:NULL;
            fighters[s].item_gobj=(bits&16)?&tether[s]:NULL;fighters[s].input.held_buttons[0]=(bits&32)?HSD_PAD_LR:0;
            fighters[s].input.pressed_buttons=(bits&64)?HSD_PAD_A:0;fighters[s].ground_or_air=(bits&128)?GA_Air:GA_Ground;}
        current=0;bool stock=ftCo_800C3B10(&objects[0]);current=1;CHECK(stock==definition->on_air_tether_input(&objects[1]));compare(stock && (bits&128),false);
    }
    for(unsigned bits=0;bits<16;++bits){reset(3);for(unsigned s=0;s<2;++s){fighters[s].ground_or_air=(bits&1)?GA_Air:GA_Ground;
        fighters[s].self_vel.x=(bits&2)?-4:4;fighters[s].fall_fast=bits&4;fighters[s].co_attrs.air_drift_max=(bits&8)?1.5f:5;}
        current=0;ftCo_800C3CC0(&objects[0]);current=1;script_fn_Tether_EnterHit(&objects[1]);compare(false,true);
    }
    for(int tick=-1;tick<=fighter_attrs.xB0+2;++tick)for(unsigned bits=0;bits<16;++bits)for(unsigned scale=0;scale<3;++scale){reset(tick);
        create_fail=bits&1;blocked=bits&2;finished=bits&4;for(unsigned s=0;s<2;++s){fighters[s].facing_dir=(bits&8)?-1:1;fighters[s].x34_scale.y=(float[]){0.5f,1,1.75f}[scale];}
        current=0;ftCo_AirCatch_Anim(&objects[0]);current=1;definition->motion_states[360].anim_cb(&objects[1]);compare(true,false);
    }
    for(unsigned counter=0;counter<3;++counter)for(unsigned bits=0;bits<16;++bits){reset((float[]){19,20,21}[counter]);
        request_fast=bits&1;for(unsigned s=0;s<2;++s){fighters[s].fall_fast=bits&2;fighters[s].pos_delta.y=(bits&4)?-1:0;fighters[s].co_attrs.gravity=(bits&8)?0.17f:0.125f;}
        current=0;ftCo_AirCatch_Phys(&objects[0]);current=1;definition->motion_states[360].phys_cb(&objects[1]);compare(true,false);
        reset(0);for(unsigned s=0;s<2;++s){fighters[s].pos_delta=(Vec3){(bits&1)?-5:3,(bits&2)?-4:2,(bits&4)?-7:1};fighters[s].co_attrs.terminal_velocity=(bits&8)?1:5;}
        current=0;ftCo_AirCatchHit_Phys(&objects[0]);current=1;definition->motion_states[361].phys_cb(&objects[1]);compare(false,false);
    }
    for(unsigned bits=0;bits<8;++bits){reset(0);collision_result=bits&1;hit_aux=bits&2;for(unsigned s=0;s<2;++s)fighters[s].self_vel.y=(bits&4)?1:0;
        current=0;ftCo_AirCatch_Coll(&objects[0]);current=1;definition->motion_states[360].coll_cb(&objects[1]);compare(false,false);
        reset(0);collision_result=bits&1;hit_aux=bits&2;for(unsigned s=0;s<2;++s)fighters[s].self_vel.y=(bits&4)?1:0;
        current=0;ftCo_AirCatchHit_Coll(&objects[0]);current=1;definition->motion_states[361].coll_cb(&objects[1]);compare(false,false);
    }
    reset(0);current=0;ftCo_AirCatchHit_Anim(&objects[0]);ftCo_AirCatch_IASA(&objects[0]);ftCo_AirCatchHit_IASA(&objects[0]);
    current=1;definition->motion_states[361].anim_cb(&objects[1]);definition->motion_states[360].input_cb(&objects[1]);definition->motion_states[361].input_cb(&objects[1]);compare(false,false);
    // Normal input followed by ordered animation/physics/collision frames, retaining
    // callback bindings and the same article handle throughout each whole timeline.
    for(unsigned bits=0;bits<16;++bits){reset(0);create_fail=bits&1;blocked=bits&2;
        for(unsigned s=0;s<2;++s){fighters[s].input.held_buttons[0]=HSD_PAD_LR;fighters[s].input.pressed_buttons=HSD_PAD_A;
            fighters[s].facing_dir=(bits&4)?-1:1;fighters[s].u.lk.xC=NULL;tether_live[s]=false;}
        integer("tether_handle",-1);current=0;CHECK(ftCo_800C3B10(&objects[0]));current=1;CHECK(definition->on_air_tether_input(&objects[1]));compare(true,false);
        for(unsigned tick=1;tick<=66 && fighters[0].motion_id==360;++tick){
            memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));++scenario;
            finished=tick==65;request_fast=(bits&8) && tick>=20;
            current=0;ftCo_AirCatch_Anim(&objects[0]);current=1;definition->motion_states[360].anim_cb(&objects[1]);compare(true,false);
            if(fighters[0].motion_id!=360)break;
            memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));
            current=0;ftCo_AirCatch_Phys(&objects[0]);current=1;definition->motion_states[360].phys_cb(&objects[1]);compare(true,false);
            memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));
            collision_result=false;current=0;ftCo_AirCatch_Coll(&objects[0]);current=1;definition->motion_states[360].coll_cb(&objects[1]);compare(true,false);
        }
    }
    CHECK(event_totals[Change] && event_totals[Create] && event_totals[Launch] && event_totals[Retract] && event_totals[Remove]);
    CHECK(event_totals[Freefall] && event_totals[Fast] && event_totals[Fall] && event_totals[Landing] && event_totals[HitBounce] && event_totals[BasicLanding]);
}
static uint32_t parameter_bits(const NativeFighterParameters* p,const char* name)
{ for(unsigned i=0;i<p->count;++i)if(!strcmp(p->fields[i].name,name))return p->fields[i].bits;CHECK(false);return 0; }
int main(int argc,char** argv)
{
    if(argc!=3)return 2;family=!strcmp(argv[2],"link")?Ft_Kind_Link:Ft_Kind_CLink;
    FILE* file=fopen(argv[1],"rb");CHECK(file);fseek(file,0,SEEK_END);long size=ftell(file);rewind(file);char* text=malloc(size+1);CHECK(text);
    CHECK(fread(text,1,size,file)==(size_t)size);fclose(file);NativeFighterParameters parameters;unsigned error;
    CHECK(nativeFighterParameters_Parse(text,size,&parameters,&error));free(text);uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<parameters.count;++i)bits[i]=parameters.fields[i].bits;CHECK(FreakMelee_SetFighterParameters(bits,parameters.count,parameters.schema_hash));
    fighter_attrs.xA4=parameter_bits(&parameters,"tether_air_create");fighter_attrs.xA8=parameter_bits(&parameters,"tether_air_launch");
    fighter_attrs.xAC=parameter_bits(&parameters,"tether_air_retract");fighter_attrs.xB0=parameter_bits(&parameters,"tether_air_remove");
    fighter_attrs.xBC=family==Ft_Kind_Link?It_Kind_Link_HShot:It_Kind_CLink_HShot;
    uint32_t launch=parameter_bits(&parameters,family==Ft_Kind_Link?"It_Kind_Link_HShot.source_launch_speed":"It_Kind_CLink_HShot.source_launch_speed");
    memcpy(&tether_attrs.pos_x_0,&launch,4);article.x4_specialAttributes=&tether_attrs;
    NativeFighterHost host={0};
#include "unused_host.h"
    host.change_motion_state=Fighter_ChangeMotionState;host.set_capture_mask=ftCommon_8007E2F4;host.clamp_self_vel_x=ftCommon_ClampSelfVelX;
    host.ground_to_air=ftCommon_8007D5D4;host.resolve_bone=ftParts_GetBoneIndex;host.bone_count=bone_count;host.joint_position=lb_8000B1CC;
    host.spawn_bone_attached_article=spawn;host.article_ref_exists=ref;host.signal_article=signal;host.stage_segment_intersection=mpCheckAllRemap;
    host.play_sound=ft_PlaySFX;host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.enter_freefall=ftCo_80096900;
    host.check_fastfall=ftCommon_CheckFallFast;host.fastfall=ftCommon_FallFast;host.fall=ftCommon_Fall;host.air_accel_normal=ftCommon_CalcSelfAccel_Drift;
    host.air_collision=air_collision;host.enter_landing=ftCo_Landing_Enter;host.air_catch_hit_collision=ftCo_AirCatchHit_Coll;host.callback_source_alive=nativeCallbackSource_Alive;
    MotionState base_states[400]={0};NativeFighterDefinition base={.motion_states=base_states,.motion_state_count=400,.fighter_root=family==Ft_Kind_Link?"ftDataLink":"ftDataClink"};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition && definition->on_air_tether_input);matrix();
    printf("{\"assertions\":%u,\"source_cases\":%u,\"max_float_difference\":%.9g,\"coverage\":{\"input_configs\":256,\"hit_entry_configs\":16,\"ordered_timelines\":16,\"creates\":%u,\"launches\":%u,\"retractions\":%u,\"removals\":%u,\"freefall_calls\":%u,\"fastfalls\":%u,\"falls\":%u,\"landings\":%u,\"hit_bounces\":%u,\"basic_landings\":%u}}\n",
        checks,cases,max_difference,event_totals[Create],event_totals[Launch],event_totals[Retract],event_totals[Remove],event_totals[Freefall],event_totals[Fast],event_totals[Fall],event_totals[Landing],event_totals[HitBounce],event_totals[BasicLanding]);return 0;
}
