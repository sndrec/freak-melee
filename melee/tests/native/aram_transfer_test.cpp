#include <aurora/aurora.h>
extern "C" {
#include <dolphin/ar.h>
#include <dolphin/os.h>
}
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace aurora { extern AuroraConfig g_config; }
static ARQRequest* completed;
static unsigned completions;
static void on_complete(ARQRequest* request)
{
    completed = request;
    ++completions;
}
static void require(bool condition, const char* message)
{
    if (!condition) { std::fputs(message, stderr); std::exit(1); }
}

int main()
{
    // Exercise the actual Aurora AR provider with Melee's public declarations.
    // This memory-only path requires no renderer or window initialization.
    aurora::g_config.mem1Size = 128 * 1024 * 1024;
    aurora::g_config.mem2Size = 16 * 1024 * 1024;
    OSInit();
    const uintptr_t arena_lo = reinterpret_cast<uintptr_t>(OSGetArenaLo());
    const uintptr_t arena_hi = reinterpret_cast<uintptr_t>(OSGetArenaHi());
    require(arena_lo > 0xFFFFFFFFULL && arena_hi > arena_lo,
            "native startup arena\n");
    require(arena_hi - arena_lo >= 127 * 1024 * 1024,
            "startup arena capacity\n");
    require(OSSecondsToTicks(1) == 40500000 &&
                OSTicksToMilliseconds(OSSecondsToTicks(1)) == 1000,
            "native clock conversion\n");
    u32 allocation_sizes[4];
    require(ARInit(allocation_sizes, 4) == 0x4000, "ARAM initialization\n");
    require(ARAlloc(32) == 0x4000 && ARAlloc(64) == 0x4020,
            "ARAM allocation stack\n");
    u32 freed_size = 0;
    require(ARFree(&freed_size) == 0x4020 && freed_size == 64,
            "ARAM allocation pop\n");
    require(ARFree(&freed_size) == 0x4000 && freed_size == 32,
            "ARAM stack restoration\n");
    auto* source = static_cast<unsigned char*>(std::malloc(1024));
    auto* destination = static_cast<unsigned char*>(std::malloc(1024));
    require(source && destination && reinterpret_cast<uintptr_t>(source) > 0xFFFFFFFFULL &&
                reinterpret_cast<uintptr_t>(destination) > 0xFFFFFFFFULL, "native addresses\n");
    for (unsigned i = 0; i < 1024; ++i) source[i] = static_cast<unsigned char>(i * 19);
    std::memset(destination, 0, 1024);
    ARQRequest request{};
    ARQPostRequest(&request, 0, ARAM_DIR_MRAM_TO_ARAM, 0,
                   reinterpret_cast<uintptr_t>(source), 0x4000, 1024, on_complete);
    require(completed == &request && completions == 1, "write callback identity\n");
    ARQPostRequest(&request, 0, ARAM_DIR_ARAM_TO_MRAM, 0, 0x4000,
                   reinterpret_cast<uintptr_t>(destination), 1024, on_complete);
    require(completed == &request && completions == 2, "read callback identity\n");
    require(std::memcmp(source, destination, 1024) == 0, "native ARAM round trip\n");
    std::free(source);
    std::free(destination);
    std::puts("Native ARAM transfer ABI checks passed");
    return 0;
}
