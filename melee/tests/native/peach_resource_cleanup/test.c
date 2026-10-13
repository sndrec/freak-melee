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

enum Role { Parasol,Toad,Vegetable,Old,Count };
enum Step { Restore,ForgetToad,ForgetVeg,Hide,Show,Pause,Resume,DropCallback,DropStats,PickupCallback,PickupStats,Free };
typedef struct CleanupEvent { int step,role; } CleanupEvent;
enum Mutation { None,RemoveSource,RecycleSource,ChangeKind,ChangeDefinition,RemoveOld,RecycleOld,RemoveParasol,RecycleParasol };
static CleanupEvent events[96],expected[96];static unsigned event_count,expected_count,checks,pairs,safety;
static HSD_GObj source,foreign,objects[Count],*heads[32];HSD_GObj** HSD_GObjPLinkHead=heads;
static Fighter fighter,other;static Item items[Count];static HSD_JObj joints[Count];
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
}
static void drop_callback(HSD_GObj* g,bool normal){CHECK(g==&source&&normal);record(DropCallback,role(fighter.item_gobj));if(!mutate_pickup)invalidate();}
static void pickup_callback(HSD_GObj* g,bool normal){CHECK((g==&source||g==&foreign)&&normal);record(PickupCallback,role(((Fighter*)g->user_data)->item_gobj));if(mutate_pickup)invalidate();}
bool ftCo_800C5240(HSD_GObj* g){(void)g;return false;}
void ftCo_800C5500(HSD_GObj* g){(void)g;CHECK(false);}
void pl_8003EA08(int player,int flags){CHECK(player==0&&flags==0);record(DropStats,role(fighter.item_gobj));}
void pl_8003E17C(int player,int flags,Item_GObj* g){CHECK(player==0&&flags==0);record(PickupStats,role(g));}
void HSD_JObjClearFlagsAll(HSD_JObj* joint,u32 flags){CHECK(flags==JOBJ_HIDDEN);for(int i=0;i<Count;i++)if(joint==&joints[i]){joint->flags&=~flags;record(Show,i);return;}CHECK(false);}
void HSD_JObjSetFlagsAll(HSD_JObj* joint,u32 flags){CHECK(flags==JOBJ_HIDDEN);for(int i=0;i<Count;i++)if(joint==&joints[i]){joint->flags|=flags;record(Hide,i);return;}CHECK(false);}
// Unrelated allocation, attachment, scale and removal housekeeping boundaries.
Item_GObj* Item_CreateEngine(SpawnItem* spawn){(void)spawn;CHECK(false);return NULL;}
void Item_8026AB54(Item_GObj* g,HSD_GObj* parent,Fighter_Part part){(void)g;(void)parent;(void)part;CHECK(false);}
void it_80274594(Item_GObj* g){(void)g;CHECK(false);}
bool it_80272D1C(HSD_GObj* g){uint32_t id;return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Item,&id);}
void it_802725D4(HSD_GObj* g){(void)g;}
static void Item_802675A8(HSD_GObj* g){(void)g;}
static void func_8026A8EC_inline1(HSD_GObj* g){(void)g;}
void efLib_DestroyAll(HSD_GObj* g){(void)g;}
static void ItemSwitch(HSD_GObj* g){(void)g;}
bool ftLib_80086960(HSD_GObj* g){uint32_t id;return nativeCallbackSource_Snapshot(g,NativeCallbackSource_Fighter,&id);}
static void DestroyItemInline(HSD_GObj* g,Item* i){(void)g;(void)i;CHECK(false);}
static void func_8026A8EC_inline3(HSD_GObj* g){(void)g;}
void lb_8000FD18(void* desc){(void)desc;CHECK(false);}
void Item_8026B0B4(HSD_GObj* g){(void)g;}
void efAsync_QueueClear(void* queue){(void)queue;}
void nativeArticleModel_Destroy(HSD_GObj* g,unsigned index){(void)g;(void)index;}
void HSD_GObjFree(HSD_GObj* g){record(Free,role(g));unlink_item(g);g->user_data=(void*)(uintptr_t)1;}
void ftKb_SpecialNPe_8010C3C0(HSD_GObj* g){(void)g;CHECK(false);}
ItemKind itGetKind(Item_GObj* g){return ((Item*)g->user_data)->kind;}

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
static void compare(Projection* expected_projection)
{Projection actual=project();CHECK(event_count==expected_count&&!memcmp(events,expected,event_count*sizeof(*events)));CHECK(!memcmp(expected_projection,&actual,sizeof actual));++pairs;}
static unsigned calls(int step){unsigned n=0;for(unsigned i=0;i<event_count;i++)n+=events[i].step==step;return n;}
static bool live(int index){uint32_t generation;return nativeCallbackSource_Snapshot(&objects[index],NativeCallbackSource_Item,&generation);}
int main(void)
{
    stock_logics[Parasol].destroyed=itPeachParasol_Logic60_Destroyed;stock_logics[Toad].destroyed=itPeachToad_Logic91_Destroyed;stock_logics[Vegetable].destroyed=itPeachTurnip_Logic56_Destroyed;
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,.held_item_id=nativeItem_HeldId,.held_item_kind=itGetKind,.bind_existing_item=nativeItem_BindExisting,.set_item_visible=nativeItem_SetVisible,.set_item_paused=nativeItem_SetPaused,.notify_item_drop=nativeItem_NotifyDrop,.notify_item_pickup=ftpickupitem_80094818,.signal_article=signal_article,.target_call=nativeFighterEvent_Call,.target_call_bool=nativeFighterEvent_CallBool,.item_remove=Item_8026A8EC,.item_creator=nativeArticle_Creator,.item_current_owner=current_owner};
    NativeFighterDefinition base={0};definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition&&definition->article_logic_count==3);CHECK(nativeFighterEvents_Validate(definition));
    const uint32_t ids[]={0,UINT32_C(0x80000001),UINT32_C(0xFEDCBA98),UINT32_C(0xFFFFFFFE)};
    for(unsigned presence=0;presence<8;presence++)for(int saved=-1;saved<=Old;saved++){
        if(saved==Parasol||saved==Toad)continue;
        for(int held=-1;held<Count;held++)for(int owner=0;owner<3;owner++)for(unsigned id=0;id<4;id++){
            reset(false,presence,saved,held,owner,ids[id]);ftPe_Init_OnDeath2(&source);Projection want=project();expected_count=event_count;memcpy(expected,events,sizeof events);
            reset(true,presence,saved,held,owner,ids[id]);script_fn_cleanup_resources(&source);compare(&want);
        }
    }
    for(int held=-1;held<Count;held++)for(int owner=0;owner<3;owner++)for(unsigned id=0;id<4;id++){
        reset(false,0,held,held,owner,ids[id]);actual_save_held_slice(&source);Projection want=project();expected_count=event_count;memcpy(expected,events,sizeof events);
        reset(true,0,held,held,owner,ids[id]);script_fn_save_held_for_parasol(&source);compare(&want);
    }
    /* Current recipient owns its restoration variables; creator state differs. */
    for(unsigned presence=0;presence<8;presence++)for(int creator_saved=0;creator_saved<2;creator_saved++)for(int recipient_saved=0;recipient_saved<2;recipient_saved++)for(int held=-1;held<Count;held++)for(int owner=0;owner<3;owner++)for(unsigned id=0;id<4;id++){
        Projection want;
        for(int mode=0;mode<2;mode++){
            reset(mode,presence,creator_saved?Vegetable:-1,held,1,UINT32_C(0x23456789));
            objects[Old].hsd_obj=&joints[Old];items[Old].x1C=ids[id];items[Old].owner=owner==0?NULL:owner==1?&source:&foreign;joints[Old].flags=JOBJ_HIDDEN;items[Old].xDC8_word.flags.x3=true;items[Old].xDC8_word.flags.x7=true;
            items[Parasol].owner=&foreign;other.item_gobj=&objects[Parasol];other.death2_cb=other.death3_cb=other.take_dmg_cb=sentinel;
            if(mode){int32_t h=items[Parasol].x1C, saved=recipient_saved?(int32_t)items[Old].x1C:-1;memcpy(other.native_module_state+field("parasol_handle"),&h,4);memcpy(other.native_module_state+field("saved_handle"),&saved,4);script_fn_cleanup_resources(&source);compare(&want);}
            else{other.u.pe.parasol_gobj_0=&objects[Parasol];other.u.pe.parasol_gobj_1=recipient_saved?&objects[Old]:NULL;ftPe_Init_OnDeath2(&source);want=project();expected_count=event_count;memcpy(expected,events,sizeof events);}
        }
    }
    /* Safety probes have no stock parity claim for invalid original pointers. */
    for(unsigned change=RemoveSource;change<=RecycleOld;change++){
        reset(true,0,Old,Old,2,UINT32_C(0xFEDCBA98));mutation=change;bool ok=nativeItem_NotifyDrop(&source,items[Old].x1C,true);CHECK(!ok&&calls(DropCallback)==1&&calls(DropStats)==(change==RemoveSource||change==RecycleSource?0:1));++safety;
    }
    for(unsigned change=RemoveSource;change<=RecycleOld;change++){
        reset(true,0,-1,Old,2,UINT32_C(0xFEDCBA98));items[Old].x1C=UINT32_C(0xFEDCBA98);mutation=change;script_fn_save_held_for_parasol(&source);CHECK(get("saved_handle")== (int32_t)UINT32_C(0xFEDCBA98)&&calls(Hide)==1&&calls(Pause)==1&&calls(DropCallback)==1&&calls(DropStats)==(change==RemoveSource||change==RecycleSource?0:1));++safety;
    }
    const enum Mutation pickup_mutations[]={RemoveSource,RecycleSource,RemoveOld,RecycleOld,RemoveParasol,RecycleParasol};
    for(unsigned m=0;m<sizeof pickup_mutations/sizeof pickup_mutations[0];m++){
        enum Mutation change=pickup_mutations[m];reset(true,7,Old,Parasol,2,UINT32_C(0xFEDCBA98));mutation=change;mutate_pickup=true;script_fn_cleanup_resources(&source);
        CHECK(calls(PickupCallback)==1&&calls(PickupStats)==(change==RemoveSource||change==RecycleSource?0:1)&&get("saved_handle")==-1);
        if(change==RemoveSource||change==RecycleSource){CHECK(live(Toad)&&live(Vegetable)&&calls(ForgetToad)==0);}
        else{CHECK(!live(Toad)&&live(Vegetable));if(change==RemoveParasol)CHECK(!live(Parasol));if(change==RecycleParasol)CHECK(live(Parasol)&&items[Parasol].x1C==UINT32_C(0x61234567));}
        ++safety;
    }
    reset(true,1,Old,Parasol,2,UINT32_C(0xFEDCBA98));unlink_item(&objects[Old]);objects[Old].user_data=(void*)(uintptr_t)1;script_fn_cleanup_resources(&source);CHECK(get("saved_handle")==-1&&get("parasol_handle")==-1&&calls(PickupCallback)==0&&calls(Show)==0&&!live(Parasol));++safety;
    reset(true,1,Old,Parasol,2,UINT32_C(0xFEDCBA98));++items[Old].x1C;script_fn_cleanup_resources(&source);CHECK(get("saved_handle")==-1&&get("parasol_handle")==-1&&calls(PickupCallback)==0&&calls(Show)==0&&!live(Parasol));++safety;
    reset(true,1,Old,Parasol,2,UINT32_C(0xFEDCBA98));objects[Old].hsd_obj=NULL;script_fn_cleanup_resources(&source);CHECK(get("saved_handle")==-1&&fighter.item_gobj==&objects[Old]&&calls(PickupCallback)==0&&calls(Show)==0&&!live(Parasol));++safety;
    for(int owner=0;owner<3;owner++)for(unsigned id=0;id<4;id++){
        reset(true,0,Old,-1,owner,ids[id]);HSD_GObj* prior_owner=items[Old].owner;HSD_GObj* prior_creator=items[Old].native_creator;
        CHECK(nativeItem_BindExisting(&source,ids[id],false)&&fighter.item_gobj==&objects[Old]);CHECK(nativeItem_BindExisting(&source,ids[id],true)&&fighter.x1984_heldItemSpec==&objects[Old]);CHECK(items[Old].owner==prior_owner&&items[Old].native_creator==prior_creator);++safety;
    }
    printf("Peach cleanup: %u actual/generated pairs, %u safety cases, %u checks. Current-holder Parasol; producer constructor excluded.\n",pairs,safety,checks);return 0;
}
