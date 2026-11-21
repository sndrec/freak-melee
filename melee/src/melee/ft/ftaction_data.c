#include "ftaction_data.h"
#ifdef MELEE_NATIVE
#include "native_hitbox_lifetime.h"
#endif

#include <melee/lb/lbcommand_data.h>
#include <melee/lb/types.h>
#include <melee/ft/types.h>
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
static const FieldLayout halves = LAYOUT(16, 16);
static const FieldLayout sound_tail = LAYOUT(16, 8, 8);
static const FieldLayout gfx_head = LAYOUT(6, 8, 1, 1, 1, 15);
static const FieldLayout hitbox_head = LAYOUT(6, 3, 3, 1, 8, 1, 10);
static const FieldLayout hitbox_third = LAYOUT(9, 9, 9, 1, 1, 1, 1, 1);
static const FieldLayout hitbox_fourth = LAYOUT(9, 5, 8, 3, 5, 1, 1);
static const FieldLayout hitbox_damage = LAYOUT(6, 3, 23);
static const FieldLayout hitbox_flags = LAYOUT(6, 24, 1, 1);
static const FieldLayout command_var = LAYOUT(6, 2, 24);
static const FieldLayout hurt_state = LAYOUT(6, 8, 18);
static const FieldLayout dobj_flags = LAYOUT(6, 7, 19);
static const FieldLayout throw_hitbox_1 = LAYOUT(9, 9, 9);
static const FieldLayout throw_hitbox_2 = LAYOUT(9, 4, 3, 4);
static const FieldLayout random_sfx = LAYOUT(6, 8, 8, 4, 6);
static const FieldLayout stage_sfx = LAYOUT(6, 10, 8, 8);
static const FieldLayout tex_anim = LAYOUT(6, 1, 7, 7, 11);
static const FieldLayout part_anim = LAYOUT(6, 7, 7, 12);
static const FieldLayout pair13 = LAYOUT(6, 13, 13);
static const FieldLayout one12_13 = LAYOUT(6, 1, 12, 13);
static const FieldLayout two10_14 = LAYOUT(6, 2, 10, 14);
static const FieldLayout byte_head = LAYOUT(6, 8);
static const FieldLayout flag25 = LAYOUT(6, 1, 25);
static const FieldLayout footstep = LAYOUT(6, 8, 1, 1, 8, 8);
static const FieldLayout unknown_fx = LAYOUT(6, 2, 8, 8, 8);
static const FieldLayout charge = LAYOUT(6, 10, 16);
static const FieldLayout charge_tail = LAYOUT(8, 24);
static const FieldLayout wind = LAYOUT(6, 18, 8);
static const FieldLayout flag8 = LAYOUT(6, 1, 8);
#undef LAYOUT

// Keep this in opcode order with ftAction_803C0870. The independent ISO audit
// records the same corpus; the native import test compares its graph totals.
static const u8 action_widths[49] = {
    5, 5, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 7, 4, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 3, 2, 1, 4,
};

u32 ftActionData_SourceCommandWidth(u32 source_word)
{
    u32 opcode = source_word >> 26;
    if (opcode < 10) {
        return (opcode == 5 || opcode == 7) ? 2 : 1;
    }
    if (nativeHitbox_IsExtended(opcode))
        return nativeHitbox_ExtendedWidth((source_word >> 23) & 7);
    if (opcode == NATIVE_HITBOX_TIMED_OPCODE) return 6;
    if (opcode == NATIVE_HITBOX_EXPLICIT_OPCODE) return 5;
    if (opcode == FT_ACTION_THROW_FRACTIONAL_OPCODE)
        return FT_ACTION_THROW_FRACTIONAL_WORDS;
    return opcode <= 58 ? action_widths[opcode - 10] : 0;
}

u32 ftActionData_SourceFieldMaximum(unsigned opcode, unsigned field, unsigned width)
{
    // The encoded index is wider than the engine's actual throw-record array.
    if (ftAction_IsThrowHitbox(opcode) && field == 0) return 1;
    if (field == 0 &&
        (nativeHitbox_IsCreate(opcode) || (opcode >= 12 && opcode <= 15)))
        return ARRAY_SIZE(((Fighter*) 0)->x914) - 1;
    return width == 32 ? UINT32_MAX : (1u << width) - 1;
}

