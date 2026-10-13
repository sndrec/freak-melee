// Full generated templates versus unchanged original lifecycle bodies.
// Common item/knockback inlines execute their actual production decisions;
// animation presentation and visibility setters are recorded fixture boundaries.
// This is console/source evidence, not gameplay or renderer acceptance.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/ftcamera.h>
#include <melee/ft/kinds/ftMars/forward.h>
#include <melee/ft/kinds/ftMars/ftmars_clone.h>
#include <melee/ft/kinds/ftEmblem/ftemblem_clone.h>
#include <sysdolphin/baselib/gobj.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* ParameterEntry)(const uint32_t*, unsigned, uint64_t);
static Fighter fighter;
static HSD_GObj object;
static unsigned calls, comparisons;
static bool heavy;
static Item_HoldKinds hold_kind;
typedef struct Call { int operation, a, b; float value; } Call;
static Call log_calls[16];
static void unexpected_host_call(void) { fputs("Unexpected closure Host call\n", stderr); abort(); }
static void record(int operation, int a, int b, float value)
{ assert(calls < 16); log_calls[calls++] = (Call){operation, a, b, value}; }
bool itIsHeavy(Item_GObj* item) { (void)item; return heavy; }
Item_HoldKinds itGetHoldKind(Item_GObj* item) { (void)item; return hold_kind; }
void ftAnim_80070FB4(Fighter_GObj* gobj, s32 a, s32 b) { assert(gobj == &object); record(1,a,b,0); }
void ftAnim_80070C48(Fighter_GObj* gobj, s32 a) { assert(gobj == &object); record(2,a,0,0); }
void ftAnim_80070CC4(Fighter_GObj* gobj, int a) { assert(gobj == &object); record(3,a,0,0); }
void ftAnim_800704F0(Fighter_GObj* gobj, int a, float value) { assert(gobj == &object); record(4,a,0,value); }
void ftParts_80074A4C(Fighter_GObj* gobj, int a, int b) { assert(gobj == &object); record(5,a,b,0); }
static void current_group(Fighter_GObj* gobj, int a, int b) { assert(gobj == &object); record(6,a,b,0); }
static void play_sound(Fighter* fp, enum_t sound, u8 volume, u8 pan)
{ assert(fp == &fighter); record(7,sound,volume,(float)pan); }
static bool source_alive(HSD_GObj* gobj, uint32_t generation, uint32_t source_class)
{ return gobj == &object && gobj->user_data == &fighter && generation == fighter.x8_spawnNum && source_class == NativeCallbackSource_Fighter; }
static void change_motion(Fighter_GObj* gobj, FtMotionId motion, MotionFlags flags,
                          float start, float speed, float blend, Fighter_GObj* other)
{
    assert(gobj==&object);(void)flags;(void)start;(void)speed;(void)blend;(void)other;
    fighter.motion_id=motion;
    if(fighter.x21EC) fighter.x21EC(gobj);
}
static void start_animation(Fighter_GObj* gobj) { assert(gobj==&object); }
#include "marth_roy_closure_fixture.h"

