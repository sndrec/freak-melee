// Generated semantic storage against unchanged common charge source bodies.
// Animation/color/rumble/sound delivery are controlled ordered boundaries.
#include "behavior.generated.h"
#include <Runtime/platform.h>
#include <placeholder.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0DF0.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftcolanim.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned checks, failures, pairs, invalid_writes;
#define CHECK(value) do { ++checks; if (!(value)) { ++failures; \
    if (failures < 12) fprintf(stderr,"line %u: %s\n",__LINE__,#value); } } while(0)
typedef struct PresentationEvent { int operation, argument, second; float value; } PresentationEvent;
static PresentationEvent events[2][16];
static unsigned event_count[2], side;
static int shake_count = 5;
static struct ftCommonData common;
struct ftCommonData* p_ftCommonData = &common;
static Vec2 shake_positions[5] = {{0,0},{1,-1},{-2,2},{3,-3},{-4,4}};
static struct Fighter_ShakeTable_t shake = { shake_positions, 5 };
struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable = &shake;

static void event(int op, int argument, int second, float value)
{
    CHECK(event_count[side] < 16);
    if (event_count[side] < 16)
        events[side][event_count[side]++] = (PresentationEvent){op,argument,second,value};
}
void ftAnim_SetAnimRate(Fighter_GObj* object, float value)
{ event(1,0,0,value); ((Fighter*)object->user_data)->frame_speed_mul=value; }
bool ftCo_800BFFD0(Fighter* fighter, enum_t color, bool value)
{ (void)fighter; event(2,color,value,0); return true; }
void ftCo_800C0200(Fighter* fighter, int color)
{ (void)fighter; event(3,color,0,0); }
void ftCommon_8007EBAC(Fighter* fighter, u32 id, u32 value)
{ (void)fighter; event(4,id,value,0); }
void ftCommon_8007ECD4(Fighter* fighter, s32 id)
{ (void)fighter; event(5,id,0,0); }
void ft_PlaySFX(Fighter* fighter, enum_t sound, u8 volume, u8 pan)
{ (void)fighter; event(6,sound,volume,(float)pan); }
static float common_attribute(NativeFighterCommonAttribute attribute)
{
    CHECK(attribute == NativeFighterCommon_SmashShakeCount);
    return (float)shake_count;
}
#include "original.h"

static void compare(const Fighter* a, const Fighter* b)
{
    CHECK(!memcmp(&a->smash_attrs,&b->smash_attrs,sizeof(SmashAttr)));
    CHECK(a->frame_speed_mul==b->frame_speed_mul);
    CHECK(event_count[0]==event_count[1]);
    CHECK(!memcmp(events[0],events[1],event_count[0]*sizeof(PresentationEvent)));
}
static void reset_events(void)
{ memset(events,0,sizeof(events));event_count[0]=event_count[1]=0; }
static unsigned offset(const NativeFighterDefinition* definition,const char* name)
{
    for (unsigned i=0;i<definition->state_field_count;++i)
        if (!strcmp(definition->state_fields[i].name,name)) return definition->state_fields[i].offset;
    CHECK(false); return 0;
}
static int32_t read_int(Fighter* fighter,const NativeFighterDefinition* definition,const char* name)
{ int32_t result;memcpy(&result,fighter->native_module_state+offset(definition,name),4);return result; }
static float read_float(Fighter* fighter,const NativeFighterDefinition* definition,const char* name)
{ float result;memcpy(&result,fighter->native_module_state+offset(definition,name),4);return result; }
static void seed(Fighter* fighter)
{
    memset(fighter,0,sizeof(*fighter));
    fighter->x8_spawnNum=UINT32_C(0x80000001);
    fighter->frame_speed_mul=1.25f;
    fighter->smash_attrs.x2120_damageMul=1.4f;
    fighter->smash_attrs.x2130_sfxBool=1;
    fighter->smash_attrs.x212E=23;
    fighter->smash_attrs.x212F=-12;
}
static void initialize(const NativeFighterDefinition* definition,HSD_GObj* object)
{
#if SMASH_CALLBACK_ROUTE
    definition->motion_states[0].phys_cb(object);
#else
    definition->on_load(object);
#endif
}
int main(void)
{
    NativeFighterHost host={.common_attribute=common_attribute};
    MotionState motion={0};
    NativeFighterDefinition base={.fighter_root="ftDataNess",
        .motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost absent=host;absent.common_attribute=NULL;
    CHECK(FreakMelee_GetFighterModule(&base,&absent)==NULL);
    const NativeFighterDefinition* definition=FreakMelee_GetFighterModule(&base,&host);
    CHECK(definition!=NULL);if(!definition)return 1;
    Fighter* fighters=calloc(2,sizeof(*fighters));CHECK(fighters!=NULL);if(!fighters)return 1;
    HSD_GObj objects[2]={0};for(unsigned i=0;i<2;++i)objects[i].user_data=&fighters[i];
    seed(&fighters[0]);fighters[1]=fighters[0];
    initialize(definition,&objects[0]);
    // Independent expected field writes. No stock smash handler is delegated.
    SmashAttr* expected=&fighters[1].smash_attrs;
    expected->state=(SmashState)4;expected->x2124_frameSpeedMul=1.25f;
    expected->x2118_frames=0;expected->x211C_holdFrame=60;
    expected->x212C=0;expected->x212D=5;expected->x2128=119;
    compare(&fighters[0],&fighters[1]);
    CHECK(fighters[0].smash_attrs.x2120_damageMul==1.4f);
    CHECK(fighters[0].smash_attrs.x2130_sfxBool==1);
    script_fn_ReadAll(&objects[0]);
    CHECK(read_int(&fighters[0],definition,"mode")==4);
    CHECK(read_float(&fighters[0],definition,"frames")==0);
    CHECK(read_float(&fighters[0],definition,"limit")==60);
    CHECK(read_float(&fighters[0],definition,"saved_rate")==1.25f);
    CHECK(read_int(&fighters[0],definition,"color")==119);
    CHECK(read_int(&fighters[0],definition,"index")==0);
    CHECK(read_int(&fighters[0],definition,"period")==5);
    CHECK(read_int(&fighters[0],definition,"table_count")==5);
    static const int32_t values[]={INT32_MIN,-1,0,1,4,5,254,255,256,INT32_MAX};
    static const int counts[]={-1,0,1,5,255,256};
    for(unsigned c=0;c<sizeof(counts)/sizeof(counts[0]);++c)
        for(unsigned v=0;v<sizeof(values)/sizeof(values[0]);++v){
            shake_count=counts[c];int32_t value=values[v];
            seed(&fighters[0]);fighters[0].smash_attrs.state=(SmashState)3;
            script_fn_SetMode(&objects[0],value);
            CHECK(fighters[0].smash_attrs.state==((uint32_t)value<=4?(SmashState)value:SmashState_Release));
            fighters[0].smash_attrs.x212C=2;
            script_fn_SetIndex(&objects[0],value);
            bool valid=shake_count>0&&shake_count<=255&&value>=0&&value<shake_count;
            CHECK(fighters[0].smash_attrs.x212C==(valid?(u8)value:2));
            fighters[0].smash_attrs.x212D=3;
            script_fn_SetPeriod(&objects[0],value);
            valid=shake_count>0&&shake_count<=255&&value>=1&&value<=shake_count;
            CHECK(fighters[0].smash_attrs.x212D==(valid?(u8)value:3));
            ++invalid_writes;
        }
    shake_count=5;common.x7C8=30;
    static const float frames[]={0,29,30,59,60,61};
    for(int mode=0;mode<=4;++mode)for(unsigned f=0;f<6;++f)
        for(int held=0;held<2;++held)for(int latch=0;latch<2;++latch)
        for(int color=0;color<2;++color){
            seed(&fighters[0]);initialize(definition,&objects[0]);
            script_fn_SetMode(&objects[0],mode);
            fighters[0].smash_attrs.x2118_frames=frames[f];
            fighters[0].smash_attrs.x212C=4;
            fighters[0].smash_attrs.x2130_sfxBool=latch;
            fighters[0].smash_attrs.x2128=color?123:119;
            fighters[0].input.held_buttons[0]=held?HSD_PAD_A:0;
            fighters[1]=fighters[0];reset_events();
            side=0;ftCo_800DEF38(&objects[0]);side=1;ftCo_800DEF38(&objects[1]);
            compare(&fighters[0],&fighters[1]);
            if(mode==4){
                CHECK(fighters[0].smash_attrs.state==4);
                CHECK(fighters[0].smash_attrs.x2118_frames==frames[f]+1);
                CHECK(fighters[0].frame_speed_mul==1.25f);
                for(unsigned i=0;i<event_count[0];++i)CHECK(events[0][i].operation==6);
            }
            if(mode==2&&frames[f]>=59){
                CHECK(fighters[0].smash_attrs.state==SmashState_Release);
                CHECK(fighters[0].smash_attrs.x2118_frames==60);
                CHECK(event_count[0]>=3);
                CHECK(events[0][0].operation==1&&events[0][1].operation==3&&events[0][2].operation==5);
            }
            Vec2 shifts[2]={{77,88},{77,88}};
            Vec2* results[2]={ftCo_800DEEE8(&fighters[0],&shifts[0]),ftCo_800DEEE8(&fighters[1],&shifts[1])};
            CHECK((results[0]!=NULL)==(results[1]!=NULL));CHECK(!memcmp(shifts,shifts+1,sizeof(Vec2)));
            bool shaking=fighters[0].smash_attrs.state==SmashState_Charging||fighters[0].smash_attrs.state==4;
            CHECK((results[0]!=NULL)==shaking);
            if(shaking){CHECK(shifts[0].x==0);CHECK(shifts[0].y==0);}
            CHECK(ftCo_800DEEB8(&fighters[0],10)==ftCo_800DEEB8(&fighters[1],10));
            int updated_mode=fighters[0].smash_attrs.state;
            reset_events();side=0;ftCo_800DF0D0(&objects[0]);side=1;ftCo_800DF0D0(&objects[1]);
            compare(&fighters[0],&fighters[1]);
            if(updated_mode==SmashState_PreCharge){
                CHECK(fighters[0].smash_attrs.state==(held?SmashState_Charging:SmashState_None));
                if(held){
                    CHECK(fighters[0].frame_speed_mul==0);
                    CHECK(fighters[0].smash_attrs.x2124_frameSpeedMul==1.25f);
                    CHECK(fighters[0].smash_attrs.x212C==0&&fighters[0].smash_attrs.x212D==5);
                    CHECK(fighters[0].smash_attrs.x2130_sfxBool==0);
                    CHECK(event_count[0]==(color?2:3));
                    CHECK(events[0][0].operation==1);
                    CHECK(events[0][event_count[0]-1].operation==4);
                    if(!color)CHECK(events[0][1].operation==2);
                }else CHECK(event_count[0]==0);
            }else if(updated_mode==SmashState_Charging&&!held){
                CHECK(fighters[0].smash_attrs.state==SmashState_Release);
                CHECK(event_count[0]==3);
                CHECK(events[0][0].operation==1&&events[0][1].operation==3&&events[0][2].operation==5);
            }else CHECK(event_count[0]==0);
            float retained=fighters[0].smash_attrs.x2118_frames;
            ftCo_800DEEA8(&objects[0]);CHECK(fighters[0].smash_attrs.state==SmashState_None);
            CHECK(fighters[0].smash_attrs.x2118_frames==retained);
            ++pairs;
        }
    free(fighters);
    printf("%u checks, %u failures, %u common source pairs, %u bounded write cases\n",checks,failures,pairs,invalid_writes);
    return failures?1:0;
}
