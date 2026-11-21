#include <melee/ef/efasync.h>
#include <melee/ef/efdata.h>
#include <melee/ef/eflib.h>
#include <melee/lb/native_model_joint.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/quatlib.h>
#include <sysdolphin/baselib/dat_particle.h>
#include <sysdolphin/baselib/particle_state_internal.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

extern EF_DAT_Entry efAsync_DatEntries[51];

static void check(bool ok, unsigned line)
{
    if (!ok) { fprintf(stderr, "Effect limit check failed at line %u\n", line); exit(1); }
}
#define CHECK(expr) check(!!(expr), __LINE__)

static void check_models(HSD_GObj* owner, HSD_Joint* joint)
{
    EF_EffectDesc desc = {.model_desc.joint = joint, .model_units = EF_MODEL_UNITS_PER_TICK};
    EF_Effect* first = efLib_Create(-1, owner, &desc);
    CHECK(first && efLib_Usage.live_model_units == desc.model_units);
    u32 serial = efLib_AllocationSerial;
    CHECK(!efLib_Create(-1, owner, &desc) && efLib_AllocationSerial == serial);
    efLib_Usage.model_units_this_tick = 0;
    EF_Effect* second = efLib_Create(-1, owner, &desc);
    CHECK(second && efLib_Usage.live_model_units == EF_MODEL_LIVE_UNITS);
    efLib_Usage.model_units_this_tick = 0;
    CHECK(!efLib_Create(-1, owner, &desc) && !efLib_Usage.model_units_this_tick);
    HSD_GObjFree(first->gobj);
    EF_Effect* replacement = efLib_Create(-1, owner, &desc);
    CHECK(replacement && efLib_Usage.live_model_units == EF_MODEL_LIVE_UNITS);
    HSD_GObjFree(second->gobj);
    HSD_GObjFree(replacement->gobj);
    CHECK(!efLib_Usage.live_model_units && efLib_Usage.model_units_this_tick == desc.model_units);
}

static void check_queues(HSD_GObj* owner, HSD_Joint* joint)
{
    EF_QueuedEffect* requests[EF_MAX_QUEUED_EFFECTS];
    for (unsigned i = 0; i < EF_MAX_QUEUED_EFFECTS; ++i)
        CHECK((requests[i] = HSD_ObjAlloc(&efAsync_AllocData)));
    CHECK(!HSD_ObjAlloc(&efAsync_AllocData));
    for (unsigned i = 0; i < EF_MAX_QUEUED_EFFECTS; ++i)
        HSD_ObjFree(&efAsync_AllocData, requests[i]);
    CHECK(!efAsync_AllocData.used);
    for (unsigned i = 0; i < EF_MAX_QUEUED_EFFECTS; ++i) {
        EF_QueuedEffect* request = efAsync_AllocateRequest();
        CHECK(request);
        HSD_ObjFree(&efAsync_AllocData, request);
    }
    CHECK(!efAsync_AllocateRequest() && !efAsync_AllocData.used);
    efLib_Usage.requests_this_tick = 0;
    EF_QueuedEffect* next = efAsync_AllocateRequest();
    CHECK(next && efLib_Usage.requests_this_tick == 1);
    HSD_ObjFree(&efAsync_AllocData, next);

    HSD_AnimJoint animation = {0};
    efLib_Usage.model_units_this_tick = 0;
    EF_EffectDesc stock = {.model_desc = {.joint = joint, .animjoint = &animation}};
    efAsync_DatEntries[0].data = &stock;
    efAsync_DatEntries[0].effect_count = 1;
    efLib_LoadKind = EF_LOADKIND_SYNC;
    EF_Effect* effects[32];
    for (unsigned i = 0; i < 32; ++i)
        CHECK((effects[i] = efLib_Create(0, owner, NULL)));
    CHECK(efLib_AnimCount == 32 && !efLib_Create(0, owner, NULL));
    CHECK(efLib_Usage.live_model_units == 32 * EF_MODEL_MIN_UNITS);
    HSD_GObjFree(effects[0]->gobj);
    CHECK(efLib_AnimCount == 31 && efLib_AnimQueue[0] == effects[1]->gobj->hsd_obj);
    CHECK((effects[0] = efLib_Create(0, owner, NULL)) && efLib_AnimCount == 32);
    for (unsigned i = 0; i < 32; ++i) HSD_GObjFree(effects[i]->gobj);
    CHECK(!efLib_AnimCount && !efLib_Usage.live_model_units);
    stock.model_desc.animjoint = NULL;
    efLib_Usage.model_units_this_tick = 0;
    for (unsigned i = 0; i < EF_MODEL_UNITS_PER_TICK / EF_MODEL_MIN_UNITS; ++i) {
        EF_Effect* effect = efLib_Create(0, owner, NULL);
        CHECK(effect);
        HSD_GObjFree(effect->gobj);
    }
    CHECK(!efLib_Create(0, owner, NULL) && !efLib_Usage.live_model_units);
    efAsync_DatEntries[0].data = NULL;
    efAsync_DatEntries[0].effect_count = 0;
}

