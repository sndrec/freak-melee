// Actual backend, lifetime, joint and stock-helper bodies. Allocation, stage
// response and animation dispatch are explicit deterministic fixture boundaries.
#include <melee/it/native_article_resource.h>
#include <melee/it/native_article.h>
#include <melee/it/types.h>
#include <melee/it/item.h>
#include <melee/it/kinds/types.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ef/native_model_effect.h>
#include <melee/ef/efsync.h>
#include <stdarg.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, source_cases, probe_calls, floor_calls, speed_calls, setup_calls;
static unsigned object_count, link_count, model_count, adds, requests, advances, removes;
static unsigned scenario;
static HSD_GObj objects[6], owner, creator;
static ItemLink links[6];
static HSD_JObj models[6][3], attachment;
static HSD_AObj aobjs[6][3];
static HSD_Joint model_descriptor;
static HSD_AnimJoint anim_descriptor;
static HSD_MatAnimJoint mat_descriptor;
static HSD_ShapeAnimJoint shape_descriptor;
static HSD_JObj* last_animation_joint;
static HSD_AnimJoint* last_anim;
static HSD_MatAnimJoint* last_mat;
static HSD_ShapeAnimJoint* last_shape;
static float last_frame;
static Item item;
static Article article;
static ItemLogicTable logic;
static Fighter fighter;
static FighterBone parts[MAX_FT_PARTS];
static FighterPartsTable table;
static FighterPartsTable* tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=tables;
static NativeFighterArticleLogic article_logic;
static NativeFighterDefinition definition, replacement_definition;
static HSD_GObj* live_heads[32];
HSD_GObj** HSD_GObjPLinkHead=live_heads;
HSD_ObjAllocData item_link_alloc_data;
u8 HSD_GObj_JObjKind;
static float it_804D6D48=6;
#define CHECK(x) do { ++checks; assert(x); } while (0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_Link?&definition:kind==Ft_Kind_CLink?&replacement_definition:NULL; }
Fighter_GObj* nativeArticle_Creator(Item_GObj* source)
{
    Item* data=source->user_data;
    Fighter_GObj* result=data->xDD4_itemVar.native_article.attachment_owner;
    if (!nativeCallbackSource_Alive(result,data->xDD4_itemVar.native_article.creator_spawn_id,
                                    NativeCallbackSource_Fighter)) return NULL;
    return nativeFighterRuntime_Definition(((Fighter*)result->user_data)->kind)==
        nativeFighterRuntime_Definition(data->native_article_author_kind)?result:NULL;
}
static unsigned effect_calls, effect_mode, generator_calls;
static HSD_GObj* effect_object;
static Vec3 effect_point, generator_point;
static float effect_facing, generator_facing;
static s32 generator_id;
static bool generator_fail;
int efLib_LoadKind;
HSD_Generator* efLib_CreateGenerator(s32 id, Vec3* point)
{ ++generator_calls; generator_id=id; generator_point=*point; return generator_fail ? NULL : (HSD_Generator*)(uintptr_t)1; }
HSD_Generator* efLib_CreateGenerator_Translate_FacingDir(s32 id,Vec3* point,float facing)
{ generator_facing=facing; return efLib_CreateGenerator(id,point); }
#include "resource_effect_payloads.h"
EF_SpawnResult efSync_Spawn(s32 effect, HSD_GObj* source, ...)
{
    ++effect_calls;
    CHECK(source == &objects[0] && source->user_data == &links[0]);
    effect_object = source;
    va_list args; va_start(args, source);
    va_list original; va_copy(original,args);
    effect_point = *va_arg(args, Vec3*);
    if(effect>=0x3EF && effect<=0x3F2) effect_facing=*va_arg(args,float*);
    va_end(args);
    EF_SpawnResult result;
    if((unsigned)effect<0x250 || effect/1000==0x1E) {
        result=(EF_SpawnResult){.kind=EF_SPAWN_GENERATOR};
        result.object.generator=efLib_CreateGenerator(effect,&effect_point);
    } else result=stock_resource_dispatch(effect,source,original);
    va_end(original);
    if (effect_mode == 1) nativeArticleResource_Release(&owner);
    if (effect_mode == 2) nativeArticleResource_CancelDefinition(&definition);
    if (effect_mode == 3) {
        ++item.x1C;
        nativeArticleResource_CancelGeneration(&owner, item.x1C - 1);
    }
    if (effect_mode == 4) {
        ++fighter.x8_spawnNum;
        nativeArticleResource_CancelCreator(&creator, (uint32_t)fighter.x8_spawnNum - 1);
    }
    if (effect_mode != 0) {
        CHECK(!nativeArticleResource_Exists(&owner,0));
        CHECK(source->user_data == &links[0] && links[0].gobj == source);
    }
    return result;
}

