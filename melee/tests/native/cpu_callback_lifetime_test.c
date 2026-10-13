// Actual CPU decision/classifier bodies; controlled package and engine boundaries.
#include <melee/ft/fighter.h>
#include <melee/ft/ftcpuattack.h>
#include <melee/ft/ftcmdscript.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/ft/kinds/ftCommon/ftCo_0A01.h>
#include <melee/ft/kinds/ftDonkey/forward.h>
#include <melee/ft/kinds/ftKirby/forward.h>
#include <melee/ft/kinds/ftNess/forward.h>
#include <melee/ft/kinds/ftPeach/forward.h>
#include <melee/ft/kinds/ftSamus/forward.h>
#include <melee/ft/kinds/ftYoshi/forward.h>
#include <melee/ft/kinds/ftZelda/forward.h>
#include <melee/it/inlines.h>
#include <melee/ef/forward.h>
#include <melee/gm/forward.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <windows.h>
#undef NDEBUG
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter *fighter, target;
static HSD_GObj* object;
static HSD_GObj* heads[32];
HSD_GObj** HSD_GObjPLinkHead = heads;
struct Fighter_804D64FC_t* Fighter_804D64FC;
static NativeFighterDefinition definition;
static void *fighter_page, *object_page;
static size_t fighter_size, object_size;
static unsigned checks, cases, callback_calls, continuation_calls, execute_calls;
static unsigned reset_calls, wait_calls, candidate_prepare_calls;
static bool result_bool, classifier_bool, weapon_attack, grounded_test;
static bool explicit_travel_advance;
static bool definition_enabled=true;
static bool pending_geometry=true;
static unsigned fallback_calls;
static unsigned pending_identity_mutation;
static int command_result, callback_kind, hazard; // 1 removed/protected, 2 recycled
static unsigned char replacement[sizeof(Fighter)];
static unsigned char replacement_object[sizeof(HSD_GObj)];
#define CHECK(x) do { ++checks; assert(x); } while(0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == Ft_Kind_CustomFirst && definition_enabled ? &definition : NULL; }
EF_Effect* nativeModelEffect_Resolve(HSD_GObj* obj,uint32_t id)
{ (void)obj; (void)id; abort(); }
bool nativeModelEffect_Snapshot(HSD_GObj* obj,uint32_t* id)
{ (void)obj; (void)id; abort(); }

