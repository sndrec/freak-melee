// Actual module signal bodies and callback lifetime/depth implementation.
// Only live-list setup, runtime package lookup and authored recipients are fake.
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/ef/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <windows.h>
#undef NDEBUG
#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FighterData, CallerData, RecipientData, FighterObject, CallerObject,
       RecipientObject, Package, ForeignPackage, Logics, Signals, PageCount };
static struct { void* data; size_t size; } pages[PageCount];
static Fighter* fighter;
static Item *source, *recipient;
static HSD_GObj *fighter_object, *caller_object, *recipient_object;
static NativeFighterDefinition *package, *foreign_package;
static NativeFighterArticleLogic* logics;
static NativeFighterArticleSignal* signals;
static ItemLogicTable item_logics[2], wrong_logic;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static unsigned checks, cases, calls, queries, lifecycle_cases;
static HSD_GObj* expected_recipient;
static float received_value;
static HSD_GObj* active_caller;
static unsigned active_class, remove_mask;
static bool reuse, protect_metadata;
static unsigned recursive_calls, recursive_rejections;
static unsigned char replacement_images[2][sizeof(Fighter) > sizeof(Item)
                                            ? sizeof(Fighter) : sizeof(Item)];
#define CHECK(x) do { ++checks; assert(x); } while (0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    ++queries;
    if(kind==Ft_Kind_CustomFirst || kind==Ft_Kind_CustomFirst+1) return package;
    return kind==Ft_Kind_CustomFirst+2?foreign_package:NULL;
}

EF_Effect* nativeModelEffect_Resolve(HSD_GObj* object,uint32_t id)
{ (void)object; (void)id; abort(); }
bool nativeModelEffect_Snapshot(HSD_GObj* object,uint32_t* id)
{ (void)object; (void)id; abort(); }

#include "article_signal_originals.h"

static void record_callback(HSD_GObj* object,float value)
{
    CHECK(object==expected_recipient);
    ++calls; received_value=value;
    Item* item=object->user_data;
    memcpy(item->xDD4_itemVar.native_article.state,&value,sizeof(value));
}

static void protect_page(unsigned index)
{
    DWORD old;
    CHECK(VirtualProtect(pages[index].data,pages[index].size,PAGE_NOACCESS,&old));
}

static void unlink_object(HSD_GObj* object,unsigned source_class)
{
    unsigned link=source_class==NativeCallbackSource_Fighter
        ?HSD_GOBJ_PLINK_FIGHTER:HSD_GOBJ_PLINK_ITEM;
    HSD_GObj** current=&heads[link];
    while(*current!=NULL && *current!=object) current=&(*current)->next;
    CHECK(*current==object);
    *current=object->next;
}

static void remove_or_reuse(HSD_GObj* object,unsigned source_class,
                            unsigned object_page,unsigned data_page,unsigned image)
{
    if(!reuse) {
        unlink_object(object,source_class);
        protect_page(object_page); protect_page(data_page);
        return;
    }
    // Same live address now belongs to a different generation and package.
    void* data=object->user_data;
    memset(data,0xA5,pages[data_page].size);
    if(source_class==NativeCallbackSource_Fighter) {
        Fighter* replacement=data;
        replacement->kind=Ft_Kind_CustomFirst+2;
        replacement->x8_spawnNum=(s32)0x92345678;
    } else {
        Item* replacement=data;
        replacement->kind=It_Kind_Ness_PKThunder1;
        replacement->x1C=0xABCDEF01;
        replacement->native_article_author_kind=Ft_Kind_CustomFirst+2;
        replacement->xB8_itemLogicTable=&item_logics[1];
    }
    memcpy(replacement_images[image],data,pages[data_page].size);
}

