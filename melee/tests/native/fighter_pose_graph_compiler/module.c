#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "pose_probe.inc"

static HSD_GObj source;
static Fighter* fighter;
static FighterBone parts[2];
static HSD_JObj live_root, blend_root, live_part, blend_part;
static HSD_Joint ground, air, costume;
static unsigned char clip_bytes[32];
static FigaTree* clip=(FigaTree*)clip_bytes;
static const NativeFighterDefinition* definition;
static uint32_t generation=UINT32_C(0x80000001);
static unsigned checks, calls, validations, fail_operation, invalidate_at, invalidate_mode;
static unsigned clip_resolves, bank_state;
static bool live;
static char order[32];
static float frame, weight_a, weight_b;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static unsigned field(const char* name)
{
    for(unsigned i=0;i<definition->state_field_count;++i)
        if(!strcmp(definition->state_fields[i].name,name)) return definition->state_fields[i].offset;
    assert(false); return 0;
}
static void real(const char* name,float value)
{ memcpy(fighter->native_module_state+field(name),&value,4); }
static int continued(void)
{ int result; memcpy(&result,fighter->native_module_state+field("continued"),4); return result; }
static bool alive(HSD_GObj* object,uint32_t id,uint32_t cls)
{
    CHECK(object==&source && cls==NativeCallbackSource_Fighter && id==generation);
    return live && (uint32_t)fighter->x8_spawnNum==id;
}
static void called(char operation)
{
    CHECK(calls<sizeof(order)-1); order[calls]=operation; order[calls+1]=0;
    ++calls;
    if(calls==invalidate_at) {
        if(invalidate_mode==1) {
            live=false; DWORD old;
            CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_NOACCESS,&old));
        } else if(invalidate_mode==2) ++fighter->x8_spawnNum;
    }
}
static uint32_t hash(const char* text)
{
    uint32_t value=UINT32_C(2166136261);
    while(*text) { value^=(unsigned char)*text++; value*=UINT32_C(16777619); }
    return value;
}
static HSD_Joint* graph(Fighter_GObj* object,int32_t index)
{
    CHECK(object==&source && live && (uint32_t)fighter->x8_spawnNum==generation);
    CHECK(index>=0 && index<2);
    const NativeFighterPoseGraph* recipe=&definition->pose_graphs[index];
    if(recipe->name_hash==hash("GroundRest")) {
        CHECK(recipe->slot==4 && !strcmp(recipe->path,"0")); return &ground;
    }
    CHECK(recipe->name_hash==hash("AirRest") && recipe->slot==5 && !strcmp(recipe->path,"0/1"));
    return &air;
}
static bool validate(Fighter_GObj* object,NativeFighterPoseOperation operation,
    int32_t destination,int32_t start,HSD_Joint* descriptor,FigaTree* animation)
{
    CHECK(object==&source && live && (uint32_t)fighter->x8_spawnNum==generation);
    CHECK(destination==NativeFighterPose_Live || destination==NativeFighterPose_Blend);
    ++validations;
    if(operation==NativeFighterPose_LoadAnimation) CHECK(animation==clip && descriptor==NULL && start==0);
    else if(operation==NativeFighterPose_RequestFrame || operation==NativeFighterPose_Evaluate)
        CHECK(animation==NULL && descriptor==NULL && start==0);
    else {
        CHECK(animation==NULL && start==1);
        CHECK((operation==NativeFighterPose_CopyGraph || operation==NativeFighterPose_BlendGraph)
            ? descriptor==&ground || descriptor==&air : descriptor==NULL);
    }
    return fail_operation!=(unsigned)operation+1;
}
static FigaTree* animation(Fighter_GObj* object,int32_t id)
{
    CHECK(object==&source && live && (uint32_t)fighter->x8_spawnNum==generation);
    ++clip_resolves;
    if(bank_state || id<0 || id>=fighter->x58C) return NULL;
    CHECK(id==305 || id==308); return clip;
}
static void load(Fighter* object,bool blend,FigaTree* animation)
{ CHECK(object==fighter && animation==clip); called(blend?'B':'L'); }
static void request(HSD_JObj* root,float value)
{ CHECK(root==&live_root || root==&blend_root); CHECK(value==frame); called(root==&blend_root?'r':'R'); }
static HSD_Joint* find(Fighter* object,HSD_JObj* joint,HSD_Joint* root)
{
    CHECK(object==fighter && root==&costume && (joint==&live_part || joint==&blend_part));
    return &costume;
}
static void copy_live(Fighter* object,Fighter_Part start,HSD_Joint* descriptor)
{ CHECK(object==fighter && start==1 && (descriptor==&costume || descriptor==&air)); called(descriptor==&costume?'c':'g'); }
static void copy_blend(Fighter* object,Fighter_Part start,HSD_Joint* descriptor)
{ CHECK(object==fighter && start==1 && (descriptor==&costume || descriptor==&ground)); called(descriptor==&costume?'C':'G'); }
static void evaluate(HSD_JObj* root)
{ CHECK(root==&live_root || root==&blend_root); called(root==&blend_root?'E':'e'); }
static void blend_live(Fighter* object,Fighter_Part start,float a,float b,HSD_Joint* descriptor)
{ CHECK(object==fighter && start==1 && descriptor==&air && a==weight_a && b==weight_b); called('d'); }
static void blend_alternate(Fighter* object,Fighter_Part start,float a,float b,HSD_Joint* descriptor)
{ CHECK(object==fighter && start==1 && descriptor==&ground && a==weight_a && b==weight_b); called('A'); }
static void blend_to_live(Fighter* object,Fighter_Part start,float a,float b)
{ CHECK(object==fighter && start==1 && a==weight_a && b==weight_b); called('t'); }
static void copy_to_live(Fighter* object,Fighter_Part start)
{ CHECK(object==fighter && start==1); called('T'); }
static void reset(float next_frame,float a,float b)
{
    DWORD old; CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
    memset(fighter,0,sizeof(*fighter)); memset(parts,0,sizeof(parts));
    source.user_data=fighter; source.hsd_obj=&live_root; fighter->x8_spawnNum=generation;
    fighter->parts=parts; parts[1].joint=&live_part; parts[1].x4_jobj2=&blend_part;
    fighter->x8AC_animSkeleton=&blend_root; fighter->x108_costume_joint=&costume; fighter->x58C=400;
    real("frame",frame=next_frame); real("descriptor_weight",weight_a=a); real("existing_weight",weight_b=b);
    live=true; calls=validations=clip_resolves=bank_state=0; order[0]=0; fail_operation=invalidate_at=invalidate_mode=0;
}
int main(void)
{
    fighter=VirtualAlloc(NULL,sizeof(*fighter),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE); assert(fighter);
    MotionState motion={0}; NativeFighterDefinition base={.fighter_root="ftDataSeak",
        .motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.callback_source_alive=alive,.pose_graph=graph,.pose_validate=validate,
        .pose_clip=animation,.load_pose_animation=load,.request_pose_frame=request,
        .find_costume_pose=find,.copy_bone_pose=copy_live,.copy_blend_pose=copy_blend,
        .model_advance_animation=evaluate,.blend_graph_live=blend_live,
        .blend_graph_alternate=blend_alternate,.blend_pose_to_live=blend_to_live,.copy_pose_to_live=copy_to_live};
    definition=FreakMelee_GetFighterModule(&base,&host);
    CHECK(definition && definition->pose_graph_count==2);
    CHECK(definition->pose_graphs[0].name_hash<definition->pose_graphs[1].name_hash);
    CHECK((definition->asset_layout.joint_graph_mask&0x30)==0x30);
#define MISSING(member) do { NativeFighterHost absent=host; absent.member=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&absent)); } while(0)
    MISSING(callback_source_alive); MISSING(pose_graph); MISSING(pose_validate);
    MISSING(pose_clip); MISSING(load_pose_animation); MISSING(request_pose_frame);
    MISSING(find_costume_pose); MISSING(copy_bone_pose); MISSING(copy_blend_pose);
    MISSING(model_advance_animation); MISSING(blend_graph_live); MISSING(blend_graph_alternate);
    MISSING(blend_pose_to_live); MISSING(copy_pose_to_live);
    CHECK(FreakMelee_GetFighterModule(&base,&host));
    const float weights[][2]={{0,0},{1,0},{0,1},{0.25f,0.5f},{-2,3},{2,4}};
    for(unsigned i=0;i<sizeof(weights)/sizeof(*weights);++i) {
        reset(-1.5f,weights[i][0],weights[i][1]); script_fn_PosePolicy(&source);
        CHECK(calls==14 && validations==14 && continued()==1);
        CHECK(!strcmp(order,"BrCEAtTgLRcedG"));
    }
    for(unsigned operation=1;operation<=8;++operation) {
        reset(4,0.25f,0.75f); fail_operation=operation; script_fn_PosePolicy(&source);
        CHECK(validations==14 && calls<14 && continued()==1);
    }
    reset(NAN,0.25f,0.75f); script_fn_PosePolicy(&source);
    CHECK(calls==12 && validations==12 && continued()==1);
    reset(4,NAN,0.75f); script_fn_PosePolicy(&source);
    CHECK(calls==11 && validations==11 && continued()==1);
    reset(4,0.25f,0.75f); fighter->x58C=305; script_fn_PosePolicy(&source);
    CHECK(calls==12 && validations==12 && continued()==1);
    reset(4,0.25f,0.75f); fighter->x58C=308; script_fn_PosePolicy(&source);
    CHECK(calls==13 && validations==13 && continued()==1);
    for(unsigned mode=0;mode<=2;++mode) {
        reset(4,0.25f,0.75f); invalidate_at=1; invalidate_mode=mode;
        script_fn_NestedPose(&source);
        DWORD old; CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
        CHECK(calls==(mode?1:2) && continued()==(mode?0:2));
        CHECK(!strcmp(order,mode?"e":"ed"));
        reset(4,0.25f,0.75f); invalidate_at=1; invalidate_mode=mode;
        script_fn_NestedStart(&source);
        CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
        CHECK(calls==(mode?1:2) && continued()==(mode?0:3));
        CHECK(!strcmp(order,mode?"e":"ed"));
        reset(4,0.25f,0.75f); invalidate_at=1; invalidate_mode=mode;
        script_fn_NestedFrame(&source);
        CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
        CHECK(calls==(mode?1:2) && continued()==(mode?0:4));
        CHECK(!strcmp(order,mode?"e":"eR"));
        reset(4,0.25f,0.25f); invalidate_at=1; invalidate_mode=mode;
        script_fn_NestedBothWeights(&source);
        CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
        CHECK(calls==(mode?1:3) && continued()==(mode?0:5));
        CHECK(!strcmp(order,mode?"e":"eed"));
    }
    const int invalid_clips[]={-1,INT32_MIN,400,INT32_MAX};
    for(unsigned i=0;i<sizeof(invalid_clips)/sizeof(*invalid_clips);++i) {
        reset(4,0.25f,0.75f); script_fn_DynamicClip(&source,invalid_clips[i]);
        CHECK(calls==0 && validations==0);
        CHECK(clip_resolves==1);
    }
    // Controlled missing/replaced/poisoned bank outcomes. Real bank ownership
    // validation is tested by the engine fixture, not simulated dereferences here.
    for(unsigned bad=1;bad<=3;++bad) {
        reset(4,0.25f,0.75f); bank_state=bad; script_fn_DynamicClip(&source,305);
        CHECK(clip_resolves==1 && calls==0 && validations==0);
    }
    // RequestFrame is pure. Each remaining operation is a guarded source effect.
    const unsigned effect_steps[]={1,3,4,5,6,7,8,9,11,12,13,14};
    for(unsigned mode=1;mode<=2;++mode)
        for(unsigned i=0;i<sizeof(effect_steps)/sizeof(*effect_steps);++i) {
            reset(4,0.25f,0.75f); invalidate_at=effect_steps[i]; invalidate_mode=mode;
            script_fn_PosePolicy(&source);
            DWORD old; CHECK(VirtualProtect(fighter,sizeof(*fighter),PAGE_READWRITE,&old));
            CHECK(calls==invalidate_at && continued()==0);
        }
    CHECK(VirtualFree(fighter,0,MEM_RELEASE));
    printf("fighter pose generated module: %u checks\n",checks);
    return 0;
}
