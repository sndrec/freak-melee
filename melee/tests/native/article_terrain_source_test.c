#include <melee/mp/mplib.h>
#include <melee/mp/mpisland.h>
#include <melee/mp/types.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>

// Actual mp types/globals; stage construction is the controlled boundary.
static MapCollData collision;
static MapLine descriptions[16];
static CollLine lines[16];
static CollLine* groundCollLine = lines;
static MapCollData* mpLib_804D64B4;
struct mpIsland_80458E88_t mpIsland_80458E88;
static mp_UnkStruct0 islands[4];
static unsigned checks, cases, map_reads, line_reads;
#define CHECK(value) do { ++checks; assert(value); } while (0)

MapCollData* mpLib_8004D164(void) { ++map_reads; return mpLib_804D64B4; }
CollLine* mpGetGroundCollLine(void) { ++line_reads; return groundCollLine; }

// Invalid raw lookup must fail immediately in this console test, rather than
// entering the stock diagnostic's infinite loop. No accepted case reaches it.
static void invalid_lookup(const char* format, const char* file, int line, int id)
{
    (void)format; (void)file; (void)line; (void)id;
    fputs("Invalid line reached actual raw lookup.\n", stderr);
    abort();
}
#define OSReport invalid_lookup
#include "article_terrain_originals.h"
#undef OSReport

