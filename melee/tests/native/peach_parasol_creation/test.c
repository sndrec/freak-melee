#include <melee/ft/fighter.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_events.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftPeach/ftpeach.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialhi.h>
#include <melee/ft/kinds/ftPeach/ftpeachspecialn.h>
#include <melee/ft/kinds/ftPeach/ftpeachspeciallw.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_article.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/it_3F14.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/db/db.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_26B1.h>
#include <melee/it/it_2725.h>
#include <melee/it/kinds/itpeachparasol.h>
#include <melee/it/kinds/itpeachtoad.h>
#include <melee/it/kinds/itpeachturnip.h>
#include <melee/it/itCharItems.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

enum Role { Parasol,Toad,Vegetable,Old,Created,Count };
enum Step { Restore,ForgetToad,ForgetVeg,Hide,Show,Pause,Resume,DropCallback,DropStats,PickupCallback,PickupStats,Free,Position,Allocate,PickupSound,PoseClear,PoseSet,Dirty,DisableHit,AttachJoint,AttachmentUpdate,AnimationChange,AnimationAdvance,StopSound,Debug };
typedef struct CleanupEvent { int step,role; } CleanupEvent;
enum Mutation { None,RemoveSource,RecycleSource,ChangeKind,ChangeDefinition,RemoveOld,RecycleOld,RemoveParasol,RecycleParasol,RemoveCreated,RecycleCreated };
static CleanupEvent events[96],expected[96];static unsigned event_count,expected_count,checks,pairs,safety;
static HSD_GObj source,foreign,objects[Count],*heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static Fighter fighter,other;static Item items[Count];static HSD_JObj joints[Count],bone_joint,child_joint;
static FighterBone bones[MAX_FT_PARTS];static FighterPartsTable rig;
static HSD_Joint descriptor,child_descriptor;static ItemModelDesc model;static Article asset;
static ItemCommonData common;ItemCommonData* it_804D6D28=&common;
static SpawnItem observed;static bool fail_allocation,missing_assets,drop_mutates_pose,mutate_attachment;static uint32_t creation_id;
static u8 debug_display;static int db_ShowItemCollisionBubbles;
static float facing_value;

