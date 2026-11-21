#include "itcommand_data.h"
#include "native_item_model_admission.h"

#include <melee/lb/lbcommand_data.h>
#include <melee/lb/types.h>
#include <melee/it/types.h>
#include <sysdolphin/baselib/dat_import_internal.h>

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct FieldLayout {
    u8 count;
    u8 widths[8];
} FieldLayout;

#define LAYOUT(...) { sizeof((u8[]) { __VA_ARGS__ }), { __VA_ARGS__ } }
static const FieldLayout basic = LAYOUT(6, 26);
static const FieldLayout event = LAYOUT(6, 8, 18);
static const FieldLayout item_gfx = LAYOUT(6, 10, 16);
static const FieldLayout item_hitbox = LAYOUT(6, 3, 3, 7, 13);
static const FieldLayout hitbox_damage = LAYOUT(6, 3, 23);
static const FieldLayout hitbox_third = LAYOUT(9, 9, 9, 1, 1, 1, 1, 1);
static const FieldLayout item_hitbox_fourth = LAYOUT(9, 5, 1, 8, 3, 4, 1, 1);
static const FieldLayout halves = LAYOUT(16, 16);
static const FieldLayout item_command_16 = LAYOUT(6, 8, 2, 16);
static const FieldLayout pair13 = LAYOUT(6, 13, 13);
#undef LAYOUT

static bool fail(HSD_DatImport* import, HSD_DatResult result, u32 offset)
{
    import->status = result;
    import->error_offset = offset;
    return false;
}

static u32 command_width(u32 opcode, u32 raw)
{
    if (opcode < 10) return opcode == 5 || opcode == 7 ? 2 : 1;
    if (opcode == 10) return 5;
    if (opcode == 11) return 6;
    if (opcode == 16) {
        u32 subopcode = (raw >> 18) & 0xFF;
        return subopcode <= 2 || subopcode == 10 || subopcode == 11 ? 3 : 2;
    }
    return opcode <= 25 ? 1 : 0;
}

static const FieldLayout* word_layout(u32 opcode, u32 word)
{
    if (word != 0) {
        if (opcode == 10 && word <= 4) return &halves;
        if (opcode == 11) {
            if (word <= 2) return &halves;
            if (word == 3) return &hitbox_third;
            if (word == 4) return &item_hitbox_fourth;
        }
        return NULL;
    }
    switch (opcode) {
    case 9: return &event;
    case 10: return &item_gfx;
    case 11: return &item_hitbox;
    case 12:
    case 13: return &hitbox_damage;
    case 16: return &item_command_16;
    case 21:
    case 23: return &pair13;
    case 24: return &event;
    default: return &basic;
    }
}

static u32 transcode_word(u32 source, const FieldLayout* layout)
{
    if (layout == NULL) return source;
    u32 result = 0;
    u32 source_shift = 32;
    u32 target_shift = 0;
    for (u32 i = 0; i < layout->count; ++i) {
        u32 width = layout->widths[i];
        source_shift -= width;
        u32 mask = width == 32 ? UINT32_MAX : (1U << width) - 1;
        result |= ((source >> source_shift) & mask) << target_shift;
        target_shift += width;
    }
    return result;
}


enum {
    ITEM_BONE_CAPACITY = sizeof(((DynamicBoneTable*) 0)->bones) /
                         sizeof(((DynamicBoneTable*) 0)->bones[0]),
    ITEM_HITBOX_CAPACITY = sizeof(((Item*) 0)->x5D4_hitboxes) /
                           sizeof(((Item*) 0)->x5D4_hitboxes[0]),
};

static bool validate_operands(HSD_DatImport* import, const void* owner,
                              u32 at, u32 raw)
{
    const Article* article = owner;
    u32 opcode = raw >> 26;
    if (((opcode == 11 || opcode == 12 || opcode == 13) &&
         ((raw >> 23) & 7) >= ITEM_HITBOX_CAPACITY) ||
        (opcode == 14 && (raw & 0x3FFFFFF) >= ITEM_HITBOX_CAPACITY))
        return fail(import, HSD_DAT_DESCRIPTOR, at);
    const ItemModelDesc* model = article != NULL ? article->x10_modelDesc : NULL;
    if (opcode == 11) {
        u32 bone = (raw >> 13) & 127;
        if (bone != 0 && (model == NULL || model->x4_bone_count == 0 ||
                          bone >= ITEM_BONE_CAPACITY))
            return fail(import, HSD_DAT_DESCRIPTOR, at);
    }
    if (opcode == 10 && (model == NULL ||
        (model->x4_bone_count != 0 &&
         ((raw >> 16) & 1023) >= ITEM_BONE_CAPACITY)))
        return fail(import, HSD_DAT_DESCRIPTOR, at);
    return true;
}

