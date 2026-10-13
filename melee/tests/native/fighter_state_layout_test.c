#include <melee/ft/native_fighter_state.h>
#include <melee/it/types.h>
#include <melee/gm/native_rollback_pages.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void metadata(void)
{
    NativeFighterStateField fields[] = {
        { "enabled", 0, 1, NativeFighterState_Bool, 1, false },
        { "history", 4, 16, NativeFighterState_Float, 4, true },
        { "tail", 508, 1, NativeFighterState_Int, 4, false },
    };
    assert(nativeFighterState_Validate(fields, 3, 512, 512));
    assert(!nativeFighterState_Validate(fields, 3, 511, 512));
    assert(!nativeFighterState_Validate(fields, 3, 512, 256));
    assert(nativeFighterState_Validate(NULL, 0, 0, 512));
    assert(!nativeFighterState_Validate(NULL, 1, 512, 512));
    assert(!nativeFighterState_Validate(fields, 0, 512, 512));
    NativeFighterStateField valid = fields[1];
    for (unsigned failure = 0; failure < 9; ++failure) {
        fields[1] = valid;
        switch (failure) {
        case 0: fields[1].name = "enabled"; break;
        case 1: fields[1].name = "9history"; break;
        case 2: fields[1].offset = 0; break;
        case 3: fields[1].offset = 5; break;
        case 4: fields[1].count = 0; break;
        case 5: fields[1].count = 129; break;
        case 6: fields[1].stride = 1; break;
        case 7: fields[1].type = 255; break;
        case 8: fields[1].array = false; break;
        }
        assert(!nativeFighterState_Validate(fields, 3, 512, 512));
    }
    fields[1] = valid;
    fields[1].count = 1;
    assert(nativeFighterState_Validate(fields, 3, 512, 512));
    NativeFighterStateField full = {
        "ring", 0, 128, NativeFighterState_Float, 4, true
    };
    assert(nativeFighterState_Validate(&full, 1, 512, 512));
    char long_name[65]; memset(long_name, 'a', 64); long_name[64] = 0;
    full.name = long_name;
    assert(!nativeFighterState_Validate(&full, 1, 512, 512));
}

static void item_restore(void)
{
    SYSTEM_INFO info; GetSystemInfo(&info);
    size_t bytes = (sizeof(Item) * 2 + info.dwPageSize - 1) &
                   ~(size_t)(info.dwPageSize - 1);
    Item* items = VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
    assert(items != NULL);
    // Include the final array element and fields after the grown item union.
    size_t state_bytes = sizeof(items[0].xDD4_itemVar.native_article.state);
    assert(state_bytes == 512);
    items[0].xDD4_itemVar.native_article.state[state_bytes - 1] = 11;
    items[1].xDD4_itemVar.native_article.state[state_bytes - 1] = 22;
    items[0].xDD4_itemVar.native_article.creator_spawn_id = 77;
    items[1].xDD4_itemVar.native_article.creator_spawn_id = 91;
    unsigned char* expected = malloc(bytes);
    assert(expected != NULL);
    memcpy(expected, items, bytes);
    NativeRollbackPagedRegion region = { items, bytes };
    assert(nativeRollbackPages_Init(&region, 1, NULL, 0, 4, 16));
    assert(nativeRollbackPages_BeginFrame(10));
    memset(items[0].xDD4_itemVar.native_article.state, 0xAB, state_bytes);
    items[0].xDD4_itemVar.native_article.creator_spawn_id = 0;
    memset((unsigned char*)&items[0] +
               offsetof(Item, xDD4_itemVar) + sizeof(items[0].xDD4_itemVar),
           0x91, sizeof(Item) - offsetof(Item, xDD4_itemVar) -
                     sizeof(items[0].xDD4_itemVar));
    assert(nativeRollbackPages_EndFrame());
    assert(nativeRollbackPages_BeginFrame(11));
    memset(items[1].xDD4_itemVar.native_article.state, 0xCD, state_bytes);
    items[1].xDD4_itemVar.native_article.creator_spawn_id = 101;
    assert(nativeRollbackPages_EndFrame());
    assert(nativeRollbackPages_ToFrame(10, 12));
    assert(memcmp(items, expected, bytes) == 0);
    nativeRollbackPages_Shutdown();
    free(expected);
    VirtualFree(items, 0, MEM_RELEASE);
}

int main(void)
{
    metadata(); item_restore();
    puts("Typed state metadata and expanded Item rollback restore passed");
    return 0;
}
