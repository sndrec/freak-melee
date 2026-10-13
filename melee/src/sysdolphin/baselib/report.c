#include "debug.h"

#include <stdarg.h>
#include <stdio.h>

/* The GameCube build keeps its MSL stream hook in debug.c. Native builds
 * dispatch reports explicitly without modifying CRT-private FILE members. */
#ifdef MELEE_NATIVE
static ReportCallback report_callback;

void HSD_LogInit(void)
{
    /* Reports must survive a crash even when stdout is redirected to a file. */
    setvbuf(stdout, NULL, _IONBF, 0);
}

void HSD_SetReportCallback(ReportCallback callback)
{
    report_callback = callback;
}

void OSReport(char* format, ...)
{
    va_list args;
    va_start(args, format);
    /* Always retain the complete message in the host log. */
    vfprintf(stdout, format, args);
    va_end(args);

    ReportCallback callback = report_callback;
    if (callback != NULL) {
        /* The in-game diagnostic console has bounded storage. Keep its preview
         * allocation-free and identify oversized messages explicitly. */
        unsigned char preview[4096];
        va_start(args, format);
        int length = vsnprintf((char*) preview, sizeof(preview), format, args);
        va_end(args);
        if (length > 0) {
            size_t count = (size_t) length;
            if (count >= sizeof(preview)) {
                count = sizeof(preview) - 1;
            }
            callback(preview, count);
            if ((size_t) length >= sizeof(preview)) {
                static const unsigned char marker[] =
                    "\n[console preview truncated; full message in host log]\n";
                callback(marker, sizeof(marker) - 1);
            }
        }
    }
}
#endif