static int32_t get(const char* name);
static bool scripted,observe;static enum Mutation mutation;static bool mutate_pickup;
static const NativeFighterDefinition *definition,*registered;static NativeFighterDefinition replacement;
static FighterPartsTable* rigs[Ft_Kind_RuntimeCapacity];FighterPartsTable** ftPartsTable=rigs;
Fighter_ItemEvent ftData_OnItemPickupExt[Ft_Kind_RuntimeCapacity],ftData_OnItemDropExt[Ft_Kind_RuntimeCapacity];
static ItemLogicTable stock_logics[Count];
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"pair %u safety %u line %d: %s\n",pairs,safety,__LINE__,#x);exit(1);} } while(0)
static int role(HSD_GObj* g){if(!g)return -1;for(int i=0;i<Count;i++)if(g==&objects[i])return i;return -2;}
__attribute__((no_instrument_function)) static void record(int step,int role)
{CHECK(event_count<96);events[event_count++]=(CleanupEvent){step,role};}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind==Ft_Kind_CustomFirst ? registered : kind==Ft_Kind_CustomFirst+1 ? definition : NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation){(void)g;*generation=0;return false;}
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation){(void)g;(void)generation;return NULL;}
void __assert(char* file,u32 line,char* text){fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,text);abort();}
void OSReport(char* format,...){va_list args;va_start(args,format);vfprintf(stderr,format,args);va_end(args);}
HSD_GObj* ftLib_8008741C(u32 id)
{for(HSD_GObj* g=heads[HSD_GOBJ_PLINK_FIGHTER];g;g=g->next)if(g->user_data&&((Fighter*)g->user_data)->x8_spawnNum==id)return g;return NULL;}
u32 ftLib_80087460(HSD_GObj* g){return ((Fighter*)g->user_data)->x8_spawnNum;}
static uint32_t current_owner(Item_GObj* g)
{HSD_GObj* owner=((Item*)g->user_data)->owner;uint32_t id;return nativeCallbackSource_Snapshot(owner,NativeCallbackSource_Fighter,&id)?id:0;}
static void unlink_item(HSD_GObj* g)
{HSD_GObj** link=&heads[HSD_GOBJ_PLINK_ITEM];while(*link&&*link!=g)link=&(*link)->next;if(*link)*link=g->next;g->next=NULL;}
static void invalidate(void)
{
    if(mutation==RemoveSource){heads[HSD_GOBJ_PLINK_FIGHTER]=&foreign;source.user_data=(void*)(uintptr_t)1;}
    else if(mutation==RecycleSource)++fighter.x8_spawnNum;
    else if(mutation==ChangeKind)fighter.kind=Ft_Kind_CustomFirst+1;
    else if(mutation==ChangeDefinition)registered=&replacement;
    else if(mutation==RemoveOld){unlink_item(&objects[Old]);objects[Old].user_data=(void*)(uintptr_t)1;}
    else if(mutation==RecycleOld)++items[Old].x1C;
    else if(mutation==RemoveParasol){unlink_item(&objects[Parasol]);objects[Parasol].user_data=(void*)(uintptr_t)1;}
    else if(mutation==RecycleParasol)items[Parasol].x1C=UINT32_C(0x61234567);
    else if(mutation==RemoveCreated){unlink_item(&objects[Created]);objects[Created].user_data=(void*)(uintptr_t)1;}
    else if(mutation==RecycleCreated){items[Created].x1C=UINT32_C(0x61234567);items[Created].owner=&foreign;items[Created].native_article_author_kind=Ft_Kind_None;}
}
static void drop_callback(HSD_GObj* g,bool normal){CHECK(g==&source&&normal);record(DropCallback,role(fighter.item_gobj));if(!mutate_pickup&&!mutate_attachment)invalidate();if(drop_mutates_pose){bone_joint.translate=(Vec3){99,88,77};fighter.facing_dir=-fighter.facing_dir;}}
static void pickup_callback(HSD_GObj* g,bool normal){CHECK((g==&source||g==&foreign)&&normal);record(PickupCallback,role(((Fighter*)g->user_data)->item_gobj));if(mutate_pickup)invalidate();}
bool ftCo_800C5240(HSD_GObj* g){(void)g;return false;}
void ftCo_800C5500(HSD_GObj* g){(void)g;CHECK(false);}
void pl_8003EA08(int player,int flags){CHECK(player==0&&flags==0);record(DropStats,role(fighter.item_gobj));}
void pl_8003E17C(int player,int flags,Item_GObj* g){CHECK(player==0&&flags==0);record(PickupStats,role(g));}
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags){CHECK(flags==JOBJ_HIDDEN);if(joint==&child_joint){child_joint.flags&=~flags;record(Show,Parasol);return;}for(int i=0;i<Count;i++)if(joint==&joints[i]){joint->flags&=~flags;record(Show,i);return;}CHECK(false);}
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags){CHECK(flags==JOBJ_HIDDEN);for(int i=0;i<Count;i++)if(joint==&joints[i]){joint->flags|=flags;record(Hide,i);return;}CHECK(false);}
// Unrelated allocation, attachment, scale and removal housekeeping boundaries.
Item_GObj* Item_CreateEngine(SpawnItem* spawn){(void)spawn;CHECK(false);return NULL;}
void it_80274594(Item_GObj* g){(void)g;CHECK(false);}
bool it_80272D1C(HSD_GObj* g){uint32_t id;return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Item,&id);}
void it_802725D4(HSD_GObj* g){(void)g;}
static void Item_802675A8(HSD_GObj* g){(void)g;}
static void func_8026A8EC_inline1(HSD_GObj* g){(void)g;}
void efLib_DestroyAll(HSD_GObj* g){(void)g;}
static void ItemSwitch(HSD_GObj* g){(void)g;}
bool ftLib_80086960(HSD_GObj* g){uint32_t id;return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Fighter,&id);}
u8 ftLib_80086BE0(HSD_GObj* g){CHECK(g==&source);return 2;}
s32 ftLib_80086EB4(HSD_GObj* g){CHECK(g==&source);return 3;}
u8 fn_8022697C(Fighter_GObj* g){CHECK(g==&source);return debug_display;}
uint32_t nativeFighterTarget_Id(HSD_GObj* g){uint32_t id;return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Fighter,&id)?id:0;}
Article* nativeFighterRuntime_ArticleAssets(FighterKind kind,ItemKind item){CHECK(kind==fighter.kind&&item==It_Kind_Peach_Parasol);return missing_assets?NULL:&asset;}
void it_8026BB68(HSD_GObj* g,Vec3* p){(void)g;(void)p;CHECK(false);}
static unsigned bone_count(Fighter* fp){CHECK(fp==&fighter);return 114;}
void lb_8000B1CC(HSD_JObj* joint,Vec3* offset,Vec3* position){CHECK(joint==&bone_joint&&offset==NULL);*position=bone_joint.translate;record(Position,109);}
void HSD_JObjClearFlags(HSD_JObj* j,u32 f){record(PoseClear,j==&joints[Created]?0:1);j->flags&=~f;}
void HSD_JObjSetFlags(HSD_JObj* j,u32 f){record(PoseSet,j==&joints[Created]?0:1);j->flags|=f;}
void HSD_JObjSetMtxDirtySub(HSD_JObj* j){record(Dirty,Created);j->flags|=JOBJ_MTX_DIRTY;}
void Item_8026AE84(Item* ip,enum_t sound,u8 pan,u8 volume){CHECK(ip==&items[Created]&&sound==45&&pan==127&&volume==64);record(PickupSound,Created);}
int lbAudioAx_800236B8(int sound){CHECK(sound==63);record(StopSound,Created);return 0;}
void it_802756D0(Item_GObj* g){CHECK(g==&objects[Created]);record(DisableHit,Created);}
void it_80279BBC(Item* ip){CHECK(ip==&items[Created]);}
void it_80275158(Item_GObj* g,f32 life){(void)g;(void)life;CHECK(false);}
void it_80274F48(Item_GObj* g,s32 part,HSD_GObj* owner,Fighter_Part raw){CHECK(g==&objects[Created]&&part==7&&owner==&source&&raw==109);record(AttachJoint,raw);}
void it_80274C88(Item_GObj* g){CHECK(g==&objects[Created]);record(AttachmentUpdate,Created);}
void Item_80268E5C(HSD_GObj* g,enum_t state,Item_StateChangeFlags flags){CHECK(g==&objects[Created]&&flags==ITEM_ANIM_UPDATE);items[Created].msid=state;items[Created].on_accessory=NULL;record(AnimationChange,state);}
static void change(HSD_GObj* g,int state){Item_80268E5C(g,state,ITEM_ANIM_UPDATE);}
void HSD_JObjAnimAll(HSD_JObj* j){CHECK(j==&joints[Created]);record(AnimationAdvance,Created);}
float lbGetJObjCurrFrame(HSD_JObj* j){CHECK(j==&joints[Created]);return 5;}
void it_802799E4(Item_GObj* g){CHECK(g==&objects[Created]);CHECK(fighter.item_gobj==NULL);CHECK((scripted?get("parasol_handle"):-1)==-1);if(mutate_attachment)invalidate();}
static HSD_GObj* Item_8026862C(SpawnItem* spawn,NativeItemBehaviorSelection selection,FighterKind author);
static void Item_802674AC(SpawnItem* spawn);
static int32_t get(const char* name);

