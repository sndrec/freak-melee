// Production resource and shared joint bodies; deterministic HSD allocation
// boundary. Production callback lifetime scans the actual fixture live lists.
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article.h>
#include <melee/it/item.h>
#include <melee/it/kinds/types.h>
#include <melee/it/types.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <melee/lb/lb_00B0.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/objalloc.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { POOL = 540, OWNERS = 8 };
static unsigned checks, gobj_calls, userdata_calls, model_calls, frees, draws;
static int fail_gobj = -1, fail_userdata = -1, fail_model = -1;
static HSD_GObj objects[POOL], owners[OWNERS], creator;
static ItemLink links[POOL];
static HSD_JObj models[POOL][2], common_joint, thumb, replacement;
static bool objects_live[POOL], links_live[POOL], models_live[POOL];
static Item items[OWNERS];
static Fighter fighter;
static FighterBone parts[MAX_FT_PARTS];
static ftData fighter_data;
static ftData_x8 fighter_common;
static Article article;
static HSD_Joint model_descriptor;
static ItemLogicTable item_logics[OWNERS];
static NativeFighterArticleLogic article_logics[OWNERS];
static const ItemKind fixture_kinds[OWNERS]={
    It_Kind_Link_Bomb, It_Kind_CLink_Bomb, It_Kind_Link_Boomerang,
    It_Kind_CLink_Boomerang, It_Kind_Link_HShot, It_Kind_CLink_HShot,
    It_Kind_Link_Arrow, It_Kind_CLink_Arrow,
};
static NativeFighterDefinition definition, foreign_definition;
static FighterPartsTable part_table;
static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = part_tables;
HSD_ObjAllocData item_link_alloc_data;
u8 HSD_GObj_JObjKind;
static HSD_GObj* live_heads[32];
HSD_GObj** HSD_GObjPLinkHead = live_heads;
static bool require_detached;
#define CHECK(x) do { ++checks; assert(x); } while (0)

// Only slot storage is a fixture boundary; the included release body is the
// actual runtime code. All model-resource cancellation below is production.
typedef struct NativeFighterSlot {
    bool active;
    FighterKind fighter_kind;
    const NativeFighterDefinition* definition;
    void *character_arena, *animation_arena, *costume_arena, *motion_states, *package;
    NativeFighterModule module;
} NativeFighterSlot;
static unsigned holdable_clears, module_closes;
void nativeHoldableVisual_Clear(FighterKind kind)
{ CHECK(kind == Ft_Kind_Link); ++holdable_clears; }
void nativeFighterModule_Close(NativeFighterModule* module)
{ memset(module, 0, sizeof(*module)); ++module_closes; }
#include "article_resource_fixture.h"

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (kind == Ft_Kind_Link) return &definition;
    if (kind == Ft_Kind_CLink) return &foreign_definition;
    return NULL;
}

Fighter_GObj* nativeArticle_Creator(Item_GObj* object)
{
    Item* item = object->user_data;
    Fighter_GObj* source = item->xDD4_itemVar.native_article.attachment_owner;
    if (!nativeCallbackSource_Alive(source,
            item->xDD4_itemVar.native_article.creator_spawn_id,
            NativeCallbackSource_Fighter)) return NULL;
    Fighter* data = source->user_data;
    return nativeFighterRuntime_Definition(data->kind) ==
        nativeFighterRuntime_Definition(item->native_article_author_kind)
        ? source : NULL;
}

EF_SpawnResult efSync_Spawn(s32 effect, HSD_GObj* source, ...)
{ (void)effect; (void)source; abort(); }

EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object, uint32_t id)
{ (void)object; (void)id; abort(); }
bool nativeModelEffect_Snapshot(HSD_GObj* object, uint32_t* id)
{ (void)object; (void)id; abort(); }

HSD_GObj* GObj_Create(u16 classifier, u8 link, u8 priority)
{
    CHECK(classifier == HSD_GOBJ_CLASS_ITEMLINK && link == 10 && priority == 0);
    if ((int)gobj_calls++ == fail_gobj) return NULL;
    for (unsigned i = 0; i < POOL; ++i) if (!objects_live[i]) {
        objects_live[i] = true;
        memset(&objects[i], 0, sizeof(objects[i]));
        return &objects[i];
    }
    abort();
}

