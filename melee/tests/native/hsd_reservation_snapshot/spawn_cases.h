#include <melee/gr/grzakogenerator.h>
#include <melee/gr/native_stage_spawn_snapshot.h>
#include <sysdolphin/baselib/psstructs.h>

static unsigned spawn_checks, spawn_refusals, spawn_callbacks;
static size_t spawn_bytes;
static Vec3 spawn_offset;
static grZakoGenerator_Spawn spawn_definitions[4][10];
static unsigned char spawn_saved[16384], spawn_bad[16384], spawn_after[16384];
static unsigned char spawn_end[16384], spawn_repeat[16384];
void lb_8000B1CC(HSD_JObj* joint, Vec3* from, Vec3* to)
{
    CHECK(joint);
    to->x = from->x + spawn_offset.x;
    to->y = from->y + spawn_offset.y;
    to->z = from->z + spawn_offset.z;
}
f32 HSD_Randf(void) { return 0.125f; }
void grLib_801C9874(HSD_Generator* generator) { (void)generator; abort(); }
static HSD_Generator* spawn_callback(Vec3* position, s32 index)
{
    CHECK(position && index >= 0 && index < 10);
    ++spawn_callbacks;
    return NULL;
}
static void reject_spawn_candidate(size_t bytes)
{
    CHECK(!nativeStageSpawn_Validate(spawn_bad, bytes));
    CHECK(!nativeStageSpawn_Load(spawn_bad, bytes));
    size_t after;
    CHECK(nativeStageSpawn_Save(spawn_after, sizeof(spawn_after), &after));
    CHECK(after == spawn_bytes && !memcmp(spawn_saved, spawn_after, after));
    ++spawn_refusals;
}
static void test_spawn_state(void)
{
    unsigned first = checks;
    camera_idle = true;
    nativeStageSpawn_EndScene();
    stage_spawn_state.count = 0;
    const unsigned counts[] = {1, 2, 4, 10};
    grZakoGenerator_Config* configs[4];
    HSD_JObj joint = {0};
    for (unsigned i = 0; i < 4; ++i) {
        for (unsigned j = 0; j < counts[i]; ++j) {
            spawn_definitions[i][j].pos0 = (Vec3){(float)j, -1, 2};
            spawn_definitions[i][j].pos1 = (Vec3){(float)j + 2, 1, 4};
        }
        configs[i] = grZakoGenerator_801CA394(spawn_definitions[i], counts[i], spawn_callback, i ? 1.0f : 0.3f);
        CHECK(configs[i] == &stage_spawn_state.records[i].config);
        CHECK(configs[i]->spawns == stage_spawn_state.records[i].segments);
    }
    nativeStageSpawn_BeginScene();
    CHECK(nativeStageSpawn_Save(spawn_saved, sizeof(spawn_saved), &spawn_bytes));
    CHECK(spawn_bytes == sizeof(NativeStageSpawnSnapshotHeader) + 4*sizeof(grZakoGenerator_Config) + 17*sizeof(grZakoGenerator_Spawn));
    CHECK(nativeStageSpawn_SnapshotBytes() == spawn_bytes);
    size_t end_bytes;
    for (unsigned pass = 0; pass < 2; ++pass) {
        CHECK(nativeStageSpawn_Load(spawn_saved, spawn_bytes));
        spawn_callbacks = 0;
        for (unsigned frame = 0; frame < 16; ++frame) {
            spawn_offset = (Vec3){(float)frame, 0, (float)frame * 2};
            for (unsigned i = 0; i < 4; ++i) {
                configs[i]->x18 = frame;
                CHECK(grZakoGenerator_801CA43C(configs[i], &joint, 0) == (s32)counts[i]);
                CHECK(configs[i]->spawns[0].pos0.x == frame);
            }
        }
        CHECK(spawn_callbacks == 17*16);
        CHECK(nativeStageSpawn_Save(pass ? spawn_repeat : spawn_end, sizeof(spawn_end), &end_bytes));
        CHECK(end_bytes == spawn_bytes);
    }
    CHECK(!memcmp(spawn_end, spawn_repeat, spawn_bytes));
    CHECK(nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    CHECK(!grZakoGenerator_801CA43C(NULL, &joint, 0));
    CHECK(!grZakoGenerator_801CA43C(configs[0], NULL, 0));
    NativeStageSpawnSnapshotHeader header;
    memcpy(&header, spawn_saved, sizeof(header));
#define BAD_HEADER(change) do { memcpy(spawn_bad,spawn_saved,spawn_bytes); \
    NativeStageSpawnSnapshotHeader h=header; change; memcpy(spawn_bad,&h,sizeof(h)); \
    reject_spawn_candidate(spawn_bytes); } while(0)
    BAD_HEADER(++h.epoch); BAD_HEADER(h.reserved=1); BAD_HEADER(h.count=STAGE_SPAWN_CONFIGS+1);
#undef BAD_HEADER
    size_t last = sizeof(header);
    for (unsigned i = 0; i < 3; ++i)
        last += sizeof(grZakoGenerator_Config) + counts[i]*sizeof(grZakoGenerator_Spawn);
#define BAD_CONFIG(change) do { memcpy(spawn_bad,spawn_saved,spawn_bytes); \
    grZakoGenerator_Config c=*configs[3]; change; memcpy(spawn_bad+last,&c,sizeof(c)); \
    reject_spawn_candidate(spawn_bytes); } while(0)
    BAD_CONFIG(c.count=STAGE_SPAWN_SEGMENTS+1);
    BAD_CONFIG(c.spawn_descs=(void*)1); BAD_CONFIG(c.spawns=configs[0]->spawns);
    BAD_CONFIG(c.callback=NULL); BAD_CONFIG(c.gen=(void*)1); BAD_CONFIG(c.x10=0.5f);
