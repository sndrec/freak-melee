#include <melee/ft/fighter.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ef/native_model_effect.h>
#include <melee/it/native_item_owner.h>
#include <melee/it/native_item_behavior.h>
#include <melee/it/native_article.h>
#include <melee/it/item.h>
#include <melee/it/inlines.h>
#include <melee/it/it_2725.h>
#include <melee/it/it_3F14.h>
#include <melee/it/it_3F2F.h>
#include <melee/it/it_279C.h>
#include <melee/it/ithitbox.h>
#include <melee/it/native_holdable.h>
#include <melee/it/native_script_item_actor.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/gobjuserdata.h>
#include <sysdolphin/baselib/gobjproc.h>
#include <sysdolphin/baselib/objalloc.h>
#include <sysdolphin/baselib/native_gobj_pool_storage.h>
#include <melee/ft/native_fighter_pool.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

enum Mutation { None, SourceRemoved, SourceReused, SourceKind, SourceDefinition,
                ItemRemoved, ItemReused, CreatorChanged };
static unsigned checks, cases, constructor_calls, removals;
static unsigned attachment_calls, pickup_calls, pickup_tail, scale_calls, notification_calls;
static enum Mutation mutation;
static bool mutation_at_notification, allocation_failure;
static Fighter_Part expected_part;
static HSD_GObj owner_objects[3];
#define source owner_objects[0]
#define other owner_objects[1]
#define object owner_objects[2]
NativeMatchPool nativeGObjPool, nativeFighterPool;
static NativeMatchPoolSlot owner_slots[3];
static uint32_t owner_free_order[3];
static Fighter fighter, foreign;
static FighterBone bones[MAX_FT_PARTS];
static Item item;
static HSD_JObj joint, part_joint;
static ItemLogicTable item_logic;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead=heads;
static const NativeFighterDefinition first_definition={0}, second_definition={0};
static const NativeFighterDefinition* definitions[Ft_Kind_RuntimeCapacity];
static FighterPartsTable rig;
static FighterPartsTable* rigs[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable=rigs;
static SpawnItem observed_spawn;
static unsigned creation_steps[32], step_count;
#define REQUIRE(x) do { ++checks; if(!(x)) { fprintf(stderr,"case %u line %d: %s\n",cases,__LINE__,#x); exit(1); } } while(0)
static void step(unsigned value) { REQUIRE(step_count<32); creation_steps[step_count++]=value; }
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return (unsigned)kind<Ft_Kind_RuntimeCapacity ? definitions[kind] : NULL; }
bool nativeModelEffect_Snapshot(HSD_GObj* g,uint32_t* generation) { (void)g; *generation=0; return false; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* g,uint32_t generation) { (void)g;(void)generation;return NULL; }
void __assert(char* file,u32 line,char* text) { fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,text); exit(1); }
static void invalidate(enum Mutation change)
{
    if(change==SourceRemoved) { heads[HSD_GOBJ_PLINK_FIGHTER]=&other; source.user_data=(void*)(uintptr_t)1; }
    else if(change==SourceReused) ++fighter.x8_spawnNum;
    else if(change==SourceKind) fighter.kind=Ft_Kind_CustomFirst+1;
    else if(change==SourceDefinition) definitions[fighter.kind]=&second_definition;
    else if(change==ItemRemoved) { heads[HSD_GOBJ_PLINK_ITEM]=NULL; object.user_data=(void*)(uintptr_t)1; }
    else if(change==ItemReused) ++item.x1C;
    else if(change==CreatorChanged) nativeItem_CaptureCreator(&object,&other);
}
static void pickup(HSD_GObj* g) { REQUIRE(g==&object); ++pickup_calls; if(!mutation_at_notification) invalidate(mutation); }
void it_8026BB20(HSD_GObj* g) { REQUIRE(g==&object); step(20); }
void it_8026BB44(HSD_GObj* g) { REQUIRE(g==&object); step(21); }
void it_8026B724(HSD_GObj* g) { REQUIRE(g==&object); step(22); }
void it_8026B73C(HSD_GObj* g) { REQUIRE(g==&object); step(23); }
void ftCommon_8007E6DC(HSD_GObj* g,HSD_GObj* held,s32 normal)
{ REQUIRE(g==&source && held==&object && normal); step(24); invalidate(mutation); }

