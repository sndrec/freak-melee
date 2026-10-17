#include <melee/ft/native_fighter_skin_weights.h>

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "skin weights line %u: %s\n", __LINE__, #x); exit(1); \
} } while (0)

static void valid(const NativeSkinWeights* skin)
{
    CHECK(skin->count > 0 && skin->count <= NATIVE_SKIN_MAX_INFLUENCES);
    double total = 0;
    for (unsigned i = 0; i < skin->count; ++i) {
        CHECK(isfinite(skin->weights[i]) && skin->weights[i] > 0);
        CHECK(i == 0 || skin->joints[i - 1] < skin->joints[i]);
        total += skin->weights[i];
    }
    CHECK(fabs(total - 1) < 0.000002);
}

int main(void)
{
    NativeSkinWeights skin = { .count = 5, .joints = { 9, 3, 9, 12, 7 },
        .weights = { .1234567f, .25f, .2f, .4265433f, 0 } };
    CHECK(nativeSkinWeights_Normalize(&skin));
    valid(&skin);
    CHECK(skin.count == 3);
    CHECK(fabsf(nativeSkinWeights_Get(&skin, 9) - .3234567f) < .0000001f);
    const float a = nativeSkinWeights_Get(&skin, 3);
    const float b = nativeSkinWeights_Get(&skin, 12);
    CHECK(nativeSkinWeights_Set(&skin, 17, .173245f));
    valid(&skin);
    CHECK(skin.count == 4);
    CHECK(fabsf(nativeSkinWeights_Get(&skin, 17) - .173245f) < .0000001f);
    CHECK(fabsf(nativeSkinWeights_Get(&skin, 3) / nativeSkinWeights_Get(&skin, 12) - a / b) < .000001f);
    CHECK(nativeSkinWeights_Set(&skin, 17, 0));
    valid(&skin);
    CHECK(skin.count == 3);
    CHECK(fabsf(nativeSkinWeights_Get(&skin, 3) - a) < .000001f);
    CHECK(fabsf(nativeSkinWeights_Get(&skin, 12) - b) < .000001f);

    skin = (NativeSkinWeights){ .count = 1, .joints = { 4 }, .weights = { 1 } };
    CHECK(nativeSkinWeights_Set(&skin, 4, 0));
    CHECK(skin.count == 1 && skin.weights[0] == 1);
    CHECK(nativeSkinWeights_Set(&skin, 8, .000123f));
    valid(&skin);
    CHECK(nativeSkinWeights_Get(&skin, 8) > .00012f);
    CHECK(nativeSkinWeights_Get(&skin, 8) < .00013f);
    CHECK(nativeSkinWeights_Set(&skin, 22, 1));
    CHECK(skin.count == 1 && skin.joints[0] == 22 && skin.weights[0] == 1);

    skin.count = NATIVE_SKIN_MAX_INFLUENCES;
    for (unsigned i = 0; i < skin.count; ++i) {
        skin.joints[i] = (uint16_t) (2 * i);
        skin.weights[i] = 1.0f / skin.count;
    }
    NativeSkinWeights before = skin;
    CHECK(!nativeSkinWeights_Set(&skin, 99, .25f));
    CHECK(!memcmp(&before, &skin, sizeof(skin)));
    CHECK(nativeSkinWeights_Set(&skin, 30, .223456f));
    valid(&skin);
    CHECK(skin.count == NATIVE_SKIN_MAX_INFLUENCES);

    const float invalid[] = { NAN, INFINITY, -INFINITY, -1 };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i) {
        NativeSkinWeights bad = skin;
        bad.weights[2] = invalid[i];
        before = bad;
        CHECK(!nativeSkinWeights_Normalize(&bad));
        CHECK(!memcmp(&before, &bad, sizeof(bad)));
        before = skin;
        CHECK(!nativeSkinWeights_Set(&skin, 8, invalid[i]));
        CHECK(!memcmp(&before, &skin, sizeof(skin)));
    }
    NativeSkinWeights bad = { .count = 1 };
    CHECK(!nativeSkinWeights_Normalize(&bad));
    bad.count = NATIVE_SKIN_MAX_INFLUENCES + 1;
    CHECK(!nativeSkinWeights_Normalize(&bad));
    skin = (NativeSkinWeights){ .count = 2, .joints = { 1, 2 },
        .weights = { FLT_MIN, FLT_MAX } };
    CHECK(nativeSkinWeights_Normalize(&skin));
    valid(&skin);
    CHECK(skin.count == 1 && skin.joints[0] == 2);

    // Repeated brush-like edits retain every other bone's relative influence.
    uint32_t seed = 0x78359a12;
    for (unsigned sample = 0; sample < 1000; ++sample) {
        skin = (NativeSkinWeights){ .count = 3 + sample % 14 };
        for (unsigned i = 0; i < skin.count; ++i) {
            seed = seed * 1664525 + 1013904223;
            skin.joints[i] = (uint16_t) i;
            skin.weights[i] = 1 + (seed & 0xffff);
        }
        CHECK(nativeSkinWeights_Normalize(&skin));
        const float ratio = skin.weights[0] / skin.weights[1];
        const unsigned target = skin.count - 1;
        CHECK(nativeSkinWeights_Set(&skin, (uint16_t) target, .1234567f));
        valid(&skin);
        CHECK(fabsf(skin.weights[0] / skin.weights[1] - ratio) <= fmaxf(.000002f, ratio * .000001f));
        CHECK(fabsf(skin.weights[target] - .1234567f) < .0000001f);
    }
    printf("PASS: %u skin normalization, precision, proportional editing and capacity checks\n", checks);
    return 0;
}
