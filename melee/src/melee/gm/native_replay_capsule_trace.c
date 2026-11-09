#include "native_replay_capsule_trace.h"

#ifdef NATIVE_NETPLAY_DIAGNOSTICS

#include "native_semantic_state.h"

#include <melee/ft/types.h>

#include <string.h>

// Store semantic values only. Addresses and struct padding vary between peers.
// Float fields use their IEEE-754 bits, so reports show even a one-bit change.
#define HIT_FIELDS(U, F, V) \
    U(state) U(x4) U(unk_count) F(damage) V(b_offset, offset) F(scale) \
    U(kb_angle) U(x24) U(x28) U(x2C) U(element) U(x34) \
    U(sfx_severity) U(sfx_kind) U(x40_b0) U(x40_b1) U(x40_b2) U(x40_b3) \
    U(x40_b4) U(x41_b4) U(x41_b5) U(x41_b6) U(x41_b7) U(x42_b0) \
    U(x42_b1) U(x42_b2) U(x42_b3) U(x42_b4) U(x42_b5) U(x42_b6) \
    U(x42_b7) U(x43_b0) U(x43_b1) U(x43_b2) U(x43_b3) U(x43_b4) \
    U(x43_b5) U(x43_b6) U(x43_b7) U(x44) U(x45) \
    V(x4C, current) V(x58, previous)
#define HURT_FIELDS(U, F, V) \
    U(capsule.state) V(capsule.a_offset, offset_a) V(capsule.b_offset, offset_b) \
    F(capsule.scale) U(capsule.skip_update_pos) V(capsule.a_pos, cached_a) \
    V(capsule.b_pos, cached_b) U(capsule.bone_idx) U(height) U(is_grabbable)

#define UINT_COLUMN(field) #field,
#define FLOAT_COLUMN(field) #field ":f32",
#define VECTOR_COLUMNS(field, label) \
    #label ".x:f32", #label ".y:f32", #label ".z:f32",
static const char* const hit_columns[] = {
    HIT_FIELDS(UINT_COLUMN, FLOAT_COLUMN, VECTOR_COLUMNS)
};
static const char* const hurt_columns[] = {
    HURT_FIELDS(UINT_COLUMN, FLOAT_COLUMN, VECTOR_COLUMNS)
};
#undef UINT_COLUMN
#undef FLOAT_COLUMN
#undef VECTOR_COLUMNS

enum {
    HitVictimCount = 12,
    HitWords = ARRAY_SIZE(hit_columns) + HitVictimCount * 2,
    HurtWords = ARRAY_SIZE(hurt_columns),
    HitSlots = 7,
    HurtSlots = 15,
};

typedef struct FighterCapsuleTrace {
    uint32_t hit[HitSlots][HitWords];
    uint32_t hurt[HurtSlots][HurtWords];
    unsigned hurt_count;
} FighterCapsuleTrace;

static FighterCapsuleTrace history[NativeReplay_StateTraceFrames]
                                   [NATIVE_SEMANTIC_FIGHTER_CAPACITY];

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static void capture_hit(uint32_t* words, const HitCapsule* capsule)
{
#define CAPTURE_UINT(field) *words++ = (uint32_t)capsule->field;
#define CAPTURE_FLOAT(field) *words++ = float_bits(capsule->field);
#define CAPTURE_VECTOR(field, label) \
    CAPTURE_FLOAT(field.x) CAPTURE_FLOAT(field.y) CAPTURE_FLOAT(field.z)
    HIT_FIELDS(CAPTURE_UINT, CAPTURE_FLOAT, CAPTURE_VECTOR)
    for (unsigned i = 0; i < HitVictimCount; ++i) {
        *words++ = capsule->victims_1[i].victim ? capsule->victims_1[i].x4 : 0;
    }
    for (unsigned i = 0; i < HitVictimCount; ++i) {
        *words++ = capsule->victims_2[i].victim ? capsule->victims_2[i].x4 : 0;
    }
}