static void continuation(Fighter* fp)
{
    CHECK(fp == fighter);
    CHECK(nativeCallbackSource_Alive(object, 77, NativeCallbackSource_Fighter));
    ++continuation_calls;
}
float HSD_Randf(void) { return 0.0f; }
float lb_8000D008(float y,float x) { return atan2f(y,x); }
bool ftCo_800A5A90(Fighter* fp) { continuation(fp); return weapon_attack; }
bool ftCo_800A2C08(Fighter* fp) { continuation(fp); return grounded_test; }
bool ftCo_800A1BA8(Fighter* fp) { continuation(fp); return false; }
float ftCo_800A1AB4(Fighter* fp,Fighter* other)
{ continuation(fp); CHECK(other == &target); return 50.0f; }
void ftCo_800A0C8C(Fighter* fp) { continuation(fp); ++reset_calls; }
void ftCo_800A0098(Fighter* fp) { continuation(fp); }
void ftCo_800B9704(Fighter* fp) { continuation(fp); ++candidate_prepare_calls; }
void ftCo_800B63D8(Fighter* fp) { continuation(fp); ++wait_calls; }
void ftCo_800B4880(Fighter* fp,int command)
{ continuation(fp); CHECK(command >= 1 && command <= 61); ++execute_calls; }
void ftCo_800B4A78(Fighter* fp) { continuation(fp); ++execute_calls; }
void ftCo_800B463C(Fighter* fp,cmd_t command)
{ continuation(fp); (void)command; }
void ftCo_800B46B8(Fighter* fp,cmd_t command,arg_t arg)
{ continuation(fp); (void)command; (void)arg; }
bool ftCo_800B8A9C(Fighter* fp) { continuation(fp); fp->cpu.xA4=17; return true; }
bool ftCo_800BA2E8(Fighter* fp,Fighter* other)
{ (void)fp; (void)other; abort(); }
bool ftCo_800B9A04(Fighter* fp,Item* item,float angle,float distance)
{ (void)fp; (void)item; (void)angle; (void)distance; abort(); }
bool ftCo_800B9790(Fighter* fp,float angle,float distance)
{ (void)fp; (void)angle; (void)distance; abort(); }
bool ftCo_800B98C8(Fighter* fp,float angle,float distance)
{ (void)fp; (void)angle; (void)distance; abort(); }
int ftCo_800B6208(struct ftCo_AttackEntry* entry) { (void)entry; abort(); }
void ftCo_800B920C(Fighter* fp) { (void)fp; abort(); }
void ftCo_800B92D4(Fighter* fp) { (void)fp; abort(); }
void ftCo_800B9340(Fighter* fp) { (void)fp; abort(); }
void ftCo_800B9504(Fighter* fp) { (void)fp; abort(); }
void ftCo_800A05F4(Fighter* fp) { continuation(fp);++fallback_calls; }
static Fighter* ftCo_800A5CE0(Fighter* fp) { continuation(fp);return NULL; }
static void ftCo_800A08F0(Fighter* fp) { continuation(fp);++fallback_calls; }
static void ftCo_800A9CB4(Fighter* fp) { continuation(fp);++fallback_calls; }
static void ftCo_800AB224(Fighter* fp) { continuation(fp);++fallback_calls; }
static bool ftCo_800A3554(Fighter* fp,float scale)
{ continuation(fp);CHECK(scale==1);return pending_geometry; }
void ftCo_800BA674(Fighter* fp,Fighter* other) { continuation(fp);CHECK(other==&target);++fallback_calls; }
static void ftCo_CpuReturnToPreviousBehavior(Fighter* fp,struct CpuFighter* cpu)
{ continuation(fp);CHECK(cpu==&fp->cpu);++fallback_calls; }
static void ftCo_CpuTurnAround(Fighter* fp) { continuation(fp);++fallback_calls; }
static void ftCo_CpuFinishWithNeutralStick(Fighter* fp) { continuation(fp);++fallback_calls; }
static u8 gm_GetCurrentGameMode(void) { return 0; }
static int gm_8016C75C(HSD_GObj* source) { CHECK(source==object);return 0; }

#include "cpu_callback_originals.h"

