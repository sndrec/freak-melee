// Actual common dispatch, with article construction and capture transition
// boundaries shared with the fighter phase fixture. This is not a solver test.
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_CatchPull.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemThrow.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>

static bool held_throw, pummel, directional_throw;
static unsigned common_cases, register_count[2], wait_count[2], throw_checks[2];

bool ftCo_800951D0(Fighter_GObj* g) { ++throw_checks[side(g)];return held_throw; }
bool ftCo_80095254(Fighter_GObj* g) { ++throw_checks[side(g)];return held_throw; }
bool ftCo_800952DC(Fighter_GObj* g) { ++throw_checks[side(g)];return held_throw; }
// The native Link policy branches are not the subject of this Samus fixture.
bool fn_800D8EC8(Fighter_GObj* g) { CHECK(GET_FIGHTER(g)->kind==Ft_Kind_Samus);return false; }
bool fn_800D9228(Fighter_GObj* g) { CHECK(GET_FIGHTER(g)->kind==Ft_Kind_Samus);return false; }
void fn_800D949C(Fighter_GObj* g) { CHECK(GET_FIGHTER(g)->kind==Ft_Kind_Samus); }
void fn_800D8BFC(Fighter_GObj* g) { (void)g;abort(); }
void fn_800DAADC(Fighter_GObj* a,Fighter_GObj* b) { (void)a;(void)b;abort(); }
void fn_800DA054(Fighter_GObj* g) { (void)g;abort(); }
void ftAnim_SetAnimRate(Fighter_GObj* g,float rate) { (void)g;(void)rate;abort(); }
void ftAnim_8006EBA4(Fighter_GObj* g) { (void)g;abort(); }
void fn_800DA1D8(Fighter_GObj* g) { ++wait_count[side(g)];event(side(g),WaitCapture,0,0,0,0,0,0); }
bool fn_800DA4C0(Fighter_GObj* g) { event(side(g),Pummel,pummel,0,0,0,0,0);return pummel; }
bool ftCo_800DD1E4(Fighter_GObj* g) { event(side(g),Throw,directional_throw,0,0,0,0,0);return directional_throw; }
void ftCo_Fall_Enter(Fighter_GObj* g) { event(side(g),ExitFall,0,0,0,0,0,0);fighters[side(g)].motion_id=29; }
void ftCommon_8007E2D0(Fighter* fp,s16 mask,HSD_GObjEvent capture,HSD_GObjEvent item,void(*caught)(HSD_GObj*,HSD_GObj*))
{ CHECK(mask==1 && capture==fn_800D9CE8 && item==fn_800D8BFC && caught==fn_800DAADC);++register_count[fside(fp)]; }
void it_802BAA94(Item_GObj* g)
{ CHECK(tether_live[side(g)]);event(side(g),Retract,5,0,0,0,0,0); }
void it_802A7840(HSD_GObj* g) { (void)g;abort(); }

#include "catch_originals.h"