void nativeReplayCapsuleTrace_Capture(uint64_t frame, unsigned actor,
                                     const Fighter* fighter)
{
    _Static_assert(ARRAY_SIZE(fighter->x914) + ARRAY_SIZE(fighter->xDF4) + 1 ==
                       HitSlots, "Update diagnostic hitbox slot mapping");
    _Static_assert(ARRAY_SIZE(fighter->hurt_capsules) == HurtSlots,
                   "Update diagnostic hurtbox capacity");
    _Static_assert(ARRAY_SIZE(fighter->x914[0].victims_1) == HitVictimCount &&
                       ARRAY_SIZE(fighter->x914[0].victims_2) == HitVictimCount,
                   "Update diagnostic victim capacity");

    FighterCapsuleTrace* trace =
        &history[frame % NativeReplay_StateTraceFrames][actor];
    unsigned slot = 0;
    for (unsigned i = 0; i < ARRAY_SIZE(fighter->x914); ++i) {
        capture_hit(trace->hit[slot++], &fighter->x914[i]);
    }
    for (unsigned i = 0; i < ARRAY_SIZE(fighter->xDF4); ++i) {
        capture_hit(trace->hit[slot++], &fighter->xDF4[i]);
    }
    capture_hit(trace->hit[slot], &fighter->x1064_thrownHitbox);

    trace->hurt_count = fighter->hurt_capsules_len;
    if (trace->hurt_count > HurtSlots) trace->hurt_count = HurtSlots;
    for (unsigned i = 0; i < trace->hurt_count; ++i) {
        const FighterHurtCapsule* capsule = &fighter->hurt_capsules[i];
        uint32_t* words = trace->hurt[i];
        HURT_FIELDS(CAPTURE_UINT, CAPTURE_FLOAT, CAPTURE_VECTOR)
    }
#undef CAPTURE_UINT
#undef CAPTURE_FLOAT
#undef CAPTURE_VECTOR
}

void nativeReplayCapsuleTrace_WriteColumns(FILE* output)
{
    fputs("[hitbox-columns] frame actor slot", output);
    for (unsigned i = 0; i < ARRAY_SIZE(hit_columns); ++i) {
        fprintf(output, " %s", hit_columns[i]);
    }
    for (unsigned group = 1; group <= 2; ++group) {
        for (unsigned i = 0; i < HitVictimCount; ++i) {
            fprintf(output, " victims_%u[%u].counter", group, i);
        }
    }
    fputs("\n[hurtbox-columns] frame actor slot", output);
    for (unsigned i = 0; i < ARRAY_SIZE(hurt_columns); ++i) {
        fprintf(output, " %s", hurt_columns[i]);
    }
    fputs("\n[capsule-encoding] unsigned hex words; floats are IEEE-754 bits; "
          "hitbox slots 0-3 attack, 4-5 auxiliary, 6 throw\n", output);
}

static bool write_capsule(FILE* output, const char* label, uint64_t frame,
                          unsigned actor, unsigned slot, const uint32_t* words,
                          unsigned count, size_t* remaining)
{
    char line[64 + HitWords * 9];
    size_t length = snprintf(line, sizeof(line), "[%s] %llu %u %u", label,
                             (unsigned long long)frame, actor, slot);
    for (unsigned i = 0; i < count; ++i) {
        length += snprintf(line + length, sizeof(line) - length, " %x", words[i]);
    }
    line[length++] = '\n';
    if (length > *remaining || fwrite(line, 1, length, output) != length) {
        return false;
    }
    *remaining -= length;
    return true;
}

bool nativeReplayCapsuleTrace_WriteFrame(FILE* output, uint64_t frame,
                                        unsigned actor_count, size_t* remaining)
{
    const FighterCapsuleTrace* traces = history[frame % NativeReplay_StateTraceFrames];
    for (unsigned actor = 0; actor < actor_count; ++actor) {
        const FighterCapsuleTrace* trace = &traces[actor];
        uint32_t hurt_count = trace->hurt_count;
        if (!write_capsule(output, "hurtbox-count", frame, actor, 0,
                           &hurt_count, 1, remaining)) return false;
        for (unsigned slot = 0; slot < HitSlots; ++slot) {
            if (!write_capsule(output, "hitbox", frame, actor, slot,
                               trace->hit[slot], HitWords, remaining)) return false;
        }
        for (unsigned slot = 0; slot < trace->hurt_count; ++slot) {
            if (!write_capsule(output, "hurtbox", frame, actor, slot,
                               trace->hurt[slot], HurtWords, remaining)) return false;
        }
    }
    return true;
}

#endif
