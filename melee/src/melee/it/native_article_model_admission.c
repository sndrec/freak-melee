#include "native_article_model_demand.h"

#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>

bool nativeArticleModel_EnvironmentReady(const NativeArticleModelDemand* demand)
{
    if (!demand || !demand->joints || !HSD_JObjUsesStandardMethods()) return false;
    if (demand->drawables && !HSD_DObjUsesStandardMethods()) return false;
    if (demand->polygons && !HSD_PObjUsesStandardMethods()) return false;
    if (demand->materials &&
        (!HSD_MObjUsesStandardMethods() ||
         (tobj_toon && tobj_toon->imagedesc))) return false;
    if (demand->textures && !HSD_TObjUsesStandardMethods()) return false;
    // Shadow modes are rejected by the descriptor census. A dormant global
    // shadow list therefore never participates in these admitted materials.
    return !(demand->uncovered & ~NativeArticleModel_MaterialEnvironment);
}

bool nativeArticleModel_PrepareStatic(const HSD_Joint* descriptor,
                                     NativeHsdReservation* reservation,
                                     NativeArticleModelDemand* output)
{
    NativeArticleModelDemand demand;
    if (!output || !nativeArticleModel_Census(descriptor, &demand) ||
        !nativeArticleModel_EnvironmentReady(&demand) ||
        !nativeHsdReservation_Begin(reservation, demand.construction,
                                    demand.construction_count)) return false;

    const HSD_Joint* pending[512];
    uint32_t count = 1;
    pending[0] = descriptor;
    bool okay = true;
    while (count && okay) {
        const HSD_Joint* joint = pending[--count];
        // Census already proved the traversal's complete bounds, and rejects
        // instances/particles/references. Preparation sees the same immutable
        // descriptor, not a name-hash lookup or a donor's live graph.
        if (joint->next) pending[count++] = joint->next;
        if (joint->child) pending[count++] = joint->child;
        for (const HSD_DObjDesc* drawable = joint->u.dobjdesc;
             drawable && okay; drawable = drawable->next) {
            if (!drawable->mobjdesc) continue;
            HSD_MObj* material = HSD_MObjAlloc();
            HSD_MOBJ_METHOD(material)->load(material, drawable->mobjdesc);
            okay = HSD_MObjCompileTev(material, HSD_TExpCompileMode_Checked) >= 0;
            HSD_MObjRemove(material);
        }
    }
    bool complete = nativeHsdReservation_End(reservation);
    if (!okay || !complete) return false;
    demand.uncovered &= ~NativeArticleModel_MaterialEnvironment;
    *output = demand;
    return true;
}
