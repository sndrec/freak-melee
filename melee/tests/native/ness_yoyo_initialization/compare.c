// Actual generated initialization + production resource node/model operations.
// HSD heap/model/animation/render and active-definition identity are boundaries.
#include "behavior.generated.h"
#include <Runtime/platform.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/itYoyo.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itnessyoyo.h>
#include <melee/it/kinds/itlinkhookshot.h>
#include <melee/mp/mpcoll.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>
#include <stdio.h>
#include <stdlib.h>

enum { COUNT = 90, EVENTS = 512 };
static unsigned checks, failures, family, counts[2], allocated[2], model_calls[2], draw_calls[2];
static int fail_object=-1, fail_link=-1, fail_model=-1;
static unsigned link_calls[2], freed[2];
static bool removed, live[2][COUNT], link_live[2][COUNT];
static Fighter fighters[2]; static HSD_GObj creators[2], articles[2]; static Item items[2];
static FighterBone bones[2][68]; static HSD_JObj anchor;
static ItemLink links[2][COUNT]; static HSD_GObj objects[2][COUNT];
static HSD_JObj models[2][COUNT][2];
static Article data[2]; static itYoyoAttributes attributes;
static HSD_Joint string_descriptor, tip_descriptor;
static char material_descriptor;
static const NativeFighterDefinition* definition;
static FighterPartsTable part_table; static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=part_tables;
HSD_ObjAllocData item_link_alloc_data;
u8 HSD_GObj_JObjKind;
int mpColl_804D64AC=73;
typedef struct InitEvent { unsigned operation,node; float value; } InitEvent;
static InitEvent events[2][EVENTS];

