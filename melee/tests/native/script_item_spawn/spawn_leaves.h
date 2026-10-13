#include <melee/ft/fighter.h>
#include <melee/ft/native_fighter_target.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/it/native_script_item_spawn.h>
extern unsigned spawn_stage,spawn_retire_stage,spawn_fail_stage;
extern unsigned spawn_order[128],spawn_order_count;
extern HSD_GObj* spawn_caller;
extern const NativeScriptItemProgram* spawn_food_asset;
static Item spawn_items[32];static bool spawn_live[32];static HSD_JObj spawn_joints[32];
static ItemLogicTable spawn_food_logic;
static void stage(unsigned value)
{
    spawn_stage=value;if(spawn_order_count==128)abort();spawn_order[spawn_order_count++]=value;
    if(spawn_retire_stage==value){spawn_retire_stage=0;nativeScriptFighter_Detach(spawn_caller);}
    if(spawn_fail_stage==value){spawn_fail_stage=0;HSD_GObj* object=HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];for(unsigned i=0;i<NATIVE_SCRIPT_ITEM_ACTORS;++i)if(nativeScriptItemActors[i].active&&nativeScriptItemActors[i].object==object)nativeScriptItemActors[i].faulted=true;}
}
void* HSD_ObjAlloc(HSD_ObjAllocData* data)
{(void)data;for(unsigned i=0;i<32;++i)if(!spawn_live[i]){spawn_live[i]=true;memset(&spawn_items[i],0xCC,sizeof(Item));stage(StageConstruct);return &spawn_items[i];}return NULL;}
void HSD_ObjFree(HSD_ObjAllocData* data,void* object)
{(void)data;for(unsigned i=0;i<32;++i)if(object==&spawn_items[i]){spawn_live[i]=false;++scheduler_frees;return;}abort();}
void Item_80267978(HSD_GObj* object)
{
    Item* item=object->user_data;item->xC4_article_data=(Article*)(item->kind==It_Kind_Foods?spawn_food_asset->article:scheduler_spec->article);
    item->xB8_itemLogicTable=&spawn_food_logic;
}
bool nativeItemBehavior_Select(Item_GObj* object,const SpawnItem* spawn,NativeItemBehaviorSelection selection,FighterKind author,uint32_t owner)
{
    (void)spawn;if(selection==NativeItemBehavior_Engine)return true;
    const NativeScriptItemProgram* p=nativeScriptItem_Program(author,owner);if(!p)return false;
    scheduler_spec=p;Item* item=object->user_data;item->xC4_article_data=(Article*)p->article;
    item->native_article_author_kind=author;item->native_script_owner=owner;item->xB8_itemLogicTable=NULL;item->xBC_itemStateContainer=NULL;
    if(p->recipe){item->xDD4_itemVar.native_holdable.owner_kind=(u16)author;item->xDD4_itemVar.native_holdable.definition_index=(u16)p->recipe_index;}
    return nativeScriptItem_Attach(object,p)&&!scheduler_fail_selector;
}
void nativeItem_CaptureCreator(Item_GObj* object,Fighter_GObj* creator)
{
    Item* item=object->user_data;item->native_creator=creator;
    if(creator){Fighter* fp=creator->user_data;item->native_creator_generation=(uint32_t)fp->x8_spawnNum;item->native_creator_kind=fp->kind;item->native_creator_definition=NULL;}
}
Fighter_GObj* nativeItem_Creator(Item_GObj* object)
{
    Item* item=object->user_data;return item->native_creator&&nativeCallbackSource_Alive(item->native_creator,item->native_creator_generation,NativeCallbackSource_Fighter)?item->native_creator:NULL;
}
void it_80275158(Item_GObj* object,float value){((Item*)object->user_data)->xD44_lifeTimer=value;}
void db_80225DD8(HSD_GObj* object,HSD_GObj* owner){(void)object;(void)owner;stage(StageDebug);}
bool nativeItemBehavior_EngineAssetsAvailable(ItemKind kind){return kind==It_Kind_Foods;}
bool nativeFighterRuntime_ResolveArticleForItem(const Fighter* fp,ItemKind kind,Article** result){(void)fp;(void)kind;(void)result;return false;}
const NativeFighterArticleLogic* nativeFighterRuntime_ArticleLogic(const Fighter* fp,ItemKind kind){(void)fp;(void)kind;return NULL;}
void it_802750F8(HSD_GObj* object){(void)object;stage(StageUpdate);}
void it_8026B3A8(HSD_GObj* object){((Item*)object->user_data)->xDC8_word.flags.x15=0;}
void it_80273670(HSD_GObj* object,int anim,float frame){(void)object;(void)anim;(void)frame;stage(StageAppearance);}
void it_8026BB68(HSD_GObj* object,Vec3* output){Fighter* fp=object->user_data;*output=fp->cur_pos;}
void ftLib_80086990(HSD_GObj* object,Vec3* output){Fighter* fp=object->user_data;*output=fp->cur_pos;output->y+=3;}
void lb_8000B1CC(HSD_JObj* joint,Vec3* input,Vec3* output){(void)input;*output=joint->translate;}
uint32_t nativeFighterTarget_Id(HSD_GObj* object){return object?(uint32_t)((Fighter*)object->user_data)->x8_spawnNum:0;}
void ftpickupitem_80094818(Fighter_GObj* object,bool flag){(void)object;(void)flag;stage(StageInventory);}
void it_802756D0(HSD_GObj* object){(void)object;}
void it_8026B724(HSD_GObj* object){(void)object;}
void it_8026B73C(HSD_GObj* object){(void)object;}
Fighter_Part ftParts_GetBoneIndex(Fighter* fp,Fighter_Part part){(void)fp;return part;}
void it_80274574(HSD_GObj* object){(void)object;}
void it_80279B64(Item* item){item->x548_colorOverlay.x8_ptr1=NULL;item->x548_colorOverlay.x4_pri=0;item->x548_colorOverlay.x28_colanim.ptr=NULL;item->x548_colorOverlay.x7C_color_enable=item->x548_colorOverlay.x7C_flag2=false;}
static void Item_80267130(HSD_GObj* object,SpawnItem* spawn){Item* item=object->user_data;item->x40_vel=spawn->vel;item->pos=spawn->prev_pos;item->init_facing_dir=item->facing_dir=spawn->facing_dir;item->owner=spawn->x0_parent_gobj;}
void Item_802680CC(HSD_GObj* object){for(unsigned i=0;i<32;++i)if(object->user_data==&spawn_items[i]){memset(&spawn_joints[i],0,sizeof(HSD_JObj));object->hsd_obj=&spawn_joints[i];++scheduler_models;return;}abort();}
bool it_8026B894(HSD_GObj* object,HSD_GObj* referenced){(void)object;(void)referenced;return false;}
// Shared recipe protocol is exercised separately; pickup still runs the actual
// native Item wrapper. The source Article hook stages are production code.
