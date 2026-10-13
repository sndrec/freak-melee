// Actual generated DLL entry and unchanged original lifecycle bodies. Stock
// callback sentinels are never delegates: any accidental invocation fails.
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, phases, callbacks, article_callbacks, lifecycle_pairs;
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c); exit(1); } } while (0)
static void unexpected(void) { CHECK(false); }
static void stock_slot(HSD_GObj* g) { (void)g; CHECK(false); }
#include "motion.inc"
#include "metadata.inc"
#include "parameters.inc"

typedef struct Observation { int operation, a, b, c; } Observation;
static Observation events[32];
static unsigned event_count;
static Fighter_GObj* live_object;
static Fighter* live_fighter;
static const NativeFighterDefinition* definition;
static HMODULE library;
static void event(int operation,int a,int b,int c)
{
    CHECK(event_count<32); events[event_count++]=(Observation){operation,a,b,c};
}
static void copy(Fighter_GObj* g) { CHECK(g==live_object); event(1,0,0,0); }
static void reload(Fighter_GObj* g) { CHECK(g==live_object); event(2,0,0,0); }
void ftParts_80074A4C(Fighter_GObj* g,int a,int b) { CHECK(g==live_object); event(3,a,b,0); }
void Fighter_OnItemPickup(Fighter_GObj* g,bool a,bool b,bool c) { CHECK(g==live_object); event(4,a,b,c); }
void Fighter_OnItemDrop(Fighter_GObj* g,bool a,bool b,bool c) { CHECK(g==live_object); event(5,a,b,c); }
void Fighter_OnItemVisible(Fighter_GObj* g,bool a) { CHECK(g==live_object); event(6,a,0,0); }
void Fighter_OnItemInvisible(Fighter_GObj* g,bool a) { CHECK(g==live_object); event(7,a,0,0); }
void Fighter_OnKnockbackEnter(Fighter_GObj* g,s32 a) { CHECK(g==live_object); event(8,a,0,0); }
void Fighter_OnKnockbackExit(Fighter_GObj* g,s32 a) { CHECK(g==live_object); event(9,a,0,0); }
bool ftCo_800BFFD0(Fighter* f,int a,bool b) { CHECK(f==live_fighter); event(10,a,b,0); return true; }
void it_8026B3F8(void* data,ItemKind kind) { event(11,kind,(int)(uintptr_t)data,0); }
#define GET_FIGHTER(g) ((Fighter*)(g)->user_data)
#define PUSH_ATTRS(f,T) copy((f)->gobj)
#define COPY_ATTRS(g,T) reload(g)
#include "original.inc"
#include "original_calls.inc"

