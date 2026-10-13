#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_0CDD.h>
#include <melee/ft/kinds/ftCommon/ftCo_Attack100.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemScopeStart.h>
#include <melee/ft/kinds/ftCommon/ftCo_ItemScopeFire.h>
#include <melee/ft/inlines.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/types.h>
#include <melee/it/kinds/itsscope.h>
#include <melee/it/it_26B1.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks,pairs,cases;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"scope %u: %s\n",__LINE__,#x);exit(1);}}while(0)
static Fighter fighter,before;
static HSD_GObj actor,item_object;
static Item item;
static Article article;
static itSScopeAttributes attrs;
static ftCommonData common;
ftCommonData* p_ftCommonData=&common;
static bool frames,empty;
static unsigned trace[32],trace_count;
static void record(unsigned op){CHECK(trace_count<32);trace[trace_count++]=op;}
static void ground(HSD_GObj* object){CHECK(object==&actor);record(1);}
static void air(HSD_GObj* object,int level){CHECK(object==&actor);record(100+(unsigned)level);}
bool ftAnim_IsFramesRemaining(Fighter_GObj* object){CHECK(object==&actor);return frames;}
bool it_8026B594(Item_GObj* object){CHECK(object==&item_object);return empty;}
void ftAnim_8006EBA4(Fighter_GObj* object){CHECK(object==&actor);record(10);}
void ftCommon_8007E79C(Fighter_GObj* object,s32 value){CHECK(object==&actor&&value==1);record(11);}
void ftCommon_8007E7E4(Fighter_GObj* object,s32 value){CHECK(object==&actor&&value==1);record(12);}
void ftCommon_ClampAirDrift(Fighter* fp){CHECK(fp==&fighter);record(13);}
void ft_800892A0(Fighter_GObj* object){CHECK(object==&actor);record(14);}
void ft_80089824(Fighter_GObj* object){CHECK(object==&actor);record(15);}
void Fighter_ChangeMotionState(Fighter_GObj* object,FtMotionId motion,MotionFlags flags,float frame,float rate,float blend,HSD_GObj* donor)
{CHECK(object==&actor&&!donor&&frame==0&&rate==1&&blend==0);record(200+(unsigned)motion);record(flags);fighter.motion_id=motion;}
// Unrelated presentation callbacks are linked but never delivered here.
void fn_800D84D4(Fighter_GObj* object,int level){CHECK(object==&actor);record(300+(unsigned)level);}
static inline Item* get_item(HSD_GObj* object){return object->user_data;}
#define GET_ITEM(object) get_item(object)
#define GET_ATTRS(ip) ((itSScopeAttributes*)(ip)->xC4_article_data->x4_specialAttributes)
#include "item_charge.inc"
#include "reference.inc"

void ftCo_ItemScopeRapid_IASA(Fighter_GObj*);
void ftCo_ItemScopeAirRapid_IASA(Fighter_GObj*);
void ftCo_ItemScopeRapid_Anim(Fighter_GObj*);
void ftCo_ItemScopeAirRapid_Anim(Fighter_GObj*);
void fn_800D79B4(HSD_GObj*,void(*)(HSD_GObj*),void(*)(HSD_GObj*,int));
void fn_800D7BDC(Fighter_GObj*);
void fn_800D7C60(Fighter_GObj*);
void fn_800D8140(Fighter_GObj*,int);
void fn_800D81D0(Fighter_GObj*,int);
void ft_800D76B8(Fighter_GObj*);
void fn_800D7938(Fighter_GObj*);
FtMotionId fn_800D769C(Fighter*,FtMotionId);
#include "actual.inc"