void* HSD_ObjAlloc(HSD_ObjAllocData* allocator)
{
    CHECK(allocator == &item_link_alloc_data);
    if ((int)userdata_calls++ == fail_userdata) return NULL;
    for (unsigned i = 0; i < POOL; ++i) if (!links_live[i]) {
        links_live[i] = true;
        // Real constructors must initialize all consumed data, not trust zeros.
        memset(&links[i], 0xA5, sizeof(links[i]));
        return &links[i];
    }
    abort();
}

void HSD_ObjFree(HSD_ObjAllocData* allocator, void* value)
{
    CHECK(allocator == &item_link_alloc_data);
    ptrdiff_t index = (ItemLink*)value - links;
    CHECK(index >= 0 && index < POOL && links_live[index]);
    links_live[index] = false;
    // Poison links immediately. Release must never traverse a freed neighbor.
    memset(value, 0xDD, sizeof(ItemLink));
}

void GObj_InitUserData(HSD_GObj* object, u8 kind, void (*remove)(void*), void* data)
{ CHECK(kind == 6 && remove != NULL); object->user_data = data; object->user_data_remove_func = remove; }

void HSD_GObjFree(HSD_GObj* object)
{
    ptrdiff_t index = object - objects;
    CHECK(index >= 0 && index < POOL && objects_live[index]);
    if (require_detached) {
        CHECK(parts[139].joint != object->hsd_obj);
        CHECK(fighter.mv.co.capturedamage.x18 != object->hsd_obj);
        for (unsigned i = 0; i < 4; ++i) CHECK(fighter.x914[i].jobj != object->hsd_obj);
        CHECK(fighter.x1064_thrownHitbox.jobj != object->hsd_obj);
    }
    if (object->hsd_obj != NULL) {
        ptrdiff_t model_index = ((HSD_JObj(*)[2])object->hsd_obj) - models;
        CHECK(model_index >= 0 && model_index < POOL && models_live[model_index]);
        models_live[model_index] = false;
        memset(models[model_index], 0xDD, sizeof(models[model_index]));
    }
    if (object->user_data_remove_func) object->user_data_remove_func(object->user_data);
    objects_live[index] = false;
    memset(object, 0xDD, sizeof(*object));
    ++frees;
}

HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor)
{
    CHECK(descriptor == &model_descriptor);
    if ((int)model_calls++ == fail_model) return NULL;
    for (unsigned i = 0; i < POOL; ++i) if (!models_live[i]) {
        models_live[i] = true;
        memset(models[i], 0, sizeof(models[i]));
        models[i][0].child = &models[i][1];
        models[i][1].parent = &models[i][0];
        models[i][0].scale = models[i][1].scale = (Vec3){1,1,1};
        return &models[i][0];
    }
    abort();
}

