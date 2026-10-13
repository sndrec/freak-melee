// Exact original callback bodies and common item callers versus the compiled
// authored hooks. The driver extracts source into yoshi_item_original.inc.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_module.h>
#include <melee/ft/native_fighter_parameters.h>
#include <melee/ft/fighter.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/inlines.h>
#include <melee/ft/kinds/ftCommon/ftpickupitem.h>
#include <melee/ft/kinds/ftCommon/ftCo_HammerWait.h>
#include <melee/pl/plbonuslib.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const NativeFighterDefinition* (__cdecl* Entry)(const NativeFighterDefinition*, const NativeFighterHost*);
typedef int (__cdecl* SetParameters)(const uint32_t*, unsigned, uint64_t);
typedef struct ItemCallbackEvent { int operation, flag, item_part, hand_part, held, auxiliary; } ItemCallbackEvent;
static ItemCallbackEvent events[16], expected[16];
static unsigned event_count, expected_count, comparisons;
static Fighter fighters[2];
static Fighter_GObj objects[2];
static Item_GObj items[2][2];
static bool hammer;
static bool heavy_item;
static int hold_kind;
Fighter_ItemEvent ftData_OnItemPickupExt[Ft_Kind_RuntimeCapacity];
Fighter_ItemEvent ftData_OnItemDropExt[Ft_Kind_RuntimeCapacity];
Fighter_ItemEvent ftData_OnItemPickup[Ft_Kind_RuntimeCapacity];
Fighter_ItemEvent ftData_OnItemDrop[Ft_Kind_RuntimeCapacity];

static void require(bool ok, const char* why)
{ if (!ok) { fprintf(stderr, "%s at comparison %u\n", why, comparisons); exit(1); } }
static void record(Fighter_GObj* object, int op, int flag, int item, int hand)
{
    Fighter* f = GET_FIGHTER(object);
    require(event_count < 16, "Event capacity");
    events[event_count++] = (ItemCallbackEvent){op,flag,item,hand,f->item_gobj != NULL,f->x1978 != NULL};
}
// Execute the real shared Fighter_OnItemPickup/Drop inlines, observing their
// animation operations rather than replacing the original callback behavior.
bool itIsHeavy(Item_GObj* item) { (void)item; return heavy_item; }
Item_HoldKinds itGetHoldKind(Item_GObj* item) { (void)item; return hold_kind; }
void ftAnim_80070FB4(Fighter_GObj* gobj, s32 part, s32 animation)
{ record(gobj, 1, animation, part, 0); }
void ftAnim_80070C48(Fighter_GObj* gobj, s32 part)
{ record(gobj, 2, 1, part, 0); }
void ftAnim_80070CC4(Fighter_GObj* gobj, int part)
{ record(gobj, 2, 0, part, 0); }
void pl_8003E17C(int player, int secondary, Item_GObj* item)
{
    Fighter* f = &fighters[player];
    require(item == (f->x1978 != NULL ? f->x1978 : f->item_gobj), "Pickup accounting item identity");
    record(&objects[player], 3, secondary, 0, 0);
}
void pl_8003EA08(int player, int secondary)
{ record(&objects[player], 4, secondary, 0, 0); }
bool ftCo_800C5240(Fighter_GObj* gobj)
{ record(gobj, 5, hammer, 0, 0); return hammer; }
void ftCo_800C5500(Fighter_GObj* gobj)
{ record(gobj, 6, 0, 0, 0); }

#include "yoshi_item_original.inc"

static void unexpected_operation(void)
{ require(false, "Operation outside item callback fixture"); }
static void initialize(unsigned held, unsigned auxiliary, unsigned secondary)
{
    memset(fighters, 0, sizeof(fighters));
    memset(objects, 0, sizeof(objects));
    memset(items, 0, sizeof(items));
    for (unsigned i = 0; i < 2; ++i) {
        objects[i].user_data = &fighters[i];
        fighters[i].kind = i ? Ft_Kind_CustomFirst : Ft_Kind_Yoshi;
        fighters[i].player_id = i;
        fighters[i].x221F_b4 = secondary;
        fighters[i].item_gobj = held ? &items[i][0] : NULL;
        fighters[i].x1978 = auxiliary ? &items[i][1] : NULL;
    }
    event_count = 0;
}
static void remember(void)
{ expected_count = event_count; memcpy(expected, events, sizeof(expected)); event_count = 0; }
static void compare(void)
{
    ++comparisons;
    require(expected_count == event_count && !memcmp(expected, events, event_count*sizeof(*events)),
            "Exact ordered original-C item arguments differ");
    require((fighters[0].item_gobj != NULL) == (fighters[1].item_gobj != NULL) &&
            (fighters[0].x1978 != NULL) == (fighters[1].x1978 != NULL), "Held/auxiliary release differs");
}