static bool alive(HSD_GObj* g,uint32_t generation,uint32_t kind)
{
    CHECK(g==live_object && generation==live_fighter->x8_spawnNum && kind==NativeCallbackSource_Fighter);
    return true;
}
static void own(const void* pointer)
{
    if (!pointer) return;
    MEMORY_BASIC_INFORMATION m;
    CHECK(VirtualQuery(pointer,&m,sizeof(m))==sizeof(m));
    CHECK(m.AllocationBase==library);
}
static int32_t state(const char* name,int32_t value,bool write)
{
    for (unsigned i=0;i<definition->state_field_count;++i) {
        const NativeFighterStateField* f=definition->state_fields+i;
        if (strcmp(name,f->name)) continue;
        CHECK(f->type==NativeFighterState_Int && f->count==1);
        if (write) memcpy(live_fighter->native_module_state+f->offset,&value,4);
        else memcpy(&value,live_fighter->native_module_state+f->offset,4);
        return value;
    }
    CHECK(false); return 0;
}
static void observations(HSD_GObjEvent original,HSD_GObjEvent authored)
{
    event_count=0; original(live_object);
    Observation expected[32]; unsigned n=event_count;
    memcpy(expected,events,n*sizeof(*events)); event_count=0;
    authored(live_object);
    if(event_count!=n || memcmp(events,expected,n*sizeof(*events))) {
        fprintf(stderr,"Lifecycle pair %u counts %u/%u expected op %d actual %d\n",lifecycle_pairs,n,event_count,n?expected[0].operation:-1,event_count?events[0].operation:-1);
    }
    CHECK(event_count==n && !memcmp(events,expected,n*sizeof(*events)));
    ++lifecycle_pairs;
}
static void item_observations(Fighter_ItemEvent original,Fighter_ItemEvent authored,bool flag)
{
    event_count=0; original(live_object,flag);
    Observation expected[32]; unsigned n=event_count;
    memcpy(expected,events,n*sizeof(*events)); event_count=0;
    authored(live_object,flag);
    CHECK(event_count==n && !memcmp(events,expected,n*sizeof(*events)));
    ++lifecycle_pairs;
}
#define ARTICLE_POINTER(p) do { if(p) { own((const void*)(p)); ++article_callbacks; } } while(0)
int main(int argc,char** argv)
{
    CHECK(argc==2); library=LoadLibraryA(argv[1]); CHECK(library);
    NativeFighterHost host={0};
#include "required_host.inc"
    host.copy_ext_attributes=copy; host.reload_ext_attributes=reload;
    host.set_visibility_group=ftParts_80074A4C;
    host.fighter_item_pickup=Fighter_OnItemPickup; host.fighter_item_drop=Fighter_OnItemDrop;
    host.fighter_item_visible=Fighter_OnItemVisible; host.fighter_item_invisible=Fighter_OnItemInvisible;
    host.fighter_knockback_enter=Fighter_OnKnockbackEnter; host.fighter_knockback_exit=Fighter_OnKnockbackExit;
    host.start_color=ftCo_800BFFD0; host.callback_source_alive=alive;
    NativeFighterDefinition base={0};
    base.code_key="owned_pair_probe"; base.source_kind=MEMBER_KIND; base.animation_source_kind=MEMBER_KIND;
#if SHEIK
    base.fighter_root="ftDataSeak";
#else
    base.fighter_root="ftDataZelda";
#endif
    base.motion_states=stock_states; base.motion_state_count=sizeof(stock_states)/sizeof(*stock_states);
    base.motion_state_first=341; base.action_count=ACTION_COUNT; base.demo_count=14;
    base.bones=BASE_BONES; base.bone_count=sizeof(BASE_BONES)/sizeof(*BASE_BONES); base.joint_count=PART_COUNT;
    base.article_assets=BASE_ARTICLES; base.article_asset_count=ARTICLE_COUNT; base.demo_motion_names=&BASE_DEMOS;
    base.sound_banks=UINT64_C(1)<<27; base.effect_bank=17;
    base.asset_layout.article_mask=(1<<ARTICLE_COUNT)-1;
#if SHEIK
    base.asset_layout.joint_graph_mask=0x30; base.asset_layout.item_format=FtCharacterItems_Chain;
    base.pose_graphs=BASE_POSES; base.pose_graph_count=2;
    int ground[]={341,347,355,361},air[]={344,348,358,363};
#else
    int ground[]={341,343,349,355},air[]={342,346,352,357};
#endif
    for(unsigned i=0;i<4;++i) { base.special_ground_state[i]=ground[i]; base.special_air_state[i]=air[i]; }
#define VISIT_CALLBACK(field) base.field=(__typeof__(base.field))stock_slot;
#include "callbacks.inc"
#undef VISIT_CALLBACK
    base.items=(const NativeItemDefinition*)(uintptr_t)1; base.item_count=1;
    base.article_logics=(const NativeFighterArticleLogic*)(uintptr_t)1; base.article_logic_count=1;
    typedef const NativeFighterDefinition* (*Entry)(const NativeFighterDefinition*,const NativeFighterHost*);
    Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule"); CHECK(entry);
    typedef int (*Parameters)(const uint32_t*,unsigned,uint64_t);
    Parameters bind=(Parameters)GetProcAddress(library,"FreakMelee_SetFighterParameters"); CHECK(bind);
    CHECK(bind(values,sizeof(values)/sizeof(*values),SCHEMA));
    definition=entry(&base,&host); CHECK(definition);
    CHECK(definition->source_kind==Ft_Kind_None && base.source_kind==MEMBER_KIND);
    CHECK(definition->animation_source_kind==MEMBER_KIND && definition->bones==base.bones && definition->bone_count==sizeof(BASE_BONES)/sizeof(*BASE_BONES));
    CHECK(definition->joint_count==PART_COUNT && definition->action_count==ACTION_COUNT && definition->demo_count==14);
    CHECK(definition->article_assets==base.article_assets && definition->article_asset_count==ARTICLE_COUNT);
    CHECK(!memcmp(&definition->asset_layout,&base.asset_layout,sizeof(base.asset_layout)));
    CHECK(definition->demo_motion_names==base.demo_motion_names && definition->sound_banks==base.sound_banks);
    CHECK(definition->pose_graph_count==base.pose_graph_count);
    for(unsigned i=0;i<base.pose_graph_count;++i) {
        const NativeFighterPoseGraph* a=base.pose_graphs+i; const NativeFighterPoseGraph* b=definition->pose_graphs+i;
        CHECK(a->name_hash==b->name_hash && a->slot==b->slot && !strcmp(a->path,b->path));
    }
    CHECK(!definition->items && !definition->item_count && !definition->neutral_article);
    for(unsigned i=0;i<definition->callback_export_count;++i) own((const void*)definition->callback_exports[i].callback);
#define VISIT_CALLBACK(field) do { if(definition->field) { own((const void*)definition->field); ++callbacks; } } while(0)
#include "callbacks.inc"
#undef VISIT_CALLBACK
    for(unsigned i=0;i<base.motion_state_count;++i) {
        const MotionState* a=base.motion_states+i; const MotionState* b=definition->motion_states+i;
        CHECK(a->anim_id==b->anim_id && a->x4_flags==b->x4_flags && a->_==b->_ && a->cam_cb==b->cam_cb);
        HSD_GObjEvent old[]={a->anim_cb,a->input_cb,a->phys_cb,a->coll_cb};
        HSD_GObjEvent now[]={b->anim_cb,b->input_cb,b->phys_cb,b->coll_cb};
        for(unsigned j=0;j<4;++j) { if(old[j]) { CHECK(now[j]); ++phases; } own((void*)now[j]); }
    }
    CHECK(definition->article_logic_count==ARTICLE_COUNT);
    for(unsigned i=0;i<ARTICLE_COUNT;++i) {
        const NativeFighterArticleLogic* a=definition->article_logics+i; own(a->logic); own(a->logic->states);
        bool mapped=false; for(unsigned j=0;j<ARTICLE_COUNT;++j) if(a->kind==BASE_ARTICLES[j].kind) mapped=true;
        CHECK(mapped); ARTICLE_POINTER(a->on_created); ARTICLE_POINTER(a->on_out_of_bounds);
        ItemLogicTable* l=a->logic;
        ARTICLE_POINTER(l->spawned); ARTICLE_POINTER(l->destroyed); ARTICLE_POINTER(l->picked_up);
        ARTICLE_POINTER(l->dropped); ARTICLE_POINTER(l->thrown); ARTICLE_POINTER(l->dmg_dealt);
        ARTICLE_POINTER(l->dmg_received); ARTICLE_POINTER(l->entered_air); ARTICLE_POINTER(l->reflected);
        ARTICLE_POINTER(l->clanked); ARTICLE_POINTER(l->absorbed); ARTICLE_POINTER(l->shield_bounced);
        ARTICLE_POINTER(l->hit_shield); ARTICLE_POINTER(l->evt_unk);
        for(unsigned j=0;j<a->state_count;++j) { ARTICLE_POINTER(l->states[j].animated); ARTICLE_POINTER(l->states[j].physics_updated); ARTICLE_POINTER(l->states[j].collided); }
        for(unsigned j=0;j<a->signal_count;++j) ARTICLE_POINTER(a->signals[j].callback);
    }
    Fighter f={0}; HSD_GObj g={0}; ftData data={0}; void* items[4]={(void*)1,(void*)2,(void*)3,(void*)4};
    g.user_data=&f; f.gobj=&g; f.ft_data=&data; data.x48_items=items; f.x8_spawnNum=UINT32_C(0x81000001); f.x5AC.model_num=2;
    live_object=&g; live_fighter=&f;
    event_count=0; OriginalOnLoad(&g); CHECK(event_count==ARTICLE_COUNT+1);
    for(unsigned i=0;i<ARTICLE_COUNT;++i) CHECK(events[i+1].operation==11 && events[i+1].a==BASE_ARTICLES[i].kind && events[i+1].b==(int)i+1);
    event_count=0; f.can_walljump=false; definition->on_load(&g); CHECK(event_count==1 && events[0].operation==1);
    CHECK(f.can_walljump==SHEIK); ++lifecycle_pairs;
#if SHEIK
    CHECK(state("needle_count",0,false)==0 && state("needle_held",0,false)==-1 && state("chain_handle",0,false)==-1);
    state("needle_count",5,true); state("needle_held",71,true); state("chain_handle",72,true);
    f.u.sk.x0=5; f.u.sk.x4=(Item_GObj*)(uintptr_t)71; f.u.sk.x8=(HSD_GObj*)(uintptr_t)72;
#else
    CHECK(state("din_handle",0,false)==-1); state("din_handle",71,true); f.u.zd.x222C=(HSD_GObj*)(uintptr_t)71;
#endif
    observations(OriginalOnDeath,definition->on_death);
#if SHEIK
    CHECK(!f.u.sk.x0 && !f.u.sk.x4 && !f.u.sk.x8);
    CHECK(!state("needle_count",0,false) && state("needle_held",0,false)==-1 && state("chain_handle",0,false)==-1);
    for(int n=0;n<8;++n) { f.u.sk.x0=n; state("needle_count",n,true); observations(OriginalUnkMotionStates4,definition->on_color_animation_reset); }
#else
    CHECK(!f.u.zd.x222C && state("din_handle",0,false)==-1);
#endif
    observations(OriginalLoadSpecialAttrs,definition->on_load_special_attrs);
    observations(OriginalOnItemVisible,definition->on_item_visible); observations(OriginalOnItemInvisible,definition->on_item_invisible);
    observations(OriginalOnKnockbackEnter,definition->on_knockback_enter); observations(OriginalOnKnockbackExit,definition->on_knockback_exit);
    for(unsigned flag=0;flag<2;++flag) {
        item_observations(OriginalOnItemPickup,definition->on_item_pickup,flag); item_observations(OriginalOnItemPickup,definition->on_item_pickup_ext,flag);
        item_observations(OriginalOnItemDrop,definition->on_item_drop,flag); item_observations(OriginalOnItemDrop,definition->on_item_drop_ext,flag);
    }
    CHECK(definition->on_form_activated && definition->on_cpu_pending_action);
#if !SHEIK
    CHECK(definition->on_cpu_attack_decision && definition->on_cpu_travel_decision && definition->on_cpu_recovery);
#endif
    FreeLibrary(library);
    printf("{\"checks\":%u,\"phases\":%u,\"definition_callbacks\":%u,\"article_callbacks\":%u,\"lifecycle_pairs\":%u}\n",checks,phases,callbacks,article_callbacks,lifecycle_pairs);
    return 0;
}