void HSD_GObjObject_80390A70(HSD_GObj* object, u8 kind, void* model)
{ CHECK(kind == HSD_GObj_JObjKind); object->hsd_obj = model; }
void GObj_SetupGXLink(HSD_GObj* object, GObj_RenderFunc draw, u8 link, u32 priority)
{ CHECK(draw != NULL && link == 6 && priority == 0); object->render_cb = draw; }
void HSD_GObj_JObjCallback(HSD_GObj* object, int pass)
{ CHECK(object->hsd_obj != NULL && pass == 0); ++draws; }
// Controlled initialization boundary; the YoYo source fixture links the real body.
void mpColl_80041EE4(CollData* collision)
{
    Vec3 position = collision->cur_pos;
    memset(collision, 0, sizeof(*collision));
    collision->cur_pos = collision->prev_pos = collision->last_pos =
        collision->x28_vec = position;
}
void mpColl_SetECBSource_Fixed(CollData* collision, HSD_GObj* source,
                             float a, float b, float c, float d)
{ CHECK(source == NULL && a == b && b == c && c == d && a > 0); (void)collision; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { joint->flags |= JOBJ_MTX_DIRTY; }
void HSD_JObjSetFlags(HSD_JObj* joint, u32 flags) { joint->flags |= flags; }
void HSD_JObjClearFlags(HSD_JObj* joint, u32 flags) { joint->flags &= ~flags; }
void HSD_JObjSetFlagsAll(HSD_JObj* joint, u32 flags)
{ for (; joint; joint=joint->next) { HSD_JObjSetFlags(joint,flags); HSD_JObjSetFlagsAll(joint->child,flags); } }
void HSD_JObjClearFlagsAll(HSD_JObj* joint, u32 flags)
{ for (; joint; joint=joint->next) { HSD_JObjClearFlags(joint,flags); HSD_JObjClearFlagsAll(joint->child,flags); } }
void C_MTXCopy(Mtx source, Mtx destination) { memcpy(destination, source, sizeof(Mtx)); }
// Geometry/model controls have their own original-source matrix. This lifecycle
// fixture must not silently exercise them through an unverified fake boundary.
bool mpColl_800471F8(CollData* collision) { (void)collision; abort(); }
bool mpColl_80048844(CollData* collision) { (void)collision; abort(); }
bool mpColl_8004B108(CollData* collision) { (void)collision; abort(); }
bool mpGetSpeed(int line, Vec3* position, Vec3* speed)
{ (void)line; (void)position; (void)speed; abort(); }
void C_MTXMultVec(Mtx matrix, Vec3* point, Vec3* output)
{ (void)matrix; (void)point; (void)output; abort(); }
void HSD_JObjSetupMatrixSub(HSD_JObj* joint) { (void)joint; abort(); }
void HSD_JObjAddAnimAll(HSD_JObj* joint, HSD_AnimJoint* anim,
                       HSD_MatAnimJoint* matanim, HSD_ShapeAnimJoint* shapeanim)
{ (void)joint; (void)anim; (void)matanim; (void)shapeanim; abort(); }
void HSD_JObjReqAnimAll(HSD_JObj* joint, float frame)
{ (void)joint; (void)frame; abort(); }
void HSD_JObjAnimAll(HSD_JObj* joint) { (void)joint; abort(); }
void HSD_JObjRemoveAnimAll(HSD_JObj* joint) { (void)joint; abort(); }
void lb_8000BA0C(HSD_JObj* joint, float rate)
{ (void)joint; (void)rate; abort(); }
void __assert(char* file, u32 line, char* condition)
{ fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,condition); abort(); }

static void no_leaks(void)
{
    for (unsigned i=0;i<POOL;++i) CHECK(!objects_live[i] && !links_live[i] && !models_live[i]);
}

static void reset(void)
{
    no_leaks();
    memset(&fighter,0,sizeof(fighter)); memset(parts,0,sizeof(parts));
    fighter.kind=Ft_Kind_Link; fighter.x8_spawnNum=0x87654321;
    fighter.parts=parts; fighter.ft_data=&fighter_data;
    fighter_data.x8=&fighter_common; fighter_common.x11=1;
    fighter.motion_id=ftCo_MS_CatchPull;
    parts[0].joint=&thumb; parts[1].joint=&common_joint;
    part_table.parts_num=76; part_tables[Ft_Kind_Link]=&part_table;
    creator=(HSD_GObj){.user_data=&fighter};
    live_heads[HSD_GOBJ_PLINK_FIGHTER]=&creator;
    memset(&article,0,sizeof(article));
    article.native_model_root_count=1;
    article.native_model_roots[0].name_hash=0x12345678;
    article.native_model_roots[0].root=&model_descriptor;
    definition=(NativeFighterDefinition){.article_logic_count=OWNERS,.article_logics=article_logics};
    for (unsigned i=0;i<OWNERS;++i) {
        article_logics[i]=(NativeFighterArticleLogic){.kind=fixture_kinds[i],
            .logic=&item_logics[i],.resource_capacity=90,.resource_reserved_part=i==0?139:-1};
        items[i]=(Item){.kind=fixture_kinds[i],.x1C=0x80000000u+i,
            .native_article_author_kind=Ft_Kind_Link,.xB8_itemLogicTable=&item_logics[i],
            .xC4_article_data=&article};
        items[i].xDD4_itemVar.native_article.attachment_owner=&creator;
        items[i].xDD4_itemVar.native_article.creator_spawn_id=(u32)fighter.x8_spawnNum;
        owners[i]=(HSD_GObj){.user_data=&items[i],.next=i+1<OWNERS?&owners[i+1]:NULL};
        items[i].entity=&owners[i];
    }
    // Identical metadata still belongs to a distinct immutable package object.
    foreign_definition=definition;
    live_heads[HSD_GOBJ_PLINK_ITEM]=owners;
    gobj_calls=userdata_calls=model_calls=frees=draws=0;
    fail_gobj=fail_userdata=fail_model=-1;
    require_detached=false;
}