static void check_particle_bank(void)
{
    // A bank with one texture slot and one particle command, both big-endian.
    u8 data[84] = {0};
    data[7] = 1;
    data[11] = 12;
    data[72] = 0xff;
    data[79] = 1;
    HSD_Dat dat = {.data = data, .data_size = sizeof(data)};
    HSD_DatImport import;
    u64 arena[512];
    HSD_PSDataBank bank = {0};
    CHECK(HSD_DatImportInit(&import, &dat, arena, sizeof(arena), 4) == HSD_DAT_OK);
    CHECK(HSD_DatImportParticleBank(&import, 0, 76, &bank));
    CHECK(bank.command_count == 1 && bank.commands[0]->cmdList[0] == 0xff);
    HSD_PSCmdList** commands = bank.commands;
    u64 rejected_arena[512];
    data[15] = 1; // the sole texture slot has index zero
    CHECK(HSD_DatImportInit(&import, &dat, rejected_arena, sizeof(rejected_arena), 4) == HSD_DAT_OK);
    CHECK(!HSD_DatImportParticleBank(&import, 0, 76, &bank) && bank.commands == commands);
    data[15] = 0;
    data[52] = 0x7f;
    data[53] = 0x80; // positive infinity in the emission rate
    CHECK(HSD_DatImportInit(&import, &dat, rejected_arena, sizeof(rejected_arena), 4) == HSD_DAT_OK);
    CHECK(!HSD_DatImportParticleBank(&import, 0, 76, &bank) && bank.commands == commands);
}

