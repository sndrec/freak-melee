#ifndef MELEE_FT_FTANIM_TRACK_PARTS_H
#define MELEE_FT_FTANIM_TRACK_PARTS_H

#include "ftparts.h"
#include "types.h"

// A clip can contain a track for an attachment even while its slot is empty.
// Consume that node in clip order so the next joint gets its own tracks.
static inline unsigned ftAnim_NextTrackPart(const Fighter* fp, unsigned part, unsigned count)
{
    for (; part < count; ++part) {
        const FighterBone* bone = &fp->parts[part];
        if (bone->flags_b1 && !bone->flags_b2) break;
        u32 mask = ftParts_8007506C(fp->kind, part);
        if ((mask & fp->x594_bits) || (!mask && bone->flags_b1)) break;
    }
    return part;
}

#endif
