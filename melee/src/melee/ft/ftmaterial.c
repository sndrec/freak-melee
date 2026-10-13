#include "ftmaterial.h"

#include "fighter.h"
#include "forward.h"
#include "ft_0C8C.h"
#include "ftCo_800C7CA0.h"
#include "ftdevice.h"
#include "kinds/ftCommon/ftCo_09F4.h"
#include "types.h"
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbrefract.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/state.h>
#include <sysdolphin/baselib/tev.h>
#include <sysdolphin/baselib/tobj.h>

HSD_MObjInfo ftMObj = { ftMaterial_800BF260 };

void ftMaterial_800BF260(void)
{
    hsdInitClassInfo(&ftMObj.parent, &hsdMObj.parent,
                     "sysdolphin_base_library", "ft_mobj",
                     sizeof(HSD_MObjInfo), sizeof(HSD_MObj));
    ftMObj.setup = ftMaterial_800BF2B8;
}

void ftMaterial_800BF2B8(HSD_MObj* mobj, u32 rendermode)
{
    Fighter* fp;
    HSD_TObj* tobj;
    HSD_TExp texp;
    HSD_PEDesc pe;
    HSD_TObj** cur_tobj;
    HSD_TExp* texp1;
    HSD_PEDesc* pe_p;

    fp = GET_FIGHTER(HSD_GObj_804D7814);

    if (fp->x2226_b5) {
        lbRefract_80022998(mobj, rendermode,
                           fp->smash_attrs.x2134_vibrateFrame);
        return;
    }

    if (!fp->x2223_b2 && (!fp->x2228_b0 || !fp->x2224_b0)) {
        if (fp->is_metal) {
            mobj = ft_804D6580;
        } else if (fp->x2227_b3) {
            mobj = ft_804D6588;
        }
    }

    HSD_StateInitTev();

    {
        rendermode = mobj->rendermode;
        HSD_SetMaterialColor(mobj->mat->ambient, mobj->mat->diffuse,
                             mobj->mat->specular, mobj->mat->alpha);
        if (rendermode & RENDER_SPECULAR) {
            HSD_SetMaterialShininess(mobj->mat->shininess);
        }
        {
            cur_tobj = NULL;
            {
                tobj = mobj->tobj;
                if (rendermode & RENDER_SHADOW && tobj_shadows != NULL) {
                    cur_tobj = &tobj;
                    while (*cur_tobj != NULL) {
                        cur_tobj = &(*cur_tobj)->next;
                    }
                    *cur_tobj = tobj_shadows;
                }
                if ((rendermode & RENDER_TOON) && tobj_toon != NULL &&
                    tobj_toon->imagedesc != NULL)
                {
                    tobj_toon->next = tobj;
                    tobj = tobj_toon;
                }
                HSD_TObjSetup(tobj);
                HSD_TObjSetupTextureCoordGen(tobj);
                HSD_MOBJ_METHOD(mobj)->setup_tev(mobj, tobj, rendermode);
            }
            if (fp->x61D != 0xFF) {
                rendermode |= RENDER_NO_ZUPDATE | RENDER_XLU;
            }
            {
                HSD_TExp* new_texp =
                    ftMaterial_800BF534(fp, mobj, &texp, rendermode);
                texp1 = new_texp;
                ftMaterial_800BF6BC(fp, mobj, texp1);
                if (fp->x2223_b2 && !fp->x2223_b3) {
                    rendermode |= RENDER_NO_ZUPDATE;
                }
                {
                    if (fp->x2223_b3 && fp->x61D == 0xFF) {
                        pe.flags = (1 << 3) | (1 << 4) | (1 << 5);
                        pe.dst_alpha = 0;
                        pe.type = 0;
                        pe.src_factor = 4;
                        pe_p = &pe;
                        pe.dst_factor = 5;
                        pe.logic_op = 15;
                        pe.z_comp = 3;
                        pe.alpha_comp0 = 7;
                        pe.ref0 = 0;
                        pe.alpha_op = 0;
                        pe.alpha_comp1 = 7;
                        pe.ref1 = 0;
                    } else {
                        pe_p = mobj->pe;
                    }
                    HSD_SetupRenderModeWithCustomPE(rendermode, pe_p);
                }
                if (texp1 == NULL) {
                    ftCo_8009F75C(fp, true);
                }
            }
            if (cur_tobj != NULL) {
                *cur_tobj = NULL;
            }
        }
    }
}

void ftMaterial_800BFB4C(Fighter_GObj* gobj, GXColor* diffuse)
{
    HSD_JObj* cur = GET_JOBJ(gobj);

    while (cur != NULL) {
        HSD_DObj* dobj = HSD_JObjGetDObj(cur);
        while (dobj != NULL) {
            HSD_MObj* mobj = dobj != NULL ? dobj->mobj : NULL;
            if (mobj != NULL) {
                if (mobj->mat != NULL) {
                    HSD_Material* mat = mobj->mat;
                    mat->diffuse = *diffuse;
                }
            }
            dobj = dobj != NULL ? dobj->next : NULL;
        }
        if (!(HSD_JObjGetFlags(cur) & JOBJ_INSTANCE)) {
            HSD_JObj* child;
            if (cur == NULL) {
                child = NULL;
            } else {
                child = cur->child;
            }
            if (child != NULL) {
                HSD_JObj* child;
                if (cur == NULL) {
                    child = NULL;
                } else {
                    child = cur->child;
                }
                cur = child;
                continue;
            }
        }
        {
            HSD_JObj* next;
            if (cur == NULL) {
                next = NULL;
            } else {
                next = cur->next;
            }
            if (next != NULL) {
                HSD_JObj* next;
                if (cur == NULL) {
                    next = NULL;
                } else {
                    next = cur->next;
                }
                cur = next;
            } else {
                while (true) {
                    HSD_JObj* parent;
                    if (cur == NULL) {
                        parent = NULL;
                    } else {
                        parent = cur->parent;
                    }
                    if (parent == NULL) {
                        cur = NULL;
                    } else {
                        HSD_JObj* parent;
                        if (cur == NULL) {
                            parent = NULL;
                        } else {
                            parent = cur->parent;
                        }
                        {
                            HSD_JObj* next;
                            if (parent == NULL) {
                                next = NULL;
                            } else {
                                next = parent->next;
                            }
                            if (next != NULL) {
                                HSD_JObj* parent;
                                if (cur == NULL) {
                                    parent = NULL;
                                } else {
                                    parent = cur->parent;
                                }
                                {
                                    HSD_JObj* next;
                                    if (parent == NULL) {
                                        next = NULL;
                                    } else {
                                        next = parent->next;
                                    }
                                    cur = next;
                                }
                            } else {
                                HSD_JObj* parent;
                                if (cur == NULL) {
                                    parent = NULL;
                                } else {
                                    parent = cur->parent;
                                }
                                cur = parent;
                                continue;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

HSD_MObj* ft_804D6580;

HSD_MObj* ft_804D6588;
