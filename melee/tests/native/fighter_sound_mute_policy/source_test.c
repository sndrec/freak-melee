#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ft_0877.h>
#include <melee/ft/ft_0881.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/lb/lbaudio_ax.h>
#include <melee/lb/types.h>
#include <melee/lb/inlines.h>
#include <sysdolphin/baselib/random.h>

static unsigned checks,pairs,authored_cases;
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"FAIL %u %s\n",__LINE__,#x);exit(1);}}while(0)
// Actual registry/resolver bodies, with only slot publication controlled.
#include "capacity.inc"
static struct {bool active;FighterKind fighter_kind;const NativeFighterDefinition* definition;} slots[NATIVE_FIGHTER_ACTOR_CAPACITY];
#include "resolver.inc"

enum {Map,Random,PlayGlobal,PlayGroup,Pitch,PlayPositioned,PositionedHandle};
typedef struct SoundEvent {int type,a,b,c,d,e,f,g,h,i,j;float direction;} SoundEvent;
typedef struct Journal {SoundEvent events[32];unsigned count;} Journal;
static Journal journal;static int selected_random,next_handle,map_delta;
static Fighter* expected_fighter;static HSD_GObj positioned_object;
static SoundEvent* event(int type){CHECK(journal.count<32);SoundEvent* e=&journal.events[journal.count++];memset(e,0,sizeof(*e));e->type=type;return e;}
s32 ft_80087D0C(Fighter* fighter,s32 sound){CHECK(fighter==expected_fighter);event(Map)->a=sound;return sound+map_delta;}
s32 HSD_Randi(s32 bound){SoundEvent* e=event(Random);e->a=bound;int result=bound==200?137:selected_random;CHECK(result>=0&&result<bound);e->b=result;return result;}
int lbAudioAx_800237A8(enum_t sound,int vol,int pan){SoundEvent* e=event(PlayGlobal);e->a=sound;e->b=vol;e->c=pan;return ++next_handle;}
int lbAudioAx_80023870(int sound,int vol,int pan,int group){SoundEvent* e=event(PlayGroup);e->a=sound;e->b=vol;e->c=pan;e->d=group;return ++next_handle;}
int lbAudioAx_80024B94(int handle,int pitch){SoundEvent* e=event(Pitch);e->a=handle;e->b=pitch;return 1;}
HSD_GObj* lbAudioAx_800263E8(float direction,HSD_GObj* owner,int behavior,int sound,int vol,int pan,int p0,int p1,int p2,int group,int p3){
    CHECK(owner&&owner->user_data==expected_fighter);SoundEvent* e=event(PlayPositioned);e->direction=direction;e->a=behavior;e->b=sound;e->c=vol;e->d=pan;e->e=p0;e->f=p1;e->g=p2;e->h=group;e->i=p3;return &positioned_object;
}
int lbAudioAx_800264E4(HSD_GObj* object){CHECK(object==&positioned_object);event(PositionedHandle);return ++next_handle;}
#include "original.inc"
#include "candidate.inc"

