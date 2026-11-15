#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sysdolphin/baselib/audio_rollback.h>

static int stopped[8];
static int stopped_count;

bool AXDriverKeyOff(int instance)
{
    stopped[stopped_count++] = instance;
    return true;
}

static void require(bool result, const char* message)
{
    if (!result) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

int main(void)
{
    int instance;
    HSD_AudioRollbackReset();
    require(HSD_AudioRollbackBeginFrame(10, false), "begin stable frame");
    require(!HSD_AudioRollbackReuseStart(100, 0, 7, &instance),
            "first execution does not reuse");
    HSD_AudioRollbackRecordStart(100, 0, 7, 400);
    HSD_AudioRollbackRecordStart(101, 1, 7, 401);
    require(HSD_AudioRollbackEndFrame(), "end stable frame");
    require(HSD_AudioRollbackFrameHasStarts(10), "stable starts query");
    require(!HSD_AudioRollbackFrameHasStarts(9), "expired starts query");

    require(HSD_AudioRollbackBeginFrame(10, true), "begin replayed frame");
    require(HSD_AudioRollbackReuseStart(100, 0, 7, &instance) &&
                instance == 400,
            "matching start reuses live instance");
    require(!HSD_AudioRollbackReuseStart(102, 0, 7, &instance),
            "new start is not reused");
    HSD_AudioRollbackRecordStart(102, 0, 7, 402);
    require(HSD_AudioRollbackEndFrame(), "end replayed frame");
    require(stopped_count == 1 && stopped[0] == 401,
            "canceled timeline start stopped");

    HSD_AudioRollbackStats stats = HSD_AudioRollbackGetStats();
    require(stats.reused_starts == 1 && stats.new_starts == 1 &&
                stats.canceled_starts == 1,
            "audio reconciliation statistics");

    require(HSD_AudioRollbackBeginFrame(10, true),
            "corrected frame became stable");
    require(HSD_AudioRollbackReuseStart(102, 0, 7, &instance) &&
                instance == 402,
            "new corrected start persisted");
    HSD_AudioRollbackAbortFrame();

    HSD_AudioRollbackReset();
    require(HSD_AudioRollbackBeginFrame(20, false),
            "begin duplicate stable frame");
    HSD_AudioRollbackRecordStart(200, 0, 3, 500);
    HSD_AudioRollbackRecordStart(200, 0, 3, 501);
    require(HSD_AudioRollbackEndFrame(), "end duplicate stable frame");
    require(HSD_AudioRollbackBeginFrame(20, true),
            "begin duplicate replayed frame");
    require(HSD_AudioRollbackReuseStart(200, 0, 3, &instance) &&
                instance == 500,
            "first duplicate preserves order");
    require(HSD_AudioRollbackReuseStart(200, 0, 3, &instance) &&
                instance == 501,
            "second duplicate preserves order");
    require(HSD_AudioRollbackEndFrame(), "end duplicate replayed frame");

    require(HSD_AudioRollbackBeginFrame(21, false), "begin overflow frame");
    for (int i = 0; i < 40; ++i) {
        HSD_AudioRollbackRecordStart(300 + i, 0, 3, 600 + i);
    }
    require(!HSD_AudioRollbackEndFrame(), "bounded start log rejects overflow");
    puts("Rollback sound starts reuse live instances and cancel stale starts");
    return 0;
}