static void construct(unsigned owner_index,unsigned count)
{
    HSD_GObj* owner=&owners[owner_index];
    CHECK(nativeArticleResource_Begin(owner,count));
    for (unsigned i=0;i<count;++i) {
        CHECK(nativeArticleResource_CreateNode(owner,i, &(const NativeArticleResourceInit){0,.75f,5,{0}}));
        CHECK(nativeArticleResource_LoadModel(owner,i,0x12345678));
    }
}

static void metadata_validation_matrix(void)
{
    reset();
    CHECK(!nativeArticleResource_ValidateDefinition(NULL,76));
    NativeFighterDefinition malformed=definition;
    malformed.article_logic_count=65;
    CHECK(!nativeArticleResource_ValidateDefinition(&malformed,76));
    malformed=definition; malformed.article_logics=NULL;
    CHECK(!nativeArticleResource_ValidateDefinition(&malformed,76));
    malformed=definition; malformed.article_logic_count=0;
    CHECK(!nativeArticleResource_ValidateDefinition(&malformed,76));
    malformed.article_logics=NULL;
    CHECK(nativeArticleResource_ValidateDefinition(&malformed,76));
    static const struct {
        uint16_t capacity;
        int16_t reservation;
        bool valid, enabled;
    } declarations[]={
        {0,139,true,false}, {1,139,false,false}, {91,139,false,false},
        {2,-2,false,false}, {2,140,false,false}, {2,75,false,false},
        {2,76,true,true}, {90,-1,true,true}, {90,139,true,true},
    };
    for(unsigned i=0;i<sizeof(declarations)/sizeof(declarations[0]);++i) {
        article_logics[0].resource_capacity=declarations[i].capacity;
        article_logics[0].resource_reserved_part=declarations[i].reservation;
        CHECK(nativeArticleResource_ValidateDefinition(&definition,76)==declarations[i].valid);
        CHECK(nativeArticleResource_Begin(owners,2)==declarations[i].enabled);
        nativeArticleResource_Release(owners);
    }
    article_logics[0].resource_capacity=2;
    article_logics[0].resource_reserved_part=76;
    CHECK(nativeArticleResource_ValidateDefinition(&definition,76));
    part_table.parts_num=77;
    CHECK(!nativeArticleResource_ValidateDefinition(&definition,77));
    CHECK(!nativeArticleResource_Begin(owners,2));
    part_table.parts_num=76;
    CHECK(!nativeArticleResource_Begin(owners,0));
    CHECK(!nativeArticleResource_Begin(owners,3));
    CHECK(!nativeArticleResource_Begin(owners,UINT32_MAX));
    CHECK(gobj_calls==0 && userdata_calls==0 && model_calls==0);
    no_leaks();
}