EF_Effect* nativeModelEffect_Resolve(HSD_GObj* source,uint32_t id)
{ (void)source; (void)id; abort(); }
bool nativeModelEffect_Snapshot(HSD_GObj* source,uint32_t* id)
{ (void)source; (void)id; abort(); }
HSD_GObj* GObj_Create(u16 cls,u8 link,u8 priority)
{ CHECK(cls==HSD_GOBJ_CLASS_ITEMLINK && link==10 && priority==0 && object_count<6); return &objects[object_count++]; }
void* HSD_ObjAlloc(HSD_ObjAllocData* pool)
{ CHECK(pool==&item_link_alloc_data && link_count<6); return &links[link_count++]; }
void HSD_ObjFree(HSD_ObjAllocData* pool,void* data)
{ CHECK(pool==&item_link_alloc_data && data!=NULL); }
void GObj_InitUserData(HSD_GObj* g,u8 kind,void(*remove)(void*),void* data)
{ CHECK(kind==6); g->user_data=data; g->user_data_remove_func=remove; }
void HSD_GObjFree(HSD_GObj* g)
{ if(g->user_data_remove_func) g->user_data_remove_func(g->user_data); memset(g,0,sizeof(*g)); }
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* descriptor)
{
    CHECK(descriptor==&model_descriptor && model_count<6);
    unsigned n=model_count++;
    for(unsigned i=0;i<3;++i) {
        models[n][i].scale=(Vec3){1,1,1}; models[n][i].aobj=&aobjs[n][i];
        aobjs[n][i].framerate=1;
        if(i<2) models[n][i].child=&models[n][i+1];
        if(i) models[n][i].parent=&models[n][i-1];
    }
    return models[n];
}
void HSD_GObjObject_80390A70(HSD_GObj* g,u8 kind,void* model)
{ CHECK(kind==HSD_GObj_JObjKind); g->hsd_obj=model; }
void GObj_SetupGXLink(HSD_GObj* g,GObj_RenderFunc cb,u8 link,u32 priority)
{ CHECK(link==6 && priority==0); g->render_cb=cb; }
void HSD_GObj_JObjCallback(HSD_GObj* g,int pass) { (void)g; (void)pass; }
void it_802A24A0(HSD_GObj* g,int pass) { (void)g; (void)pass; }
void mpColl_80041EE4(CollData* c)
{
    Vec3 position = c->cur_pos;
    memset(c,0,sizeof(*c));
    c->cur_pos=c->prev_pos=c->last_pos=c->x28_vec=position;
    c->floor.index=c->ceiling.index=c->left_facing_wall.index=c->right_facing_wall.index=-1;
}
void mpColl_SetECBSource_Fixed(CollData* c,HSD_GObj* g,float a,float b,float d,float e)
{ (void)c; CHECK(g==NULL && a==b && b==d && d==e); }
bool mpColl_800471F8(CollData* c)
{
    ++probe_calls;
    c->prev_pos=(Vec3){101,102,103}; // Distinct engine previous-step field.
    c->cur_pos.x+=.25f*(float)scenario; c->cur_pos.y-=.5f*(float)scenario;
    c->env_flags=(int32_t)(0x80000000u | (scenario&1?Collide_FloorMask:Collide_WallMask));
    c->floor.index=(int32_t)0x81234567; c->left_facing_wall.index=3; c->right_facing_wall.index=4;
    c->ceiling.index=5;
    c->floor.normal=(Vec3){.6f,.8f,0}; c->left_facing_wall.normal=(Vec3){1,0,0};
    c->right_facing_wall.normal=(Vec3){-1,0,0}; c->ceiling.normal=(Vec3){0,-1,0};
    return (scenario&1)!=0;
}
bool mpColl_80048844(CollData* c) { (void)c; abort(); }
bool mpColl_8004B108(CollData* c)
{ ++floor_calls; c->cur_pos.y+=3; c->env_flags=Collide_FloorMask; return scenario%3!=0; }
bool mpLib_80054ED8(int line) { return line==7; }
bool mpGetSpeed(int line,Vec3* position,Vec3* output)
{
    ++speed_calls;
    if(!mpLib_80054ED8(line)) return false;
    *output=(Vec3){position->x*.25f,position->y*-.125f,position->z*.5f};
    return true;
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { j->flags|=JOBJ_MTX_DIRTY; }
void HSD_JObjSetupMatrixSub(HSD_JObj* j) { ++setup_calls; j->flags&=~JOBJ_MTX_DIRTY; }
void HSD_JObjSetFlags(HSD_JObj* j,u32 f) { j->flags|=f; }
void HSD_JObjClearFlags(HSD_JObj* j,u32 f) { j->flags&=~f; }
void HSD_JObjSetFlagsAll(HSD_JObj* j,u32 f)
{ for(;j;j=j->next) { j->flags|=f; if(!(j->flags&JOBJ_INSTANCE)) HSD_JObjSetFlagsAll(j->child,f); } }
void HSD_JObjClearFlagsAll(HSD_JObj* j,u32 f)
{ for(;j;j=j->next) { j->flags&=~f; if(!(j->flags&JOBJ_INSTANCE)) HSD_JObjClearFlagsAll(j->child,f); } }
void HSD_JObjAddAnimAll(HSD_JObj* j,HSD_AnimJoint* a,HSD_MatAnimJoint* m,HSD_ShapeAnimJoint* s)
{ ++adds; last_animation_joint=j; last_anim=a; last_mat=m; last_shape=s; }
void HSD_JObjReqAnimAll(HSD_JObj* j,float frame)
{ ++requests; last_animation_joint=j; last_frame=frame; }
void HSD_JObjAnimAll(HSD_JObj* j) { ++advances; last_animation_joint=j; }
void HSD_JObjRemoveAnimAll(HSD_JObj* j) { ++removes; last_animation_joint=j; }
void C_MTXCopy(Mtx a,Mtx b) { memcpy(b,a,sizeof(Mtx)); }
void C_MTXIdentity(Mtx m) { memset(m,0,sizeof(Mtx)); for(unsigned i=0;i<3;++i)m[i][i]=1; }
void C_MTXConcat(Mtx a,Mtx b,Mtx result)
{
    Mtx output;
    for(unsigned r=0;r<3;++r) for(unsigned c=0;c<4;++c) {
        output[r][c]=a[r][0]*b[0][c]+a[r][1]*b[1][c]+a[r][2]*b[2][c];
        if(c==3)output[r][c]+=a[r][3];
    }
    C_MTXCopy(output,result);
}
void C_MTXMultVec(Mtx m,Vec3* v,Vec3* output)
{
    Vec3 result={m[0][0]*v->x+m[0][1]*v->y+m[0][2]*v->z+m[0][3],
                 m[1][0]*v->x+m[1][1]*v->y+m[1][2]*v->z+m[1][3],
                 m[2][0]*v->x+m[2][1]*v->y+m[2][2]*v->z+m[2][3]};
    *output=result;
}
void __assert(char* file,u32 line,char* condition)
{ fprintf(stderr,"%s:%u %s\n",file,(unsigned)line,condition); abort(); }
#include "resource_geometry_originals.h"

static void reset(void)
{
    nativeArticleResource_Clear();
    memset(objects,0,sizeof(objects)); memset(links,0,sizeof(links));
    memset(models,0,sizeof(models)); memset(aobjs,0,sizeof(aobjs));
    object_count=link_count=model_count=probe_calls=floor_calls=speed_calls=setup_calls=0;
    adds=requests=advances=removes=0;
    fighter=(Fighter){.kind=Ft_Kind_Link,.x8_spawnNum=(s32)0x87654321,.parts=parts};
    creator=(HSD_GObj){.user_data=&fighter};
    parts[0].joint=&attachment;
    table.parts_num=76; tables[Ft_Kind_Link]=&table;
    article=(Article){0};
    article.native_model_root_count=1;
    article.native_model_roots[0].name_hash=0x111; article.native_model_roots[0].root=&model_descriptor;
    article_logic=(NativeFighterArticleLogic){.kind=It_Kind_Link_HShot,.logic=&logic,
        .resource_capacity=2,.resource_reserved_part=-1};
    definition=(NativeFighterDefinition){.article_logic_count=1,.article_logics=&article_logic};
    replacement_definition=definition;
    item=(Item){.kind=It_Kind_Link_HShot,.x1C=0xFEDCBA98,.native_article_author_kind=Ft_Kind_Link,
        .xB8_itemLogicTable=&logic,.xC4_article_data=&article};
    owner=(HSD_GObj){.user_data=&item}; item.entity=&owner;
    item.xDD4_itemVar.native_article.attachment_owner=&creator;
    item.xDD4_itemVar.native_article.creator_spawn_id=(u32)fighter.x8_spawnNum;
    live_heads[HSD_GOBJ_PLINK_ITEM]=&owner; live_heads[HSD_GOBJ_PLINK_FIGHTER]=&creator;
    CHECK(nativeArticleResource_Begin(&owner,2));
    CHECK(nativeArticleResource_CreateNode(&owner,0, &(const NativeArticleResourceInit){0,1,5,{0}}));
    CHECK(nativeArticleResource_LoadModel(&owner,0,0x111));
}

static void stock_geometry_matrix(void)
{
    for(scenario=0;scenario<32;++scenario) {
        reset();
        ItemLink expected={.pos={2*(float)scenario-9,3.5f,-7},.vel={.5f,-.75f,2},.x1CC=7};
        expected.coll_data.cur_pos=(Vec3){-9,4,8};
        expected.coll_data.last_pos=(Vec3){5,6,7};
        links[0].pos=expected.pos; links[0].vel=expected.vel;
        links[0].coll_data=expected.coll_data;
        CHECK(nativeArticleResource_WriteInt(&owner,0,NativeArticleResource_SurfaceLine,expected.x1CC));
        it_802A4420(&expected);
        for(unsigned c=0;c<3;++c) {
            float p=nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)c);
            float v=nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)(c+3));
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)c,p+v));
        }
        unsigned before=probe_calls;
        int flags=it_802A3D90(&expected);
        for(unsigned c=0;c<3;++c) {
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)(c+9),
                  nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)(c+6))));
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)(c+6),
                  nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)c)));
        }
        bool result=nativeArticleResource_SweepStage(&owner,0);
        for(unsigned c=0;c<3;++c) CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)c,
            nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)(c+6))));
        CHECK(probe_calls==before+2 && result==expected.x2C_b1);
        CHECK(flags==(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_EnvironmentFlags)&
                        (Collide_FloorMask|Collide_WallMask)));
        CHECK(memcmp(&expected.pos,&links[0].pos,sizeof(Vec3))==0);
        CHECK(memcmp(&expected.coll_data,&links[0].coll_data,sizeof(CollData))==0);
        it_802A43B8(&expected);
        for(unsigned c=0;c<3;++c) {
            float p=nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)c);
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)(c+6),p));
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)(c+9),p));
        }
        CHECK(memcmp(&expected.coll_data,&links[0].coll_data,sizeof(CollData))==0);
        bool floor_result=mpColl_8004B108(&expected.coll_data);
        CHECK(nativeArticleResource_FollowFloor(&owner,0)==floor_result && floor_calls==2);
        CHECK(memcmp(&expected.coll_data,&links[0].coll_data,sizeof(CollData))==0);
        it_802A43EC(&expected);
        for(unsigned c=0;c<3;++c) {
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)(c+9),
                nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)(c+6))));
            CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)(c+6),
                nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)c)));
        }
        CHECK(memcmp(&expected.coll_data,&links[0].coll_data,sizeof(CollData))==0);
        it_802A4454(&expected);
        Vec3 speed;
        CHECK(nativeArticleResource_ReadLineSpeed(&owner,0,
            nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_SurfaceLine),&speed));
        CHECK(nativeArticleResource_WriteFloat(&owner,0,NativeArticleResource_PositionX,
            nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_PositionX)+speed.x));
        CHECK(nativeArticleResource_WriteFloat(&owner,0,NativeArticleResource_PositionY,
            nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_PositionY)+speed.y));
        CHECK(nativeArticleResource_WriteFloat(&owner,0,NativeArticleResource_PositionZ,
            nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_PositionZ)+speed.z));
        CHECK(memcmp(&expected.pos,&links[0].pos,sizeof(Vec3))==0);
        ++source_cases;
    }
}