static void DestroyItemInline(HSD_GObj* g,Item* i){(void)g;(void)i;CHECK(false);}
static void func_8026A8EC_inline3(HSD_GObj* g){(void)g;}
void lb_8000FD18(void* desc){(void)desc;CHECK(false);}
void Item_8026B0B4(HSD_GObj* g){(void)g;}
void efAsync_QueueClear(void* queue){(void)queue;}
void nativeArticleModel_Destroy(HSD_GObj* g,unsigned index){(void)g;(void)index;}
void HSD_GObjFree(HSD_GObj* g){record(Free,role(g));unlink_item(g);g->user_data=(void*)(uintptr_t)1;}
void ftKb_SpecialNPe_8010C3C0(HSD_GObj* g){(void)g;CHECK(false);}
ItemKind itGetKind(Item_GObj* g){return ((Item*)g->user_data)->kind;}

#include "parameters.inc"
#include "generated.inc"
#include "actual.inc"

// Instrument true body entries rather than wrapping/rewriting stock callbacks.
__attribute__((no_instrument_function)) void __cyg_profile_func_enter(void* fn,void* caller)
{
    (void)caller;if(!observe)return;
    if(fn==(void*)ftPe_8011D518||fn==(void*)script_fn_restore_held)record(Restore,0);
    else if(fn==(void*)ftPe_SpecialN_DoDeath2||fn==(void*)script_fn_forget_toad)record(ForgetToad,0);
    else if(fn==(void*)ftPe_SpecialLw_UnsetVeg||fn==(void*)script_fn_forget_vegetable)record(ForgetVeg,0);
    else if(fn==(void*)it_8026B724)record(Pause,-1);
    else if(fn==(void*)it_8026B73C)record(Resume,-1);
    else if(fn==(void*)db_80225DD8)record(Debug,Created);
}
__attribute__((no_instrument_function)) void __cyg_profile_func_exit(void* fn,void* caller){(void)fn;(void)caller;}
static unsigned field(const char* name)
{for(unsigned i=0;i<definition->state_field_count;i++)if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;CHECK(false);return 0;}
static void set(const char* name,int32_t value){memcpy(fighter.native_module_state+field(name),&value,4);}
static int32_t get(const char* name){int32_t value;memcpy(&value,fighter.native_module_state+field(name),4);return value;}
static void sentinel(HSD_GObj* g){(void)g;}
static void reset(bool use_script,unsigned presence,int saved,int held,int owner,uint32_t id)
{
    observe=false;scripted=use_script;mutation=None;mutate_pickup=false;event_count=0;
    memset(&fighter,0,sizeof fighter);memset(&other,0,sizeof other);memset(items,0,sizeof items);memset(joints,0,sizeof joints);memset(heads,0,sizeof heads);
    source=(HSD_GObj){.user_data=&fighter,.classifier=HSD_GOBJ_CLASS_FIGHTER,.next=&foreign};foreign=(HSD_GObj){.user_data=&other,.classifier=HSD_GOBJ_CLASS_FIGHTER};
    heads[HSD_GOBJ_PLINK_FIGHTER]=&source;fighter.kind=Ft_Kind_CustomFirst;fighter.x8_spawnNum=UINT32_C(0x81234567);other.kind=Ft_Kind_CustomFirst+1;other.x8_spawnNum=UINT32_C(0xFEDCBA98);registered=definition;
    fighter.death2_cb=fighter.death3_cb=fighter.take_dmg_cb=sentinel;
    for(int i=Count-1;i>=0;i--){objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&joints[i],.next=heads[HSD_GOBJ_PLINK_ITEM]};heads[HSD_GOBJ_PLINK_ITEM]=&objects[i];items[i].entity=&objects[i];items[i].x1C=UINT32_C(0x10203040)+i;items[i].owner=&source;items[i].native_article_author_kind=use_script&&i<Old?fighter.kind:Ft_Kind_None;items[i].kind=i==Parasol?It_Kind_Peach_Parasol:i==Toad?It_Kind_Peach_Toad:i==Vegetable?It_Kind_Peach_Turnip:It_Kind_Bat;items[i].xB8_itemLogicTable=&stock_logics[i];
        if(use_script&&i<Old){for(unsigned k=0;k<definition->article_logic_count;k++)if(definition->article_logics[k].kind==items[i].kind)items[i].xB8_itemLogicTable=definition->article_logics[k].logic;items[i].xDD4_itemVar.native_article.attachment_owner=&source;items[i].xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum;}
        else if(i==Vegetable)items[i].xDD4_itemVar.peachturnip.xDE4_owner=&source;
    }
    if(saved>=0){items[saved].x1C=id;items[saved].owner=owner==0?NULL:owner==1?&source:&foreign;joints[saved].flags=JOBJ_HIDDEN;items[saved].xDC8_word.flags.x3=true;items[saved].xDC8_word.flags.x7=true;}
    nativeItem_CaptureCreator(&objects[Old],&foreign);
    if(use_script){int32_t none=-1;memcpy(other.native_module_state+field("parasol_handle"),&none,4);memcpy(other.native_module_state+field("saved_handle"),&none,4);}
    fighter.item_gobj=held<0?NULL:&objects[held];
    if(use_script){set("parasol_handle",presence&1?items[Parasol].x1C:-1);set("toad_handle",presence&2?items[Toad].x1C:-1);set("vegetable_handle",presence&4?items[Vegetable].x1C:-1);set("saved_handle",saved<0?-1:items[saved].x1C);}
    else{fighter.u.pe.parasol_gobj_0=presence&1?&objects[Parasol]:NULL;fighter.u.pe.toad_gobj=presence&2?&objects[Toad]:NULL;fighter.u.pe.veg_gobj=presence&4?&objects[Vegetable]:NULL;fighter.u.pe.parasol_gobj_1=saved<0?NULL:&objects[saved];}
    ftData_OnItemDropExt[fighter.kind]=drop_callback;ftData_OnItemPickupExt[other.kind]=pickup_callback;ftData_OnItemPickupExt[fighter.kind]=pickup_callback;observe=true;
}
typedef struct Projection {int held,parasol,toad,vegetable,saved;int recipient_held,recipient_parasol,recipient_saved;unsigned recipient_callbacks;unsigned callbacks;bool live[Count];u32 joint_flags[Count];u32 item_flags[Count];int owner[Count];HSD_GObj* creator[Count];uint32_t creator_generation[Count];FighterKind creator_kind[Count];const NativeFighterDefinition* creator_definition[Count];} Projection;
static int handle_role(int32_t value){if(value==-1)return -1;for(int i=0;i<Count;i++)if(items[i].x1C==(uint32_t)value)return i;return -2;}
static Projection project(void)
{
    Projection p={0};p.held=role(fighter.item_gobj);p.parasol=scripted?handle_role(get("parasol_handle")):role(fighter.u.pe.parasol_gobj_0);p.toad=scripted?handle_role(get("toad_handle")):role(fighter.u.pe.toad_gobj);p.vegetable=scripted?handle_role(get("vegetable_handle")):role(fighter.u.pe.veg_gobj);p.saved=scripted?handle_role(get("saved_handle")):role(fighter.u.pe.parasol_gobj_1);
    p.recipient_held=role(other.item_gobj);p.recipient_callbacks=(other.death2_cb!=NULL)|((other.death3_cb!=NULL)<<1)|((other.take_dmg_cb!=NULL)<<2);
    if(scripted){int32_t value;memcpy(&value,other.native_module_state+field("parasol_handle"),4);p.recipient_parasol=handle_role(value);memcpy(&value,other.native_module_state+field("saved_handle"),4);p.recipient_saved=handle_role(value);}else{p.recipient_parasol=role(other.u.pe.parasol_gobj_0);p.recipient_saved=role(other.u.pe.parasol_gobj_1);}
    p.callbacks=(fighter.death2_cb!=NULL)|((fighter.death3_cb!=NULL)<<1)|((fighter.take_dmg_cb!=NULL)<<2);
    for(int i=0;i<Count;i++){uint32_t generation;p.live[i]=nativeCallbackSource_Snapshot(&objects[i],NativeCallbackSource_Item,&generation);p.joint_flags[i]=joints[i].flags;CHECK(sizeof items[i].xDC8_word==sizeof p.item_flags[i]);memcpy(&p.item_flags[i],&items[i].xDC8_word,sizeof p.item_flags[i]);p.owner[i]=items[i].owner==&source?1:items[i].owner==&foreign?2:items[i].owner==NULL?0:3;p.creator[i]=items[i].native_creator;p.creator_generation[i]=items[i].native_creator_generation;p.creator_kind[i]=items[i].native_creator_kind;p.creator_definition[i]=items[i].native_creator_definition;}
    return p;
}

