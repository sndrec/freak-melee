#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <melee/gm/native_home.h>
#include <melee/ft/fighter.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftanim_track_parts.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftanimation_bank.h>
#include <melee/ft/ftcharacter_assets.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_pose.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_lifetime.h>
#include <melee/ft/native_source_package.h>
#include <melee/cm/camera.h>
#include <melee/cm/types.h>
#include <melee/if/ifall.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/native_workshop_gizmo.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static struct {
    bool authoring, preview_active, preview_live_test, preview_transitioning;
    bool workshop_camera_active, workshop_camera_owned, workshop_camera_retain;
    bool workshop_hud_hidden, workshop_hud_previously_hidden;
    bool workshop_background_saved, workshop_preview_hitboxes;
    bool workshop_draft_applied;
    const NativeFighterProject* workshop_pose_project;
    GXColor workshop_previous_background;
    Vec3 workshop_camera_target;
    float workshop_camera_yaw, workshop_camera_pitch, workshop_camera_distance;
    Fighter_GObj* workshop_pose_gobj;
    uint32_t workshop_pose_generation;
    FighterKind workshop_pose_kind;
    ftData* workshop_pose_data;
    const NativeSourcePackage* workshop_pose_source;
    NativeFighterLifetime workshop_pose_lifetime;
    NativeFighterLifetime workshop_companion_lifetime;
    NativeMatchPoolReference workshop_companion_procs[32];
    unsigned workshop_companion_proc_count, workshop_member;
    NativeFighterAnimationClip workshop_pose_clip;
    FigaTree* workshop_pose_tree;
    const FigaTree* workshop_demo_source_tree;
    const struct FtCostumeAssets* workshop_pose_costume;
    HSD_JObj* workshop_pose_root;
    FighterBone* workshop_pose_parts;
    float workshop_pose_frame;
} home;
static struct {
    FigaTree tree;
    FigaTrack tracks[FT_ANIMATION_SCRATCH_TRACK_CAPACITY];
    uint8_t bytes[FT_ANIMATION_IMPORT_ARENA_SIZE];
    const Fighter* loading_fighter;
    int loading_animation;
} workshop_draft;
bool nativeFighterAnimation_PreviewTree(const NativeFighterProject* project, NativeFighterAnimationClip clip,
    void* bytes, size_t size, FigaTree* tree, FigaTrack* tracks, size_t capacity)
{ CHECK(false); return false; } // These previews use the installed fighter.
void ftData_80085CD8(Fighter* fighter, Fighter* source, enum_t animation)
{ CHECK(false); } // Reloading a draft clip requires an edited project.
void ftColl_8007AFF8(Fighter_GObj* actor)
{
    Fighter* fighter = actor->user_data;
    for (unsigned i = 0; i < ARRAY_SIZE(fighter->x914); ++i)
        fighter->x914[i].state = HitCapsule_Disabled;
}
static Fighter_GObj objects[2], *selected;
static Fighter fighters[2];
static FighterBone bones[2][3];
static HSD_JObj joints[2][3];
// Registry/publication/storage lookup is controlled; retention and pool operations
// use the production NativeFighterLifetime header and native_match_pool.c.
static NativeSourcePackage packages[2], replacement_package;
static const NativeSourcePackage* registered[2];
static NativeScriptFighterProgram published[2];
static NativeScriptStorageInstance storage[2], replacement_storage;
static bool attached[2];
static NativeScriptVMStatus status[2];
NativeMatchPool nativeGObjPool;
NativeMatchPool nativeGObjProcPool;
static HSD_GObjProc processes[4];
static NativeMatchPoolSlot process_slots[4];
static uint32_t process_order[4];
static NativeMatchPoolReference process_refs[4];
static bool paired;
static NativeMatchPoolSlot object_slots[2];
static uint32_t object_free_order[2];
static NativeMatchPoolReference object_references[2];
static uint64_t scene_epoch;
static unsigned mutation;
enum { NoMutation, SourceReplacement, StorageDead, StorageReplacement,
    ProgramIdentity, BankInvalid, Detached, PoolReused, SceneRetired, Faulted,
    ParameterIdentity, LayoutIdentity, UserdataChanged, MutationCount };
