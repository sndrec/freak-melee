#include "native_match_fixture.h"
#ifdef MELEE_NATIVE
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/it/kinds/inlines.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/kinds/ftCommon/ftCo_Fall.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { FIXTURE_ITEMS = 8 };
static struct {
    uint64_t sample;
    ItemKind kind;
    Vec3 position;
    bool applied;
    bool capture_eligible;
} items[FIXTURE_ITEMS];
static unsigned item_count;
static struct {
    uint64_t sample;
    unsigned port;
    Vec3 position;
    bool applied;
} airborne[8];
static unsigned airborne_count;

void nativeMatchFixture_Reset(void)
{
    memset(items, 0, sizeof(items));
    item_count = 0;
    memset(airborne, 0, sizeof(airborne));
    airborne_count = 0;
}

bool nativeMatchFixture_AddItem(const char* specification, bool capture_eligible)
{
    unsigned kind;
    unsigned long long sample;
    float x, y;
    int end = 0;
    if (item_count == FIXTURE_ITEMS || specification == NULL ||
        sscanf(specification, "%u,%llu,%f,%f%n", &kind, &sample, &x, &y, &end) != 4 ||
        specification[end] != '\0' || kind >= It_Common_End || sample > 1000000 ||
        !isfinite(x) || !isfinite(y) || fabsf(x) > 10000 || fabsf(y) > 10000)
        return false;
    items[item_count].sample = sample;
    items[item_count].kind = kind;
    items[item_count].position = (Vec3) { x, y, 0 };
    items[item_count].capture_eligible = capture_eligible;
    ++item_count;
    return true;
}

bool nativeMatchFixture_AddAirborne(const char* specification)
{
    unsigned port;
    unsigned long long sample;
    float x, y;
    int end = 0;
    if (airborne_count == sizeof(airborne) / sizeof(airborne[0]) ||
        specification == NULL ||
        sscanf(specification, "%u,%llu,%f,%f%n", &port, &sample, &x, &y, &end) != 4 ||
        specification[end] != '\0' || port < 1 || port > 4 || sample > 1000000 ||
        !isfinite(x) || !isfinite(y) || fabsf(x) > 10000 || fabsf(y) > 10000)
        return false;
    airborne[airborne_count].port = port - 1;
    airborne[airborne_count].sample = sample;
    airborne[airborne_count].position = (Vec3) { x, y, 0 };
    ++airborne_count;
    return true;
}

bool nativeMatchFixture_Configured(void)
{
    return item_count != 0 || airborne_count != 0;
}

bool nativeMatchFixture_Apply(uint64_t sample)
{
    for (unsigned i = 0; i < airborne_count; ++i) {
        if (airborne[i].applied || airborne[i].sample != sample) continue;
        airborne[i].applied = true;
        Fighter_GObj* object = HSD_GObjPLinkHead != NULL
            ? HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER] : NULL;
        for (; object != NULL; object = object->next) {
            Fighter* fighter = object->user_data;
            if (fighter->player_id != airborne[i].port || fighter->x221F_b4) continue;
            // Begin a clean fall using the ordinary engine transition. No CPU
            // destination, queue, RNG or recovery decisions are injected.
            fighter->cur_pos = fighter->prev_pos = airborne[i].position;
            HSD_JObjSetTranslate(object->hsd_obj, &fighter->cur_pos);
            ftCommon_8007E2FC(object);
            ftCo_Fall_Enter(object);
            ft_80081B38(object);
            ftCommon_8007D60C(fighter);
            OSReport("[fixture] P%u airborne at sample %llu position=%g,%g jumps-used=%d.\n",
                     airborne[i].port + 1, (unsigned long long) sample,
                     fighter->cur_pos.x, fighter->cur_pos.y, fighter->x1968_jumpsUsed);
            break;
        }
        if (object == NULL) {
            OSReport("[fixture] No P%u at sample %llu.\n", airborne[i].port + 1,
                     (unsigned long long) sample);
            return false;
        }
    }
    for (unsigned i = 0; i < item_count; ++i) {
        if (items[i].applied || items[i].sample != sample) continue;
        items[i].applied = true;
        if (HSD_GObjPLinkHead == NULL ||
            HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER] == NULL) {
            OSReport("[fixture] No match fighters at sample %llu.\n",
                     (unsigned long long) sample);
            return false;
        }
        SpawnItem spawn = { 0 };
        spawn.kind = items[i].kind;
        Item_InitSpawn(&spawn, NULL, &items[i].position, 1);
        Item_GObj* item = Item_80268B18(&spawn);
        OSReport("[fixture] Item %u at sample %llu %s.\n", (unsigned) spawn.kind,
                 (unsigned long long) sample, item != NULL ? "spawned" : "failed");
        if (item == NULL) return false;
        // Explicit offline proxy for capture collision eligibility and the
        // release path's "not generator-owned" contract. This exercises the
        // capture lifecycle, not adventure-enemy assets or item callbacks.
        if (items[i].capture_eligible) {
            Item* data = item->user_data;
            data->xDD0_flag.b4 = true;
            data->xDD4_itemVar.zako.idx = -1;
        }
    }
    return true;
}
#endif
