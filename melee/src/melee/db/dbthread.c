#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "db.h"
#include <dolphin/os.h>

void db_PrintThreadInfo(void)
{
    ULONG_PTR low, high;
    unsigned char current;
    GetCurrentThreadStackLimits(&low, &high);
    OSReport("------ Native thread info ------\n");
    OSReport("thread:%lu stack low:%p high:%p current:%p size:%llu\n",
        GetCurrentThreadId(), (void*) low, (void*) high, &current,
        (unsigned long long) (high - low));
}
