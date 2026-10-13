#ifndef MELEE_PL_PLCOMMON_DATA_H
#define MELEE_PL_PLCOMMON_DATA_H

#include "forward.h"

#include <Runtime/platform.h>

enum { PL_COMMON_DATA_SIZE = 0x184 };

bool plCommonData_Import(const void* bytes, size_t size,
                         pl_804D6470_t* output);

#endif