static void bounds_and_models(void)
{
    reset();
    CHECK(!nativeArticleResource_Begin(NULL,2));
    article_logics[0].resource_capacity=91;
    CHECK(!nativeArticleResource_Begin(owners,2));
    article_logics[0].resource_capacity=90;
    CHECK(!nativeArticleResource_Begin(owners,91));
    CHECK(!nativeArticleResource_Begin(owners,1));
    article_logics[0].resource_reserved_part=75;
    CHECK(!nativeArticleResource_Begin(owners,2));
    article_logics[0].resource_reserved_part=140;
    CHECK(!nativeArticleResource_Begin(owners,2));
    article_logics[0].resource_capacity=2;
    article_logics[0].resource_reserved_part=-1;
    CHECK(nativeArticleResource_Begin(owners,2));
    CHECK(!nativeArticleResource_Begin(owners,2));
    CHECK(!nativeArticleResource_CreateNode(owners,1, &(const NativeArticleResourceInit){0,1,5,{0}}));
    CHECK(!nativeArticleResource_CreateNode(owners,0, &(const NativeArticleResourceInit){76,1,5,{0}}));
    CHECK(!nativeArticleResource_CreateNode(owners,0, &(const NativeArticleResourceInit){0,NAN,5,{0}}));
    CHECK(!nativeArticleResource_CreateNode(owners,0, &(const NativeArticleResourceInit){0,0,5,{0}}));
    CHECK(!nativeArticleResource_CreateNode(owners,0, &(const NativeArticleResourceInit){0,1,16,{0}}));
    CHECK(!nativeArticleResource_CreateNode(owners,0,NULL));
    unsigned allocations = gobj_calls;
    for (unsigned axis = 0; axis < 3; ++axis) {
        NativeArticleResourceInit invalid = {0,1,5,{2,3,4}};
        if (axis == 0) invalid.position.x = NAN;
        if (axis == 1) invalid.position.y = INFINITY;
        if (axis == 2) invalid.position.z = -INFINITY;
        CHECK(!nativeArticleResource_CreateNode(owners,0,&invalid));
        CHECK(gobj_calls == allocations);
    }
    CHECK(!nativeArticleResource_Exists(owners,0));
    NativeArticleResourceInit positioned = {0,1,5,{12,-7,3.5f}};
    CHECK(nativeArticleResource_CreateNode(owners,0,&positioned));
    CHECK(memcmp(&links[0].pos,&positioned.position,sizeof(Vec3))==0);
    CHECK(memcmp(&links[0].coll_data.cur_pos,&positioned.position,sizeof(Vec3))==0);
    CHECK(memcmp(&links[0].coll_data.prev_pos,&positioned.position,sizeof(Vec3))==0);
    CHECK(memcmp(&links[0].coll_data.last_pos,&positioned.position,sizeof(Vec3))==0);
    CHECK(memcmp(&links[0].coll_data.x28_vec,&positioned.position,sizeof(Vec3))==0);
    CHECK(!nativeArticleResource_LoadModel(owners,0,0));
    CHECK(!nativeArticleResource_LoadModel(owners,0,1));
    article.native_model_root_count=NATIVE_FIGHTER_ARTICLE_MODEL_ROOT_LIMIT+1;
    CHECK(!nativeArticleResource_LoadModel(owners,0,0x12345678));
    article.native_model_root_count=1;
    CHECK(nativeArticleResource_LoadModel(owners,0,0x12345678));
    CHECK(!nativeArticleResource_LoadModel(owners,0,0x12345678));
    CHECK(nativeArticleResource_Count(owners)==2 && nativeArticleResource_Exists(owners,0));
    CHECK(!nativeArticleResource_BindExternalPart(owners,0,139));
    nativeArticleResource_WriteJoint(owners,0,"0",0,3.25f);
    CHECK(nativeArticleResource_ReadJoint(owners,0,"0",0)==3.25f);
    nativeArticleResource_WriteJoint(owners,0,"",15,0);
    CHECK(nativeArticleResource_ReadJoint(owners,0,"",15)==0);
    CHECK(nativeArticleResource_ReadJoint(owners,0,"0",15)==1);
    nativeArticleResource_WriteJoint(owners,0,"0",0,NAN);
    CHECK(nativeArticleResource_ReadJoint(owners,0,"0",0)==3.25f);
    Mtx matrix={{1,0,0,2},{0,1,0,3},{0,0,1,4}};
    CHECK(nativeArticleResource_SetWorldMatrix(owners,0,matrix));
    CHECK(memcmp(models[0][0].mtx,matrix,sizeof(matrix))==0);
    matrix[0][0]=NAN;
    CHECK(!nativeArticleResource_SetWorldMatrix(owners,0,matrix));
    objects[0].render_cb(&objects[0],0); CHECK(draws==0);
    CHECK(nativeArticleResource_SetDraw(owners,0,true));
    objects[0].render_cb(&objects[0],0); CHECK(draws==1);
    CHECK(nativeArticleResource_CreateNode(owners,1, &(const NativeArticleResourceInit){0,1,5,{0}}));
    CHECK(links[1].next==&links[0] && links[0].prev==&links[1]);
    CHECK(links[0].next==NULL && links[1].prev==NULL && links[0].x1CC==-1);
    CHECK(!nativeArticleResource_CreateNode(owners,2, &(const NativeArticleResourceInit){0,1,5,{0}}));
    nativeArticleResource_Release(owners); nativeArticleResource_Release(owners);
    CHECK(nativeArticleResource_Count(owners)==0); no_leaks();
    article_logics[0].resource_reserved_part=100;
    article_logics[1].resource_capacity=2;
    article_logics[1].resource_reserved_part=100;
    CHECK(nativeArticleResource_Begin(owners,2));
    CHECK(!nativeArticleResource_Begin(&owners[1],2));
    article_logics[1].resource_reserved_part=101;
    CHECK(nativeArticleResource_Begin(&owners[1],2));
    CHECK(nativeArticleResource_CreateNode(owners,0, &(const NativeArticleResourceInit){0,1,5,{0}}));
    CHECK(nativeArticleResource_LoadModel(owners,0,0x12345678));
    CHECK(nativeArticleResource_BindExternalPart(owners,0,100));
    CHECK(parts[100].joint==models[0]);
    nativeArticleResource_Release(owners);
    nativeArticleResource_Release(&owners[1]);
    CHECK(parts[100].joint==NULL); no_leaks();
    article_logics[1].resource_reserved_part=100;
    CHECK(nativeArticleResource_Begin(&owners[1],2));
    nativeArticleResource_Release(&owners[1]);
    article_logics[0].resource_reserved_part=-1;
    CHECK(nativeArticleResource_Begin(owners,2));
    CHECK(nativeArticleResource_CreateNode(owners,0, &(const NativeArticleResourceInit){0,1,5,{0}}));
    CHECK(nativeArticleResource_LoadModel(owners,0,0x12345678));
    fighter.motion_id=ftCo_MS_Wait;
    CHECK(!nativeArticleResource_SetCaptureAnchor(owners,0));
    fighter.motion_id=ftCo_MS_CatchPull;
    CHECK(nativeArticleResource_SetCaptureAnchor(owners,0));
    fighter.parts=NULL;
    nativeArticleResource_Release(owners);
    CHECK(fighter.mv.co.capturedamage.x18==NULL); no_leaks();
}

