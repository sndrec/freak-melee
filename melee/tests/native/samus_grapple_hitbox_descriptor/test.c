// Port-layout proof, not a gameplay or scripting acceptance fixture.
#include <melee/ft/fighter.h>
#include <melee/ft/ftcoll.h>
#include <melee/ft/ftparts.h>
#include <melee/it/kinds/itsamusgrapple.h>
#include <melee/lb/lbcollision.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif
static unsigned checks, cases, damage_differences, sequence;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr, "case %u line %d: %s\n", cases, __LINE__, #x); exit(1); \
} } while (0)

// These two common engine leaves are controlled observers here. Their scaling,
// staleness and angle side effects have separate common-hitbox cohorts.
static uint32_t observed_damage;
static int observed_angle;
static unsigned damage_order, angle_order, bone_order;
void ftColl_8007ABD0(HitCapsule* hit, u32 damage, Fighter_GObj* source)
{
    CHECK(source && source->user_data);
    observed_damage = damage;
    damage_order = ++sequence;
    hit->damage = (float) damage;
    hit->unk_count = damage;
}
void ftColl_8007AC9C(HitCapsule* hit, int angle, Fighter_GObj* source)
{
    CHECK(source && source->user_data);
    observed_angle = angle;
    angle_order = ++sequence;
    hit->kb_angle = angle;
}
Fighter_Part ftParts_GetBoneIndex(Fighter* fighter, Fighter_Part bone)
{
    CHECK(fighter && fighter->parts);
    bone_order = ++sequence;
    return (Fighter_Part) (((unsigned) bone + 7) % 140);
}

// The initializer, original initializer and common victim-history routines are
// extracted without rewriting arithmetic or branches by run.py.
#include "native_initializer.inc"

static Fighter initial, actual, expected, legacy;
static FighterBone bones[140];
static HSD_JObj joints[140];
static HSD_GObj source;

