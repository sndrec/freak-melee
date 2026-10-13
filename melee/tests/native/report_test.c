#include <sysdolphin/baselib/debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char observed[8192];
static size_t observed_size;

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static void observe(const unsigned char* data, size_t size)
{
    require(observed_size + size < sizeof(observed), "Observer buffer overflow");
    memcpy(observed + observed_size, data, size);
    observed_size += size;
    observed[observed_size] = '\0';
}

int main(void)
{
    require(freopen("native-report-output.log", "w+", stdout) != NULL,
            "Cannot capture host log");
    HSD_LogInit();
    HSD_SetReportCallback(observe);
    OSReport("frame %d: %.2f %s\n", 42, 1.25, "ready");
    const char* first = "frame 42: 1.25 ready\n";
    require(strcmp(observed, first) == 0, "Formatted report did not reach console");

    HSD_SetReportCallback(NULL);
    OSReport("host only\n");
    require(strcmp(observed, first) == 0, "Detached observer was invoked");

    observed_size = 0;
    observed[0] = '\0';
    char oversized[6001];
    memset(oversized, 'x', sizeof(oversized) - 1);
    oversized[sizeof(oversized) - 1] = '\0';
    HSD_SetReportCallback(observe);
    OSReport("%s", oversized);
    require(strstr(observed, "console preview truncated") != NULL,
            "Oversized console preview did not identify truncation");

    require(fseek(stdout, 0, SEEK_SET) == 0, "Cannot read captured host log");
    char output[8192];
    size_t length = fread(output, 1, sizeof(output) - 1, stdout);
    output[length] = '\0';
    size_t prefix_length = strlen(first) + strlen("host only\n");
    require(length == prefix_length + 6000, "Host log dropped report bytes");
    require(strncmp(output, first, strlen(first)) == 0, "Host formatting differs");
    require(strcmp(output + prefix_length, oversized) == 0,
            "Host log truncated an oversized report");
    fclose(stdout);
    return 0;
}