// Engine creation is an explicit controlled leaf. Full historical factory
// extraction is retired; constructor provenance/admission and native teardown
// are covered by script_constructor_items.
HSD_GObj* Item_CreateEngine(SpawnItem* spawn)
{
    if(spawn->kind!=It_Kind_BombHei && spawn->kind!=It_Kind_Dosei && spawn->kind!=It_Kind_Sword)return NULL;
    observed_spawn=*spawn;++constructor_calls;
    if(allocation_failure)return NULL;
    item.kind=spawn->kind;item.native_article_author_kind=Ft_Kind_None;
    nativeItem_CaptureCreator(&object,spawn->x0_parent_gobj);
    invalidate(mutation);return &object;
}
void Item_8026A8EC(HSD_GObj* g) { REQUIRE(g==&object);++removals;heads[HSD_GOBJ_PLINK_ITEM]=NULL;g->user_data=(void*)(uintptr_t)1; }
bool nativeScriptItem_IsAttached(Item_GObj* g){(void)g;return false;}
NativeScriptVMStatus nativeScriptItem_RunHook(Item_GObj* g,uint32_t h,const NativeScriptValue* a,uint32_t n,NativeScriptValue* r){(void)g;(void)h;(void)a;(void)n;(void)r;abort();}
NativeScriptVMStatus nativeHoldable_PickedUp(Item_GObj* g,NativeScriptValue* r){(void)g;(void)r;abort();}
void it_80273168(HSD_GObj* g) { REQUIRE(g==&object); }
void it_802742F4(HSD_GObj* g,HSD_GObj* owner,Fighter_Part part)
{ REQUIRE(g==&object && owner==&source && part==expected_part); ++attachment_calls; item.owner=owner; }
void Item_8026B074(Item* data) { REQUIRE(data==&item); ++pickup_tail; }
void db_80225DD8(Item_GObj* g, Fighter_GObj* owner)
{
    REQUIRE(g==&object && owner==&source && attachment_calls==1 && pickup_tail==1);
    ++notification_calls;
    if(mutation_at_notification) invalidate(mutation);
}
void (HSD_JObjSetMtxDirty)(HSD_JObj* g) { REQUIRE(g==&joint); }
void it_80275534(HSD_GObj* g,float value)
{ REQUIRE(g==&object && value==item.x3C && joint.scale.x==item.scl && joint.scale.y==item.scl && joint.scale.z==item.scl); ++scale_calls; }
void it_80274DFC(HSD_GObj* g) { REQUIRE(g==&object && scale_calls==1); ++scale_calls; }
void it_80274E44(HSD_GObj* g) { REQUIRE(g==&object && scale_calls==2); ++scale_calls; }

#undef source
#undef other
#undef object
#include "item_owner_actual.inc"
#define source owner_objects[0]
#define other owner_objects[1]
#define object owner_objects[2]

