#include "native_fighter_material_animation.h"
#ifdef MELEE_NATIVE
#include <sysdolphin/baselib/jobj.h>
static u32 pointer(const HSD_Dat* dat, u32 slot)
{
    u32 target;
    const char* external;
    return HSD_DatResolve(dat, slot, &target, &external) == HSD_DAT_REF_LOCAL
               ? target
               : 0;
}
typedef struct SequenceScan {
    const NativeFighterModelEditor* model;
    unsigned material, count, capacity, visited;
    NativeMaterialTextureSequence* output;
} SequenceScan;
static void walk(SequenceScan* scan, u32 joint, u32 animation, unsigned depth)
{
    if (depth > 512) {
        return;
    }
    const HSD_Dat* dat = &scan->model->dat;
    while (joint && animation && scan->visited++ < 4096) {
        if (!HSD_DatBytes(dat, joint, 64) || !HSD_DatBytes(dat, animation, 12))
        {
            return;
        }
        u32 flags = 0;
        HSD_DatReadU32(dat, joint + 4, &flags);
        u32 mesh =
            flags & (JOBJ_PTCL | JOBJ_SPLINE) ? 0 : pointer(dat, joint + 16);
        u32 mat = pointer(dat, animation + 8);
        for (unsigned m = 0; mesh && mat && m < 1024; ++m) {
            if (!HSD_DatBytes(dat, mesh, 16) || !HSD_DatBytes(dat, mat, 12)) {
                return;
            }
            if (pointer(dat, mesh + 8) ==
                scan->model->materials[scan->material].offset)
            {
                for (u32 tex = pointer(dat, mat + 8), layer = 0;
                     tex && layer < 8; tex = pointer(dat, tex), ++layer)
                {
                    u16 frames = 0;
                    if (!HSD_DatBytes(dat, tex, 24) ||
                        !HSD_DatReadU16(dat, tex + 20, &frames))
                    {
                        break;
                    }
                    if (!frames) {
                        continue;
                    }
                    bool found = false;
                    for (unsigned i = 0; i < scan->count; ++i) {
                        if (scan->output[i].offset == tex) {
                            found = true;
                        }
                    }
                    if (!found && scan->count < scan->capacity) {
                        u32 id = 0;
                        HSD_DatReadU32(dat, tex + 4, &id);
                        scan->output[scan->count++] =
                            (NativeMaterialTextureSequence){ tex, id, frames };
                    }
                }
            }
            mesh = pointer(dat, mesh + 4);
            mat = pointer(dat, mat);
        }
        walk(scan, pointer(dat, joint + 8), pointer(dat, animation),
             depth + 1);
        joint = pointer(dat, joint + 12);
        animation = pointer(dat, animation + 4);
    }
}
unsigned nativeFighterMaterial_Sequences(const NativeFighterModelEditor* model,
                                         unsigned material,
                                         NativeMaterialTextureSequence* output,
                                         unsigned capacity)
{
    if (!model || material >= model->material_count || !output || !capacity) {
        return 0;
    }
    u32 joint, animation;
    if (!HSD_DatFindRoot(&model->dat, model->root_name, &joint) ||
        !model->project->facts.costume_matanim[0] ||
        !HSD_DatFindRoot(&model->dat, model->project->facts.costume_matanim,
                         &animation))
    {
        return 0;
    }
    SequenceScan scan = { model, material, 0, capacity, 0, output };
    walk(&scan, joint, animation, 0);
    return scan.count;
}
#endif