static void remove_or_reuse(void)
{
    if (hazard == 1) {
        heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
        DWORD old;
        CHECK(VirtualProtect(fighter_page,fighter_size,PAGE_NOACCESS,&old));
        CHECK(VirtualProtect(object_page,object_size,PAGE_NOACCESS,&old));
    } else if (hazard == 2) {
        memset(fighter,0xA6,sizeof(*fighter));
        fighter->gobj = object;
        fighter->x8_spawnNum = 78;
        fighter->kind = Ft_Kind_CustomFirst + 1;
        memcpy(replacement,fighter,sizeof(*fighter));
        memcpy(replacement_object,object,sizeof(*object));
    }
}
static bool bool_callback(HSD_GObj* source)
{
    CHECK(source == object); ++callback_calls;
    if (explicit_travel_advance && result_bool) ++fighter->cpu.x90;
    if (callback_kind == 1) remove_or_reuse(); return result_bool;
}
static bool pending_callback(HSD_GObj* source)
{
    CHECK(source==object);++callback_calls;
    if(hazard) { remove_or_reuse();return result_bool; }
    if(pending_identity_mutation) {
        if(pending_identity_mutation==1) fighter->kind=Ft_Kind_Mario;
        else definition_enabled=false;
        return result_bool;
    }
    if(!fighter->cpu.xF8_b5) return false;
    ftCo_800B4880(fighter,38);fighter->cpu.xF8_b5=false;return true;
}
static bool classify_callback(HSD_GObj* source)
{
    CHECK(source == object); ++callback_calls;
    if (callback_kind == 2) remove_or_reuse();
    return classifier_bool;
}
static bool candidate_consume_without_write;
static bool candidate_callback(HSD_GObj* source,float angle,float distance)
{
    CHECK(source == object); CHECK(angle == 0.0f); CHECK(distance == 50.0f);
    ++callback_calls;
    if (candidate_consume_without_write) return true;
    bool selected = command_result >= 1 && command_result <= 61;
    if (selected) fighter->cpu.xA4 = command_result;
    if (callback_kind == 3) remove_or_reuse();
    return selected;
}
static void setup(void)
{
    DWORD old;
    CHECK(VirtualProtect(fighter_page,fighter_size,PAGE_READWRITE,&old));
    CHECK(VirtualProtect(object_page,object_size,PAGE_READWRITE,&old));
    memset(fighter,0,sizeof(*fighter)); memset(object,0,sizeof(*object));
    memset(&definition,0,sizeof(definition)); memset(heads,0,sizeof(heads));
    fighter->gobj = object; object->user_data = fighter;
    fighter->x8_spawnNum = 77; fighter->kind = Ft_Kind_CustomFirst;
    fighter->motion_id = ftCo_MS_Count; fighter->ground_or_air = GA_Air;
    fighter->cpu.x44 = &target; fighter->cpu.xF9_b5 = true;
    fighter->cpu.x90 = 11; fighter->cpu.x18 = 9; fighter->cpu.x1C = 10;
    target.cur_pos.x = 50.0f; target.cur_pos.y = 0.0f;
    heads[HSD_GOBJ_PLINK_FIGHTER] = object;
    definition.on_cpu_attack_decision = bool_callback;
    definition.on_cpu_travel_decision = bool_callback;
    definition.on_cpu_attack_motion = classify_callback;
    definition.on_cpu_special_candidate = candidate_callback;
    hazard=0; callback_kind=0; result_bool=false; classifier_bool=false;
    explicit_travel_advance=false;
    definition_enabled=true;pending_geometry=true;fallback_calls=0;
    pending_identity_mutation=0;
    weapon_attack=false; grounded_test=false; command_result=17;
    callback_calls=continuation_calls=execute_calls=reset_calls=wait_calls=0;
    candidate_prepare_calls=0;
}
static void check_hazard(void)
{
    if (hazard == 1) CHECK(!nativeCallbackSource_Alive(object,77,NativeCallbackSource_Fighter));
    else {
        CHECK(!nativeCallbackSource_Alive(object,77,NativeCallbackSource_Fighter));
        CHECK(memcmp(replacement,fighter,sizeof(*fighter)) == 0);
        CHECK(memcmp(replacement_object,object,sizeof(*object)) == 0);
    }
}
static void run_case(unsigned site)
{
    if(site==0) ftCo_800B658C(fighter);
    else if(site==1) ftCo_800B9020(fighter);
    else if(site==2) CHECK(!ftCo_800B9CBC(fighter));
    else if(site==3) CHECK(!cpu_outer_candidate_fragment(fighter));
    else (void)ftCo_800B630C(fighter);
}
static void hazard_matrix(void)
{
    // Direct decisions, their classifier continuation, candidate direct/nested/outer.
    for(unsigned site=0;site<5;++site) for(int mode=1;mode<=2;++mode)
        for(int value=0;value<=1;++value) {
            setup(); hazard=mode; result_bool=value; classifier_bool=value;
            callback_kind = site>=2 && site<=3 ? 3 : site==4 ? 2 : 1;
            if(site==2 || site==3) command_result=value?17:0;
            run_case(site); check_hazard(); CHECK(callback_calls==1); ++cases;
        }
    for(unsigned site=0;site<2;++site) for(int mode=1;mode<=2;++mode)
        for(int value=0;value<=1;++value) {
            setup(); hazard=mode; callback_kind=2; classifier_bool=value;
            run_case(site); check_hazard(); CHECK(callback_calls==2); ++cases;
        }
    for(int mode=1;mode<=2;++mode) for(int value=0;value<=1;++value) {
        setup(); hazard=mode; callback_kind=3; command_result=value?17:0;
        ftCo_800B9020(fighter); check_hazard(); CHECK(callback_calls==3);
        CHECK(execute_calls==0 && reset_calls==0); ++cases;
    }
}
static void normal_matrix(void)
{
    for(unsigned site=0;site<2;++site) {
        setup(); result_bool=true; run_case(site);
        CHECK(callback_calls==1 && execute_calls==0 && reset_calls==0);
        // Handling alone has no fighter decision attached: the authored
        // travel policy, rather than this common caller, owns any advance.
        CHECK(fighter->cpu.x90==11); ++cases;
        setup(); classifier_bool=true; fighter->cpu.xA4=17; run_case(site);
        CHECK(fighter->cpu.xA4==0 && reset_calls==1 && execute_calls==0);
        CHECK(fighter->cpu.x90==11); ++cases;
        setup(); fighter->cpu.xA4=17; run_case(site);
        CHECK(execute_calls==1 && fighter->cpu.xA4==0 && fighter->cpu.x90==11);
        CHECK(wait_calls==(site==0?1:0)); ++cases;
        setup(); fighter->cpu.x44=NULL; run_case(site);
        CHECK(callback_calls==0 && reset_calls==1 && fighter->cpu.x18==10); ++cases;
    }
    setup();result_bool=true;explicit_travel_advance=true;ftCo_800B9020(fighter);
    CHECK(callback_calls==1 && execute_calls==0 && reset_calls==0);
    CHECK(fighter->cpu.x90==12);++cases;
    const int commands[]={INT_MIN,-1,0,1,17,61,62,INT_MAX};
    for(unsigned i=0;i<sizeof(commands)/sizeof(commands[0]);++i) {
        bool valid=commands[i]>=1 && commands[i]<=61;
        setup(); command_result=commands[i]; CHECK(ftCo_800B9CBC(fighter)==valid);
        CHECK(fighter->cpu.xA4==(valid?commands[i]:0)); ++cases;
        setup(); command_result=commands[i]; ftCo_800B9020(fighter);
        CHECK(execute_calls==(valid?1:0) && reset_calls==(valid?0:1));
        CHECK(fighter->cpu.xA4==0 && fighter->cpu.x90==(valid?12:11)); ++cases;
        setup(); command_result=commands[i]; CHECK(cpu_outer_candidate_fragment(fighter)==valid);
        CHECK(fighter->cpu.x18==(valid?3:9) && execute_calls==(valid?1:0)); ++cases;
    }
    setup(); candidate_consume_without_write=true;
    CHECK(ftCo_800B9CBC(fighter)); CHECK(fighter->cpu.xA4==0); ++cases;
    candidate_consume_without_write=false;
    setup(); fighter->cpu.x18=3; CHECK(cpu_outer_candidate_fragment(fighter));
    CHECK(callback_calls==0 && execute_calls==0); ++cases;
    for(int i=0;i<3;++i) {
        setup(); if(i==0) fighter->cpu.x44=NULL;
        else if(i==1) fighter->cpu.xF9_b5=false; else fighter->cpu.x34=1;
        CHECK(!ftCo_800B9CBC(fighter)); CHECK(callback_calls==0);
        CHECK(fighter->cpu.xA4==0); ++cases;
    }
    setup(); fighter->motion_id=ftCo_MS_Attack11;
    CHECK(ftCo_800B630C(fighter)); CHECK(callback_calls==0); ++cases;
    setup(); weapon_attack=true; CHECK(ftCo_800B630C(fighter)); CHECK(callback_calls==0); ++cases;
    for(int result=0;result<=1;++result) {
        setup(); classifier_bool=result; CHECK(ftCo_800B630C(fighter)==result);
        CHECK(callback_calls==1); ++cases;
    }
    setup(); ftCo_800B658C(fighter);
    CHECK(wait_calls==1 && execute_calls==1 && fighter->cpu.xA4==0);
    CHECK(callback_calls==2 && fighter->cpu.x90==11); ++cases;
    setup(); weapon_attack=true; ftCo_800B9020(fighter);
    CHECK(callback_calls==1 && reset_calls==1 && execute_calls==0);
    CHECK(fighter->cpu.x90==11); ++cases;
}
static void invalid_entry_matrix(void)
{
    for(unsigned site=0;site<4;++site) for(int mode=0;mode<3;++mode) {
        setup();
        if(mode==0) fighter->gobj=NULL;
        else if(mode==1) object->user_data=NULL;
        else {
            heads[HSD_GOBJ_PLINK_FIGHTER]=NULL;
            DWORD old;
            CHECK(VirtualProtect(object_page,object_size,PAGE_NOACCESS,&old));
        }
        run_case(site);
        CHECK(callback_calls==0 && continuation_calls==0);
        CHECK(fighter->cpu.x90==11 && fighter->cpu.xA4==0);
        CHECK(fighter->cpu.x18==9); ++cases;
    }
}
static void pending_matrix(void)
{
    for(unsigned site=0;site<2;++site) {
        // Exact raw fallback, opted-in authored replacement, absent authored
        // hook, and unrelated stock character at the same priority point.
        for(unsigned kind=0;kind<4;++kind) for(unsigned hook=0;hook<2;++hook)
            for(unsigned pending=0;pending<2;++pending) {
            setup();fighter->cpu.level=4;grounded_test=true;
            fighter->kind=kind==0?Ft_Kind_Zelda:kind==1?Ft_Kind_Seak:
                kind==2?Ft_Kind_Mario:Ft_Kind_CustomFirst;
            fighter->cpu.xF8_b5=pending;
            if(hook) definition.on_cpu_pending_action=pending_callback;
            if(site) cpu_pending_charge_prefix(fighter);else ftCo_800ABA34(fighter);
            bool handled=pending&&(kind<2||(kind==3&&hook));
            CHECK(execute_calls==(handled?1:0));
            CHECK(fighter->cpu.xF8_b5==(pending&&!handled));
            CHECK(fallback_calls==(handled?0:1));
            CHECK(callback_calls==(kind==3&&hook?1:0));++cases;
        }
        for(int mode=1;mode<=2;++mode) for(int handled=0;handled<=1;++handled) {
            setup();definition.on_cpu_pending_action=pending_callback;
            result_bool=handled;hazard=mode;
            if(site) cpu_pending_charge_prefix(fighter);else ftCo_800ABA34(fighter);
            check_hazard();CHECK(callback_calls==1&&execute_calls==0&&fallback_calls==0);++cases;
        }
        for(unsigned mode=1;mode<=2;++mode) for(int handled=0;handled<=1;++handled) {
            setup();definition.on_cpu_pending_action=pending_callback;
            pending_identity_mutation=mode;result_bool=handled;
            if(site) cpu_pending_charge_prefix(fighter);else ftCo_800ABA34(fighter);
            CHECK(callback_calls==1&&execute_calls==0&&fallback_calls==0);++cases;
        }
        setup();definition.on_cpu_pending_action=pending_callback;fighter->cpu.xF8_b5=true;
        fighter->cpu.xC=11;
        if(site) cpu_pending_charge_prefix(fighter);else ftCo_800ABA34(fighter);
        CHECK(callback_calls==0&&execute_calls==1&&fighter->cpu.xF8_b5);++cases;
    }
    setup();definition.on_cpu_pending_action=pending_callback;fighter->cpu.xF8_b5=true;
    pending_geometry=false;cpu_pending_charge_prefix(fighter);
    CHECK(callback_calls==0&&fallback_calls==1&&fighter->cpu.xF8_b5);++cases;
    setup();definition.on_cpu_pending_action=pending_callback;fighter->cpu.xF8_b5=true;
    fighter->cpu.xC=12;fighter->cpu.x7C=30;ftCo_800ABA34(fighter);
    CHECK(callback_calls==0&&execute_calls==1&&fighter->cpu.xF8_b5);++cases;
    setup();definition.on_cpu_pending_action=pending_callback;fighter->cpu.xF8_b5=true;
    fighter->cpu.xC=16;fighter->ground_or_air=GA_Ground;fighter->cpu.x7C=300;
    ftCo_800ABA34(fighter);CHECK(callback_calls==0&&fallback_calls==1&&fighter->cpu.xF8_b5);++cases;
}
int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SYSTEM_INFO info; GetSystemInfo(&info);
    fighter_size=(sizeof(Fighter)+info.dwPageSize-1)/info.dwPageSize*info.dwPageSize;
    object_size=info.dwPageSize;
    fighter_page=VirtualAlloc(NULL,fighter_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    object_page=VirtualAlloc(NULL,object_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    CHECK(fighter_page && object_page); fighter=fighter_page; object=object_page;
    if(argc==2 && strcmp(argv[1],"--reject-travel-removal")==0) {
        setup(); hazard=1; callback_kind=1; result_bool=true; ftCo_800B9020(fighter); return 0;
    }
    hazard_matrix(); normal_matrix(); invalid_entry_matrix();pending_matrix();
    printf("{\"assertions\":%u,\"cases\":%u,\"protected_or_reused_cases\":32}\n",checks,cases);
    return 0;
}
