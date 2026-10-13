#include <melee/ft/native_fighter_target.h>
#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
HSD_GObj** HSD_GObjPLinkHead;
#include "airborne_body.inc"
#include "airborne_cases.inc"
int main(void) { airborne_lifetime_cases(); puts("Airborne target actual source PASS"); return 0; }
