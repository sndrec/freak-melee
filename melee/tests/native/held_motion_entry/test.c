#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_fighter_held_motion.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/gobjobject.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, original_pairs, authored_cases, safety_cases;
#define CHECK(value) do { ++checks; assert(value); } while (0)
HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static Fighter fighter;
static HSD_GObj object, saved_item;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
static NativeFighterDefinition definition, replacement;
static const NativeFighterDefinition* current_definition;
static unsigned query_calls, query_depth, invalidate_stage, invalidate_mode;
static int32_t policy_result;
static bool use_owned_mapping;
static unsigned entry_kind;
static int initial_motion;
static int old_timer;
static unsigned initial_flags;

typedef struct EntryEvent {
    unsigned kind;
    int current, previous, selected;
    uint32_t flags;
    float start, speed, blend;
    unsigned common_flags;
    int timer;
} EntryEvent;
static EntryEvent events[3];
static unsigned event_count;
static unsigned common_flags(void)
{
    return fighter.x2221_b4 | (fighter.x2221_b6 << 1) | (fighter.x2221_b7 << 2);
}
static void invalidate(unsigned stage)
{
    if (invalidate_stage != stage) return;
    switch (invalidate_mode) {
    case 1: heads[HSD_GOBJ_PLINK_FIGHTER] = NULL; break;
    case 2: ++fighter.x8_spawnNum; break;
    case 3: fighter.kind = Ft_Kind_CustomFirst + 1; break;
    case 4: current_definition = &replacement; break;
    }
}
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    (void)kind; return current_definition; // Controlled immutable runtime catalog boundary.
}
static int32_t select_owned(Fighter_GObj* source, int32_t canonical, int32_t previous)
{
    CHECK(source == &object); CHECK(previous == initial_motion);
    CHECK(canonical == (entry_kind == 2 ? ftCo_MS_ItemParasolFallSpecial : ftCo_MS_ItemParasolOpen));
    CHECK(fighter.motion_id == initial_motion);
    if (entry_kind != 2) CHECK(fighter.mv.co.parasol_open.prev_msid == initial_motion);
    ++query_calls;
    if (query_depth) {
        --query_depth;
        CHECK(nativeFighterHeldMotion_Select(source, canonical, previous) == -1);
    }
    invalidate(1);
    if (!use_owned_mapping) return policy_result;
    return fighter.native_module_state[0] ? (entry_kind == 2 ? 370 : 369) : canonical;
}
void Fighter_ChangeMotionState(Fighter_GObj* source, FtMotionId selected,
    MotionFlags flags, f32 start, f32 speed, f32 blend, Fighter_GObj* donor)
{
    CHECK(source == &object); CHECK(donor == NULL); CHECK(event_count < 3);
    events[event_count++] = (EntryEvent){1,fighter.motion_id,
        fighter.mv.co.parasol_open.prev_msid,selected,flags,start,speed,blend,common_flags(),fighter.x2104};
    // Controlled transition boundary: publish ID and the relevant known flag effect.
    fighter.motion_id = selected;
    if (!(flags & Ft_MF_SkipParasol)) fighter.x2221_b4 = false;
    invalidate(2);
}
void ftAnim_8006EBA4(Fighter_GObj* source)
{
    CHECK(source == &object); CHECK(event_count < 3);
    events[event_count++] = (EntryEvent){2,fighter.motion_id,
        fighter.mv.co.parasol_open.prev_msid,0,0,0,0,0,common_flags(),fighter.x2104};
    invalidate(3);
}
#include "original53.inc"
#include "candidate.inc"

