#include <melee/ft/ft_0892.h>
#include <melee/ft/kinds/ftCommon/ftCo_Turn.h>
#include <melee/ft/kinds/ftCommon/ftCo_Jump.h>
#include <melee/ft/kinds/ftCommon/ftCo_Pass.h>
#include <melee/mp/mpcoll.h>
#include <melee/mp/mplib.h>
static u32 input_floor_flags;
static int input_floor_line;
u32 mpLineGetFlags(int line){CHECK(line==input_floor_line);return input_floor_flags;}