static uint32_t random_word(uint32_t* state)
{
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static void encode_console(const itSamusGrapple_HitboxData* data, uint32_t words[5])
{
    const itSamusGrapple_Hitbox* h = &data->create_hitbox;
    const struct spawn_hitbox_0* a = &h->create_hitbox_0;
    const struct spawn_hitbox_3* b = &h->create_hitbox_3;
    const struct spawn_hitbox_4* c = &h->create_hitbox_4;
    words[0] = a->opcode << 26 | a->id << 23 | a->hit_group << 20 |
        a->only_hit_grabbed << 19 | a->bone << 11 |
        a->use_common_bone_ids << 10 | a->damage;
    words[1] = h->create_hitbox_1.size << 16 |
        (uint16_t) h->create_hitbox_1.z_offset;
    words[2] = (uint32_t) (uint16_t) h->create_hitbox_2.y_offset << 16 |
        (uint16_t) h->create_hitbox_2.x_offset;
    words[3] = b->angle << 23 | b->knockback_growth << 14 |
        b->weight_set_knockback << 5 | b->item_hit_interaction << 4 |
        b->ignore_thrown_fighters << 3 | b->ignore_fighter_scale << 2 |
        b->clank << 1 | b->rebound;
    words[4] = c->base_knockback << 23 | c->element << 18 |
        (uint32_t) (uint8_t) c->shield_damage << 10 |
        c->hit_sfx_severity << 7 | c->hit_sfx_kind << 2 |
        c->hit_grounded << 1 | c->hit_aerial;
}

// Independent console-word decoding of this initializer's contract. In
// particular, its size coefficient and x/z offset convention are retained.
static void console_initializer(Fighter_GObj* object, const uint32_t* w, bool victim_only)
{
    Fighter* f = GET_FIGHTER(object);
    unsigned slot = w[0] >> 23 & 7, group = w[0] >> 20 & 7;
    HitCapsule* hit = &f->x914[slot];
    if (hit->state == HitCapsule_Disabled || hit->x4 != group) {
        hit->x4 = group;
        hit->state = HitCapsule_Enabled;
        f->x2219_b3 = 1;
        ftColl_800768A0(f, hit);
    }
    unsigned bone = w[0] >> 11 & 255;
    if (w[0] & 0x400) bone = ftParts_GetBoneIndex(f, (Fighter_Part) bone);
    hit->jobj = f->parts[bone].joint;
    // The retail halfword at byte 2 contains the low 16 bits of word 0.
    ftColl_8007ABD0(hit, w[0] & 0x3FF, object);
    hit->scale = (w[1] >> 16) * 0.003906f;
    hit->b_offset.x = (int16_t) w[1] * 0.003906f;
    hit->b_offset.y = (int16_t) (w[2] >> 16) * 0.003906f;
    hit->b_offset.z = (int16_t) w[2] * 0.003906f;
    ftColl_8007AC9C(hit, w[3] >> 23, object);
    hit->x24 = w[3] >> 14 & 511;
    hit->x28 = w[3] >> 5 & 511;
    hit->x43_b0 = w[3] >> 4 & 1;
    hit->x43_b1 = w[3] >> 2 & 1;
    hit->x40_b0 = w[3] >> 1 & 1;
    hit->x40_b1 = w[3] & 1;
    hit->x2C = w[4] >> 23;
    hit->element = w[4] >> 18 & 31;
    hit->x34 = (int8_t) (w[4] >> 10);
    hit->sfx_severity = w[4] >> 7 & 7;
    hit->sfx_kind = w[4] >> 2 & 31;
    hit->x40_b2 = w[4] & 1;
    hit->x40_b3 = w[4] >> 1 & 1;
    hit->x42_b5 = 1;
    hit->x42_b7 = 1;
    hit->x41_b4 = 0;
    hit->x41_b6 = 0;
    hit->x41_b5 = 0;
    hit->x42_b0 = 0;
    hit->x42_b4 = 0;
    hit->x41_b7 = 0;
    hit->hit_grabbed_victim_only = victim_only;
    hit->x42_b1 = 1;
    hit->x42_b2 = 0;
    hit->x43_b2 = 0;
}

static void compare(itSamusGrapple_HitboxData* data, unsigned mode)
{
    ++cases;
    unsigned slot = data->create_hitbox.create_hitbox_0.id;
    unsigned group = data->create_hitbox.create_hitbox_0.hit_group;
    initial = (Fighter) { .parts = bones };
    for (unsigned i = 0; i < 4; ++i) {
        memset(&initial.x914[i], 0x65, sizeof(HitCapsule));
        HitCapsule* hit = &initial.x914[i];
        hit->state = i == slot ? (HitCapsuleState) (mode % 4) : HitCapsule_Unk3;
        hit->x4 = mode & 4 ? group : (group + 1) % 8;
        hit->x44 = (uint8_t) i;
        hit->x45 = (uint8_t) (i + 1);
        hit->jobj = bones[i].joint;
        for (unsigned v = 0; v < 12; ++v) {
            hit->victims_1[v].victim = &joints[(i * 12 + v) % 140];
            hit->victims_2[v].victim = &joints[(i * 12 + v + 60) % 140];
        }
    }
    actual = expected = legacy = initial;
    uint32_t words[5];
    encode_console(data, words);
    sequence = bone_order = 0;
    source.user_data = &actual;
    it_802B7160(&source, data);
    uint32_t damage = observed_damage;
    int angle = observed_angle;
    unsigned damage_at = damage_order, angle_at = angle_order, bone_at = bone_order;
    sequence = bone_order = 0;
    source.user_data = &expected;
    console_initializer(&source, words, data->x134_4);
    CHECK(damage == observed_damage && angle == observed_angle);
    CHECK(damage_at == damage_order && angle_at == angle_order && bone_at == bone_order);
    CHECK(actual.x2219_b3 == expected.x2219_b3);
    // Byte comparison includes preserved state/history, unrelated capsules,
    // inactive flag bits and union bytes, not just newly assigned fields.
    CHECK(!memcmp(actual.x914, expected.x914, sizeof(actual.x914)));
    CHECK(actual.parts == initial.parts);
    source.user_data = &legacy;
    original_initializer(&source, data);
    CHECK(legacy.x914[slot].x40_b3 == actual.x914[slot].x40_b3);
    if (observed_damage != damage) ++damage_differences;
}

int main(void)
{
    for (unsigned i = 0; i < 140; ++i) bones[i].joint = &joints[i];
    itSamusGrapple_HitboxData data = { .create_hitbox = it_803B8660 };
    compare(&data, 0);
    CHECK(actual.x914[0].damage == 0 && legacy.x914[0].damage == 17);
    CHECK(actual.x914[0].scale == 1200 * 0.003906f);
    CHECK(actual.x914[0].jobj == &joints[139]);
    CHECK(!actual.x914[0].hit_grabbed_victim_only);
    uint32_t rng = 0x5A4D53u;
    for (unsigned i = 0; i < 8192; ++i) {
        struct spawn_hitbox_0* a = &data.create_hitbox.create_hitbox_0;
        struct spawn_hitbox_3* b = &data.create_hitbox.create_hitbox_3;
        struct spawn_hitbox_4* c = &data.create_hitbox.create_hitbox_4;
        uint32_t r = random_word(&rng);
        a->opcode = 11; a->id = i % 4; a->hit_group = i / 4 % 8;
        a->only_hit_grabbed = i / 32 % 2; a->bone = i / 64 % 140;
        a->use_common_bone_ids = i / 128 % 2; a->damage = r & 1023;
        data.create_hitbox.create_hitbox_1.size = random_word(&rng) >> 16;
        data.create_hitbox.create_hitbox_1.z_offset = (int16_t) r;
        data.create_hitbox.create_hitbox_2.y_offset = (int16_t) (r >> 16);
        data.create_hitbox.create_hitbox_2.x_offset = (int16_t) random_word(&rng);
        r = random_word(&rng);
        b->angle = r & 511; b->knockback_growth = r >> 9 & 511;
        b->weight_set_knockback = r >> 18 & 511;
        b->item_hit_interaction = i / 2 % 2; b->ignore_thrown_fighters = i / 4 % 2;
        b->ignore_fighter_scale = i / 8 % 2; b->clank = i / 16 % 2; b->rebound = i / 32 % 2;
        r = random_word(&rng);
        c->base_knockback = r & 511; c->element = r >> 9 & 31;
        c->shield_damage = (int8_t) (r >> 14); c->hit_sfx_severity = r >> 22 & 7;
        c->hit_sfx_kind = r >> 25 & 31;
        c->hit_grounded = i % 2; c->hit_aerial = i / 2 % 2;
        data.x134_4 = i / 4 % 2;
        // Sweep each descriptor over every initial capsule-state/group mode.
        for (unsigned mode = 0; mode < 8; ++mode) compare(&data, mode);
    }
    CHECK(damage_differences > 60000);
    printf("{\"status\":\"PASS\",\"checks\":%u,\"descriptor_cases\":%u,"
           "\"legacy_damage_differences\":%u,\"stock_typed_damage\":0,"
           "\"stock_legacy_damage\":17}\n", checks, cases, damage_differences);
    return 0;
}
