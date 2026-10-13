#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_events.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/ftlib.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <melee/ft/kinds/ftLink/ftlinkspecialn.h>
#include <melee/ft/kinds/ftLink/ftlink.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/kinds/ftKirby/ftkirby.h>
#include <melee/it/item.h>
#include <melee/it/kinds/itlinkbow.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* ModuleEntry)(
    const NativeFighterDefinition*, const NativeFighterHost*);
typedef struct BowEvent { int op, state, flags; float value; } BowEvent;
typedef struct Trace { BowEvent events[16]; unsigned count; } Trace;
enum { Change = 1, Advance, Frame, Interaction };
static Trace* trace;
static Fighter fighters[2];
static HSD_GObj owners[2], objects[2], alternate[2];
static Fighter alternate_data[2];
static Item items[2];
static HSD_JObj roots[2];
static unsigned comparisons, failures, restored_cleanup_cases, missing_handler_cases;
static const NativeFighterDefinition* author_definition;
static const NativeFighterDefinition* recipient_definition;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
static unsigned author_handle_offset, recipient_handle_offset;
static const NativeFighterArticleLogic* authored;

void __assert(char* file, u32 line, char* message)
{ fprintf(stderr, "%s:%u: %s\n", file, (unsigned)line, message); abort(); }
static void event(int op, int state, int flags, float value)
{ if (trace->count == 16) abort(); trace->events[trace->count++] = (BowEvent){op,state,flags,value}; }
static Item* data(HSD_GObj* object) { return object->user_data; }
HSD_GObj* ftLib_8008741C(u32 target)
{
    for(HSD_GObj* live=heads[HSD_GOBJ_PLINK_FIGHTER];live;live=live->next)
        if(live->user_data && (uint32_t)((Fighter*)live->user_data)->x8_spawnNum==(uint32_t)target) return live;
    return NULL;
}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    return kind==Ft_Kind_CustomFirst?author_definition:
        kind==Ft_Kind_CustomFirst+1?recipient_definition:NULL;
}
static Fighter_GObj* fixture_creator(Item_GObj* gobj)
{
    if(!gobj||!gobj->user_data)return NULL;
    Item* item=data(gobj);
    HSD_GObj* creator=ftLib_8008741C(item->xDD4_itemVar.native_article.creator_spawn_id);
    return creator && creator==item->xDD4_itemVar.native_article.attachment_owner &&
        nativeFighterRuntime_Definition(((Fighter*)creator->user_data)->kind)==author_definition?creator:NULL;
}
static uint32_t current_owner(Item_GObj* object)
{
    HSD_GObj* owner=data(object)->owner;
    for(HSD_GObj* live=heads[HSD_GOBJ_PLINK_FIGHTER];live;live=live->next)
        if(live==owner && live->user_data)return (uint32_t)((Fighter*)live->user_data)->x8_spawnNum;
    return 0;
}
static unsigned field_offset(const NativeFighterDefinition* definition,const char* name)
{
    for(unsigned i=0;i<definition->state_field_count;++i)
        if(!strcmp(definition->state_fields[i].name,name))return definition->state_fields[i].offset;
    abort();
}
void Item_80268E5C(HSD_GObj* object, enum_t state, Item_StateChangeFlags flags)
{ event(Change,state,flags,0); data(object)->msid=state; data(object)->x5CC_currentAnimFrame=100+state; }
void Item_802694CC(HSD_GObj* object)
{ event(Advance,0,0,0); data(object)->x5CC_currentAnimFrame+=.5f; }
void Item_80268DD4(HSD_GObj* object, float frame)
{ event(Frame,0,0,frame); data(object)->x5CC_currentAnimFrame=frame; }
void HSD_JObjSetMtxDirtySub(HSD_JObj* root) { root->flags |= JOBJ_MTX_DIRTY; }
static void scale(Item_GObj* object, float value)
{ Vec3 values={value,value,value}; HSD_JObjSetScale(object->hsd_obj,&values); }
float ftLib_800869D4(Fighter_GObj* object) { return ((Fighter*)object->user_data)->x34_scale.y; }
/* Kirby-only branches are deliberately unreachable in this Link/Young Link
 * fixture. The ignored stock Boomerang query is compiled from ftlink.c. */
