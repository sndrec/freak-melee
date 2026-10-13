// Load the authored candidate and compare directly with original Yoshi C.
// Generic engine stubs preserve state changes and record ordered arguments.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftYoshi/ftyoshiguard.h>
#include <melee/ft/kinds/ftYoshi/ftyoshi.h>
#include <melee/ft/kinds/ftYoshi/types.h>
#include <melee/ft/kinds/ftCommon/ftCo_JumpAerial.h>
#include <melee/ft/native_fighter_visuals.h>
#include <melee/ft/native_capture.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_CaptureWait.h>
#include <melee/ft/kinds/ftCommon/ftCo_Thrown.h>
#include <melee/ft/native_capture_enclosure.h>
#include <melee/it/types.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/pl/plbonuslib.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* SetParameters)(const uint32_t*, unsigned, uint64_t);
typedef struct PolicyEvent { int id, a, b; float x, y; } PolicyEvent;
static PolicyEvent events[128], expected[128];
static unsigned event_count, expected_count, comparisons;
static bool remains, breaks;
static int interrupt;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
Vec3 const ftYs_Unk1_803B75C0 = {0};
static Fighter fighters[2];
static Fighter caught[2];
static HSD_GObj caught_objects[2];
static HSD_GObj objects[2];
static FighterBone parts[2][4];
static HSD_JObj joints[2][4];
static HSD_Joint descriptors[2][4];
static ftData data[2];
static ftData_x8 information[2];
static ftYoshiAttributes attrs[2];
static FtPartsVisLookup lookups[2][2];
static TempS variants[2][2][2];
static u8 mesh_indices[2][2][2] = {{{0,1},{2,3}},{{0,1},{2,3}}};
static HSD_DObj meshes[2][4];
static HSD_DObj* mesh_lists[2][4];
static HSD_MObj materials[2][4];
static HSD_AObj animations[2][4];
static NativeFighterParameters parameters;
static SetParameters set_parameters;
static const NativeFighterDefinition* policy;

