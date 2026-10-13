// Actual shared insertion bodies; HSD allocation/part registration are bounded
// controlled dependencies. No renderer or game process is started.
#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ftparts.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_model_graph.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#undef HSD_ASSERTREPORT
#define HSD_ASSERTREPORT(line, condition, ...) assert(condition)

static Fighter fighter;
static HSD_GObj object;
static FighterBone bones[6];
static FighterPartsTable part_table = { .parts_num = 6 };
static FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity];
FighterPartsTable** ftPartsTable = part_tables;
static struct Fighter_804D6540_x0_t placement;
static struct Fighter_804D6540_t omission_table;
static struct Fighter_804D6540_t* omission_tables[Ft_Kind_RuntimeCapacity];
struct Fighter_804D6540_t** Fighter_804D6540 = omission_tables;
static NativeFighterDefinition definition;
static ftData data;
static void* graphs[16];
static HSD_Joint descriptor, child_descriptor, sibling_descriptor;
static HSD_JObj parents[2], grandparents[2], children[2], siblings[2], created[2];
static HSD_DObj draw_objects[2];
static HSD_DObj* draw_list[124];
static unsigned loads, removals, registrations, fail_load, checks;
static ftCommonData common;
ftCommonData* p_ftCommonData = &common;
struct Fighter_ShakeTable_t* Fighter_SmashChargeShakeTable;
#define CHECK(value) do { ++checks; assert(value); } while (0)

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{ return kind == Ft_Kind_CustomFirst ? &definition : NULL; }
HSD_JObj* ftParts_8007482C(HSD_Joint* source)
{
    ++loads;
    CHECK(source->child == NULL && source->next == NULL);
    if (loads == fail_load) return NULL;
    CHECK(loads <= 2);
    HSD_JObj* result = &created[loads - 1];
    result->scale = source->scale;
    result->translate = source->position;
    if (source->u.dobjdesc != NULL) result->u.dobj = &draw_objects[loads - 1];
    return result;
}
HSD_JObj* HSD_JObjRemove(HSD_JObj* joint)
{ CHECK(joint == &created[0]); ++removals; return NULL; }
void ftParts_80074194(Fighter* fp, FighterBone* bone, HSD_JObj* joint,
                      int* mesh_index, u32 depth)
{
    CHECK(fp == &fighter && bone == &bones[4]); ++registrations;
    bone->joint = joint; bone->flags_b1 = true; bone->xC = depth;
    if (joint->u.dobj != NULL) draw_list[(*mesh_index)++] = joint->u.dobj;
}

#include "model_graph_actual.inc"

