// Actual constructor and ordinary throw paths, with retail-pinned saved fields.
#include <melee/ft/types.h>
#include <melee/it/types.h>
#include <melee/it/itCommonItems.h>
#include <melee/it/itCharItems.h>
#include <melee/it/kinds/itdosei.h>
#include <melee/it/inlines.h>
#include <melee/it/it_2725.h>
#include <melee/it/ithitbox.h>
#include <melee/it/itcoll.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#define GET_ATTRS(ip) ((itBombHeiAttributes*) (ip)->xC4_article_data->x4_specialAttributes)

typedef struct ScaleEvent { int kind, a, b; float value; } ScaleEvent;
typedef struct ScaleObservation {
    Item item;
    ScaleEvent events[32];
    unsigned event_count;
    bool succeeded;
    Quaternion rotation;
    Vec3 scale;
} ScaleObservation;
static Item item;
static HSD_GObj fighter_gobj, item_gobj;
static HSD_JObj joint, child;
static Article article;
static itBombHeiAttributes bomb_attrs;
static itDoseiAttributes saturn_attrs;
static itPeachTurnipAttributes turnip_attrs;
static ScaleEvent events[32];
static float owner_scale;
static unsigned event_count, checks;
static bool spawn_failure;
static bool timer_expired;

static void require(bool condition, const char* reason)
{
    ++checks;
    if (!condition) { fprintf(stderr, "%s\n", reason); exit(1); }
}
static void event(int kind, int a, int b, float value)
{
    require(event_count < 32, "bounded constructor/throw trace");
    events[event_count++] = (ScaleEvent){ kind, a, b, value };
}
HSD_GObj* Item_80268B18(SpawnItem* spawn)
{
    require(spawn->x0_parent_gobj == &fighter_gobj &&
            spawn->x4_parent_gobj2 == &fighter_gobj, "both original creator parents");
    require(spawn->pos.x == 1 && spawn->pos.y == 2 && spawn->pos.z == 0 &&
            !memcmp(&spawn->pos, &spawn->prev_pos, sizeof(Vec3)), "actual ordered spawn positions");
    require(spawn->vel.x == 0 && spawn->vel.y == 0 && spawn->vel.z == 0 &&
            spawn->facing_dir == -1 && spawn->x3C_damage == 0 && spawn->x40 == 0 &&
            spawn->x44_flag.b0, "actual direct spawn flags/velocity/facing");
    event(1, spawn->kind, 0, 0);
    if (spawn_failure) return NULL;
    item.kind = spawn->kind; item.owner = spawn->x0_parent_gobj;
    item.facing_dir = spawn->facing_dir;
    return &item_gobj;
}
void Item_8026AB54(HSD_GObj* g, HSD_GObj* owner, Fighter_Part part)
{
    require(g == &item_gobj && owner == &fighter_gobj && part == 109, "direct held attachment arguments");
    event(2, part, 0, 0);
}
void it_80274594(HSD_GObj* g)
{
    require(g == &item_gobj, "owner scale source");
    event(3, 0, 0, owner_scale); item.scl *= owner_scale;
}
void it_80275158(HSD_GObj* g, float life) { require(g==&item_gobj, "Turnip lifetime source"); event(15,0,0,life); }
s32 it_802BD32C(HSD_GObj* g) { require(g==&item_gobj, "Turnip weighted selector boundary"); event(16,0,0,0); return 0; }
void it_8026BD0C(HSD_GObj* g) { require(g==&item_gobj, "Turnip presentation source"); event(17,0,0,0); }
void itBombhei_UpdateStatePreserveBone(HSD_GObj* g, int state, int flags)
{ require(g == &item_gobj, "Bomb state source"); item.msid = state; event(4, state, flags, 0); }
void it_80275474(HSD_GObj* g) { require(g == &item_gobj, "Bomb presentation source"); event(5, 0, 0, 0); }
void it_80275534(HSD_GObj* g,float value) { require(g==&item_gobj && joint.scale.x==item.scl && joint.scale.y==item.scl && joint.scale.z==item.scl,"new uniform scale before geometry"); event(6,0,0,item.scl); }
void it_80274DFC(HSD_GObj* g) { require(g==&item_gobj,"geometry2 source"); event(18,0,0,0); }
void it_80274E44(HSD_GObj* g) { require(g==&item_gobj,"geometry3 source"); event(19,0,0,0); }
void HSD_JObjSetMtxDirtySub(HSD_JObj* j) { j->flags|=JOBJ_MTX_DIRTY; event(20,0,0,0); }
void (HSD_JObjSetMtxDirty)(HSD_JObj* j) { j->flags|=JOBJ_MTX_DIRTY; event(20,0,0,0); }
void __assert(char* file,u32 line,char* message) { fprintf(stderr,"%s:%lu %s\n",file,(unsigned long)line,message); exit(1); }
void it_8026B390(HSD_GObj* g) { require(g == &item_gobj, "Saturn update source"); event(7, 0, 0, 0); }
void HSD_JObjClearFlagsAll(HSD_JObj* j, u32 flags)
{ require(j == &child && flags == JOBJ_HIDDEN, "Saturn child visibility"); event(8, 0, 0, 0); }
void Item_80268E5C(HSD_GObj* g, enum_t state, Item_StateChangeFlags flags)
{ require(g == &item_gobj, "state source"); item.msid = state; event(9, state, flags, 0); }
void lb_8000BA0C(HSD_JObj* j, float rate)
{ require(j == &joint && rate == saturn_attrs.unk0, "Saturn rate"); event(10, 0, 0, rate); }
bool it_80272C6C(HSD_GObj* g) { require(g==&item_gobj,"timer source"); event(11,0,0,0); return !timer_expired; }
void itBombhei_UpdateStatePreserveBoneMotion10(HSD_GObj* g,int state,int flags) { require(g==&item_gobj,"Bomb timer transition source"); item.msid=state; event(12,state,flags,0); }
void it_8027D820(HSD_GObj* g) { require(g==&item_gobj,"Bomb spinning source"); event(13,0,0,0); }
void it_8027DE18(HSD_GObj* g) { require(g==&item_gobj,"Bomb spawned continuation"); event(14,0,0,0); }
void itDosei_80282BFC(HSD_GObj* g) { require(g==&item_gobj,"Saturn spawned continuation"); item.msid=8; item.owner=NULL; event(21,0,0,0); }
void it_80273670(HSD_GObj* g,int descriptor,float frame) { require(g==&item_gobj,"Turnip appearance source"); event(22,descriptor,0,frame); }
void it_80272460(HitCapsule* hit,u32 damage,HSD_GObj* g) { require(g==&item_gobj && hit==&item.x5D4_hitboxes[0].hit,"Turnip damage source"); event(23,damage,0,0); }

