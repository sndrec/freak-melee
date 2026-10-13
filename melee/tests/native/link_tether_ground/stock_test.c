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
#include <melee/ft/kinds/ftLink/ftlink.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/ftlinkspecials.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
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

#include "ground.generated.c"

static Fighter fighters[2];
static HSD_GObj objects[2], tether[2];
static Item items[2];
static HSD_GObj other_articles[2][4];
static Item other_items[2][4];
static bool other_live[2][4];
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
enum { OtherRemove=BasicLanding+1, ModelGroup };
static unsigned event_totals[ModelGroup + 1];
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

void it_802A7B34(Fighter_GObj* g) { (void)g;abort(); }
void it_802A2B10(Item_GObj* g)
{ if(g==NULL)return;unsigned s=side(g);CHECK(tether_live[s]);event(s,Remove,0,0,0,0,0,0);tether_live[s]=false;if(s)script_fn_Tether_Unbind(&objects[1],(int32_t)items[1].x1C);else {fighters[0].u.lk.xC=NULL;fighters[0].accessory2_cb=NULL;fighters[0].death1_cb=NULL;fighters[0].accessory3_cb=NULL;} }
void it_802A78B8(Item_GObj* g,Vec3* p) { event(side(g),Launch,p->x,p->y,p->z,0,0,0); }
void it_802A77DC(Item_GObj* g) { event(side(g),Retract,0,0,0,0,0,0); }
static unsigned other_index(HSD_GObj* g,unsigned* s)
{ for(unsigned j=0;j<2;++j)for(unsigned i=0;i<4;++i)if(g==&other_articles[j][i]){*s=j;return i;}CHECK(false);return 0; }
static void remove_other(HSD_GObj* g)
{ unsigned j,i=other_index(g,&j);CHECK(other_live[j][i]);event(j,OtherRemove,i,0,0,0,0,0);other_live[j][i]=false; }
void it_802A07B4(Item_GObj* g) { remove_other(g); }
void it_802A8A7C(Item_GObj* g) { remove_other(g); }
void it_802AF304(Item_GObj* g) { remove_other(g); }
void it_802C8C34(Item_GObj* g) { remove_other(g); }
void ftParts_80074A4C(Fighter_GObj* g,int group,int value)
{ CHECK(group>=0 && group<3 && value==0);event(side(g),ModelGroup,group,value,0,0,0,0); }
static bool owned(Fighter_GObj* g,s32 raw_id)
{ uint32_t id=(uint32_t)raw_id; CHECK(g==&objects[1]);if(id==items[1].x1C)return tether_live[1] && items[1].owner==g;
  for(unsigned i=0;i<4;++i)if(id==other_items[1][i].x1C)return other_live[1][i] && other_items[1][i].owner==g;return false; }
static bool remove_owned(Fighter_GObj* g,s32 raw_id)
{ uint32_t id=(uint32_t)raw_id; CHECK(g==&objects[1]);for(unsigned i=0;i<4;++i)if(id==other_items[1][i].x1C){remove_other(&other_articles[1][i]);return true;}CHECK(false);return false; }
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

void ft_8008A2BC(Fighter_GObj* g) { event(side(g),HitBounce,0,0,0,0,0,0); }