static bool fail(HSD_DatImport* import, HSD_DatResult result, u32 offset)
{
    import->status = result;
    import->error_offset = offset;
    return false;
}

static const FieldLayout* word_layout(u32 opcode, u32 word)
{
    if (opcode == FT_ACTION_THROW_FRACTIONAL_OPCODE) {
        if (word >= FT_ACTION_THROW_WORDS) return NULL;
        opcode = FT_ACTION_THROW_OPCODE;
    }
    if (nativeHitbox_OwnsFlags(opcode)) {
        if (word >= 5) return NULL;
        opcode = 11;
    }
    if (word != 0) {
        switch (opcode) {
        case 10: return &halves;
        case 11:
            if (word <= 2) return &halves;
            return word == 3 ? &hitbox_third : &hitbox_fourth;
        case 17: return word == 2 ? &sound_tail : NULL;
        case 34: return word == 1 ? &throw_hitbox_1 : &throw_hitbox_2;
        case 39:
            if (word == 2) return &halves;
            return word == 3 ? &sound_tail : NULL;
        case 54:
        case 55: return word == 2 ? &sound_tail : NULL;
        case 56: return &charge_tail;
        case 58: return &halves;
        default: return NULL;
        }
    }
    switch (opcode) {
    case 9: return &event;
    case 10: return &gfx_head;
    case 11: return &hitbox_head;
    case 12:
    case 13:
    case 34: return &hitbox_damage;
    case 14: return &hitbox_flags;
    case 17: return &event;
    case 19: return &command_var;
    case 28: return &hurt_state;
    case 31: return &dobj_flags;
    case 38: return &random_sfx;
    case 39: return &stage_sfx;
    case 40: return &tex_anim;
    case 41: return &part_anim;
    case 42: return &pair13;
    case 43: return &one12_13;
    case 45: return &two10_14;
    case 46: return &event;
    case 47: return &byte_head;
    case 49: return &flag25;
    case 54: return &footstep;
    case 55: return &unknown_fx;
    case 56: return &charge;
    case 57: return &flag8;
    case 58: return &wind;
    default: return &basic;
    }
}