ftLk_SpecialNIndex ftKb_SpecialNLk800FB394(Fighter_GObj* object)
{ (void)object; abort(); }
void ftKb_SpecialNLk800FB444(Fighter_GObj* object) { (void)object; abort(); }
bool ftKb_SpecialNLk800FB470(Fighter_GObj* object) { (void)object; abort(); }
Item_GObj* Item_80268B18(SpawnItem* spawn) { (void)spawn; abort(); }
void Item_8026AB54(Item_GObj* object, HSD_GObj* owner, Fighter_Part part)
{ (void)object;(void)owner;(void)part;abort(); }
void Item_8026A8EC(Item_GObj* object) { (void)object; abort(); }
bool it_8026B894(Item_GObj* object, HSD_GObj* other)
{ (void)object;(void)other;event(Interaction,0,0,0);return false; }

static int owner_handle(unsigned index)
{ int value; memcpy(&value,fighters[index].native_module_state+author_handle_offset,4);return value; }
static int recipient_handle(void)
{ int value; memcpy(&value,alternate_data[1].native_module_state+recipient_handle_offset,4);return value; }
static void reset(ItemKind kind, unsigned state, int motion, bool has_reference,
                  unsigned owner_case, bool detaches, float frame)
{
    memset(items,0,sizeof(items));memset(roots,0,sizeof(roots));
    memset(fighters,0,sizeof(fighters));memset(alternate_data,0,sizeof(alternate_data));
    for(unsigned i=0;i<2;++i) {
        owners[i]=(HSD_GObj){.user_data=&fighters[i]};
        alternate[i]=(HSD_GObj){.user_data=&alternate_data[i]};
        objects[i]=(HSD_GObj){.user_data=&items[i],.hsd_obj=&roots[i]};
        fighters[i].kind=i?Ft_Kind_CustomFirst:Ft_Kind_Link;
        fighters[i].x8_spawnNum=16777217;
        alternate_data[i].kind=i?Ft_Kind_CustomFirst+1:Ft_Kind_Link;
        alternate_data[i].x8_spawnNum=(int32_t)UINT32_C(0x80000001);
        alternate_data[i].u.lk.x14=&objects[i];
        memset(alternate_data[i].native_module_state,0xA5,12);
        int recipient_handle=123;
        memcpy(alternate_data[i].native_module_state+recipient_handle_offset,&recipient_handle,4);
        int sentinel=16777219;
        memcpy(alternate_data[i].native_module_state+field_offset(recipient_definition,"unrelated"),&sentinel,4);
        fighters[i].motion_id=motion;fighters[i].x34_scale.y=1.125f;
        fighters[i].x2070.x2071_b6=detaches;
        fighters[i].u.lk.x14=has_reference?&objects[i]:NULL;
        int handle=has_reference?123:-1;
        memcpy(fighters[i].native_module_state+author_handle_offset,&handle,4);
        items[i].x1C=UINT32_C(0xFEDCBA98);
        items[i].kind=kind;items[i].msid=state;items[i].x5CC_currentAnimFrame=frame;
        items[i].owner=owner_case==2?&alternate[i]:owner_case==3?NULL:&owners[i];
        roots[i].scale=(Vec3){2,3,4};roots[i].flags=JOBJ_MTX_INDEP_SRT;
    }
    items[0].xDD4_itemVar.linkbow.x0=1.125f;
    items[0].xDD4_itemVar.linkbow.x4=owner_case==1?NULL:&owners[0];
    owners[1].next=&alternate[1]; alternate[1].next=NULL;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&owners[1];
    heads[HSD_GOBJ_PLINK_ITEM]=&objects[1];objects[1].next=NULL;
    items[1].entity=&objects[1];
    items[1].xDD4_itemVar.native_article.creator_spawn_id=owner_case==1?0:16777217;
    items[1].xDD4_itemVar.native_article.attachment_owner=owner_case==1?NULL:&owners[1];
    float creator_scale=1.125f;
    memcpy(items[1].xDD4_itemVar.native_article.state,&creator_scale,4);
}
static void compare(const Trace* stock, const Trace* script, int a, int b,
                    const char* phase)
{
    ++comparisons;
    bool equal=a==b && stock->count==script->count &&
        memcmp(stock->events,script->events,stock->count*sizeof(BowEvent))==0 &&
        items[0].msid==items[1].msid &&
        items[0].x5CC_currentAnimFrame==items[1].x5CC_currentAnimFrame &&
        memcmp(&roots[0].scale,&roots[1].scale,sizeof(Vec3))==0 &&
        roots[0].flags==roots[1].flags &&
        (fighters[0].u.lk.x14!=NULL)==(owner_handle(1)>=0) &&
        (alternate_data[0].u.lk.x14!=NULL)==(recipient_handle()>=0);
    int sentinel;
    memcpy(&sentinel,alternate_data[1].native_module_state+field_offset(recipient_definition,"unrelated"),4);
    equal &= sentinel==16777219;
    for(unsigned i=0;i<12;++i)equal &= alternate_data[1].native_module_state[i]==0xA5;
    if(!equal) {
        ++failures;
        if(failures>10)return;
        fprintf(stderr,"Bow parity failed %s kind=%d state=%d result=%d/%d trace=%u/%u\n",
            phase,items[0].kind,items[0].msid,a,b,stock->count,script->count);
        fprintf(stderr,"  owner motion=%d/%d handle=%d/%d frame=%g/%g scale=%g/%g flags=%x/%x\n",
            fighters[0].motion_id,fighters[1].motion_id,fighters[0].u.lk.x14!=NULL,
            owner_handle(1),items[0].x5CC_currentAnimFrame,items[1].x5CC_currentAnimFrame,
            roots[0].scale.x,roots[1].scale.x,(unsigned)roots[0].flags,(unsigned)roots[1].flags);
    }
}
static void predicate(HSD_GObjPredicate stock, HSD_GObjPredicate script,
                       const char* phase)
{ Trace a={0},b={0};trace=&a;int x=stock(&objects[0]);trace=&b;int y=script(&objects[1]);compare(&a,&b,x,y,phase); }
static void callback(HSD_GObjEvent stock, HSD_GObjEvent script,const char* phase)
{ Trace a={0},b={0};trace=&a;stock(&objects[0]);trace=&b;script(&objects[1]);compare(&a,&b,0,0,phase); }