static void negative_and_typed_matrix(void)
{
    reset();
    for(unsigned channel=0;channel<NativeArticleResource_FloatCount;++channel) {
        ItemLink before=links[0];
        bool writable=channel<NativeArticleResource_FloorNormalX;
        CHECK(nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)channel,3.5f)==writable);
        if(writable)CHECK(nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)channel)==3.5f);
        else CHECK(memcmp(&before,&links[0],sizeof(before))==0);
        before=links[0];
        CHECK(!nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)channel,NAN));
        CHECK(!nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)channel,INFINITY));
        CHECK(!nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)channel,-INFINITY));
        CHECK(memcmp(&before,&links[0],sizeof(before))==0);
    }
    CHECK(!nativeArticleResource_WriteFloat(&owner,0,(NativeArticleResourceFloat)-1,1));
    CHECK(!nativeArticleResource_WriteFloat(&owner,0,NativeArticleResource_FloatCount,1));
    CHECK(nativeArticleResource_ReadFloat(&owner,0,(NativeArticleResourceFloat)-1)==0);
    for(unsigned channel=0;channel<NativeArticleResource_IntCount;++channel) {
        ItemLink before=links[0];
        CHECK(nativeArticleResource_WriteInt(&owner,0,(NativeArticleResourceInt)channel,(s32)0x81234567)==
              (channel==NativeArticleResource_SurfaceLine));
        if(channel)CHECK(memcmp(&before,&links[0],sizeof(before))==0);
        else CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_SurfaceLine)==(s32)0x81234567);
    }
    CHECK(!nativeArticleResource_WriteInt(&owner,0,(NativeArticleResourceInt)-1,1));
    CHECK(!nativeArticleResource_WriteInt(&owner,0,NativeArticleResource_IntCount,1));
    scenario=1;
    CHECK(nativeArticleResource_SweepStage(&owner,0));
    CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_FloorLine)==(s32)0x81234567);
    CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_LeftWallLine)==3);
    CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_RightWallLine)==4);
    CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_CeilingLine)==5);
    CHECK(nativeArticleResource_ReadInt(&owner,0,(NativeArticleResourceInt)-1)==-1);
    CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_IntCount)==-1);
    CHECK((u32)nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_EnvironmentFlags)==(0x80000000u|Collide_FloorMask));
    CHECK(nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_FloorNormalY)==.8f);
    CHECK(nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_LeftWallNormalX)==1);
    CHECK(nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_RightWallNormalX)==-1);
    CHECK(nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_CeilingNormalY)==-1);
    unsigned calls=probe_calls+floor_calls+speed_calls;
    for(unsigned bad=1;bad<=3;++bad) {
        unsigned index=bad==3?UINT32_MAX:bad;
        CHECK(nativeArticleResource_ReadFloat(&owner,index,NativeArticleResource_PositionX)==0);
        CHECK(!nativeArticleResource_WriteFloat(&owner,index,NativeArticleResource_PositionX,1));
        CHECK(nativeArticleResource_ReadInt(&owner,index,NativeArticleResource_FloorLine)==-1);
        CHECK(nativeArticleResource_ReadInt(&owner,index,NativeArticleResource_EnvironmentFlags)==0);
        CHECK(!nativeArticleResource_WriteInt(&owner,index,NativeArticleResource_SurfaceLine,1));
        CHECK(!nativeArticleResource_SweepStage(&owner,index));
        CHECK(!nativeArticleResource_FollowFloor(&owner,index));
        Vec3 output={9,9,9}; CHECK(!nativeArticleResource_ReadLineSpeed(&owner,index,7,&output));
        CHECK(output.x==0 && output.y==0 && output.z==0);
    }
    CHECK(calls==probe_calls+floor_calls+speed_calls);
    Vec3 output={9,9,9}; CHECK(!nativeArticleResource_ReadLineSpeed(&owner,0,-1,&output));
    CHECK(output.x==0 && output.y==0 && output.z==0);
    CHECK(!nativeArticleResource_ReadLineSpeed(&owner,0,7,NULL));
    ++item.x1C;
    CHECK(!nativeArticleResource_WriteFloat(&owner,0,NativeArticleResource_PositionX,1));
    CHECK(!nativeArticleResource_SweepStage(&owner,0));
    --item.x1C;
    item.native_article_author_kind=Ft_Kind_CLink;
    CHECK(!nativeArticleResource_FollowFloor(&owner,0));
    item.native_article_author_kind=Ft_Kind_Link;
    // Unlinked storage cannot be dereferenced, even if the retained address
    // still matches. Model operations share the same generation validation.
    live_heads[HSD_GOBJ_PLINK_ITEM]=NULL;
    owner.user_data=(void*)1;
    CHECK(nativeArticleResource_ReadFloat(&owner,0,NativeArticleResource_PositionX)==0);
    CHECK(nativeArticleResource_ReadInt(&owner,0,NativeArticleResource_SurfaceLine)==-1);
    CHECK(!nativeArticleResource_WriteInt(&owner,0,NativeArticleResource_SurfaceLine,7));
    CHECK(!nativeArticleResource_RequestAnimation(&owner,0,"",0));
    CHECK(!nativeArticleResource_SweepStage(&owner,0));
    owner.user_data=&item;
    live_heads[HSD_GOBJ_PLINK_ITEM]=&owner;
}

