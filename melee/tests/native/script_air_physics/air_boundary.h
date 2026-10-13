#ifndef SCRIPT_AIR_BOUNDARY_H
#define SCRIPT_AIR_BOUNDARY_H
#include <melee/ft/fighter.h>
enum {Air_Facing=1,Air_MapA,Air_MapE,Air_Environment,Air_Helper,Air_Wall,Air_Cliff,Air_Aerial};
extern unsigned air_trace[32],air_count,air_stop,air_failure;
extern bool air_contact,air_environment,air_wall,air_cliff;
extern int air_line;extern u32 air_flags;
void air_reset(void);void air_point(unsigned,HSD_GObj*);
#endif