static void common_matrix(void)
{
    for(unsigned route=0;route<3;++route)for(unsigned bits=0;bits<32;++bits){
        reset(7);held_throw=bits&1;
        memset(register_count,0,sizeof(register_count));memset(throw_checks,0,sizeof(throw_checks));
        for(unsigned s=0;s<2;++s){
            tether_live[s]=bits&2;fighters[s].u.ss.x223C=tether_live[s]?&tether[s]:NULL;
            fighters[s].ground_or_air=GA_Ground;
            fighters[s].input.held_buttons[0]=(bits&4)?HSD_PAD_LR:0;
            fighters[s].input.pressed_buttons=(bits&8)?HSD_PAD_A:0;
            fighters[s].mv.co.common.x0=(bits&16)?1:0;
        }
        integer("tether_handle",tether_live[1]?(int32_t)items[1].x1C:-1);
        current=0;bool stock=route==0?ftCo_Catch_CheckInput(&objects[0]):route==1?ftCo_800D8A38(&objects[0]):ftCo_800D8AE0(&objects[0]);
        current=1;bool authored=route==0?ftCo_Catch_CheckInput(&objects[1]):route==1?ftCo_800D8A38(&objects[1]):ftCo_800D8AE0(&objects[1]);
        CHECK(stock==authored);CHECK(register_count[0]==register_count[1]);CHECK(throw_checks[0]==throw_checks[1]);
        if(register_count[0])same(fighters[0].mv.co.catch.x0,read_scalar("tether_counter"));
        compare(false,false);++common_cases;
    }
    // Actual common animation continuation, including the finished-animation
    // check after the authored frame decision and constructor failure return.
    for(unsigned dash=0;dash<2;++dash)for(unsigned bits=0;bits<16;++bits){
        reset((float)fighter_attrs.x9C-1);if(dash){fighters[0].mv.co.catch.x0=(float)fighter_attrs.xAC-1;scalar("tether_counter",(float)fighter_attrs.xAC-1);}
        create_fail=bits&1;finished=bits&2;blocked=bits&4;
        for(unsigned s=0;s<2;++s){fighters[s].motion_id=dash?214:212;fighters[s].facing_dir=(bits&8)?-1:1;fighters[s].ground_or_air=GA_Ground;}
        current=0;if(dash)ftCo_CatchDash_Anim(&objects[0]);else ftCo_Catch_Anim(&objects[0]);
        current=1;if(dash)ftCo_CatchDash_Anim(&objects[1]);else ftCo_Catch_Anim(&objects[1]);
        compare(true,false);++common_cases;
    }
    for(unsigned dash=0;dash<2;++dash)for(unsigned bits=0;bits<4;++bits){
        reset(3);memset(wait_count,0,sizeof(wait_count));completed=bits&1;finished=bits&2;
        for(unsigned s=0;s<2;++s){fighters[s].motion_id=dash?214:212;fighters[s].cur_anim_frame=9.25f;fighters[s].gr_vel=3;
            items[s].xDD4_itemVar.samusgrapple.x14=completed;items[s].xDD4_itemVar.samusgrapple.xC=&endpoint_models[s];}
        current=0;fn_800D9CE8(&objects[0]);current=1;fn_800D9CE8(&objects[1]);
        CHECK(fighters[0].mv.co.capturedamage.x18==&endpoint_models[0]);CHECK(fighters[1].mv.co.capturedamage.x18==&endpoint_models[1]);
        CHECK(fighters[0].accessory1_cb==fn_800DA190 && fighters[1].accessory1_cb==NULL);
        current=0;unsigned before=event_count[0];fighters[0].accessory1_cb(&objects[0]);CHECK(before==event_count[0]);
        current=0;ftCo_CatchPull_Anim(&objects[0]);current=1;ftCo_CatchPull_Anim(&objects[1]);
        CHECK(wait_count[0]==wait_count[1]);compare(false,false);++common_cases;
    }
    // Missing retained resource is a defined authored fallback. The original
    // entry dereferences its null raw pointer, so only ready is source-compared.
    reset(3);memset(wait_count,0,sizeof(wait_count));for(unsigned s=0;s<2;++s){tether_live[s]=false;fighters[s].u.ss.x223C=NULL;}
    integer("tether_handle",-1);current=0;ftCo_CatchPull_Anim(&objects[0]);current=1;ftCo_CatchPull_Anim(&objects[1]);
    CHECK(wait_count[0]==1 && wait_count[1]==1);compare(false,false);++common_cases;
    for(unsigned bits=0;bits<4;++bits){
        reset(3);pummel=bits&1;directional_throw=bits&2;
        current=0;ftCo_CatchWait_IASA(&objects[0]);current=1;ftCo_CatchWait_IASA(&objects[1]);compare(false,false);++common_cases;
        reset(3);current=0;fn_800D8E30(&objects[0]);current=1;fn_800D8E30(&objects[1]);compare(false,false);++common_cases;
    }
}
