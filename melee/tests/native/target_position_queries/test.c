#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/native_callback_lifetime.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/ft/ftlib.h>
#include <melee/it/native_article_refs.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define GET_FIGHTER(gobj) ((Fighter*)(gobj)->user_data)

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static Fighter fighters[5];
static HSD_GObj objects[5];
static Item items[2];
static HSD_GObj item_objects[2];
static HSD_JObj joints[2];
static ItemLogicTable logic, wrong_logic;
static NativeFighterArticleLogic article = {
    .kind = It_Kind_Zelda_DinFire, .logic = &logic,
};
static NativeFighterDefinition definition = {
    .article_logic_count = 1, .article_logics = &article,
};
static NativeFighterDefinition other_definition;
static ftData data, other_data;
static unsigned checks, pairs, translation_cases, invalidation_cases;
static int mode, special[5], special_result[5];
static unsigned events[128], event_count;
static HSD_GObj* mutation_object;
static unsigned mutation, mutation_event;
static bool changed_definition;
static unsigned definition_calls, mutation_definition_call;
static void* protected_data;
static size_t protected_size;

#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"Query check failed line%d: %s\n",__LINE__,#x); abort(); } } while(0)

static void mutate(void)
{
    if (!mutation || mutation_object == NULL) return;
    HSD_GObj* object = mutation_object;
    unsigned selected = mutation;
    mutation = 0;
    if (selected == 1) {
        void* retired_data = object->user_data;
        unsigned link = object == &item_objects[0] || object == &item_objects[1]
            ? HSD_GOBJ_PLINK_ITEM : HSD_GOBJ_PLINK_FIGHTER;
        HSD_GObj** cursor = &heads[link];
        while (*cursor && *cursor != object) cursor = &(*cursor)->next;
        if (*cursor == object) *cursor = object->next;
        object->user_data = (void*)1;
        if (retired_data == protected_data) {
            DWORD old;
            CHECK(VirtualProtect(protected_data,protected_size,PAGE_NOACCESS,&old));
        }
    } else if (selected == 2) {
        if (object == &item_objects[0] || object == &item_objects[1])
            ++((Item*)object->user_data)->x1C;
        else ++((Fighter*)object->user_data)->x8_spawnNum;
    } else if (selected == 3) {
        if (object == &item_objects[0] || object == &item_objects[1])
            ((Item*)object->user_data)->xB8_itemLogicTable = &wrong_logic;
        else ++((Fighter*)object->user_data)->kind;
    } else if (selected == 4) changed_definition = true;
    else if (selected == 5) ((Fighter*)object->user_data)->ft_data = &other_data;
    else if (selected == 6) object->hsd_obj=&joints[0];
    else if (selected == 7) ((Item*)object->user_data)->native_removal_in_progress=true;
}

static void event(unsigned value)
{
    CHECK(event_count < 128);
    events[event_count++] = value;
    if (mutation_event == value) mutate();
}

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    ++definition_calls;
    if (mutation_event == 99 && definition_calls == mutation_definition_call) mutate();
    if (kind == Ft_Kind_CustomFirst)
        return changed_definition ? &other_definition : &definition;
    return kind == Ft_Kind_CustomFirst+1 ? &other_definition : NULL;
}

bool nativeModelEffect_Snapshot(HSD_GObj* gobj, uint32_t* generation)
{ (void)gobj; (void)generation; abort(); }
void* nativeModelEffect_Resolve(HSD_GObj* gobj, uint32_t generation)
{ (void)gobj; (void)generation; abort(); }
void __assert(char* file, u32 line, char* expression)
{ fprintf(stderr,"HSD assert %s:%d:%s\n",file,line,expression); abort(); }