static void check(bool value,const char* label)
{ ++checks;if(!value&&failures++<15)fprintf(stderr,"%s\n",label); }
static void record(unsigned operation,unsigned node,float value)
{ check(counts[family]<EVENTS,"bounded event buffer");events[family][counts[family]++]=(InitEvent){operation,node,value}; }
static unsigned object_index(HSD_GObj* object)
{ for(unsigned i=0;i<COUNT;++i)if(object==&objects[family][i])return i;abort(); }
static unsigned model_index(HSD_JObj* model)
{ for(unsigned i=0;i<COUNT;++i)if(model==&models[family][i][0])return i;abort(); }
HSD_GObj* GObj_Create(u16 classifier,u8 link,u8 priority)
{
    check(classifier==7&&link==10&&priority==0,"node object construction classes");
    unsigned index=allocated[family]++;if((int)index==fail_object)return NULL;
    check(index<COUNT,"bounded object count");live[family][index]=true;
    memset(&objects[family][index],0,sizeof(HSD_GObj));return &objects[family][index];
}
void* HSD_ObjAlloc(HSD_ObjAllocData* allocator)
{
    check(allocator==&item_link_alloc_data,"node link allocator");
    unsigned index=link_calls[family]++;if((int)index==fail_link)return NULL;
    check(index<COUNT,"bounded link count");link_live[family][index]=true;
    // Poison uninitialized source fields; compare only defined observations.
    memset(&links[family][index],0xA5,sizeof(ItemLink));return &links[family][index];
}
void HSD_ObjFree(HSD_ObjAllocData* allocator,void* value)
{ check(allocator==&item_link_alloc_data,"free link allocator");for(unsigned i=0;i<COUNT;++i)if(value==&links[family][i]){check(link_live[family][i],"single link release");link_live[family][i]=false;memset(value,0xDD,sizeof(ItemLink));return;}abort(); }
void GObj_InitUserData(HSD_GObj* object,u8 type,void(*remove)(void*),void* user)
{ check(type==6&&remove&&user,"typed link userdata");object->user_data=user;object->user_data_remove_func=remove; }
void HSD_GObjFree(HSD_GObj* object)
{ unsigned i=object_index(object);check(live[family][i],"single object release");if(object->user_data_remove_func)object->user_data_remove_func(object->user_data);live[family][i]=false;++freed[family]; }
HSD_JObj* HSD_JObjLoadJoint(HSD_Joint* desc)
{
    unsigned index=model_calls[family]++;if((int)index==fail_model)return NULL;
    check(index<COUNT&&(desc==&string_descriptor||desc==&tip_descriptor),"model source descriptor");
    memset(models[family][index],0,sizeof(models[family][index]));models[family][index][0].child=&models[family][index][1];
    models[family][index][1].parent=&models[family][index][0];record(1,index,desc==&tip_descriptor);return &models[family][index][0];
}
void HSD_GObjObject_80390A70(HSD_GObj* object,u8 type,void* model)
{ check(type==HSD_GObj_JObjKind,"model object type");object->hsd_obj=model;record(2,object_index(object),0); }
void GObj_SetupGXLink(HSD_GObj* object,GObj_RenderFunc callback,u8 link,u32 priority)
{ check(callback&&link==6&&priority==0,"model draw registration");object->render_cb=callback;record(3,object_index(object),0); }
void HSD_GObj_JObjCallback(HSD_GObj* object,int pass)
{ check(object->hsd_obj&&pass==0,"draw live model");++draw_calls[family]; }
void Item_8026AB54(HSD_GObj* object,HSD_GObj* creator,Fighter_Part part)
{ check(object==&articles[family]&&creator==&creators[family]&&part==42,"raw42 attachment");record(4,0,part); }
void HSD_JObjRemoveAnimAll(HSD_JObj* model){record(5,model_index(model),0);}
void HSD_JObjAddAnimAll(HSD_JObj* model,HSD_AnimJoint* joint,HSD_MatAnimJoint* material,HSD_ShapeAnimJoint* shape)
{ check(!joint&&!shape&&material==(HSD_MatAnimJoint*)&material_descriptor,"material-only animation");record(6,model_index(model),0); }
void lb_8000BA0C(HSD_JObj* model,float rate){record(7,model_index(model),rate);}
void HSD_JObjReqAnimAll(HSD_JObj* model,float frame){record(8,model_index(model),frame);}
void HSD_JObjAnimAll(HSD_JObj* model){record(9,model_index(model),0);}
float ftLib_80086A0C(HSD_GObj* object){check(object==&creators[0],"instance scale source");return fighters[0].x34_scale.y;}
Item_GObj* Item_80268B18(SpawnItem* spawn)
{ check(spawn->kind==It_Kind_Ness_Yoyo&&spawn->x0_parent_gobj==&creators[0]&&spawn->x4_parent_gobj2==&creators[0]&&spawn->x44_flag.b0,"YoYo source spawn owner/collision");items[0].pos=spawn->pos;items[0].facing_dir=spawn->facing_dir;return &articles[0]; }
void Item_80268E5C(HSD_GObj* object,enum_t state,Item_StateChangeFlags flags)
{ check(object==&articles[family]&&state==0&&flags==ITEM_ANIM_UPDATE,"YoYo pickup common state");record(10,0,flags); }
void Item_8026A8EC(Item_GObj* object)
{ check(object==&articles[1],"failed authored item removal");removed=true; }

#define memzero(p,n) memset((p),0,(n))
#define GET_ITEM(gobj) ((Item*)(gobj)->user_data)
static const Vec3 it_803B8698={0,0,0};
#include "original.h"
#undef memzero

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_Ness?definition:NULL; }
bool nativeCallbackSource_Alive(HSD_GObj* object,uint32_t generation,uint32_t kind)
{
    if(kind==NativeCallbackSource_Fighter)return object==&creators[1]&&object->user_data==&fighters[1]&&generation==fighters[1].x8_spawnNum;
    return kind==NativeCallbackSource_Item&&object==&articles[1]&&!removed&&object->user_data==&items[1]&&generation==items[1].x1C;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* object,uint32_t kind,uint32_t* id)
{ if(!id)return false;*id=kind==NativeCallbackSource_Fighter?fighters[1].x8_spawnNum:items[1].x1C;return nativeCallbackSource_Alive(object,*id,kind); }
Fighter_GObj* nativeArticle_Creator(Item_GObj* object)
{ return object==&articles[1]&&items[1].xDD4_itemVar.native_article.creator_spawn_id==fighters[1].x8_spawnNum?&creators[1]:NULL; }
static bool alive_host(HSD_GObj* object,uint32_t generation,uint32_t kind)
{ return nativeCallbackSource_Alive(object,generation,(NativeCallbackSourceClass)kind); }
static unsigned bone_count(Fighter* fighter){check(fighter==&fighters[1],"part count source");return 68;}

