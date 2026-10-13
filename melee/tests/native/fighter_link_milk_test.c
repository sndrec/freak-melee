// Unchanged stock Milk/query/HSD C versus actual compiler output; no game launch.
#include <melee/ft/ftcommon.h>
#include <melee/ft/inlines.h>
#include <melee/ft/ftlib.h>
#include <melee/ft/kinds/ftCLink/ftclink.h>
#include <melee/ft/kinds/ftLink/types.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/it/inlines.h>
#include <melee/it/kinds/inlines.h>
#include <melee/it/kinds/itclinkmilk.h>
#include <melee/it/native_article_model.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "milk_fixture.h"

typedef struct MilkEvent { int operation, state, flags; } MilkEvent;
typedef struct MilkTrace { MilkEvent events[8]; unsigned count; } MilkTrace;
enum { Change = 1, Advance, Remove, Attach, Interaction };
static unsigned active, comparisons, failures;
static Item items[2];
static Fighter fighters[2], transferred[2];
static HSD_GObj objects[2], creators[2], recipients[2];
static HSD_JObj roots[2], children[2], descendants[2], siblings[2];
static MilkTrace traces[2];
static SpawnItem spawns[2];
static bool create_fails;
static const NativeFighterArticleLogic* authored;
static unsigned milk_offset;

void __assert(char* file, u32 line, char* message)
{ fprintf(stderr, "%s:%u: %s\n", file, (unsigned)line, message); abort(); }

static void event(int operation, int state, int flags)
{
    MilkTrace* trace = &traces[active];
    if (trace->count == 8) abort();
    trace->events[trace->count++] = (MilkEvent){operation, state, flags};
}

void Item_80268E5C(Item_GObj* object, enum_t state, Item_StateChangeFlags flags)
{ event(Change, state, flags); ((Item*)object->user_data)->msid = state; }
void Item_802694CC(HSD_GObj* object)
{ (void)object; event(Advance, 0, 0); }
void Item_8026A8EC(Item_GObj* object)
{ (void)object; event(Remove, 0, 0); }
bool it_8026B894(Item_GObj* object, HSD_GObj* other)
{ (void)object; (void)other; event(Interaction, 0, 0); return false; }
Item_GObj* Item_80268B18(SpawnItem* spawn)
{ spawns[active] = *spawn; return create_fails ? NULL : &objects[active]; }
void Item_8026AB54(Item_GObj* object, HSD_GObj* parent, Fighter_Part bone)
{
    (void)parent;
    event(Attach, bone, 0);
    if (active == 0) it_2725_Logic80_PickedUp(object);
    else authored->logic->picked_up(object);
}

static Fighter_GObj* creator(Item_GObj* object)
{
    if (object != &objects[1]) return NULL;
    HSD_GObj* parent = items[1].xDD4_itemVar.native_article.attachment_owner;
    return parent == &creators[1] && parent->user_data != NULL ? parent : NULL;
}
static int milk_handle(void)
{ int32_t value; memcpy(&value, fighters[1].native_module_state + milk_offset, 4); return value; }
static void set_handle(bool present)
{
    fighters[0].u.lk.x18 = present ? &objects[0] : NULL;
    int32_t value = present ? 123 : -1;
    memcpy(fighters[1].native_module_state + milk_offset, &value, 4);
}

static void reset(unsigned state, int motion, bool reference, unsigned ownership,
                  int command, int creator_command, unsigned flags, float scale)
{
    memset(items, 0, sizeof(items)); memset(fighters, 0, sizeof(fighters));
    memset(transferred, 0, sizeof(transferred)); memset(traces, 0, sizeof(traces));
    memset(spawns, 0, sizeof(spawns));
    for (unsigned i = 0; i < 2; ++i) {
        roots[i] = (HSD_JObj){.child=&children[i], .flags=JOBJ_HIDDEN};
        children[i] = (HSD_JObj){.parent=&roots[i], .child=&descendants[i],
            .next=&siblings[i], .scale={2,3,4}, .flags=flags};
        descendants[i] = (HSD_JObj){.parent=&children[i], .flags=JOBJ_HIDDEN};
        siblings[i] = (HSD_JObj){.parent=&roots[i], .flags=JOBJ_HIDDEN};
        creators[i] = (HSD_GObj){.user_data=ownership==5 ? NULL : &fighters[i]};
        recipients[i] = (HSD_GObj){.user_data=&transferred[i]};
        objects[i] = (HSD_GObj){.user_data=&items[i], .hsd_obj=&roots[i]};
        items[i].entity = &objects[i]; items[i].kind = It_Kind_CLink_Milk;
        items[i].msid = state; items[i].facing_dir = 1;
        items[i].owner = ownership==1 ? &recipients[i] : ownership==2 || ownership==4 ? NULL : &creators[i];
        items[i].xDAC_itcmd_var0=command; items[i].xDB0_itcmd_var1=17;
        items[i].xDB4_itcmd_var2=23; items[i].xDB8_itcmd_var3=29;
        items[i].xDCC_flag.b3=true;
        fighters[i].motion_id=motion; fighters[i].cmd_vars[1]=creator_command;
        fighters[i].x34_scale.y=scale; fighters[i].co_attrs.model_scaling=1.75f;
        fighters[i].cur_pos=(Vec3){7,-3,2};
    }
    items[0].xDD4_itemVar.clinkmilk.x0 = ownership==3 || ownership==4 ? NULL : &creators[0];
    items[1].xDD4_itemVar.native_article.attachment_owner = ownership==3 || ownership==4 ? NULL : &creators[1];
    items[1].xDD4_itemVar.native_article.creator_spawn_id = 19;
    set_handle(reference);
}

