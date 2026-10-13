#include <melee/ef/efasync.h>
#include <melee/ft/kinds/ftCommon/ftCo_ShieldBreakFly.h>
#include <melee/ft/kinds/ftCommon/inlines.h>

static bool shield_break_case;
static HSD_JObj shield_break_joint;
static FighterBone shield_break_part;
static struct ftData_x8 shield_break_model;

void efAsync_Spawn(HSD_GObj* object, void* count, u32 flags, u32 effect, HSD_JObj* joint, ...)
{
    CHECK(shield_break_case&&count==&((Fighter*)object->user_data)->x60C&&
        flags==0&&effect==1051&&joint==&shield_break_joint);
    step(40,object);
}