static bool equal(Vec3 a, Vec3 b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

static void reset(void)
{
    memset(&collision, 0, sizeof(collision));
    memset(&mpIsland_80458E88, 0, sizeof(mpIsland_80458E88));
    memset(islands, 0, sizeof(islands));
    collision.line_count = 16; collision.lines = descriptions;
    mpLib_804D64B4 = &collision;
    for (unsigned i = 0; i < 16; ++i) {
        descriptions[i] = (MapLine){ .next_id0 = -1 };
        lines[i] = (CollLine){ .x0 = &descriptions[i],
                             .flags = LINE_FLAG_ENABLED | CollLine_Floor };
    }
    map_reads = line_reads = 0;
}

static void fail_query(int line, unsigned expected_map_reads)
{
    ++cases;
    Vec3 minimum = { 11, 12, 13 }, maximum = { 21, 22, 23 };
    map_reads = line_reads = 0;
    CHECK(!stage_line_island_bounds(line, &minimum, &maximum));
    CHECK(equal(minimum, (Vec3){ 0 }));
    CHECK(equal(maximum, (Vec3){ 0 }));
    CHECK(map_reads == expected_map_reads);
    CHECK(line_reads == 0);
}

static void found_query(int line, mp_UnkStruct0* expected)
{
    ++cases;
    map_reads = line_reads = 0;
    CHECK(mpIsland_8005AB54(line) == expected);
    CHECK(line_reads == 1);
    Vec3 minimum = { 11, 12, 13 }, maximum = { 21, 22, 23 };
    map_reads = line_reads = 0;
    CHECK(stage_line_island_bounds(line, &minimum, &maximum));
    CHECK(equal(minimum, expected->x8));
    CHECK(equal(maximum, expected->x14));
    CHECK(map_reads == 1 && line_reads == 1);
}

static void guards(void)
{
    reset();
    Vec3 sentinel = { 7, 8, 9 };
    ++cases; CHECK(!stage_line_island_bounds(0, NULL, &sentinel));
    CHECK(equal(sentinel, (Vec3){ 7, 8, 9 })); CHECK(!map_reads && !line_reads);
    ++cases; CHECK(!stage_line_island_bounds(0, &sentinel, NULL));
    CHECK(equal(sentinel, (Vec3){ 7, 8, 9 })); CHECK(!map_reads && !line_reads);
    ++cases; CHECK(!stage_line_island_bounds(0, NULL, NULL));
    CHECK(!map_reads && !line_reads);
    mpLib_804D64B4 = NULL;
    fail_query(0, 1);
    mpLib_804D64B4 = &collision;
    int invalid[] = { INT_MIN, -100, -2, -1, 16, 17, INT_MAX };
    for (unsigned i = 0; i < sizeof(invalid) / sizeof(*invalid); ++i)
        fail_query(invalid[i], 1);
    collision.line_count = 0; fail_query(0, 1);
    collision.line_count = -1; fail_query(0, 1);
    collision.line_count = 16;
    ++cases;
    CHECK(mpIsland_8005AB54(-1) == NULL); CHECK(!line_reads);
    // Empty island list still reads the valid line table, but has no bounds.
    Vec3 a, b;
    ++cases; CHECK(!stage_line_island_bounds(0, &a, &b));
    CHECK(equal(a, (Vec3){ 0 }) && equal(b, (Vec3){ 0 }));
    CHECK(line_reads == 1);
    lines[0].flags &= ~LINE_FLAG_ENABLED; fail_query(0, 1);
    lines[0].flags |= LINE_FLAG_ENABLED | LINE_FLAG_HIDDEN; fail_query(0, 1);
}

static void chains(void)
{
    // Three interleaved floor chains ensure the search follows links, rather
    // than assuming contiguous IDs or selecting the first island.
    for (unsigned sample = 0; sample < 64; ++sample) {
        reset();
        islands[0] = (mp_UnkStruct0){ .next = &islands[1], .x24 = 0, .x26 = 5,
            .x8 = { (float)sample + 30, -4, 3 }, .x14 = { -20, 17, -8 } };
        islands[1] = (mp_UnkStruct0){ .next = &islands[2], .x24 = 1, .x26 = 4,
            .x8 = { -6, (float)sample, 2 }, .x14 = { 9, 0, 5 } };
        islands[2] = (mp_UnkStruct0){ .x24 = 2, .x26 = 8,
            .x8 = { 7, 6, 5 }, .x14 = { -1, -2, -(float)sample } };
        mpIsland_80458E88.next = &islands[0];
        descriptions[0].next_id0 = 3; descriptions[3].next_id0 = 5;
        descriptions[1].next_id0 = 4;
        descriptions[2].next_id0 = 7; descriptions[7].next_id0 = 8;
        int ids[] = { 0, 3, 5, 1, 4, 2, 7, 8 };
        unsigned owners[] = { 0, 0, 0, 1, 1, 2, 2, 2 };
        for (unsigned i = 0; i < 8; ++i) found_query(ids[i], &islands[owners[i]]);
        // Output names preserve stock endpoint order; they do not sort axes.
        CHECK(islands[0].x8.x > islands[0].x14.x);
        // Endpoint stops traversal even if a further floor link exists.
        descriptions[5].next_id0 = 9;
        Vec3 a, b;
        ++cases; CHECK(mpIsland_8005AB54(9) == NULL);
        CHECK(!stage_line_island_bounds(9, &a, &b));
        CHECK(equal(a, (Vec3){ 0 }) && equal(b, (Vec3){ 0 }));
        // A non-floor next link terminates the candidate chain.
        lines[7].flags &= ~CollLine_Floor;
        ++cases; CHECK(mpIsland_8005AB54(8) == NULL);
        CHECK(!stage_line_island_bounds(8, &a, &b));
        CHECK(equal(a, (Vec3){ 0 }) && equal(b, (Vec3){ 0 }));
        // An absent next link likewise prevents a later endpoint match.
        descriptions[1].next_id0 = -1;
        ++cases; CHECK(mpIsland_8005AB54(4) == NULL);
        CHECK(!stage_line_island_bounds(4, &a, &b));
        CHECK(equal(a, (Vec3){ 0 }) && equal(b, (Vec3){ 0 }));
        lines[0].flags |= LINE_FLAG_HIDDEN; fail_query(0, 1);
        lines[1].flags &= ~LINE_FLAG_ENABLED; fail_query(1, 1);
        // Shared output storage follows ordinary ordered assignment semantics.
        Vec3 aliased;
        ++cases; CHECK(stage_line_island_bounds(2, &aliased, &aliased));
        CHECK(equal(aliased, islands[2].x14));
    }
}

int main(void)
{
    guards(); chains();
    printf("{\"assertions\":%u,\"cases\":%u,\"floor_chain_samples\":64}\n", checks, cases);
    return 0;
}
