#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "article_returns.inc"

static HSD_GObj source, recipient, foreign;
static Item* item;
static Fighter fighter;
static unsigned checks, calls, guards, mode;
static bool item_live, fighter_live, binding;
static const uint32_t item_generation=UINT32_C(0x80000001), fighter_generation=UINT32_C(0xfedcba98);
#define CHECK(x) do { ++checks; assert(x); } while(0)
static bool alive(HSD_GObj* object,uint32_t generation,uint32_t cls)
{
    ++guards;
    if(cls==NativeCallbackSource_Item) {
        CHECK(object==&source && generation==item_generation);
        return item_live && (uint32_t)item->x1C==generation;
    }
    CHECK(cls==NativeCallbackSource_Fighter && object==&recipient && generation==fighter_generation);
    return fighter_live && (uint32_t)fighter.x8_spawnNum==generation;
}
static Fighter_GObj* creator(Item_GObj* object)
{
    CHECK(object==&source && item_live);
    return binding && fighter_live && (uint32_t)fighter.x8_spawnNum==fighter_generation &&
        fighter.kind==Ft_Kind_CustomFirst ? &recipient : NULL;
}
static void boundary(void)
{
    ++calls;
    if(mode==1) {
        item_live=false; DWORD old;
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_NOACCESS,&old));
    } else if(mode==2) ++item->x1C;
    else if(mode==3) fighter_live=false;
    else if(mode==4) ++fighter.x8_spawnNum;
    else if(mode==5) fighter.kind=Ft_Kind_Mario;
    else if(mode==6) binding=false;
    else if(mode==7) item->owner=&foreign;
}
static int32_t dispatch_int(uint32_t id,uint32_t hash)
{ CHECK(id==item_generation || id==fighter_generation); (void)hash; boundary(); return 16777217; }
static float dispatch_float(uint32_t id,uint32_t hash)
{ dispatch_int(id,hash); return -1.25f; }
static bool dispatch_bool(uint32_t id,uint32_t hash)
{ dispatch_int(id,hash); return true; }
static void remove_item(Item_GObj* object) { CHECK(object==&source); boundary(); }
static void reset(unsigned next)
{
    DWORD old; CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
    memset(item,0,sizeof(*item)); memset(&fighter,0,sizeof(fighter));
    source.user_data=item; item->entity=&source; item->x1C=item_generation;
    recipient.user_data=&fighter; fighter.x8_spawnNum=fighter_generation; fighter.kind=Ft_Kind_CustomFirst;
    item->owner=&recipient; item_live=fighter_live=binding=true;
    calls=guards=0; mode=next;
}
static unsigned offset(const NativeFighterArticleLogic* article,const char* name)
{
    for(unsigned i=0;i<article->state_field_count;++i)
        if(!strcmp(article->state_fields[i].name,name)) return article->state_fields[i].offset;
    abort();
}
static int32_t integer(unsigned at) { int32_t v; memcpy(&v,item->xDD4_itemVar.native_article.state+at,4); return v; }
static float real(unsigned at) { float v; memcpy(&v,item->xDD4_itemVar.native_article.state+at,4); return v; }
static bool flag(unsigned at) { return item->xDD4_itemVar.native_article.state[at]!=0; }
static void write_int(unsigned at,int32_t value) { memcpy(item->xDD4_itemVar.native_article.state+at,&value,4); }
static int32_t received(const NativeFighterDefinition* definition)
{
    for(unsigned i=0;i<definition->state_field_count;++i)
        if(!strcmp(definition->state_fields[i].name,"received")) {
            int32_t value;
            memcpy(&value,fighter.native_module_state+definition->state_fields[i].offset,4);
            return value;
        }
    abort();
}
static void signal(const NativeFighterArticleLogic* article,const char* name)
{
    uint32_t hash=UINT32_C(2166136261);
    for(const char* p=name;*p;++p) { hash^=(unsigned char)*p; hash*=UINT32_C(16777619); }
    for(unsigned i=0;i<article->signal_count;++i)
        if(article->signals[i].name_hash==hash) { article->signals[i].callback(&source,0); return; }
    abort();
}
int main(void)
{
    item=VirtualAlloc(NULL,sizeof(*item),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE); assert(item);
    MotionState motion={0}; NativeFighterDefinition base={.fighter_root="ftDataNess",
        .motion_state_first=341,.motion_state_count=1,.motion_states=&motion};
    NativeFighterHost host={.callback_source_alive=alive,.item_creator=creator,
        .target_call_int=dispatch_int,.target_call_float=dispatch_float,
        .target_call_bool=dispatch_bool,.item_remove=remove_item};
    const NativeFighterDefinition* def=FreakMelee_GetFighterModule(&base,&host);
    CHECK(def && def->article_logic_count==2);
    NativeFighterHost missing=host; missing.callback_source_alive=NULL;
    CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    missing=host; missing.item_creator=NULL; CHECK(!FreakMelee_GetFighterModule(&base,&missing));
    CHECK(FreakMelee_GetFighterModule(&base,&host));
    const NativeFighterArticleLogic* article=&def->article_logics[0];
    unsigned value=offset(article,"value"), trace=offset(article,"trace"), amount=offset(article,"amount"), ready=offset(article,"ready");
#define WORD(at) integer(at)
    reset(0); article->on_created(&source);
    CHECK(WORD(value)==16777224 && WORD(trace)==12);
    CHECK(real(amount)==2.5f && flag(ready));
    CHECK(scripted_article_0_helper_0(&source,(int32_t)UINT32_C(0x80000001),-1)==INT32_MIN);
    const uint32_t ids[]={0,16777217,UINT32_C(0x80000001),UINT32_MAX};
    for(unsigned i=0;i<sizeof(ids)/sizeof(*ids);++i) {
        int32_t id=(int32_t)ids[i];
        CHECK(scripted_article_0_helper_0(&source,id,0)==id);
        CHECK(scripted_article_0_helper_13(&source,id)==(float)id);
    }
    CHECK(!scripted_article_0_helper_12(&source,0.0f));
    CHECK(!scripted_article_0_helper_12(&source,-0.0f));
    CHECK(scripted_article_0_helper_12(&source,0.25f));
    CHECK(scripted_article_0_helper_12(&source,-1.0f));
    reset(0); signal(article,"palette");
    CHECK(WORD(value)==(int32_t)UINT32_C(0x80000001));
    CHECK(real(amount)==1.25f && flag(ready));
    const char* eight_pure[]={"eight_creator","eight_owner"};
    const char* eight_nested[]={"eight_nested_creator","eight_nested_owner"};
    const char* eight_invalidating[]={"eight_invalidating_creator","eight_invalidating_owner"};
    for(unsigned i=0;i<2;++i) {
        reset(0); signal(article,eight_pure[i]);
        CHECK(WORD(value)==16777243 && received(def)==16777243 && WORD(trace)==0);
        reset(0); signal(article,eight_nested[i]);
        CHECK(received(def)==16777217 && WORD(trace)==1);
        reset(0); binding=false; signal(article,eight_nested[i]);
        CHECK(received(def)==0 && WORD(trace)==0);
        for(unsigned m=0;m<=7;++m) {
            reset(m); signal(article,eight_invalidating[i]);
            DWORD old; CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
            CHECK(calls==1);
            bool invoked=m==0 || (i==0 && m==7);
            CHECK(received(def)==(invoked ? (int32_t)(UINT32_C(16777217)+item_generation) : 0));
            CHECK(WORD(trace)==(m==1 || m==2 ? 0 : 100));
        }
    }
    reset(0); binding=false; signal(article,"absent"); CHECK(WORD(value)==0 && WORD(trace)==0 && calls==0);
    reset(0); item->owner=&foreign; signal(article,"owner"); CHECK(WORD(value)==0 && WORD(trace)==0);
    signal(article,"absent"); CHECK(WORD(value)==16777217 && WORD(trace)==1);
    for(unsigned m=0;m<=7;++m) {
        reset(m); write_int(value,91); signal(article,"local");
        DWORD old; CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
        CHECK(calls==1);
        CHECK(WORD(value)==(m==1 || m==2 ? 91 : (int32_t)(UINT32_C(16777217)+item_generation)));
        CHECK(WORD(trace)==(m==1 || m==2 ? 0 : 100));
        reset(m); write_int(value,91); signal(article,"recipient");
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old)); CHECK(calls==1);
        CHECK(WORD(value)==(m==1 || m==2 ? 91 : m>=3 && m<=6 ? 0 : 16777217));
        CHECK(WORD(trace)==(m==1 || m==2 ? 0 : 900));
        reset(m); write_int(value,91); signal(article,"arguments");
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old)); CHECK(calls==1);
        CHECK(WORD(value)==(m==1 || m==2 ? 91 : m>=3 && m<=6 ? 0 : 16777217));
        CHECK(WORD(trace)==(m==1 || m==2 ? 0 : 901));
    }
    for(unsigned m=0;m<=2;++m) {
        DWORD old; reset(m); CHECK(scripted_article_0_mutating_value_helper_6(&source)==(m?0.0f:-1.25f));
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
        reset(m); CHECK(scripted_article_0_mutating_value_helper_7(&source)==!m);
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
        reset(m); CHECK(scripted_article_0_mutating_value_helper_8(&source)==(m?0:5));
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
        reset(m); CHECK(scripted_article_0_mutating_value_helper_9(&source)==(m?0:(int32_t)(UINT32_C(16777218)+item_generation)));
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
        reset(m); CHECK(scripted_article_0_mutating_value_helper_10(&source)==0.0f);
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old));
        reset(m); scripted_article_0_mutating_helper_11(&source);
        CHECK(VirtualProtect(item,sizeof(*item),PAGE_READWRITE,&old)); CHECK(calls==1);
    }
    CHECK(VirtualFree(item,0,MEM_RELEASE));
    printf("article helper returns: %u checks\n",checks); return 0;
}