static void setup(void)
{
    memset(&fighter,0xA5,sizeof(fighter));memset(&actor,0,sizeof(actor));memset(&item,0,sizeof(item));
    actor.user_data=&fighter;item_object.user_data=&item;fighter.gobj=&actor;fighter.item_gobj=&item_object;
    fighter.input.held_buttons[0]=fighter.input.pressed_buttons=0;fighter.motion_id=ftCo_MS_ItemScopeStart;
    fighter.cur_anim_frame=0;fighter.frame_speed_mul=1;item.xC4_article_data=&article;article.x4_specialAttributes=&attrs;
    attrs.x4=8;attrs.x8=16;for(unsigned i=0;i<10;++i)attrs.xC[i]=(int)i+1;
    common.x5B8=64;common.x5BC=6;item.xD4C=64;frames=empty=false;trace_count=0;
}
static void canary(void)
{
    Fighter saved=fighter;
    memcpy(&saved.mv,&before.mv,8);saved.motion_id=before.motion_id;saved.throw_flags=before.throw_flags;
    saved.accessory4_cb=before.accessory4_cb;saved.take_dmg_cb=before.take_dmg_cb;
    CHECK(!memcmp(&saved,&before,sizeof(saved)));
}
static void raw_start(float timer,int release)
{memcpy((u8*)&fighter+0x2340,&timer,4);memcpy((u8*)&fighter+0x2344,&release,4);}
static float raw_timer(void){float timer;memcpy(&timer,(u8*)&fighter+0x2340,4);return timer;}
static int raw_release(void){int release;memcpy(&release,(u8*)&fighter+0x2344,4);return release;}
static void charging(void)
{
    const float times[]={-0.0f,0,7.5f,8,63,64,65,128};
    for(unsigned t=0;t<8;++t)for(unsigned held=0;held<2;++held)for(unsigned release=0;release<2;++release)for(unsigned remain=0;remain<2;++remain) {
        setup();fighter.input.held_buttons[0]=held?HSD_PAD_A:0;frames=remain!=0;raw_start(times[t],(int)release);
        reference_fn_800D79B4(&actor,ground,air);float timer=raw_timer();int released=raw_release();unsigned expected[32],count=trace_count;memcpy(expected,trace,sizeof(expected));
        setup();fighter.input.held_buttons[0]=held?HSD_PAD_A:0;frames=remain!=0;fighter.mv.co.itemscope_start.charge_frames=times[t];fighter.mv.co.itemscope_start.released=(int)release;before=fighter;
        fn_800D79B4(&actor,ground,air);CHECK(!memcmp(&timer,&fighter.mv.co.itemscope_start.charge_frames,4)&&released==fighter.mv.co.itemscope_start.released);
        CHECK(trace_count==count&&!memcmp(trace,expected,count*sizeof(*trace)));canary();++pairs;
    }
    ++cases;
    // Positive ammo1 with a nonzero requested level has native undefined
    // fallthrough in it_80291DAC. Use12 instead; no production rule is changed.
    for(unsigned t=0;t<8;++t)for(int a=0;a<12;++a){int ammo=a==1?12:a;setup();item.xD4C=ammo;memcpy((u8*)&fighter+offsetof(FighterOverlay,x2340),&times[t],4);reference_fn_800D7938(&actor);int expected_ammo=item.xD4C;unsigned count=trace_count;
        setup();item.xD4C=ammo;fighter.mv.co.itemscope_start.charge_frames=times[t];before=fighter;fn_800D7938(&actor);CHECK(item.xD4C==expected_ammo&&trace_count==count);canary();++pairs;}
    ++cases;
    for(unsigned e=0;e<2;++e){setup();empty=e!=0;reference_ft_800D76B8(&actor);float timer=raw_timer();int released=raw_release();unsigned expected[32],count=trace_count;memcpy(expected,trace,sizeof(expected));
        setup();empty=e!=0;before=fighter;ft_800D76B8(&actor);CHECK(!memcmp(&timer,&fighter.mv.co.itemscope_start.charge_frames,4)&&released==fighter.mv.co.itemscope_start.released);CHECK(count==trace_count&&!memcmp(trace,expected,count*sizeof(*trace)));canary();++pairs;}
    ++cases;
}
static void rapid(void)
{
    const uint32_t budgets[]={0,1,6,UINT32_MAX,UINT32_C(0x80000000)};
    for(unsigned b=0;b<5;++b)for(unsigned pressed=0;pressed<2;++pressed)for(unsigned air_mode=0;air_mode<2;++air_mode){setup();common.x5BC=budgets[b];fighter.input.pressed_buttons=pressed?HSD_PAD_A:0;int initial=123;memcpy((u8*)&fighter+0x2340,&initial,4);
        // The historical AirRapid loads a 32-bit-layout GObj user_data at44.
        _Alignas(16) u8 legacy_object[sizeof(HSD_GObj)]={0};Fighter* pointer=&fighter;memcpy(legacy_object+0x2C,&pointer,sizeof(pointer));
        if(air_mode)reference_ftCo_ItemScopeAirRapid_IASA((HSD_GObj*)legacy_object);else reference_ftCo_ItemScopeRapid_IASA(&actor);
        int expected;memcpy(&expected,(u8*)&fighter+0x2340,4);
        setup();common.x5BC=budgets[b];fighter.input.pressed_buttons=pressed?HSD_PAD_A:0;fighter.mv.co.itemscope_rapid.remaining_cycles=initial;before=fighter;
        if(air_mode)ftCo_ItemScopeAirRapid_IASA(&actor);else ftCo_ItemScopeRapid_IASA(&actor);
        CHECK(fighter.mv.co.itemscope_rapid.remaining_cycles==expected);canary();++pairs;
    }
    ++cases;
    for(unsigned air_mode=0;air_mode<2;++air_mode)for(unsigned e=0;e<2;++e)for(int n=1;n<4;++n){setup();empty=e!=0;fighter.mv.co.common.x0=n;
        if(air_mode)reference_ftCo_ItemScopeAirRapid_Anim(&actor);else reference_ftCo_ItemScopeRapid_Anim(&actor);
        int expected_count=fighter.mv.co.common.x0;unsigned expected[32],count=trace_count;memcpy(expected,trace,sizeof(expected));
        setup();empty=e!=0;fighter.mv.co.itemscope_rapid.remaining_cycles=n;before=fighter;
        if(air_mode)ftCo_ItemScopeAirRapid_Anim(&actor);else ftCo_ItemScopeRapid_Anim(&actor);
        CHECK(fighter.mv.co.itemscope_rapid.remaining_cycles==expected_count);CHECK(trace_count==count&&!memcmp(trace,expected,count*sizeof(*trace)));canary();++pairs;
    }
    ++cases;
}
static void entries(void)
{
    for(unsigned air_mode=0;air_mode<2;++air_mode)for(unsigned is_empty=0;is_empty<2;++is_empty)for(int level=0;level<10;++level){
        setup();fighter.motion_id=is_empty?ftCo_MS_ItemScopeStartEmpty:ftCo_MS_ItemScopeStart;
        if(air_mode)reference_fn_800D81D0(&actor,level);else reference_fn_800D8140(&actor,level);
        unsigned expected[32],count=trace_count;memcpy(expected,trace,sizeof(expected));int charge=fighter.mv.co.common.x0;
        setup();fighter.motion_id=is_empty?ftCo_MS_ItemScopeStartEmpty:ftCo_MS_ItemScopeStart;before=fighter;
        if(air_mode)fn_800D81D0(&actor,level);else fn_800D8140(&actor,level);
        CHECK(fighter.mv.co.itemscope_fire.charge_level==charge&&trace_count==count&&!memcmp(trace,expected,count*sizeof(*trace)));canary();++pairs;
        trace_count=0;fn_800D86B8(&actor);CHECK(trace_count==1&&trace[0]==300+(unsigned)level);canary();
    }
    ++cases;
    for(unsigned air_mode=0;air_mode<2;++air_mode){setup();if(air_mode)reference_fn_800D7C60(&actor);else reference_fn_800D7BDC(&actor);unsigned expected[32],count=trace_count;memcpy(expected,trace,sizeof(expected));int remaining=fighter.mv.co.common.x0;
        setup();before=fighter;if(air_mode)fn_800D7C60(&actor);else fn_800D7BDC(&actor);CHECK(fighter.mv.co.itemscope_rapid.remaining_cycles==remaining&&trace_count==count&&!memcmp(trace,expected,count*sizeof(*trace)));canary();++pairs;}
    ++cases;
}
int main(void)
{CHECK(sizeof(void*)==8&&offsetof(Fighter,mv)!=0x2340&&offsetof(HSD_GObj,user_data)!=0x2C);charging();rapid();entries();printf("{\"status\":\"PASS typed Scope layout\",\"checks\":%u,\"native_pairs\":%u,\"cases\":%u}\n",checks,pairs,cases);return 0;}
