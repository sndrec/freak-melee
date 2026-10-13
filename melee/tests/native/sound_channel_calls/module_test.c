#undef NDEBUG
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#ifndef SOUND_CHANNEL_MODULE
#define SOUND_CHANNEL_MODULE "sound_channel_action.inc"
#endif
#include SOUND_CHANNEL_MODULE
_Static_assert(sizeof(Fighter)<=65536-4096,"Fixture Fighter must fit protected allocation");

// Win32 allocation is only a fixture transition. Protect both object and user
// data after retirement so any generated cached-pointer continuation crashes.
__declspec(dllimport) void* __stdcall VirtualAlloc(void*,size_t,unsigned long,unsigned long);
__declspec(dllimport) int __stdcall VirtualProtect(void*,size_t,unsigned long,unsigned long*);
__declspec(dllimport) int __stdcall VirtualFree(void*,size_t,unsigned long);
static void* pages;
static HSD_GObj* object;
static Fighter* fighter;
static const NativeFighterDefinition* definition;
static unsigned checks,calls,argument_calls,stop_at,retirement_mode,last_channel;
static int last_id;static u8 last_volume,last_pan;
static bool live;
static uint32_t generation;
#define CHECK(v) do{++checks;assert(v);}while(0)

static void retire(void)
{
    if(retirement_mode==2){++fighter->x8_spawnNum;return;}
    live=false;unsigned long previous;CHECK(VirtualProtect(pages,65536,1,&previous));
}
static bool alive(HSD_GObj* source,uint32_t id,uint32_t source_class)
{
    CHECK(source==object&&id==generation&&source_class==NativeCallbackSource_Fighter);
    return live && fighter->x8_spawnNum==generation;
}
static void landing(HSD_GObj* source){CHECK(source==object);++argument_calls;if(argument_calls==stop_at)retire();}
static void channel(unsigned index,Fighter* source,int id,u8 volume,u8 pan)
{
    CHECK(source==fighter);++calls;last_channel=index;last_id=id;last_volume=volume;last_pan=pan;
    if(stop_at==5)retire();
}
#define CHANNEL(n) static void channel##n(Fighter*f,int id,u8 volume,u8 pan){channel(n,f,id,volume,pan);}
CHANNEL(0) CHANNEL(1) CHANNEL(2) CHANNEL(3) CHANNEL(4) CHANNEL(5) CHANNEL(6)
static const NativeFighterSoundChannel channels[]={channel0,channel1,channel2,channel3,channel4,channel5,channel6};
static unsigned offset(const char* name){for(unsigned i=0;i<definition->state_field_count;++i)if(!strcmp(name,definition->state_fields[i].name))return definition->state_fields[i].offset;assert(0);return 0;}
static void set(const char* name,int32_t v){memcpy(fighter->native_module_state+offset(name),&v,4);}
static int32_t get(const char* name){int32_t v;memcpy(&v,fighter->native_module_state+offset(name),4);return v;}
static void reset(int ch,int id,int volume,int pan,unsigned stop,unsigned mode)
{
    unsigned long previous;CHECK(VirtualProtect(pages,65536,4,&previous));memset(pages,0,65536);
    object=pages;fighter=(Fighter*)((unsigned char*)pages+4096);object->user_data=fighter;
    generation=UINT32_C(0x80000001);fighter->x8_spawnNum=generation;live=true;
    calls=argument_calls=0;stop_at=stop;retirement_mode=mode;set("channel",ch);set("sound_id",id);set("volume",volume);set("pan",pan);
}
static void invoke(void)
{
#if SOUND_CHANNEL_CALLBACKS
    definition->motion_states[0].phys_cb(object);
#else
    definition->on_load(object);
#endif
}
int main(void)
{
    pages=VirtualAlloc(NULL,65536,0x3000,4);CHECK(pages);
    MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataTest",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.sound_channels=channels,.callback_source_alive=alive,.enter_landing_basic=landing};
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
    NativeFighterHost invalid=host;invalid.sound_channels=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&invalid));
    NativeFighterSoundChannel partial[7];
    for(unsigned i=0;i<7;++i){memcpy(partial,channels,sizeof(partial));partial[i]=NULL;invalid=host;invalid.sound_channels=partial;CHECK(!FreakMelee_GetFighterModule(&base,&invalid));}
    invalid=host;invalid.callback_source_alive=NULL;CHECK(!FreakMelee_GetFighterModule(&base,&invalid));
    definition=FreakMelee_GetFighterModule(&base,&host);CHECK(definition);
    const int ids[]={-1,0,16777217,INT_MAX,INT_MIN};
    for(unsigned ch=0;ch<7;++ch)for(unsigned id=0;id<5;++id)for(unsigned v=0;v<3;++v){
        int volume=v==0?127:v==1?300:-1;reset(ch,ids[id],volume,-1,0,0);invoke();
        CHECK(calls==1&&last_channel==ch&&last_id==ids[id]);CHECK(last_volume==(u8)volume&&last_pan==255);CHECK(argument_calls==4&&get("order")==1234&&get("after")==99);
    }
    const int invalid_channels[]={-1,7,INT_MIN,INT_MAX};
    for(unsigned i=0;i<4;++i){reset(invalid_channels[i],-1,127,64,0,0);invoke();CHECK(!calls&&argument_calls==4&&get("order")==1234&&get("after")==99);}
    for(unsigned mode=1;mode<=2;++mode)for(unsigned stop=1;stop<=5;++stop){
        reset(6,-1,127,64,stop,mode);invoke();unsigned long previous;CHECK(VirtualProtect(pages,65536,4,&previous));
        const int prefixes[]={0,1,12,123,1234,1234};CHECK(get("order")==prefixes[stop]&&get("after")==0);
        CHECK(argument_calls==(stop<5?stop:4));CHECK(calls==(stop==5?1:0));
    }
    CHECK(VirtualFree(pages,0,0x8000));printf("PASS %u generated sound-channel checks, all7 channels, Int bounds/order/stop/narrowing and PAGE_NOACCESS/recycle guards\n",checks);return 0;
}
