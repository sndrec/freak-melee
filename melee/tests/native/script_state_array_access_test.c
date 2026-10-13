#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
#include <stdio.h>

static int32_t next_value;
static unsigned index_calls;
static int32_t next_index(void) { ++index_calls; return next_value; }
#include "script_state_array_access_fixture.h"

static uint32_t bits(float value)
{ uint32_t result; memcpy(&result, &value, sizeof(result)); return result; }

int main(void)
{
    uint8_t first[512], second[512]; memset(first, 0xCD, sizeof(first)); memset(second, 0xEF, sizeof(second));
    for (int32_t index = -2; index < 18; ++index) {
        next_value = index; index_calls = 0;
        uint8_t before[512]; memcpy(before, first, sizeof(first));
        fixture_write(first, -0.0f); assert(index_calls == 1);
        index_calls = 0; float value = fixture_read(first); assert(index_calls == 1);
        if ((uint32_t)index < 16) {
            assert(bits(value) == UINT32_C(0x80000000));
            memcpy(before + 264 + 4 * index, first + 264 + 4 * index, 4);
        } else assert(bits(value) == 0);
        assert(memcmp(first, before, sizeof(first)) == 0);
        for (unsigned i = 0; i < sizeof(second); ++i) assert(second[i] == 0xEF);
    }
    script_state_array_set_bool(first, 508, 3, 4, true);
    assert(script_state_array_bool(first, 508, 3, 4));
    assert(!script_state_array_bool(first, 508, -1, 4));
    assert(!script_state_array_bool(first, 508, 4, 4));
    script_state_array_set_int(first, 0, 1, 2, INT32_MIN);
    assert(script_state_array_int(first, 0, 1, 2) == INT32_MIN);
    assert(script_state_array_int(first, 0, 2, 2) == 0);
    script_state_array_set_int(NULL, 0, 0, 1, 9);
    assert(script_state_array_int(NULL, 0, 0, 1) == 0);
    for (unsigned cycle = 0; cycle < 3; ++cycle)
        for (unsigned index = 0; index < 16; ++index)
            script_state_array_set_float(first, 264, (cycle * 16 + index) % 16, 16, cycle * 100 + index);
    for (unsigned index = 0; index < 16; ++index)
        assert(script_state_array_float(first, 264, index, 16) == 200 + index);
    puts("Actual emitted state-array access preserves bounds, bits, instances and ring wrap");
    return 0;
}