int main(int argc, char** argv)
{
    require(argc == 3, "Expected module and FMP");
    NativeFighterParameters parameters;
    FILE* file = fopen(argv[2], "rb"); require(file != NULL, "FMP open");
    char text[32768]; size_t size = fread(text, 1, sizeof(text), file); fclose(file); unsigned line;
    require(nativeFighterParameters_Parse(text, size, &parameters, &line), "FMP parse");
    HMODULE library = LoadLibraryA(argv[1]); require(library != NULL, "Module load");
    Entry entry = (Entry)GetProcAddress(library, "FreakMelee_GetFighterModule");
    SetParameters upload = (SetParameters)GetProcAddress(library, "FreakMelee_SetFighterParameters");
    require(entry && upload, "Module exports");
    MotionState motions[28] = {0};
    NativeFighterDefinition base = {
        .motion_state_first=341,.motion_state_count=28,.motion_states=motions,
        .special_ground_state={346,360,364,366},
        .special_air_state={351,360,365,368},
    };
    NativeFighterHost host = {0};
    _Static_assert(sizeof(host)%sizeof(void*) == 0, "Host function pointers only");
    for (unsigned offset = 0; offset < sizeof(host); offset += sizeof(void*)) {
        uintptr_t pointer = (uintptr_t)unexpected_operation;
        memcpy((char*)&host + offset, &pointer, sizeof(pointer));
    }
    host.fighter_item_pickup = Fighter_OnItemPickup;
    host.fighter_item_drop = Fighter_OnItemDrop;
    const NativeFighterDefinition* policy = entry(&base, &host);
    require(policy != NULL, "Candidate module");
    require(policy->on_item_pickup && policy->on_item_drop &&
            policy->on_item_pickup_ext && policy->on_item_drop_ext, "All four authored item table slots required");
    uint32_t words[128];
    for (unsigned i=0; i<parameters.count; ++i) words[i]=parameters.fields[i].bits;
    require(upload(words, parameters.count, parameters.schema_hash), "Owned parameters");
    ftData_OnItemPickup[Ft_Kind_Yoshi] = original_ftYs_Init_OnItemPickup;
    ftData_OnItemPickupExt[Ft_Kind_Yoshi] = original_ftYs_Init_OnItemPickup;
    ftData_OnItemDrop[Ft_Kind_Yoshi] = original_ftYs_Init_OnItemDrop;
    ftData_OnItemDropExt[Ft_Kind_Yoshi] = original_ftYs_Init_OnItemDrop;
    ftData_OnItemPickup[Ft_Kind_CustomFirst] = policy->on_item_pickup;
    ftData_OnItemPickupExt[Ft_Kind_CustomFirst] = policy->on_item_pickup_ext;
    ftData_OnItemDrop[Ft_Kind_CustomFirst] = policy->on_item_drop;
    ftData_OnItemDropExt[Ft_Kind_CustomFirst] = policy->on_item_drop_ext;
    static const int flags[] = {-2,0,1,2};
    for (unsigned held=0; held<2; ++held) for (unsigned auxiliary=0; auxiliary<2; ++auxiliary)
    for (unsigned secondary=0; secondary<2; ++secondary) for (unsigned flag=0; flag<4; ++flag) {
        heavy_item = secondary;
        hold_kind = (held + auxiliary + flag) % 6;
        initialize(held,auxiliary,secondary);
        original_ftpickupitem_80094818(&objects[0], flags[flag]); remember();
        ftpickupitem_80094818(&objects[1], flags[flag]); compare();
        initialize(held,auxiliary,secondary);
        original_ftCommon_8007E7E4(&objects[0], flags[flag]); remember();
        ftCommon_8007E7E4(&objects[1], flags[flag]); compare();
        initialize(held,auxiliary,secondary);
        original_ftCommon_8007E79C(&objects[0], flags[flag]); remember();
        ftCommon_8007E79C(&objects[1], flags[flag]); compare();
        for (unsigned special=0; special<2; ++special) for (hammer=false;;hammer=true) {
            initialize(held,auxiliary,secondary);
            original_ftCommon_8007E6DC(&objects[0], &items[0][special], flags[flag]); remember();
            ftCommon_8007E6DC(&objects[1], &items[1][special], flags[flag]); compare();
            if (hammer) break;
        }
    }
    printf("Yoshi ordinary/extended item original-C comparisons: %u exact samples.\n", comparisons);
    FreeLibrary(library);
    return 0;
}