static FtCharacterAssets characters[2];
static FtCostumeAssets costumes[2];
static struct {
    struct Fighter_WaitAnimData action[2], demo[2];
} tables[2];
static struct {
    FigaTree action, demo, replacement;
    FigaTrack tracks[3];
} arenas[2];
static s8 bytes[2][64];
static bool live[2], rig_valid = true;
static bool retire_motion, retire_evaluate;
static unsigned motion_mutation, evaluation_mutation;
static unsigned motion_calls, attach_calls, request_calls, evaluate_calls;
static unsigned validation_calls, resume_calls, remove_calls;
static FigaTree* last_tree;
static HSD_JObj* last_joint;
static float requested_frame, requested_rate;
static FighterPartsTable part_tables[2];
static FighterPartsTable* table_ptrs[64];
FighterPartsTable** ftPartsTable = table_ptrs;
CameraDebugMode cm_80453004;

static unsigned actor(Fighter_GObj* g) { CHECK(g == objects || g == objects + 1); return (unsigned)(g - objects); }
bool nativeHome_IsPractice(void) { return true; }
static Fighter_GObj* workshop_preview_actor(bool dormant) { (void)dormant; return selected; }
uint32_t nativeFighterRuntime_MemberId(Fighter_GObj* source, unsigned member) {
    (void)source; return paired && member < 2 ? 100 + member : 0;
}
Fighter_GObj* ftLib_8008741C(uint32_t id) { return id >= 100 && id < 102 ? objects + id - 100 : NULL; }
static bool workshop_motion_for_action(Fighter* f, unsigned a, FtMotionId* m) {
    (void)f; if (a >= 2) return false; *m = (FtMotionId)(42 + a); return true;
}
static void workshop_camera_apply(void) {}
static void workshop_preview_presentation_begin(void) {}
bool nativeCallbackSource_Snapshot(HSD_GObj* g, uint32_t c, uint32_t* n) {
    CHECK(c == NativeCallbackSource_Fighter);
    if (!g || (g != objects && g != objects + 1) || !live[g - objects]) return false;
    *n = fighters[g - objects].x8_spawnNum; return true;
}
bool nativeCallbackSource_Alive(HSD_GObj* g, uint32_t n, uint32_t c) {
    uint32_t actual; return nativeCallbackSource_Snapshot(g, c, &actual) && n == actual;
}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind k) {
    (void)k; return NULL;
}
const NativeSourcePackage* nativeFighterRuntime_SourcePackage(FighterKind k) {
    return (unsigned)k >= 40 && (unsigned)k < 42 ? registered[k-40] : NULL;
}
const NativeScriptFighterProgram* nativeScriptFighter_Program(FighterKind k) {
    return (unsigned)k >= 40 && (unsigned)k < 42 ? published+k-40 : NULL;
}
bool nativeScriptFighter_IsAttached(Fighter_GObj* g) { return attached[actor(g)]; }
NativeScriptVMStatus nativeScriptFighter_Status(Fighter_GObj* g) { return status[actor(g)]; }
bool nativeScriptFighter_View(Fighter_GObj* g, uint32_t generation, NativeScriptReceiverView* v) {
    unsigned i=actor(g);
    if (!attached[i] || generation!=fighters[i].x8_spawnNum) return false;
    *v=(NativeScriptReceiverView){&packages[i].compiler.bank.declarations,
        mutation==StorageReplacement && i==0 ? &replacement_storage : storage+i};
    return true;
}
static void change_retained(unsigned change) {
    mutation=change;
    switch(change) {
    case SourceReplacement: registered[0]=&replacement_package; break;
    case StorageDead: storage[0].live=false; break;
    case StorageReplacement: replacement_storage=storage[0]; break;
    case ProgramIdentity: ++published[0].identity; break;
    case BankInvalid: packages[0].compiler.bank.valid=false; break;
    case Detached: attached[0]=false; break;
    case PoolReused:
        CHECK(nativeMatchPool_Free(&nativeGObjPool,object_references[0]));
        CHECK(nativeMatchPool_Allocate(&nativeGObjPool,object_references)==objects);
        // Same address, callback generation and userdata: only the real pool
        // reference distinguishes this replacement from the retained object.
        objects[0].classifier=HSD_GOBJ_CLASS_FIGHTER;
        objects[0].user_data=fighters; objects[0].hsd_obj=joints[0]; break;
    case SceneRetired: ++nativeGObjPool.lifetime; break;
    case Faulted: status[0]=NativeScriptVM_Error; break;
    case ParameterIdentity: ++packages[0].compiler.bank.storage_parameters.identity; break;
    case LayoutIdentity: ++packages[0].compiler.bank.storage_layout.identity; break;
    case UserdataChanged: objects[0].user_data=fighters+1; break;
    default: CHECK(false);
    }
}
bool nativeFighterRuntime_PoseAssets(Fighter_GObj* g, NativeFighterPoseAssets* a) {
    unsigned i = actor(g); Fighter* f = g->user_data;
    if (!live[i] || f != fighters + i || f->kind != (FighterKind)(40+i) ||
        f->ft_data != &characters[i].data) return false;
    *a = (NativeFighterPoseAssets){.character=characters+i,.costume=costumes+i,
            .action_count=2,.demo_count=2,.pose_graph_count=0,
        .animation_bytes=(const uint8_t*)bytes[i],
        .animation_size=sizeof(bytes[i]),.animation_arena=arenas+i,
        .animation_arena_size=sizeof(arenas[i])}; return true;
}
FigaTree* nativeFighterRuntime_PoseClip(Fighter_GObj* g, int32_t index) {
    unsigned i = actor(g); return (unsigned)index < 2 ? tables[i].action[index].x14 : NULL;
}
bool nativeFighterPose_Validate(Fighter_GObj* g, NativeFighterPoseOperation o,
    int32_t d, int32_t s, HSD_Joint* graph, FigaTree* clip) {
    (void)g; CHECK(o==NativeFighterPose_CopyCostume && d==0 && s==0 && !graph && !clip);
    ++validation_calls; return rig_valid;
}
u32 ftParts_8007506C(FighterKind k, int part) { (void)k; (void)part; return 0; }
void lbAnim_8001E6D8(HSD_JObj* joint, FigaTree* tree, FigaTrack* track, s8 count) {
    CHECK(count == 1 && track >= arenas[0].tracks); ++attach_calls; last_tree=tree; last_joint=joint;
}
void ftAnim_80070758(HSD_JObj* j) { CHECK(j != NULL); ++remove_calls; }
void ftAnim_8006FA58(Fighter* f, Fighter_Part p, HSD_Joint* j) { CHECK(f && p==0 && j); }
void ftAnim_80070710(HSD_JObj* j, float frame) { CHECK(j); ++request_calls; requested_frame=frame; }
void HSD_JObjAnimAll(HSD_JObj* j) { CHECK(j); ++evaluate_calls; if(retire_evaluate)live[actor(home.workshop_pose_gobj)]=false;
    if(evaluation_mutation)change_retained(evaluation_mutation); }