static void lifetime_callback(HSD_GObj* object,float value)
{
    record_callback(object,value);
    if(remove_mask&1) remove_or_reuse(active_caller,active_class,
        active_class==NativeCallbackSource_Fighter?FighterObject:CallerObject,
        active_class==NativeCallbackSource_Fighter?FighterData:CallerData,0);
    if(remove_mask&2) remove_or_reuse(object,NativeCallbackSource_Item,
        RecipientObject,RecipientData,1);
    if(protect_metadata) {
        // The dispatcher cached all three pointers before calling us.
        protect_page(Package); protect_page(Logics); protect_page(Signals);
    }
}

static void recursive_callback(HSD_GObj* object,float value)
{
    CHECK(object==recipient_object);
    ++recursive_calls;
    if(!signal_article(active_caller,recipient->x1C,0x12345678,value+1))
        ++recursive_rejections;
}

static void reset(void)
{
    for(unsigned i=0;i<PageCount;++i) {
        DWORD old;
        CHECK(VirtualProtect(pages[i].data,pages[i].size,PAGE_READWRITE,&old));
        memset(pages[i].data,0,pages[i].size);
    }
    memset(heads,0,sizeof(heads)); HSD_GObjPLinkHead=heads;
    fighter->kind=Ft_Kind_CustomFirst; fighter->x8_spawnNum=(s32)0x81234567;
    fighter_object->user_data=fighter;
    source->kind=It_Kind_Ness_PKThunder2; source->x1C=0xCAFED00D;
    source->native_article_author_kind=Ft_Kind_CustomFirst;
    source->xB8_itemLogicTable=&item_logics[0];
    caller_object->user_data=source; caller_object->next=recipient_object;
    recipient->kind=It_Kind_Ness_PKThunder1; recipient->x1C=0xFEDCBA98;
    recipient->native_article_author_kind=Ft_Kind_CustomFirst;
    recipient->xB8_itemLogicTable=&item_logics[1];
    recipient_object->user_data=recipient;
    heads[HSD_GOBJ_PLINK_FIGHTER]=fighter_object;
    heads[HSD_GOBJ_PLINK_ITEM]=caller_object;
    signals[0]=(NativeFighterArticleSignal){.name_hash=0x12345678,.callback=record_callback};
    logics[0]=(NativeFighterArticleLogic){.kind=source->kind,.logic=&item_logics[0]};
    logics[1]=(NativeFighterArticleLogic){.kind=recipient->kind,.logic=&item_logics[1],
        .signal_count=1,.signals=signals};
    *package=(NativeFighterDefinition){.article_logic_count=2,.article_logics=logics};
    *foreign_package=*package; // Identity, not structural equality, is required.
    calls=queries=0; received_value=NAN; expected_recipient=recipient_object;
    remove_mask=0; reuse=protect_metadata=false;
}

static HSD_GObj* caller(unsigned source_class)
{ return source_class==NativeCallbackSource_Fighter?fighter_object:caller_object; }

static void positive_matrix(void)
{
    const uint32_t handles[]={0,1,16777217,0x80000001,0xFEDCBA98,0xFFFFFFFE};
    const float values[]={0.0f,-0.0f,1.25f,-19.5f,FLT_MAX,-FLT_MIN};
    for(unsigned cls=0;cls<2;++cls) for(unsigned h=0;h<6;++h)
    for(unsigned alias=0;alias<2;++alias) for(unsigned ownership=0;ownership<4;++ownership)
    for(unsigned v=0;v<6;++v) {
        reset(); recipient->x1C=handles[h];
        recipient->native_article_author_kind=Ft_Kind_CustomFirst+alias;
        // A raw damage owner/reflector is never consulted by author dispatch.
        HSD_GObj* owner=ownership==0?NULL:ownership==1?fighter_object:
            ownership==2?caller_object:(HSD_GObj*)(uintptr_t)1;
        recipient->owner=owner; source->owner=owner;
        recipient->xC64_reflectGObj=owner; source->xC64_reflectGObj=owner;
        CHECK(article_signal_author(caller(cls))==package);
        CHECK(signal_article(caller(cls),handles[h],0x12345678,values[v]));
        CHECK(calls==1 && memcmp(&received_value,&values[v],sizeof(float))==0);
        CHECK(memcmp(recipient->xDD4_itemVar.native_article.state,&values[v],sizeof(float))==0);
        CHECK(source->owner==owner && recipient->owner==owner);
        ++cases;
    }
}

