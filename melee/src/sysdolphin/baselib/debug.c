#include "debug.h"

#include <stdio.h>
#ifdef MELEE_NATIVE
#include <stdlib.h>
#include <windows.h>

static void HSD_PrintNativePanicStack(void)
{
    void* frames[32];
    USHORT count = CaptureStackBackTrace(0, 32, frames, NULL);
    fprintf(stderr, "native panic stack (exe base %p):", GetModuleHandleA(NULL));
    for (USHORT i = 0; i < count; ++i) {
        fprintf(stderr, " %p", frames[i]);
    }
    fputc('\n', stderr);
}
#endif

#include <dolphin/os.h>

#ifndef MELEE_NATIVE
struct DebugContext {
    OSContext context;
    u8 unk[0x10];
} HSD_Debug_804C2608;
#endif

static PanicCallback panicCallback;
#ifndef MELEE_NATIVE
static ReportCallback reportCallback;
static __io_proc logFunc;

#ifdef MUST_MATCH
#pragma peephole off
#endif

static int report_func(__file_handle arg0, unsigned char* arg1, size_t* arg2,
                       __idle_proc arg3)
{
    if (reportCallback != NULL) {
        reportCallback(arg1, *arg2);
    }
    logFunc(arg0, arg1, arg2, arg3);
    return 0;
}

void HSD_LogInit(void)
{
    if (logFunc == NULL) {
        logFunc = stdout->write_proc;
    }
    stdout->write_proc = report_func;
    stdout->state.error = 0;
}
#endif

void __assert(char* str, u32 arg1, char* arg2)
{
    OSReport("assertion \"%s\" failed", arg2);
    HSD_Panic(str, arg1, "");
}

void HSD_Panic(char* arg0, u32 line, char* arg2)
{
#ifdef MELEE_NATIVE
    fprintf(stderr, "%s in %s on line %lu.\n", arg2, arg0, line);
    HSD_PrintNativePanicStack();
    if (panicCallback != NULL) {
        panicCallback(arg0, line, arg2);
    }
    fflush(stdout);
    fflush(stderr);
    abort();
#else
    if (panicCallback != NULL) {
        OSSaveContext(&HSD_Debug_804C2608.context);
        OSReport("%s in %s on line %d.\n", arg2, arg0, line);
        panicCallback(&HSD_Debug_804C2608.context);
    }
    OSPanic(arg0, line, arg2);
#endif
}

#ifndef MELEE_NATIVE
void HSD_SetReportCallback(ReportCallback cb)
{
    reportCallback = cb;
}
#endif

void HSD_SetPanicCallback(PanicCallback cb)
{
    panicCallback = cb;
}
