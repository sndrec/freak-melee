#ifndef SYSDOLPHIN_BASELIB_AXDRIVER_STATIC_H
#define SYSDOLPHIN_BASELIB_AXDRIVER_STATIC_H

#include <sysdolphin/baselib/axdriver.h> // IWYU pragma: export

static HSD_SM AXDriver_804C45A0[HSD_AX_SOUND_CAPACITY];
static HSD_SM* AXDriver_804C5920[0x40];

static u8 AXDriver_804C5A20[2][0x10]; // unknown type
/* 4D6038 */ static int AXDriver_804D6038 = -1;
/* 4D603C */ static u32 AXDriver_804D603C = 2;

/* 4D778C */ static int vidhigh;
/* 4D778C */ static int AXDriver_804D778C;
/* 4D7790 */ static HSD_SM* AXDriver_804D7790;
/* 4D7794 */ static HSD_SM* AXDriver_804D7794;
/* 4D7798 */ static void* AXDriver_804D7798;
/* 4D779C */ static u32 AXDriver_804D779C;
static HSD_SfxCommands sfx_commands;
/* 4D77C8 */ static int AXDriver_804D77C8;
/* 4D77CC */ static u32 AXDriver_804D77CC;
/* 4D77D0 */ static int AXDriver_804D77D0;
/* 4D77E0 */ static int AXDriver_804D77E0;
/* 4D77E4 */ static int AXDriver_804D77E4;
/* 4D77E8 */ static int AXDriver_804D77E8;

#endif
