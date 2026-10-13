#include "efassets.h"

#include <sysdolphin/baselib/dat_import_internal.h>

#include <stdint.h>
#include <math.h>

enum {
    EFFECT_SOURCE_HEADER_SIZE = 8,
    EFFECT_SOURCE_DESC_SIZE = 20,
    EFFECT_IMPORT_EXTRA_BYTES = 2 * 1024 * 1024,
};

static void* import_graph_reference(HSD_DatImport* import, u32 slot,
                                    HSD_DatImportType type)
{
    const char* external;
    HSD_DatReferenceKind kind;
    u32 offset;
    kind = HSD_DatResolve(import->dat, slot, &offset, &external);
    if (kind == HSD_DAT_REF_NULL) {
        return NULL;
    }
    if (kind != HSD_DAT_REF_LOCAL) {
        import->status = kind == HSD_DAT_REF_EXTERNAL ? HSD_DAT_EXTERNAL
                                                       : HSD_DAT_RELOCATION;
        import->error_offset = slot;
        import->external_symbol = external;
        return NULL;
    }
    return HSD_DatImportGraph(import, offset, type);
}

static void decode_model(HSD_DatImport* import, u32 offset,
                         StaticModelDesc* model)
{
    model->joint = import_graph_reference(import, offset, DAT_JOINT);
    model->animjoint =
        import_graph_reference(import, offset + 4, DAT_ANIMJOINT);
    model->matanim_joint =
        import_graph_reference(import, offset + 8, DAT_MATANIM_JOINT);
    model->shapeanim_joint =
        import_graph_reference(import, offset + 12, DAT_SHAPE_JOINT);
}

static u32 count_effects(const HSD_Dat* dat, u32 root_offset,
                         u32 command_offset)
{
    u32 count = 0;
    for (u32 source = root_offset + EFFECT_SOURCE_HEADER_SIZE;
         source <= command_offset &&
         command_offset - source >= EFFECT_SOURCE_DESC_SIZE;
         source += EFFECT_SOURCE_DESC_SIZE)
    {
        const char* external;
        f32 lifetime;
        u32 target;
        if (!HSD_DatReadF32(dat, source, &lifetime) || !isfinite(lifetime) ||
            HSD_DatResolve(dat, source + 4, &target, &external) !=
                HSD_DAT_REF_LOCAL)
        {
            break;
        }
        for (u32 field = 2; field < 5; ++field) {
            HSD_DatReferenceKind kind = HSD_DatResolve(
                dat, source + field * 4, &target, &external);
            if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) {
                return 0;
            }
        }
        ++count;
    }
    return count;
}

static EF_ImportedData* import_failure(HSD_DatResult failure, u32 offset,
                                       HSD_DatResult* status,
                                       u32* error_offset)
{
    *status = failure;
    *error_offset = offset;
    return NULL;
}

EF_ImportedData* efAssets_Import(const void* bytes, size_t size,
                                 const char* root_name, void* arena,
                                 size_t arena_size, HSD_DatResult* status,
                                 u32* error_offset)
{
    HSD_Dat dat;
    HSD_DatImport import;
    EF_ImportedData* output;
    const char* external;
    u32 command_offset;
    u32 max_records;
    u32 root_offset;
    u32 texture_offset;
    HSD_DatReferenceKind command_kind;
    HSD_DatReferenceKind texture_kind;
    u32 i;

    if (root_name == NULL || status == NULL || error_offset == NULL) {
        return NULL;
    }
    *status = HSD_DAT_OK;
    *error_offset = 0;
    *status = HSD_DatOpen(&dat, bytes, size);
    if (*status != HSD_DAT_OK) {
        return NULL;
    }
    if (!HSD_DatFindRoot(&dat, root_name, &root_offset)) {
        return import_failure(HSD_DAT_SYMBOL, 0, status, error_offset);
    }
    command_kind =
        HSD_DatResolve(&dat, root_offset, &command_offset, &external);
    texture_kind =
        HSD_DatResolve(&dat, root_offset + 4, &texture_offset, &external);
    if ((command_kind == HSD_DAT_REF_LOCAL) !=
        (texture_kind == HSD_DAT_REF_LOCAL))
    {
        return import_failure(HSD_DAT_RELOCATION, root_offset, status,
                              error_offset);
    }
    if (command_kind != HSD_DAT_REF_LOCAL &&
        (command_kind != HSD_DAT_REF_NULL ||
         texture_kind != HSD_DAT_REF_NULL))
    {
        return import_failure(HSD_DAT_RELOCATION, root_offset, status,
                              error_offset);
    }
    if (command_kind == HSD_DAT_REF_NULL) {
        command_offset = dat.data_size;
    }
    if (command_offset <= root_offset + EFFECT_SOURCE_HEADER_SIZE)
    {
        return import_failure(HSD_DAT_DESCRIPTOR, root_offset, status,
                              error_offset);
    }

    if (dat.relocation_count > (0x20000000U - 1024) / 2 ||
        size > (SIZE_MAX - EFFECT_IMPORT_EXTRA_BYTES) / 4)
    {
        return import_failure(HSD_DAT_CAPACITY, 0, status, error_offset);
    }
    max_records = dat.relocation_count * 2 + 1024;
    *status = HSD_DatImportInit(&import, &dat, arena, arena_size, max_records);
    if (*status != HSD_DAT_OK) {
        return NULL;
    }

    output = HSD_DatImportAlloc(&import, sizeof(*output));
    if (output == NULL) {
        return import_failure(import.status, import.error_offset, status,
                              error_offset);
    }
    if (command_kind == HSD_DAT_REF_LOCAL &&
        !HSD_DatImportParticleBank(&import, command_offset, texture_offset,
                                   &output->particle_bank))
    {
        return import_failure(import.status, import.error_offset, status,
                              error_offset);
    }
    output->effect_count = count_effects(&dat, root_offset, command_offset);
    if (output->effect_count == 0) {
        if (output->particle_bank.command_count == 0) {
            return import_failure(HSD_DAT_DESCRIPTOR, root_offset, status,
                                  error_offset);
        }
        return output;
    }
    output->effects = HSD_DatImportAlloc(
        &import, (size_t) output->effect_count * sizeof(*output->effects));
    if (output->effects == NULL) {
        return import_failure(import.status, import.error_offset, status,
                              error_offset);
    }
    for (i = 0; i < output->effect_count; ++i) {
        u32 source = root_offset + EFFECT_SOURCE_HEADER_SIZE +
                     i * EFFECT_SOURCE_DESC_SIZE;
        if (!HSD_DatReadF32(&dat, source, &output->effects[i].lifetime)) {
            return import_failure(HSD_DAT_DESCRIPTOR, source, status,
                                  error_offset);
        }
        decode_model(&import, source + 4, &output->effects[i].model_desc);
        if (import.status != HSD_DAT_OK) {
            return import_failure(import.status, import.error_offset, status,
                                  error_offset);
        }
    }
    return output;
}
