#include <melee/gm/native_match_fixture.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/debug.h>
#include <stdio.h>
#include <stdlib.h>

static HSD_GObj fighter;
static HSD_GObj item_object;
static Item item_data;
static HSD_GObj* links[HSD_GOBJ_PLINK_FIGHTER + 1];
HSD_GObj** HSD_GObjPLinkHead = links;
static unsigned spawned;
static bool fail_spawn;
static SpawnItem last_spawn;
static Fighter fighter_data;
static HSD_JObj fighter_joint;
static unsigned airborne_steps;

void __assert(char* file, u32 line, char* expression)
{
    fprintf(stderr, "%s:%lu %s\n", file, (unsigned long) line, expression);
    exit(1);
}
void HSD_JObjSetMtxDirtySub(HSD_JObj* joint) { (void) joint; }
void ftCommon_8007E2FC(Fighter_GObj* object)
{
    (void) object;
    if (airborne_steps++ != 0) exit(1);
}
void ftCo_Fall_Enter(Fighter_GObj* object)
{
    (void) object;
    if (airborne_steps++ != 1) exit(1);
}
void ft_80081B38(Fighter_GObj* object)
{
    (void) object;
    if (airborne_steps++ != 2) exit(1);
}
void ftCommon_8007D60C(Fighter* data)
{
    if (airborne_steps++ != 3) exit(1);
    data->x1968_jumpsUsed = 5;
}

void OSReport(char* format, ...) { (void) format; }

Item_GObj* Item_80268B18(SpawnItem* spawn)
{
    ++spawned;
    last_spawn = *spawn;
    item_object.user_data = &item_data;
    return fail_spawn ? NULL : &item_object;
}

static void require(bool condition, const char* message)
{
    if (condition) return;
    fprintf(stderr, "%s\n", message);
    exit(1);
}

int main(void)
{
    nativeMatchFixture_Reset();
    require(!nativeMatchFixture_Configured(), "reset clears configuration");
    const char* invalid[] = { NULL, "", "2,120,0", "2,120,0,0,extra",
        "-1,120,0,0", "999,120,0,0", "2,-1,0,0", "2,1000001,0,0",
        "2,120,nan,0", "2,120,0,inf", "2,120,10001,0" };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        require(!nativeMatchFixture_AddItem(invalid[i], false), "reject invalid fixture");
        require(!nativeMatchFixture_AddItem(invalid[i], true), "reject invalid capturable fixture");
    }
    require(nativeMatchFixture_AddItem("2,120,-55,10", false), "configure barrel");
    require(nativeMatchFixture_Configured(), "fixture marked configured");
    links[HSD_GOBJ_PLINK_FIGHTER] = &fighter;
    require(nativeMatchFixture_Apply(119) && spawned == 0, "no early spawn");
    require(nativeMatchFixture_Apply(120) && spawned == 1, "spawn on selected sample");
    require(last_spawn.kind == 2 && last_spawn.pos.x == -55 &&
            last_spawn.pos.y == 10 && last_spawn.pos.z == 0 &&
            last_spawn.prev_pos.x == -55 && last_spawn.facing_dir == 1 &&
            last_spawn.vel.x == 0 && last_spawn.vel.y == 0 &&
            last_spawn.x0_parent_gobj == NULL, "ordinary item spawn parameters");
    require(nativeMatchFixture_Apply(120) && spawned == 1, "no duplicate spawn");
    require(!item_data.xDD0_flag.b4, "normal fixture retains ineligible item logic");
    nativeMatchFixture_Reset();
    for (unsigned i = 0; i < 8; ++i)
        require(nativeMatchFixture_AddItem("2,10,0,0", false), "fill fixture capacity");
    require(!nativeMatchFixture_AddItem("2,10,0,0", true), "bounded fixture capacity");
    require(nativeMatchFixture_Apply(10) && spawned == 9, "apply all scheduled items");
    nativeMatchFixture_Reset();
    require(nativeMatchFixture_AddItem("2,10,0,0", true), "configure failed spawn");
    fail_spawn = true;
    require(!nativeMatchFixture_Apply(10), "spawn failure aborts comparison");
    nativeMatchFixture_Reset();
    require(nativeMatchFixture_AddItem("2,10,0,0", true), "configure missing match");
    links[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    require(!nativeMatchFixture_Apply(10), "missing match aborts comparison");
    nativeMatchFixture_Reset();
    links[HSD_GOBJ_PLINK_FIGHTER] = &fighter;
    fail_spawn = false;
    item_data.xDD0_flag.b4 = true;
    item_data.xDD4_itemVar.zako.idx = 27;
    require(nativeMatchFixture_AddItem("0,10,0,0", false) && nativeMatchFixture_Apply(10),
            "normal fixture may spawn capture-eligible item logic");
    require(item_data.xDD0_flag.b4, "normal fixture never clears item eligibility");
    require(item_data.xDD4_itemVar.zako.idx == 27, "normal fixture preserves item variables");
    nativeMatchFixture_Reset();
    item_data.xDD0_flag.b4 = false;
    require(nativeMatchFixture_AddItem("0,10,0,0", true) && nativeMatchFixture_Apply(9),
            "explicit proxy schedules eligibility only at spawn");
    require(!item_data.xDD0_flag.b4, "no early eligibility mutation");
    require(item_data.xDD4_itemVar.zako.idx == 27, "no early generator-index mutation");
    require(nativeMatchFixture_Apply(10) && item_data.xDD0_flag.b4,
            "explicit proxy sets capture eligibility after successful spawn");
    require(item_data.xDD4_itemVar.zako.idx == -1,
            "explicit proxy satisfies release's non-generator-owned prerequisite");
    nativeMatchFixture_Reset();
    item_data.xDD0_flag.b4 = false;
    item_data.xDD4_itemVar.zako.idx = 37;
    fail_spawn = true;
    require(nativeMatchFixture_AddItem("0,10,0,0", true) && !nativeMatchFixture_Apply(10),
            "explicit proxy aborts failed spawn");
    require(!item_data.xDD0_flag.b4, "failed spawn never mutates eligibility");
    require(item_data.xDD4_itemVar.zako.idx == 37, "failed spawn never mutates item variables");
    fail_spawn = false;
    nativeMatchFixture_Reset();
    const char* invalid_airborne[] = { NULL, "", "0,120,0,0", "5,120,0,0",
        "1,-1,0,0", "1,1000001,0,0", "1,120,nan,0", "1,120,0,inf",
        "1,120,10001,0", "1,120,0,0,extra" };
    for (unsigned i = 0; i < sizeof(invalid_airborne) / sizeof(invalid_airborne[0]); ++i)
        require(!nativeMatchFixture_AddAirborne(invalid_airborne[i]), "reject invalid airborne placement");
    require(nativeMatchFixture_AddAirborne("1,120,-175,-5"), "configure airborne placement");
    fighter.user_data = &fighter_data;
    fighter.hsd_obj = &fighter_joint;
    links[HSD_GOBJ_PLINK_FIGHTER] = &fighter;
    require(nativeMatchFixture_Configured() && nativeMatchFixture_Apply(119) &&
            airborne_steps == 0, "no early airborne transition");
    require(nativeMatchFixture_Apply(120) && airborne_steps == 4 &&
            fighter_data.cur_pos.x == -175 && fighter_data.cur_pos.y == -5 &&
            fighter_data.prev_pos.x == -175 && fighter_joint.translate.y == -5,
            "airborne placement resets position and uses shared fall/ECB transitions");
    require(nativeMatchFixture_Apply(120) && airborne_steps == 4, "no duplicate airborne transition");
    return 0;
}