int main(int argc,char** argv)
{
    if(argc!=3) return 2;
    HMODULE library=LoadLibraryA(argv[1]);if(library==NULL)return 2;
    ModuleEntry entry=(ModuleEntry)GetProcAddress(library,"FreakMelee_GetFighterModule");
    if(entry==NULL)return 2;
    HMODULE recipient_library=LoadLibraryA(argv[2]);if(!recipient_library)return 2;
    ModuleEntry recipient_entry=(ModuleEntry)GetProcAddress(recipient_library,"FreakMelee_GetFighterModule");
    if(!recipient_entry)return 2;
    MotionState motions[1]={0};
    NativeFighterDefinition base={.source_kind=Ft_Kind_Link,.motion_state_count=1,.motion_states=motions};
    NativeFighterHost host={.callback_source_alive=nativeCallbackSource_Alive,
        .target_call=nativeFighterEvent_Call,.item_current_owner=current_owner,.item_creator=fixture_creator,.fighter_model_scale=ftLib_800869D4,
        .item_set_uniform_scale=scale,.item_interact=it_8026B894,
        .item_advance_animation=Item_802694CC,.item_set_animation_frame=Item_80268DD4,
        .item_change_state_flags=Item_80268E5C};
    const NativeFighterDefinition* module=entry(&base,&host);if(module==NULL)return 2;
    author_definition=module;
    NativeFighterDefinition recipient_base=base;recipient_base.state_bytes=11;
    recipient_definition=recipient_entry(&recipient_base,&host);if(!recipient_definition)return 2;
    if(!nativeFighterEvents_Validate(module)||!nativeFighterEvents_Validate(recipient_definition))return 2;
    author_handle_offset=field_offset(module,"bow_handle");
    recipient_handle_offset=field_offset(recipient_definition,"bow_handle");
    if(author_handle_offset!=4||recipient_handle_offset!=20)return 2;
    NativeFighterHost missing=host;missing.item_set_animation_frame=NULL;
    if(entry(&base,&missing)!=NULL)return 1;
    module=entry(&base,&host);if(module==NULL)return 2;
    const float frames[]={0,1,23.999f,24,25};
    for(unsigned family=0;family<2;++family) {
        ItemKind kind=family?It_Kind_CLink_Bow:It_Kind_Link_Bow;authored=NULL;
        for(unsigned i=0;i<module->article_logic_count;++i)
            if(module->article_logics[i].kind==kind)authored=&module->article_logics[i];
        if(authored==NULL||authored->state_count!=7)return 2;
        for(unsigned state=0;state<7;++state) {
            for(int motion=343;motion<=350;++motion)
            for(unsigned reference=0;reference<2;++reference)
            for(unsigned ownership=0;ownership<4;++ownership)
            for(unsigned detach=0;detach<2;++detach)
            for(unsigned frame=0;frame<5;++frame) {
                reset(kind,state,motion,reference,ownership,detach,frames[frame]);
                if((ownership==1||ownership==2)&&(state==2||state==5)&&
                   (frames[frame]==0||frames[frame]>=24)) ++restored_cleanup_cases;
                predicate(it_803F6E98[state].animated,
                    authored->logic->states[state].animated,"animation");
            }
            reset(kind,state,344,true,0,false,7);
            predicate(it_803F6E98[state].collided,
                authored->logic->states[state].collided,"collision");
            reset(kind,state,344,true,0,false,7);
            callback(it_803F6E98[state].physics_updated,
                authored->logic->states[state].physics_updated,"physics");
        }
        for(int motion=343;motion<=350;++motion)
        for(unsigned reference=0;reference<2;++reference)
        for(unsigned ownership=0;ownership<4;++ownership) {
            reset(kind,0,motion,reference,ownership,false,7);
            callback(itLinkBow_Logic100_PickedUp,authored->logic->picked_up,"pickup");
        }
        for(unsigned ownership=0;ownership<4;++ownership)
        for(unsigned reference=0;reference<2;++reference) {
            reset(kind,0,344,reference,ownership,false,7);
            callback(itLinkBow_Logic100_Destroyed,authored->logic->destroyed,"current-recipient destruction");
        }
        // An unrelated recipient has no authored Bow export. It is never
        // mutated through a stock union cast, but the expired article still ends.
        for(unsigned state=2;state<=5;state+=3) {
            reset(kind,state,344,true,2,false,24);
            alternate_data[1].kind=Ft_Kind_CustomFirst+2;
            uint8_t before[sizeof(alternate_data[1].native_module_state)];
            memcpy(before,alternate_data[1].native_module_state,sizeof(before));
            Trace ignored={0};trace=&ignored;
            bool terminal=authored->logic->states[state].animated(&objects[1]);
            bool cleanup_supported=true;
            for(unsigned i=0;i<authored->state_field_count;++i)
                if(!strcmp(authored->state_fields[i].name,"cleanup_supported"))
                    memcpy(&cleanup_supported,items[1].xDD4_itemVar.native_article.state+
                        authored->state_fields[i].offset,sizeof(cleanup_supported));
            ++missing_handler_cases;
            if(!terminal || cleanup_supported ||
               memcmp(before,alternate_data[1].native_module_state,sizeof(before)))++failures;
        }
    }
    printf("Bow direct callbacks: %u comparisons, %u failures; %u restored cleanup cases; %u missing-export terminal policy cases\n",
        comparisons,failures,restored_cleanup_cases,missing_handler_cases);
    FreeLibrary(recipient_library);FreeLibrary(library);return failures?1:0;
}
