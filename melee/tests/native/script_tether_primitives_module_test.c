#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if TETHER_CALLBACKS
#include "script_tether_primitives_callback_fixture.h"
#else
#include "script_tether_primitives_action_fixture.h"
#endif

static unsigned checks, stage_calls, audio_calls, probe_calls, occupant_calls;
static Fighter_GObj* live;
static uint32_t generation;
static bool blocked, fast_request, probe_result, occupied;
static GroundOrAir ground_air;
static int damage_calls, cliff_calls, freefall_calls;
static unsigned invalidate;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static bool alive(HSD_GObj* object,uint32_t id,uint32_t source)
{ return object == live && id == generation && source == NativeCallbackSource_Fighter && object->user_data && ((Fighter*)object->user_data)->x8_spawnNum == generation; }
static bool segment(Vec3* pos,int* line,u32* flags,Vec3* normal,int skip,int only,
                    float x0,float y0,float x1,float y1)
{
    CHECK(!pos && !line && !flags && !normal && skip == -1 && only == -1);
    CHECK(x0==1 && y0==2 && x1==3 && y1==4); ++stage_calls; return blocked;
}
static int audio(enum_t sound,int volume,int pan)
{
    ++audio_calls;
    if (sound==701) CHECK(volume==127 && pan==64);
    else if (sound==702) CHECK(volume==100 && pan==32);
    else { CHECK(sound==703); CHECK(volume==101 && pan==33); }
    return sound+1000;
}
static bool check_fastfall(Fighter* fighter) { CHECK(fighter == live->user_data); return fast_request; }
static void fastfall(Fighter* fighter) { CHECK(fighter == live->user_data); fighter->fall_fast=true; }
static bool probe(Fighter_GObj* object,float y,float height)
{ CHECK(object==live && y==5 && height==6); ++probe_calls; return probe_result; }
static Fighter_GObj* occupant(Fighter_GObj* object)
{ CHECK(object==live); ++occupant_calls; return occupied ? object : NULL; }
static GroundOrAir air_collision(Fighter_GObj* object) { CHECK(object==live); return ground_air; }
static void invalidate_source(Fighter_GObj* object)
{ if (invalidate == 1) object->user_data = NULL; else if (invalidate == 2) ((Fighter*)object->user_data)->x8_spawnNum = generation + 1; }
static void damage_fall(Fighter_GObj* object) { CHECK(object==live); ++damage_calls; invalidate_source(object); }
static void cliff(Fighter_GObj* object,float scale) { CHECK(object==live && scale==1.5f); ++cliff_calls; invalidate_source(object); }
static void freefall(Fighter_GObj* object,int first,int second,bool interrupt,float mobility,float lag)
{ CHECK(object==live && first==1 && second==0 && interrupt && mobility==1 && lag==4.25f); ++freefall_calls; invalidate_source(object); }
static unsigned field(const NativeFighterStateField* fields,unsigned count,const char* name)
{ for (unsigned i=0;i<count;++i) if (!strcmp(fields[i].name,name)) return fields[i].offset; assert(0); return 0; }
static bool boolean(const NativeFighterDefinition* d,Fighter* f,const char* name)
{ return f->native_module_state[field(d->state_fields,d->state_field_count,name)] != 0; }
static float scalar(const NativeFighterDefinition* d,Fighter* f,const char* name)
{ float value;memcpy(&value,f->native_module_state+field(d->state_fields,d->state_field_count,name),4);return value; }
static int integer(const NativeFighterDefinition* d,Fighter* f,const char* name)
{ int value;memcpy(&value,f->native_module_state+field(d->state_fields,d->state_field_count,name),4);return value; }
int main(void)
{
    MotionState motion={0}; NativeFighterDefinition base={.fighter_root="ftDataLink",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.stage_segment_intersection=segment,.play_global_sound=audio,.check_fastfall=check_fastfall,
        .fastfall=fastfall,.probe_ledge=probe,.find_ledge_occupant=occupant,.air_collision=air_collision,
        .enter_freefall=freefall,.enter_damage_fall=damage_fall,.enter_cliff_jump_phase2=cliff,.callback_source_alive=alive};
    const NativeFighterDefinition* d=FreakMelee_GetFighterModule(&base,&host);CHECK(d);
#define MISSING(member) do { NativeFighterHost missing=host; missing.member=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&missing)); } while(0)
    MISSING(stage_segment_intersection); MISSING(play_global_sound); MISSING(check_fastfall); MISSING(fastfall);
    MISSING(probe_ledge); MISSING(find_ledge_occupant); MISSING(air_collision); MISSING(enter_damage_fall);
    MISSING(enter_freefall); MISSING(enter_cliff_jump_phase2); MISSING(callback_source_alive);
    d=FreakMelee_GetFighterModule(&base,&host);CHECK(d);
    Fighter* fighter=calloc(1,sizeof(*fighter));Item* item=calloc(1,sizeof(*item));CHECK(fighter && item);
    HSD_GObj fg={.user_data=fighter},ig={.user_data=item};live=&fg;generation=UINT32_C(0x80000003);
    fighter->x8_spawnNum=generation;item->entity=&ig;
    for(unsigned bits=0;bits<128;++bits){
        blocked=(bits&1)!=0;fast_request=(bits&2)!=0;probe_result=(bits&4)!=0;occupied=(bits&8)!=0;
        ground_air=(bits&16)?GA_Air:GA_Ground;fighter->input.held_buttons[0]=(bits&32)?HSD_PAD_LR:0;
        fighter->used_tether=(bits&64)!=0;fighter->fall_fast=(bits&1)!=0;
        fighter->x2EC=4.25f;fighter->coll_data.cur_pos=(Vec3){31,32,33};fighter->cur_pos=(Vec3){41,42,43};
        fighter->pos_delta=(Vec3){51,52,53};fighter->x74_self_accel=(Vec3){61,62,63};
        unsigned old_occupant=occupant_calls;
#if TETHER_CALLBACKS
        d->motion_states[0].phys_cb(&fg);
#else
        d->on_load(&fg);
#endif
        CHECK(boolean(d,fighter,"was_used")==((bits&64)!=0));CHECK(boolean(d,fighter,"was_fast")==((bits&1)!=0));
        CHECK(boolean(d,fighter,"shield")==((bits&32)!=0));CHECK(boolean(d,fighter,"blocked")==blocked);
        CHECK(boolean(d,fighter,"fast")==fast_request);CHECK(boolean(d,fighter,"available")== (probe_result && !occupied));
        CHECK(occupant_calls-old_occupant==(unsigned)probe_result);CHECK(boolean(d,fighter,"in_air")==((bits&16)!=0));
        CHECK(integer(d,fighter,"sound")==1701);CHECK(integer(d,fighter,"masks")== (Collide_LeftWallMask|Collide_RightWallMask));
        CHECK(scalar(d,fighter,"lag")==4.25f);CHECK(scalar(d,fighter,"cx")==31);CHECK(scalar(d,fighter,"cy")==32);CHECK(scalar(d,fighter,"cz")==33);
        CHECK(fighter->pos_delta.x==9 && fighter->pos_delta.y==10 && fighter->pos_delta.z==11);
        CHECK(fighter->x74_self_accel.x==61 && fighter->x74_self_accel.y==62 && fighter->x74_self_accel.z==12);
        CHECK(fighter->cur_pos.x==41 && fighter->cur_pos.y==42 && fighter->cur_pos.z==43);CHECK(fighter->used_tether && fighter->fall_fast);
        const NativeFighterArticleLogic* a=&d->article_logics[0];a->on_created(&ig);
        CHECK(item->xDD4_itemVar.native_article.state[field(a->state_fields,a->state_field_count,"blocked")]==blocked);
        int sound;memcpy(&sound,item->xDD4_itemVar.native_article.state+field(a->state_fields,a->state_field_count,"sound"),4);CHECK(sound==1702);
    }
    CHECK(d->on_death1 && d->on_air_tether_input); authored_damage(&fg);d->on_death1(&fg);
    CHECK(damage_calls==1 && cliff_calls==1);
    for (unsigned mode=0;mode<3;++mode) {
        invalidate=mode;fg.user_data=fighter;fighter->x8_spawnNum=generation;
        unsigned continued=field(d->state_fields,d->state_field_count,"continued");
        fighter->native_module_state[continued]=false;
        d->on_death1(&fg);
        CHECK(fighter->native_module_state[continued] == (mode==0));
        fg.user_data=fighter;fighter->x8_spawnNum=generation;
        fighter->input.held_buttons[0]=HSD_PAD_LR;fighter->native_module_state[continued]=false;
        CHECK(fighter->death2_cb);fighter->death2_cb(&fg);CHECK(fighter->native_module_state[continued] == (mode==0));
        fg.user_data=fighter;fighter->x8_spawnNum=generation;fighter->used_tether=true;
        CHECK(d->on_air_tether_input(&fg) == (mode==0));
        CHECK(fighter->used_tether == (mode!=0));
    }
    CHECK(freefall_calls==3);CHECK(stage_calls==384 && audio_calls==384 && probe_calls==128 && occupant_calls==64);
    free(item);free(fighter);printf("{\"generated_module_checks\":%u,\"matrix_cases\":128}\n",checks);return 0;
}
