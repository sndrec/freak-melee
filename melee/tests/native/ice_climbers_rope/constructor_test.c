/* Complete original paired bodies and actual generated two-program callbacks.
 * Registry/lifetime, collision-history copy and scalar physics are production
 * bodies. Family topology, motion/animation, stage probes and world transforms
 * are explicitly controlled boundaries, not claims about their lower math. */
#include <windows.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ef/eflib.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0892.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>
#include <melee/ft/kinds/ftCommon/ftCo_Landing.h>
#include <melee/ft/kinds/ftPopo/ftpopospecialhi.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_refs.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/itclimbersstring.h>
#include <melee/it/kinds/itclimbersice.h>
#include <melee/ft/kinds/ftPopo/ftpopospeciallw.h>
#include <melee/lb/lbvector.h>
#include <melee/ft/kinds/ftCommon/inlines.h>
#include <melee/ft/kinds/ftNana/ftnana.h>
#include <melee/ft/kinds/ftPopo/ftpopo.h>
#include <melee/ft/kinds/ftPopo/ftpopospecials.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_relations.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/lb/lb_00B0.h>
#include <melee/mp/mpcoll.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>

static unsigned checks, pairs, safety;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"pair%u safety%u line%d %s\n",pairs,safety,__LINE__,#x); exit(1); } } while (0)
static Fighter fighters[2], expected[2];
static HSD_GObj objects[2], *heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static FighterBone parts[2][48];
static HSD_JObj joints[2][48];
static ftIceClimberAttributes attributes[2];
static const NativeFighterDefinition* definitions[2];
static FighterPartsTable parts_description = { .parts_num = 48 };
static FighterPartsTable* parts_table[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = parts_table;
static bool scripted, present[2], remaining[2], grounded_probe, landed_probe, cliff_probe;
static unsigned invalidation, invalidation_role;

enum { Change, Start, ToAir, ToGround, GroundPhysics, GroundProbe, AirProbe,
       Freefall, CliffProbe, Landing, Spawn, RotationX, RotationY, WorldPosition, Rate, CopyCollision };
typedef struct Trace {
    int op, role, a, b;
    float x, y, z;
    unsigned callbacks, commands;
} Trace;
static Trace trace[128], expected_trace[128];
static unsigned trace_count, expected_count;
static unsigned role_of(Fighter_GObj* g) { CHECK(g == &objects[0] || g == &objects[1]); return g == &objects[1]; }
static unsigned fighter_role(Fighter* f) { CHECK(f == &fighters[0] || f == &fighters[1]); return f == &fighters[1]; }
static void event(int op, unsigned role, int a, int b, float x, float y, float z)
{
    Fighter* f = &fighters[role];
    unsigned cb = (f->take_dmg_cb != NULL) | ((f->death2_cb != NULL) << 1) |
        ((f->pre_hitlag_cb != NULL) << 2) | ((f->post_hitlag_cb != NULL) << 3) |
        ((f->accessory4_cb != NULL) << 4);
    unsigned commands = 0;
    for (unsigned i=0;i<4;++i) commands |= (f->cmd_vars[i] != 0) << i;
    CHECK(trace_count < 128);
    trace[trace_count++] = (Trace){op,(int)role,a,b,x,y,z,cb,commands};
}
static void unexpected(void) { CHECK(!"unexpected controlled boundary"); }
void __assert(char* f, u32 line, char* m) { fprintf(stderr,"%s:%lu %s\n",f,line,m);exit(1); }
bool nativeModelEffect_Snapshot(HSD_GObj* g, uint32_t* generation) { (void)g;*generation=0;return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g, uint32_t generation) { (void)g;(void)generation;return NULL; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    return kind == Ft_Kind_CustomFirst ? definitions[0] : kind == Ft_Kind_CustomFirst+1 ? definitions[1] : NULL;
}
static void registry(void)
{
    heads[HSD_GOBJ_PLINK_FIGHTER] = present[0] ? &objects[0] : present[1] ? &objects[1] : NULL;
    objects[0].next = present[1] ? &objects[1] : NULL;
    objects[1].next = NULL;
}
Fighter_GObj* ftLib_8008741C(u32 id)
{
    for (HSD_GObj* g=heads[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next)
        if (g->user_data && (uint32_t)((Fighter*)g->user_data)->x8_spawnNum == (uint32_t)id) return g;
    return NULL;
}
u32 ftLib_80087460(Fighter_GObj* g) { return ((Fighter*)g->user_data)->x8_spawnNum; }
uint32_t nativeFighterTarget_Id(HSD_GObj* g)
{
    uint32_t id;
    return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Fighter,&id) ? id : 0;
}
HSD_GObj* Player_GetEntityAtIndex(int player, int member) { CHECK(player==0);return (unsigned)member<2 && present[member] ? &objects[member] : NULL; }
static uint32_t member_id(Fighter_GObj* source, unsigned member)
{
    uint32_t id;
    if (!nativeCallbackSource_Snapshot(source,NativeCallbackSource_Fighter,&id)) return 0;
    return member<2 && present[member] ? (uint32_t)fighters[member].x8_spawnNum : 0;
}
static void invalidate(unsigned boundary, unsigned role)
{
    if (role != invalidation_role) return;
    if (invalidation == boundary) { present[role]=false;objects[role].user_data=NULL;registry(); }
    if (invalidation == boundary+10) ++fighters[role].x8_spawnNum;
}
void Fighter_ChangeMotionState(Fighter_GObj* g,FtMotionId id,MotionFlags flags,float frame,float rate,float blend,Fighter_GObj* donor)
{
    unsigned r=role_of(g);CHECK(donor==NULL);event(Change,r,id,(int)flags,frame,rate,blend);
    fighters[r].motion_id=id;fighters[r].cur_anim_frame=frame;fighters[r].frame_speed_mul=rate;invalidate(1,r);
}
void ftAnim_8006EBA4(Fighter_GObj* g) { unsigned r=role_of(g);event(Start,r,0,0,0,0,0);invalidate(2,r); }
bool ftAnim_IsFramesRemaining(Fighter_GObj* g) { return remaining[role_of(g)]; }
void ftAnim_SetAnimRate(Fighter_GObj* g,float rate) { unsigned r=role_of(g);event(Rate,r,0,0,rate,0,0);fighters[r].frame_speed_mul=rate; }
void ft_8008A2BC(Fighter_GObj* g) { unsigned r=role_of(g);event(Change,r,14,0,0,0,0);fighters[r].motion_id=14; }
void ftCo_Fall_Enter(Fighter_GObj* g) { unsigned r=role_of(g);event(Change,r,29,0,0,0,0);fighters[r].motion_id=29; }
void ftCommon_8007D5D4(Fighter* f) { unsigned r=fighter_role(f);event(ToAir,r,0,0,0,0,0);f->ground_or_air=GA_Air; }
void ftCommon_8007D60C(Fighter* f) { unsigned r=fighter_role(f);event(ToAir,r,1,0,0,0,0);f->ground_or_air=GA_Air;f->x1968_jumpsUsed=f->co_attrs.max_jumps; }
void ftCommon_8007D7FC(Fighter* f) { unsigned r=fighter_role(f);event(ToGround,r,0,0,0,0,0);f->ground_or_air=GA_Ground; }
void ft_80084F3C(Fighter_GObj* g) { unsigned r=role_of(g);event(GroundPhysics,r,0,0,0,0,0);fighters[r].xE4_ground_accel_1=-.125f; }
bool ft_800827A0(Fighter_GObj* g) { event(GroundProbe,role_of(g),0,0,0,0,0);return grounded_probe; }
extern ftCollisionBox ftNn_Unk2_803CDD60;
bool ft_80082888(Fighter_GObj* g,ftCollisionBox* box) { CHECK(!memcmp(box,&ftNn_Unk2_803CDD60,sizeof(*box)));event(GroundProbe,role_of(g),1,0,0,0,0);return grounded_probe; }
bool ft_800824A0(Fighter_GObj* g,ftCollisionBox* box) { CHECK(!memcmp(box,&ftNn_Unk2_803CDD60,sizeof(*box)));event(AirProbe,role_of(g),1,0,0,0,0);return landed_probe; }
GroundOrAir ft_80081D0C(Fighter_GObj* g) { event(AirProbe,role_of(g),0,0,0,0,0);return landed_probe ? GA_Air : GA_Ground; }
void ftCo_80096900(Fighter_GObj* g,int a,int b,bool interrupt,float mobility,float lag)
{
    unsigned r=role_of(g);event(Freefall,r,a,b,interrupt,mobility,lag);fighters[r].motion_id=35;
}
void efLib_PauseAll(HSD_GObj* g) { (void)g;unexpected(); }
void efLib_ResumeAll(HSD_GObj* g) { (void)g;unexpected(); }

void ftCommon_8007F76C(Fighter_GObj*g){(void)g;unexpected();}
static Item_GObj rope_object;
static Item rope_item;
static Article rope_article;
static itClimbersStringAttributes rope_attributes;
bool ft_CheckGroundAndLedge(Fighter_GObj*g,int facing){CHECK(facing==(fighters[role_of(g)].facing_dir>0?1:-1));event(AirProbe,role_of(g),2,0,0,0,0);return landed_probe;}
bool ftCliffCommon_80081298(Fighter_GObj*g){event(CliffProbe,role_of(g),0,0,0,0,0);return cliff_probe;}
void ftCo_LandingFallSpecial_Enter(Fighter_GObj*g,bool interrupt,float lag){event(Landing,role_of(g),interrupt,0,lag,0,0);fighters[role_of(g)].motion_id=43;}
void ftPartSetRotX(Fighter* f,int part,float value)
{
    unsigned r=fighter_role(f);CHECK(part==0);event(RotationX,r,part,0,value,0,0);joints[r][part].rotate.x=value;
}
void ftPartSetRotY(Fighter* f,int part,float value)
{
    unsigned r=fighter_role(f);CHECK(part==0);event(RotationY,r,part,0,value,0,0);joints[r][part].rotate.y=value;
}
float ftPartGetRotX(Fighter* f,int part) { return joints[fighter_role(f)][part].rotate.x; }
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* result)
{
    CHECK(offset==NULL);
    for(unsigned r=0;r<2;++r)for(unsigned p=0;p<48;++p)if(joint==&joints[r][p]){
        event(WorldPosition,r,p,0,0,0,0);*result=joint->translate;invalidate(3,r);return;
    }
    unexpected();
}
static const NativeFighterCallbackExport* find_export(uint32_t id,uint32_t hash,NativeFighterReturnType type,Fighter_GObj** recipient)
{
    *recipient=ftLib_8008741C(id);
    if(!*recipient)return NULL;
    const NativeFighterDefinition*d=nativeFighterRuntime_Definition(((Fighter*)(*recipient)->user_data)->kind);
    if(!d)return NULL;
    for(unsigned i=0;i<d->callback_export_count;++i)
        if(d->callback_exports[i].name_hash==hash && d->callback_exports[i].result_type==type)return &d->callback_exports[i];
    return NULL;
}
static bool target_bool(uint32_t id,uint32_t hash) { Fighter_GObj*g;const NativeFighterCallbackExport*e=find_export(id,hash,NativeFighterReturn_Bool,&g);return e?e->bool_callback(g):false; }
static float target_float(uint32_t id,uint32_t hash) { Fighter_GObj*g;const NativeFighterCallbackExport*e=find_export(id,hash,NativeFighterReturn_Float,&g);return e?e->float_callback(g):0; }
static bool target_void(uint32_t id,uint32_t hash) { Fighter_GObj*g;const NativeFighterCallbackExport*e=find_export(id,hash,NativeFighterReturn_Void,&g);if(!e)return false;e->callback(g);return true; }
static int32_t target_motion(uint32_t id) { Fighter_GObj*g=ftLib_8008741C(id);return g?((Fighter*)g->user_data)->motion_id:-1; }
static bool target_airborne(uint32_t id) { Fighter_GObj*g=ftLib_8008741C(id);return g&&((Fighter*)g->user_data)->ground_or_air==GA_Air; }
static unsigned bone_count(Fighter* f) { (void)f;return 48; }
static Fighter_Part resolve_bone(Fighter* f,Fighter_Part p) { (void)f;return p; }

