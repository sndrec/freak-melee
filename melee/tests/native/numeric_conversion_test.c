#include <stdint.h>
#include <stdio.h>

static int expect_u32(const char* label, float input, uint32_t expected)
{
    uint32_t actual = (uint32_t) input;
    if (actual == expected) {
        return 0;
    }

    fprintf(stderr, "%s: %.9g converted to %u, expected %u\n", label,
            input, actual, expected);
    return 1;
}

static int expect_u32_from_double(const char* label, double input,
                                  uint32_t expected)
{
    uint32_t actual = (uint32_t) input;
    if (actual == expected) {
        return 0;
    }

    fprintf(stderr, "%s: %.17g converted to %u, expected %u\n", label,
            input, actual, expected);
    return 1;
}

int main(void)
{
    int failures = 0;

    failures += expect_u32("negative fraction", -0.25f, 0);
    failures += expect_u32("negative integer", -128.0f, 0);
    failures += expect_u32("zero", 0.0f, 0);
    failures += expect_u32("positive fraction", 123.75f, 123);
    failures += expect_u32("largest representable value below 2^32",
                           4294967040.0f, UINT32_C(0xFFFFFF00));
    failures += expect_u32("2^32", 4294967296.0f, UINT32_MAX);
    failures += expect_u32_from_double("negative double", -0.25, 0);
    failures += expect_u32_from_double("fractional double", 123.75, 123);
    failures += expect_u32_from_double("double 2^32", 4294967296.0,
                                       UINT32_MAX);

    if (failures != 0) {
        return 1;
    }

    puts("native numeric conversion test passed");
    return 0;
}
