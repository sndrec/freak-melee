#include "native_replay_capsule_trace.h"

#ifdef NATIVE_NETPLAY_DIAGNOSTICS

#include "native_semantic_state.h"

#include <melee/ft/types.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>

#include <math.h>
#include <string.h>
#include <xmmintrin.h>

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
static const char* const transform_columns[] = {
    "source", "mxcsr_control", "crt_fma3", "joint_flags", "has_parent",
    "rotate.x:f32", "rotate.y:f32", "rotate.z:f32", "rotate.w:f32",
    "scale.x:f32", "scale.y:f32", "scale.z:f32",
    "translate.x:f32", "translate.y:f32", "translate.z:f32",
    "world.00:f32", "world.01:f32", "world.02:f32", "world.03:f32",
    "world.10:f32", "world.11:f32", "world.12:f32", "world.13:f32",
    "world.20:f32", "world.21:f32", "world.22:f32", "world.23:f32",
    "parent.00:f32", "parent.01:f32", "parent.02:f32", "parent.03:f32",
    "parent.10:f32", "parent.11:f32", "parent.12:f32", "parent.13:f32",
    "parent.20:f32", "parent.21:f32", "parent.22:f32", "parent.23:f32",
};
#undef UINT_COLUMN
#undef FLOAT_COLUMN
#undef VECTOR_COLUMNS

enum {
    HitVictimCount = 12,
    HitWords = ARRAY_SIZE(hit_columns) + HitVictimCount * 2,
    HurtWords = ARRAY_SIZE(hurt_columns),
    TransformWords = ARRAY_SIZE(transform_columns),
    HitSlots = 7,
    HurtSlots = 15,
};

typedef struct HurtboxTransforms {
    uint32_t words[HurtSlots][TransformWords];
    uint16_t seen;
} HurtboxTransforms;

typedef struct FighterCapsuleTrace {
    uint32_t hit[HitSlots][HitWords];
    uint32_t hurt[HurtSlots][HurtWords];
    unsigned hurt_count;
    HurtboxTransforms transforms;
} FighterCapsuleTrace;

static FighterCapsuleTrace history[NativeReplay_StateTraceFrames]
                                   [NATIVE_SEMANTIC_FIGHTER_CAPACITY];
static struct {
    struct {
        const Fighter* fighter;
        HurtboxTransforms transforms;
    } actors[NATIVE_SEMANTIC_FIGHTER_CAPACITY];
    unsigned count;
    uint64_t frame;
    bool active;
} tick;

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void nativeReplayCapsuleTrace_BeginTick(uint64_t frame)
{
    tick.active = true;
    tick.frame = frame;
    tick.count = 0;
    for (const HSD_GObj* gobj = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_FIGHTER];
         gobj != NULL && tick.count < ARRAY_SIZE(tick.actors); gobj = gobj->next) {
        if (gobj->user_data == NULL) continue;
        tick.actors[tick.count].fighter = gobj->user_data;
        tick.actors[tick.count].transforms.seen = 0;
        ++tick.count;
    }
}

void nativeReplayCapsuleTrace_EndMatch(void)
{
    tick.active = false;
}

void nativeReplayCapsuleTrace_RecordTransform(const HurtCapsule* capsule,
                                             NativeHurtboxTransformSource source)
{
    if (!tick.active || capsule->bone == NULL) return;
    for (unsigned actor = 0; actor < tick.count; ++actor) {
        const Fighter* fighter = tick.actors[actor].fighter;
        uintptr_t base = (uintptr_t)&fighter->hurt_capsules[0].capsule;
        uintptr_t addr = (uintptr_t)capsule;
        if (addr < base || addr - base >= sizeof(fighter->hurt_capsules) ||
            (addr - base) % sizeof(FighterHurtCapsule) != 0) continue;
        unsigned slot = (unsigned)((addr - base) / sizeof(FighterHurtCapsule));
        HurtboxTransforms* transforms = &tick.actors[actor].transforms;
        transforms->seen |= (uint16_t)(1u << slot);
        uint32_t* words = transforms->words[slot];
        const HSD_JObj* joint = capsule->bone;
        *words++ = source;
        *words++ = _mm_getcsr() & 0xffc0; // Exclude sticky exception status bits.
        *words++ = _get_FMA3_enable();
        *words++ = joint->flags;
        *words++ = joint->parent != NULL;
        *words++ = float_bits(joint->rotate.x);
        *words++ = float_bits(joint->rotate.y);
        *words++ = float_bits(joint->rotate.z);
        *words++ = (joint->flags & JOBJ_USE_QUATERNION) ? float_bits(joint->rotate.w) : 0;
        memcpy(words, &joint->scale, sizeof(Vec3));
        words += 3;
        memcpy(words, &joint->translate, sizeof(Vec3));
        words += 3;
        memcpy(words, joint->mtx, sizeof(Mtx));
        words += 12;
        if (joint->parent != NULL) {
            memcpy(words, joint->parent->mtx, sizeof(Mtx));
        } else {
            memset(words, 0, sizeof(Mtx));
        }
        return;
    }
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
    trace->transforms.seen = 0;
    if (tick.active && tick.frame == frame) {
        for (unsigned i = 0; i < tick.count; ++i) {
            if (tick.actors[i].fighter != fighter) continue;
            trace->transforms.seen = tick.actors[i].transforms.seen;
            for (unsigned slot = 0; slot < trace->hurt_count; ++slot) {
                if (!(trace->transforms.seen & (1u << slot))) continue;
                memcpy(trace->transforms.words[slot],
                       tick.actors[i].transforms.words[slot],
                       sizeof(trace->transforms.words[slot]));
            }
            break;
        }
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
    fputs("\n[hurt-transform-columns] frame actor slot", output);
    for (unsigned i = 0; i < ARRAY_SIZE(transform_columns); ++i) {
        fprintf(output, " %s", transform_columns[i]);
    }
    fputs("\n[hurt-transform-sources] 1 hit, 2 grab, 3 contact, 4 bounds, 5 debug draw", output);
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
            if ((trace->transforms.seen & (1u << slot)) &&
                !write_capsule(output, "hurt-transform", frame, actor, slot,
                               trace->transforms.words[slot], TransformWords, remaining)) {
                return false;
            }
        }
    }
    return true;
}

#endif