static void reset(unsigned mode, unsigned ordinal)
{
    memset(&fighter, 0, sizeof(fighter)); memset(bones, 0, sizeof(bones));
    memset(&definition, 0, sizeof(definition)); memset(&descriptor, 0, sizeof(descriptor));
    memset(&child_descriptor, 0, sizeof(child_descriptor)); memset(&sibling_descriptor, 0, sizeof(sibling_descriptor));
    memset(parents, 0, sizeof(parents)); memset(created, 0, sizeof(created));
    memset(children, 0, sizeof(children)); memset(siblings, 0, sizeof(siblings));
    memset(draw_list, 0, sizeof(draw_list));
    loads = removals = registrations = fail_load = 0;
    object.user_data = &fighter; fighter.kind = Ft_Kind_CustomFirst;
    fighter.parts = bones; fighter.ft_data = &data;
    fighter.dobj_list.data = draw_list;
    data.x48_items = graphs; graphs[6] = &descriptor;
    definition.asset_layout.joint_graph_mask = 1u << 6;
    part_table.parts_num = 6; part_tables[fighter.kind] = &part_table;
    omission_tables[fighter.kind] = &omission_table;
    placement = (struct Fighter_804D6540_x0_t){4, 2, mode, ordinal};
    omission_table.x0 = &placement; omission_table.x4 = 1;
    bones[2].joint = &parents[0]; bones[2].x4_jobj2 = &parents[1]; bones[2].xC = 7;
    for (unsigned i = 0; i < 2; ++i) {
        parents[i].parent = &grandparents[i];
        parents[i].child = &children[i]; children[i].parent = &parents[i];
        parents[i].next = &siblings[i]; siblings[i].parent = &grandparents[i];
    }
    descriptor.scale = (Vec3){1, 2, 3}; descriptor.position = (Vec3){4, 5, 6};
    child_descriptor.scale = (Vec3){7, 8, 9}; child_descriptor.position = (Vec3){10, 11, 12};
    sibling_descriptor.scale = (Vec3){13, 14, 15}; sibling_descriptor.position = (Vec3){16, 17, 18};
    descriptor.child = &child_descriptor; descriptor.next = &sibling_descriptor;
}
static void check_result(unsigned mode, unsigned ordinal)
{
    CHECK(loads == 2 && registrations == 1 && removals == 0);
    CHECK(bones[4].joint == &created[0] && bones[4].x4_jobj2 == &created[1] && bones[4].flags_b2);
    CHECK(bones[4].xC == 7 + (mode <= 1));
    float expected = ordinal == 1 ? 7 : ordinal == 2 ? 13 : 1;
    CHECK(created[0].scale.x == expected && created[1].scale.x == expected);
    for (unsigned i = 0; i < 2; ++i) {
        CHECK(created[i].parent == (mode <= 1 ? &parents[i] : &grandparents[i]));
        if (mode == 0) CHECK(parents[i].child == &created[i] && created[i].child == &children[i] && children[i].parent == &created[i]);
        if (mode == 1) CHECK(parents[i].child == &created[i] && created[i].next == &children[i]);
        if (mode == 2) CHECK(parents[i].next == &created[i] && created[i].child == &siblings[i]);
        if (mode == 3) CHECK(parents[i].next == &created[i] && created[i].next == &siblings[i]);
    }
}
int main(void)
{
    unsigned pairs = 0;
    for (unsigned mode = 0; mode < 4; ++mode)
        for (unsigned selected = 0; selected < 4; ++selected) {
            unsigned ordinal = selected == 3 ? 255 : selected;
            reset(mode, ordinal);
            ftParts_800753D4(&fighter, &placement, &descriptor); check_result(mode, ordinal);
            reset(mode, ordinal);
            CHECK(nativeFighterModelGraph_Install(&object, 6, 0)); check_result(mode, ordinal);
            CHECK(!nativeFighterModelGraph_Install(&object, 6, 0) && loads == 2);
            ++pairs;
        }
    reset(0, 255); CHECK(!nativeFighterModelGraph_Install(NULL, 6, 0));
    CHECK(!nativeFighterModelGraph_Install(&object, -1, 0));
    CHECK(!nativeFighterModelGraph_Install(&object, 16, 0));
    CHECK(!nativeFighterModelGraph_Install(&object, 6, -1));
    CHECK(!nativeFighterModelGraph_Install(&object, 6, 1));
    definition.asset_layout.joint_graph_mask = 0; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 255); graphs[6] = NULL; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 3); CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 255); placement.x0 = 6; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 255); placement.x1 = 6; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 255); placement.x2 = 4; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 255); bones[2].x4_jobj2 = NULL; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(2, 255); parents[1].parent = NULL; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    reset(0, 255); fail_load = 1; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0) && removals == 0 && registrations == 0);
    reset(0, 255); fail_load = 2; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0) && removals == 1 && registrations == 0 && parents[0].child == &children[0]);
    HSD_DObjDesc mesh = {0};
    reset(0, 255); descriptor.u.dobjdesc = &mesh; fighter.dobj_list.count = 3;
    draw_list[0] = &draw_objects[1];
    CHECK(nativeFighterModelGraph_Install(&object, 6, 0));
    CHECK(fighter.dobj_list.count == 4 && draw_list[0] == &draw_objects[1] && draw_list[3] == &draw_objects[0]);
    reset(0, 255); descriptor.u.dobjdesc = &mesh; fighter.dobj_list.count = 124;
    CHECK(!nativeFighterModelGraph_Install(&object, 6, 0) && loads == 0);
    reset(0, 255); descriptor.flags = JOBJ_SPLINE; CHECK(!nativeFighterModelGraph_Install(&object, 6, 0));
    common.x2E4 = .125f; common.x2E8 = .75f;
    CHECK(common_attribute(NativeFighterCommon_ShieldPushbackLightMin) == .125f);
    CHECK(common_attribute(NativeFighterCommon_ShieldPushbackLightMax) == .75f);
    printf("%u two-pose original/native graph pairs plus invalid/assets/append checks: %u\n", pairs, checks);
    return 0;
}