static void rejection_matrix(void)
{
    for(unsigned cls=0;cls<2;++cls) for(unsigned reason=0;reason<30;++reason) {
        reset(); HSD_GObj* sender=caller(cls);
        uint32_t handle=recipient->x1C, name=0x12345678;
        float value=2;
        switch(reason) {
        case 0: sender=NULL; break;
        case 1: HSD_GObjPLinkHead=NULL; break;
        case 2:
            unlink_object(sender,cls);
            protect_page(cls==NativeCallbackSource_Fighter?FighterObject:CallerObject);
            protect_page(cls==NativeCallbackSource_Fighter?FighterData:CallerData); break;
        case 3: sender->user_data=NULL; break;
        case 4:
            if(cls==NativeCallbackSource_Fighter) fighter->kind=Ft_Kind_Link;
            else source->native_article_author_kind=Ft_Kind_Link; break;
        case 5:
            if(cls==NativeCallbackSource_Fighter) fighter->kind=Ft_Kind_RuntimeCapacity;
            else source->native_article_author_kind=Ft_Kind_RuntimeCapacity; break;
        case 6:
            if(cls==NativeCallbackSource_Fighter) fighter->kind=Ft_Kind_CustomFirst+3;
            else source->native_article_author_kind=Ft_Kind_CustomFirst+3; break;
        case 7: package->article_logics=NULL; break;
        case 8:
            if(cls==NativeCallbackSource_Item) source->kind=It_Kind_Ness_PKFire;
            else package->article_logic_count=0; break;
        case 9:
            if(cls==NativeCallbackSource_Item) source->xB8_itemLogicTable=&wrong_logic;
            else package->article_logic_count=0; break;
        case 10:
            if(cls==NativeCallbackSource_Item) logics[0].logic=NULL;
            else package->article_logic_count=0; break;
        case 11: unlink_object(recipient_object,NativeCallbackSource_Item);
            protect_page(RecipientObject); protect_page(RecipientData); break;
        case 12: recipient_object->user_data=NULL; break;
        case 13: --handle; break;
        case 14: handle=UINT32_MAX; recipient->x1C=UINT32_MAX; break;
        case 15: recipient->native_article_author_kind=Ft_Kind_CustomFirst+2; break;
        case 16: recipient->native_article_author_kind=Ft_Kind_Link; break;
        case 17: recipient->native_article_author_kind=Ft_Kind_RuntimeCapacity; break;
        case 18: recipient->kind=It_Kind_Ness_PKFire; break;
        case 19: recipient->xB8_itemLogicTable=&wrong_logic; break;
        case 20: logics[1].logic=NULL; break;
        case 21: --name; break;
        case 22: signals[0].callback=NULL; break;
        case 23: logics[1].signal_count=0; logics[1].signals=NULL; break;
        case 24: value=NAN; break;
        case 25: value=INFINITY; break;
        case 26: value=-INFINITY; break;
        case 27: heads[HSD_GOBJ_PLINK_ITEM]=NULL; break;
        case 28: source->native_article_author_kind=(FighterKind)-1;
            if(cls==NativeCallbackSource_Fighter) fighter->kind=(FighterKind)-1; break;
        case 29:
            if(cls==NativeCallbackSource_Fighter) fighter->kind=Ft_Kind_CustomFirst+2;
            else source->native_article_author_kind=Ft_Kind_CustomFirst+2; break;
        }
        CHECK(!signal_article(sender,handle,name,value));
        CHECK(calls==0);
        if(reason==14 || (reason>=24 && reason<=26)) CHECK(queries==0);
        ++cases;
    }
}