void it_802A7AAC(Fighter_GObj*);
#include "ground_originals.h"

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
        fighters[s].kind=family;fighters[s].motion_id=360;fighters[s].ground_or_air=GA_Air;fighters[s].dat_attrs=&fighter_attrs;fighters[s].x5AC.model_num=3;
        fighters[s].parts=parts[s];parts[s][11].joint=&thumbs[s];thumbs[s].mtx[0][3]=11;thumbs[s].mtx[1][3]=13;thumbs[s].mtx[2][3]=17;
        fighters[s].facing_dir=1;fighters[s].x34_scale=(Vec3){1,1,1};fighters[s].x2EC=4.25f;fighters[s].coll_data.cur_pos=(Vec3){19,23,29};
        fighters[s].cur_pos=(Vec3){31,37,41};fighters[s].pos_delta=(Vec3){0.75f,-2,3};fighters[s].self_vel=(Vec3){1,-1,2};
        fighters[s].x74_self_accel=(Vec3){3,4,5};fighters[s].co_attrs.gravity=0.125f;fighters[s].co_attrs.terminal_velocity=3.5f;fighters[s].co_attrs.air_drift_max=1.5f;
        tether[s].user_data=&items[s];items[s].x1C=0x80000203u;items[s].owner=&objects[s];items[s].xC4_article_data=&article;tether_live[s]=true;
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
static void cleanup_matrix(void)
{
    for(unsigned mode=0;mode<7;++mode)for(unsigned mask=0;mask<32;++mask)for(unsigned used=0;used<2;++used) {
        reset(0);
        for(unsigned j=0;j<2;++j) {
            tether_live[j]=(mask&2)!=0;fighters[j].u.lk.xC=tether_live[j]?&tether[j]:NULL;
            for(unsigned i=0;i<4;++i){other_articles[j][i].user_data=&other_items[j][i];other_items[j][i].x1C=0x80001300u+i;other_items[j][i].owner=&objects[j];
                other_live[j][i]=(mask&(1u<<(i?i+1:0)))!=0;}
            fighters[j].u.lk.boomerang_gobj=other_live[j][0]?&other_articles[j][0]:NULL;
            fighters[j].u.lk.arrow_gobj=other_live[j][1]?&other_articles[j][1]:NULL;
            fighters[j].u.lk.x14=other_live[j][2]?&other_articles[j][2]:NULL;
            fighters[j].u.lk.x18=other_live[j][3]?&other_articles[j][3]:NULL;
            fighters[j].u.lk.used_boomerang=used;
        }
        integer("tether_handle",tether_live[1]?(int32_t)items[1].x1C:-1);
        const char* names[]={"cleanup_boomerang","cleanup_arrow","cleanup_bow","cleanup_milk"};
        for(unsigned i=0;i<4;++i)integer(names[i],other_live[1][i]?(int32_t)other_items[1][i].x1C:-1);
        fighters[1].native_module_state[offset("cleanup_boomerang_used")]=used;
        // Bind each recipient's actual own function pointers. Mode3 exercises
        // death2 -> death3 -> death1 in the engine's defined callback order.
        fighters[0].death1_cb=it_802A7AAC;fighters[1].death1_cb=script_fn_Tether_Remove;
        fighters[0].accessory2_cb=it_802A7AF0;fighters[1].accessory2_cb=script_fn_Tether_Update;
        fighters[0].accessory3_cb=it_802A7B34;fighters[1].accessory3_cb=script_fn_Tether_Hitlag;
        current=0;
        if(mode==0)ftLk_800EAF58(&objects[0]);
        else if(mode==1)ftLk_800EAF38(&objects[0]);
        else if(mode==2){if(family==Ft_Kind_Link)ftLk_Init_OnDeath(&objects[0]);else ftCl_Init_OnDeath(&objects[0]);}
        else if(mode==3){ftLk_800EAF58(&objects[0]);ftLk_800EAF38(&objects[0]);if(fighters[0].death1_cb)fighters[0].death1_cb(&objects[0]);}
        else if(mode==4)ftCo_800D94D8(&objects[0]);
        else if(mode==5)fn_800D949C(&objects[0]);else it_802A7AAC(&objects[0]);
        current=1;
        if(mode==0)script_fn_Fighter_RemoveArticles(&objects[1]);
        else if(mode==1)script_fn_Fighter_RemoveBoomerang(&objects[1]);
        else if(mode==2)script_fn_Fighter_DeathReset(&objects[1]);
        else if(mode==3){script_fn_Fighter_RemoveArticles(&objects[1]);script_fn_Fighter_RemoveBoomerang(&objects[1]);if(fighters[1].death1_cb)fighters[1].death1_cb(&objects[1]);}
        else if(mode==4){if(owned(&objects[1],(uint32_t)read_int("tether_handle")))script_fn_Tether_Remove(&objects[1]);}
        else if(mode==5)script_fn_Tether_ExitGround(&objects[1]);else script_fn_Tether_Remove(&objects[1]);
        compare(false,false);
        for(unsigned i=0;i<4;++i){CHECK(other_live[0][i]==other_live[1][i]);}
        Item_GObj* stock[]={fighters[0].u.lk.boomerang_gobj,fighters[0].u.lk.arrow_gobj,fighters[0].u.lk.x14,fighters[0].u.lk.x18};
        for(unsigned i=0;i<4;++i)CHECK((stock[i]!=NULL)==(read_int(names[i])!=-1));
        CHECK(fighters[0].u.lk.used_boomerang==(fighters[1].native_module_state[offset("cleanup_boomerang_used")]!=0));
    }
}
static void matrix(void)
{
    // Direct helper return and effects, not common Catch integration.
    for(unsigned dash=0;dash<2;++dash) {
        int remove=dash?fighter_attrs.xA0:fighter_attrs.x90;
        for(int tick=-2;tick<=remove+2;++tick)for(unsigned bits=0;bits<8;++bits)for(unsigned scale=0;scale<3;++scale) {
            reset(tick);create_fail=bits&1;blocked=bits&2;
            for(unsigned j=0;j<2;++j){fighters[j].motion_id=dash?214:212;fighters[j].facing_dir=(bits&4)?-1:1;
                fighters[j].x34_scale.y=(float[]){0.5f,1,1.75f}[scale];fighters[j].mv.co.catch.x0=tick;}
            current=0;bool a=dash?fn_800D9228(&objects[0]):fn_800D8EC8(&objects[0]);
            current=1;if(dash)script_fn_Tether_DashFrame(&objects[1]);else script_fn_Tether_StandingFrame(&objects[1]);
            bool b=fighters[1].native_module_state[offset("tether_ground_consumed")]!=0;
            CHECK(a==b);compare(true,false);
        }
        // Ordered real counter progression; no null article after failed constructor.
        for(unsigned bits=0;bits<8;++bits)for(unsigned scale=0;scale<3;++scale) {
            reset(0);create_fail=bits&1;blocked=bits&2;
            for(unsigned j=0;j<2;++j){fighters[j].motion_id=dash?214:212;fighters[j].mv.co.catch.x0=0;
                fighters[j].facing_dir=(bits&4)?-1:1;fighters[j].x34_scale.y=(float[]){0.5f,1,1.75f}[scale];
                fighters[j].u.lk.xC=NULL;tether_live[j]=false;}
            integer("tether_handle",-1);
            for(unsigned tick=1;tick<=(unsigned)remove+2;++tick) {
                memset(events,0,sizeof(events));memset(event_count,0,sizeof(event_count));++scenario;
                current=0;bool a=dash?fn_800D9228(&objects[0]):fn_800D8EC8(&objects[0]);
                current=1;if(dash)script_fn_Tether_DashFrame(&objects[1]);else script_fn_Tether_StandingFrame(&objects[1]);
            bool b=fighters[1].native_module_state[offset("tether_ground_consumed")]!=0;
                CHECK(a==b);compare(true,false);if(a)break;
            }
        }
    }
    CHECK(event_totals[Create] && event_totals[Launch] && event_totals[Retract] && event_totals[Remove] && event_totals[HitBounce]);
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
    fighter_attrs.x84=parameter_bits(&parameters,"tether_standing_create");fighter_attrs.x88=parameter_bits(&parameters,"tether_standing_launch");
    fighter_attrs.x8C=parameter_bits(&parameters,"tether_standing_retract");fighter_attrs.x90=parameter_bits(&parameters,"tether_standing_remove");
    fighter_attrs.x94=parameter_bits(&parameters,"tether_dash_create");fighter_attrs.x98=parameter_bits(&parameters,"tether_dash_launch");
    fighter_attrs.x9C=parameter_bits(&parameters,"tether_dash_retract");fighter_attrs.xA0=parameter_bits(&parameters,"tether_dash_remove");
    fighter_attrs.xBC=family==Ft_Kind_Link?It_Kind_Link_HShot:It_Kind_CLink_HShot;
    uint32_t launch=parameter_bits(&parameters,family==Ft_Kind_Link?"It_Kind_Link_HShot.source_launch_speed":"It_Kind_CLink_HShot.source_launch_speed");
    memcpy(&tether_attrs.pos_x_0,&launch,4);article.x4_specialAttributes=&tether_attrs;
    NativeFighterHost host={0};
#include "unused_host.h"

    host.resolve_bone=ftParts_GetBoneIndex;host.bone_count=bone_count;host.joint_position=lb_8000B1CC;
    host.spawn_bone_attached_article=spawn;host.article_ref_exists=ref;host.signal_article=signal;host.stage_segment_intersection=mpCheckAllRemap;
    host.play_sound=ft_PlaySFX;

    host.finish_ground_action=ft_8008A2BC;host.callback_source_alive=nativeCallbackSource_Alive;host.owned_article_exists=owned;host.remove_owned_article=remove_owned;host.set_visibility_group=ftParts_80074A4C;
    MotionState base_states[400]={0};NativeFighterDefinition base={.motion_states=base_states,.motion_state_count=400,.fighter_root=family==Ft_Kind_Link?"ftDataLink":"ftDataClink"};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);matrix();cleanup_matrix();
    printf("{\"assertions\":%u,\"source_cases\":%u,\"max_float_difference\":%.9g,\"coverage\":{\"ordered_timelines\":48,\"cleanup_configs\":448,\"creates\":%u,\"launches\":%u,\"retractions\":%u,\"removals\":%u,\"ground_finishes\":%u}}\n",
        checks,cases,max_difference,event_totals[Create],event_totals[Launch],event_totals[Retract],event_totals[Remove],event_totals[HitBounce]);return 0;
}
