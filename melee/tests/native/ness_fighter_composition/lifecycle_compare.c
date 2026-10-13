// Four live resources: original ordered cleanup versus the complete module's
// generated callbacks and real article signals. Allocation/registry are explicit
// fixture boundaries; resource release records original live node order.
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/kinds/ftNess/ftness.h>
#include <melee/ft/kinds/ftNess/ftnessattackhi4.h>
#include <melee/ft/kinds/ftNess/ftnessattacks4.h>
#include <melee/ft/kinds/ftNess/ftnessspecialn.h>
#include <melee/ft/kinds/ftNess/ftnessspecialhi.h>
#include <melee/ft/kinds/ftNess/types.h>
#include <melee/ft/ftparts.h>
#include <melee/it/kinds/itnessbat.h>
#include <melee/it/kinds/itnessyoyo.h>
#include <melee/it/kinds/itnesspkflash.h>
#include <melee/it/kinds/itnesspkthunderball.h>
#include <melee/it/kinds/itnesspkthundertrail.h>
#include <melee/it/it_26B1.h>
#include <melee/it/item.h>
#include <melee/ef/eflib.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, pairs, family, node_count;
static Fighter fighters[2];
static HSD_GObj owners[2], objects[2][10], nodes[2][90];
static Item items[2][10];
static ItemLink links[2][90];
static bool removed[2][10], released;
static const NativeFighterDefinition* definition;
enum { Flash, Thunder, Bat, Yoyo, Trail };
#define CHECK(condition) do { ++checks; if (!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); \
} } while (0)
typedef struct LifeEvent { unsigned operation, object, owner_mask; } LifeEvent;
static LifeEvent events[2][128];
static unsigned counts[2];
static void record(unsigned operation, unsigned object)
{
    unsigned mask = 0;
    for (unsigned i = 0; i < 10; ++i) if (items[family][i].owner) mask |= 1u<<i;
    CHECK(counts[family] < 128);
    events[family][counts[family]++] = (LifeEvent){operation,object,mask};
}
static unsigned item_index(HSD_GObj* object)
{
    for (unsigned i = 0; i < 10; ++i) if (object == &objects[family][i]) return i;
    CHECK(false); return 0;
}
void Item_8026A8EC(Item_GObj* object)
{ unsigned i = item_index(object); record(2,i); removed[family][i] = true; }
void HSD_GObjFree(HSD_GObj* object)
{
    for (unsigned i = 0; i < node_count; ++i)
        if (object == &nodes[family][i]) { record(1,i); return; }
    CHECK(false);
}
void efLib_DestroyAll(HSD_GObj* object)
{ CHECK(object == &owners[family]); record(3,0); }
void ftParts_80074A4C(HSD_GObj* object, int group, int value)
{ CHECK(object == &owners[family] && group == 0 && value == 0); record(5,0); }
static void rotate(Fighter* fighter, int part, float value)
{ CHECK(fighter == &fighters[family] && part == 0 && value == 0); record(4,0); }
#define ftPartSetRotX rotate
#define GET_FIGHTER(object) ((Fighter*)(object)->user_data)
#define GET_ITEM(object) ((Item*)(object)->user_data)
#include "lifecycle_original.h"
#undef ftPartSetRotX
static void stock_slot(HSD_GObj* gobj) { (void)gobj; }
#include "base.h"
#include "behavior.generated.c"
static void unexpected(void) { CHECK(false); }
static bool alive(HSD_GObj* object, uint32_t generation, uint32_t kind)
{
    if (kind == NativeCallbackSource_Fighter)
        return object == &owners[1] && object->user_data == &fighters[1] &&
               generation == (uint32_t)fighters[1].x8_spawnNum;
    for (unsigned i = 0; i < 10; ++i)
        if (object == &objects[1][i])
            return !removed[1][i] && object->user_data == &items[1][i] &&
                   generation == items[1][i].x1C;
    return false;
}
static const NativeFighterArticleLogic* logic(unsigned index)
{
    ItemKind kind = index == Flash ? It_Kind_Ness_PKFlush : index == Thunder ?
        It_Kind_Ness_PKThunder : index == Bat ? It_Kind_Ness_Bat : index == Yoyo ?
        It_Kind_Ness_Yoyo : It_Kind_Ness_PKThunder1;
    for (unsigned i = 0; i < definition->article_logic_count; ++i)
        if (definition->article_logics[i].kind == kind) return &definition->article_logics[i];
    CHECK(false); return NULL;
}
static unsigned field(const char* name)
{
    for (unsigned i = 0; i < definition->state_field_count; ++i)
        if (!strcmp(name, definition->state_fields[i].name)) return definition->state_fields[i].offset;
    CHECK(false); return 0;
}
static void put(const char* name, int32_t value)
{ memcpy(fighters[1].native_module_state + field(name), &value, 4); }
static int32_t get(const char* name)
{ int32_t value; memcpy(&value,fighters[1].native_module_state+field(name),4);return value; }
static uint32_t current_owner(Item_GObj* object)
{ return ((Item*)object->user_data)->owner ? (uint32_t)fighters[1].x8_spawnNum : UINT32_MAX; }
static bool call_target(uint32_t target, uint32_t name)
{
    CHECK(target == (uint32_t)fighters[1].x8_spawnNum);
    for (unsigned i = 0; i < definition->callback_export_count; ++i)
        if (definition->callback_exports[i].name_hash == name) {
            definition->callback_exports[i].callback(&owners[1]); return true;
        }
    CHECK(false); return false;
}
static bool signal(HSD_GObj* source, uint32_t handle, uint32_t name, float value)
{
    CHECK(source == &owners[1] || source == &objects[1][Yoyo] || source == &objects[1][Thunder]);
    for (unsigned i = 0; i < 10; ++i) if (items[1][i].x1C == handle && !removed[1][i]) {
        const NativeFighterArticleLogic* item_logic = logic(i);
        for (unsigned s = 0; s < item_logic->signal_count; ++s)
            if (item_logic->signals[s].name_hash == name) {
                item_logic->signals[s].callback(&objects[1][i], value); return true;
            }
    }
    return false;
}
static void release(Item_GObj* object)
{
    CHECK(object == &objects[1][Yoyo]);
    if (!released) for (unsigned i = node_count; i > 0; --i) HSD_GObjFree(&nodes[1][i-1]);
    released = true;
}
static void initialize(unsigned mask, int motion, unsigned reflected, unsigned pause)
{
    memset(fighters,0,sizeof(fighters)); memset(items,0,sizeof(items));
    memset(removed,0,sizeof(removed)); memset(counts,0,sizeof(counts)); released = false;
    for (unsigned f = 0; f < 2; ++f) {
        owners[f] = (HSD_GObj){.user_data=&fighters[f],.classifier=HSD_GOBJ_CLASS_FIGHTER};
        fighters[f].gobj = &owners[f]; fighters[f].x8_spawnNum = 0x80000099u;
        fighters[f].motion_id = motion; fighters[f].u.ns.pkthunder_gfx = true;
        for (unsigned i = 0; i < 10; ++i) {
            objects[f][i]=(HSD_GObj){.user_data=&items[f][i],.classifier=HSD_GOBJ_CLASS_ITEM};
            items[f][i].entity=&objects[f][i]; items[f][i].x1C=0x80000010u+i;
            items[f][i].owner=&owners[f]; items[f][i].xDC8_word.flags.x13=true;
            items[f][i].xDD4_itemVar.native_article.attachment_owner=&owners[f];
            items[f][i].xDD4_itemVar.native_article.creator_spawn_id=fighters[f].x8_spawnNum;
        }
        items[f][Yoyo].xDC8_word.flags.x3=pause&1;
        items[f][Yoyo].xDC8_word.flags.x7=(pause>>1)&1;
        for (unsigned i = 0; i < node_count; ++i) {
            links[f][i].gobj=&nodes[f][i]; links[f][i].next=i?&links[f][i-1]:NULL;
        }
    }
    fighters[0].u.ns.pkflash_gobj=mask&1?&objects[0][Flash]:NULL;
    fighters[0].u.ns.pkthunder_gobj=mask&2?&objects[0][Thunder]:NULL;
    fighters[0].u.ns.bat_gobj=mask&4?&objects[0][Bat]:NULL;
    fighters[0].u.ns.yoyo_gobj=mask&8?&objects[0][Yoyo]:NULL;
    items[0][Bat].xDD4_itemVar.nessbat.x0=&owners[0];
    items[0][Yoyo].xDD4_itemVar.nessyoyo.x8=node_count?&links[0][node_count-1]:NULL;
    items[0][Thunder].xDD4_itemVar.pkthunder.xF00=reflected;
    for (unsigned i = 0; i < 6; ++i) items[0][Thunder].xDD4_itemVar.pkthunder.xDD4[i]=&objects[0][Trail+i];
    put("flash_handle",mask&1?items[1][Flash].x1C:-1);
    put("thunder_ball_handle",mask&2?items[1][Thunder].x1C:-1);
    put("bat_handle",mask&4?items[1][Bat].x1C:-1);
    put("yoyo_handle",mask&8?items[1][Yoyo].x1C:-1);
    fighters[1].native_module_state[field("thunder_effects_active")]=true;
    const NativeFighterArticleLogic* ball=logic(Thunder);
    for (unsigned i=0;i<ball->state_field_count;++i) {
        const NativeFighterStateField* state=&ball->state_fields[i];
        if (!strcmp(state->name,"reflected")) items[1][Thunder].xDD4_itemVar.native_article.state[state->offset]=reflected;
        if (!strcmp(state->name,"trail_handles")) for(unsigned j=0;j<6;++j)
            memcpy(items[1][Thunder].xDD4_itemVar.native_article.state+state->offset+4*j,&items[1][Trail+j].x1C,4);
    }
}
static void compare(void)
{
    CHECK(!fighters[0].u.ns.pkflash_gobj && get("flash_handle")==-1);
    CHECK(!fighters[0].u.ns.pkthunder_gobj && get("thunder_ball_handle")==-1);
    CHECK(!fighters[0].u.ns.bat_gobj && get("bat_handle")==-1);
    CHECK(!fighters[0].u.ns.yoyo_gobj && get("yoyo_handle")==-1);
    CHECK(fighters[0].u.ns.pkthunder_gfx==fighters[1].native_module_state[field("thunder_effects_active")]);
    CHECK(!fighters[0].take_dmg_cb && !fighters[1].take_dmg_cb && !fighters[0].death2_cb && !fighters[1].death2_cb);
    for (unsigned i=0;i<10;++i) {
        CHECK((items[0][i].owner!=NULL)==(items[1][i].owner!=NULL));
        CHECK(items[0][i].xDC8_word.flags.x13==items[1][i].xDC8_word.flags.x13);
        CHECK(removed[0][i]==removed[1][i]);
    }
    CHECK(items[0][Yoyo].xDC8_word.flags.x3==items[1][Yoyo].xDC8_word.flags.x3);
    CHECK(items[0][Yoyo].xDC8_word.flags.x5==items[1][Yoyo].xDC8_word.flags.x5);
    CHECK(counts[0]==counts[1] && !memcmp(events[0],events[1],counts[0]*sizeof(LifeEvent)));
}
static void sentinel(HSD_GObj* object) { (void)object; }
static void compare_reset(void)
{
    for (unsigned mask=0;mask<16;++mask) {
        initialize(mask,361,0,3);
        for(unsigned f=0;f<2;++f) {
            fighters[f].death2_cb=sentinel; fighters[f].take_dmg_cb=sentinel;
        }
        family=0; ftNs_Init_OnDeath(&owners[0]);
        family=1; definition->on_death(&owners[1]);
        CHECK(get("flash_handle")==-1 && get("thunder_ball_handle")==-1 &&
              get("bat_handle")==-1 && get("yoyo_handle")==-1);
        CHECK(!fighters[0].u.ns.pkflash_gobj && !fighters[0].u.ns.pkthunder_gobj &&
              !fighters[0].u.ns.bat_gobj && !fighters[0].u.ns.yoyo_gobj);
        CHECK(!fighters[0].u.ns.pkthunder_gfx &&
              !fighters[1].native_module_state[field("thunder_effects_active")]);
        CHECK(counts[0]==1 && counts[1]==1 && !memcmp(events[0],events[1],sizeof(LifeEvent)));
        for(unsigned f=0;f<2;++f) {
            CHECK(fighters[f].death2_cb==sentinel && fighters[f].take_dmg_cb==sentinel);
            for(unsigned i=0;i<10;++i) CHECK(!removed[f][i]);
        }
        ++pairs;
    }
}
int main(int argc, char** argv)
{
    CHECK(argc==2);
    NativeFighterHost host={0};
#include "required_host.h"
    host.callback_source_alive=alive; host.signal_article=signal;
    host.item_current_owner=current_owner; host.target_call=call_target;
    host.item_pause=it_8026B724; host.item_resume=it_8026B73C;
    host.item_remove=Item_8026A8EC; host.article_resource_release=release;
    host.destroy_effects=efLib_DestroyAll; host.set_root_rotation_x=rotate;
    host.set_visibility_group=ftParts_80074A4C;
    definition=FreakMelee_GetFighterModule(&ftNessClone_Definition,&host); CHECK(definition);
    FILE* file=fopen(argv[1],"rb"); CHECK(file);
    char text[NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY+1];
    size_t length=fread(text,1,sizeof(text)-1,file); CHECK(!ferror(file)&&feof(file));fclose(file);
    NativeFighterParameters parameters; CHECK(nativeFighterParameters_Parse(text,length,&parameters,NULL));
    uint32_t words[NATIVE_FIGHTER_PARAMETER_CAPACITY];
    for(unsigned i=0;i<parameters.count;++i) words[i]=parameters.fields[i].bits;
    CHECK(FreakMelee_SetFighterParameters(words,parameters.count,parameters.schema_hash));
    static const unsigned node_counts[]={0,20,90};
    for(unsigned n=0;n<3;++n) for(unsigned mask=0;mask<16;++mask)
        for(int m=357;m<=367;++m) for(unsigned r=0;r<2;++r) for(unsigned p=0;p<4;++p) {
            for(unsigned callback=0;callback<2;++callback) {
                node_count=node_counts[n]; initialize(mask,m,r,p);
                family=0; ftNs_Init_OnDamage(&owners[0]);
                family=1;
                if(callback) authored_death2(&owners[1]); else authored_damage(&owners[1]);
                compare(); ++pairs;
            }
        }
    compare_reset();
    printf("Ness composed lifecycle: %u checks, 0 failures, %u source pairs\n",checks,pairs);
    return 0;
}