static void allocation_failure_matrix(void)
{
    // One batched sweep of every allocation boundary in a maximum collection.
    for (unsigned failure=0;failure<3;++failure) for (unsigned at=0;at<90;++at) {
        reset();
        if (failure==0) fail_gobj=(int)at;
        if (failure==1) fail_userdata=(int)at;
        if (failure==2) fail_model=(int)at;
        CHECK(nativeArticleResource_Begin(owners,90));
        for (unsigned i=0;i<=at;++i) {
            bool created=nativeArticleResource_CreateNode(owners,i, &(const NativeArticleResourceInit){0,1,5,{0}});
            if (failure<2 && i==at) { CHECK(!created); break; }
            CHECK(created);
            bool loaded=nativeArticleResource_LoadModel(owners,i,0x12345678);
            CHECK(loaded == !(failure==2 && i==at));
        }
        nativeArticleResource_Release(owners); no_leaks();
    }
    reset();
    article_logics[0].resource_reserved_part=-1;
    for (unsigned i=0;i<6;++i) construct(i,90);
    article_logics[6].resource_reserved_part=139;
    CHECK(!nativeArticleResource_Begin(&owners[6],2));
    for (unsigned i=0;i<6;++i) nativeArticleResource_Release(&owners[i]);
    no_leaks();
    article_logics[7].resource_reserved_part=139;
    construct(7,2); nativeArticleResource_Release(&owners[7]); no_leaks();
}