static void setup(void)
{
    ++cases;
    if(getenv("ITEM_OWNER_DEBUG")) fprintf(stderr,"fixture case %u\n",cases);
    memset(&source,0,sizeof source); memset(&other,0,sizeof other); memset(&object,0,sizeof object);
    memset(&fighter,0,sizeof fighter); memset(&foreign,0,sizeof foreign); memset(&item,0,sizeof item);
    memset(&joint,0,sizeof joint); memset(heads,0,sizeof heads); memset(definitions,0,sizeof definitions); memset(rigs,0,sizeof rigs);
    memset(&item_logic,0,sizeof item_logic); memset(&observed_spawn,0,sizeof observed_spawn);
    REQUIRE(nativeMatchPool_Bind(&nativeGObjPool, owner_objects,
        sizeof(owner_objects), owner_slots, owner_free_order, 3,
        sizeof(HSD_GObj), _Alignof(HSD_GObj), cases));
    NativeMatchPoolReference owner_reference;
    REQUIRE(nativeMatchPool_Allocate(&nativeGObjPool, &owner_reference)==&source);
    REQUIRE(nativeMatchPool_Allocate(&nativeGObjPool, &owner_reference)==&other);
    REQUIRE(nativeMatchPool_Allocate(&nativeGObjPool, &owner_reference)==&object);
    source.user_data=&fighter; source.next=&other; other.user_data=&foreign;
    fighter.kind=Ft_Kind_CustomFirst; foreign.kind=Ft_Kind_CustomFirst+1;
    fighter.x8_spawnNum=0xC0000001U; foreign.x8_spawnNum=0xC0000002U;
    definitions[fighter.kind]=&first_definition; definitions[foreign.kind]=&first_definition;
    object.user_data=&item; object.hsd_obj=&joint; item.native_article_author_kind=Ft_Kind_None;
    item.x1C=0xFEDCBA98U; item.owner=&source; item.scl=1.25f; item.throw_scale=.75f;
    item.xB8_itemLogicTable=&item_logic; item_logic.picked_up=pickup;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&source; heads[HSD_GOBJ_PLINK_ITEM]=&object;
    nativeItem_CaptureCreator(&object,&source);
    memset(bones,0,sizeof bones); fighter.parts=bones;
    rig.parts_num=4; rigs[fighter.kind]=&rig; fighter.parts[3].joint=&part_joint; fighter.parts[4].joint=&part_joint;
    fighter.x34_scale.y=2; fighter.co_attrs.model_scaling=99; foreign.x34_scale.y=.5f;
    mutation=None; mutation_at_notification=allocation_failure=false; expected_part=3;
    notification_calls=0;
    constructor_calls=removals=step_count=attachment_calls=pickup_calls=pickup_tail=scale_calls=0;
}
static void creator_tests(void)
{
    setup(); REQUIRE(nativeItem_Creator(&object)==&source); item.owner=&other; REQUIRE(nativeItem_Creator(&object)==&source);
    for(enum Mutation m=SourceRemoved;m<=SourceDefinition;m++) { setup(); invalidate(m); REQUIRE(nativeItem_Creator(&object)==NULL); }
    setup(); heads[HSD_GOBJ_PLINK_ITEM]=NULL; object.user_data=(void*)(uintptr_t)1; REQUIRE(nativeItem_Creator(&object)==NULL);
    setup(); source.user_data=NULL; REQUIRE(nativeItem_Creator(&object)==NULL);
    setup(); nativeItem_CaptureCreator(&object,NULL); REQUIRE(item.native_creator==NULL && item.native_creator_generation==0 && item.native_creator_kind==Ft_Kind_None && item.native_creator_definition==NULL);
    setup(); heads[HSD_GOBJ_PLINK_FIGHTER]=&other; source.user_data=(void*)(uintptr_t)1; nativeItem_CaptureCreator(&object,&source); REQUIRE(item.native_creator==NULL);
    setup(); definitions[fighter.kind]=NULL; nativeItem_CaptureCreator(&object,&source); REQUIRE(nativeItem_Creator(&object)==&source);
}
static void binding_tests(void)
{
    const uint32_t handles[]={0,1,0x80000001U,0xFEDCBA98U,0xFFFFFFFEU};
    for(unsigned authored=0;authored<2;authored++) for(unsigned h=0;h<5;h++) for(unsigned aux=0;aux<2;aux++) {
        setup(); item.x1C=handles[h]; item.owner=&other;
        if(authored) { item.native_article_author_kind=fighter.kind; item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum; item.xDD4_itemVar.native_article.attachment_owner=&source; }
        REQUIRE(nativeItem_BindFighter(&source,handles[h],aux)); REQUIRE((uint32_t)nativeItem_HeldId(&source,aux)==handles[h]);
        REQUIRE((aux ? fighter.x1984_heldItemSpec : fighter.item_gobj)==&object);
        REQUIRE(nativeItem_HeldId(&source,!aux)==-1); REQUIRE(!nativeItem_BindFighter(&other,handles[h],aux));
        REQUIRE(!nativeItem_BindFighter(&source,UINT32_MAX,aux)); REQUIRE(!nativeItem_BindFighter(&source,handles[h]^0x10U,aux));
        REQUIRE(nativeItem_ClearBinding(&source,aux)); REQUIRE(nativeItem_HeldId(&source,aux)==-1);
    }
    setup(); REQUIRE(nativeItem_BindFighter(&source,item.x1C,false)); heads[HSD_GOBJ_PLINK_ITEM]=NULL; object.user_data=(void*)(uintptr_t)1; REQUIRE(nativeItem_HeldId(&source,false)==-1);
    setup(); fighter.item_gobj=&object; ++item.x1C; REQUIRE((uint32_t)nativeItem_HeldId(&source,false)==0xFEDCBA99U);
    setup(); item.native_article_author_kind=fighter.kind; item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum; item.xDD4_itemVar.native_article.attachment_owner=&source;
    ++fighter.x8_spawnNum; REQUIRE(!nativeItem_BindFighter(&source,item.x1C,false));
    setup(); item.native_article_author_kind=foreign.kind; item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum; item.xDD4_itemVar.native_article.attachment_owner=&source;
    REQUIRE(!nativeItem_BindFighter(&source,item.x1C,false)); // Same package pointer does not grant cross-kind binding.
    setup(); heads[HSD_GOBJ_PLINK_FIGHTER]=&other; source.user_data=(void*)(uintptr_t)1;
    REQUIRE(!nativeItem_ClearBinding(&source,false) && nativeItem_HeldId(&source,false)==-1 && !nativeItem_BindFighter(&source,0xFEDCBA98U,false));
    setup(); item.native_article_author_kind=fighter.kind; item.xDD4_itemVar.native_article.creator_spawn_id=fighter.x8_spawnNum; item.xDD4_itemVar.native_article.attachment_owner=&source;
    definitions[fighter.kind]=&second_definition;
    REQUIRE(nativeItem_Creator(&object)==NULL); // Captured identity rejects replacement.
    REQUIRE(nativeArticle_Creator(&object)==&source && nativeItem_BindFighter(&source,item.x1C,false));
    REQUIRE(nativeItem_Attach(&source,item.x1C,3,false)); // Deliberately preserved authored boundary uses current registry.
    setup(); heads[HSD_GOBJ_PLINK_ITEM]=NULL; object.user_data=NULL; REQUIRE(!nativeItem_BindFighter(&source,0xFEDCBA98U,false));
    setup(); HSD_GObjPLinkHead=NULL; REQUIRE(!nativeItem_BindFighter(&source,item.x1C,false) && !nativeItem_ClearBinding(&source,false) && nativeItem_HeldId(&source,false)==-1); HSD_GObjPLinkHead=heads;
}
static void existing_item_tests(void)
{
    const uint32_t handles[]={0,1,0x80000001U,0xFEDCBA98U,0xFFFFFFFEU};
    for(unsigned h=0;h<5;h++) for(unsigned owner=0;owner<3;owner++) for(unsigned aux=0;aux<2;aux++) {
        setup(); item.x1C=handles[h];
        item.owner=owner==0 ? NULL : owner==1 ? &source : &other;
        nativeItem_CaptureCreator(&object,&other);
        Item before=item;
        REQUIRE(!nativeItem_BindFighter(&source,handles[h],aux));
        REQUIRE(nativeItem_BindExisting(&source,handles[h],aux));
        REQUIRE((aux ? fighter.x1984_heldItemSpec : fighter.item_gobj)==&object);
        REQUIRE(memcmp(&before,&item,sizeof item)==0); // Binding is not pickup/ownership/attachment.
        REQUIRE(nativeItem_SetVisible(handles[h],false)); REQUIRE(nativeItem_SetPaused(handles[h],true));
        REQUIRE(nativeItem_SetVisible(handles[h],true)); REQUIRE(nativeItem_SetPaused(handles[h],false));
        REQUIRE(step_count==4 && creation_steps[0]==21 && creation_steps[1]==22 && creation_steps[2]==20 && creation_steps[3]==23);
        REQUIRE(nativeItem_NotifyDrop(&source,handles[h],true)); REQUIRE(creation_steps[4]==24);
    }
    for(enum Mutation m=SourceRemoved;m<=ItemReused;m++) {
        setup(); mutation=m; uint32_t handle=item.x1C;
        REQUIRE(!nativeItem_NotifyDrop(&source,handle,true)); REQUIRE(step_count==1);
    }
    setup(); uint32_t old=item.x1C; ++item.x1C;
    REQUIRE(!nativeItem_BindExisting(&source,old,false));
    REQUIRE(!nativeItem_SetVisible(old,true) && !nativeItem_SetPaused(old,false));
    REQUIRE(!nativeItem_NotifyDrop(&source,old,true) && step_count==0);
    setup(); heads[HSD_GOBJ_PLINK_ITEM]=NULL; object.user_data=(void*)(uintptr_t)1;
    REQUIRE(!nativeItem_BindExisting(&source,0xFEDCBA98U,false));
    REQUIRE(!nativeItem_SetVisible(0xFEDCBA98U,true) && !nativeItem_SetPaused(0xFEDCBA98U,false));
    REQUIRE(!nativeItem_NotifyDrop(&source,0xFEDCBA98U,true));
    setup(); heads[HSD_GOBJ_PLINK_FIGHTER]=&other; source.user_data=(void*)(uintptr_t)1;
    REQUIRE(!nativeItem_BindExisting(&source,item.x1C,false));
    REQUIRE(!nativeItem_NotifyDrop(&source,item.x1C,true));
    setup(); object.hsd_obj=NULL; REQUIRE(!nativeItem_SetVisible(item.x1C,true));
    REQUIRE(nativeItem_SetPaused(item.x1C,true));
    setup(); REQUIRE(!nativeItem_SetVisible(UINT32_MAX,true) && !nativeItem_SetPaused(UINT32_MAX,true));
    REQUIRE(!nativeItem_BindExisting(&source,UINT32_MAX,false));
    setup(); HSD_GObjPLinkHead=NULL;
    REQUIRE(!nativeItem_BindExisting(&source,item.x1C,false));
    REQUIRE(!nativeItem_SetVisible(item.x1C,true) && !nativeItem_SetPaused(item.x1C,true));
    REQUIRE(!nativeItem_NotifyDrop(&source,item.x1C,true)); HSD_GObjPLinkHead=heads;
}