static const NativeFighterStateField* field(const NativeFighterDefinition* module, const char* name)
{
    for (unsigned i=0; i<module->state_field_count; ++i)
        if (!strcmp(module->state_fields[i].name,name)) return &module->state_fields[i];
    abort();
}
static void reset(const NativeFighterDefinition* module, unsigned seed)
{
    memset(&fighter,0,sizeof(fighter));
    fighter.x8_spawnNum = UINT32_C(0xF1234567);
    fighter.x5AC.model_num=3;
    object.user_data=&fighter; fighter.gobj=&object;
    memset(fighter.native_module_state,0xA5,module->state_bytes);
    fighter.native_module_state[field(module,"side_air_boost_used")->offset]=seed&1;
    fighter.u.ms.x222C=seed&1;
    calls=0; memset(log_calls,0,sizeof(log_calls));
}
static void parameters(HMODULE library, const char* path)
{
    char text[32768]; FILE* file=fopen(path,"rb"); assert(file);
    size_t size=fread(text,1,sizeof(text),file); assert(!ferror(file)&&size<sizeof(text)); fclose(file);
    NativeFighterParameters values; unsigned line;
    assert(nativeFighterParameters_Parse(text,size,&values,&line));
    uint32_t bits[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<values.count;++i) bits[i]=values.fields[i].bits;
    ParameterEntry bind=(ParameterEntry)GetProcAddress(library,"FreakMelee_SetFighterParameters");
    assert(bind && bind(bits,values.count,values.schema_hash));
}
static void events(const NativeFighterDefinition* module, unsigned family)
{
    HSD_GObjEvent original[]={family?ftFe_Init_OnDeath:ftMs_Init_OnDeath,
        family?ftFe_Init_OnItemVisible:ftMs_Init_OnItemVisible,
        family?ftFe_Init_OnItemInvisible:ftMs_Init_OnItemInvisible,
        family?ftFe_Init_OnKnockbackEnter:ftMs_Init_OnKnockbackEnter,
        family?ftFe_Init_OnKnockbackExit:ftMs_Init_OnKnockbackExit};
    HSD_GObjEvent authored[]={module->on_death,module->on_item_visible,module->on_item_invisible,
        module->on_knockback_enter,module->on_knockback_exit};
    for(unsigned seed=0;seed<32;++seed) {
        heavy=(seed>>1)&1; hold_kind=(Item_HoldKinds)((seed>>2)%6);
        for(unsigned event=0;event<7;++event) {
            reset(module,seed);
            if(event<5) original[event](&object);
            else if(event==5) {
                if(family) ftFe_Init_OnItemPickup(&object,seed&1);
                else ftMs_Init_OnItemPickup(&object,seed&1);
            } else {
                if(family) ftFe_Init_OnItemDrop(&object,seed&1);
                else ftMs_Init_OnItemDrop(&object,seed&1);
            }
            Call expected[16]; memcpy(expected,log_calls,sizeof(expected)); unsigned expected_calls=calls;
            uint32_t expected_boost=fighter.u.ms.x222C;
            reset(module,seed);
            // A disagreeing legacy word must not influence owned callbacks.
            fighter.u.ms.x222C=!(seed&1);
            if(event<5) authored[event](&object);
            else if(event==5) module->on_item_pickup(&object,seed&1);
            else module->on_item_drop(&object,seed&1);
            assert(calls==expected_calls && !memcmp(expected,log_calls,sizeof(expected)));
            assert(fighter.native_module_state[field(module,"side_air_boost_used")->offset]==expected_boost);
            assert(fighter.u.ms.x222C==!(seed&1));
            ++comparisons;
        }
        reset(module,seed); module->on_landing_action(&object);
        assert(!fighter.native_module_state[field(module,"side_air_boost_used")->offset]); ++comparisons;
    }
    for(int motion=0;motion<=400;++motion) {
        reset(module,0); fighter.motion_id=motion;
        assert(module->on_body_rebound(&object)==(motion==369||motion==371));
        assert(module->on_jab_restart(&object,motion)==(!family&&motion==46));
        assert(module->on_dynamics_wind(&object)); comparisons+=3;
    }
    reset(module,0); module->on_shield_start(&object);
    if(family) assert(calls==0);
    else assert(calls==2 && log_calls[0].operation==6 && log_calls[0].a==1 && log_calls[0].b==1 &&
        log_calls[1].operation==7 && log_calls[1].a==190115 && log_calls[1].b==127 && log_calls[1].value==64);
    Call expected[16];memcpy(expected,log_calls,sizeof(expected)); unsigned count=calls;
    reset(module,0);module->on_shield_reflect_start(&object);
    assert(count==calls&&!memcmp(expected,log_calls,sizeof(expected)));comparisons+=2;
    for(unsigned boost=0;boost<2;++boost) {
        reset(module,boost); fighter.cmd_vars[0]=77;
        fighter.mv.ms.specialn.cur_frame=121;
        int zero=0; memcpy(fighter.native_module_state+field(module,"charge_ticks")->offset,&zero,4);
        ftMs_SpecialN_80136730(&object);
        fighter.cmd_vars[0]=77;
        int charged=121;memcpy(fighter.native_module_state+field(module,"charge_ticks")->offset,&charged,4);
        fighter.native_module_state[field(module,"side_transition")->offset]=0;
        module->special_n(&object);
        assert(fighter.cmd_vars[0]==0);
        assert(!memcmp(fighter.native_module_state+field(module,"charge_ticks")->offset,&zero,4));++comparisons;
    }
}
int main(int argc,char** argv)
{
    assert(argc==5);
    NativeFighterHost host={0};closure_host_traps(&host);
    static MotionState common[ftCo_MS_Count];host.common_motion_states=common;
    host.callback_source_alive=source_alive;
    host.set_visibility_group=ftParts_80074A4C;
    host.set_current_visibility_group=current_group;
    host.fighter_item_pickup=Fighter_OnItemPickup;host.fighter_item_drop=Fighter_OnItemDrop;
    host.fighter_item_visible=Fighter_OnItemVisible;host.fighter_item_invisible=Fighter_OnItemInvisible;
    host.fighter_knockback_enter=Fighter_OnKnockbackEnter;host.fighter_knockback_exit=Fighter_OnKnockbackExit;
    host.play_sound=play_sound;
    host.change_motion_state=change_motion;host.start_animation=start_animation;
    const NativeFighterDefinition* bases[]={&ftMarsClone_Definition,&ftEmblemClone_Definition};
    for(unsigned family=0;family<2;++family) {
        HMODULE library=LoadLibraryA(argv[1+family*2]);assert(library);
        Entry entry=(Entry)GetProcAddress(library,"FreakMelee_GetFighterModule");assert(entry);
        parameters(library,argv[2+family*2]);
        const NativeFighterDefinition* module=entry(bases[family],&host);assert(module);
        assert(module->source_kind==Ft_Kind_None && module->motion_state_count==32);
        assert(module->bones==bases[family]->bones && module->bone_count==52 && module->omission_count==0);
        for(unsigned i=0;i<52;++i) {
            assert(module->bones[i].part==closure_expected_bones[family][i].part &&
                module->bones[i].joint==closure_expected_bones[family][i].joint);
            ++comparisons;
        }
        assert(module->demo_motion_names==bases[family]->demo_motion_names);
        for(unsigned i=0;i<32;++i) {
            const MotionState *stock=&bases[family]->motion_states[i], *script=&module->motion_states[i];
            assert(stock->anim_id==script->anim_id && stock->x4_flags==script->x4_flags);
            assert(script->anim_cb && script->input_cb && script->phys_cb && script->coll_cb);
            assert(script->anim_cb!=stock->anim_cb && script->input_cb!=stock->input_cb &&
                script->phys_cb!=stock->phys_cb && script->coll_cb!=stock->coll_cb);
            ++comparisons;
        }
        events(module,family);
        FreeLibrary(library);
    }
    printf("Marth/Roy closure: %u source comparisons, 0 failures\n",comparisons);
    return 0;
}