static void require(bool ok, const char* message)
{ if (!ok) { fprintf(stderr, "%s at comparison %u\n",message,comparisons); exit(1); } }
static void record(int id,int a,int b,float x,float y)
{ require(event_count<128,"Event capacity"); events[event_count++]=(PolicyEvent){id,a,b,x,y}; }
static void remember(void)
{ expected_count=event_count; memcpy(expected,events,sizeof(events)); event_count=0; }
static void compare(void)
{
    ++comparisons;
    if(expected_count!=event_count || memcmp(expected,events,event_count*sizeof(*events))) {
        fprintf(stderr,"Events expected %u got %u\n",expected_count,event_count);
        for(unsigned i=0;i<expected_count||i<event_count;++i)
            fprintf(stderr,"%u: %d,%d,%d,%g,%g / %d,%d,%d,%g,%g\n",i,
                expected[i].id,expected[i].a,expected[i].b,expected[i].x,expected[i].y,
                events[i].id,events[i].a,events[i].b,events[i].x,events[i].y);
        require(false,"Original C event order/arguments differ");
    }
    Fighter *a=&fighters[0],*b=&fighters[1];
    require(a->motion_id==b->motion_id && a->cur_anim_frame==b->cur_anim_frame &&
        a->gr_vel==b->gr_vel && a->facing_dir==b->facing_dir &&
        a->x670_timer_lstick_tilt_x==b->x670_timer_lstick_tilt_x &&
        a->x673==b->x673 && a->trigger_analog_timer==b->trigger_analog_timer &&
        a->x221A_b7==b->x221A_b7 && a->x221B_b0==b->x221B_b0 &&
        a->x221C_b1==b->x221C_b1 && a->x221C_b2==b->x221C_b2 &&
        a->x221C_b3==b->x221C_b3 && a->x221E_b4==b->x221E_b4 &&
        memcmp(&a->mv.co.guard,&b->mv.co.guard,sizeof(a->mv.co.guard))==0,
        "Original C guard fields differ");
}
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor)
{ (void)blend; Fighter* f=GET_FIGHTER(g);require(!donor||donor==f->victim_gobj,"Unexpected animation donor"); f->motion_id=motion;f->cur_anim_frame=frame;record(1,motion,flags,frame,rate); }
void ftAnim_8006EBA4(Fighter_GObj* g) { (void)g;record(2,0,0,0,0); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g) { (void)g;record(3,0,0,0,0);return remains; }
void ftColl_8007B0C0(Fighter_GObj* g,HurtCapsuleState status) { (void)g;record(4,status,0,0,0); }
void ftColl_HurtboxInit(Fighter* f,FighterHurtCapsule* dst,ftHurtboxInit* h)
{ (void)f;(void)dst;record(5,h->bone_idx,h->height,h->scale,h->is_grabbable);require(!h->a_offset.x&&!h->a_offset.y&&!h->a_offset.z&&!h->b_offset.x&&!h->b_offset.y&&!h->b_offset.z,"Capsule offset"); }
void ftParts_80074B0C(Fighter_GObj* g,int group,int variant)
{ GET_FIGHTER(g)->x5F4_arr[group].idx=variant;record(6,group,variant,0,0); }
Fighter_Part ftParts_GetBoneIndex(Fighter* f,Fighter_Part p) { (void)f;return p==FtPart_HipN?2:p; }
void efAsync_Spawn(HSD_GObj* g,void* handle,u32 mode,u32 id,HSD_JObj* joint,...)
{ (void)handle;va_list v;va_start(v,joint);void* x=va_arg(v,void*);va_end(v);Fighter* f=GET_FIGHTER(g);require(joint==f->parts[2].joint&&x==&f->co_attrs.xBC,"Async descriptor/bone");record(7,mode,id,0,0); }
void ft_PlaySFX(Fighter* f,enum_t id,u8 volume,u8 pan) { (void)f;record(8,id,volume,pan,0); }
void ftCo_80092450(Fighter_GObj* g) { (void)g;record(9,0,0,0,0); }
void ftCo_80091D58(Fighter* f) { record(10,0,0,f->co_attrs.initial_shield_size,0); }
void ftCo_80092BCC(Fighter_GObj* g) { (void)g;record(11,0,0,0,0); }
bool ftCo_800925A4(Fighter_GObj* g) { (void)g;record(12,0,0,0,0);return breaks; }
void ftCo_800928CC(Fighter_GObj* g) { (void)g;record(13,0,0,0,0); }
void ftCo_80092BE8(Fighter_GObj* g) { (void)g;record(14,0,0,0,0); }
void ftCo_80093BC0(Fighter_GObj* g) { (void)g;record(15,0,0,0,0); }
void ftCo_80093240(Fighter_GObj* g) { (void)g; }
void ftCo_800932DC(Fighter_GObj* g) { (void)g; }
void ftCo_8009370C(Fighter_GObj* g,HSD_GObjEvent cb)
{ require(cb!=NULL,"Reflect response missing");cb(g);record(16,0,0,0,0); }
void ft_8008A2BC(Fighter_GObj* g) { (void)g;record(17,0,0,0,0); }
#define CHECK(name,id) bool name(Fighter_GObj* g) { (void)g;record(id,0,0,0,0);return interrupt==id; }
CHECK(ftCo_80093694,20) CHECK(ftCo_8009515C,21) CHECK(ftCo_80099794,22)
CHECK(ftCo_8009917C,23) CHECK(ftCo_Catch_CheckInput,24) CHECK(ftCo_8009A080,25)
#define EVENT(name,id) void name(Fighter_GObj* g) { (void)g;record(id,0,0,0,0); }
EVENT(ftCo_GuardOn_Phys,30) EVENT(ftCo_GuardOn_Coll,31)
EVENT(ftCo_Guard_Phys,30) EVENT(ftCo_Guard_Coll,31)
EVENT(ftCo_GuardOff_Phys,32) EVENT(ftCo_GuardOff_Coll,31)
EVENT(ftCo_GuardSetOff_Phys,33) EVENT(ftCo_GuardSetOff_Coll,34)
EVENT(ftCo_GuardReflect_IASA,35) EVENT(ftCo_GuardReflect_Phys,30) EVENT(ftCo_GuardReflect_Coll,31)
void ftAnim_8006FA58(Fighter* f,Fighter_Part part,HSD_Joint* descriptor)
{ require(descriptor==f->x108_costume_joint->child,"Original raw costume descriptor mismatch");record(36,part,0,0,0); }
static HSD_Joint* find_pose(Fighter* f,HSD_JObj* j,HSD_Joint* d)
{ require(j==f->parts[1].joint&&d==f->x108_costume_joint,"Generic pose lookup mismatch");return d->child; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { (void)j; }
void OSReport(char* format, ...) { (void)format; }
void __assert(char* file,u32 line,char* expression)
{ fprintf(stderr,"%s:%lu %s\n",file,line,expression);exit(1); }
void ftCommon_8007D5D4(Fighter* f) { (void)f;record(37,0,0,0,0); }
void ftCo_JumpAerial_Begin(Fighter_GObj* g,FtMotionId id,Vec3* v,bool reset,bool driven,bool sound)
{ record(38,id,reset|(driven<<1)|(sound<<2),v->x,v->y);require(v->z==0,"Jump z");GET_FIGHTER(g)->motion_id=id; }

void ftCo_80099314(Fighter_GObj* g,FtMotionId id,bool reverse)
{ GET_FIGHTER(g)->mv.co.escape.x0=reverse;record(52,id,reverse,0,0); }
EVENT(ftCo_800998EC,53)
void original_ftCo_80099644(Fighter_GObj*);
#include "yoshi_escape_policy_original.inc"
void ftAnim_SetAnimRate(Fighter_GObj* g,float rate) { (void)g;record(42,0,0,rate,0); }
void ftCo_800DB368(Fighter* holder,Fighter* victim)
{ require(holder->victim_gobj->user_data==victim&&victim->victim_gobj->user_data==holder,"Reciprocal capture links");record(43,0,0,0,0); }
EVENT(ftCo_800DB464,44)
float ftCommon_GetModelScale(Fighter* f) { return f->x34_scale.y; }
float ftYs_Init_8012BAC0(Fighter* f) { return ((ftYoshiAttributes*)f->dat_attrs)->x120; }
void ftCommon_8007E2F4(Fighter* f,s16 mask) { (void)f;record(45,mask,0,0,0); }
void ftCommon_8007EBAC(Fighter* f,u32 mode,u32 arg) { (void)f;record(46,mode,arg,0,0); }
void ftColl_8007B62C(Fighter_GObj* g,int status) { (void)g;record(47,status,0,0,0); }
void pl_80040614(int player,bool team,float timer) { record(48,player,team,timer,0); }
EVENT(ftCommon_8007E358,49)
void it_802A7840(HSD_GObj* g) { (void)g;require(false,"Tether item outside Yoshi fixture"); }
void it_802BAA94(Item_GObj* g) { (void)g;require(false,"Tether item outside Yoshi fixture"); }
EVENT(fn_800DA190,50) EVENT(ftCo_800DE508,51)
bool ftCommon_GrabMash(Fighter* f,float rate) { (void)f;(void)rate;require(false,"Mash outside fixture");return false; }
void ftCo_800DA698(Fighter_GObj* g,bool released) { (void)g;(void)released;require(false,"Release outside fixture"); }
void ftCo_CaptureCut_Enter(Fighter_GObj* g) { (void)g;require(false,"CaptureCut outside fixture"); }
void fn_800DC070(Fighter_GObj* g) { (void)g;require(false,"CaptureCut outside fixture"); }
bool fn_800DAD18(Fighter_GObj* g) { (void)g;require(false,"Capture physics outside fixture");return false; }
void ft_80083C00(Fighter_GObj* g,HSD_GObjEvent cb) { (void)g;(void)cb;require(false,"Capture collision outside fixture"); }
void ft_8008403C(Fighter_GObj* g,HSD_GObjEvent cb) { (void)g;(void)cb;require(false,"Capture collision outside fixture"); }
void ftCommon_8007D7FC(Fighter* f) { (void)f;require(false,"Force air outside fixture"); }
void ftCommon_UnlockECB(Fighter* f) { (void)f;require(false,"ECB unlock outside fixture"); }
void fn_800DAA40(Fighter_GObj* holder,Fighter_GObj* victim) { (void)holder;(void)victim;require(false,"Capture release outside fixture"); }
void ftCo_800DE2CC(Fighter_GObj* holder,Fighter_GObj* victim) { (void)holder;(void)victim;require(false,"Enclosure release outside fixture"); }
void ftCo_CaptureEnclosure_Enter(Fighter_GObj* victim,Fighter_GObj* holder,FtMotionId motion,const NativeCaptureEnclosureSettings* settings,HSD_Joint* model,float facing)
{ (void)victim;(void)holder;(void)motion;(void)settings;(void)model;(void)facing;require(false,"Enclosure outside fixture"); }
static bool scripted_capture;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return scripted_capture&&kind==Ft_Kind_Yoshi?policy:NULL; }
void original_fn_800DB790(Fighter_GObj*);
void original_fn_800DBAE4(Fighter_GObj*);
#include "yoshi_capture_policy_original.inc"
static void ftYs_JumpAerial_Anim_Cb(Fighter_GObj*);
#include "../../src/melee/ft/kinds/ftYoshi/ftyoshiguard.c"
#include "yoshi_common_original.inc"