static void attachment_matrix(void)
{
    for(unsigned mode=0;mode<16;++mode) {
        reset();
        Mtx matrix={{2,.25f,1,7},{.5f,3,-.75f,-2},{1,2,4,9}};
        matrix[0][0]+=mode*.125f;
        C_MTXCopy(matrix,attachment.mtx);
        attachment.flags=JOBJ_MTX_DIRTY;
        Vec3 actual,expected; Mtx stock_matrix,read_matrix;
        it_802A2EE4_inline(stock_matrix,&links[0],&expected);
        attachment.flags|=JOBJ_MTX_DIRTY;
        unsigned prior=setup_calls;
        Vec3 local={0,0,6};
        CHECK(nativeArticleResource_ReadAttachmentPoint(&owner,0,&local,&actual));
        CHECK(memcmp(&expected,&actual,sizeof(actual))==0 && setup_calls==prior+1);
        samus_grapple_setup_pos(&links[0],&expected,stock_matrix);
        local=(Vec3){0};
        CHECK(nativeArticleResource_ReadAttachmentPoint(&owner,0,&local,&actual));
        CHECK(memcmp(&expected,&actual,sizeof(actual))==0);
        CHECK(nativeArticleResource_ReadAttachmentMatrix(&owner,0,read_matrix));
        CHECK(memcmp(matrix,read_matrix,sizeof(matrix))==0);
        local=(Vec3){.5f,-2,3}; C_MTXMultVec(matrix,&local,&expected);
        CHECK(nativeArticleResource_ReadAttachmentPoint(&owner,0,&local,&local));
        CHECK(memcmp(&local,&expected,sizeof(local))==0); // Alias safe.
        CHECK(nativeArticleResource_SetWorldMatrix(&owner,0,matrix));
        models[0][0].translate=(Vec3){101,102,103};
        models[0][0].flags|=JOBJ_MTX_DIRTY;
        prior=setup_calls;
        CHECK(nativeArticleResource_ReadWorldMatrix(&owner,0,read_matrix));
        CHECK(memcmp(matrix,read_matrix,sizeof(matrix))==0 && setup_calls==prior);
        CHECK((models[0][0].flags&0x03800000)==0x03800000);
        ++source_cases;
    }
    reset();
    Mtx output; Vec3 point={NAN,0,0},world={9,9,9};
    CHECK(!nativeArticleResource_ReadAttachmentPoint(&owner,0,&point,&world) && world.x==0);
    CHECK(!nativeArticleResource_ReadAttachmentPoint(&owner,0,NULL,&world));
    CHECK(!nativeArticleResource_ReadAttachmentMatrix(&owner,0,NULL));
    CHECK(!nativeArticleResource_ReadWorldMatrix(&owner,0,NULL));
    CHECK(!nativeArticleResource_ReadWorldMatrix(&owner,1,output));
    CHECK(output[0][0]==0 && output[2][3]==0);
    ++fighter.x8_spawnNum;
    CHECK(!nativeArticleResource_ReadAttachmentMatrix(&owner,0,output));
    CHECK(!nativeArticleResource_ReadAttachmentPoint(&owner,0,&(Vec3){0},&world));
    --fighter.x8_spawnNum;
    fighter.kind=Ft_Kind_CLink;
    CHECK(!nativeArticleResource_ReadAttachmentMatrix(&owner,0,output));
    fighter.kind=Ft_Kind_Link;
    unsigned before=setup_calls;
    live_heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
    creator.user_data=(void*)1;
    CHECK(!nativeArticleResource_ReadAttachmentMatrix(&owner,0,output));
    CHECK(!nativeArticleResource_ReadAttachmentPoint(&owner,0,&(Vec3){1,2,3},&world));
    CHECK(setup_calls==before && output[0][0]==0 && world.x==0);
    // Owned resource models remain readable after their creator disappears;
    // only a retained external attachment requires the creator's live rig.
    CHECK(nativeArticleResource_ReadWorldMatrix(&owner,0,output));
    creator.user_data=&fighter;
    live_heads[HSD_GOBJ_PLINK_FIGHTER]=&creator;
    links[0].jobj=NULL;
    CHECK(!nativeArticleResource_ReadAttachmentMatrix(&owner,0,output));
}