#include "constructor_dependencies.inc"
#include "actual.inc"
#include "constructor_actual.inc"
#include "constructor_candidate.inc"
#include "candidate_boundaries.inc"
#include "paired_parameters.inc"

static unsigned field(unsigned role,const char* name)
{
    for(unsigned i=0;i<definitions[role]->state_field_count;++i)
        if(!strcmp(definitions[role]->state_fields[i].name,name))return definitions[role]->state_fields[i].offset;
    CHECK(false);return 0;
}
static void set_int(unsigned r,const char*n,int32_t v) { memcpy(fighters[r].native_module_state+field(r,n),&v,4); }
static void set_float(unsigned r,const char*n,float v) { memcpy(fighters[r].native_module_state+field(r,n),&v,4); }
static int32_t get_int(unsigned r,const char*n) { int32_t v;memcpy(&v,fighters[r].native_module_state+field(r,n),4);return v; }
static float get_float(unsigned r,const char*n) { float v;memcpy(&v,fighters[r].native_module_state+field(r,n),4);return v; }
static uint32_t name_hash(const char* s) { uint32_t h=2166136261u;while(*s)h=(h^(unsigned char)*s++)*16777619u;return h; }
static void initialize(bool script,int primary_motion,int secondary_motion)
{
    scripted=script;invalidation=0;trace_count=0;memset(trace,0,sizeof(trace));
    memset(fighters,0,sizeof(fighters));memset(objects,0,sizeof(objects));memset(joints,0,sizeof(joints));
    memset(parts,0,sizeof(parts));memset(heads,0,sizeof(heads));
    for(unsigned r=0;r<2;++r){
        present[r]=true;remaining[r]=true;objects[r].user_data=&fighters[r];fighters[r].gobj=&objects[r];
        fighters[r].kind=script?Ft_Kind_CustomFirst+r:r?Ft_Kind_Nana:Ft_Kind_Popo;
        fighters[r].x8_spawnNum=UINT32_C(0xFEDCBA98)-r*UINT32_C(0x1020304);fighters[r].dat_attrs=&attributes[r];
        fighters[r].parts=parts[r];parts_table[fighters[r].kind]=&parts_description;
        fighters[r].motion_id=r?secondary_motion:primary_motion;fighters[r].ground_or_air=GA_Air;
        fighters[r].cur_anim_frame=8.25f+r;fighters[r].frame_speed_mul=.75f+r;fighters[r].facing_dir=r?-1:1;
        fighters[r].x34_scale=(Vec3){1,1.25f,1};fighters[r].cur_pos=(Vec3){1.5f+r*2,3.25f+r,2.75f-r};
        fighters[r].self_vel=(Vec3){.625f+r,-.5f-r,.875f+r};fighters[r].x74_self_accel=(Vec3){.25f+r,.5f+r,.75f+r};
        fighters[r].gr_vel=.25f+r;fighters[r].xE4_ground_accel_1=.375f+r;
        fighters[r].co_attrs.gravity=.12f;fighters[r].co_attrs.terminal_velocity=2.5f;fighters[r].co_attrs.aerial_friction=.125f;
        fighters[r].co_attrs.ground_friction=.175f;fighters[r].co_attrs.max_jumps=2;
        fighters[r].co_attrs.air_drift_stick_mul=.0625f;fighters[r].co_attrs.air_drift_max=1.25f;fighters[r].co_attrs.air_max_horizontal_velocity=2.5f;
        fighters[r].coll_data.floor.normal=(Vec3){.6f,.8f,0};fighters[r].coll_data.cur_pos=fighters[r].cur_pos;
        fighters[r].coll_data.prev_pos=(Vec3){-1,-2,-3};fighters[r].coll_data.last_pos=(Vec3){-4,-5,-6};
        for(unsigned p=0;p<48;++p){parts[r][p].joint=&joints[r][p];joints[r][p].translate=(Vec3){p*.125f+r,p*.0625f-r,p*.03125f};}
        joints[r][0].rotate.x=.125f+r;for(unsigned p=0;p<4;++p)fighters[r].cmd_vars[p]=p+1;
    }
    registry();grounded_probe=true;landed_probe=false;cliff_probe=false;
    if(script){set_int(0,"ice_handle",-1);set_int(0,"linked_rope_handle",-1);set_int(0,"rope_counter",3);}
    else fighters[0].mv.pp.unk_80123954.x0=3;
}
static void save(void) { memcpy(expected,fighters,sizeof(expected));expected_count=trace_count;memcpy(expected_trace,trace,sizeof(trace)); }
static void same_float(float a,float b) { if(memcmp(&a,&b,4))fprintf(stderr,"float original %.9g generated %.9g\n",b,a);CHECK(!memcmp(&a,&b,4)); }
static void compare(void)
{
    ++pairs;CHECK(trace_count==expected_count);
    if(memcmp(trace,expected_trace,trace_count*sizeof(*trace))){for(unsigned i=0;i<trace_count;++i){Trace*a=&expected_trace[i],*b=&trace[i];fprintf(stderr,"event%u original op%d role%d a%d b%d xyz%.9g %.9g %.9g cb%u cmd%u generated op%d role%d a%d b%d xyz%.9g %.9g %.9g cb%u cmd%u\n",i,a->op,a->role,a->a,a->b,a->x,a->y,a->z,a->callbacks,a->commands,b->op,b->role,b->a,b->b,b->x,b->y,b->z,b->callbacks,b->commands);}}
    CHECK(!memcmp(trace,expected_trace,trace_count*sizeof(*trace)));
    for(unsigned r=0;r<2;++r){Fighter*f=&fighters[r],*e=&expected[r];
        CHECK(f->motion_id==e->motion_id);CHECK(f->ground_or_air==e->ground_or_air);CHECK(f->x1968_jumpsUsed==e->x1968_jumpsUsed);
        same_float(f->cur_anim_frame,e->cur_anim_frame);same_float(f->frame_speed_mul,e->frame_speed_mul);same_float(f->facing_dir,e->facing_dir);
        same_float(f->gr_vel,e->gr_vel);same_float(f->xE4_ground_accel_1,e->xE4_ground_accel_1);
        for(unsigned i=0;i<3;++i){same_float((&f->cur_pos.x)[i],(&e->cur_pos.x)[i]);same_float((&f->self_vel.x)[i],(&e->self_vel.x)[i]);same_float((&f->x74_self_accel.x)[i],(&e->x74_self_accel.x)[i]);}
        for(unsigned i=0;i<4;++i)CHECK(f->cmd_vars[i]==e->cmd_vars[i]);
        CHECK((f->x1A5C!=NULL)==(e->x1A5C!=NULL));CHECK(f->x221A_b1==e->x221A_b1);CHECK(f->x2222_b2==e->x2222_b2);
        CHECK((f->take_dmg_cb!=NULL)==(e->take_dmg_cb!=NULL));CHECK((f->death2_cb!=NULL)==(e->death2_cb!=NULL));
        CHECK((f->pre_hitlag_cb!=NULL)==(e->pre_hitlag_cb!=NULL));CHECK((f->post_hitlag_cb!=NULL)==(e->post_hitlag_cb!=NULL));
        CHECK((f->accessory4_cb!=NULL)==(e->accessory4_cb!=NULL));CHECK((f->x21F8!=NULL)==(e->x21F8!=NULL));
        CHECK(!memcmp(&f->coll_data,&e->coll_data,sizeof(f->coll_data)));
        if(f->x1A5C)CHECK(f->x1A5C==e->x1A5C);
    }
}
static HSD_GObjEvent phase(unsigned r,int id,unsigned which)
{
    const MotionState*m=&definitions[r]->motion_states[id-341];
    return which==0?m->anim_cb:which==1?m->input_cb:which==2?m->phys_cb:m->coll_cb;
}
#include "constructor_cases.inc"