void HSD_AObjSetRate(HSD_AObj* a,float rate) { a->framerate=rate; }
void HSD_MObjReqAnim(HSD_MObj* m,float frame) { (void)m;record(40,0,0,frame,0); }
void HSD_MObjAnim(HSD_MObj* m) { (void)m;record(41,0,0,0,0); }
#include "../../src/melee/ft/native_fighter_visuals.c"
static void copy_attrs(Fighter_GObj* g) { (void)g; }
static float common_attribute(NativeFighterCommonAttribute a)
{
    switch(a) {
    case NativeFighterCommon_ShieldStartHealth:return common.x260_startShieldHealth;
    case NativeFighterCommon_ShieldMinimumHold:return common.x268;
    case NativeFighterCommon_ShieldPushbackDamage:return common.x28C;
    case NativeFighterCommon_ShieldPushbackBase:return common.x290;
    case NativeFighterCommon_ShieldPushbackVelocity:return common.x294;
    case NativeFighterCommon_ShieldReflectDuration:return common.x2A4;
    case NativeFighterCommon_ShieldPowershieldDuration:return common.x2B4;
    default:require(false,"Unexpected common attribute");return 0;
    }
}
static unsigned bone_count(Fighter* f) { (void)f;return 4; }
static void initialize_fighters(void)
{
    memset(fighters,0,sizeof(fighters));memset(joints,0,sizeof(joints));
    memset(parts,0,sizeof(parts));memset(data,0,sizeof(data));event_count=0;
    for(unsigned i=0;i<2;++i) {
        objects[i].user_data=&fighters[i]; fighters[i].gobj=&objects[i];
        for(unsigned b=0;b<4;++b) {parts[i][b].joint=&joints[i][b];joints[i][b].flags=JOBJ_MTX_DIRTY;}
        fighters[i].x5AC.model_num=1;fighters[i].hurt_capsules_len=1;fighters[i].parts=parts[i];fighters[i].ft_data=&data[i];data[i].x8=&information[i];information[i].x11=3;
        fighters[i].dat_attrs=&attrs[i];data[i].ext_attr=&attrs[i];attrs[i].xC=30;
        data[i].ext_attr=&attrs[i];fighters[i].x108_costume_joint=&descriptors[i][0];descriptors[i][0].child=&descriptors[i][1];
        fighters[i].co_attrs.initial_shield_size=2.375f;fighters[i].facing_dir=1;fighters[i].shield_health=37;
        for(unsigned b=0;b<4;++b){mesh_lists[i][b]=&meshes[i][b];meshes[i][b].mobj=&materials[i][b];materials[i][b].aobj=&animations[i][b];animations[i][b].end_frame=30;animations[i][b].framerate=1.375f;}
        fighters[i].dobj_list=(DObjList){4,mesh_lists[i]};
        for(unsigned table=0;table<2;++table){lookups[i][table].x0=2;lookups[i][table].x4=variants[i][table];variants[i][table][1]=(TempS){2,mesh_indices[i][table]};fighters[i].x5AC.xC[table]=&lookups[i][table];}
        policy->on_load(&objects[i]);
        for(unsigned b=0;b<4;++b)require(animations[i][b].framerate==0,"Load must freeze material animation");
    }
    event_count=0;
}
static void bind_parameter(const char* name,uint32_t bits)
{ for(unsigned i=0;i<parameters.count;++i)if(!strcmp(parameters.fields[i].name,name)){parameters.fields[i].bits=bits;return;}require(false,"Named parameter missing"); }
static void upload(void)
{ uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];for(unsigned i=0;i<parameters.count;++i)words[i]=parameters.fields[i].bits;require(set_parameters(words,parameters.count,parameters.schema_hash),"Module parameter binding"); }
static void unexpected_operation(void) { require(false,"Operation outside the common-action fixture"); }