static void depth_matrix(void)
{
    reset();
    for(unsigned i=0;i<64;++i) CHECK(nativeCallbackDispatch_Enter());
    CHECK(!nativeCallbackDispatch_Enter());
    CHECK(!signal_article(fighter_object,recipient->x1C,0x12345678,1));
    CHECK(calls==0);
    for(unsigned i=0;i<64;++i) nativeCallbackDispatch_Leave();
    CHECK(signal_article(fighter_object,recipient->x1C,0x12345678,1));
    CHECK(calls==1);
    for(unsigned cls=0;cls<2;++cls) {
        reset(); active_caller=caller(cls); recursive_calls=recursive_rejections=0;
        signals[0].callback=recursive_callback;
        CHECK(signal_article(active_caller,recipient->x1C,0x12345678,0));
        CHECK(recursive_calls==64 && recursive_rejections==1);
        signals[0].callback=record_callback;
        CHECK(signal_article(active_caller,recipient->x1C,0x12345678,1));
        CHECK(calls==1); ++cases;
    }
    ++cases;
}

static void lifetime_matrix(void)
{
    for(unsigned cls=0;cls<2;++cls) for(unsigned mask=1;mask<=3;++mask)
    for(unsigned replace=0;replace<2;++replace) {
        reset(); active_class=cls; active_caller=caller(cls);
        uint32_t caller_id=cls==NativeCallbackSource_Fighter
            ?(uint32_t)fighter->x8_spawnNum:source->x1C;
        uint32_t recipient_id=recipient->x1C;
        remove_mask=mask; reuse=replace!=0; protect_metadata=!reuse;
        signals[0].callback=lifetime_callback;
        CHECK(signal_article(active_caller,recipient_id,0x12345678,3));
        CHECK(calls==1);
        CHECK(nativeCallbackSource_Alive(active_caller,caller_id,cls)==!(mask&1));
        CHECK(nativeCallbackSource_Alive(recipient_object,recipient_id,
                                         NativeCallbackSource_Item)==!(mask&2));
        if(reuse) {
            if(mask&1) {
                unsigned p=cls==NativeCallbackSource_Fighter?FighterData:CallerData;
                CHECK(memcmp(pages[p].data,replacement_images[0],pages[p].size)==0);
            }
            if(mask&2) CHECK(memcmp(recipient,replacement_images[1],sizeof(*recipient))==0);
        }
        ++cases; ++lifecycle_cases;
    }
}

int main(void)
{
    const size_t sizes[PageCount]={sizeof(Fighter),sizeof(Item),sizeof(Item),
        sizeof(HSD_GObj),sizeof(HSD_GObj),sizeof(HSD_GObj),
        sizeof(NativeFighterDefinition),sizeof(NativeFighterDefinition),
        2*sizeof(NativeFighterArticleLogic),sizeof(NativeFighterArticleSignal)};
    for(unsigned i=0;i<PageCount;++i) {
        pages[i].size=sizes[i];
        pages[i].data=VirtualAlloc(NULL,sizes[i],MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        CHECK(pages[i].data!=NULL);
    }
    fighter=pages[FighterData].data; source=pages[CallerData].data;
    recipient=pages[RecipientData].data; fighter_object=pages[FighterObject].data;
    caller_object=pages[CallerObject].data; recipient_object=pages[RecipientObject].data;
    package=pages[Package].data; foreign_package=pages[ForeignPackage].data;
    logics=pages[Logics].data; signals=pages[Signals].data;
    positive_matrix(); rejection_matrix(); depth_matrix(); lifetime_matrix();
    for(unsigned i=0;i<PageCount;++i) CHECK(VirtualFree(pages[i].data,0,MEM_RELEASE));
    printf("{\"checks\":%u,\"cases\":%u,\"lifetime_cases\":%u,\"dispatch_depth\":64}\n",
           checks,cases,lifecycle_cases);
    return 0;
}
