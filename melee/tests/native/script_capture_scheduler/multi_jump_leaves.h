#include <melee/ft/native_fighter_jump.h>
#include <melee/ft/native_script_binding_air_physics.h>
#include <melee/ft/ft_084E.h>
#include <melee/ft/ft_0819.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/ftcliffcommon.h>
#include <melee/ft/kinds/ftCommon/ftCo_FallSpecial.h>

static NativeFighterDefinition multi_definition={.motion_state_first=341,.motion_state_count=6};
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{return kind==Ft_Kind_CustomFirst?&multi_definition:NULL;}

// The native ECB selection/mirroring and air-box body execute. The stage
// collision and environmental response are controlled, recorded leaves.
static bool box_contact,box_interrupted;
static ftCollisionBox box_observed;
static int box_direction;
static unsigned box_plain_calls,box_ledge_calls,box_environment_calls;
void mpCollSetFacingDir(CollData* coll,int direction)
{(void)coll;box_direction=direction;}
bool mpColl_8004730C(CollData* coll,ftCollisionBox* box)
{box_observed=*box;++box_plain_calls;coll->cur_pos.y=7;return box_contact;}
bool mpColl_800475F4(CollData* coll,ftCollisionBox* box)
{box_observed=*box;++box_ledge_calls;coll->cur_pos.y=9;return box_contact;}
bool ft_80081A00(Fighter_GObj* object)
{(void)object;++box_environment_calls;return box_interrupted;}

// Unrelated existing air-physics bindings remain explicitly unavailable here.
void ft_800851D0(Fighter_GObj* object){(void)object;abort();}
bool ftCommon_ResolveAirLanding(Fighter_GObj* object,bool (*callback)(Fighter_GObj*,int))
{(void)object;(void)callback;abort();}
bool ftCo_80096CC8(Fighter_GObj* object,int line){(void)object;(void)line;abort();}
bool ftCliffCommon_80081298(Fighter_GObj* object){(void)object;abort();}
struct Fighter_804D6520_t* Fighter_804D6520;