#include "peach_rare_scale_original.inc"
#include "peach_rare_scale_candidate.inc"
#include "peach_rare_scale_legacy.inc"

static ScaleObservation run(unsigned version, ItemKind kind, float scale,
                            float parent_scale, unsigned branch, bool failure)
{
    memset(&item, 0, sizeof(item)); memset(events, 0, sizeof(events));
    memset(&reference_vars,0,sizeof reference_vars);
    memset(&joint, 0, sizeof(joint)); joint.child = &child;
    item_gobj = (HSD_GObj){ .user_data = &item, .hsd_obj = &joint };
    item.xC4_article_data = &article; item.scl = scale; item.throw_scale=1;
    article.x4_specialAttributes = kind == It_Kind_BombHei ? (void*) &bomb_attrs : kind==It_Kind_Peach_Turnip ? (void*)&turnip_attrs : (void*) &saturn_attrs;
    if (kind == It_Kind_BombHei) {
        reference_vars.bombhei.xDE8 = 1;
        reference_vars.bombhei.xDE0 = branch;
        item.xDD4_itemVar.bombhei.xDE0 = branch;
    } else {
        reference_vars.dosei.xDE0 = 1;
        reference_vars.peachturnip.xDE0_scl=1;
        item.msid = branch ? 5 : 0;
    }
    owner_scale = parent_scale; spawn_failure = failure; event_count = 0;
    Vec3 pos = { 1, 2, 0 };
    HSD_GObj* result = version == 0 ? console_it_802BD4AC(&fighter_gobj, &pos, 109, kind, -1) :
        version == 1 ? candidate_it_802BD4AC(&fighter_gobj, &pos, 109, kind, -1) :
                       legacy_it_802BD4AC(&fighter_gobj, &pos, 109, kind, -1);
    require(result == (failure ? NULL : &item_gobj), "constructor result");
    if (!failure) {
        require(item.scl == (kind == It_Kind_Sword ? scale : scale * parent_scale), "constructor live scale");
        if (kind == It_Kind_BombHei) {
            if (version == 0) console_it_3F14_Logic6_Thrown(result);
            else if (version == 1) candidate_it_3F14_Logic6_Thrown(result);
            else legacy_it_3F14_Logic6_Thrown(result);
        } else if (kind == It_Kind_Dosei) {
            if (version == 0) console_itDosei_Logic7_Thrown(result);
            else if (version == 1) candidate_itDosei_Logic7_Thrown(result);
            else legacy_itDosei_Logic7_Thrown(result);
        } else if(kind==It_Kind_Peach_Turnip) {
            if(version==0) console_itPeachTurnip_Logic56_Thrown(result);
            else if(version==1) candidate_itPeachTurnip_Logic56_Thrown(result);
            else legacy_itPeachTurnip_Logic56_Thrown(result);
        }
    }
    if(version!=1) reference_project(&item);
    ScaleObservation observation = {0}; observation.item = item;
    memcpy(observation.events, events, sizeof(events)); observation.event_count = event_count;
    observation.succeeded = result != NULL;
    observation.rotation=joint.rotate; observation.scale=joint.scale;
    return observation;
}