// These linked operations are deliberately outside initialization. Abort if
// any authored/runtime path unexpectedly starts using them in this fixture.
void C_MTXCopy(Mtx a,Mtx b){(void)a;(void)b;abort();}
void C_MTXMultVec(Mtx m,Vec3* p,Vec3* q){(void)m;(void)p;(void)q;abort();}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint){(void)joint;abort();}
void HSD_JObjSetupMatrixSub(HSD_JObj* joint){(void)joint;abort();}
bool mpColl_800471F8(CollData* collision){(void)collision;abort();}
bool mpColl_8004B108(CollData* collision){(void)collision;abort();}
bool mpGetSpeed(int line,Vec3* position,Vec3* speed){(void)line;(void)position;(void)speed;abort();}
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjClearFlags(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjSetFlags(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void __assert(char* file,u32 line,char* condition){fprintf(stderr,"%s:%u %s\n",file,line,condition);abort();}

// Include the actual immutable snapshot unit, not a duplicate constructor.
#include <melee/it/native_article_resource.c>

static unsigned state_offset(const char* name)
{ const NativeFighterArticleLogic* logic=&definition->article_logics[0];for(unsigned i=0;i<logic->state_field_count;++i)if(!strcmp(logic->state_fields[i].name,name))return logic->state_fields[i].offset;abort(); }
static int32_t state_int(const char* name)
{ int32_t value;memcpy(&value,items[1].xDD4_itemVar.native_article.state+state_offset(name),4);return value; }
static void reset(int count,int motion,float facing,Vec3 position,float scale)
{
    family=1;nativeArticleResource_Clear();
    memset(fighters,0,sizeof(fighters));memset(items,0,sizeof(items));memset(data,0,sizeof(data));
    memset(counts,0,sizeof(counts));memset(allocated,0,sizeof(allocated));memset(link_calls,0,sizeof(link_calls));
    memset(model_calls,0,sizeof(model_calls));memset(live,0,sizeof(live));memset(link_live,0,sizeof(link_live));
    memset(draw_calls,0,sizeof(draw_calls));memset(freed,0,sizeof(freed));removed=false;fail_object=fail_link=fail_model=-1;
    attributes=(itYoyoAttributes){.link_count=count,.string_joint=&string_descriptor,.yoyo_joint=&tip_descriptor,.yoyo_material_animation=(HSD_MatAnimJoint*)&material_descriptor};
    for(unsigned f=0;f<2;++f){
        creators[f]=(HSD_GObj){.user_data=&fighters[f],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        articles[f]=(HSD_GObj){.user_data=&items[f]};items[f].entity=&articles[f];items[f].owner=&creators[f];
        fighters[f].kind=Ft_Kind_Ness;fighters[f].x8_spawnNum=0x80000099;fighters[f].parts=bones[f];fighters[f].x34_scale.y=scale;
        bones[f][42].joint=&anchor;items[f].pos=position;items[f].facing_dir=facing;items[f].kind=It_Kind_Ness_Yoyo;items[f].x1C=0x80000010;
        items[f].native_article_author_kind=Ft_Kind_Ness;items[f].xB8_itemLogicTable=definition->article_logics[0].logic;
        items[f].xDD4_itemVar.native_article.attachment_owner=&creators[f];items[f].xDD4_itemVar.native_article.creator_spawn_id=fighters[f].x8_spawnNum;
        items[f].xDD4_itemVar.native_article.creation_value=motion;items[f].xC4_article_data=&data[f];data[f].x4_specialAttributes=&attributes;
        data[f].native_model_root_count=2;data[f].native_model_roots[0].name_hash=definition->article_logics[0].model_roots[0].name_hash;
        data[f].native_model_roots[0].root=&string_descriptor;data[f].native_model_roots[1].name_hash=definition->article_logics[0].model_roots[1].name_hash;data[f].native_model_roots[1].root=&tip_descriptor;
        data[f].native_model_animation_count=1;data[f].native_model_animations[0].name_hash=definition->article_logics[0].model_animations[0].name_hash;
        data[f].native_model_animations[0].matanim=(HSD_MatAnimJoint*)&material_descriptor;
    }
    part_table.parts_num=68;part_tables[Ft_Kind_Ness]=&part_table;script_parameters[7].i=count;
}
static void compare_success(unsigned count,int motion)
{
    check(items[0].facing_dir==items[1].facing_dir,"motion345 facing reversal");
    check(state_int("source_motion")==motion&&state_int("tip")==count-1&&items[0].xDD4_itemVar.nessyoyo.x0==motion,"owned motion/tip fields");
    check(items[0].xDD4_itemVar.nessyoyo.x10==&creators[0]&&nativeArticle_Creator(&articles[1])==&creators[1],"saved original creator/owned identity");
    check(items[0].xDD4_itemVar.nessyoyo.x8==&links[0][count-1]&&items[0].xDD4_itemVar.nessyoyo.xC==&links[0][0]&&nativeArticleResource_Count(&articles[1])==count,"tail/head publication and resource count");
    check(allocated[0]==count&&allocated[1]==count&&link_calls[0]==count&&link_calls[1]==count,"exact node/link allocation count");
    for(unsigned i=0;i<count;++i){
        ItemLink* a=&links[0][i];ItemLink* b=&links[1][i];
        check(!memcmp(&a->pos,&b->pos,sizeof(Vec3))&&!memcmp(&a->vel,&b->vel,sizeof(Vec3)),"node position/velocity");
        check(!memcmp(&a->coll_data.cur_pos,&b->coll_data.cur_pos,sizeof(Vec3))&&!memcmp(&a->coll_data.last_pos,&b->coll_data.last_pos,sizeof(Vec3)),"initial collision current/last");
        check(!memcmp(&a->coll_data.prev_pos,&b->coll_data.prev_pos,sizeof(Vec3))&&!memcmp(&a->coll_data.x28_vec,&b->coll_data.x28_vec,sizeof(Vec3)),"initial collision previous/x28");
        check(a->coll_data.ecb_source.kind==b->coll_data.ecb_source.kind&&a->coll_data.ecb_source.up==b->coll_data.ecb_source.up&&a->coll_data.ecb_source.down==b->coll_data.ecb_source.down&&a->coll_data.ecb_source.front==b->coll_data.ecb_source.front&&a->coll_data.ecb_source.back==b->coll_data.ecb_source.back,"tip scaled radius/string radius");
        check(a->coll_data.x34_flags.b1234==b->coll_data.x34_flags.b1234&&a->coll_data.x38==b->coll_data.x38&&a->coll_data.floor.index==b->coll_data.floor.index,"collision category/frame/floor");
        check(a->coll_data.x34_flags.b0==b->coll_data.x34_flags.b0&&a->coll_data.x34_flags.b6==b->coll_data.x34_flags.b6&&a->coll_data.x34_flags.b7==b->coll_data.x34_flags.b7&&a->coll_data.x35_flags.b0==b->coll_data.x35_flags.b0&&a->coll_data.env_flags==b->coll_data.env_flags&&a->coll_data.x130_flags==b->coll_data.x130_flags,"defined collision flags");
        check(!memcmp(&a->coll_data.ecb,&b->coll_data.ecb,sizeof(ftECB))&&!memcmp(&a->coll_data.prev_ecb,&b->coll_data.prev_ecb,sizeof(ftECB))&&!memcmp(&a->coll_data.xE4_ecb,&b->coll_data.xE4_ecb,sizeof(ftECB))&&!memcmp(&a->coll_data.desired_ecb,&b->coll_data.desired_ecb,sizeof(ftECB))&&!memcmp(&a->coll_data.x64_ecb,&b->coll_data.x64_ecb,sizeof(ftECB)),"all defined initial ECB shapes");
        check(a->jobj==&anchor&&b->jobj==&anchor&&a->x1CC==b->x1CC,"attachment anchor and initial surface0");
        check(a->next==(i?&links[0][i-1]:NULL)&&b->next==(i?&links[1][i-1]:NULL)&&a->prev==(i+1<count?&links[0][i+1]:NULL)&&b->prev==(i+1<count?&links[1][i+1]:NULL),"exact adjacent node links");
        check(a->x2C_b0==b->x2C_b0&&items[1].xDD4_itemVar.native_article.state[state_offset("pinned")+i]==(i==count-1),"initial pin/draw policy");
        check(!items[1].xDD4_itemVar.native_article.state[state_offset("touching")+i]&&!items[1].xDD4_itemVar.native_article.state[state_offset("contacted")+i],"owned flags initialized");
        family=0;objects[0][i].render_cb(&objects[0][i],0);family=1;objects[1][i].render_cb(&objects[1][i],0);
    }
    check(draw_calls[0]==1&&draw_calls[1]==1,"only tip initially rendered");
    check(counts[0]==counts[1]&&!memcmp(events[0],events[1],counts[0]*sizeof(InitEvent)),"model/attachment/material initialization order");
}
int main(void)
{
    NativeFighterHost host={.callback_source_alive=alive_host,.item_creator=nativeArticle_Creator,.bone_count=bone_count,
        .article_resource_begin=nativeArticleResource_Begin,.article_resource_create=nativeArticleResource_CreateNode,
        .article_resource_load_model=nativeArticleResource_LoadModel,.article_resource_set_draw=nativeArticleResource_SetDraw,
        .article_resource_release=nativeArticleResource_Release,.article_resource_write_int=nativeArticleResource_WriteInt,
        .article_resource_add_animation=nativeArticleResource_AddAnimation,.article_resource_remove_animation=nativeArticleResource_RemoveAnimation,
        .article_resource_set_animation_rate=nativeArticleResource_SetAnimationRate,.article_resource_request_animation=nativeArticleResource_RequestAnimation,
        .article_resource_advance_animation=nativeArticleResource_AdvanceAnimation,.item_attach_to_owner=Item_8026AB54,
        .item_remove=Item_8026A8EC,.item_change_state_flags=Item_80268E5C};
    MotionState motion={0};NativeFighterDefinition base={.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    definition=FreakMelee_GetFighterModule(&base,&host);if(!definition){fprintf(stderr,"module entry rejected fixture Host\n");return 2;}
    script_parameters[0].i=42;script_parameters[1].i=345;script_parameters[2].i=5;script_parameters[3].f=1;script_parameters[4].f=2;script_parameters[5].f=1;script_parameters[6].f=0;
    unsigned scenarios=0;
    const int lengths[]={2,3,20,90};const Vec3 positions[]={{0,0,0},{3.5f,-4,7},{-12,8.25f,-0.5f}};const float scales[]={0.5f,1,1.75f};
    for(unsigned n=0;n<4;++n)for(int motion=341;motion<=347;++motion)for(int dir=-1;dir<=1;dir+=2)for(unsigned p=0;p<3;++p)for(unsigned s=0;s<3;++s){
        reset(lengths[n],motion,dir,positions[p],scales[s]);family=0;
        it_802BE9D8(&creators[0],dir,(Vec3*)&positions[p],motion);itNessYoyo_Logic59_PickedUp(&articles[0]);
        family=1;definition->article_logics[0].on_created(&articles[1]);definition->article_logics[0].logic->picked_up(&articles[1]);
        compare_success(lengths[n],motion);++scenarios;
        nativeArticleResource_Release(&articles[1]);for(unsigned i=0;i<(unsigned)lengths[n];++i)check(!live[1][i]&&!link_live[1][i],"successful resource retirement");
    }
    unsigned failure_cases=0;
    for(unsigned stage=0;stage<3;++stage)for(int index=0;index<20;++index){
        reset(20,345,-1,(Vec3){3,-2,4},1);family=1;
        if(stage==0)fail_object=index;else if(stage==1)fail_link=index;else fail_model=index;
        definition->article_logics[0].on_created(&articles[1]);check(removed,"failed creation removes item");
        for(unsigned i=0;i<COUNT;++i)check(!live[1][i]&&!link_live[1][i],"atomic release after allocation/model failure");++failure_cases;
    }
    family=1;nativeArticleResource_Clear();
    printf("YoYo initialization: %u checks, %u failures, %u source pairs, %u defined failure cases\n",checks,failures,scenarios,failure_cases);return failures?1:0;
}
