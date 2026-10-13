#include "lbcolanim_data.h"

#include <stdint.h>
#include <string.h>

enum { MAX_COLOR_COMMAND_WORDS = 4096 };

static bool fail(HSD_DatResult* status, u32* error_offset,
                 HSD_DatResult result, u32 offset)
{
    *status = result;
    *error_offset = offset;
    return false;
}

static u32 pack_fields(u32 source, const u8* widths, u32 count)
{
    u32 result = 0;
    u32 source_shift = 32;
    u32 target_shift = 0;
    for (u32 i = 0; i < count; ++i) {
        u32 width = widths[i];
        source_shift -= width;
        u32 mask = width == 32 ? UINT32_MAX : (1U << width) - 1;
        result |= ((source >> source_shift) & mask) << target_shift;
        target_shift += width;
    }
    return result;
}

static u32 opcode_width(u32 opcode)
{
    switch (opcode) {
    case 5:
    case 7:
    case 13:
    case 14:
    case 15:
    case 18:
    case 19:
        return 2;
    case 21:
        return 5;
    case 22:
        return 3;
    default:
        return opcode <= 23 ? 1 : 0;
    }
}

static u32 transcode_head(u32 raw, u32 opcode)
{
    static const u8 basic[] = { 6, 26 };
    static const u8 event[] = { 6, 8, 18 };
    static const u8 light_rotation[] = { 6, 1, 1, 12, 12 };
    static const u8 light_position[] = { 6, 13, 13 };
    static const u8 spawn_gfx[] = { 6, 8, 1, 1, 1, 15 };
    static const u8 fighter_callback[] = { 6, 1, 8, 17 };
    switch (opcode) {
    case 9:
    case 22:
        return pack_fields(raw, event, 3);
    case 13:
        return pack_fields(raw, light_rotation, 5);
    case 16:
        return pack_fields(raw, light_position, 3);
    case 21:
        return pack_fields(raw, spawn_gfx, 6);
    case 23:
        return pack_fields(raw, fighter_callback, 4);
    default:
        return pack_fields(raw, basic, 2);
    }
}

static bool transcode_payload(const HSD_Dat* dat, u32 opcode, u32 word,
                              u32 source, u32* output)
{
    static const u8 halves[] = { 16, 16 };
    static const u8 sound_tail[] = { 16, 8, 8 };
    if (opcode == 21) {
        u32 raw;
        if (!HSD_DatReadU32(dat, source, &raw)) {
            return false;
        }
        *output = pack_fields(raw, halves, 2);
        return true;
    }
    if (opcode == 22) {
        u32 raw;
        if (!HSD_DatReadU32(dat, source, &raw)) {
            return false;
        }
        *output = word == 2 ? pack_fields(raw, sound_tail, 3) : raw;
        return true;
    }
    // Lighting/color payloads are GXColor byte sequences, not scalar words.
    const void* color = HSD_DatBytes(dat, source, 4);
    if (color == NULL) {
        return false;
    }
    memcpy(output, color, 4);
    return true;
}

