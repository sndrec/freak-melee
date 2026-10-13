// Production held dispatcher and live generation registry; immutable catalogs
// are controlled so both same-package and foreign-held ownership can be tested.
#include <melee/it/native_held_article.h>
#include <melee/it/native_script_item_actor.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, calls, hazard, depth_calls;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static HSD_GObj payload[3];
#define holder payload[0]
#define article payload[1]
#define replacement payload[2]
static NativeMatchPoolSlot pool_slots[3];static uint32_t pool_free[3];static uint64_t epoch;
static Fighter fighter;
static Item item;
static NativeFighterDefinition holder_definition, author_definition, other_definition;
static NativeFighterArticleLogic policy;
static ItemLogicTable logic, other_logic;
static bool holder_changed, author_changed, accepted;
// This C-policy oracle has actual pooled GObj identities. Interpreted policy
// callbacks remain explicitly unavailable, covered by connected actor fixtures.
NativeMatchPool nativeGObjPool;
const NativeScriptItemProgram* nativeScriptItem_Spec(Item_GObj* object){(void)object;return NULL;}
NativeScriptVMStatus nativeScriptItem_Status(Item_GObj* object){(void)object;return NativeScriptVM_SourceRetired;}
bool nativeScriptItem_HookBound(Item_GObj* object,uint32_t hook){(void)object;(void)hook;return false;}
NativeScriptVMStatus nativeScriptItem_RunHook(Item_GObj* object,uint32_t hook,const NativeScriptValue* arguments,uint32_t count,NativeScriptValue* result)
{(void)object;(void)hook;(void)arguments;(void)count;(void)result;return NativeScriptVM_SourceRetired;}
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"failed %s:%d: %s\n",__FILE__,__LINE__,#x); exit(1); } } while(0)
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (kind == Ft_Kind_CustomFirst) return holder_changed ? &other_definition : &holder_definition;
    if (kind == Ft_Kind_CustomFirst+1) return author_changed ? &other_definition : &author_definition;
    return NULL;
}
static void mutate(void)
{
    switch (hazard) {
    case 1: heads[HSD_GOBJ_PLINK_FIGHTER]=NULL; holder.user_data=(void*)1; break;
    case 2: heads[HSD_GOBJ_PLINK_ITEM]=NULL; article.user_data=(void*)1; break;
    case 3: ++fighter.x8_spawnNum; break;
    case 4: ++item.x1C; break;
    case 5: fighter.kind=Ft_Kind_Mario; break;
    case 6: item.native_article_author_kind=Ft_Kind_Mario; break;
    case 7: holder_changed=true; break;
    case 8: author_changed=true; break;
    case 9: item.xB8_itemLogicTable=&other_logic; break;
    case 10: fighter.item_gobj=&replacement; break;
    case 11: item.kind=It_Kind_Capsule; break;
    case 12: item.native_removal_in_progress=true; break;
    case 13: HSD_GObjPLinkHead=NULL; break;
    }
}
static int32_t status(HSD_GObj* object)
{
    CHECK(object==&article); ++calls;
    if (hazard==14) { ++depth_calls; int32_t nested=99;
        CHECK(nativeHeldArticle_Status(&holder,&nested)); return nested; }
    mutate(); return -123456789;
}
static bool request(HSD_GObj* object,int32_t action,float rate,float duration)
{
    CHECK(object==&article && action==-314159 && rate==1.25f && duration==15.5f);
    ++calls; mutate(); return accepted;
}
static void neutral(HSD_GObj* object)
{
    CHECK(object == &article); ++calls;
    if (hazard == 14) {
        ++depth_calls;
        nativeHeldArticle_NotifyNeutral(&holder,NULL);
    } else mutate();
}
static void reset(void)
{
    memset(heads,0,sizeof(heads)); HSD_GObjPLinkHead=heads;
    memset(&fighter,0,sizeof(fighter)); memset(&item,0,sizeof(item));
    memset(&holder_definition,0,sizeof(holder_definition));
    memset(&author_definition,0,sizeof(author_definition)); memset(&policy,0,sizeof(policy));
    CHECK(nativeMatchPool_Bind(&nativeGObjPool,payload,sizeof(payload),pool_slots,pool_free,
        3,sizeof(HSD_GObj),_Alignof(HSD_GObj),++epoch));
    for(unsigned i=0;i<3;++i){NativeMatchPoolReference reference;CHECK(nativeMatchPool_Allocate(&nativeGObjPool,&reference)==&payload[i]);}
    holder=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_FIGHTER,.user_data=&fighter};
    article=(HSD_GObj){.classifier=HSD_GOBJ_CLASS_ITEM,.user_data=&item};
    fighter.kind=Ft_Kind_CustomFirst; fighter.x8_spawnNum=0x1234567; fighter.item_gobj=&article;
    item.x1C=0x87654321; item.kind=It_Kind_Peach_Parasol;
    // Current holder is from another package; immutable article author selects policy.
    item.native_article_author_kind=Ft_Kind_CustomFirst+1; item.xB8_itemLogicTable=&logic;
    item.owner=&holder; policy.kind=item.kind; policy.logic=&logic;
    policy.on_held_status=status; policy.on_held_request=request;
    author_definition.article_logic_count=1; author_definition.article_logics=&policy;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&holder; heads[HSD_GOBJ_PLINK_ITEM]=&article;
    calls=hazard=depth_calls=0; holder_changed=author_changed=false; accepted=true;
}
int main(void)
{
    int32_t value;
    bool claimed;
    reset(); CHECK(nativeHeldArticle_Status(&holder,&value)); CHECK(value==-123456789 && calls==1);
    for (unsigned result=0; result<2; ++result) {
        reset(); accepted=result;
        CHECK(nativeHeldArticle_Request(&holder,-314159,1.25f,15.5f)); CHECK(calls==1);
    }
    for (unsigned why=1; why<=13; ++why) {
        reset(); hazard=why;
        CHECK(nativeHeldArticle_Status(&holder,&value)); CHECK(value==-1 && calls==1);
        reset(); hazard=why;
        CHECK(nativeHeldArticle_Request(&holder,-314159,1.25f,15.5f)); CHECK(calls==1);
    }
    reset(); hazard=14; CHECK(nativeHeldArticle_Status(&holder,&value));
    CHECK(value==-1 && depth_calls==64); hazard=0;
    CHECK(nativeHeldArticle_Status(&holder,&value)); CHECK(value==-123456789);
    reset(); policy.on_held_status=NULL; policy.on_held_request=NULL;
    CHECK(!nativeHeldArticle_Status(&holder,&value)); CHECK(!calls);
    CHECK(!nativeHeldArticle_Request(&holder,-314159,1.25f,15.5f));
    reset(); item.native_article_author_kind=Ft_Kind_Mario;
    CHECK(!nativeHeldArticle_Status(&holder,&value)); CHECK(!calls);
    reset(); fighter.item_gobj=NULL; CHECK(!nativeHeldArticle_Status(&holder,&value));
    reset(); heads[HSD_GOBJ_PLINK_ITEM]=NULL; article.user_data=(void*)1;
    CHECK(nativeHeldArticle_Status(&holder,&value)); CHECK(value==-1 && !calls);
    reset(); item.xB8_itemLogicTable=&other_logic;
    CHECK(nativeHeldArticle_Status(&holder,&value)); CHECK(value==-1 && !calls);
    reset(); heads[HSD_GOBJ_PLINK_FIGHTER]=NULL; holder.user_data=(void*)1;
    CHECK(nativeHeldArticle_Status(&holder,&value)); CHECK(value==-1 && !calls);
    reset(); CHECK(nativeHeldArticle_Status(&holder,NULL)); CHECK(!calls);
    reset(); claimed=true;
    CHECK(!nativeHeldArticle_NotifyNeutral(&holder,&claimed)); CHECK(!claimed && !calls);
    policy.on_holder_neutral=neutral;
    CHECK(nativeHeldArticle_NotifyNeutral(&holder,&claimed)); CHECK(claimed && calls==1);
    // Neutral notification is independent of the status/request pair. Its
    // presence must not accidentally claim an ordinary status or request.
    reset(); policy.on_held_status=NULL; policy.on_held_request=NULL;
    policy.on_holder_neutral=neutral;
    CHECK(!nativeHeldArticle_Status(&holder,&value)); CHECK(value==-1 && !calls);
    CHECK(!nativeHeldArticle_Request(&holder,0,1,0)); CHECK(!calls);
    CHECK(nativeHeldArticle_NotifyNeutral(&holder,NULL)); CHECK(calls==1);
    for (unsigned why=1; why<=13; ++why) {
        reset(); policy.on_holder_neutral=neutral; hazard=why;
        claimed=false;
        CHECK(!nativeHeldArticle_NotifyNeutral(&holder,&claimed)); CHECK(claimed && calls==1);
    }
    reset(); policy.on_holder_neutral=neutral; hazard=14;
    CHECK(nativeHeldArticle_NotifyNeutral(&holder,NULL)); CHECK(depth_calls==64);
    hazard=0; CHECK(nativeHeldArticle_NotifyNeutral(&holder,NULL)); CHECK(calls==65);
    printf("Held article identity/dispatch checks: %u\n",checks); return 0;
}
