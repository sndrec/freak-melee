#include <melee/ft/native_fighter_module.h>
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjobject.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef LIFECYCLE_DLL
#include <windows.h>
#else
#ifdef LIFECYCLE_BASE
#include "held_lifecycle_base_fixture.h"
#else
#include "held_lifecycle_fixture.h"
#endif
#endif
static unsigned checks,calls,invalidation;
#define CHECK(x) do { ++checks;assert(x); }while(0)
static HSD_GObj* source;
static bool notified;
static bool alive(HSD_GObj*g,uint32_t id,uint32_t type) {
    return g==source && g->user_data && type==NativeCallbackSource_Fighter && ((Fighter*)g->user_data)->x8_spawnNum==id;
}
static bool notify(Fighter_GObj*g,bool* claimed) {
    CHECK(g==source && claimed==NULL);++calls;
    if(invalidation==1)g->user_data=NULL;
    if(invalidation==2)((Fighter*)g->user_data)->x8_spawnNum++;
    return notified;
}
static unsigned field(const NativeFighterDefinition*d,const char*name) {
    for(unsigned i=0;i<d->state_field_count;++i)if(!strcmp(d->state_fields[i].name,name))return d->state_fields[i].offset;
    abort();
}
int main(int argc,char**argv) {
#ifdef LIFECYCLE_DLL
    CHECK(argc==2);HMODULE dll=LoadLibraryA(argv[1]);CHECK(dll);
    const NativeFighterDefinition*(*get_module)(const NativeFighterDefinition*,const NativeFighterHost*)=(void*)GetProcAddress(dll,"FreakMelee_GetFighterModule");CHECK(get_module);
#else
    (void)argc;(void)argv;
    const NativeFighterDefinition*(*get_module)(const NativeFighterDefinition*,const NativeFighterHost*)=FreakMelee_GetFighterModule;
#endif
    MotionState motion={0};NativeFighterDefinition base={.fighter_root="ftDataMario",.motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.notify_held_neutral=notify,.callback_source_alive=alive};
    NativeFighterHost bad=host;bad.notify_held_neutral=NULL;CHECK(!get_module(&base,&bad));
    bad=host;bad.callback_source_alive=NULL;CHECK(!get_module(&base,&bad));
    const NativeFighterDefinition*d=get_module(&base,&host);CHECK(d&&d->on_neutral_enter&&d->on_air_resume_motion&&d->on_held_timer_expired&&d->on_landing_action);
    CHECK(d->article_logic_count==2);
    CHECK(d->article_logics[0].on_holder_neutral && d->article_logics[1].on_holder_neutral);
    CHECK(!d->article_logics[0].on_held_status&&!d->article_logics[0].on_held_request);
    Fighter*f=calloc(1,sizeof(*f));CHECK(f);HSD_GObj object={.user_data=f};source=&object;
    const float blends[]={-1.0f,0.0f,1.5f,10.0f};
    for(unsigned b=0;b<4;++b)for(unsigned result=0;result<2;++result)for(invalidation=0;invalidation<3;++invalidation) {
        object.user_data=f;f->x8_spawnNum=UINT32_C(0xFEDCBA98);notified=result;
        unsigned off=field(d,"continued");f->native_module_state[off]=77;
        d->on_neutral_enter(&object,blends[b]);float observed;memcpy(&observed,f->native_module_state+field(d,"blend_seen"),4);
        CHECK(!memcmp(&observed,&blends[b],4));CHECK(f->native_module_state[off]==(invalidation?77:result));
    }
    CHECK(calls==24);object.user_data=f;f->x8_spawnNum=UINT32_C(0xFEDCBA98);
    const int32_t previous[]={INT32_MIN,-1,0,35,370,INT32_MAX};
    for(unsigned i=0;i<6;++i) {CHECK(d->on_air_resume_motion(&object,previous[i])==(previous[i]==370?ftCo_MS_FallSpecial:previous[i]));int32_t observed;memcpy(&observed,f->native_module_state+field(d,"resumed"),4);CHECK(observed==previous[i]);}
    for(unsigned flag=0;flag<2;++flag) {f->mv.co.landing.allow_interrupt=flag;d->on_landing_action(&object);CHECK(f->native_module_state[field(d,"landed")]==flag);}
    f->x2221_b4=1;f->x2221_b5=0;CHECK(d->on_held_timer_expired(&object));CHECK(f->x2221_b5==1&&f->x2221_b4==1);
    Item*item=calloc(1,sizeof(*item));CHECK(item);HSD_GObj article={.user_data=item};item->x1C=UINT32_C(0x89ABCDEF);
    for(unsigned i=1;i<=8;++i) {d->article_logics[0].on_holder_neutral(&article);int32_t count;memcpy(&count,item->xDD4_itemVar.native_article.state,4);CHECK(count==(int32_t)i);}
    d->article_logics[1].on_holder_neutral(&article);int32_t count;memcpy(&count,item->xDD4_itemVar.native_article.state,4);CHECK(count==8);
    free(item);free(f);
#ifdef LIFECYCLE_DLL
    CHECK(FreeLibrary(dll));
#endif
    printf("Lifecycle module PASS %u checks\n",checks);return 0;
}