int main(int argc,char** argv)
{
    require(argc==3,"Expected module and owned parameters");
    FILE* file=fopen(argv[2],"rb");require(file!=NULL,"FMP open");char text[32768];size_t size=fread(text,1,sizeof(text),file);fclose(file);unsigned line;
    require(nativeFighterParameters_Parse(text,size,&parameters,&line),"FMP parse");
    HMODULE library=LoadLibraryA(argv[1]);require(library!=NULL,"Module load");
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");set_parameters=(SetParameters)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    require(entry&&set_parameters,"Module exports");
    MotionState motions[28]={0};NativeFighterDefinition base={.fighter_root="ftDataYoshi",.motion_state_first=341,.motion_state_count=28,.motion_states=motions};
    base.special_ground_state[NativeFighterSpecial_Neutral]=346;base.special_air_state[NativeFighterSpecial_Neutral]=351;
    base.special_ground_state[NativeFighterSpecial_Side]=356;base.special_air_state[NativeFighterSpecial_Side]=360;
    base.special_ground_state[NativeFighterSpecial_Up]=364;base.special_air_state[NativeFighterSpecial_Up]=365;
    base.special_ground_state[NativeFighterSpecial_Down]=366;base.special_air_state[NativeFighterSpecial_Down]=368;
    NativeFighterHost host={.change_motion_state=Fighter_ChangeMotionState,.start_animation=ftAnim_8006EBA4,
        .animation_frames_remaining=ftAnim_IsFramesRemaining,.set_hurtboxes=ftColl_8007B0C0,.init_hurtbox=ftColl_HurtboxInit,
        .set_current_visibility_group=ftParts_80074B0C,.resolve_bone=ftParts_GetBoneIndex,.spawn_async_effect=efAsync_Spawn,
        .play_sound=ft_PlaySFX,.install_guard=ftCo_80092450,.scale_guard_joint=ftCo_80091D58,
        .consume_guard_release_input=ftCo_80092BCC,.drain_guard=ftCo_800925A4,.enter_guard_hold=ftCo_800928CC,
        .enter_guard_release=ftCo_80092BE8,.tick_powershield=ftCo_80093BC0,.guard_sdi=ftCo_80093240,.guard_post_sdi=ftCo_800932DC,
        .install_guard_reflector=ftCo_8009370C,.finish_ground_action=ft_8008A2BC,.finish_action=ft_8008A2BC,.check_powershield=ftCo_80093694,
        .check_guard_item_throw=ftCo_8009515C,.check_ground_roll=ftCo_80099794,.check_ground_escape=ftCo_8009917C,
        .check_catch=ftCo_Catch_CheckInput,.check_guard_platform_drop=ftCo_8009A080,.guard_physics=ftCo_Guard_Phys,
        .guard_collision=ftCo_Guard_Coll,.ground_friction=ftCo_GuardOff_Phys,.guard_hit_physics=ftCo_GuardSetOff_Phys,
        .guard_hit_collision=ftCo_GuardSetOff_Coll,.guard_reflect_actions=ftCo_GuardReflect_IASA,
        .copy_bone_pose=ftAnim_8006FA58,.find_costume_pose=find_pose,.set_joint_translation=HSD_JObjSetTranslate,
        .current_model_group=nativeFighterVisuals_CurrentGroup,.bone_count=bone_count,.set_group_material_frame=nativeFighterVisuals_SetGroupMaterialFrame,.initialize_group_material_animation=nativeFighterVisuals_InitializeGroupMaterialAnimation,
        .copy_ext_attributes=copy_attrs,.common_attribute=common_attribute,.ground_to_air=ftCommon_8007D5D4,
        .begin_aerial_jump=ftCo_JumpAerial_Begin,.add_joint_rotation_y=HSD_JObjAddRotationY,.update_aerial_jump=ftCo_JumpAerial_Anim};
    host.set_animation_rate=ftAnim_SetAnimRate;host.capture_enclose_caught=nativeCapture_EncloseCaught;
    host.constrain_caught=ftCo_800DB368;host.set_body_status=ftColl_8007B62C;
    // The complete candidate also contains accepted specials/articles. Their
    // unused operations must fail immediately if this fixture reaches them.
    _Static_assert(sizeof(host)%sizeof(void*)==0,"Host contains function pointers");
    for(unsigned offset=0;offset<sizeof(host);offset+=sizeof(void*)) {
        uintptr_t value;memcpy(&value,(char*)&host+offset,sizeof(value));
        if(!value){value=(uintptr_t)unexpected_operation;memcpy((char*)&host+offset,&value,sizeof(value));}
    }
    policy=entry(&base,&host);require(policy!=NULL,"Candidate definition");upload();
    common.x260_startShieldHealth=60;common.x268=5;common.x28C=.25f;common.x290=.75f;common.x294=1.125f;common.x2A4=3;common.x2B4=4;
    static HSD_GObjEvent stock_entries[]={ftYs_Init_8012BECC,ftYs_Shield_8012C1D4,ftYs_Shield_8012C49C,NULL,ftYs_Shield_8012C850,ftYs_Shield_8012C914};
    for(unsigned mode=0;mode<6;++mode)for(unsigned special=0;special<2;++special)
    for(unsigned power=0;power<2;++power)for(unsigned sign=0;sign<2;++sign)for(unsigned light=0;light<4;++light) {
        initialize_fighters();for(unsigned i=0;i<2;++i){fighters[i].cur_anim_frame=3.375f;fighters[i].lightshield_amount=light*.375f;fighters[i].x19A4=7;fighters[i].specialn_facing_dir=sign?-1:1;fighters[i].gr_vel=3.25f;fighters[i].x221C_b2=power;fighters[i].x673=13;}
        if(mode==3)ftYs_Shield_8012C600(&objects[0],special);else stock_entries[mode](&objects[0]);remember();
        policy->on_shield_transition(&objects[1],mode,special);compare();
        require(!memcmp(&joints[0][3].translate,&joints[1][3].translate,sizeof(Vec3)),"Shield translation");
    }
    static HSD_GObjEvent stock_anim[]={ftYs_GuardOn_0_Anim,ftYs_GuardHold_Anim,ftYs_GuardOff_Anim,ftYs_GuardDamage_Anim,ftYs_GuardOn_1_Anim};
    static HSD_GObjEvent stock_input[]={ftYs_GuardOn_0_IASA,ftYs_GuardHold_IASA,ftYs_GuardOff_IASA,ftYs_GuardDamage_IASA,ftYs_GuardOn_1_IASA};
    for(unsigned state=0;state<5;++state)for(unsigned finished=0;finished<2;++finished)for(unsigned broken=0;broken<2;++broken)
    for(unsigned release=0;release<2;++release)for(unsigned active=0;active<2;++active)for(unsigned reflecting=0;reflecting<2;++reflecting) {
        initialize_fighters();remains=!finished;breaks=broken;
        for(unsigned i=0;i<2;++i){fighters[i].mv.co.guard.x0=3.125f;fighters[i].mv.co.guard.xC=release;fighters[i].x221B_b0=active;fighters[i].reflecting=reflecting;}
        stock_anim[state](&objects[0]);remember();policy->motion_states[state].anim_cb(&objects[1]);compare();
    }
    for(unsigned state=0;state<5;++state)for(interrupt=0;interrupt<=25;++interrupt) {
        initialize_fighters();stock_input[state](&objects[0]);remember();policy->motion_states[state].input_cb(&objects[1]);compare();
    }
    for(unsigned finished=0;finished<2;++finished)for(unsigned reverse=0;reverse<2;++reverse)
    for(unsigned held=0;held<2;++held)for(int health=-1;health<=1;++health) {
        initialize_fighters();remains=!finished;
        original_ftCo_80099438(&objects[0],233,reverse);remember();
        ftCo_80099314(&objects[1],233,reverse);policy->on_roll_enter(&objects[1]);compare();
        for(unsigned i=0;i<2;++i){fighters[i].throw_flags_b3=reverse;fighters[i].input.held_buttons[0]=held?HSD_PAD_LR:0;fighters[i].shield_health=health;fighters[i].gr_vel=3.25f;}
        event_count=0;original_ftCo_80099644(&objects[0]);remember();fighters[1].anim_cb(&objects[1]);compare();
        require(fighters[0].throw_flags_b3==fighters[1].throw_flags_b3,"Roll reverse cue consumption");
    }
    for(unsigned current=0;current<2;++current)for(unsigned previous=0;previous<2;++previous) {
        initialize_fighters();for(unsigned i=0;i<2;++i){fighters[i].x5F4_arr[0].idx=current;fighters[i].x5F4_arr[0].prev=previous;}
        original_ftCo_80099954(&objects[0]);remember();policy->on_spotdodge_enter(&objects[1]);
        ++comparisons;require(expected_count==event_count+1&&!memcmp(expected,events,event_count*sizeof(*events))&&expected[event_count].id==53,"Spotdodge must restore current selector before shared entry");
    }
    remains=true;
    for(unsigned count=1;count<=13;++count)for(unsigned sign=0;sign<2;++sign)for(int stick=-4;stick<=4;++stick) {
        initialize_fighters();bind_parameter("aerial_turn_frames",count);upload();
        for(unsigned i=0;i<2;++i){attrs[i].x0=count;attrs[i].x4=.3f;attrs[i].x8=120;fighters[i].input.lstick[0].x=stick*.25f;fighters[i].facing_dir=sign?-1:1;fighters[i].co_attrs.air_jump_h_multiplier=1.375f;}
        ftYs_JumpAerial_Enter(&objects[0]);remember();policy->on_aerial_jump(&objects[1]);
        require(expected_count==event_count&&!memcmp(expected,events,event_count*sizeof(*events)),"Jump entry events");event_count=0;
        for(unsigned tick=0;tick<=count;++tick){event_count=0;ftYs_JumpAerial_Anim_Cb(&objects[0]);remember();fighters[1].anim_cb(&objects[1]);require(expected_count==event_count&&!memcmp(expected,events,event_count*sizeof(*events)),"Jump animation order");++comparisons;require(fighters[0].facing_dir==fighters[1].facing_dir&&!memcmp(&joints[0][0].rotate,&joints[1][0].rotate,sizeof(Vec3)),"Aerial turn float math/facing");}
    }
    float caught_radius=8.25f;uint32_t radius_bits;memcpy(&radius_bits,&caught_radius,4);bind_parameter("caught_radius",radius_bits);upload();
    for(unsigned setup=0;setup<4;++setup)for(unsigned scale=1;scale<=4;++scale) {
        initialize_fighters();memset(caught,0,sizeof(caught));
        for(unsigned i=0;i<2;++i){caught_objects[i].user_data=&caught[i];fighters[i].kind=Ft_Kind_Yoshi;fighters[i].victim_gobj=&caught_objects[i];caught[i].victim_gobj=&objects[i];caught[i].kind=Ft_Kind_Fox;caught[i].parts=parts[i];caught[i].ft_data=&data[i];caught[i].x34_scale.y=scale*.5f;caught[i].motion_id=ftCo_MS_CapturePulledHi;attrs[i].x120=8.25f;}
        scripted_capture=false;
        if(setup==0)original_fn_800DB5D8(&caught_objects[0]);
        if(setup==1)original_fn_800DB790(&caught_objects[0]);
        if(setup==2)original_fn_800DBAE4(&caught_objects[0]);
        if(setup==3)original_fn_800DB6C8(&caught_objects[0]);
        remember();scripted_capture=true;
        if(setup==0)ftCo_CaptureWait_Setup(&caught_objects[1]);
        if(setup==1)fn_800DB790(&caught_objects[1]);
        if(setup==2)fn_800DBAE4(&caught_objects[1]);
        if(setup==3)fn_800DB6C8(&caught_objects[1]);
        ++comparisons;require(expected_count==event_count&&!memcmp(expected,events,event_count*sizeof(*events)),"CaptureWait setup/initial constraint order");
        require(caught[0].motion_id==caught[1].motion_id&&caught[0].invisible==caught[1].invisible&&caught[0].accessory1_cb==caught[1].accessory1_cb,"CaptureWait fields");
    }
    static const u8 remapped_frames[]={46,44,42,40,38,36,34,33,32,31,29,29};
    for(unsigned dash=0;dash<2;++dash)for(int frame=-2;frame<=124;++frame) {
        initialize_fighters();
        for(unsigned i=0;i<2;++i){fighters[i].kind=Ft_Kind_Yoshi;fighters[i].motion_id=dash?212:214;fighters[i].cur_anim_frame=frame*.25f;attrs[i].x124=16;attrs[i].x128=28;memcpy(attrs[i].x12C,remapped_frames,sizeof(remapped_frames));}
        original_fn_800D9CE8(&objects[0]);float start=fighters[0].cur_anim_frame;
        remember();float script_frame=policy->on_catch_pull_frame(&objects[1]);
        ++comparisons;require(start==script_frame,"CatchPull remapped start frame differs");
        require(expected_count==event_count+1&&!memcmp(expected,events,event_count*sizeof(*events)),"CatchPull rate/start order differs");
        require(expected[event_count].id==1&&expected[event_count].a==(dash?213:215)&&expected[event_count].b==0x4000&&expected[event_count].y==1,"Common CatchPull transition/rate contract");
    }
    for(unsigned sign=0;sign<2;++sign)for(unsigned motion=239;motion<=244;++motion) {
        initialize_fighters();memset(caught,0,sizeof(caught));
        for(unsigned i=0;i<2;++i){caught_objects[i].user_data=&caught[i];fighters[i].kind=Ft_Kind_Yoshi;fighters[i].victim_gobj=&caught_objects[i];fighters[i].facing_dir=sign?-1:1;caught[i].victim_gobj=&objects[i];caught[i].kind=Ft_Kind_Fox;caught[i].grab_timer=37.25f;}
        original_ftCo_800DE3FC(&caught_objects[0],motion,.75f);remember();
        require(expected_count==7&&expected[0].id==43&&expected[1].id==1&&expected[2].id==47&&expected[3].id==48,"Original throw setup order");
        policy->on_throw_victim_setup(&objects[1]);++comparisons;
        require(event_count==1&&!memcmp(&expected[2],events,sizeof(*events)),"Holder-authored throw body status differs");
    }
    static const float material_ends[]={0,2.375f,14.25f,30};
    static const float healths[]={-1,0,3.125f,30,60,61.5f};
    for(unsigned end=0;end<4;++end)for(unsigned health=0;health<6;++health) {
        initialize_fighters();
        for(unsigned i=0;i<2;++i)for(unsigned mesh=0;mesh<4;++mesh){animations[i][mesh].end_frame=material_ends[end];animations[i][mesh].framerate=1.375f;}
        attrs[0].xC=0;ftYoshi_InitializeShieldMaterialAnimations(&fighters[0],&lookups[0][0]);ftYoshi_InitializeShieldMaterialAnimations(&fighters[0],&lookups[0][1]);
        policy->on_load(&objects[1]);
        for(unsigned mesh=0;mesh<4;++mesh)require(animations[0][mesh].framerate==animations[1][mesh].framerate,"Material initialization rate differs");
        for(unsigned i=0;i<2;++i){fighters[i].shield_health=healths[health];fighters[i].mv.co.guard.x0=0;}
        event_count=0;remains=true;breaks=false;ftYs_GuardOn_0_Anim(&objects[0]);remember();policy->motion_states[0].anim_cb(&objects[1]);compare();
    }
    for(unsigned sign=0;sign<2;++sign)for(int stick=-4;stick<=4;++stick) {
        initialize_fighters();bind_parameter("aerial_turn_frames",12);upload();
        for(unsigned i=0;i<2;++i){attrs[i].x0=12;attrs[i].x4=.3f;attrs[i].x8=120;fighters[i].input.lstick[0].x=stick*.25f;fighters[i].facing_dir=sign?-1:1;fighters[i].co_attrs.air_jump_h_multiplier=1.375f;}
        ftYs_JumpAerial_Enter(&objects[0]);event_count=0;policy->on_aerial_jump(&objects[1]);event_count=0;
        remains=false;ftYs_JumpAerial_Anim_Cb(&objects[0]);remember();fighters[1].anim_cb(&objects[1]);
        ++comparisons;require(expected_count==event_count&&!memcmp(expected,events,event_count*sizeof(*events)),"Jump completion common entry order");
        require(fighters[0].motion_id==fighters[1].motion_id&&fighters[0].mv.co.fallaerial.smid==fighters[1].mv.co.fallaerial.smid&&fighters[0].facing_dir==fighters[1].facing_dir&&!memcmp(&joints[0][0].rotate,&joints[1][0].rotate,sizeof(Vec3)),"Jump completion must observe and update shared fall selector");
    }
    printf("Yoshi common original-C comparisons: %u exact samples.\n",comparisons);
    FreeLibrary(library);return 0;
}