static void animation_matrix(void)
{
    for(unsigned mask=0;mask<8;++mask) {
        reset();
        HSD_AnimJoint* a=&anim_descriptor;
        HSD_MatAnimJoint* m=&mat_descriptor;
        HSD_ShapeAnimJoint* s=&shape_descriptor;
        itSamusGrappleAttributes attributes={.x70=&model_descriptor,
            .x98=mask&1?&a:NULL,.x9C=mask&2?&m:NULL,.xA0=mask&4?&s:NULL};
        article.x4_specialAttributes=&attributes;
        HSD_GObj stock={0};
        samus_grapple_setup_tail(&item,&stock);
        CHECK(last_animation_joint==((HSD_JObj*)stock.hsd_obj)->child);
        CHECK(adds==1 && requests==1 && last_frame==0);
        HSD_AnimJoint* expected_anim=last_anim;
        HSD_MatAnimJoint* expected_mat=last_mat;
        HSD_ShapeAnimJoint* expected_shape=last_shape;
        article.native_model_animation_count=1;
        article.native_model_animations[0].name_hash=0x222;
        article.native_model_animations[0].anim=expected_anim;
        article.native_model_animations[0].matanim=expected_mat;
        article.native_model_animations[0].shapeanim=expected_shape;
        CHECK(nativeArticleResource_AddAnimation(&owner,0,"0",0x222));
        CHECK(last_animation_joint==models[0][0].child && last_anim==expected_anim && last_mat==expected_mat && last_shape==expected_shape);
        CHECK(nativeArticleResource_RequestAnimation(&owner,0,"0",0));
        CHECK(last_animation_joint==models[0][0].child && last_frame==0);
        CHECK(nativeArticleResource_AdvanceAnimation(&owner,0,""));
        CHECK(last_animation_joint==models[0] && advances==1);
        CHECK(nativeArticleResource_RemoveAnimation(&owner,0,"0"));
        CHECK(last_animation_joint==models[0][0].child && removes==1);
        // Actual lb rate + WalkTree + AObj setter bodies: selected root is type0,
        // descendants are updated. Instance joints stop that descent.
        models[0][1].flags=mask&1?JOBJ_INSTANCE:0;
        CHECK(nativeArticleResource_SetAnimationRate(&owner,0,"0",2.5f));
        CHECK(aobjs[0][1].framerate==1 && aobjs[0][2].framerate==(mask&1?1:2.5f));
        ++source_cases;
    }
    reset();
    CHECK(!nativeArticleResource_AddAnimation(&owner,0,"",0));
    CHECK(!nativeArticleResource_AddAnimation(&owner,0,"",0x123));
    CHECK(!nativeArticleResource_AddAnimation(&owner,0,"1",0x222));
    CHECK(!nativeArticleResource_RequestAnimation(&owner,0,"0",NAN));
    CHECK(!nativeArticleResource_RequestAnimation(&owner,0,"0",INFINITY));
    CHECK(!nativeArticleResource_SetAnimationRate(&owner,0,"0",NAN));
    CHECK(!nativeArticleResource_SetAnimationRate(&owner,0,"0",-INFINITY));
    CHECK(!nativeArticleResource_AdvanceAnimation(&owner,1,""));
    CHECK(!nativeArticleResource_RemoveAnimation(&owner,0,"0/"));
    CHECK(!nativeArticleResource_SetAnimationRate(&owner,0,NULL,1));
    article.native_model_animation_count=NATIVE_FIGHTER_ARTICLE_MODEL_ANIMATION_LIMIT+1;
    CHECK(!nativeArticleResource_AddAnimation(&owner,0,"",0x222));
    CHECK(adds==0 && requests==0 && advances==0 && removes==0);
}

