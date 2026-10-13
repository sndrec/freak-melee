// Actual generated four solvers + production resource operations; unchanged source reference.
// HSD heap/model/animation/render and active-definition identity are boundaries.
#include "behavior.generated.h"
#include "parameters.h"
#include <Runtime/platform.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/inlines.h>
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
#include <melee/lb/lbvector.h>
#include <dolphin/mtx.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjobject.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/objalloc.h>
#include <stdio.h>
#include <stdlib.h>

enum { COUNT = 90, EVENTS = 4096 };
static unsigned checks, failures, family, counts[2], allocated[2], model_calls[2], draw_calls[2];
static int fail_object=-1, fail_link=-1, fail_model=-1;
static unsigned link_calls[2], freed[2];
static bool removed, live[2][COUNT], link_live[2][COUNT];
static bool remove_during_transition;
static Fighter fighters[2]; static HSD_GObj creators[2], articles[2]; static Item items[2];
static Fighter other_fighters[2]; static HSD_GObj other_owners[2];
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
float ftLib_80086A0C(HSD_GObj* object)
{ check(object==&creators[0]||object==&other_owners[0],"current damage-owner instance scale source");return ((Fighter*)object->user_data)->x34_scale.y; }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{ check(spawn->kind==It_Kind_Ness_Yoyo&&spawn->x0_parent_gobj==&creators[0]&&spawn->x4_parent_gobj2==&creators[0]&&spawn->x44_flag.b0,"YoYo source spawn owner/collision");items[0].pos=spawn->pos;items[0].facing_dir=spawn->facing_dir;return &articles[0]; }
void Item_80268E5C(HSD_GObj* object,enum_t state,Item_StateChangeFlags flags)
{
    check(object==&articles[family]&&state>=0&&state<4&&flags==ITEM_ANIM_UPDATE,"YoYo pickup common state");
    items[family].msid=state;record(10,(unsigned)state,flags);
    if(remove_during_transition){check(family==1&&state==1,"bounded removal at authored retraction completion");removed=true;articles[1].user_data=NULL;}
}
void Item_8026A8EC(Item_GObj* object)
{ check(object==&articles[1],"failed authored item removal");removed=true; }

#define memzero(p,n) memset((p),0,(n))
#define GET_ITEM(gobj) ((Item*)(gobj)->user_data)
static const Vec3 it_803B8698={0,0,0};
const Vec3 it_803B86A4={0,0,1.8f};
#define ASSERTMSGLINE(line, value, message) do { if(!(value)) abort(); } while(0)
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