bool gm_8016B168(void) { event(10); return mode == 1; }
bool gm_8016B14C(void) { event(11); return mode == 2; }
float Stage_GetBlastZoneLeftOffset(void) { event(1); return -100; }
float Stage_GetBlastZoneRightOffset(void) { event(2); return 100; }
float Stage_GetBlastZoneBottomOffset(void) { event(3); return -100; }
float Stage_GetBlastZoneTopOffset(void) { event(4); return 100; }
bool ft_80087A18(Fighter_GObj* object)
{
    unsigned index = (unsigned)(object - objects);
    CHECK(index < 5); event(20+index); return special[index] != 0;
}
int ft_80087A80(Fighter_GObj* object)
{ unsigned index = (unsigned)(object - objects); event(30+index); return special_result[index]; }
int ftLib_GetMotionId(HSD_GObj* object)
{ return ((Fighter*)object->user_data)->motion_id; }

#include "actual_bodies.h"

static void setup(void)
{
    memset(heads,0,sizeof(heads)); memset(fighters,0,sizeof(fighters));
    memset(objects,0,sizeof(objects)); memset(items,0,sizeof(items));
    memset(item_objects,0,sizeof(item_objects)); memset(joints,0,sizeof(joints));
    memset(special,0,sizeof(special)); memset(special_result,0,sizeof(special_result));
    mutation=mutation_event=event_count=0;mutation_object=NULL;changed_definition=false;mode=0;
    definition_calls=mutation_definition_call=0;protected_data=NULL;protected_size=0;
    HSD_GObjPLinkHead=heads;
    const uint32_t ids[] = {7,UINT32_C(16777217),UINT32_C(0x80000001),UINT32_C(0xfffffffe),19};
    for(unsigned i=0;i<5;++i) {
        fighters[i].x8_spawnNum=(s32)ids[i]; fighters[i].gobj=&objects[i];
        fighters[i].kind=Ft_Kind_CustomFirst;fighters[i].ft_data=&data;
        fighters[i].player_id=(s8)i;fighters[i].team=(s8)i;
        fighters[i].motion_id=ftCo_MS_Wait;
        objects[i].user_data=&fighters[i];objects[i].next=i<4?&objects[i+1]:NULL;
        fighters[i].cur_pos=(Vec3){(float)i*10,5,(float)i};
    }
    heads[HSD_GOBJ_PLINK_FIGHTER]=objects;
    for(unsigned i=0;i<2;++i) {
        items[i].entity=&item_objects[i];items[i].x1C=(s32)(UINT32_C(0x80000001)+i);
        items[i].kind=article.kind;items[i].native_article_author_kind=Ft_Kind_CustomFirst;
        items[i].xB8_itemLogicTable=&logic;items[i].owner=&objects[0];
        item_objects[i].user_data=&items[i];item_objects[i].hsd_obj=&joints[i];
        item_objects[i].next=i==0?&item_objects[1]:NULL;
        joints[i].translate=(Vec3){1.25f+(float)i,2.5f,-19.5f};
        items[i].pos=(Vec3){900,800,700};
    }
    heads[HSD_GOBJ_PLINK_ITEM]=item_objects;
}

