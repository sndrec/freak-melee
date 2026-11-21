#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/fighter.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftanim_track_parts.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/native_source_package.h>
#include <melee/ft/native_fighter_rig.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_costume_runtime.h>
#include <melee/ft/ftanim.h>
#include <melee/lb/lbanim.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
// Actual complete resolvers refer to their real private slot fields. Other
// package loading/scene registration is controlled; no alternate resolver.
#define NATIVE_FIGHTER_ACTOR_CAPACITY (4 * NATIVE_FIGHTER_FAMILY_MEMBERS)
typedef struct NativeFighterSlot {
    bool active; const NativeSourcePackage* source; UnkCostumeStruct costume_entry;
} NativeFighterSlot;
static NativeSourcePackage package;
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind kind)
{return package.ready&&kind==package.target?&package:NULL;}

static HSD_GObj* heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
ftData* gFtDataList[Ft_Kind_RuntimeCapacity];
bool nativeModelEffect_Snapshot(HSD_GObj* o,uint32_t* g) {(void)o;(void)g;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* o,uint32_t g) {(void)o;(void)g;return NULL;}
static NativeFighterSlot slot_storage[NATIVE_FIGHTER_ACTOR_CAPACITY], *slots=slot_storage;
static const UnkCostumeStruct* selected_cosmetic;
const UnkCostumeStruct* nativeCostumeRuntime_ForFighter(const Fighter* fighter)
{ (void)fighter;return selected_cosmetic; }
#include "runtime.inc"

static unsigned checks, calls;
#define CHECK(c) do { ++checks; assert(c); } while(0)
// Lower SRT/animation resource boundaries record call family and operands.
static unsigned family[64]; static float weight_a[64],weight_b[64];
static HSD_JObj* target_joint[64];static HSD_Joint* source_joint[64];
static void call(unsigned kind,float a,float b) {CHECK(calls<64);family[calls]=kind;weight_a[calls]=a;weight_b[calls++]=b;}
void lbAnim_8001E6D8(HSD_JObj* j,FigaTree* t,FigaTrack* tracks,s8 n) {CHECK(j&&t);(void)tracks;call(1,(float)n,0);}
void lb_8000B760(HSD_JObj* j,HSD_Joint* d) {CHECK(j&&d);call(2,0,0);target_joint[calls-1]=j;source_joint[calls-1]=d;}
void lb_8000B5DC(HSD_JObj* j,HSD_Joint* d) {CHECK(j&&d);call(3,0,0);target_joint[calls-1]=j;source_joint[calls-1]=d;}
void lb_8000B6A4(HSD_JObj* j,HSD_Joint* d) {CHECK(j&&d);call(4,0,0);target_joint[calls-1]=j;source_joint[calls-1]=d;}
void lb_8000B4FC(HSD_JObj* j,HSD_Joint* d) {CHECK(j&&d);call(5,0,0);target_joint[calls-1]=j;source_joint[calls-1]=d;}
void ftCommon_8007F6A4(Fighter* f,HSD_JObj* j) {CHECK(f&&j);call(6,0,0);target_joint[calls-1]=j;source_joint[calls-1]=NULL;}
void lb_8000C868(HSD_Joint* d,HSD_JObj* a,HSD_JObj* b,float x,float y) {CHECK(d&&a&&b);call(7,x,y);target_joint[calls-1]=a;source_joint[calls-1]=d;}
void lb_8000C490(HSD_JObj* d,HSD_JObj* a,HSD_JObj* b,float x,float y) {CHECK(d&&a&&b);call(8,x,y);}
void lbCopyJObjSRT(HSD_JObj* d,HSD_JObj* a) {CHECK(d&&a);call(9,0,0);}
void OSReport(char* format,...) {(void)format;assert(0);}
void __assert(char* file,u32 line,char* condition) {(void)file;(void)line;(void)condition;abort();}
void HSD_JObjClearFlags(HSD_JObj* joint,u32 flags) {CHECK(joint);joint->flags &= ~flags;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) {(void)joint;}
void HSD_Panic(char* f,u32 l,char* fmt) {(void)f;(void)l;(void)fmt;abort();}
// Actual iterator uses this fixed engine stack; its body is extracted too.
#include "shared.inc"

