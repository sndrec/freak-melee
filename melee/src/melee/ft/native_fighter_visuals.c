#include "native_fighter_visuals.h"
#include "types.h"
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/debug.h>
#include <math.h>

// Visibility tables group costume meshes by independently selectable variants.
// Requesting and evaluating their material animation is distinct from changing
// the currently visible variant or the fighter's skeletal animation.
void nativeFighterVisuals_SetGroupMaterialFrame(Fighter* fighter, int table, int group,
                                     int variant, float frame)
{
    if ((unsigned) table >= 5 || (unsigned) group >= fighter->x5AC.model_num ||
        !isfinite(frame)) return;
    FtPartsVisLookup* groups = fighter->x5AC.xC[table];
    if (groups == NULL || (unsigned) variant >= groups[group].x0 ||
        groups[group].x4 == NULL) return;
    TempS* meshes = &groups[group].x4[variant];
    DObjList* list = table == 2 ? &fighter->x203C : &fighter->dobj_list;
    for (int i = 0; i < meshes->x0; ++i) {
        unsigned index = meshes->x4[i];
        if (index >= list->count || list->data[index] == NULL) continue;
        HSD_MObj* material = list->data[index]->mobj;
        HSD_MObjReqAnim(material, frame);
        HSD_MObjAnim(material);
    }
}


float nativeFighterVisuals_InitializeGroupMaterialAnimation(
    Fighter* fighter, int table, int group, int variant, float prior_end_frame)
{
    if ((unsigned)table >= 5 || (unsigned)group >= fighter->x5AC.model_num)
        return prior_end_frame;
    FtPartsVisLookup* groups = fighter->x5AC.xC[table];
    if (groups == NULL || (unsigned)variant >= groups[group].x0 ||
        groups[group].x4 == NULL) return prior_end_frame;
    TempS* meshes = &groups[group].x4[variant];
    DObjList* list = table == 2 ? &fighter->x203C : &fighter->dobj_list;
    for (int i = 0; i < meshes->x0; ++i) {
        unsigned index = meshes->x4[i];
        if (index >= list->count || list->data[index] == NULL) continue;
        HSD_MObj* material = list->data[index]->mobj;
        if (material == NULL || material->aobj == NULL) continue;
        HSD_AObjSetRate(material->aobj, 0.0f);
        float end_frame = HSD_AObjGetEndFrame(material->aobj);
        if (prior_end_frame == 0.0f) prior_end_frame = end_frame;
        else HSD_ASSERTREPORT(__LINE__, prior_end_frame == end_frame,
                              "model group material animation lengths differ\n");
    }
    return prior_end_frame;
}

int nativeFighterVisuals_CurrentGroup(Fighter* fighter, int group)
{
    if ((unsigned)group >= fighter->x5AC.model_num ||
        (unsigned)group >= sizeof(fighter->x5F4_arr) / sizeof(fighter->x5F4_arr[0])) return -1;
    return fighter->x5F4_arr[group].idx;
}

int nativeFighterVisuals_PreviousGroup(Fighter* fighter, int group)
{
    if ((unsigned)group >= fighter->x5AC.model_num ||
        (unsigned)group >= sizeof(fighter->x5F4_arr) / sizeof(fighter->x5F4_arr[0])) return -1;
    return fighter->x5F4_arr[group].prev;
}