static void borrower_and_identity_matrix(void)
{
    for (unsigned scenario=0;scenario<8;++scenario) {
        reset(); construct(0,3);
        CHECK(!nativeArticleResource_BindExternalPart(owners,2,100));
        CHECK(nativeArticleResource_BindExternalPart(owners,2,139));
        CHECK(!nativeArticleResource_BindExternalPart(owners,1,139));
        CHECK(nativeArticleResource_SetCaptureAnchor(owners,2));
        HSD_JObj* endpoint=parts[139].joint;
        fighter.x914[0].jobj=endpoint; fighter.x914[0].state=HitCapsule_Enabled;
        fighter.x914[1].jobj=&replacement; fighter.x914[1].state=HitCapsule_Enabled;
        fighter.x1064_thrownHitbox.jobj=endpoint;
        if (scenario==1) { // New consumers own new pointers; leave them untouched.
            parts[139].joint=&replacement;
            fighter.mv.co.capturedamage.x18=&replacement;
            fighter.x914[0].jobj=&replacement;
            fighter.x1064_thrownHitbox.jobj=&replacement;
        } else if (scenario==2) live_heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
        else if (scenario==3 || scenario==4) {
            if (scenario==3) ++fighter.x8_spawnNum;
            else fighter.kind=Ft_Kind_CLink;
            // New rig/consumer bindings belong to the new creator life/package.
            parts[139].joint=&replacement;
            fighter.mv.co.capturedamage.x18=&replacement;
            fighter.x914[0].jobj=&replacement;
        }
        else if (scenario==5) { // An unrelated motion union must not be scanned.
            fighter.motion_id=ftCo_MS_Wait;
            fighter.mv.co.capturedamage.x18=endpoint;
        } else if (scenario==6) { // Recycled Item address rejects old resources.
            ++items[0].x1C;
            CHECK(nativeArticleResource_Count(owners)==0);
            CHECK(!nativeArticleResource_Exists(owners,2));
            --items[0].x1C;
        } else if (scenario==7) {
            items[0].native_article_author_kind=Ft_Kind_CLink;
            CHECK(nativeArticleResource_Count(owners)==0);
            items[0].native_article_author_kind=Ft_Kind_Link;
        }
        require_detached=scenario==0 || scenario==1 || scenario>=6;
        nativeArticleResource_Release(owners);
        if (scenario==0 || scenario>=6) {
            CHECK(parts[139].joint==NULL && fighter.mv.co.capturedamage.x18==&common_joint);
            CHECK(fighter.x914[0].state==HitCapsule_Disabled && fighter.x914[0].jobj==NULL);
        } else if (scenario==1) {
            CHECK(parts[139].joint==&replacement && fighter.mv.co.capturedamage.x18==&replacement);
            CHECK(fighter.x914[0].jobj==&replacement && fighter.x914[0].state==HitCapsule_Enabled);
        } else if (scenario==2) {
            CHECK(parts[139].joint==endpoint && fighter.x914[0].jobj==endpoint);
        } else if (scenario==3 || scenario==4) {
            CHECK(parts[139].joint==&replacement && fighter.x914[0].jobj==&replacement);
            CHECK(fighter.mv.co.capturedamage.x18==&replacement);
        } else if (scenario==5) {
            CHECK(fighter.mv.co.capturedamage.x18==endpoint && parts[139].joint==NULL);
        }
        CHECK(fighter.x914[1].jobj==&replacement && fighter.x914[1].state==HitCapsule_Enabled);
        no_leaks();
    }
    reset();
    live_heads[HSD_GOBJ_PLINK_ITEM]=NULL;
    CHECK(!nativeArticleResource_Begin(owners,2));
    live_heads[HSD_GOBJ_PLINK_ITEM]=owners;
    items[0].xB8_itemLogicTable=NULL;
    CHECK(!nativeArticleResource_Begin(owners,2));
    items[0].xB8_itemLogicTable=&item_logics[0];
    ++items[0].xDD4_itemVar.native_article.creator_spawn_id;
    CHECK(!nativeArticleResource_Begin(owners,2));
}

