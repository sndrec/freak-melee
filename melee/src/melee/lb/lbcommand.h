#ifndef _lbcommands_h_
#define _lbcommands_h_

#include <Runtime/platform.h>

#include <melee/lb/forward.h>

#ifdef MELEE_NATIVE
// Seeking and animation speed can make several timed groups due at once.
// Leave the next command in place when an update has used its allowance.
enum { LB_COMMAND_UPDATE_LIMIT = 4096 };
#endif

void Command_00(CommandInfo* info);
void Command_01(CommandInfo* info);
void Command_02(CommandInfo* info);
void Command_03(CommandInfo* info);
void Command_04(CommandInfo* info);
void Command_05(CommandInfo* info);
void Command_14(CommandInfo* info);
void Command_06(CommandInfo* info);
void Command_07(CommandInfo* info);
void Command_08(CommandInfo* info);
void Command_09(CommandInfo* info);
bool Command_Execute(CommandInfo* info, u32 command);

#endif