static void effect_matrix(void)
{
    for (unsigned id = 0; id < 3; ++id) for (unsigned facing = 0; facing < 2; ++facing) {
        reset(); effect_calls=0; effect_mode=0;
        Vec3 point={-7.25f,3.5f,19};
        int32_t effect=id==0?0x3F3:id==1?0x41C:0x3F1;
        bool result=effect==0x3F1
            ? nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,effect,&point,facing?1:-1)
            : nativeArticleResource_SpawnEffectAtPosition(&owner,0,effect,&point);
        CHECK(result && effect_calls==1 && effect_object==objects);
        CHECK(memcmp(&effect_point,&point,sizeof(point))==0);
        if(effect==0x3F1) CHECK(effect_facing==(facing?1:-1));
    }
    for(unsigned i=0;i<sizeof(position_effects)/sizeof(*position_effects);++i) {
        reset(); effect_mode=0; generator_calls=0; Vec3 p={5,-2,3};
        CHECK(nativeArticleResource_SpawnEffectAtPosition(&owner,0,position_effects[i],&p));
        CHECK(generator_calls==1 && memcmp(&generator_point,&p,sizeof(p))==0);
        CHECK(!nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,position_effects[i],&p,1));
    }
    for(int id=0x3EF;id<=0x3F2;++id)for(int f=-1;f<=1;f+=2) {
        reset(); effect_mode=0; generator_calls=0; Vec3 p={7,-9,1};
        CHECK(nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,id,&p,(float)f));
        CHECK(generator_calls==1 && generator_facing==(id==0x3F0 || id==0x3F2?f:-f));
        CHECK(generator_id==(id<0x3F1?0x42:0x14B));
        CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,id,&p));
    }
    reset();effect_mode=0;generator_fail=true;Vec3 failed_point={4,5,6};
    CHECK(nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,&failed_point));
    CHECK(nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,0x3F1,&failed_point,1));
    generator_fail=false;
    const int bounds[]={0,0x24F,30000,30999};
    for(unsigned i=0;i<sizeof(bounds)/sizeof(*bounds);++i) {
        reset(); effect_mode=0; Vec3 p={1,2,3};
        CHECK(nativeArticleResource_SpawnEffectAtPosition(&owner,0,bounds[i],&p));
    }
    reset(); effect_calls=0; effect_mode=0;
    Vec3 point={1,2,3};
    CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,UINT32_MAX,0x3F3,&point));
    CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,1,0x3F3,&point));
    CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,-1,&point));
    CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,NULL));
    CHECK(!nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,0x3F1,&point,NAN));
    for(unsigned c=0;c<3;++c) {
        Vec3 bad=point; ((float*)&bad)[c]=c==0?NAN:INFINITY;
        CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,&bad));
        CHECK(!nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,0x3F1,&bad,1));
    }
    ++item.x1C; CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,&point)); --item.x1C;
    ++fighter.x8_spawnNum; CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,&point)); --fighter.x8_spawnNum;
    item.native_article_author_kind=Ft_Kind_CLink;
    CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,&point)); item.native_article_author_kind=Ft_Kind_Link;
    item.xB8_itemLogicTable=NULL; CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,0x3F3,&point)); item.xB8_itemLogicTable=&logic;
    const int wrong[]={-1,0x250,0x3E8,0x3F5,0x449,0x4CC,0x47C,29999,31000,INT32_MAX};
    for(unsigned i=0;i<sizeof(wrong)/sizeof(*wrong);++i) {
        CHECK(!nativeArticleResource_SpawnEffectAtPosition(&owner,0,wrong[i],&point));
        CHECK(!nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,wrong[i],&point,1));
    }
    CHECK(effect_calls==0);
    for(unsigned mode=1;mode<=4;++mode) {
        reset(); effect_calls=0; effect_mode=mode;
        CHECK(nativeArticleResource_SpawnEffectFacingAtPosition(&owner,0,0x3F1,&point,-1));
        CHECK(effect_calls==1 && objects[0].user_data==NULL);
        CHECK(!nativeArticleResource_Exists(&owner,0));
    }
    effect_mode=0;
}

int main(void)
{
    stock_geometry_matrix(); negative_and_typed_matrix(); attachment_matrix(); animation_matrix(); effect_matrix();
    nativeArticleResource_Clear();
    printf("{\"checks\":%u,\"stock_source_cases\":%u,\"probe_cases\":32,\"attachment_cases\":16,\"animation_cases\":8}\n",checks,source_cases);
    return 0;
}