static void teardown_matrix(void)
{
    // Removal can be requested after the Item leaves the live lists. The key
    // must not read its old userdata, nor require an active article Definition.
    reset(); construct(0,3);
    CHECK(nativeArticleResource_BindExternalPart(owners,2,139));
    uint32_t old_id = items[0].x1C;
    live_heads[HSD_GOBJ_PLINK_ITEM] = &owners[1];
    owners[0].user_data = (void*)(uintptr_t)1;
    nativeArticleResource_CancelGeneration(owners, old_id + 1);
    CHECK(objects_live[0]);
    nativeArticleResource_CancelGeneration(owners, old_id);
    CHECK(parts[139].joint == NULL); no_leaks();
    nativeArticleResource_CancelGeneration(owners, old_id);

    // A delayed old removal must not destroy resources of a recycled Item.
    owners[0].user_data = &items[0];
    live_heads[HSD_GOBJ_PLINK_ITEM] = owners;
    ++items[0].x1C;
    article_logics[0].resource_reserved_part=-1;
    construct(0,2);
    nativeArticleResource_CancelGeneration(owners, old_id);
    CHECK(nativeArticleResource_Count(owners) == 2);
    nativeArticleResource_CancelGeneration(owners, items[0].x1C); no_leaks();

    reset(); construct(0,2); construct(1,2);
    CHECK(nativeArticleResource_BindExternalPart(owners,1,139));
    nativeArticleResource_CancelCreator(&creator, (uint32_t)fighter.x8_spawnNum + 1);
    CHECK(nativeArticleResource_Count(owners) == 2);
    nativeArticleResource_CancelCreator(&creator, (uint32_t)fighter.x8_spawnNum);
    CHECK(parts[139].joint == NULL); no_leaks();

    reset(); construct(0,2);
    CHECK(nativeArticleResource_BindExternalPart(owners,1,139));
    nativeArticleResource_CancelDefinition(&foreign_definition);
    CHECK(nativeArticleResource_Count(owners) == 2);
    nativeArticleResource_CancelDefinition(&definition);
    CHECK(parts[139].joint == NULL); no_leaks();

    reset(); article_logics[0].resource_reserved_part=-1;
    construct(0,2); construct(1,2);
    nativeArticleResource_Clear(); nativeArticleResource_Clear(); no_leaks();

    // Preflight may share the live DLL's immutable Definition. Releasing a
    // detached candidate must not cancel the scene's active article resources.
    reset(); construct(0,2);
    CHECK(nativeArticleResource_BindExternalPart(owners,1,139));
    NativeFighterSlot detached = { .definition=&definition, .fighter_kind=Ft_Kind_Link };
    NativeFighterSlot active = { .active=true, .definition=&definition, .fighter_kind=Ft_Kind_Link };
    NativeFighterSlot unrelated = { .definition=&foreign_definition };
    // Actual retirement must revoke borrowed load ownership without touching
    // source userdata, including an already retired/poisoned source marker.
    load_context_count=3;
    load_contexts[0]=(NativeFighterLoadContext){ .object=&creator,
        .generation=0x80000001u, .definition=&definition, .slot=&active };
    load_contexts[1]=(NativeFighterLoadContext){ .object=(Fighter_GObj*)(uintptr_t)1,
        .generation=0x80000002u, .definition=&definition, .slot=&detached };
    load_contexts[2]=(NativeFighterLoadContext){ .object=owners,
        .generation=0x80000003u, .definition=&foreign_definition, .slot=&unrelated };
    unsigned prior_holdables=holdable_clears, prior_modules=module_closes;
    release_slot(&detached);
    CHECK(load_context_count==3 && load_contexts[1].slot==NULL &&
          load_contexts[1].definition==NULL);
    CHECK(load_contexts[1].object==(Fighter_GObj*)(uintptr_t)1 &&
          load_contexts[1].generation==0x80000002u);
    detached.definition=&definition; // Reusing the slot cannot restore the lease.
    CHECK(load_contexts[1].slot==NULL && load_contexts[1].definition==NULL);
    CHECK(load_contexts[0].slot==&active && load_contexts[0].definition==&definition);
    CHECK(load_contexts[2].slot==&unrelated && load_contexts[2].definition==&foreign_definition);
    CHECK(nativeArticleResource_Count(owners)==2 && parts[139].joint!=NULL);
    CHECK(holdable_clears==prior_holdables && module_closes==prior_modules+1);
    release_slot(&active);
    CHECK(load_contexts[0].slot==NULL && load_contexts[0].definition==NULL &&
          load_contexts[0].object==&creator && load_contexts[0].generation==0x80000001u);
    CHECK(load_contexts[2].slot==&unrelated && load_contexts[2].definition==&foreign_definition);
    // This fragment does not enter/leave callbacks; discard harness contexts.
    memset(load_contexts,0,sizeof(load_contexts)); load_context_count=0;
    CHECK(parts[139].joint==NULL && active.definition==NULL && !active.active);
    CHECK(holdable_clears==prior_holdables+1 && module_closes==prior_modules+2);
    no_leaks();
}

int main(void)
{
    metadata_validation_matrix(); bounds_and_models();
    allocation_failure_matrix(); borrower_and_identity_matrix();
    teardown_matrix();
    printf("{\"checks\":%u,\"allocation_failures\":270,\"maximum_live_nodes\":540,\"borrower_cases\":8,\"teardown_cases\":6}\n",checks);
    return 0;
}