typedef struct DescriptorArena {HSD_DatImportRecord records[8];HSD_Joint nodes[6];struct Fighter_WaitAnimData animations[1];} DescriptorArena;
static DescriptorArena char_arena, costume_arena;
static HSD_JObj live[6], blend[6];static FighterBone parts[6];
static struct Fighter_804D6540_t omission;static struct Fighter_804D6540_x0_t omission_row;
static uint8_t raw_animation[32];
typedef struct AnimationArena {FigaTree clip;FigaTrack tracks[8];} AnimationArena;
static void* animation_storage;
#define clip_arena (*(AnimationArena*)animation_storage)

static NativeSourcePoseGraph graphs[]={{.name=1,.slot=4,.path="0"},{.name=2,.slot=5,.path=""}};
static Fighter* fighter;static HSD_GObj object;

static void descriptors_init(DescriptorArena* arena,HSD_DatImport* import) {
    memset(arena,0,sizeof(*arena));
    *import=(HSD_DatImport){.arena=(u8*)arena,.arena_size=sizeof(*arena),.arena_used=sizeof(*arena),.records=arena->records,.record_capacity=8,.record_count=6};
    for(unsigned i=0;i<6;++i) {arena->records[i]=(HSD_DatImportRecord){.type=DAT_JOINT,.object=&arena->nodes[i]};
        if(i<5)arena->nodes[i].child=&arena->nodes[i+1];arena->nodes[i].scale=(Vec3){1,1,1};}
}
static void reset(void) {
    memset(slots,0,sizeof(slot_storage));memset(&package.assets,0,sizeof(package.assets));memset(fighter,0,sizeof(*fighter));selected_cosmetic=NULL;
    memset(live,0,sizeof(live));memset(blend,0,sizeof(blend));memset(parts,0,sizeof(parts));
    NativeFighterSlot* slot=&slots[0];
    descriptors_init(&char_arena,&package.assets.character.import);descriptors_init(&costume_arena,&package.assets.costume.import);
    package.assets.character.joint_graphs[4]=package.assets.character.fighter_items[4]=&char_arena.nodes[0];
    package.assets.character.joint_graphs[5]=package.assets.character.fighter_items[5]=&char_arena.nodes[0];
    package.assets.costume.joint=&costume_arena.nodes[0];slot->costume_entry.joint=package.assets.costume.joint;slot->costume_entry.native_assets=&package.assets.costume;
    package.assets.character.data.x0=&package.assets.character.common_attributes;package.assets.character.common_attributes.model_scaling=2;
    package.assets.character.data.x8=&package.assets.character.model_data;package.assets.character.model_data.x10=1;
    package.assets.character.data.xC=char_arena.animations;
    slot->active=true;package.target=Ft_Kind_CustomFirst;slot->source=&package;package.ready=package.assets.ready=true;
    package.assets.facts.animation_tag=Ft_Kind_CustomFirst;package.assets.facts.actions=1;package.assets.facts.pose_count=2;package.assets.facts.layout.joint_graph_mask=0x30;
    memcpy(package.assets.facts.poses,graphs,sizeof(graphs));package.assets.parts.parts_num=6;
    package.assets.animation_arena=&clip_arena;package.owned_input.assets.animation=raw_animation;package.owned_input.assets.animation_bytes=sizeof(raw_animation);
    memset(&clip_arena,0,sizeof(clip_arena));memset(raw_animation,0,sizeof(raw_animation));
    raw_animation[0]=1;raw_animation[1]=1;raw_animation[2]=255;
    clip_arena.clip=(FigaTree){.frames=10,.nodes=(s8*)raw_animation,.tracks=clip_arena.tracks};
    for(unsigned i=0;i<2;++i)clip_arena.tracks[i]=(FigaTrack){.length=1,.ad_head=raw_animation+20+i};
    char_arena.animations[0].x14=&clip_arena.clip;
    fighter->kind=Ft_Kind_CustomFirst;fighter->x8_spawnNum=UINT32_C(0xFEDCBA98);fighter->parts=parts;
    fighter->ft_data=&package.assets.character.data;fighter->x24=char_arena.animations;fighter->x58C=1;
    fighter->x108_costume_joint=package.assets.costume.joint;fighter->x8AC_animSkeleton=&blend[0];
    object=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data=fighter,.hsd_obj=&live[0]};
    HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=&object;
    ftPartsTable[fighter->kind]=&package.assets.parts;gFtDataList[fighter->kind]=fighter->ft_data;
    omission=(struct Fighter_804D6540_t){0};Fighter_804D6540[fighter->kind]=&omission;
    for(unsigned i=0;i<6;++i) {parts[i].joint=&live[i];parts[i].x4_jobj2=&blend[i];parts[i].flags_b1=true;
        if(i<5){live[i].child=&live[i+1];blend[i].child=&blend[i+1];live[i+1].parent=&live[i];blend[i+1].parent=&blend[i];}}
    calls=0;
}
static bool valid(NativeFighterPoseOperation op,int dest,int start,HSD_Joint* graph,FigaTree* clip)
{return nativeFighterPose_Validate(&object,op,dest,start,graph,clip);}
static void mapping(void)
{
    reset();NativeFighterRig rig={.count=6,.next_id=5001};
    for(unsigned i=0;i<MAX_FT_PARTS;++i)rig.ids[i]=i;
    rig.ids[1]=5000;rig.ids[139]=1;CHECK(nativeFighterRig_Index(&rig,NULL,0));
    const int32_t ids[]={5000,2,3,4,5};NativeFighterPoseBinding binding;
    CHECK(nativeFighterPose_BindGraph(&package.assets.character,4,"0",&rig,ids,5,&binding));
    CHECK(binding.count==5&&binding.parts[0]==1&&binding.descriptors[4]==&char_arena.nodes[5]);
    // Compare every native pose-operation branch, both destinations and
    // independent blend weights, against the actual extracted stock bodies.
    for(unsigned pattern=0;pattern<4096;++pattern)for(int destination=0;destination<2;++destination)
    for(unsigned blending=0;blending<2;++blending){
        for(unsigned i=1;i<6;++i){unsigned flags=pattern>>((i%4)*3);
            parts[i].flags_b0=(flags&1)!=0;parts[i].flags_b4=(flags&2)!=0;parts[i].flags_b5=(flags&4)!=0;
            live[i].flags=blend[i].flags=JOBJ_USE_QUATERNION;
        }
        const float a=(float)(pattern%17)*0.125f-0.5f,b=1.75f-a;
        calls=0;
        if(blending){if(destination)ftAnim_80070108(fighter,1,a,b,&char_arena.nodes[1]);else ftAnim_80070010(fighter,1,a,b,&char_arena.nodes[1]);}
        else {if(destination)ftAnim_8006FB88(fighter,1,&char_arena.nodes[1]);else ftAnim_8006FA58(fighter,1,&char_arena.nodes[1]);}
        unsigned old_count=calls,old_family[64],flags[6];float old_a[64],old_b[64];
        HSD_JObj* old_targets[64];HSD_Joint* old_sources[64];
        memcpy(old_family,family,calls*sizeof(*family));memcpy(old_a,weight_a,calls*sizeof(float));memcpy(old_b,weight_b,calls*sizeof(float));
        memcpy(old_targets,target_joint,calls*sizeof(*target_joint));memcpy(old_sources,source_joint,calls*sizeof(*source_joint));
        for(unsigned i=1;i<6;++i){flags[i]=(destination?blend[i].flags:live[i].flags);live[i].flags=blend[i].flags=JOBJ_USE_QUATERNION;}
        calls=0;CHECK(nativeFighterPose_ApplyMapped(fighter,destination,&binding,blending,a,b));
        CHECK(calls==old_count&&!memcmp(old_family,family,calls*sizeof(*family))&&
            !memcmp(old_a,weight_a,calls*sizeof(float))&&!memcmp(old_b,weight_b,calls*sizeof(float))&&
            !memcmp(old_targets,target_joint,calls*sizeof(*target_joint))&&!memcmp(old_sources,source_joint,calls*sizeof(*source_joint)));
        for(unsigned i=1;i<6;++i)CHECK(flags[i]==(destination?blend[i].flags:live[i].flags));
    }
    NativeFighterPoseBinding previous=binding;int32_t duplicate[]={5000,2,3,4,4};
    CHECK(!nativeFighterPose_BindGraph(&package.assets.character,4,"0",&rig,duplicate,5,&binding));
    CHECK(!memcmp(&binding,&previous,sizeof(binding)));
    CHECK(!nativeFighterPose_BindGraph(&package.assets.character,4,"0",&rig,ids,4,&binding));
    CHECK(!memcmp(&binding,&previous,sizeof(binding)));
    // A new physical ordering uses the same stable IDs and descriptors.
    rig.ids[2]=4;rig.ids[4]=2;CHECK(nativeFighterRig_Index(&rig,NULL,0));
    CHECK(nativeFighterPose_BindGraph(&package.assets.character,4,"0",&rig,ids,5,&binding));
    CHECK(binding.parts[1]==4&&binding.parts[3]==2);
    for(unsigned i=0;i<6;++i){parts[i].flags_b0=false;parts[i].flags_b4=true;parts[i].flags_b5=false;}
    package.assets.character.model_data.x10=0;calls=0;
    CHECK(nativeFighterPose_ApplyMapped(fighter,0,&binding,false,0,0)&&calls==5);
    for(unsigned i=0;i<5;++i)CHECK(target_joint[i]==&live[binding.parts[i]]&&source_joint[i]==binding.descriptors[i]);
    // A bad later destination must not partially write earlier bones.
    calls=0;binding.parts[4]=6;CHECK(!nativeFighterPose_ApplyMapped(fighter,0,&binding,false,0,0)&&!calls);
    binding.parts[4]=5;parts[5].joint=NULL;CHECK(!nativeFighterPose_ApplyMapped(fighter,0,&binding,false,0,0)&&!calls);
}
int main(void) {
    fighter=calloc(1,sizeof(*fighter));animation_storage=calloc(1,FT_ANIMATION_IMPORT_ARENA_SIZE);CHECK(fighter&&animation_storage);
    ftPartsTable=calloc(Ft_Kind_RuntimeCapacity,sizeof(*ftPartsTable));Fighter_804D6540=calloc(Ft_Kind_RuntimeCapacity,sizeof(*Fighter_804D6540));CHECK(ftPartsTable&&Fighter_804D6540);
    mapping();reset();
    const char* good[]={"","0","255","0/0/1","0/0/0/0/0/0/0/0/0/0/0/0"};
    const char* bad[]={NULL,"/","0/","/0","256","0000","-1","1.0","0//1","0/0/0/0/0/0/0/0/0/0/0/0/0"};
    for(unsigned i=0;i<sizeof(good)/sizeof(*good);++i)CHECK(nativeFighterPose_PathValid(good[i]));
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)CHECK(!nativeFighterPose_PathValid(bad[i]));
    for(unsigned destination=0;destination<2;++destination)for(unsigned flags=0;flags<8;++flags) {
        reset();parts[2].flags_b0=flags&1;parts[2].flags_b4=(flags>>1)&1;parts[2].flags_b5=(flags>>2)&1;
        CHECK(nativeFighterRuntime_PoseGraph(&object,0)==&char_arena.nodes[1]);
        HSD_Joint* graph=nativeFighterRuntime_PoseGraph(&object,0);
        CHECK(valid(NativeFighterPose_CopyGraph,destination,1,graph,NULL));
        if(destination)ftAnim_8006FB88(fighter,1,graph);else ftAnim_8006FA58(fighter,1,graph);
        CHECK(calls>0);calls=0;
        CHECK(valid(NativeFighterPose_BlendGraph,destination,1,graph,NULL));
        if(destination)ftAnim_80070108(fighter,1,0.25f,0.9f,graph);else ftAnim_80070010(fighter,1,0.25f,0.9f,graph);
        for(unsigned i=0;i<calls;++i)if(family[i]==7)CHECK(weight_a[i]==0.25f&&weight_b[i]==0.9f);calls=0;
        CHECK(valid(NativeFighterPose_CopyCostume,destination,1,NULL,NULL));
        HSD_Joint* costume=ftAnim_8006F994(fighter,destination?parts[1].x4_jobj2:parts[1].joint,fighter->x108_costume_joint);
        CHECK(costume==&costume_arena.nodes[1]);
        CHECK(valid(NativeFighterPose_LoadAnimation,destination,0,NULL,&clip_arena.clip));
        ftAnim_8006F4C8(fighter,destination,&clip_arena.clip);CHECK(calls==2);
        CHECK(valid(NativeFighterPose_RequestFrame,destination,0,NULL,NULL));CHECK(valid(NativeFighterPose_Evaluate,destination,0,NULL,NULL));
        calls=0;CHECK(valid(NativeFighterPose_BlendToLive,0,1,NULL,NULL));ftAnim_8006FE9C(fighter,1,0.7f,0.4f);
        for(unsigned i=0;i<calls;++i)if(family[i]==8)CHECK(weight_a[i]==0.7f&&weight_b[i]==0.4f);
        calls=0;CHECK(valid(NativeFighterPose_CopyToLive,0,1,NULL,NULL));ftAnim_8006FF74(fighter,1);
    }
    reset();NativeFighterPoseAssets output;CHECK(nativeFighterRuntime_PoseAssets(&object,&output));
    CHECK(!nativeFighterRuntime_PoseAssets(NULL,&output));CHECK(!nativeFighterRuntime_PoseGraph(&object,-1));CHECK(!nativeFighterRuntime_PoseGraph(&object,2));
    reset();fighter->ft_data=NULL;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();fighter->x24=NULL;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();fighter->x58C=2;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();fighter->kind=Ft_Kind_CustomFirst+1;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=NULL;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();slot_storage[0].active=false;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();slot_storage[0].costume_entry.joint=NULL;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();fighter->parts[1].joint=NULL;CHECK(!valid(NativeFighterPose_CopyCostume,0,1,NULL,NULL));
    reset();live[5].child=&live[0];CHECK(!valid(NativeFighterPose_CopyToLive,0,0,NULL,NULL));
    reset();blend[5].child=&blend[0];CHECK(!valid(NativeFighterPose_Evaluate,1,0,NULL,NULL));
    reset();char_arena.nodes[5].child=&char_arena.nodes[0];CHECK(!nativeFighterRuntime_PoseGraph(&object,0));
    reset();char_arena.records[2].type=DAT_DOBJ;CHECK(!nativeFighterRuntime_PoseGraph(&object,0));
    reset();package.assets.character.fighter_items[4]=NULL;CHECK(!nativeFighterRuntime_PoseGraph(&object,0));
    reset();parts[5].joint=&live[4];CHECK(!valid(NativeFighterPose_CopyToLive,0,0,NULL,NULL));
    reset();CHECK(!valid(NativeFighterPose_CopyGraph,0,2,&char_arena.nodes[1],NULL));
    reset();CHECK(!valid(NativeFighterPose_CopyGraph,0,1,&costume_arena.nodes[1],NULL));
    reset();CHECK(!valid(NativeFighterPose_CopyToLive,2,0,NULL,NULL));CHECK(!valid(NativeFighterPose_CopyToLive,0,-1,NULL,NULL));
    reset();omission.x4=33;CHECK(!valid(NativeFighterPose_CopyToLive,0,0,NULL,NULL));
    reset();omission_row.x0=1;omission.x0=&omission_row;omission.x4=1;
    CHECK(!valid(NativeFighterPose_CopyGraph,0,1,&char_arena.nodes[1],NULL));
    char_arena.nodes[4].child=NULL;CHECK(valid(NativeFighterPose_CopyGraph,0,1,&char_arena.nodes[1],NULL));
    reset();parts[0].flags_b1=false;parts[1].flags_b1=false;parts[2].flags_b1=false;parts[3].flags_b1=false;parts[4].flags_b1=false;parts[5].flags_b1=false;
    CHECK(!valid(NativeFighterPose_LoadAnimation,1,0,NULL,&clip_arena.clip));
    reset();raw_animation[2]=1;CHECK(!valid(NativeFighterPose_LoadAnimation,1,0,NULL,&clip_arena.clip));
    reset();raw_animation[0]=128;CHECK(!valid(NativeFighterPose_LoadAnimation,1,0,NULL,&clip_arena.clip));
    reset();clip_arena.clip.nodes=NULL;CHECK(!valid(NativeFighterPose_LoadAnimation,1,0,NULL,&clip_arena.clip));
    reset();clip_arena.clip.frames=NAN;CHECK(!valid(NativeFighterPose_LoadAnimation,1,0,NULL,&clip_arena.clip));
    reset();clip_arena.tracks[1].ad_head=raw_animation+32;CHECK(!valid(NativeFighterPose_LoadAnimation,1,0,NULL,&clip_arena.clip));
    reset();package.assets.character.common_attributes.model_scaling=0;CHECK(!valid(NativeFighterPose_CopyCostume,0,1,NULL,NULL));
    reset();package.assets.character.model_data.x10=6;CHECK(!valid(NativeFighterPose_CopyCostume,0,1,NULL,NULL));
    reset();
    // A nested leaf has no ancestor sibling continuation when the actual copy
    // starts there. Only that leaf's target must be available.
    costume_arena.nodes[0].child=&costume_arena.nodes[1];
    costume_arena.nodes[1].child=&costume_arena.nodes[2];
    costume_arena.nodes[1].next=&costume_arena.nodes[3];
    costume_arena.nodes[2].child=NULL;
    parts[3].joint=NULL;
    CHECK(valid(NativeFighterPose_CopyCostume,0,2,NULL,NULL));
    HSD_Joint* leaf=ftAnim_8006F994(fighter,parts[2].joint,fighter->x108_costume_joint);
    CHECK(leaf==&costume_arena.nodes[2]);calls=0;ftAnim_8006FA58(fighter,2,leaf);CHECK(calls==1);
    typedef struct DeepArena {HSD_DatImportRecord records[31];HSD_Joint nodes[31];} DeepArena;
    DeepArena* deep=calloc(1,sizeof(*deep));FtCharacterAssets* deep_assets=calloc(1,sizeof(*deep_assets));CHECK(deep&&deep_assets);
    deep_assets->import=(HSD_DatImport){.arena=(u8*)deep,.arena_size=sizeof(*deep),.arena_used=sizeof(*deep),.records=deep->records,.record_count=31,.record_capacity=31};
    for(unsigned i=0;i<31;++i) {deep->records[i]=(HSD_DatImportRecord){.type=DAT_JOINT,.object=&deep->nodes[i]};if(i<30)deep->nodes[i].child=&deep->nodes[i+1];}
    deep_assets->joint_graphs[4]=deep_assets->fighter_items[4]=deep->nodes;
    CHECK(nativeFighterPose_SelectGraph(deep_assets,4,"")==NULL);
    deep->nodes[29].child=NULL;CHECK(nativeFighterPose_SelectGraph(deep_assets,4,"")==deep->nodes);
    free(deep);free(deep_assets);
    // Owned-bank resolver checks span/identity before touching the table/clip.
    reset();CHECK(nativeFighterRuntime_PoseClip(&object,0)==&clip_arena.clip);
    CHECK(nativeFighterRuntime_PoseClip(&object,-1)==NULL);CHECK(nativeFighterRuntime_PoseClip(&object,1)==NULL);
    reset();char_arena.animations[0].x14=NULL;CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();char_arena.animations[0].x14=(FigaTree*)raw_animation;CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();selected_cosmetic=&slot_storage[0].costume_entry;CHECK(nativeFighterRuntime_PoseAssets(&object,&output));
    UnkCostumeStruct cosmetic=slot_storage[0].costume_entry;
    cosmetic.native_assets=NULL;selected_cosmetic=&cosmetic;CHECK(!nativeFighterRuntime_PoseAssets(&object,&output));
    reset();gFtDataList[fighter->kind]=NULL;CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();ftPartsTable[fighter->kind]=NULL;CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();uint32_t generation=0;CHECK(nativeCallbackSource_Snapshot(&object,NativeCallbackSource_Fighter,&generation));
    CHECK(generation==UINT32_C(0xFEDCBA98));fighter->x8_spawnNum++;
    CHECK(!nativeCallbackSource_Alive(&object,generation,NativeCallbackSource_Fighter));
    // A fresh synchronous lookup accepts the new live identity; retained callers
    // must use their original generation and abort rather than reuse the result.
    CHECK(nativeFighterRuntime_PoseClip(&object,0)==&clip_arena.clip);
    void* forbidden=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);CHECK(forbidden);
    reset();fighter->x24=forbidden;CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();package.assets.character.data.xC=forbidden;fighter->x24=forbidden;
    CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();char_arena.animations[0].x14=forbidden;CHECK(nativeFighterRuntime_PoseClip(&object,0)==NULL);
    reset();HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER]=NULL;
    CHECK(nativeFighterRuntime_PoseClip(forbidden,0)==NULL);
    CHECK(VirtualFree(forbidden,0,MEM_RELEASE));
    printf("PASS %u pose validation/resolution/shared-body boundary checks; controlled lower SRT/track installation; actual descriptor iterator\n",checks);
    free(animation_storage);free(fighter);free(ftPartsTable);free(Fighter_804D6540);return 0;
}
