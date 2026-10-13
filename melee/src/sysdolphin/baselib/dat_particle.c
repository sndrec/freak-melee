#include "dat_particle.h"

#include "dat_import_internal.h"

static bool resolve_bank_offset(HSD_DatImport* import, u32 bank_offset,
                                u32 slot, u32* target,
                                bool allow_invalid_nonzero)
{
    u32 relative;
    if (!HSD_DatReadU32(import->dat, slot, &relative)) {
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        return false;
    }
    if (relative == 0) {
        *target = 0;
        return true;
    }
    if (bank_offset > import->dat->data_size ||
        relative > import->dat->data_size - bank_offset)
    {
        if (allow_invalid_nonzero) {
            *target = 0;
            return true;
        }
        import->status = HSD_DAT_DESCRIPTOR;
        import->error_offset = slot;
        return false;
    }
    *target = bank_offset + relative;
    return true;
}

static bool import_commands(HSD_DatImport* import, u32 bank_offset,
                            HSD_PSDataBank* bank)
{
    const HSD_Dat* dat = import->dat;
    u16 version;
    u32 first;
    u32 count;
    u32 table_offset;
    u32 total;
    u32 i;

    if (!HSD_DatReadU16(dat, bank_offset, &version)) {
        return false;
    }
    if (version == 0) {
        first = 0;
        if (!HSD_DatReadU32(dat, bank_offset + 4, &count)) {
            return false;
        }
        table_offset = bank_offset + 8;
    } else if (version >= 0x40 && version <= 0x43) {
        if (!HSD_DatReadU32(dat, bank_offset + 4, &first) ||
            !HSD_DatReadU32(dat, bank_offset + 8, &count) ||
            first > 0x10000U || count > 0x10000U - first)
        {
            return false;
        }
        table_offset = bank_offset + 12;
    } else {
        return false;
    }
    total = first + count;
    if (total == 0 || total > 0x10000U) {
        return false;
    }

    bank->commands = HSD_DatImportAlloc(
        import, (size_t) total * sizeof(*bank->commands));
    if (bank->commands == NULL) {
        return false;
    }
    bank->command_count = (s32) total;
    for (i = 0; i < count; ++i) {
        u32 source;
        HSD_PSCmdList* command;
        if (!resolve_bank_offset(import, bank_offset, table_offset + i * 4,
                                 &source, false)) {
            return false;
        }
        if (source == 0) {
            continue;
        }
        if (HSD_DatBytes(dat, source, 61) == NULL) {
            return false;
        }
        command = HSD_DatImportAlloc(import, sizeof(*command));
        if (command == NULL ||
            !HSD_DatReadU16(dat, source, &command->type) ||
            !HSD_DatReadU16(dat, source + 2, &command->texGroup) ||
            !HSD_DatReadU16(dat, source + 4, &command->genLife) ||
            !HSD_DatReadU16(dat, source + 6, &command->life) ||
            !HSD_DatReadU32(dat, source + 8, &command->kind) ||
            !HSD_DatReadF32(dat, source + 12, &command->grav) ||
            !HSD_DatReadF32(dat, source + 16, &command->fric) ||
            !HSD_DatReadF32(dat, source + 20, &command->vx) ||
            !HSD_DatReadF32(dat, source + 24, &command->vy) ||
            !HSD_DatReadF32(dat, source + 28, &command->vz) ||
            !HSD_DatReadF32(dat, source + 32, &command->radius) ||
            !HSD_DatReadF32(dat, source + 36, &command->angle) ||
            !HSD_DatReadF32(dat, source + 40, &command->random) ||
            !HSD_DatReadF32(dat, source + 44, &command->size) ||
            !HSD_DatReadF32(dat, source + 48, &command->param1) ||
            !HSD_DatReadF32(dat, source + 52, &command->param2) ||
            !HSD_DatReadF32(dat, source + 56, &command->param3))
        {
            return false;
        }
        command->kind = (command->kind & 0xF1FFFFFFU) | 0x08000000U;
        command->cmdList = (u8*) HSD_DatBytes(dat, source + 60, 1);
        bank->commands[first + i] = command;
    }
    return true;
}