int main(int argc,char**argv)
{
    CHECK(argc==3);NativeFighterHost host={0};
#include "paired_hosts.inc"
    host.callback_source_alive=nativeCallbackSource_Alive;host.change_motion_state=Fighter_ChangeMotionState;host.start_animation=ftAnim_8006EBA4;
    host.animation_frames_remaining=ftAnim_IsFramesRemaining;host.set_animation_rate=ftAnim_SetAnimRate;host.finish_ground_action=ft_8008A2BC;
    host.enter_fall=ftCo_Fall_Enter;host.ground_friction=ft_80084F3C;host.ground_to_air=ftCommon_8007D5D4;host.force_air_and_use_jumps=ftCommon_8007D60C;
    host.air_to_ground=ftCommon_8007D7FC;host.special_ground_collision=ft_800827A0;host.air_collision=ft_80081D0C;
    host.ground_box_collision=ft_80082888;host.air_box_collision=ft_800824A0;host.enter_freefall=ftCo_80096900;
    host.set_root_rotation_x=ftPartSetRotX;host.set_root_rotation_y=ftPartSetRotY;host.get_bone_rotation_x=ftPartGetRotX;host.joint_position=lb_8000B1CC;
    host.resolve_bone=resolve_bone;host.bone_count=bone_count;host.pause_effects=efLib_PauseAll;host.resume_effects=efLib_ResumeAll;
    host.family_member_id=member_id;host.link_hitlag=nativeFighterRelations_Link;host.linked_hitlag_id=nativeFighterRelations_LinkedId;
    host.request_hitlag_release=Fighter_UnkSetFlag_8006CFBC;host.copy_collision_from=nativeFighterRelations_CopyCollision;
    host.target_bone_position=nativeFighterTarget_BonePosition;host.target_motion=target_motion;host.target_airborne=target_airborne;
    host.target_call=target_void;host.target_call_bool=target_bool;host.target_call_float=target_float;
    host.fall=ftCommon_Fall;host.fall_basic=ftCommon_FallBasic;host.air_decel_normal=ftCommon_CalcSelfAccel_DeaccelAir;
    host.air_accel=ftCommon_CalcSelfAccel_DriftSimple;host.ground_decelerate=ftCommon_CalcGroundAccel_Deaccel;
    host.align_velocity_to_facing=ftCommon_8007F76C;host.check_ground_and_ledge=ft_CheckGroundAndLedge;host.check_cliff_catch=ftCliffCommon_80081298;host.enter_landing_fall_special=ftCo_LandingFallSpecial_Enter;host.sqrt_float=sqrtf;
    host.clamp_ground_vel=ftCommon_ClampGroundVel;host.clamp_self_vel_x=ftCommon_ClampSelfVelX;
    host.ground_to_self_movement=ftCommon_SetSelfMovementFromGroundedMovement_NoFriction;host.abs_float=fabsf;host.sin_float=sinf;host.cos_float=cosf;
    for(unsigned r=0;r<2;++r){
        HMODULE module=LoadLibraryA(argv[r+1]);CHECK(module);
        const NativeFighterDefinition*(*get)(const NativeFighterDefinition*,const NativeFighterHost*)=(void*)GetProcAddress(module,"FreakMelee_GetFighterModule");CHECK(get);
        bool(*set)(const uint32_t*,uint32_t,uint64_t)=(void*)GetProcAddress(module,"FreakMelee_SetFighterParameters");CHECK(set);
        CHECK(set(parameter_bits[r],parameter_count[r],parameter_schema[r]));
        static MotionState motions[2][26];NativeFighterDefinition base={.fighter_root=r?"ftDataNana":"ftDataPopo",.source_kind=r?Ft_Kind_Nana:Ft_Kind_Popo,
            .motion_state_first=341,.motion_state_count=26,.motion_states=motions[r]};
        definitions[r]=get(&base,&host);CHECK(definitions[r]);
    }
#include "popo/attributes.inc"
#include "nana/attributes.inc"
    constructor_cases(&host);
    printf("PASS %u original/generated pairs, %u checks, %u authored safety cases\n",pairs,checks,safety);return 0;
}