typedef struct EntryResult {
    int motion, previous, timer;
    unsigned flags, count;
    EntryEvent events[3];
} EntryResult;
static void reset(FighterKind kind, bool present, unsigned flags, int previous, int timer, float reset_timer)
{
    memset(&fighter,0,sizeof(fighter)); memset(&object,0,sizeof(object)); memset(heads,0,sizeof(heads));
    object.user_data=&fighter; object.classifier=HSD_GOBJ_CLASS_FIGHTER;
    fighter.kind=kind; fighter.x8_spawnNum=UINT32_C(0xFEDCBA98);
    fighter.motion_id=previous; fighter.mv.co.parasol_open.prev_msid=-777;
    fighter.u.pe.parasol_gobj_0=present ? &saved_item : NULL;
    fighter.x2221_b4=flags&1; fighter.x2221_b6=(flags>>1)&1; fighter.x2221_b7=(flags>>2)&1;
    fighter.x2104=timer; common.x59C=reset_timer;
    heads[HSD_GOBJ_PLINK_FIGHTER]=&object; HSD_GObjPLinkHead=heads;
    current_definition=NULL; query_calls=query_depth=0; event_count=0;
    invalidate_stage=invalidate_mode=0; use_owned_mapping=false;
    initial_motion=previous; old_timer=timer; initial_flags=flags; fighter.native_module_state[0]=present;
}
static EntryResult result(void)
{
    EntryResult r={0};r.motion=fighter.motion_id;r.previous=fighter.mv.co.parasol_open.prev_msid;
    r.timer=fighter.x2104;r.flags=common_flags();r.count=event_count;
    memcpy(r.events,events,event_count*sizeof(*events));return r;
}
static void enter_original(void)
{
    if(entry_kind==0) original53_ft_800CEF08(&object);
    else if(entry_kind==1) original53_ftCo_800CEFE0(&object,9876); // Deliberately ignored source argument.
    else original53_ftCo_800CF3C8(&object);
}
static void enter_candidate(void)
{
    if(entry_kind==2) ftCo_800CF3C8(&object);
    else ftCo_ItemParasolOpen_Enter(&object,entry_kind==0 ? 0 : 10);
}
static void expected_entry(int selected)
{
    CHECK(event_count==(entry_kind==2 ? 1u : 2u)); CHECK(events[0].kind==1);
    CHECK(events[0].current==initial_motion); CHECK(events[0].selected==selected);
    CHECK(events[0].start==0 && events[0].speed==1);
    CHECK(events[0].blend==(entry_kind==1 ? 10 : 0));
    CHECK(events[0].flags==(entry_kind==2 ? (Ft_MF_SkipHit|Ft_MF_SkipParasol) : 0));
    CHECK(events[0].timer==old_timer); CHECK(events[0].common_flags==initial_flags);
    CHECK(fighter.motion_id==selected);
    if(entry_kind==2) {
        CHECK(fighter.mv.co.parasol_open.prev_msid==-777);
        CHECK(fighter.x2104==old_timer);CHECK(common_flags()==initial_flags);
    } else {
        CHECK(fighter.mv.co.parasol_open.prev_msid==initial_motion);
        CHECK(events[0].previous==initial_motion && events[1].previous==initial_motion);
        CHECK(events[1].current==selected && events[1].timer==old_timer);
        CHECK(fighter.x2221_b4 && !fighter.x2221_b6 && !fighter.x2221_b7);
        CHECK(fighter.x2104==((initial_flags&2) ? (int)common.x59C : old_timer));
    }
}
static void healthy_matrix(void)
{
    const FighterKind kinds[]={Ft_Kind_Mario,Ft_Kind_Peach,Ft_Kind_CustomFirst};
    const int previous[]={ftCo_MS_Wait,ftCo_MS_FallSpecial,341,369,370};
    const int timers[]={-1,0,13};const float resets[]={-2.75f,0,37.75f};
    for(entry_kind=0;entry_kind<3;++entry_kind)
        for(unsigned k=0;k<3;++k) for(unsigned present=0;present<2;++present)
            for(unsigned flags=0;flags<8;++flags) for(unsigned p=0;p<5;++p)
                for(unsigned t=0;t<3;++t) for(unsigned r=0;r<3;++r) {
                    reset(kinds[k],present,flags,previous[p],timers[t],resets[r]);enter_original();EntryResult old=result();
                    reset(kinds[k],present,flags,previous[p],timers[t],resets[r]);enter_candidate();EntryResult actual=result();
                    CHECK(old.motion==actual.motion && old.previous==actual.previous && old.timer==actual.timer);
                    CHECK(old.flags==actual.flags && old.count==actual.count);
                    for(unsigned e=0;e<old.count;++e) {
                        CHECK(old.events[e].kind==actual.events[e].kind);
                        CHECK(old.events[e].current==actual.events[e].current && old.events[e].previous==actual.events[e].previous);
                        CHECK(old.events[e].selected==actual.events[e].selected && old.events[e].flags==actual.events[e].flags);
                        CHECK(!memcmp(&old.events[e].start,&actual.events[e].start,sizeof(float)));
                        CHECK(!memcmp(&old.events[e].speed,&actual.events[e].speed,sizeof(float)));
                        CHECK(!memcmp(&old.events[e].blend,&actual.events[e].blend,sizeof(float)));
                        CHECK(old.events[e].timer==actual.events[e].timer && old.events[e].common_flags==actual.events[e].common_flags);
                    }
                    ++original_pairs;
                    int canonical=entry_kind==2 ? ftCo_MS_ItemParasolFallSpecial : ftCo_MS_ItemParasolOpen;
                    expected_entry(kinds[k]==Ft_Kind_Peach && present ? (entry_kind==2 ? 370 : 369) : canonical);
                    CHECK(query_calls==0);
                    // Authored policy reads owned state rather than stock kind/private pointer.
                    if(k==2) {
                        reset(kinds[k],present,flags,previous[p],timers[t],resets[r]);
                        current_definition=&definition;definition.on_held_motion=select_owned;use_owned_mapping=true;
                        enter_candidate();expected_entry(present ? (entry_kind==2 ? 370 : 369) : canonical);
                        CHECK(query_calls==1);++authored_cases;
                    }
                }
}
static void partial_definition_matrix(void)
{
    for(entry_kind=0;entry_kind<3;++entry_kind)
        for(unsigned raw=0;raw<2;++raw) for(unsigned present=0;present<2;++present) {
            FighterKind kind=raw ? Ft_Kind_Peach : Ft_Kind_CustomFirst;
            reset(kind,present,7,ftCo_MS_FallSpecial,13,37.75f);
            enter_original();EntryResult old=result();
            reset(kind,present,7,ftCo_MS_FallSpecial,13,37.75f);
            current_definition=&definition;definition.on_held_motion=NULL;
            enter_candidate();EntryResult actual=result();
            CHECK(old.motion==actual.motion && old.previous==actual.previous && old.timer==actual.timer);
            CHECK(old.flags==actual.flags && old.count==actual.count);
            int canonical=entry_kind==2 ? ftCo_MS_ItemParasolFallSpecial : ftCo_MS_ItemParasolOpen;
            expected_entry(raw && present ? (entry_kind==2 ? 370 : 369) : canonical);
            CHECK(query_calls==0);++original_pairs;
        }
}
static void safety_matrix(void)
{
    const int32_t ids[]={INT32_MIN,-2,-1,0,ftCo_MS_Count-1,341,369,370,371,INT32_MAX};
    for(entry_kind=0;entry_kind<3;++entry_kind) {
        int canonical=entry_kind==2 ? ftCo_MS_ItemParasolFallSpecial : ftCo_MS_ItemParasolOpen;
        for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i) {
            reset(Ft_Kind_CustomFirst,true,7,ftCo_MS_FallSpecial,13,37.75f);
            current_definition=&definition;definition.on_held_motion=select_owned;policy_result=ids[i];
            enter_candidate();int selected=ids[i]>=0 && ids[i]<371 ? ids[i] : canonical;
            expected_entry(selected);CHECK(query_calls==1);++safety_cases;
        }
        for(unsigned stage=1;stage<=3;++stage) for(unsigned mode=1;mode<=4;++mode) {
            if(entry_kind==2 && stage==3) continue;
            reset(Ft_Kind_CustomFirst,true,7,ftCo_MS_FallSpecial,13,37.75f);
            current_definition=&definition;definition.on_held_motion=select_owned;policy_result=369;
            invalidate_stage=stage;invalidate_mode=mode;enter_candidate();
            CHECK(event_count==(stage==1 ? 0u : stage==2 ? 1u : 2u));
            CHECK(fighter.x2104==13); CHECK(fighter.x2221_b6 && fighter.x2221_b7);
            CHECK(query_calls==1);++safety_cases;
        }
        reset(Ft_Kind_CustomFirst,true,7,ftCo_MS_FallSpecial,13,37.75f);
        current_definition=&definition;definition.on_held_motion=select_owned;policy_result=369;
        for(unsigned i=0;i<64;++i) CHECK(nativeCallbackDispatch_Enter());
        enter_candidate();CHECK(event_count==0 && query_calls==0);
        CHECK(!nativeCallbackDispatch_Enter()); // Rejected dispatch must not decrement existing depth.
        for(unsigned i=0;i<64;++i) nativeCallbackDispatch_Leave();++safety_cases;
        reset(Ft_Kind_CustomFirst,true,7,ftCo_MS_FallSpecial,13,37.75f);
        current_definition=&definition;definition.on_held_motion=select_owned;policy_result=369;
        for(unsigned i=0;i<63;++i) CHECK(nativeCallbackDispatch_Enter());
        query_depth=1;enter_candidate();CHECK(query_calls==1);
        expected_entry(369);
        CHECK(nativeCallbackDispatch_Enter()); CHECK(!nativeCallbackDispatch_Enter());
        for(unsigned i=0;i<64;++i) nativeCallbackDispatch_Leave();++safety_cases;
        for(unsigned null_data=0;null_data<2;++null_data) {
            reset(Ft_Kind_CustomFirst,true,7,ftCo_MS_FallSpecial,13,37.75f);
            if(null_data) object.user_data=NULL; else heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
            enter_candidate();CHECK(event_count==0 && query_calls==0);++safety_cases;
        }
    }
    // Real registry validates source before selector dereference. No retired-entry claim for original C.
    reset(Ft_Kind_CustomFirst,true,0,ftCo_MS_Wait,0,12);heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
    CHECK(nativeFighterHeldMotion_Select(&object,ftCo_MS_ItemParasolOpen,ftCo_MS_Wait)==-1);++safety_cases;
    reset(Ft_Kind_CustomFirst,true,0,ftCo_MS_Wait,0,12);object.user_data=NULL;
    CHECK(nativeFighterHeldMotion_Select(&object,ftCo_MS_ItemParasolOpen,ftCo_MS_Wait)==-1);++safety_cases;
}
int main(void)
{
    definition.motion_state_first=341;definition.motion_state_count=30;
    replacement=definition;
    healthy_matrix();partial_definition_matrix();safety_matrix();
    printf("Held motion entry PASS %u original pairs, %u authored cases, %u safety cases, %u checks\n",
        original_pairs,authored_cases,safety_cases,checks);return 0;
}