typedef void (*Entry)(Fighter*,int,u8,u8);
static Entry original_entries[]={original_ft_PlaySFX,original_ft_80088478,original_ft_800881D8,original_ft_80088510,original_ft_800885A8,original_ft_80088640,original_ft_80088328};
static Entry candidate_entries[]={ft_PlaySFX,ft_80088478,ft_800881D8,ft_80088510,ft_800885A8,ft_80088640,ft_80088328};
static int handle(Fighter* f,unsigned channel){switch(channel){case 0:return f->x2160;case 1:return f->x214C;case 2:return f->x2144;case 3:return f->x2150;case 4:return f->x2154;case 5:return f->x2158;default:return f->x2148;}}
static Fighter initial(FighterKind kind,bool muted,unsigned player,unsigned partner){Fighter f={0};f.kind=kind;f.x2225_b6=muted;f.player_id=player;f.x221F_b4=partner;f.facing_dir=partner?-1:1;f.x2160=111;f.x214C=112;f.x2144=113;f.x2150=114;f.x2154=115;f.x2158=116;f.x2148=117;return f;}
static void same_handles(Fighter* a,Fighter* b){for(unsigned i=0;i<7;++i)CHECK(handle(a,i)==handle(b,i));}
static void same_journal(Journal a,Journal b){CHECK(a.count==b.count);for(unsigned i=0;i<a.count;++i)CHECK(!memcmp(&a.events[i],&b.events[i],sizeof(SoundEvent)));}
static void reset(Fighter* f){expected_fighter=f;journal=(Journal){0};next_handle=500;map_delta=0;}
static void invoke(bool candidate,unsigned route,Fighter* f,unsigned channel,int sound,unsigned pitch,unsigned stop_behavior){
    HSD_GObj object={.user_data=f};union CmdUnion words[8]={0};CommandInfo command={.u=words};
    if(route==0){(candidate?candidate_entries[channel]:original_entries[channel])(f,sound,103,47);return;}
    if(route==1){words[0].stage_sfx_0.sfx_base=29;words[0].stage_sfx_0.x2_b0_7=channel;words[0].stage_sfx_0.pitch_select=pitch;words[1].stage_sfx_1.sfx_id=sound;words[2].stage_sfx_2.x2_b0_15=0x9876;words[3].stage_sfx_3.x2_b0_7=37;words[3].stage_sfx_3.x3_b0_7=89;
        (candidate?ftAction_80072320:original_ftAction_80072320)(&object,&command);CHECK(command.u==words+4);return;}
    if(route==2){words[0].pseudo_random_sfx_0.behavior=channel;words[0].pseudo_random_sfx_0.random_range=6;words[0].pseudo_random_sfx_0.volume=103;words[0].pseudo_random_sfx_0.panning=47;
        for(unsigned i=1;i<7;++i)words[i].pseudo_random_sfx_1.sfx_id=sound+(sound<0x83D60?(int)i-1:0);
        (candidate?ftAction_80071FC8:original_ftAction_80071FC8)(&object,&command);CHECK(command.u==words+7);return;}
    words[0].sound_effect_0.behavior=stop_behavior?stop_behavior:channel;words[1].sound_effect_1.sfx_id=sound;words[2].sound_effect_2.volume=103;words[2].sound_effect_2.panning=47;
    (candidate?ftAction_80071B50:original_ftAction_80071B50)(&object,&command);CHECK(command.u==words+3);
}
int main(void){
    NativeFighterDefinition definition={0};FighterKind custom=Ft_Kind_CustomFirst+5;
    for(unsigned kind=0;kind<Ft_Kind_StockCount;++kind){CHECK(nativeFighterRuntime_SoundMuteChannels(kind)==((kind==Ft_Kind_Samus||kind==Ft_Kind_GameWatch)?0:0x44));}
    CHECK(nativeFighterRuntime_SoundMuteChannels(custom)==0x44);
    slots[0].active=true;slots[0].fighter_kind=custom;slots[0].definition=&definition;
    // Complete stock paths compared twice: raw stock and its custom policy.
    for(unsigned kind=0;kind<Ft_Kind_StockCount;++kind)for(unsigned clone=0;clone<2;++clone)for(unsigned muted=0;muted<2;++muted)for(unsigned channel=0;channel<7;++channel)for(unsigned route=0;route<4;++route)for(unsigned sound_index=0;sound_index<5;++sound_index)for(unsigned partner=0;partner<2;++partner){
        int sound=(int[]){1001,0x83D60,0x83D61,332,371}[sound_index];selected_random=(int)((kind+channel+sound_index)%6);unsigned pitch=(kind+channel)%4;
        Fighter old=initial(kind,muted,kind%6,partner),current=initial(clone?custom:kind,muted,kind%6,partner);definition.sound_mute_channels=(kind==Ft_Kind_Samus||kind==Ft_Kind_GameWatch)?0:0x44;
        reset(&old);invoke(false,route,&old,channel,sound,pitch,0);Journal old_journal=journal;
        reset(&current);invoke(true,route,&current,channel,sound,pitch,0);same_handles(&old,&current);same_journal(old_journal,journal);++pairs;
    }
    // All masks have independent expected filtering. Allowed paths match the
    // old unmuted source; blocked paths retain only documented map/RNG work.
    for(unsigned mask=0;mask<128;++mask)for(unsigned muted=0;muted<2;++muted)for(unsigned channel=0;channel<7;++channel)for(unsigned route=0;route<4;++route)for(unsigned which=0;which<3;++which){
        int sound=(int[]){332,0x83D60,0x83D61}[which];selected_random=5;definition.sound_mute_channels=(uint8_t)mask;CHECK(nativeFighterRuntime_SoundMuteChannels(custom)==mask);
        Fighter old=initial(Ft_Kind_Samus,false,5,1),current=initial(custom,muted,5,1),before=current;
        reset(&old);invoke(false,route,&old,channel,sound,3,0);Journal expected=journal;
        bool blocked=muted&&(mask&(1u<<channel));
        if(blocked){expected=(Journal){0};for(unsigned i=0;i<journal.count;++i){SoundEvent e=journal.events[i];if((route==1&&e.type==Map)||((route!=1&&channel==0)&&(e.type==Map||e.type==Random))||(route==2&&e.type==Random&&e.a==6))expected.events[expected.count++]=e;}}
        reset(&current);invoke(true,route,&current,channel,sound,3,0);same_journal(expected,journal);same_handles(blocked?&before:&old,&current);++authored_cases;
    }
    // Explicit stop commands bypass every mute mask and preserve stop journals.
    for(unsigned mask=0;mask<128;++mask)for(unsigned stop=10;stop<=15;++stop){definition.sound_mute_channels=(uint8_t)mask;Fighter old=initial(Ft_Kind_Samus,true,3,1),current=initial(custom,true,3,1);
        reset(&old);invoke(false,3,&old,0,0,0,stop);Journal expected=journal;reset(&current);invoke(true,3,&current,0,0,0,stop);same_journal(expected,journal);same_handles(&old,&current);++authored_cases;}
    // Explicit mapping/RNG anchors, unrelated raw-kind source paths excluded.
    definition.sound_mute_channels=0;Fighter f=initial(custom,true,4,1);reset(&f);map_delta=20;candidate_entries[2](&f,1001,103,47);
    CHECK(journal.count==4&&journal.events[0].type==Map&&journal.events[1].a==0x83D61&&journal.events[1].d==0x27&&journal.events[2].d==0x33&&journal.events[3].a==1021);
    definition.sound_mute_channels=1;f=initial(custom,true,0,0);reset(&f);ft_PlaySFX(&f,332,103,47);CHECK(journal.count==2&&journal.events[0].type==Map&&journal.events[1].type==Random&&journal.events[1].a==200);CHECK(f.x2160==111);
    for(unsigned boundary=0;boundary<4;++boundary){int sound=(int[]){331,332,370,371}[boundary];f=initial(custom,true,0,0);reset(&f);ft_PlaySFX(&f,sound,103,47);CHECK(journal.count==((sound>=332&&sound<=370)?2:1));CHECK(f.x2160==111);}
    f=initial(custom,true,0,0);reset(&f);map_delta=1;ft_PlaySFX(&f,331,103,47);CHECK(journal.count==2&&journal.events[1].type==Random&&journal.events[1].a==200);
    for(unsigned random=0;random<6;++random){f=initial(custom,false,0,0);reset(&f);selected_random=random;invoke(true,2,&f,1,1001,0,0);CHECK(journal.events[0].type==Random&&journal.events[1].type==Map&&journal.events[1].a==1001+(int)random);}
    slots[0].active=false;CHECK(nativeFighterRuntime_SoundMuteChannels(custom)==0x44);slots[0].active=true;slots[0].definition=NULL;CHECK(nativeFighterRuntime_SoundMuteChannels(custom)==0x44);
    printf("PASS %u raw/custom sound source pairs, %u authored mask/stop cases, %u checks; actual dispatch/stop/RNG/cursor and registry resolver\n",pairs,authored_cases,checks);return 0;
}