bool itCommandData_Import(HSD_DatImport* import,
                          const ItScriptCollect* scripts,
                          ItCommandData* output)
{
    if (import == NULL) return false;
    if (scripts == NULL || output == NULL ||
        scripts->count > scripts->capacity ||
        (scripts->count != 0 && scripts->entries == NULL))
    {
        return fail(import, HSD_DAT_DESCRIPTOR, 0);
    }
    memset(output, 0, sizeof(*output));
    if (scripts->count == 0) return true;
    u32 data_words = import->dat->data_size / 4;
    size_t pending_capacity = (size_t) data_words + scripts->count;
    u8* state = calloc(data_words, 1);
    u32* pending = malloc(pending_capacity * sizeof(*pending));
    if (state == NULL || pending == NULL) {
        free(state);
        free(pending);
        return fail(import, HSD_DAT_CAPACITY, 0);
    }
    LbCommandEntry* entries = NULL;
    bool okay = false;
    u32 pending_count = 0;
    u32 source_begin = UINT32_MAX;
    u32 source_end = 0;
    u32 instruction_count = 0;
    for (u32 i = 0; i < scripts->count; ++i) {
        u32 source = scripts->entries[i].source;
        if (scripts->entries[i].destination == NULL || (source & 3) != 0 ||
            source > import->dat->data_size ||
            import->dat->data_size - source < 4)
        {
            fail(import, HSD_DAT_DESCRIPTOR, source);
            goto cleanup;
        }
        pending[pending_count++] = source;
    }
    while (pending_count != 0) {
        u32 at = pending[--pending_count];
        while (true) {
            if ((at & 3) != 0 || at > import->dat->data_size ||
                import->dat->data_size - at < 4) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            u32 index = at / 4;
            if (state[index] == LB_COMMAND_OPERAND) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            if (state[index] != 0) break;
            u32 raw;
            if (!HSD_DatReadU32(import->dat, at, &raw)) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            u32 opcode = raw >> 26;
            u32 width = command_width(opcode, raw);
            if (width == 0 || width >
                                  (import->dat->data_size - at) / 4)
            {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            state[index] = (u8) width;
            for (u32 word = 1; word < width; ++word) {
                if (state[index + word] != 0) {
                    fail(import, HSD_DAT_DESCRIPTOR, at + word * 4);
                    goto cleanup;
                }
                state[index + word] = LB_COMMAND_OPERAND;
            }
            if (at < source_begin) source_begin = at;
            if (at + width * 4 > source_end) source_end = at + width * 4;
            ++instruction_count;
            if (opcode == 5 || opcode == 7) {
                u32 target;
                const char* external;
                HSD_DatReferenceKind kind = HSD_DatResolve(
                    import->dat, at + 4, &target, &external);
                if (kind == HSD_DAT_REF_LOCAL && (target & 3) == 0 &&
                    target < import->dat->data_size &&
                    pending_count < pending_capacity)
                {
                    pending[pending_count++] = target;
                } else if (kind != HSD_DAT_REF_NULL) {
                    fail(import, HSD_DAT_RELOCATION, at + 4);
                    goto cleanup;
                }
            }
            if (opcode == 0 || opcode == 6 || opcode == 7) break;
            at += width * 4;
        }
    }
    if (source_begin == UINT32_MAX || source_begin >= source_end ||
        HSD_DatBytes(import->dat, source_begin,
                     source_end - source_begin) == NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, source_begin);
        goto cleanup;
    }
    entries = malloc((size_t) scripts->count * sizeof(*entries));
    if (entries == NULL) {
        fail(import, HSD_DAT_CAPACITY, source_begin);
        goto cleanup;
    }
    const ItemModelDesc* last_model = NULL;
    for (u32 i = 0; i < scripts->count; ++i) {
        const ItScriptEntry* entry = &scripts->entries[i];
        const ItemModelDesc* model =
            entry->article != NULL ? entry->article->x10_modelDesc : NULL;
        if (model != last_model && model != NULL && model->x4_bone_count != 0) {
            u32 count;
            if (!nativeItemModel_Admit(import, model, entry->source, &count))
                goto cleanup;
        }
        last_model = model;
        entries[i] = (LbCommandEntry) { entry->source, entry->article };
    }
    if (!lbCommandData_ValidateFlow(import, state, data_words, entries,
                                    scripts->count, false, validate_operands))
        goto cleanup;
    u32 word_count = (source_end - source_begin) / 4;
    union CmdUnion* commands = HSD_DatImportAlloc(
        import, (size_t) word_count * sizeof(*commands));
    if (commands == NULL) {
        import->error_offset = source_begin;
        goto cleanup;
    }
    for (u32 at = source_begin; at < source_end; at += 4) {
        if (state[at / 4] == 0 || state[at / 4] == LB_COMMAND_OPERAND) continue;
        u32 raw;
        HSD_DatReadU32(import->dat, at, &raw);
        u32 opcode = raw >> 26;
        u32 width = command_width(opcode, raw);
        u32 native = transcode_word(raw, word_layout(opcode, 0));
        memcpy(&commands[(at - source_begin) / 4], &native, 4);
        for (u32 word = 1; word < width; ++word) {
            u32 offset = at + word * 4;
            union CmdUnion* destination =
                &commands[(offset - source_begin) / 4];
            if (opcode == 5 || opcode == 7) {
                u32 target;
                const char* external;
                HSD_DatReferenceKind kind = HSD_DatResolve(
                    import->dat, offset, &target, &external);
                s32 relative = kind == HSD_DAT_REF_NULL
                                   ? INT32_MIN : (s32) target - (s32) offset;
                memcpy(destination, &relative, 4);
                break;
            }
            if ((opcode == 11 && word == 5) ||
                (opcode == 16 && word == 2))
            {
                memcpy(destination, HSD_DatBytes(import->dat, offset, 4), 4);
            } else {
                u32 value;
                HSD_DatReadU32(import->dat, offset, &value);
                value = transcode_word(value, word_layout(opcode, word));
                memcpy(destination, &value, 4);
            }
        }
    }
    for (u32 i = 0; i < scripts->count; ++i) {
        *scripts->entries[i].destination = &commands[
            (scripts->entries[i].source - source_begin) / 4];
    }
    output->commands = commands;
    output->source_begin = source_begin;
    output->word_count = word_count;
    output->entry_count = scripts->count;
    output->instruction_count = instruction_count;
    okay = true;

cleanup:
    free(entries);
    free(pending);
    free(state);
    return okay;
}
