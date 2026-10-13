#include "native_fighter_capture_rig.h"
#include "native_source_package.h"
#include "fighter.h"
#include "ftparts.h"

bool nativeFighterCapture_Joint(const NativeFighterLifetime* actor,Fighter_Part semantic,HSD_JObj** output)
{
    if (!actor || !output || !nativeFighterLifetime_Current(actor) ||
        (unsigned)semantic >= MAX_FT_PARTS) return false;
    Fighter* fp = actor->data;
    if (!fp->parts || !ftPartsTable || (unsigned)actor->kind >= Ft_Kind_RuntimeCapacity ||
        !ftPartsTable[actor->kind] || !ftPartsTable[actor->kind]->part_to_joint ||
        !GET_JOBJ(actor->object)) return false;
    FighterPartsTable* table = ftPartsTable[actor->kind];
    if (!table->parts_num || table->parts_num > MAX_FT_PARTS) return false;
    if (actor->source_owned) {
        const NativeSourcePackage* source = nativeFighterRuntime_SourcePackage(actor->kind);
        if (!source || source->address != source || !source->ready || !source->assets.ready ||
            source->target != actor->kind || actor->program->bank != &source->compiler.bank ||
            fp->x1C_actionStateList != source->assets.common_motions ||
            fp->x20_actionStateList != source->assets.motions ||
            fp->x18 != (s32)actor->program->bank->motion_layout.first ||
            table != &source->assets.parts ||
            table->parts_num != source->assets.facts.joints + source->assets.facts.omission_count)
            return false;
    }
    unsigned bone = (unsigned)ftParts_GetBoneIndex(fp, semantic);
    if (bone >= table->parts_num || !fp->parts[bone].joint) return false;
    *output = fp->parts[bone].joint;
    return true;
}