void HSD_ForeachAnim(void* root, HSD_Type type, HSD_TypeMask mask, void* func, AObj_Arg_Type argc, ...) {
    CHECK(root && type==JOBJ_TYPE && mask==1 && func==HSD_AObjSetRate && argc==AOBJ_ARG_AF);
    va_list ap; va_start(ap,argc); requested_rate=(float)va_arg(ap,double); va_end(ap);
}
void HSD_AObjSetRate(HSD_AObj* a, float rate) { (void)a; (void)rate; }
void HSD_GObj_80390C5C(HSD_GObj* g) { CHECK(live[actor(g)]); for(HSD_GObjProc* p=g->proc;p;p=p->child)p->flags_1=1; }
void HSD_GObj_80390C84(HSD_GObj* g) { CHECK(live[actor(g)]); ++resume_calls; for(HSD_GObjProc* p=g->proc;p;p=p->child)p->flags_1=0; }
void Fighter_ChangeMotionState(Fighter_GObj* g, FtMotionId motion, MotionFlags flags,
    float frame, float speed, float blend, HSD_GObj* other) {
    CHECK(motion==42 || motion==43); CHECK(flags==(Ft_MF_SkipAnimVel|Ft_MF_SkipAttackCount|Ft_MF_SkipRumble));
    CHECK(frame>=0 && speed==0 && blend==0 && !other); ++motion_calls;
    if(retire_motion)live[actor(g)]=false;
    if(motion_mutation)change_retained(motion_mutation);
}
HSD_GObj* Camera_80030A50(void) { return NULL; }
bool Camera_80030178(void) { return false; }
void Camera_8003006C(void) {}
void Camera_800300F0(void) {}
void Camera_SetBackgroundColor(u8 r,u8 g,u8 b) { (void)r;(void)g;(void)b; }
void ifAll_ShowHUD(void) {}
void nativeModelView_Clear(void) {}
void nativeWorkshopGizmo_Clear(void) {}