unsigned ftActionData_SourceFieldWidths(unsigned opcode, unsigned word, u8 widths[8])
{
    const FieldLayout* layout = word_layout(opcode, word);
    if (!layout) return 0;
    memcpy(widths, layout->widths, layout->count);
    return layout->count;
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

bool ftActionData_NextHitboxFlag(const union CmdUnion* command)
{
    u32 native;
    memcpy(&native, command, 4);
    const FieldLayout* layout = word_layout(native & 63, 0);
    u32 source_bit = 12; // PPC bit numbering: byte 1, bit 4.
    u32 source_at = 0;
    u32 target_at = 0;
    for (u32 i = 0; i < layout->count; ++i) {
        u32 width = layout->widths[i];
        if (source_bit >= source_at && source_bit < source_at + width) {
            u32 native_bit = target_at + width - 1 - (source_bit - source_at);
            return ((native >> native_bit) & 1) != 0;
        }
        source_at += width;
        target_at += width;
    }
    return false;
}

bool ftActionData_Import(HSD_DatImport* import, u32 wait_source,
                         u32 wait_count, struct Fighter_WaitAnimData* table,
                         FtActionDataStats* stats)
{
    if (import == NULL) return false;
    if (table == NULL || stats == NULL || (wait_source & 3) != 0 ||
        wait_source == 0 || wait_source > import->dat->data_size ||
        wait_count == 0 ||
        wait_count > (import->dat->data_size - wait_source) / 24)
        return fail(import, HSD_DAT_DESCRIPTOR, wait_source);
    memset(stats, 0, sizeof(*stats));
    u32 source_words = wait_source / 4;
    size_t pending_capacity = (size_t) source_words + wait_count;
    u8* state = calloc(source_words, 1);
    u32* pending = malloc(pending_capacity * sizeof(*pending));
    LbCommandEntry* entries = calloc(wait_count, sizeof(*entries));
    if (state == NULL || pending == NULL || entries == NULL) {
        free(entries);
        free(state);
        free(pending);
        return fail(import, HSD_DAT_CAPACITY, wait_source);
    }
    bool okay = false;
    u32 pending_count = 0;
    u32 source_begin = UINT32_MAX;
    u32 instruction_count = 0;
    u32 entry_count = 0;
    for (u32 i = 0; i < wait_count; ++i) {
        u32 slot = wait_source + i * 24 + 12;
        u32 target;
        const char* external;
        HSD_DatReferenceKind kind =
            HSD_DatResolve(import->dat, slot, &target, &external);
        if (kind == HSD_DAT_REF_NULL) continue;
        if (kind != HSD_DAT_REF_LOCAL || (target & 3) != 0 ||
            target >= wait_source || pending_count == pending_capacity)
        {
            fail(import, HSD_DAT_RELOCATION, slot);
            goto cleanup;
        }
        pending[pending_count++] = target;
        entries[entry_count++].source = target;
    }
    while (pending_count != 0) {
        u32 at = pending[--pending_count];
        while (true) {
            if ((at & 3) != 0 || at >= wait_source) {
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
            const unsigned hitbox_slot = (raw >> 23) & 7;
            const bool capsule = nativeHitbox_HasCapsule(opcode, hitbox_slot);
            const unsigned slots = ARRAY_SIZE(((Fighter*) 0)->x914);
            if ((nativeHitbox_IsCreate(opcode) &&
                 nativeHitbox_Slot(opcode, hitbox_slot) >= slots) ||
                ((opcode == 12 || opcode == 13) && hitbox_slot >= slots) ||
                (opcode == 14 && ((raw >> 2) & 0xffffff) >= slots) ||
                (opcode == 15 && (raw & 0x3ffffff) >= slots)) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            const u32 width = ftActionData_SourceCommandWidth(raw);
            if (width == 0 || width > (wait_source - at) / 4) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            const bool timed = nativeHitbox_IsTimed(opcode);
            const bool extended = nativeHitbox_IsExtended(opcode);
            u32 life = 0;
            if (timed || extended) {
                if (!HSD_DatReadU32(import->dat, at + 20, &life) ||
                    ((life & NATIVE_HITBOX_EXCLUDE_GRABBED) && (raw & (1u << 19)))) {
                    fail(import, HSD_DAT_DESCRIPTOR, at);
                    goto cleanup;
                }
                const u32 duration = life & NATIVE_HITBOX_DURATION_MASK;
                if (timed ? duration == 0 || duration > 256000
                          : (life & ~NATIVE_HITBOX_EXCLUDE_GRABBED) != 0) {
                    fail(import, HSD_DAT_DESCRIPTOR, at + 20);
                    goto cleanup;
                }
            }
            if (extended) {
                u32 operands[5];
                u32 base = at + NATIVE_HITBOX_EXTENSION_WORD * 4;
                for (unsigned i = 0; i < 5; ++i)
                    HSD_DatReadU32(import->dat, base + i * 4, &operands[i]);
                const NativeHitboxLaunch launch = nativeHitboxLaunch_FromWords(operands);
                const float fraction = nativeHitbox_DamageFraction(operands[0]);
                if (fraction >= 1.0f || (raw & 0x3ffu) + fraction > 1023.0f ||
                    (launch.mode == NativeHitboxLaunch_Default && fraction == 0.0f &&
                     !capsule && (timed || !(life & NATIVE_HITBOX_EXCLUDE_GRABBED))) ||
                    !nativeHitboxLaunch_IsValid(&launch)) {
                    fail(import, HSD_DAT_DESCRIPTOR, base);
                    goto cleanup;
                }
            }
            if (capsule) {
                u32 last;
                const u32 end_z_at = at + (NATIVE_HITBOX_CAPSULE_WORDS - 1) * 4;
                if (!HSD_DatReadU32(import->dat, end_z_at, &last) ||
                    (last >> 16) != 0) {
                    fail(import, HSD_DAT_DESCRIPTOR, end_z_at);
                    goto cleanup;
                }
            }
            if (ftAction_IsThrowHitbox(opcode) && ((raw >> 23) & 7) >= 2) {
                fail(import, HSD_DAT_DESCRIPTOR, at);
                goto cleanup;
            }
            if (opcode == FT_ACTION_THROW_FRACTIONAL_OPCODE) {
                u32 bits;
                float damage;
                const u32 damage_at = at + FT_ACTION_THROW_WORDS * 4;
                if (!HSD_DatReadU32(import->dat, damage_at, &bits)) {
                    fail(import, HSD_DAT_DESCRIPTOR, damage_at);
                    goto cleanup;
                }
                memcpy(&damage, &bits, sizeof(damage));
                const u32 whole = raw & FT_ACTION_THROW_DAMAGE_MAX;
                if (!(damage > (float) whole &&
                      damage <= FT_ACTION_THROW_DAMAGE_MAX &&
                      (u32) damage == whole)) {
                    fail(import, HSD_DAT_DESCRIPTOR, damage_at);
                    goto cleanup;
                }
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
            ++instruction_count;
            if (opcode == 5 || opcode == 7) {
                u32 target;
                const char* external;
                HSD_DatReferenceKind kind =
                    HSD_DatResolve(import->dat, at + 4, &target, &external);
                if (kind == HSD_DAT_REF_LOCAL && (target & 3) == 0 &&
                    target < wait_source && pending_count < pending_capacity)
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
    if (entry_count == 0 || source_begin == UINT32_MAX ||
        HSD_DatBytes(import->dat, source_begin,
                     wait_source - source_begin) == NULL)
    {
        fail(import, HSD_DAT_DESCRIPTOR, wait_source);
        goto cleanup;
    }
    if (!lbCommandData_ValidateFlow(import, state, source_words, entries,
                                    entry_count, true, NULL))
        goto cleanup;
    u32 word_count = (wait_source - source_begin) / 4;
    union CmdUnion* commands = HSD_DatImportAlloc(
        import, (size_t) word_count * sizeof(*commands));
    if (commands == NULL) {
        import->error_offset = source_begin;
        goto cleanup;
    }
    for (u32 at = source_begin; at < wait_source; at += 4) {
        if (state[at / 4] == 0 || state[at / 4] == LB_COMMAND_OPERAND) continue;
        u32 raw;
        HSD_DatReadU32(import->dat, at, &raw);
        u32 opcode = raw >> 26;
        u32 width = ftActionData_SourceCommandWidth(raw);
        u32 native = transcode_word(raw, word_layout(opcode, 0));
        memcpy(&commands[(at - source_begin) / 4], &native, 4);
        for (u32 word = 1; word < width; ++word) {
            u32 value;
            if (opcode == 5 || opcode == 7) {
                u32 target;
                const char* external;
                HSD_DatReferenceKind kind = HSD_DatResolve(
                    import->dat, at + 4, &target, &external);
                s32 relative = kind == HSD_DAT_REF_NULL
                                   ? INT32_MIN
                                   : (s32) target - (s32) (at + 4);
                memcpy(&commands[(at + 4 - source_begin) / 4], &relative, 4);
                break;
            }
            HSD_DatReadU32(import->dat, at + word * 4, &value);
            value = transcode_word(value, word_layout(opcode, word));
            memcpy(&commands[(at + word * 4 - source_begin) / 4], &value, 4);
        }
    }
    for (u32 i = 0; i < wait_count; ++i) {
        u32 target;
        const char* external;
        if (HSD_DatResolve(import->dat, wait_source + i * 24 + 12,
                           &target, &external) == HSD_DAT_REF_LOCAL)
        {
            table[i].xC = &commands[(target - source_begin) / 4];
        }
    }
    stats->commands = commands;
    stats->source_begin = source_begin;
    stats->word_count = word_count;
    stats->instruction_count = instruction_count;
    stats->entry_count = entry_count;
    okay = true;

cleanup:
    free(entries);
    free(pending);
    free(state);
    return okay;
}