// Fixed stage oracle: records the actual history entering each ordered sweep.
// Both the original and production resource entry call this same engine boundary.
typedef struct SweepEvent { unsigned node; Vec3 previous,current; } SweepEvent;
static SweepEvent sweeps[2][EVENTS]; static unsigned sweep_count[2], stage_mask;
static int node_of_collision(CollData* collision)
{ for(int i=0;i<COUNT;++i)if(collision==&links[family][i].coll_data)return i;abort(); }
bool mpColl_800471F8(CollData* collision)
{
    unsigned n=(unsigned)node_of_collision(collision),at=sweep_count[family]++;
    check(at<EVENTS,"bounded sweep recorder");
    sweeps[family][at]=(SweepEvent){n,collision->last_pos,collision->cur_pos};
    collision->env_flags=stage_mask;
    if(stage_mask&0xFFF)collision->cur_pos.x+=0.125f;
    if(stage_mask&0x18000)collision->cur_pos.y+=0.25f;
    return stage_mask!=0;
}
bool mpColl_8004B108(CollData* collision){(void)collision;abort();}
bool mpGetSpeed(int line,Vec3* position,Vec3* speed){(void)line;(void)position;(void)speed;abort();}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint){joint->flags|=JOBJ_MTX_DIRTY;}
void HSD_JObjSetupMatrixSub(HSD_JObj* joint){joint->flags&=~JOBJ_MTX_DIRTY;}
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjClearFlags(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjSetFlags(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void __assert(char* file,u32 line,char* condition){fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,condition);abort();}

// Include the actual immutable snapshot unit, not a duplicate constructor.
#include <melee/it/native_article_resource.c>

static unsigned state_offset(const char* name)
{ const NativeFighterArticleLogic* logic=&definition->article_logics[0];for(unsigned i=0;i<logic->state_field_count;++i)if(!strcmp(logic->state_fields[i].name,name))return logic->state_fields[i].offset;abort(); }
static int32_t state_int(const char* name)
{ int32_t value;memcpy(&value,items[1].xDD4_itemVar.native_article.state+state_offset(name),4);return value; }
static void reset(int count,int motion,float facing,Vec3 position,float scale)
{
    remove_during_transition=false;
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
    C_MTXIdentity(anchor.mtx);anchor.mtx[0][0]=0.8f;anchor.mtx[0][1]=-0.6f;
    anchor.mtx[1][0]=0.6f;anchor.mtx[1][1]=0.8f;
    anchor.mtx[0][3]=position.x;anchor.mtx[1][3]=position.y;anchor.mtx[2][3]=position.z;
    anchor.flags=0;
    memset(sweep_count,0,sizeof(sweep_count));
}

static unsigned pairs, bounded_lifetime_cases, phase_pairs[4], branch_count[6];
static float maximum_difference;
static const unsigned geometry_masks[]={0,0x18000,0xFFF,0x18FFF};
static uint32_t parameter_words[sizeof(fixture_parameters)/sizeof(fixture_parameters[0])];
static unsigned parameter_index(const char* name)
{ for(unsigned i=0;i<sizeof(fixture_parameters)/sizeof(fixture_parameters[0]);++i)if(!strcmp(name,fixture_parameters[i].name))return i;abort(); }
static void param_float(const char* name,float value)
{ memcpy(&parameter_words[parameter_index(name)],&value,4); }
static void param_int(const char* name,int value)
{ memcpy(&parameter_words[parameter_index(name)],&value,4); }
static void publish_parameters(void)
{ check(FreakMelee_SetFighterParameters(parameter_words,sizeof(parameter_words)/4,FIXTURE_PARAMETER_SCHEMA),"actual generated owned parameter entry"); }
static void state_float(const char* name,float value)
{ memcpy(items[1].xDD4_itemVar.native_article.state+state_offset(name),&value,4); }
static void state_bool_index(const char* name,unsigned index,bool value)
{ items[1].xDD4_itemVar.native_article.state[state_offset(name)+index]=value; }
static void fighter_hitbox(Vec3 value)
{
    fighters[0].u.ns.yoyo_hitbox_pos=value;
    for(unsigned i=0;i<definition->state_field_count;++i){
        const NativeFighterStateField* field=&definition->state_fields[i];
        if(!strcmp(field->name,"yoyo_hitbox_x"))memcpy(fighters[1].native_module_state+field->offset,&value.x,4);
        if(!strcmp(field->name,"yoyo_hitbox_y"))memcpy(fighters[1].native_module_state+field->offset,&value.y,4);
        if(!strcmp(field->name,"yoyo_hitbox_z"))memcpy(fighters[1].native_module_state+field->offset,&value.z,4);
    }
}
static Vec3 authored_hitbox(void)
{
    Vec3 value={0};for(unsigned i=0;i<definition->state_field_count;++i){
        const NativeFighterStateField* field=&definition->state_fields[i];
        if(!strcmp(field->name,"yoyo_hitbox_x"))memcpy(&value.x,fighters[1].native_module_state+field->offset,4);
        if(!strcmp(field->name,"yoyo_hitbox_y"))memcpy(&value.y,fighters[1].native_module_state+field->offset,4);
        if(!strcmp(field->name,"yoyo_hitbox_z"))memcpy(&value.z,fighters[1].native_module_state+field->offset,4);
    }return value;
}
static uint32_t current_owner(Item_GObj* object)
{
    check(object==&articles[1],"current-owner query source identity");
    if(items[1].owner==&creators[1])return fighters[1].x8_spawnNum;
    if(items[1].owner==&other_owners[1])return other_fighters[1].x8_spawnNum;
    return UINT32_MAX;
}
static float target_instance_scale(uint32_t id)
{
    if(id==fighters[1].x8_spawnNum)return fighters[1].x34_scale.y;
    if(id==other_fighters[1].x8_spawnNum&&other_owners[1].user_data==&other_fighters[1])return other_fighters[1].x34_scale.y;
    return 1;
}
static bool close_float(float a,float b,const char* label)
{
    float delta=fabsf(a-b);if(delta>maximum_difference)maximum_difference=delta;
    bool equal=isfinite(a)&&isfinite(b)&&delta<=2e-6f*fmaxf(1,fmaxf(fabsf(a),fabsf(b)));
    check(equal,label);return equal;
}
static void compare_vector(Vec3 a,Vec3 b,const char* label)
{close_float(a.x,b.x,label);close_float(a.y,b.y,label);close_float(a.z,b.z,label);}
static void seed(unsigned count,unsigned phase,unsigned variant,unsigned mask,int facing,int motion,bool changed_owner)
{
    float scale=variant&1?1.75f:0.5f;
    reset(count,motion,facing,(Vec3){3.5f,-2.25f,0.75f},scale);
    param_int("It_Kind_Ness_Yoyo.link_count",(int)count);
    attributes.up_smash_charge_link_count=variant%3?4:1;attributes.down_smash_charge_link_count=variant%3?6:2;
    attributes.link_length=variant&1?3.0f:1.25f;attributes.minimum_link_length=variant&1?1.0f:0.25f;
    attributes.horizontal_acceleration=0.12f;attributes.maximum_horizontal_speed=2.0f;
    attributes.gravity=0.2f;attributes.terminal_velocity=3.0f;attributes.link_gravity=0.15f;
    param_int("It_Kind_Ness_Yoyo.up_charge_count",attributes.up_smash_charge_link_count);
    param_int("It_Kind_Ness_Yoyo.down_charge_count",attributes.down_smash_charge_link_count);
    param_float("It_Kind_Ness_Yoyo.link_length",attributes.link_length);
    param_float("It_Kind_Ness_Yoyo.minimum_link_length",attributes.minimum_link_length);
    param_float("It_Kind_Ness_Yoyo.horizontal_acceleration",attributes.horizontal_acceleration);
    param_float("It_Kind_Ness_Yoyo.maximum_horizontal_speed",attributes.maximum_horizontal_speed);
    param_float("It_Kind_Ness_Yoyo.gravity",attributes.gravity);
    param_float("It_Kind_Ness_Yoyo.terminal_velocity",attributes.terminal_velocity);
    param_float("It_Kind_Ness_Yoyo.link_gravity",attributes.link_gravity);publish_parameters();
    family=0;it_802BE9D8(&creators[0],facing,&items[0].pos,motion);itNessYoyo_Logic59_PickedUp(&articles[0]);
    family=1;definition->article_logics[0].on_created(&articles[1]);definition->article_logics[0].logic->picked_up(&articles[1]);
    for(unsigned f=0;f<2;++f){
        other_fighters[f]=(Fighter){.x8_spawnNum=0xC0000199};other_fighters[f].x34_scale.y=2.25f;
        other_owners[f]=(HSD_GObj){.user_data=&other_fighters[f],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        if(changed_owner)items[f].owner=&other_owners[f];
        items[f].msid=phase;
        for(unsigned i=0;i<count;++i){
            bool pinned=i==count-1||(variant%3==1)||(variant%3==2&&i%2==0);
            links[f][i].x2C_b0=pinned;links[f][i].x2C_b1=variant%2;links[f][i].x2C_b2=i%3==0;
            float spacing=variant%3==0?0.1f:variant%3==1?2.0f:8.0f;
            links[f][i].pos=(Vec3){3.5f+(count-1-i)*spacing,-2.25f+(int)(i%3)-1,0.75f+0.2f*(i%2)};
            links[f][i].vel=(Vec3){variant%3==0?0.5f:variant%3==1?-4.0f:2.0f,variant%3==0?-0.75f:variant%3==1?4.0f:-3.0f,0.1f};
            links[f][i].coll_data.cur_pos=(Vec3){-0.5f+i,1.25f,2.5f};
            links[f][i].coll_data.last_pos=(Vec3){7,8,9};
            if(f==1){state_bool_index("pinned",i,pinned);state_bool_index("touching",i,variant%2);state_bool_index("contacted",i,i%3==0);}
        }
    }
    Vec3 hitbox={4.0f,-2.0f,1.25f};if(variant%3==0)hitbox=(Vec3){3.5f,-2.25f,0.75f};fighter_hitbox(hitbox);
    float retract_step=variant%3==0?0.0f:variant%3==1?0.5f:1000.0f;
    items[0].xDD4_itemVar.nessyoyo.x4=retract_step;state_float("retract_step",retract_step);
    stage_mask=geometry_masks[mask];memset(sweep_count,0,sizeof(sweep_count));memset(counts,0,sizeof(counts));
    remove_during_transition=false;
}
static void compare_solver(unsigned count,unsigned phase)
{
    bool okay=true;
    for(unsigned i=0;i<count;++i){
        ItemLink* a=&links[0][i];ItemLink* b=&links[1][i];
        compare_vector(a->pos,b->pos,"node solver position");compare_vector(a->vel,b->vel,"node solver velocity");
        compare_vector(a->coll_data.cur_pos,b->coll_data.cur_pos,"solver collision current history");
        compare_vector(a->coll_data.last_pos,b->coll_data.last_pos,"solver collision previous history");
        check(a->coll_data.env_flags==b->coll_data.env_flags,"solver environment flags");
        check(a->x2C_b0==b->x2C_b0&&a->x2C_b0==items[1].xDD4_itemVar.native_article.state[state_offset("pinned")+i],"pin and draw flags");
        check(a->x2C_b1==items[1].xDD4_itemVar.native_article.state[state_offset("touching")+i],"owned touching flag");
        check(a->x2C_b2==items[1].xDD4_itemVar.native_article.state[state_offset("contacted")+i],"owned contact flag");
        HSD_JObj* left=&models[0][i][0];HSD_JObj* right=&models[1][i][0];
        compare_vector(left->translate,right->translate,"model translation");
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)okay=close_float(left->mtx[r][c],right->mtx[r][c],"model world matrix")&&okay;
        check(left->flags==right->flags,"model matrix lifetime and dirty flags");
    }
    compare_vector(fighters[0].u.ns.yoyo_hitbox_pos,authored_hitbox(),"saved creator hitbox publication");
    check(items[0].msid==items[1].msid,"retraction completion state");
    check(sweep_count[0]==sweep_count[1],"exact number of sweeps");
    unsigned sweep=sweep_count[0]<sweep_count[1]?sweep_count[0]:sweep_count[1];
    for(unsigned i=0;i<sweep;++i){
        check(sweeps[0][i].node==sweeps[1][i].node,"exact sweep node order including repeated current node");
        compare_vector(sweeps[0][i].previous,sweeps[1][i].previous,"ordered sweep previous input");
        compare_vector(sweeps[0][i].current,sweeps[1][i].current,"ordered sweep current input");
    }
    check(counts[0]==counts[1]&&!memcmp(events[0],events[1],counts[0]*sizeof(InitEvent)),"common transition ordering");
    (void)okay;++phase_pairs[phase];++pairs;
    if(phase==2){
        if(sweep_count[0]>1)++branch_count[0];else ++branch_count[1];
        if(stage_mask&0x18000)++branch_count[2];
        if(stage_mask&0xFFF)++branch_count[3];
    }
    if(phase==3){if(items[0].msid==1)++branch_count[4];else ++branch_count[5];}
}
static void compare_removed_transition(unsigned count)
{
    seed(count,3,2,0,-1,345,false);
    remove_during_transition=true;family=1;
    definition->article_logics[0].logic->states[3].physics_updated(&articles[1]);
    check(removed&&articles[1].user_data==NULL,"state transition removes source generation");
    check(counts[1]==1&&events[1][0].operation==10&&events[1][0].node==1,"one completion transition before removal");
    compare_vector(links[1][count-1].pos,(Vec3){3.5f,-2.25f,0.75f},"no saved-hitbox overwrite after removal");
    for(unsigned i=0;i<count;++i){Mtx zero={0};check(!memcmp(models[1][i][0].mtx,zero,sizeof(Mtx)),"no model continuation after removal");}
    ++bounded_lifetime_cases;
}
static void compare_missing_owner(unsigned mode)
{
    seed(8,0,1,0,1,342,true);
    other_fighters[0].x34_scale.y=1;
    if(mode==0)items[1].owner=NULL;
    if(mode==1)other_owners[1].user_data=NULL;
    if(mode==2)other_owners[1].user_data=(void*)(uintptr_t)1;
    if(mode==3)items[1].owner=(HSD_GObj*)(uintptr_t)1;
    family=0;itNessyoyo_UnkMotion0_Phys(&articles[0]);
    family=1;definition->article_logics[0].logic->states[0].physics_updated(&articles[1]);
    for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
        close_float(models[0][7][0].mtx[r][c],models[1][7][0].mtx[r][c],"absent/dead/poisoned current-owner scale defaults one");
    ++bounded_lifetime_cases;
}
int main(void)
{
    for(unsigned i=0;i<sizeof(parameter_words)/4;++i)parameter_words[i]=fixture_parameters[i].bits;
    NativeFighterHost host={.callback_source_alive=alive_host,.item_creator=nativeArticle_Creator,.bone_count=bone_count,
        .article_resource_begin=nativeArticleResource_Begin,.article_resource_create=nativeArticleResource_CreateNode,
        .article_resource_load_model=nativeArticleResource_LoadModel,.article_resource_set_draw=nativeArticleResource_SetDraw,
        .article_resource_release=nativeArticleResource_Release,.article_resource_write_int=nativeArticleResource_WriteInt,
        .article_resource_add_animation=nativeArticleResource_AddAnimation,.article_resource_remove_animation=nativeArticleResource_RemoveAnimation,
        .article_resource_set_animation_rate=nativeArticleResource_SetAnimationRate,.article_resource_request_animation=nativeArticleResource_RequestAnimation,
        .article_resource_advance_animation=nativeArticleResource_AdvanceAnimation,.item_attach_to_owner=Item_8026AB54,
        .item_remove=Item_8026A8EC,.item_change_state_flags=Item_80268E5C,
        .abs_float=fabsf,.sqrt_float=sqrtf,.vector_normalize=lbVector_Normalize,
        .article_resource_read_float=nativeArticleResource_ReadFloat,.article_resource_write_float=nativeArticleResource_WriteFloat,
        .article_resource_read_int=nativeArticleResource_ReadInt,.article_resource_write_joint=nativeArticleResource_WriteJoint,
        .article_resource_set_world_matrix=nativeArticleResource_SetWorldMatrix,.article_resource_sweep_stage=nativeArticleResource_SweepStage,
        .article_resource_read_attachment_matrix=nativeArticleResource_ReadAttachmentMatrix,.article_resource_read_attachment_point=nativeArticleResource_ReadAttachmentPoint,
        .item_current_owner=current_owner,.target_instance_scale_y=target_instance_scale};
    MotionState motion={0};NativeFighterDefinition base={.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    definition=FreakMelee_GetFighterModule(&base,&host);if(!definition){fprintf(stderr,"module entry rejected fixture Host\n");return 2;}
    const unsigned lengths[]={2,8,90};
    HSD_GObjEvent original_physics[]={itNessyoyo_UnkMotion0_Phys,itNessyoyo_UnkMotion1_Phys,itNessyoyo_UnkMotion2_Phys,itNessyoyo_UnkMotion3_Phys};
    for(unsigned n=0;n<3;++n)for(unsigned phase=0;phase<4;++phase)for(unsigned variant=0;variant<6;++variant)
    for(unsigned mask=0;mask<4;++mask)for(int facing=-1;facing<=1;facing+=2)for(int motion=342;motion<=345;motion+=3)
    for(unsigned changed=0;changed<2;++changed){
        seed(lengths[n],phase,variant,mask,facing,motion,changed!=0);
        family=0;original_physics[phase](&articles[0]);
        family=1;definition->article_logics[0].logic->states[phase].physics_updated(&articles[1]);
        unsigned before=failures;compare_solver(lengths[n],phase);
        if(failures!=before){fprintf(stderr,"case count%u phase%u variant%u mask%x facing%d motion%d changed%u\n",lengths[n],phase,variant,geometry_masks[mask],facing,motion,changed);return 1;}
    }
    // Repeated frames exercise previous poses/history and the completed
    // retraction's next callback selection, rather than only isolated seeds.
    for(unsigned n=0;n<3;++n)for(unsigned initial=1;initial<4;++initial)for(unsigned changed=0;changed<2;++changed){
        seed(lengths[n],initial,5,0,-1,342,changed!=0);
        for(unsigned frame=0;frame<12;++frame){
            unsigned phase=items[0].msid;check(phase==items[1].msid&&phase<4,"live phase selection in repeated frames");
            stage_mask=geometry_masks[frame%4];memset(sweep_count,0,sizeof(sweep_count));memset(counts,0,sizeof(counts));
            family=0;original_physics[phase](&articles[0]);
            family=1;definition->article_logics[0].logic->states[phase].physics_updated(&articles[1]);
            unsigned before=failures;compare_solver(lengths[n],phase);
            if(failures!=before){fprintf(stderr,"trajectory count%u initial%u changed%u frame%u phase%u\n",lengths[n],initial,changed,frame,phase);return 1;}
        }
    }
    // Pure vertical and coincident-node directions cover the orientation
    // helper's alternate basis choices and its defined zero normalization.
    for(unsigned n=0;n<3;++n)for(unsigned phase=0;phase<4;++phase)for(unsigned axis=0;axis<3;++axis){
        seed(lengths[n],phase,1,0,1,342,false);
        for(unsigned f=0;f<2;++f)for(unsigned i=0;i<lengths[n];++i){
            float direction=axis==0?1.0f:axis==1?-1.0f:0.0f;
            links[f][i].pos=(Vec3){3.5f,-2.25f+direction*2.0f*(lengths[n]-1-i),0.75f};
            links[f][i].vel=(Vec3){0};links[f][i].x2C_b0=true;
            if(f==1)state_bool_index("pinned",i,true);
        }
        fighter_hitbox(links[0][lengths[n]-1].pos);
        items[0].xDD4_itemVar.nessyoyo.x4=0;state_float("retract_step",0);
        family=0;original_physics[phase](&articles[0]);
        family=1;definition->article_logics[0].logic->states[phase].physics_updated(&articles[1]);
        unsigned before=failures;compare_solver(lengths[n],phase);
        if(failures!=before){fprintf(stderr,"axis case count%u phase%u axis%u\n",lengths[n],phase,axis);return 1;}
    }
    for(unsigned i=0;i<6;++i)check(branch_count[i]!=0,"meaningful released/retracted/floor/wall branches entered");
    compare_removed_transition(2);compare_removed_transition(90);
    for(unsigned mode=0;mode<4;++mode)compare_missing_owner(mode);
    family=1;nativeArticleResource_Clear();
    printf("YoYo solvers: %u checks, %u failures, %u source pairs, %u lifetime cases; max difference %.9g\n",checks,failures,pairs,bounded_lifetime_cases,maximum_difference);
    printf("Phase pairs: %u %u %u %u\n",phase_pairs[0],phase_pairs[1],phase_pairs[2],phase_pairs[3]);return failures?1:0;
}