static bool equal(const ScaleObservation* a, const ScaleObservation* b)
{
    return !memcmp(&a->item, &b->item, sizeof(Item)) &&
        a->event_count == b->event_count && a->succeeded == b->succeeded &&
        !memcmp(a->events, b->events, sizeof(a->events)) &&
        !memcmp(&a->rotation,&b->rotation,sizeof a->rotation) && !memcmp(&a->scale,&b->scale,sizeof a->scale);
}
static ScaleObservation lifecycle(unsigned version,ItemKind kind,float remembered,
                                  unsigned branch,unsigned path,bool expired,float facing)
{
    memset(&item,0,sizeof item); memset(&reference_vars,0,sizeof reference_vars);
    memset(events,0,sizeof events); memset(&joint,0,sizeof joint); joint.child=&child;
    item_gobj=(HSD_GObj){.user_data=&item,.hsd_obj=&joint};
    item.kind=kind; item.xC4_article_data=&article; item.scl=2.75f; item.throw_scale=remembered;
    item.facing_dir=facing; item.x40_vel=(Vec3){3,4,5}; item.msid=branch ? 5 : 0;
    item.xDD4_itemVar.bombhei.xDE0=branch;
    reference_vars.bombhei.xDE0=branch;
    if(kind==It_Kind_BombHei) {
        reference_vars.bombhei.xDE8=remembered;
        article.x4_specialAttributes=&bomb_attrs;
    } else if(kind==It_Kind_Dosei) {
        reference_vars.dosei.xDE0=remembered;
        article.x4_specialAttributes=&saturn_attrs;
        // Bomb's branch byte is unrelated to Saturn; do not invent state there.
        memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);
        memset(&reference_vars,0,sizeof reference_vars); reference_vars.dosei.xDE0=remembered;
    } else {
        memset(&item.xDD4_itemVar,0,sizeof item.xDD4_itemVar);
        memset(&reference_vars,0,sizeof reference_vars);
        reference_vars.peachturnip.xDE0_scl=remembered;
        reference_vars.peachturnip.xDD8=branch;
        reference_vars.peachturnip.xDDC_damage=23;
        item.xDD4_itemVar.peachturnip.xDD8=branch;
        item.xDD4_itemVar.peachturnip.xDDC_damage=23;
        article.x4_specialAttributes=&turnip_attrs;
    }
    timer_expired=expired; event_count=0;
    if(kind==It_Kind_BombHei) {
        if(path==0) { if(version) candidate_it_3F14_Logic6_Thrown(&item_gobj); else console_it_3F14_Logic6_Thrown(&item_gobj); }
        else if(path==1) { if(version) candidate_it_3F14_Logic6_Dropped(&item_gobj); else console_it_3F14_Logic6_Dropped(&item_gobj); }
        else if(path==2) { require(!(version ? candidate_itBombhei_UnkMotion10_Anim(&item_gobj) : console_itBombhei_UnkMotion10_Anim(&item_gobj)),"Bomb timer keeps item alive"); }
        else { if(version) candidate_itBombhei_Logic6_Spawned(&item_gobj); else console_itBombhei_Logic6_Spawned(&item_gobj); }
    } else if(kind==It_Kind_Dosei) {
        if(path==0) { if(version) candidate_itDosei_Logic7_Thrown(&item_gobj); else console_itDosei_Logic7_Thrown(&item_gobj); }
        else if(path==1) { if(version) candidate_itDosei_Logic7_Dropped(&item_gobj); else console_itDosei_Logic7_Dropped(&item_gobj); }
        else { if(version) candidate_itDosei_Logic7_Spawned(&item_gobj); else console_itDosei_Logic7_Spawned(&item_gobj); }
    } else if(path==0) { if(version) candidate_itPeachTurnip_Logic56_Thrown(&item_gobj); else console_itPeachTurnip_Logic56_Thrown(&item_gobj); }
    else { if(version) candidate_itPeachTurnip_Logic56_Dropped(&item_gobj); else console_itPeachTurnip_Logic56_Dropped(&item_gobj); }
    if(!version) reference_project(&item);
    ScaleObservation observation={0}; observation.item=item; observation.event_count=event_count;
    memcpy(observation.events,events,sizeof events); observation.rotation=joint.rotate; observation.scale=joint.scale;
    return observation;
}
int main(void)
{
    ItemAttr attributes={0};
    for(unsigned i=0;i<3;i++) {
        item.xCC_item_attr=&attributes; item.throw_scale=73; item.scl=94;
        attributes.x60_scale=i==0 ? .5f : i==1 ? 1 : 2;
        candidate_initialize_scale(&item);
        require(item.scl==attributes.x60_scale && item.throw_scale==1,"actual allocator defaults separate remembered from instance scale");
    }
    require(offsetof(Reference_itPeachTurnip_ItemVars, xDE8_scl) != offsetof(Reference_itBombHei_ItemVars, xDE8),
            "negative control requires the actual widened-pointer Bomb alias mismatch");
    require(offsetof(Reference_itPeachTurnip_ItemVars, xDE0_scl) == offsetof(Reference_itDosei_ItemVars, xDE0),
            "Saturn native alias happens to match");
    bomb_attrs.x0 = 1.75f; saturn_attrs.unk0 = .75f;
    bomb_attrs.x4=2; bomb_attrs.x8=3; bomb_attrs.x10=4; bomb_attrs.x14=5; saturn_attrs.unk4=60;
    const float scales[] = { .5f, .75f, 1, 1.25f, 2 };
    turnip_attrs.x0_lifetime=400; turnip_attrs.x8[0].x4_damage=23;
    const ItemKind kinds[] = { It_Kind_BombHei, It_Kind_Dosei, It_Kind_Sword, It_Kind_Peach_Turnip };
    unsigned pairs = 0, old_mismatches = 0;
    for (unsigned kind = 0; kind < 4; ++kind)
        for (unsigned s = 0; s < 5; ++s)
            for (unsigned o = 0; o < 5; ++o)
                for (unsigned branch = 0; branch < 2; ++branch) {
                    ScaleObservation reference = run(0, kinds[kind], scales[s], scales[o], branch, false);
                    ScaleObservation candidate = run(1, kinds[kind], scales[s], scales[o], branch, false);
                    ScaleObservation legacy = run(2, kinds[kind], scales[s], scales[o], branch, false);
                    require(equal(&reference, &candidate), "actual repaired constructor/throw differs from retail semantic reference");
                    if (kind == 0 && branch == 0 && legacy.item.throw_scale != reference.item.throw_scale)
                        ++old_mismatches;
                    if (kind != 0) require(equal(&reference, &legacy), "unaffected Saturn/Sword original behavior");
                    if (kind == 0) {
                        require(candidate.item.throw_scale == scales[s] * scales[o], "Bomb intended saved scale");
                        unsigned restores = 0;
                        for (unsigned e = 0; e < candidate.event_count; ++e) if (candidate.events[e].kind == 6) {
                            ++restores; require(candidate.events[e].value == scales[s] * scales[o], "actual Bomb throw restored saved scale");
                        }
                        require(restores == (scales[s] * scales[o] != 1), "Bomb non-unit-only restore");
                    }
                    ++pairs;
                }
    for (unsigned kind = 0; kind < 4; ++kind) {
        ScaleObservation reference = run(0, kinds[kind], .75f, 2, 0, true);
        ScaleObservation candidate = run(1, kinds[kind], .75f, 2, 0, true);
        require(equal(&reference, &candidate) && candidate.event_count == 1, "failed spawn has no attachment/scale/throw continuation");
        ++pairs;
    }
    require(old_mismatches == 22, "baseline23 recorded exactly 22 of 25 Bomb saved-scale mismatches");
    unsigned lifecycle_pairs=0;
    for(unsigned kind=0;kind<4;kind++) {
        if(kinds[kind]==It_Kind_Sword) continue;
        for(unsigned path=0;path<4;path++) {
            if(path==2 && kinds[kind]!=It_Kind_BombHei) continue;
            if(path==3 && kinds[kind]==It_Kind_Peach_Turnip) continue;
            for(unsigned scale=0;scale<5;scale++) for(unsigned branch=0;branch<2;branch++)
            for(unsigned face=0;face<2;face++) for(unsigned expired=0;expired<(path==2 ? 2 : 1);expired++) {
                ScaleObservation reference=lifecycle(0,kinds[kind],scales[scale],branch,path,expired,face ? 1 : -1);
                ScaleObservation candidate=lifecycle(1,kinds[kind],scales[scale],branch,path,expired,face ? 1 : -1);
                require(equal(&reference,&candidate),"shared storage changed actual spawn/throw/drop/timer semantics");
                unsigned restores=0;
                for(unsigned e=0;e<candidate.event_count;e++) if(candidate.events[e].kind==6) {
                    ++restores;
                    require(candidate.events[e].value==scales[scale],"restoration uses remembered value");
                    require(e+2<candidate.event_count && candidate.events[e+1].kind==18 && candidate.events[e+2].kind==19,"actual geometry refresh order after uniform scale");
                }
                unsigned expected=0;
                if(path==0) expected=kinds[kind]==It_Kind_BombHei ? scales[scale]!=1 : 1;
                if(path==2) expected=expired && scales[scale]!=1;
                require(restores==expected,"throw/drop/timer restoration timing");
                require(candidate.item.throw_scale==(path==3 ? 1 : scales[scale]),"remembered value survives state changes; spawn resets1");
                require(candidate.item.scl==(expected ? scales[scale] : 2.75f),"drop/unexpired timer does not restore scale");
                ++lifecycle_pairs;
            }
        }
    }
    printf("%u actual constructor/throw + %u spawn/drop/timer comparison pairs; %u retained baseline23 Bomb mismatches; %u checks PASS.\n", pairs,lifecycle_pairs,old_mismatches,checks);
    return 0;
}