bool lbColorScript_Import(const HSD_Dat* dat, const LbColorScriptTable* tables,
                          u32 table_count, u32 script_end,
                          union ColorOverlay_x8_t* commands,
                          u32 command_capacity, u32* command_count,
                          HSD_DatResult* status, u32* error_offset)
{
    u32 script_begin = UINT32_MAX;
    u32 pending[MAX_COLOR_COMMAND_WORDS];
    u8 visited[MAX_COLOR_COMMAND_WORDS] = { 0 };
    u8 payload[MAX_COLOR_COMMAND_WORDS] = { 0 };
    u32 pending_count = 0;

    if (dat == NULL || tables == NULL || table_count == 0 ||
        commands == NULL || command_count == NULL || status == NULL ||
        error_offset == NULL || command_capacity > MAX_COLOR_COMMAND_WORDS)
    {
        return false;
    }
    *status = HSD_DAT_OK;
    *error_offset = 0;
    *command_count = 0;
    for (u32 table_index = 0; table_index < table_count; ++table_index) {
        const LbColorScriptTable* table = &tables[table_index];
        if (table->entries == NULL || table->count == 0 ||
            HSD_DatBytes(dat, table->offset, (size_t) table->count * 8) == NULL)
        {
            return fail(status, error_offset, HSD_DAT_DESCRIPTOR,
                        table->offset);
        }
        for (u32 i = 0; i < table->count; ++i) {
            u32 slot = table->offset + i * 8;
            u32 target;
            const char* external;
            HSD_DatReferenceKind kind = HSD_DatResolve(
                dat, slot, &target, &external);
            if (kind != HSD_DAT_REF_LOCAL && kind != HSD_DAT_REF_NULL) {
                return fail(status, error_offset, HSD_DAT_RELOCATION, slot);
            }
            if (kind == HSD_DAT_REF_LOCAL) {
                if (target >= script_end || (target & 3) != 0) {
                    return fail(status, error_offset, HSD_DAT_DESCRIPTOR,
                                slot);
                }
                if (target < script_begin) {
                    script_begin = target;
                }
            }
        }
    }
    if (script_begin == UINT32_MAX || script_end <= script_begin ||
        ((script_end - script_begin) & 3) != 0 ||
        (script_end - script_begin) / 4 > command_capacity ||
        HSD_DatBytes(dat, script_begin, script_end - script_begin) == NULL)
    {
        return fail(status, error_offset, HSD_DAT_CAPACITY, script_end);
    }
    *command_count = (script_end - script_begin) / 4;
    memset(commands, 0, (size_t) *command_count * sizeof(*commands));

    for (u32 table_index = 0; table_index < table_count; ++table_index) {
        const LbColorScriptTable* table = &tables[table_index];
        for (u32 i = 0; i < table->count; ++i) {
            u32 slot = table->offset + i * 8;
            u32 target;
            const char* external;
            HSD_DatReferenceKind kind = HSD_DatResolve(
                dat, slot, &target, &external);
            const u8* metadata = HSD_DatBytes(dat, slot + 4, 2);
            if (metadata == NULL) {
                return fail(status, error_offset, HSD_DAT_DESCRIPTOR, slot);
            }
            table->entries[i].unk4 = metadata[0];
            table->entries[i].unk5 = metadata[1];
            if (kind == HSD_DAT_REF_LOCAL) {
                if (pending_count >= MAX_COLOR_COMMAND_WORDS) {
                    return fail(status, error_offset, HSD_DAT_CAPACITY, slot);
                }
                table->entries[i].unk =
                    &commands[(target - script_begin) / 4];
                pending[pending_count++] = target;
            }
        }
    }

    while (pending_count != 0) {
        u32 at = pending[--pending_count];
        while (true) {
            if (at < script_begin || at + 4 > script_end) {
                return fail(status, error_offset, HSD_DAT_DESCRIPTOR, at);
            }
            u32 index = (at - script_begin) / 4;
            if (payload[index]) {
                return fail(status, error_offset, HSD_DAT_DESCRIPTOR, at);
            }
            if (visited[index]) {
                break;
            }
            u32 raw;
            if (!HSD_DatReadU32(dat, at, &raw)) {
                return fail(status, error_offset, HSD_DAT_DESCRIPTOR, at);
            }
            u32 opcode = raw >> 26;
            u32 width = opcode_width(opcode);
            if (width == 0 || at + width * 4 > script_end) {
                return fail(status, error_offset, HSD_DAT_DESCRIPTOR, at);
            }
            visited[index] = true;
            for (u32 i = 1; i < width; ++i) {
                if (visited[index + i] || payload[index + i]) {
                    return fail(status, error_offset, HSD_DAT_DESCRIPTOR,
                                at + i * 4);
                }
                payload[index + i] = true;
            }
            u32 native = transcode_head(raw, opcode);
            memcpy(&commands[index], &native, 4);
            if (opcode == 5 || opcode == 7) {
                u32 target;
                const char* external;
                HSD_DatReferenceKind kind = HSD_DatResolve(
                    dat, at + 4, &target, &external);
                s32 relative;
                if (kind == HSD_DAT_REF_NULL) {
                    relative = INT32_MIN;
                } else if (kind == HSD_DAT_REF_LOCAL &&
                           target >= script_begin && target < script_end &&
                           (target & 3) == 0)
                {
                    relative = (s32) target - (s32) (at + 4);
                    if (pending_count >= MAX_COLOR_COMMAND_WORDS) {
                        return fail(status, error_offset, HSD_DAT_CAPACITY,
                                    at + 4);
                    }
                    pending[pending_count++] = target;
                } else {
                    return fail(status, error_offset, HSD_DAT_RELOCATION,
                                at + 4);
                }
                memcpy(&commands[index + 1], &relative, 4);
            } else {
                for (u32 i = 1; i < width; ++i) {
                    u32 value;
                    if (!transcode_payload(dat, opcode, i, at + i * 4,
                                           &value))
                    {
                        return fail(status, error_offset,
                                    HSD_DAT_DESCRIPTOR, at + i * 4);
                    }
                    memcpy(&commands[index + i], &value, 4);
                }
            }
            if (opcode == 0 || opcode == 6 || opcode == 7 || opcode == 10) {
                break;
            }
            at += width * 4;
        }
    }
    return true;
}

bool lbColorScript_ImportDirect(
    const HSD_Dat* dat, u32 pointer_slot, u32 script_end,
    union ColorOverlay_x8_t* commands, u32 command_capacity,
    u32* command_count, union ColorOverlay_x8_t** script,
    HSD_DatResult* status, u32* error_offset)
{
    struct Fighter_804D653C_t entry = { 0 };
    LbColorScriptTable table = { pointer_slot, 1, &entry };
    if (script == NULL ||
        !lbColorScript_Import(dat, &table, 1, script_end, commands,
                              command_capacity, command_count, status,
                              error_offset))
    {
        return false;
    }
    *script = entry.unk;
    return *script != NULL;
}