#undef BAD_CONFIG
    memcpy(spawn_bad, spawn_saved, spawn_bytes);
    reject_spawn_candidate(spawn_bytes-1);
    memcpy(spawn_bad, spawn_saved, spawn_bytes);
    spawn_bad[spawn_bytes] = 0;
    reject_spawn_candidate(spawn_bytes+1);
    // A malformed late record must not overwrite any earlier transformed data.
    configs[0]->spawns[0].pos0.x = 777;
    CHECK(!nativeStageSpawn_Load(spawn_bad, spawn_bytes+1));
    CHECK(configs[0]->spawns[0].pos0.x == 777);
    CHECK(nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    size_t written = 991;
    memset(spawn_after, 0xCD, sizeof(spawn_after));
    CHECK(!nativeStageSpawn_Save(spawn_after, spawn_bytes-1, &written));
    CHECK(written == 991 && spawn_after[0] == 0xCD);
    CHECK(!nativeStageSpawn_Save(&stage_spawn_state, sizeof(stage_spawn_state), &written));
    CHECK(!nativeStageSpawn_Save(spawn_after, sizeof(spawn_after), (void*)spawn_definitions));
    CHECK(!nativeStageSpawn_Save(spawn_after, sizeof(spawn_after), (void*)spawn_after));
    CHECK(nativeStageSpawn_Save(spawn_after+1, sizeof(spawn_after)-1, &written));
    CHECK(nativeStageSpawn_Load(spawn_after+1, written));
    spawn_definitions[3][9].pos1.x += 1;
    CHECK(!nativeStageSpawn_SnapshotBytes());
    CHECK(!nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    spawn_definitions[3][9].pos1.x -= 1;
    // Rewinding occupancy resurrects the constructor cursor, without allocating.
    grZakoGenerator_Config* added = grZakoGenerator_801CA394(spawn_definitions[0], 1, spawn_callback, 0.3f);
    CHECK(added && stage_spawn_state.count == 5 && nativeStageSpawn_SnapshotBytes());
    CHECK(nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    CHECK(stage_spawn_state.count == 4);
    CHECK(grZakoGenerator_801CA394(spawn_definitions[0], 1, spawn_callback, 0.3f) == added);
    CHECK(nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    CHECK(!grZakoGenerator_801CA394(spawn_definitions[0], 0, spawn_callback, 1));
    CHECK(!grZakoGenerator_801CA394(spawn_definitions[0], STAGE_SPAWN_SEGMENTS+1, spawn_callback, 1));
    for (unsigned i = 4; i < STAGE_SPAWN_CONFIGS; ++i)
        CHECK(grZakoGenerator_801CA394(spawn_definitions[0], 1, spawn_callback, 0.3f));
    CHECK(!grZakoGenerator_801CA394(spawn_definitions[0], 1, spawn_callback, 0.3f));
    CHECK(stage_spawn_state.count == STAGE_SPAWN_CONFIGS);
    CHECK(nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    camera_idle = false;
    CHECK(!nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    camera_idle = true;
    nativeStageSpawn_EndScene();
    CHECK(!nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    stage_spawn_state.count = 0;
    nativeStageSpawn_BeginScene();
    CHECK(nativeStageSpawn_SnapshotBytes() == sizeof(header));
    CHECK(!nativeStageSpawn_Load(spawn_saved, spawn_bytes));
    nativeStageSpawn_EndScene();
    camera_idle = false;
    spawn_checks = checks-first;
}