static void nearest_matrix(void)
{
    const float values[]={-101,-100,-99,0,99,100,101};
    for(unsigned variation=0;variation<96;++variation) for(unsigned origin=0;origin<7;++origin)
    for(unsigned game=0;game<3;++game) {
        setup();mode=(int)game;fighters[0].cpu.half_width=(float)(variation%3);
        fighters[0].cpu.half_height=(float)((variation/3)%3);
        for(unsigned i=1;i<5;++i) {
            Fighter* f=&fighters[i];unsigned v=(variation+i*7)%32;
            f->cur_pos=(Vec3){values[(variation+i)%7],values[(variation/7+i)%7],(float)(i*13)};
            if(v&1) f->team=0;
            if(v&2) f->player_id=0;
            if(v&4) f->x2219_b1=true;
            if(v&8) f->x2168=1;
            if(v&16) { special[i]=1;special_result[i]=(int)(variation%3);f->cpu.x7C=variation%2?121:120; }
            if(variation%11==0) f->motion_id=ftCo_MS_DeadDown;
        }
        Vec3 position={values[origin],12.25f,-3.5f};
        event_count=0;Fighter* expected=ftCo_800A4E8C(&fighters[0],&position);
        unsigned original_events[128], count=event_count;
        memcpy(original_events,events,sizeof(events));event_count=0;
        uint32_t result=UINT32_C(0x12345678);
        bool found=ftCo_CpuNearestEligibleTarget(&objects[0],&position,&result);
        CHECK(found==(expected!=NULL));
        CHECK(result==(expected?(u32)expected->x8_spawnNum:UINT32_C(0x12345678)));
        CHECK(count==event_count);CHECK(!memcmp(original_events,events,count*sizeof(*events)));
        ++pairs;
    }
    for(unsigned reversed=0;reversed<2;++reversed) {
        setup();fighters[1].cur_pos=(Vec3){-10,0,0};fighters[2].cur_pos=(Vec3){10,0,0};
        objects[2].next=NULL;if(reversed) {objects[0].next=&objects[2];objects[2].next=&objects[1];objects[1].next=NULL;}
        Vec3 origin={0};uint32_t selected=0;
        CHECK(ftCo_CpuNearestEligibleTarget(&objects[0],&origin,&selected));
        CHECK(selected==(u32)fighters[reversed?2:1].x8_spawnNum);++pairs;
    }
    // Candidate selected early then becomes invalid while the next candidate's
    // pure shared observation executes: final selected-ID guard must reject it.
    for(unsigned which=0;which<2;++which) for(unsigned change=1;change<=5;++change) {
        setup();fighters[1].cur_pos=(Vec3){1,0,0};fighters[2].cur_pos=(Vec3){2,0,0};
        mutation_object=&objects[which];mutation=change;mutation_event=22;
        Vec3 origin={0};uint32_t selected=UINT32_C(0x12345678);
        CHECK(!ftCo_CpuNearestEligibleTarget(&objects[0],&origin,&selected));
        CHECK(selected==UINT32_C(0x12345678));++invalidation_cases;
    }
    for(unsigned which=0;which<2;++which) {
        setup();fighters[1].cur_pos=(Vec3){1,0,0};fighters[2].cur_pos=(Vec3){2,0,0};
        protected_size=(sizeof(Fighter)+4095)&~(size_t)4095;
        protected_data=VirtualAlloc(NULL,protected_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        CHECK(protected_data);memcpy(protected_data,&fighters[which],sizeof(Fighter));
        objects[which].user_data=protected_data;
        mutation_object=&objects[which];mutation=1;mutation_event=22;
        Vec3 origin={0};uint32_t selected=UINT32_C(0x12345678);
        CHECK(!ftCo_CpuNearestEligibleTarget(&objects[0],&origin,&selected));
        CHECK(selected==UINT32_C(0x12345678));++invalidation_cases;
        CHECK(VirtualFree(protected_data,0,MEM_RELEASE));
    }
    setup();uint32_t selected=99;Vec3 origin={0};
    CHECK(!ftCo_CpuNearestEligibleTarget(NULL,&origin,&selected)&&selected==99);
    CHECK(!ftCo_CpuNearestEligibleTarget(&objects[0],NULL,&selected)&&selected==99);
    CHECK(!ftCo_CpuNearestEligibleTarget(&objects[0],&origin,NULL));
    heads[HSD_GOBJ_PLINK_FIGHTER]=&objects[1];objects[0].user_data=(void*)1;
    CHECK(!ftCo_CpuNearestEligibleTarget(&objects[0],&origin,&selected)&&selected==99);
}

static void translation_matrix(void)
{
    const uint32_t ids[]={0,16777217,UINT32_C(0x80000001),UINT32_C(0xfffffffe)};
    for(unsigned caller=0;caller<2;++caller) for(unsigned i=0;i<4;++i) {
        setup();items[0].x1C=300;items[1].x1C=(s32)ids[i];items[1].owner=&objects[4];
        HSD_GObj* source=caller?&item_objects[0]:&objects[0];Vec3 result={8,9,10};
        CHECK(nativeArticleRef_PrimaryModelTranslation(source,ids[i],article.kind,&result));
        CHECK(!memcmp(&result,&joints[1].translate,sizeof(result)));++translation_cases;
    }
    for(unsigned invalid=0;invalid<13;++invalid) {
        setup();HSD_GObj* source=&item_objects[0];uint32_t id=(u32)items[1].x1C;
        ItemKind kind=article.kind;Vec3 result={8,9,10},expected=result;
        switch(invalid) {
        case 0:id=UINT32_MAX;break;
        case 1:++id;break;
        case 2:kind=It_Kind_Zelda_DinFire_Explode;break;
        case 3:items[1].native_article_author_kind=Ft_Kind_CustomFirst+1;break;
        case 4:items[1].xB8_itemLogicTable=&wrong_logic;break;
        case 5:items[1].native_removal_in_progress=true;break;
        case 6:item_objects[1].hsd_obj=NULL;break;
        case 7:items[1].entity=(HSD_GObj*)1;break;
        case 8:items[0].xB8_itemLogicTable=&wrong_logic;break;
        case 9:items[0].native_removal_in_progress=true;break;
        case 10:heads[HSD_GOBJ_PLINK_ITEM]=&item_objects[1];item_objects[0].user_data=(void*)1;break;
        case 11:item_objects[0].next=NULL;item_objects[1].user_data=(void*)1;break;
        case 12:definition.article_logic_count=0;break;
        }
        CHECK(!nativeArticleRef_PrimaryModelTranslation(source,id,kind,&result));
        CHECK(!memcmp(&result,&expected,sizeof(result)));definition.article_logic_count=1;++translation_cases;
    }
    // Controlled Definition observation may invalidate the retained caller or
    // recipient after the vector was read. The real query must not publish it.
    for(unsigned caller=0;caller<2;++caller) for(unsigned which=0;which<2;++which)
    for(unsigned change=1;change<=7;++change) {
        if((change==5 && (caller || which)) || (change==6 && !which) ||
           (change>=6 && !which && !caller)) continue;
        setup();HSD_GObj* source=caller?&item_objects[0]:&objects[0];
        mutation_object=which?&item_objects[1]:source;mutation=change;
        mutation_event=99;mutation_definition_call=which?4:5;
        Vec3 result={8,9,10},expected=result;
        CHECK(!nativeArticleRef_PrimaryModelTranslation(source,(u32)items[1].x1C,article.kind,&result));
        CHECK(!memcmp(&result,&expected,sizeof(result)));++invalidation_cases;
    }
    for(unsigned caller=0;caller<2;++caller) for(unsigned which=0;which<2;++which) {
        setup();HSD_GObj* source=caller?&item_objects[0]:&objects[0];
        mutation_object=which?&item_objects[1]:source;
        size_t bytes=(!which&&!caller)?sizeof(Fighter):sizeof(Item);
        protected_size=(bytes+4095)&~(size_t)4095;
        protected_data=VirtualAlloc(NULL,protected_size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
        CHECK(protected_data);memcpy(protected_data,mutation_object->user_data,bytes);
        mutation_object->user_data=protected_data;
        mutation=1;mutation_event=99;mutation_definition_call=which?4:5;
        Vec3 result={8,9,10},expected=result;
        CHECK(!nativeArticleRef_PrimaryModelTranslation(source,(u32)items[1].x1C,article.kind,&result));
        CHECK(!memcmp(&result,&expected,sizeof(result)));++invalidation_cases;
        CHECK(VirtualFree(protected_data,0,MEM_RELEASE));
    }
    setup();Vec3 output={8,9,10};
    CHECK(!nativeArticleRef_PrimaryModelTranslation(&objects[0],(u32)items[1].x1C,article.kind,NULL));
    CHECK(!nativeArticleRef_PrimaryModelTranslation(NULL,(u32)items[1].x1C,article.kind,&output));
}

int main(void)
{
    nearest_matrix();translation_matrix();
    printf("{\"checks\":%u,\"nearest_pairs\":%u,\"translation_cases\":%u,\"invalidation_cases\":%u}\n",
        checks,pairs,translation_cases,invalidation_cases);
    return 0;
}
