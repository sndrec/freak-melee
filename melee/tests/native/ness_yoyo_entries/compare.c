// Generated entry/animation callbacks + actual production resources; unchanged references.
// HSD heap/model/animation/render and active-definition identity are boundaries.
#include "behavior.generated.h"
#include "parameters.h"
#include <Runtime/platform.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/inlines.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/it/native_article.h>
#include <melee/it/native_article_resource.h>
#include <melee/it/itYoyo.h>
#include <melee/it/item.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itnessyoyo.h>
#include <melee/it/kinds/itnessbat.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftNess/ftnessattacks4.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/ft_0BF0.h>
#include <melee/lb/native_model_joint.h>
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
static int32_t fighter_word(Fighter*,const char*);
static Fighter fighters[2]; static HSD_GObj creators[2], articles[2]; static Item items[2];
static Fighter other_fighters[2]; static HSD_GObj other_owners[2];
static HSD_GObj* fighter_heads[32];
HSD_GObj** HSD_GObjPLinkHead=fighter_heads;
static FighterBone bones[2][68]; static HSD_JObj anchor;
static ItemLink links[2][COUNT]; static HSD_GObj objects[2][COUNT];
static HSD_JObj models[2][COUNT][2], creator_models[2];
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
{ unsigned i=object_index(object);check(live[family][i],"single object release");if(object->user_data_remove_func)object->user_data_remove_func(object->user_data);live[family][i]=false;++freed[family];record(20,i,0); }
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
void it_8026B73C(Item_GObj* object)
{ check(object==&articles[family],"resume published YoYo"); Fighter* current=items[family].owner->user_data;bool published=family==0?current->u.ns.yoyo_gobj==object:(uint32_t)fighter_word(current,"yoyo_handle")==items[1].x1C;record(11,0,published); }
enum_t ftLib_GetMotionId(Fighter_GObj* object)
{ return ((Fighter*)object->user_data)->motion_id; }
// Unused queries in the directly linked target unit are explicit traps; the
// fixture accepts only its identity, motion and instance-scale paths.
void ftLib_800866DC(Fighter_GObj* object,Vec3* value){(void)object;(void)value;abort();}
bool ftLib_80086A18(Fighter_GObj* object){(void)object;abort();}
float ftLib_800869D4(Fighter_GObj* object){(void)object;abort();}
float ftCo_80094098(Fighter_GObj* object,float* value){(void)object;(void)value;abort();}
bool it_8026B894(Item_GObj* object,HSD_GObj* other)
{ check(object==&articles[family]&&other==&other_owners[family],"interaction boundary identity");record(12,0,0);return false; }
#define HYPOT(x, y) my_sqrtf((x) * (x) + (y) * (y))
#include "original.h"
#undef memzero

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_Ness?definition:NULL; }
bool nativeCallbackSource_Alive(HSD_GObj* object,uint32_t generation,uint32_t kind)
{
    if(kind==NativeCallbackSource_Fighter)return (object==&creators[1]&&object->user_data==&fighters[1]&&generation==fighters[1].x8_spawnNum)||(object==&other_owners[1]&&object->user_data==&other_fighters[1]&&generation==other_fighters[1].x8_spawnNum);
    return kind==NativeCallbackSource_Item&&object==&articles[1]&&!removed&&object->user_data==&items[1]&&generation==items[1].x1C;
}
bool nativeCallbackSource_Snapshot(HSD_GObj* object,uint32_t kind,uint32_t* id)
{ if(!id)return false;*id=kind==NativeCallbackSource_Fighter?fighters[1].x8_spawnNum:items[1].x1C;return nativeCallbackSource_Alive(object,*id,kind); }
Fighter_GObj* nativeArticle_Creator(Item_GObj* object)
{ return object==&articles[1]&&items[1].xDD4_itemVar.native_article.attachment_owner==&creators[1]&&items[1].xDD4_itemVar.native_article.creator_spawn_id==fighters[1].x8_spawnNum?&creators[1]:NULL; }
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
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags){if(joint){joint->flags&=~flags;if(joint->child)HSD_JObjClearFlagsAll(joint->child,flags);if(joint->next)HSD_JObjClearFlagsAll(joint->next,flags);}}
void HSD_JObjClearFlags(HSD_JObj* joint,u32 flags){(void)joint;(void)flags;abort();}
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags){if(joint){joint->flags|=flags;if(joint->child)HSD_JObjSetFlagsAll(joint->child,flags);if(joint->next)HSD_JObjSetFlagsAll(joint->next,flags);}}
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
    part_table.parts_num=68;part_tables[Ft_Kind_Ness]=&part_table;
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
static bool close_float(float a,float b,const char* label)
{
    float delta=fabsf(a-b);if(delta>maximum_difference)maximum_difference=delta;
    bool equal=isfinite(a)&&isfinite(b)&&delta<=2e-6f*fmaxf(1,fmaxf(fabsf(a),fabsf(b)));
    check(equal,label);return equal;
}
static void compare_vector(Vec3 a,Vec3 b,const char* label)
{close_float(a.x,b.x,label);close_float(a.y,b.y,label);close_float(a.z,b.z,label);}