static HSD_GObj* Item_8026862C(SpawnItem* spawn,NativeItemBehaviorSelection selection,FighterKind author)
{
    CHECK(selection==(scripted?NativeItemBehavior_Author:NativeItemBehavior_Parent));CHECK(author==(scripted?fighter.kind:Ft_Kind_None));
    CHECK(spawn->kind==It_Kind_Peach_Parasol);
    CHECK(spawn->x0_parent_gobj==&source&&spawn->x4_parent_gobj2==&source);
    CHECK(spawn->x3C_damage==0&&spawn->x40==0&&spawn->x44_flag.b0);
    CHECK(spawn->vel.x==0&&spawn->vel.y==0&&spawn->vel.z==0);
    CHECK(spawn->x48_ground_or_air==GA_Air&&spawn->x10==0);
    observed=*spawn;record(Allocate,spawn->kind);if(fail_allocation)return NULL;
    Item* ip=&items[Created];memset(ip,0,sizeof *ip);ip->entity=&objects[Created];ip->x1C=creation_id;ip->kind=spawn->kind;ip->owner=spawn->x0_parent_gobj;ip->pos=spawn->pos;ip->facing_dir=spawn->facing_dir;ip->scl=1.25f;ip->throw_scale=1;ip->xC4_article_data=&asset;ip->xC8_joint=&descriptor;ip->xD70=45;ip->sfx_unk2=63;ip->xDCD_flag.b2=false;ip->xDC8_word.flags.x15=true;
    ip->xB8_itemLogicTable=&stock_logics[Created];if(scripted){ip->native_article_author_kind=fighter.kind;for(unsigned a=0;a<definition->article_logic_count;a++)if(definition->article_logics[a].kind==ip->kind)ip->xB8_itemLogicTable=definition->article_logics[a].logic;}else ip->native_article_author_kind=Ft_Kind_None;
    objects[Created].user_data=ip;objects[Created].next=heads[HSD_GOBJ_PLINK_ITEM];heads[HSD_GOBJ_PLINK_ITEM]=&objects[Created];return &objects[Created];
}

