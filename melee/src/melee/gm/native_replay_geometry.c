#include "native_replay.h"

#ifdef MELEE_NATIVE
#include <melee/ft/fighter.h>
#include <melee/ft/types.h>
#include <melee/gm/types.h>
#include <melee/it/types.h>
#include <melee/lb/types.h>
#include <melee/pl/player.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/tobj.h>
#include <stdio.h>
#include <string.h>

// One requested replay sample only. Inspect the actual live fields instead of
// weakening aggregate parity hashes or allocating a second model/state copy.
static uint32_t bits(float value)
{
    uint32_t result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static void vector(const char* name, const Vec3* value)
{
    fprintf(stderr, " %s=%08x,%08x,%08x", name,
            bits(value->x), bits(value->y), bits(value->z));
}

static void animation(const HSD_AObj* aobj)
{
    fprintf(stderr, " anim=%u", aobj != NULL);
    if (aobj != NULL) {
        fprintf(stderr, " flags=%08x frame=%08x rewind=%08x end=%08x speed=%08x",
                aobj->flags, bits(aobj->curr_frame), bits(aobj->rewind_frame),
                bits(aobj->end_frame), bits(aobj->framerate));
    }
}

static void joints(const char* owner, const HSD_JObj* root,
                   unsigned depth, unsigned* index)
{
    if (depth > 128) return;
    for (const HSD_JObj* joint = root; joint != NULL; joint = joint->next) {
        if (*index >= 1024) return;
        unsigned at = (*index)++;
        fprintf(stderr, "[geometry-joint] %s joint=%u depth=%u flags=%08x rot=%08x,%08x,%08x,%08x",
                owner, at, depth, joint->flags & ~JOBJ_MTX_DIRTY,
                bits(joint->rotate.x), bits(joint->rotate.y),
                bits(joint->rotate.z), (joint->flags & JOBJ_USE_QUATERNION)
                    ? bits(joint->rotate.w) : 0);
        vector("scale", &joint->scale);
        vector("pos", &joint->translate);
        animation(joint->aobj);
        fputc('\n', stderr);
        fprintf(stderr, "[geometry-matrix] %s joint=%u", owner, at);
        for (unsigned row = 0; row < 3; ++row) {
            for (unsigned column = 0; column < 4; ++column) {
                fprintf(stderr, " %08x", bits(joint->mtx[row][column]));
            }
        }
        fputc('\n', stderr);
        if (union_type_dobj(joint)) {
            unsigned draw_index = 0;
            for (const HSD_DObj* draw = joint->u.dobj;
                 draw != NULL && draw_index < 128; draw = draw->next) {
                fprintf(stderr, "[geometry-material] %s joint=%u draw=%u flags=%08x",
                        owner, at, draw_index++, draw->flags);
                const HSD_MObj* material = draw->mobj;
                if (material != NULL) {
                    fprintf(stderr, " mode=%08x", material->rendermode);
                    if (material->mat != NULL) {
                        uint32_t diffuse;
                        memcpy(&diffuse, &material->mat->diffuse, sizeof(diffuse));
                        fprintf(stderr, " diffuse=%08x alpha=%08x",
                                diffuse, bits(material->mat->alpha));
                    }
                    animation(material->aobj);
                }
                fputc('\n', stderr);
            }
        }
        joints(owner, joint->child, depth + 1, index);
    }
}

static void hitbox(const char* owner, const char* category,
                   unsigned index, const HitCapsule* hit)
{
    fprintf(stderr, "[geometry-hitbox] %s group=%s hit=%u state=%d x4=%u count=%u damage=%08x scale=%08x angle=%d kb=%u,%u,%u element=%u x34=%d sfx=%d,%d",
            owner, category, index, hit->state, hit->x4, hit->unk_count,
            bits(hit->damage), bits(hit->scale), hit->kb_angle,
            hit->x24, hit->x28, hit->x2C, hit->element,
            hit->x34, hit->sfx_severity, hit->sfx_kind);
    vector("offset", &hit->b_offset);
    vector("current", &hit->x4C);
    vector("previous", &hit->x58);
    vector("contact", &hit->hurt_coll_pos);
    fprintf(stderr, " distance=%08x victims=%u,%u flags=", bits(hit->coll_distance), hit->x44, hit->x45);
    fprintf(stderr, "%u,%u,%u,%u,%u,%u,%u,%u,%u,",
            hit->x40_b0, hit->x40_b1, hit->x40_b2, hit->x40_b3, hit->x40_b4,
            hit->x41_b4, hit->x41_b5, hit->x41_b6, hit->x41_b7);
    fprintf(stderr, "%u,%u,%u,%u,%u,%u,%u,%u,",
            hit->x42_b0, hit->x42_b1, hit->x42_b2, hit->x42_b3,
            hit->x42_b4, hit->x42_b5, hit->x42_b6, hit->x42_b7);
    fprintf(stderr, "%u,%u,%u,%u,%u,%u,%u,%u\n",
            hit->x43_b0, hit->x43_b1, hit->x43_b2, hit->x43_b3,
            hit->x43_b4, hit->x43_b5, hit->x43_b6, hit->x43_b7);
}

void nativeReplay_LogGeometry(uint64_t sample_index)
{
    fprintf(stderr, "[geometry] sample %llu\n", (unsigned long long) sample_index);
    for (unsigned actor = 0; actor < GM_MAX_PLAYERS * 2; ++actor) {
        HSD_GObj* object = Player_GetEntityAtIndex(actor / 2, actor % 2);
        Fighter* fighter = object != NULL ? GET_FIGHTER(object) : NULL;
        if (fighter == NULL) continue;
        char owner[32];
        snprintf(owner, sizeof(owner), "P%u member=%u", actor / 2 + 1, actor % 2);
        unsigned index = 0;
        if (object->obj_kind == HSD_GObj_JObjKind) joints(owner, object->hsd_obj, 0, &index);
        for (unsigned slot = 0; slot < 4; ++slot) hitbox(owner, "attack", slot, &fighter->x914[slot]);
        for (unsigned slot = 0; slot < 2; ++slot) hitbox(owner, "grab", slot, &fighter->xDF4[slot]);
        hitbox(owner, "throw", 0, &fighter->x1064_thrownHitbox);
    }
    unsigned item_index = 0;
    for (HSD_GObj* object = HSD_GObjPLinkHead[HSD_GOBJ_PLINK_ITEM];
         object != NULL && item_index < 512; object = object->next) {
        Item* item = object->user_data;
        if (item == NULL) continue;
        char owner[32];
        snprintf(owner, sizeof(owner), "I%u", item_index++);
        unsigned index = 0;
        if (object->obj_kind == HSD_GObj_JObjKind) joints(owner, object->hsd_obj, 0, &index);
        for (unsigned slot = 0; slot < 3; ++slot) {
            HSD_JObj* model = nativeReplay_AdditionalItemModel(item, slot);
            if (!model) continue;
            snprintf(owner, sizeof(owner), "I%u model=%u", item_index - 1, slot + 1);
            index = 0;
            joints(owner, model, 0, &index);
        }
    }
}
#endif