static uint32_t hash_name(const char* text)
{uint32_t h=2166136261u;while(*text){h^=(unsigned char)*text++;h*=16777619u;}return h;}
static void signal(const char* name,float value)
{const NativeFighterArticleLogic* logic=&definition->article_logics[0];for(unsigned i=0;i<logic->signal_count;++i)if(logic->signals[i].name_hash==hash_name(name)){logic->signals[i].callback(&articles[1],value);return;}abort();}
static bool send_signal(HSD_GObj* source,uint32_t id,uint32_t name,float value)
{check(source==&creators[1]||source==&other_owners[1],"signal live owning fighter");check(id==items[1].x1C,"fullbit signal destination");const NativeFighterArticleLogic* logic=&definition->article_logics[0];for(unsigned i=0;i<logic->signal_count;++i)if(logic->signals[i].name_hash==name){logic->signals[i].callback(&articles[1],value);return true;}return false;}
static bool call_target(uint32_t id,uint32_t name)
{HSD_GObj* target=id==fighters[1].x8_spawnNum?&creators[1]:id==other_fighters[1].x8_spawnNum?&other_owners[1]:NULL;if(!target)return false;for(unsigned i=0;i<definition->callback_export_count;++i)if(definition->callback_exports[i].name_hash==name){definition->callback_exports[i].callback(target);return true;}return false;}
static unsigned fighter_offset(const char* name)
{for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;abort();}
static void set_fighter_word(Fighter* fighter,const char* name,uint32_t bits)
{memcpy(fighter->native_module_state+fighter_offset(name),&bits,4);}
static int32_t fighter_word(Fighter* fighter,const char* name)
{int32_t value;memcpy(&value,fighter->native_module_state+fighter_offset(name),4);return value;}
static void set_spin(float value)
{fighters[0].u.ns.x223C=value;uint32_t bits;memcpy(&bits,&value,4);set_fighter_word(&fighters[1],"yoyo_spin_rate",bits);}
static void resume_host(Item_GObj* object)
{check(object==&articles[1],"authored resume exact live item");it_8026B73C(object);}
static float root_scale(Fighter_GObj* object)
{check(object==&creators[1],"creator root scale query");return ((HSD_JObj*)object->hsd_obj)->scale.y;}
static void item_joint(Item_GObj* object,unsigned model,const char* path,unsigned channel,float value)
{check(object==&articles[1]&&model==0,"Bat model index");nativeModelJoint_Write(nativeModelJoint_Resolve(object->hsd_obj,path),channel,value);}
static void compare_events(void)
{check(counts[0]==counts[1],"same ordered event count");for(unsigned i=0;i<counts[0]&&i<counts[1];++i){check(events[0][i].operation==events[1][i].operation&&events[0][i].node==events[1][i].node,"same event operation/node");close_float(events[0][i].value,events[1][i].value,"same event value");}}
static void seed_nodes(unsigned count,int facing)
{
    reset(count,342,facing,(Vec3){3.5f,-2.25f,0.75f},1.25f);
    param_int("It_Kind_Ness_Yoyo.link_count",count);publish_parameters();
    for(unsigned f=0;f<2;++f){
        creators[f].hsd_obj=&creator_models[f];creator_models[f].scale=(Vec3){2.5f,3.25f,4.75f};
        other_owners[f]=(HSD_GObj){.user_data=&other_fighters[f],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        other_fighters[f]=(Fighter){0};other_fighters[f].x8_spawnNum=0xC0000199;
    }
    memset(fighter_heads,0,sizeof(fighter_heads));
    fighter_heads[HSD_GOBJ_PLINK_FIGHTER]=&creators[1];creators[1].next=&other_owners[1];
    family=0;items[0].xDD4_itemVar.nessyoyo.x10=&creators[0];it_802BE65C(&items[0],&anchor);
    family=1;definition->article_logics[0].on_created(&articles[1]);
    items[0].xDD4_itemVar.nessyoyo.x18=&models[0][count-1][0];
    memset(counts,0,sizeof(counts));
}
static void set_input(Vec3 velocity)
{state_float("input_x",velocity.x);state_float("input_y",velocity.y);state_float("input_z",velocity.z);}
static void compare_nodes(unsigned count)
{
    for(unsigned i=0;i<count;++i){
        compare_vector(links[0][i].pos,links[1][i].pos,"all node positions");
        compare_vector(links[0][i].vel,links[1][i].vel,"all node velocities");
        compare_vector(links[0][i].coll_data.cur_pos,links[1][i].coll_data.cur_pos,"all node current history");
        compare_vector(links[0][i].coll_data.last_pos,links[1][i].coll_data.last_pos,"all node last history");
        compare_vector(links[0][i].coll_data.prev_pos,links[1][i].coll_data.prev_pos,"all node older history");
        check(links[0][i].coll_data.env_flags==links[1][i].coll_data.env_flags,"all node environment unchanged");
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)close_float(models[0][i][0].mtx[r][c],models[1][i][0].mtx[r][c],"entry model world matrix unchanged");
    }
}
static void compare_tip(unsigned count,bool held)
{
    ItemLink* original=&links[0][count-1];
    family=1;
    compare_vector(original->vel,(Vec3){nativeArticleResource_ReadFloat(&articles[1],count-1,NativeArticleResource_VelocityX),nativeArticleResource_ReadFloat(&articles[1],count-1,NativeArticleResource_VelocityY),nativeArticleResource_ReadFloat(&articles[1],count-1,NativeArticleResource_VelocityZ)},"tip entry velocity");
    if(held){
        compare_vector(original->pos,(Vec3){nativeArticleResource_ReadFloat(&articles[1],count-1,NativeArticleResource_PositionX),nativeArticleResource_ReadFloat(&articles[1],count-1,NativeArticleResource_PositionY),nativeArticleResource_ReadFloat(&articles[1],count-1,NativeArticleResource_PositionZ)},"held position");
        compare_vector(original->coll_data.cur_pos,links[1][count-1].coll_data.cur_pos,"held collision current");
        compare_vector(original->coll_data.last_pos,links[1][count-1].coll_data.last_pos,"held collision previous");
        check(original->x2C_b0==(items[1].xDD4_itemVar.native_article.state[state_offset("pinned")+count-1]!=0),"held pin");
    }
    compare_nodes(count);check(items[0].msid==items[1].msid,"same next item phase");compare_events();
}
static unsigned entry_pairs,material_pairs,yoyo_animation_pairs,bat_pairs,interaction_pairs;
static float sqrt_difference;
static void entries(unsigned count,int facing,unsigned variant)
{
    Vec3 input={variant==0?-3.75f:variant==1?0.0f:4.125f,variant==2?-2.5f:1.25f,-0.625f};
    seed_nodes(count,facing);set_input(input);
    // Poison old histories/pose; Held must replace only current/last, preserving prev.
    for(unsigned f=0;f<2;++f){links[f][count-1].pos=(Vec3){8,9,10};links[f][count-1].coll_data.cur_pos=(Vec3){12,13,14};links[f][count-1].coll_data.last_pos=(Vec3){15,16,17};links[f][count-1].coll_data.prev_pos=(Vec3){18,19,20};}
    family=0;it_802C0010(&articles[0],&input);family=1;signal("held",0);compare_tip(count,true);compare_vector(links[0][count-1].coll_data.prev_pos,links[1][count-1].coll_data.prev_pos,"held preserves older history");++entry_pairs;
    memset(counts,0,sizeof(counts));family=0;it_802BFE5C(&articles[0],&input,99);family=1;signal("released",0);compare_tip(count,false);++entry_pairs;
    // Ordinary sqrt vs unchanged native original Newton implementation.
    for(unsigned f=0;f<2;++f)links[f][count-1].pos=(Vec3){3.5f+(variant==0?0:variant==1?1e-4f:123.125f),-2.25f+(variant==0?0:variant==1?3e-4f:-4.125f),5};
    memset(counts,0,sizeof(counts));family=0;it_802BFEC4(&articles[0]);family=1;signal("retract",0);
    float generated;memcpy(&generated,items[1].xDD4_itemVar.native_article.state+state_offset("retract_step"),4);
    float delta=fabsf(generated-items[0].xDD4_itemVar.nessyoyo.x4);if(delta>sqrt_difference)sqrt_difference=delta;
    close_float(items[0].xDD4_itemVar.nessyoyo.x4,generated,"retraction scalar boundary");check(items[0].msid==items[1].msid,"retraction state");compare_nodes(count);compare_events();++entry_pairs;
}
static void material(unsigned count,float frame)
{seed_nodes(count,-1);family=0;it_802BE5D8(&articles[0],frame);family=1;signal("material",frame);compare_events();check(counts[0]==5,"remove/add/rate/request/advance all observed");++material_pairs;}
static void yoyo_animation(unsigned count,unsigned phase,int motion,unsigned owner_mode,bool published,bool child)
{
    seed_nodes(count,1);float spin=phase&1?-0.125f:0.25f;set_spin(spin);
    for(unsigned f=0;f<2;++f){
        fighters[f].motion_id=motion;other_fighters[f].motion_id=motion;
        items[f].msid=phase;items[f].owner=owner_mode==0?&creators[f]:owner_mode==1?&other_owners[f]:NULL;
        fighters[f].u.ns.yoyo_gobj=published?&articles[f]:NULL;other_fighters[f].u.ns.yoyo_gobj=published?&articles[f]:NULL;
        models[f][count-1][1].rotate.x=1.5f;models[f][count-1][0].child=child?&models[f][count-1][1]:NULL;
    }
    set_fighter_word(&fighters[1],"yoyo_handle",published?items[1].x1C:UINT32_MAX);set_fighter_word(&other_fighters[1],"yoyo_handle",published?items[1].x1C:UINT32_MAX);
    family=0;bool a=itNessyoyo_UnkMotion3_Anim(&articles[0]);family=1;bool b=definition->article_logics[0].logic->states[phase].animated(&articles[1]);
    check(a==b,"YoYo animation removal result");close_float(models[0][count-1][1].rotate.x,models[1][count-1][1].rotate.x,"rotation before expiry");
    check((items[0].owner==NULL)==(items[1].owner==NULL),"expiry clears only current owner");
    check(freed[0]==freed[1],"expiry complete reverse resource release");
    check((fighters[0].u.ns.yoyo_gobj!=NULL)==(fighter_word(&fighters[1],"yoyo_handle")!=-1),"creator published handle");
    check((other_fighters[0].u.ns.yoyo_gobj!=NULL)==(fighter_word(&other_fighters[1],"yoyo_handle")!=-1),"current owner published handle");
    compare_events();++yoyo_animation_pairs;
}
static void bat(unsigned phase,int motion,bool published,unsigned owner_mode,unsigned command,bool creator_present)
{
    seed_nodes(2,1);family=1;nativeArticleResource_Clear();memset(counts,0,sizeof(counts));
    for(unsigned f=0;f<2;++f){
        memset(models[f][0],0,sizeof(models[f][0]));models[f][0][0].child=&models[f][0][1];models[f][0][1].parent=&models[f][0][0];
        models[f][0][0].scale=(Vec3){0.75f,1.25f,1.75f};
        articles[f].hsd_obj=&models[f][0][0];items[f].kind=It_Kind_Ness_Bat;items[f].msid=phase;
        fighters[f].motion_id=motion;fighters[f].u.ns.bat_gobj=published?&articles[f]:NULL;
        items[f].xDAC_itcmd_var0=command;items[f].owner=owner_mode==0?&creators[f]:owner_mode==1?&other_owners[f]:NULL;
    }
    items[0].xDD4_itemVar.nessbat.x0=creator_present?&creators[0]:NULL;
    items[1].xDD4_itemVar.native_article.attachment_owner=creator_present?&creators[1]:NULL;
    items[1].xB8_itemLogicTable=definition->article_logics[1].logic;
    set_fighter_word(&fighters[1],"bat_handle",published?items[1].x1C:UINT32_MAX);
    family=0;bool a=phase?itNessbat_UnkMotion1_Anim(&articles[0]):itNessbat_UnkMotion0_Anim(&articles[0]);
    family=1;bool b=definition->article_logics[1].logic->states[phase].animated(&articles[1]);
    check(a==b,"Bat animation removal result");check(models[0][0][1].flags==models[1][0][1].flags,"Bat subtree visibility");
    compare_vector(models[0][0][0].scale,models[1][0][0].scale,"Bat creator root scale");check(models[0][0][0].flags==models[1][0][0].flags,"Bat scale matrix dirty flags");
    check((items[0].owner==NULL)==(items[1].owner==NULL),"Bat current owner detach");
    check((items[0].xDD4_itemVar.nessbat.x0==NULL)==(items[1].xDD4_itemVar.native_article.attachment_owner==NULL),"Bat saved creator detach");
    check((fighters[0].u.ns.bat_gobj!=NULL)==(fighter_word(&fighters[1],"bat_handle")!=-1),"Bat conditional owner forget");
    family=0;if(phase)itNessbat_UnkMotion1_Phys(&articles[0]);else itNessbat_UnkMotion0_Phys(&articles[0]);
    bool ca=phase?itNessbat_UnkMotion1_Coll(&articles[0]):itNessbat_UnkMotion0_Coll(&articles[0]);
    family=1;definition->article_logics[1].logic->states[phase].physics_updated(&articles[1]);check(ca==definition->article_logics[1].logic->states[phase].collided(&articles[1]),"Bat inert physics/collision");++bat_pairs;
}
static unsigned owner_safety_cases;
static void owner_safety(void)
{
    for(unsigned phase=0;phase<4;++phase)for(unsigned mode=0;mode<4;++mode){
        seed_nodes(2,1);family=1;set_spin(0.25f);items[1].msid=phase;
        const uint32_t retained=(uint32_t)other_fighters[1].x8_spawnNum;
        items[1].owner=mode==0?NULL:&other_owners[1];
        other_fighters[1].motion_id=342;
        if(mode==1||mode==2){
            creators[1].next=NULL;
            if(mode==2)other_owners[1].user_data=(void*)1;
        }
        if(mode==3){
            other_fighters[1].x8_spawnNum=(s32)UINT32_C(0xF0000199);
            check(nativeFighterTarget_Motion(retained)==-1,"retained ID rejects recycled generation");
            check(nativeFighterTarget_CurrentOwner(&articles[1])==UINT32_C(0xF0000199),"current raw owner samples replacement identity");
        }
        check(nativeFighterTarget_Motion(nativeFighterTarget_CurrentOwner(&articles[1]))==(mode==3?342:-1),"live-ID owner motion boundary");
        bool expired=definition->article_logics[0].logic->states[phase].animated(&articles[1]);
        check(expired==(mode!=3),"absent/stale owner expires; live replacement continues");
        check(freed[1]==(mode==3?0:2),"safe owner expiry releases complete resource chain");
        check(items[1].owner==(mode==3?&other_owners[1]:NULL),"safe owner expiry detaches raw owner");
        ++owner_safety_cases;
    }
}
static void retract_numeric_matrix(void)
{
    uint32_t input=0x8A0CF319;
    for(unsigned n=0;n<1024;++n){
        seed_nodes(2,1);
        input=input*1664525u+1013904223u;float dx=(float)(int32_t)(input&0xFFFFu)-32768.0f;
        input=input*1664525u+1013904223u;float dy=(float)(int32_t)(input&0xFFFFu)-32768.0f;
        float scale=(float[]){1e-8f,1e-4f,0.125f,1,64}[n%5];dx*=scale;dy*=scale;
        for(unsigned f=0;f<2;++f)links[f][1].pos=(Vec3){3.5f+dx,-2.25f+dy,77};
        family=0;it_802BFEC4(&articles[0]);family=1;signal("retract",0);
        float generated;memcpy(&generated,items[1].xDD4_itemVar.native_article.state+state_offset("retract_step"),4);
        float delta=fabsf(generated-items[0].xDD4_itemVar.nessyoyo.x4);if(delta>sqrt_difference)sqrt_difference=delta;
        close_float(items[0].xDD4_itemVar.nessyoyo.x4,generated,"retraction sampled numeric boundary");compare_nodes(2);compare_events();++entry_pairs;
    }
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
        .item_remove=Item_8026A8EC,.item_change_state_flags=Item_80268E5C,.item_interact=it_8026B894,
        .abs_float=fabsf,.sqrt_float=sqrtf,.article_resource_read_float=nativeArticleResource_ReadFloat,
        .article_resource_write_float=nativeArticleResource_WriteFloat,.article_resource_read_joint=nativeArticleResource_ReadJoint,
        .article_resource_write_joint=nativeArticleResource_WriteJoint,.article_resource_read_attachment_matrix=nativeArticleResource_ReadAttachmentMatrix,
        .item_current_owner=nativeFighterTarget_CurrentOwner,.target_instance_scale_y=nativeFighterTarget_InstanceScaleY,
        .target_motion=nativeFighterTarget_Motion,.target_call=call_target,
        .signal_article=send_signal,.item_resume=resume_host,.fighter_root_scale_y=root_scale,
        .fighter_incapacitated=ftCo_800BF228,.item_set_joint_value=item_joint};
    MotionState motion={0};NativeFighterDefinition base={.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    definition=FreakMelee_GetFighterModule(&base,&host);if(!definition){fprintf(stderr,"module entry rejected Host\n");return 2;}
    const unsigned lengths[]={2,8,90};
    for(unsigned n=0;n<3;++n)for(int facing=-1;facing<=1;facing+=2)for(unsigned v=0;v<3;++v)entries(lengths[n],facing,v);
    for(unsigned n=0;n<3;++n)for(unsigned v=0;v<4;++v)material(lengths[n],(float[]){0,1,0.375f,-0.25f}[v]);
    for(unsigned n=0;n<3;++n)for(unsigned phase=0;phase<4;++phase)for(unsigned m=0;m<8;++m)for(unsigned owner=0;owner<3;++owner)for(unsigned pub=0;pub<2;++pub)for(unsigned child=0;child<2;++child)
        yoyo_animation(lengths[n],phase,(int[]){341,342,343,344,345,346,347,348}[m],owner,pub,child);
    for(unsigned phase=0;phase<2;++phase)for(unsigned m=0;m<5;++m)for(unsigned pub=0;pub<2;++pub)for(unsigned owner=0;owner<3;++owner)for(unsigned cmd=0;cmd<3;++cmd)for(unsigned creator=0;creator<2;++creator)
        bat(phase,(int[]){340,341,342,ftCo_MS_Sleep,ftCo_MS_DeadUpFallHitCameraIce}[m],pub,owner,cmd,creator);
    retract_numeric_matrix();
    owner_safety();
    for(unsigned kind=0;kind<2;++kind){seed_nodes(2,1);family=0;if(kind)itNessBat_Logic58_EvtUnk(&articles[0],&other_owners[0]);else it_2725_Logic59_EvtUnk(&articles[0],&other_owners[0]);family=1;definition->article_logics[kind].logic->evt_unk(&articles[1],&other_owners[1]);compare_events();++interaction_pairs;}
    family=1;nativeArticleResource_Clear();
    pairs=entry_pairs+material_pairs+yoyo_animation_pairs+bat_pairs+interaction_pairs;
    printf("YoYo entries/Bat: %u checks, %u failures, %u source pairs; max difference %.9g; sqrt difference %.9g\n",checks,failures,pairs,maximum_difference,sqrt_difference);
    printf("Coverage: entries%u material%u yoyo_animation%u bat%u interaction%u\n",entry_pairs,material_pairs,yoyo_animation_pairs,bat_pairs,interaction_pairs);
    printf("Owner safety: %u authored-only cases\n",owner_safety_cases);return failures?1:0;
}