static void creation_tests(void)
{
    Vec3 position={1,2,3},velocity={4,5,6};
    setup(); REQUIRE((uint32_t)nativeItem_CreateEngine(&source,It_Kind_BombHei,&position,&velocity,-1,0xFEDCBA98U,3.75f)==0xFEDCBA98U);
    REQUIRE(observed_spawn.kind==It_Kind_BombHei && !memcmp(&observed_spawn.pos,&position,sizeof position) && !memcmp(&observed_spawn.prev_pos,&position,sizeof position));
    REQUIRE(!memcmp(&observed_spawn.vel,&velocity,sizeof velocity) && observed_spawn.facing_dir==-1 && (uint32_t)observed_spawn.x40==0xFEDCBA98U && observed_spawn.x3C_damage==3);
    REQUIRE(observed_spawn.x0_parent_gobj==&source && observed_spawn.x4_parent_gobj2==&source && constructor_calls==1 && !removals);
    for(enum Mutation m=SourceRemoved;m<=CreatorChanged;m++){
        setup(); mutation=m;int32_t result=nativeItem_CreateEngine(&source,It_Kind_Dosei,&position,&velocity,1,0,0);REQUIRE(m==ItemReused?(uint32_t)result==(uint32_t)item.x1C:result==-1);
        REQUIRE(removals==((m==ItemRemoved || m==ItemReused)?0u:1u));
    }
    setup();allocation_failure=true;REQUIRE(nativeItem_CreateEngine(&source,It_Kind_BombHei,&position,&velocity,1,0,0)==-1 && constructor_calls==1 && !removals);
    const float bad[]={NAN,INFINITY,-INFINITY,32768,-32769};
    for(unsigned i=0;i<5;i++){setup();REQUIRE(nativeItem_CreateEngine(&source,It_Kind_BombHei,&position,&velocity,1,0,bad[i])==-1 && !constructor_calls);}
    for(unsigned channel=0;channel<8;channel++){
        setup();Vec3 p=position,v=velocity;float f=1;float* components[]={&p.x,&p.y,&p.z,&v.x,&v.y,&v.z};
        if(channel<6)*components[channel]=channel<3?NAN:INFINITY;else f=NAN;
        REQUIRE(nativeItem_CreateEngine(&source,It_Kind_BombHei,channel==7?NULL:&p,&v,f,0,0)==-1 && !constructor_calls);
    }
    setup();REQUIRE(nativeItem_CreateEngine(&source,It_Kind_BombHei,&position,NULL,1,0,0)==-1 && !constructor_calls);
    setup();REQUIRE(nativeItem_CreateEngine(&source,It_Kind_None,&position,&velocity,1,0,0)==-1 && !constructor_calls);
    setup();REQUIRE(nativeItem_CreateEngine(NULL,It_Kind_BombHei,&position,&velocity,1,0,0)==-1 && !constructor_calls);
    const float accepted[]={-32768,32767,-3.75f};
    for(unsigned i=0;i<3;i++){setup();item.x1C=i==0?0:i==1?0x80000001U:0xFFFFFFFEU;REQUIRE((uint32_t)nativeItem_CreateEngine(&source,It_Kind_BombHei,&position,&velocity,1,0,accepted[i])==(uint32_t)item.x1C);REQUIRE(observed_spawn.x3C_damage==(s16)accepted[i]);}
}
static void attachment_scale_tests(void)
{
    setup(); REQUIRE(nativeItem_Attach(&source,item.x1C,3,true));
    REQUIRE(notification_calls==1 && fighter.item_gobj==NULL && fighter.x1984_heldItemSpec==NULL);
    for(enum Mutation m=SourceRemoved;m<=CreatorChanged;m++) {
        setup(); mutation=m; mutation_at_notification=true;
        REQUIRE(!nativeItem_Attach(&source,item.x1C,3,true) && notification_calls==1);
    }
    setup(); REQUIRE(nativeItem_Attach(&source,item.x1C,3,false) && attachment_calls==1 && pickup_calls==1 && pickup_tail==1 && notification_calls==0);
    setup(); item_logic.picked_up=NULL; REQUIRE(nativeItem_Attach(&source,item.x1C,3,false) && pickup_calls==0 && pickup_tail==1);
    for(enum Mutation m=SourceRemoved;m<=CreatorChanged;m++) {
        setup(); mutation=m; REQUIRE(!nativeItem_Attach(&source,item.x1C,3,false));
        REQUIRE(attachment_calls==1 && pickup_calls==1 && pickup_tail==((m==ItemRemoved || m==ItemReused) ? 0 : 1) && notification_calls==0);
    }
    const int bad_parts[]={-1,4,MAX_FT_PARTS};
    for(unsigned i=0;i<3;i++) { setup(); REQUIRE(!nativeItem_Attach(&source,item.x1C,bad_parts[i],false) && attachment_calls==0); }
    setup(); fighter.parts[3].joint=NULL; REQUIRE(!nativeItem_Attach(&source,item.x1C,3,false) && attachment_calls==0);
    setup(); rigs[fighter.kind]=NULL; REQUIRE(!nativeItem_Attach(&source,item.x1C,3,false) && attachment_calls==0);
    setup(); ftPartsTable=NULL; REQUIRE(!nativeItem_Attach(&source,item.x1C,3,false) && attachment_calls==0); ftPartsTable=rigs;
    setup(); expected_part=MAX_FT_PARTS-1; rig.parts_num=MAX_FT_PARTS; bones[expected_part].joint=&part_joint;
    REQUIRE(nativeItem_Attach(&source,item.x1C,expected_part,false));
    setup(); fighter.kind=Ft_Kind_RuntimeCapacity; nativeItem_CaptureCreator(&object,&source);
    REQUIRE(!nativeItem_Attach(&source,item.x1C,3,false) && attachment_calls==0);
    for(unsigned current_owner=0;current_owner<2;current_owner++) for(unsigned remember=0;remember<2;remember++) {
        setup(); item.owner=current_owner ? &other : &source;
        REQUIRE(nativeItem_InheritOwnerScale(&source,item.x1C,remember) && scale_calls==3);
        float expected=1.25f*(current_owner ? .5f : 2);
        REQUIRE(item.scl==expected && item.throw_scale==(remember ? expected : .75f));
        REQUIRE(nativeItem_Creator(&object)==&source);
    }
    setup(); item.owner=NULL; REQUIRE(!nativeItem_InheritOwnerScale(&source,item.x1C,true) && scale_calls==0);
    setup(); item.owner=&other; source.next=NULL; REQUIRE(!nativeItem_InheritOwnerScale(&source,item.x1C,true) && scale_calls==0);
    setup(); object.hsd_obj=NULL; REQUIRE(!nativeItem_InheritOwnerScale(&source,item.x1C,true) && scale_calls==0);
    setup(); ++fighter.x8_spawnNum; item.owner=&other; REQUIRE(!nativeItem_InheritOwnerScale(&source,item.x1C,true) && scale_calls==0);
}
int main(void)
{
    creator_tests(); binding_tests(); existing_item_tests(); creation_tests(); attachment_scale_tests();
    printf("PASS %u owner/backend cases, %u assertions; actual owner/AB54/scale bodies and physical creator pool; controlled engine factory\n",cases,checks);
    return 0;
}