static void compare(Projection* expected_projection)
{Projection actual=project();CHECK(event_count==expected_count&&!memcmp(events,expected,event_count*sizeof(*events)));CHECK(!memcmp(expected_projection,&actual,sizeof actual));++pairs;}
static unsigned calls(int step){unsigned n=0;for(unsigned i=0;i<event_count;i++)n+=events[i].step==step;return n;}
static bool live(int index){uint32_t generation;return nativeCallbackSource_Snapshot(&objects[index],NativeCallbackSource_Item,&generation);}
typedef struct ProducerProjection {Projection cleanup;int request;unsigned callbacks;int auxiliary;Vec3 position,previous,velocity;float facing,rate,frame,scale;int state,attachment,hold_kind;u8 debug;bool accessory;Vec3 root_translation,root_scale,child_translation;u32 child_flags;} ProducerProjection;
static ProducerProjection producer_projection(void){ProducerProjection p={0};p.cleanup=project();p.request=scripted?get("parasol_request_kind"):fighter.mv.pe.specialhi.kind;p.callbacks=(fighter.accessory4_cb!=NULL)|((fighter.pre_hitlag_cb!=NULL)<<1)|((fighter.post_hitlag_cb!=NULL)<<2);p.auxiliary=role(fighter.x1984_heldItemSpec);p.position=observed.pos;p.previous=observed.prev_pos;p.velocity=observed.vel;p.facing=observed.facing_dir;p.hold_kind=observed.hold_kind;p.rate=items[Created].x5D0_animFrameSpeed;p.frame=items[Created].x5CC_currentAnimFrame;p.scale=items[Created].scl;p.state=items[Created].msid;p.attachment=items[Created].xDC4;p.debug=items[Created].xDAA_byte;p.accessory=items[Created].on_accessory!=NULL;p.root_translation=joints[Created].translate;p.root_scale=joints[Created].scale;p.child_translation=child_joint.translate;p.child_flags=child_joint.flags;return p;}
static void setup_producer(bool mode,int request,bool present,int held,bool fail,uint32_t id,float facing,unsigned debug,bool mutate_pose){
    reset(mode,present?1:0,-1,held,1,0);memset(&observed,0,sizeof observed);fail_allocation=fail;missing_assets=false;mutate_attachment=false;drop_mutates_pose=mutate_pose;creation_id=id;facing_value=facing;debug_display=debug;
    memset(bones,0,sizeof bones);memset(&bone_joint,0,sizeof bone_joint);bone_joint.translate=(Vec3){11,22,33};bones[109].joint=&bone_joint;fighter.parts=bones;fighter.facing_dir=facing;rig.parts_num=114;rigs[fighter.kind]=&rig;fighter.x1984_heldItemSpec=&objects[Toad];fighter.accessory4_cb=sentinel;
    memset(&descriptor,0,sizeof descriptor);memset(&child_descriptor,0,sizeof child_descriptor);descriptor.rotation=(Vec3){2,3,4};descriptor.position=(Vec3){5,6,7};descriptor.scale=(Vec3){8,9,10};descriptor.child=&child_descriptor;child_descriptor.position=(Vec3){11,12,13};memset(&child_joint,0,sizeof child_joint);child_joint.flags=JOBJ_HIDDEN;joints[Created].child=&child_joint;asset.x10_modelDesc=&model;model.x8_bone_attach_id=7;
    if(mode)set("parasol_request_kind",request);else fighter.mv.pe.specialhi.kind=request;
    unlink_item(&objects[Created]);
}
int main(void){
    stock_logics[Parasol].destroyed=itPeachParasol_Logic60_Destroyed;stock_logics[Created].destroyed=itPeachParasol_Logic60_Destroyed;stock_logics[Created].picked_up=itPeachParasol_Logic60_PickedUp;stock_logics[Toad].destroyed=itPeachToad_Logic91_Destroyed;stock_logics[Vegetable].destroyed=itPeachTurnip_Logic56_Destroyed;
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.held_item_id=nativeItem_HeldId,.held_item_kind=itGetKind,.bind_existing_item=nativeItem_BindExisting,.set_item_visible=nativeItem_SetVisible,.set_item_paused=nativeItem_SetPaused,.notify_item_drop=nativeItem_NotifyDrop,.notify_item_pickup=ftpickupitem_80094818,.signal_article=signal_article,.target_call=nativeFighterEvent_Call,.target_call_bool=nativeFighterEvent_CallBool,.item_remove=Item_8026A8EC,.item_creator=nativeArticle_Creator,.item_current_owner=current_owner,.bone_count=bone_count,.joint_position=lb_8000B1CC,.create_article_at=nativeArticle_Create,.attach_created_item=nativeItem_Attach,.bind_fighter_item=nativeItem_BindFighter,.clear_fighter_item=nativeItem_ClearBinding,.item_change_state=change,.item_advance_animation=Item_802694CC};
    MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataPeach",.motion_states=&motion,.motion_state_first=341,.motion_state_count=1};definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&nativeFighterEvents_Validate(definition));uint32_t parameter=109;CHECK(FreakMelee_SetFighterParameters(&parameter,1,fixture_parameter_schema));
    const int requests[]={It_Kind_Capsule,It_Kind_Parasol,It_Kind_Peach_Parasol};const uint32_t ids[]={0,UINT32_C(0x80000001),UINT32_C(0xFEDCBA98),UINT32_C(0xFFFFFFFE)};
    for(unsigned r=0;r<3;r++)for(unsigned present=0;present<2;present++)for(int held=-1;held<Created;held++)for(unsigned fail=0;fail<2;fail++)for(unsigned id=0;id<4;id++)for(unsigned face=0;face<2;face++)for(unsigned debug=0;debug<4;debug++)for(unsigned mutate_pose=0;mutate_pose<2;mutate_pose++){
        setup_producer(false,requests[r],present,held,fail,ids[id],face?1:-1,debug,mutate_pose);ftPe_SpecialHi_8011D424(&source);fighter.pre_hitlag_cb(&source);fighter.post_hitlag_cb(&source);ProducerProjection want=producer_projection();expected_count=event_count;memcpy(expected,events,sizeof events);
        setup_producer(true,requests[r],present,held,fail,ids[id],face?1:-1,debug,mutate_pose);script_fn_create_parasol(&source);fighter.pre_hitlag_cb(&source);fighter.post_hitlag_cb(&source);ProducerProjection actual=producer_projection();CHECK(event_count==expected_count&&!memcmp(events,expected,event_count*sizeof *events));CHECK(!memcmp(&want,&actual,sizeof want));++pairs;
    }
    /* Resolution and callback invalidation are safety probes, not comparisons
     * with undefined stock continuations through a missing/recycled pointer. */
    setup_producer(true,It_Kind_Capsule,false,Old,false,UINT32_C(0xFEDCBA98),1,3,false);missing_assets=true;script_fn_create_parasol(&source);CHECK(get("parasol_handle")==-1&&fighter.item_gobj==NULL&&get("saved_handle")== (int32_t)items[Old].x1C&&calls(Allocate)==0&&!live(Created));++safety;
    setup_producer(true,It_Kind_Capsule,false,Old,false,UINT32_C(0xFEDCBA98),1,3,false);bones[109].joint=NULL;script_fn_create_parasol(&source);CHECK(get("parasol_handle")==-1&&fighter.item_gobj==&objects[Old]&&get("saved_handle")==-1&&!live(Created)&&calls(Allocate)==1&&calls(Debug)==0&&calls(PickupCallback)==1);++safety;
    const enum Mutation attached_mutations[]={RemoveCreated,RecycleCreated,RemoveSource,RecycleSource};
    for(unsigned m=0;m<4;m++){
        setup_producer(true,It_Kind_Capsule,false,Old,false,UINT32_C(0xFEDCBA98),1,3,false);mutate_attachment=true;mutation=attached_mutations[m];script_fn_create_parasol(&source);
        CHECK(calls(Debug)==1&&calls(StopSound)==(mutation==RemoveSource||mutation==RecycleSource?1:0)&&get("parasol_handle")==-1);
        if(mutation==RemoveCreated||mutation==RecycleCreated){CHECK(fighter.item_gobj==&objects[Old]&&get("saved_handle")==-1&&calls(PickupCallback)==1);if(mutation==RemoveCreated)CHECK(!live(Created));else CHECK(live(Created)&&items[Created].x1C==UINT32_C(0x61234567)&&items[Created].owner==&foreign);}
        else CHECK(fighter.accessory4_cb==sentinel&&fighter.pre_hitlag_cb==NULL&&fighter.post_hitlag_cb==NULL&&fighter.item_gobj==NULL);
        ++safety;
    }
    printf("Parasol producer: %u actual/generated pairs, %u safety cases, %u checks. Controlled allocation/model-state internals.\n",pairs,safety,checks);return 0;
}