#include "bodies.inc"

static void setup(void) {
    memset(&home,0,sizeof(home)); memset(objects,0,sizeof(objects));
    memset(fighters,0,sizeof(fighters)); memset(bones,0,sizeof(bones));
    memset(tables,0,sizeof(tables)); memset(arenas,0,sizeof(arenas));
    memset(packages,0,sizeof(packages)); memset(published,0,sizeof(published));
    memset(storage,0,sizeof(storage)); mutation=motion_mutation=evaluation_mutation=0;
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,objects,sizeof(objects),object_slots,
        object_free_order,2,sizeof(objects[0]),_Alignof(Fighter_GObj),++scene_epoch));
    memset(processes,0,sizeof(processes)); paired=false;
    CHECK(nativeMatchPool_Bind(&nativeGObjProcPool,processes,sizeof(processes),process_slots,
        process_order,4,sizeof(processes[0]),_Alignof(HSD_GObjProc),scene_epoch));
    for(unsigned i=0;i<4;++i) {
        CHECK(nativeMatchPool_Allocate(&nativeGObjProcPool,process_refs+i)==processes+i);
        processes[i].gobj=objects+i/2;
        if(!(i&1)) processes[i].child=processes+i+1;
    }
    home.authoring=home.preview_active=true; selected=objects;
    rig_valid=true; retire_motion=retire_evaluate=false;
    motion_calls=attach_calls=request_calls=evaluate_calls=validation_calls=resume_calls=remove_calls=0;
    for(unsigned i=0;i<2;++i){
        CHECK(nativeMatchPool_Allocate(&nativeGObjPool,object_references+i)==objects+i);
        live[i]=attached[i]=true; status[i]=NativeScriptVM_Ok;
        objects[i].classifier=HSD_GOBJ_CLASS_FIGHTER;
        objects[i].user_data=fighters+i; objects[i].hsd_obj=joints[i];
        objects[i].proc=processes+i*2;
        fighters[i].gobj=objects+i; registered[i]=packages+i;
        packages[i].ready=true; packages[i].target=(FighterKind)(40+i);
        packages[i].compiler.bank.valid=true;
        packages[i].compiler.bank.storage_layout.identity=200+i;
        packages[i].compiler.bank.storage_parameters.identity=300+i;
        packages[i].program.bank=&packages[i].compiler.bank;
        packages[i].program.identity=400+i;
        published[i]=packages[i].program; // Published program is a copy, not package.program.
        storage[i]=(NativeScriptStorageInstance){.live=true,.generation=100+i,
            .layout=&packages[i].compiler.bank.storage_layout,
            .parameters=&packages[i].compiler.bank.storage_parameters};
        fighters[i].kind=(FighterKind)(40+i); fighters[i].x8_spawnNum=100+i;
        fighters[i].ft_data=&characters[i].data; fighters[i].parts=bones[i];
        fighters[i].x108_costume_joint=(HSD_Joint*)&costumes[i];
        fighters[i].anim_id=0; fighters[i].cur_anim_frame=3; fighters[i].frame_speed_mul=.5f;
        characters[i].data.xC=tables[i].action; characters[i].data.x14=tables[i].demo;
        characters[i].import.arena=(u8*)(tables+i); characters[i].import.arena_used=sizeof(tables[i]);
        part_tables[i].parts_num=3; table_ptrs[40+i]=part_tables+i;
        bytes[i][0]=1; bytes[i][1]=-1; bytes[i][2]=1; bytes[i][3]=-1;
        arenas[i].action=(FigaTree){.frames=20,.nodes=bytes[i],.tracks=arenas[i].tracks};
        arenas[i].demo=(FigaTree){.frames=1500,.nodes=bytes[i]+2,.tracks=arenas[i].tracks+1};
        arenas[i].replacement=arenas[i].demo;
        for(unsigned j=0;j<3;++j){arenas[i].tracks[j].ad_head=(u8*)bytes[i]+8;arenas[i].tracks[j].length=1;}
        tables[i].action[0].x14=&arenas[i].action; tables[i].demo[0].x14=&arenas[i].demo;
        fighters[i].x590=&arenas[i].action; bones[i][0].flags_b1=true;
        bones[i][0].joint=joints[i]; bones[i][0].xC=0;
    }
}
int main(void) {
    NativeFighterAnimationClip action={NativeFighterAnimation_Action,0},demo={NativeFighterAnimation_Demo,0};
    setup(); paired=true; processes[3].flags_1=1;
    CHECK(nativeHome_WorkshopPose(action,1,NULL));
    CHECK(nativeHome_WorkshopFighterVisible(objects) && !nativeHome_WorkshopFighterVisible(objects+1));
    CHECK(processes[2].flags_1 && processes[3].flags_1);
    nativeHome_WorkshopPoseRelease(true);
    CHECK(!processes[2].flags_1 && processes[3].flags_1);
    setup(); paired=true; CHECK(nativeHome_WorkshopPose(action,1,NULL));
    nativeHome_WorkshopPoseRelease(false);
    CHECK(!resume_calls && home.workshop_pose_gobj==NULL);
    for(unsigned i=0;i<4;++i)CHECK(processes[i].flags_1);
    setup(); paired=true; CHECK(nativeHome_WorkshopPose(action,1,NULL));
    selected=objects+1; home.workshop_member=1;
    CHECK(nativeHome_WorkshopPose(action,1,NULL));
    CHECK(!nativeHome_WorkshopFighterVisible(objects) && nativeHome_WorkshopFighterVisible(objects+1));
    for(unsigned i=0;i<4;++i)CHECK(processes[i].flags_1);
    home.preview_live_test=true;
    CHECK(nativeHome_WorkshopFighterVisible(objects) && nativeHome_WorkshopFighterVisible(objects+1));
    nativeHome_WorkshopPoseRelease(true);
    for(unsigned i=0;i<4;++i)CHECK(!processes[i].flags_1);
    setup(); paired=true; CHECK(nativeHome_WorkshopPose(action,1,NULL));
    CHECK(nativeMatchPool_Free(&nativeGObjProcPool,process_refs[2]));
    CHECK(nativeMatchPool_Allocate(&nativeGObjProcPool,process_refs+2)==processes+2);
    processes[2].gobj=objects+1; processes[2].flags_1=1;
    nativeHome_WorkshopPoseRelease(true);
    CHECK(processes[2].flags_1 && !processes[3].flags_1);
    setup(); paired=true; CHECK(nativeHome_WorkshopPose(action,1,NULL));
    storage[1].live=false; nativeHome_WorkshopPoseRelease(true);
    CHECK(processes[2].flags_1 && processes[3].flags_1);
    setup(); CHECK(nativeHome_WorkshopPose(action,1,NULL)); CHECK(motion_calls==1 && !attach_calls);
    CHECK(nativeHome_WorkshopPose(action,1,NULL)); CHECK(motion_calls==1);
    CHECK(nativeHome_WorkshopPose(demo,4,NULL)); CHECK(motion_calls==1 && last_tree==&arenas[0].demo && requested_rate==0);
    CHECK(fighters[0].x590==&arenas[0].action && fighters[0].anim_id==0 && fighters[0].cur_anim_frame==3);
    unsigned validations=validation_calls, attachments=attach_calls;
    CHECK(nativeHome_WorkshopPose(demo,5,NULL)); CHECK(validation_calls==validations && attach_calls==attachments && requested_frame==5);
    CHECK(nativeHome_WorkshopPose(demo,1200,NULL)); CHECK(!nativeHome_WorkshopPose(demo,1501,NULL));
    nativeHome_WorkshopPoseInvalidate(); CHECK(nativeHome_WorkshopPose(demo,5,NULL)); CHECK(attach_calls>attachments);
    tables[0].demo[0].x14=&arenas[0].replacement;
    CHECK(nativeHome_WorkshopPose(demo,5,NULL)); CHECK(last_tree==&arenas[0].replacement);
    nativeHome_WorkshopPoseRelease(true); CHECK(last_tree==&arenas[0].action && requested_frame==3 && requested_rate==.5f);
    CHECK(resume_calls==1);
    setup(); CHECK(nativeHome_WorkshopPose(demo,0,NULL)); selected=objects+1;
    CHECK(nativeHome_WorkshopPose(demo,0,NULL)); CHECK(last_tree==&arenas[1].demo && last_joint==joints[1] && resume_calls==1);
    setup(); rig_valid=false; CHECK(!nativeHome_WorkshopPose(demo,0,NULL)); CHECK(!attach_calls);
    setup(); tables[0].demo[0].x14=NULL; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); arenas[0].demo.frames=NAN; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); arenas[0].demo.nodes=(s8*)1; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); arenas[0].demo.tracks=(FigaTrack*)1; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); arenas[0].tracks[1].ad_head=(u8*)1; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); arenas[0].demo.frames=-1; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); bytes[0][2]=-2; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); characters[0].data.x14=(struct Fighter_WaitAnimData*)1;
    CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); fighters[0].kind=(FighterKind)41; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); live[0]=false; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); bones[0][0].flags_b1=false; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); CHECK(!nativeHome_WorkshopPose((NativeFighterAnimationClip){NativeFighterAnimation_Demo,2},0,NULL));
    CHECK(!nativeHome_WorkshopPose((NativeFighterAnimationClip){(NativeFighterAnimationBank)2,0},0,NULL));
    CHECK(!nativeHome_WorkshopPose(action,NAN,NULL)); CHECK(!nativeHome_WorkshopPose(action,65536,NULL));
    setup(); retire_motion=true; CHECK(!nativeHome_WorkshopPose(action,0,NULL)); CHECK(!resume_calls);
    setup(); retire_evaluate=true; CHECK(!nativeHome_WorkshopPose(demo,0,NULL)); CHECK(!resume_calls);
    setup(); CHECK(nativeHome_WorkshopPose(demo,0,NULL)); ++fighters[0].x8_spawnNum;
    nativeHome_WorkshopPoseRelease(true); CHECK(!resume_calls && attach_calls==1);
    setup(); CHECK(nativeHome_WorkshopPose(demo,0,NULL));
    ++fighters[0].x8_spawnNum; ++storage[0].generation;
    CHECK(nativeHome_WorkshopPose(demo,0,NULL)); CHECK(attach_calls==2 && !resume_calls);
    setup(); CHECK(nativeHome_WorkshopPose(demo,0,NULL));
    objects[0].hsd_obj=joints[0]+1;
    validations=validation_calls;
    CHECK(nativeHome_WorkshopPose(demo,0,NULL)); CHECK(validation_calls>validations);
    // Admission rejects custom actors without their owned published package.
    setup(); registered[0]=NULL; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); packages[0].ready=false; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); packages[0].target=(FighterKind)41; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); packages[0].program.identity++; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); storage[0].owner=1; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    setup(); storage[0].generation++; CHECK(!nativeHome_WorkshopPose(demo,0,NULL));
    for(unsigned change=SourceReplacement;change<MutationCount;++change) {
        // Both native motion entry and Demo evaluation are controlled callback
        // boundaries; production continuation must reject invalid retention.
        setup(); motion_mutation=change;
        CHECK(!nativeHome_WorkshopPose(action,0,NULL)); CHECK(!resume_calls);
        setup(); evaluation_mutation=change;
        CHECK(!nativeHome_WorkshopPose(demo,0,NULL)); CHECK(!resume_calls);
        setup(); CHECK(nativeHome_WorkshopPose(demo,0,NULL));
        home.workshop_preview_hitboxes=true; fighters[0].x914[0].state=HitCapsule_Enabled;
        change_retained(change); attachments=attach_calls;
        nativeHome_WorkshopPoseRelease(true);
        CHECK(!resume_calls && attach_calls==attachments);
        CHECK(fighters[0].x914[0].state==HitCapsule_Enabled);
        CHECK(!home.workshop_pose_gobj && !home.workshop_pose_source);
    }
    // Release itself can invalidate the retained storage during evaluation.
    setup(); CHECK(nativeHome_WorkshopPose(demo,0,NULL));
    evaluation_mutation=StorageDead; home.workshop_preview_hitboxes=true;
    fighters[0].x914[0].state=HitCapsule_Enabled; nativeHome_WorkshopPoseRelease(true);
    CHECK(!resume_calls && fighters[0].x914[0].state==HitCapsule_Enabled);
    printf("PASS %u checks\n",checks); return 0;
}