static void check_particle_work(void)
{
    u8 end[] = {0xff};
    HSD_PSCmdList command = {.genLife = 10, .life = 2, .cmdList = end};
    HSD_PSCmdList* commands[] = {&command};
    HSD_PSTexGroup* textures[] = {NULL};
    HSD_PSDataBank bank = {1, commands, 1, textures};
    hsd_80398A08(0);
    hsd_8039D354(0);
    psInitDataBankNative(2, &bank);

    HSD_Generator* first = hsd_8039F6CC(0, 2, 0, NULL);
    HSD_Generator* second = hsd_8039F6CC(0, 2, 0, NULL);
    CHECK(first && second && hsd_804D78F4 && hsd_804D78F4->next);
    hsd_8039D4DC(first);
    CHECK(hsd_804D78F4 && hsd_804D78F4->data == second && !hsd_804D78F4->next);
    hsd_8039D4DC(second);
    CHECK(!hsd_804D78F4 && !hsd_804D78FC);
    for (unsigned i = 2; i < HSD_GENERATORS_PER_TICK; ++i)
        CHECK(hsd_8039F05C(0, 2, 0));
    CHECK(!hsd_8039F05C(0, 2, 0));
    while (hsd_804D78E0 < HSD_GENERATORS_LIVE) {
        particle_work_this_tick.generators = 0;
        unsigned available = HSD_GENERATORS_LIVE - hsd_804D78E0;
        for (unsigned i = 0; i < available && i < HSD_GENERATORS_PER_TICK; ++i)
            CHECK(hsd_8039F05C(0, 2, 0));
    }
    particle_work_this_tick.generators = 0;
    CHECK(!hsd_8039F05C(0, 2, 0));
    HSD_Generator* emitter = hsd_804D78FC;
    emitter->type = 1; // line emitter
    emitter->count = 3.5f;
    particle_work_this_tick.emissions = HSD_PARTICLES_PER_TICK - 2;
    CHECK(hsd_8039DAD4(emitter) == 0.5f && particle_work_this_tick.emissions == HSD_PARTICLES_PER_TICK);
    CHECK(particle_work_this_tick.particles == 2);
    emitter->count = 1.25f;
    CHECK(hsd_8039DAD4(emitter) == 0.25f && particle_work_this_tick.particles == 2);
    hsd_8039CEAC(0);
    CHECK(!particle_count);
    while (hsd_804D78FC) hsd_8039D4DC(hsd_804D78FC);
    CHECK(!hsd_804D78E0);

    while (particle_count < HSD_PARTICLES_LIVE) {
        particle_work_this_tick.particles = 0;
        unsigned available = HSD_PARTICLES_LIVE - particle_count;
        for (unsigned i = 0; i < available && i < HSD_PARTICLES_PER_TICK; ++i)
            CHECK(psGenerateParticle0(NULL, 0, 2, 0, 0, end, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, NULL, 0));
        CHECK(!psGenerateParticle0(NULL, 0, 2, 0, 0, end, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, NULL, 0));
    }
    particle_work_this_tick.particles = 0;
    CHECK(!psGenerateParticle0(NULL, 0, 2, 0, 0, end, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, NULL, 0));
    hsd_8039CEAC(0);
    CHECK(!particle_count && !hsd_804D0908[0]);
    CHECK(psGenerateParticle0(NULL, 0, 2, 0, 0, end, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, NULL, 0));
    hsd_8039CEAC(0);
}

static void check_roll(void)
{
    HSD_JObj root = {.flags = JOBJ_USE_QUATERNION | JOBJ_MTX_DIRTY};
    Vec3 pose = {.x = 0.25f, .y = -0.5f, .z = 0.75f};
    EulerToQuat(&pose, &root.rotate);
    nativeModelJoint_SetRoll(&root, -1.0f);
    Mtx matrix;
    C_MTXQuat(matrix, &root.rotate);
    Vec3 out;
    HSD_QuatLib_8037EB28(matrix, &out);
    CHECK(fabsf(out.x - pose.x) < 0.0001f && fabsf(out.y - pose.y) < 0.0001f &&
          fabsf(out.z + 1.0f) < 0.0001f);
    root.rotate = (Quaternion){0};
    nativeModelJoint_SetRoll(&root, 0.0f);
    CHECK(root.rotate.w == 1.0f);
}

int main(void)
{
    const size_t bytes = 16 * 1024 * 1024;
    u8* heap = malloc(bytes);
    CHECK(heap);
    void* start = OSInitAlloc(heap, heap + bytes, 1);
    OSHeapHandle handle = OSCreateHeap(start, heap + bytes);
    CHECK(handle >= 0);
    HSD_SetHeap(handle);
    OSSetCurrentHeap(handle);
    HSD_ObjSetHeap(bytes, NULL);
    HSD_ObjInit();
    HSD_IDSetup();
    HSD_GObjLibInitDataType config = {.p_link_max = 32, .gx_link_max = 8, .gproc_pri_max = 31};
    HSD_GObj_80391304(&config);
    efLib_Init();
    HSD_GObj* owner = GObj_Create(4, 4, 0);
    CHECK(owner);
    HSD_Joint joint = {.scale = {1, 1, 1}};
    check_models(owner, &joint);
    check_queues(owner, &joint);
    check_particle_bank();
    check_particle_work();
    check_roll();
    HSD_GObjFree(owner);
    puts("Effect construction, bank validation, particle budgets, retirement, queues and authored roll passed.");
    return 0;
}