static bool import_textures(HSD_DatImport* import, u32 bank_offset,
                            HSD_PSDataBank* bank)
{
    const HSD_Dat* dat = import->dat;
    u32 count;
    u32 i;
    if (!HSD_DatReadU32(dat, bank_offset, &count) || count == 0 ||
        count > 0x10000U)
    {
        return false;
    }
    bank->texture_groups = HSD_DatImportAlloc(
        import, (size_t) count * sizeof(*bank->texture_groups));
    if (bank->texture_groups == NULL) {
        return false;
    }
    bank->texture_group_count = count;
    for (i = 0; i < count; ++i) {
        u32 source;
        u32 pointer_count;
        u32 j;
        HSD_PSTexGroup* group;
        if (!resolve_bank_offset(import, bank_offset,
                                 bank_offset + 4 + i * 4, &source, false)) {
            return false;
        }
        if (source == 0) {
            continue;
        }
        if (HSD_DatBytes(dat, source, 24) == NULL) {
            return false;
        }
        group = HSD_DatImportAlloc(import, sizeof(*group));
        if (group == NULL || !HSD_DatReadU32(dat, source, &group->num) ||
            !HSD_DatReadU32(dat, source + 4, &group->fmt) ||
            !HSD_DatReadU32(dat, source + 8, &group->tlutfmt) ||
            !HSD_DatReadU32(dat, source + 12, &group->width) ||
            !HSD_DatReadU32(dat, source + 16, &group->height) ||
            !HSD_DatReadU16(dat, source + 20, &group->palnum) ||
            !HSD_DatReadU16(dat, source + 22, &group->palflag))
        {
            return false;
        }
        pointer_count = group->num;
        if (group->fmt == GX_TF_C4 || group->fmt == GX_TF_C8 ||
            group->fmt == GX_TF_C14X2)
        {
            if (group->palflag & 1) {
                pointer_count += 1;
            } else if (group->palnum != 0) {
                pointer_count += group->palnum;
            } else {
                pointer_count *= 2;
            }
        }
        if (pointer_count == 0 || pointer_count > 0x10000U ||
            HSD_DatBytes(dat, source + 24,
                         (size_t) pointer_count * 4) == NULL)
        {
            return false;
        }
        group->texTable = HSD_DatImportAlloc(
            import, (size_t) pointer_count * sizeof(*group->texTable));
        if (group->texTable == NULL) {
            return false;
        }
        for (j = 0; j < pointer_count; ++j) {
            u32 target;
            // Some original banks omit optional palette addresses and place
            // image bytes in those flexible-array slots. Texture addresses
            // remain strict; an out-of-range optional palette is absent.
            if (!resolve_bank_offset(import, bank_offset,
                                     source + 24 + j * 4, &target,
                                     j >= group->num)) {
                return false;
            }
            group->texTable[j] =
                target == 0 ? NULL : (u8*) HSD_DatBytes(dat, target, 1);
            if (target != 0 && group->texTable[j] == NULL) {
                return false;
            }
        }
        bank->texture_groups[i] = group;
    }
    return true;
}

bool HSD_DatImportParticleBank(HSD_DatImport* import, u32 command_offset,
                               u32 texture_offset, HSD_PSDataBank* bank)
{
    if (import == NULL || bank == NULL || import->status != HSD_DAT_OK) {
        return false;
    }
    bank->command_count = 0;
    bank->commands = NULL;
    bank->texture_group_count = 0;
    bank->texture_groups = NULL;
    if (!import_commands(import, command_offset, bank) ||
        !import_textures(import, texture_offset, bank))
    {
        if (import->status == HSD_DAT_OK) {
            import->status = HSD_DAT_DESCRIPTOR;
            import->error_offset = command_offset;
        }
        return false;
    }
    return true;
}