static void compare(int stock_result, int script_result, const char* phase)
{
    ++comparisons;
    bool same = stock_result == script_result && traces[0].count == traces[1].count &&
        memcmp(traces[0].events,traces[1].events,traces[0].count*sizeof(MilkEvent)) == 0 &&
        items[0].msid==items[1].msid && items[0].xDAC_itcmd_var0==items[1].xDAC_itcmd_var0 &&
        items[0].xDB0_itcmd_var1==items[1].xDB0_itcmd_var1 &&
        items[0].xDB4_itcmd_var2==items[1].xDB4_itcmd_var2 &&
        items[0].xDB8_itcmd_var3==items[1].xDB8_itcmd_var3 &&
        items[0].xDCC_flag.b3==items[1].xDCC_flag.b3 &&
        (items[0].owner==NULL)==(items[1].owner==NULL) &&
        (items[0].xDD4_itemVar.clinkmilk.x0==NULL)==(items[1].xDD4_itemVar.native_article.attachment_owner==NULL) &&
        (items[1].xDD4_itemVar.native_article.attachment_owner!=NULL || items[1].xDD4_itemVar.native_article.creator_spawn_id==0) &&
        (fighters[0].u.lk.x18!=NULL)==(milk_handle()!=-1) &&
        memcmp(&children[0].scale,&children[1].scale,sizeof(Vec3))==0 &&
        roots[0].flags==roots[1].flags && children[0].flags==children[1].flags &&
        descendants[0].flags==descendants[1].flags && siblings[0].flags==siblings[1].flags;
    if (!same && ++failures <= 12)
        fprintf(stderr,"Milk mismatch %s result=%d/%d motion=%d cmd=%d/%d scale=%g/%g flags=%x/%x owner=%d/%d handle=%d/%d\n",
            phase,stock_result,script_result,fighters[0].motion_id,
            (int)items[0].xDAC_itcmd_var0,(int)items[1].xDAC_itcmd_var0,children[0].scale.x,children[1].scale.x,
            (unsigned)children[0].flags,(unsigned)children[1].flags,
            items[0].owner!=NULL,items[1].owner!=NULL,fighters[0].u.lk.x18!=NULL,milk_handle());
}

static void signal_remove(Item_GObj* object)
{
    for (unsigned i=0; i<authored->signal_count; ++i)
        if (authored->signals[i].name_hash == UINT32_C(3683784189)) {
            authored->signals[i].callback(object,0); return;
        }
    abort();
}

int main(void)
{
    const NativeFighterHost host = {.item_creator=creator,
        .fighter_model_scale=ftLib_800869D4, .item_joint_value=nativeArticleModel_Read,
        .item_set_joint_value=nativeArticleModel_Write, .item_change_state_flags=Item_80268E5C,
        .item_advance_animation=Item_802694CC, .item_interact=it_8026B894, .item_remove=Item_8026A8EC};
    MotionState motions[1]={0};
    NativeFighterDefinition base={.source_kind=Ft_Kind_CLink,
        .motion_state_count=1,.motion_states=motions};
    const NativeFighterDefinition* definition=FreakMelee_GetFighterModule(&base,&host);
    if (definition==NULL) { fputs("Milk module rejected complete fixture Host\n",stderr); return 2; }
    authored=&definition->article_logics[0];
    milk_offset=UINT32_MAX;
    for (unsigned i=0;i<definition->state_field_count;++i)
        if (strcmp(definition->state_fields[i].name,"milk_handle")==0)
            milk_offset=definition->state_fields[i].offset;
    if (milk_offset==UINT32_MAX || authored->kind!=It_Kind_CLink_Milk) return 2;
    NativeFighterHost missing=host; missing.item_set_joint_value=NULL;
    if (FreakMelee_GetFighterModule(&base,&missing)!=NULL) return 1;
    missing=host; missing.fighter_model_scale=NULL;
    if (FreakMelee_GetFighterModule(&base,&missing)!=NULL) return 1;
    missing=host; missing.item_creator=NULL;
    if (FreakMelee_GetFighterModule(&base,&missing)!=NULL) return 1;
    if (FreakMelee_GetFighterModule(&base,&host)==NULL) return 2;
    const int motions_to_test[]={0,341,342,343,344};
    const int commands[]={-1,0,1,2,255,INT32_MIN};
    const float scales[]={0,.125f,1.25f,-2};
    for(unsigned state=0;state<2;++state) for(unsigned motion=0;motion<5;++motion)
    for(unsigned ref=0;ref<2;++ref) for(unsigned ownership=0;ownership<6;++ownership)
    for(unsigned command=0;command<6;++command) for(unsigned parent_command=0;parent_command<6;++parent_command)
    for(unsigned flags=0;flags<4;++flags) for(unsigned scale=0;scale<4;++scale) {
        reset(state,motions_to_test[motion],ref,ownership,commands[command],commands[parent_command],
              (flags&1?JOBJ_HIDDEN:0)|(flags&2?JOBJ_CLASSICAL_SCALE:0),scales[scale]);
        active=0; int x=itClinkmilk_UnkMotion1_Anim(&objects[0]);
        active=1; int y=authored->logic->states[state].animated(&objects[1]);
        compare(x,y,"animation");
    }
    for(unsigned ownership=0;ownership<6;++ownership) for(unsigned reference=0;reference<2;++reference) {
        reset(0,342,reference,ownership,1,1,JOBJ_HIDDEN,1);
        active=0;it_802C8C34(&objects[0]);active=1;signal_remove(&objects[1]);compare(0,0,"remove");
    }
    const float facings[]={-1,0,1,1.5f};
    for(unsigned state=0;state<2;++state) for(unsigned facing=0;facing<4;++facing) {
        reset(state,342,true,0,2,1,JOBJ_HIDDEN,1);
        items[0].facing_dir=items[1].facing_dir=facings[facing];
        active=0;it_2725_Logic80_PickedUp(&objects[0]);active=1;authored->logic->picked_up(&objects[1]);compare(0,0,"pickup");
        active=0;itClinkmilk_UnkMotion1_Phys(&objects[0]);active=1;authored->logic->states[state].physics_updated(&objects[1]);compare(0,0,"physics");
        active=0;int x=itClinkmilk_UnkMotion1_Coll(&objects[0]);active=1;int y=authored->logic->states[state].collided(&objects[1]);compare(x,y,"collision");
        active=0;itCLinkMilk_Logic80_EvtUnk(&objects[0],&recipients[0]);active=1;authored->logic->evt_unk(&objects[1],&recipients[1]);compare(0,0,"interaction");
    }
    // Constructor callback ordering/init is compared against the original ctor;
    // actual allocation/attachment backend remains separately covered by core tests.
    for(unsigned face=0;face<4;++face) for(unsigned failure=0;failure<2;++failure) {
        reset(0,342,true,0,-1,-1,JOBJ_HIDDEN,1);create_fails=failure;
        items[0].facing_dir=items[1].facing_dir=facings[face];
        active=0;HSD_GObj* original=it_802C8B28(&creators[0],&fighters[0].cur_pos,7,facings[face]);
        active=1;
        if (!failure) { authored->on_created(&objects[1]); Item_8026AB54(&objects[1],&creators[1],7); }
        compare(original!=NULL,!failure,"constructor init/pickup");
        ++comparisons;
        const NativeFighterAttachmentSpawn* spawn=authored->attachment;
        if (!spawn || !spawn->origin_at_owner || spawn->origin_at_center || !spawn->flatten_z ||
            spawn->initial_collision || spawn->register_as_held || spawn->previous_at_bone ||
            (!failure && (spawns[0].pos.z!=0 || spawns[0].prev_pos.z!=0 || spawns[0].x44_flag.b0))) ++failures;
    }
    printf("Milk stock-C/generated comparisons=%u failures=%u (unregistered article)\n",comparisons,failures);
    return failures?1:0;
}
