#include "lbrefract.h"
#include "lbrefract_data.h"
#include "lbrefract_internal.h"

#include <math.h>
#include <placeholder.h>
#include <string.h>

#include "lbfile.h"
#include "lbheap.h"
#include "types.h"
#include <dolphin/gx/GXBump.h>
#include <dolphin/gx/GXEnum.h>
#include <dolphin/gx/GXGeometry.h>
#include <dolphin/gx/GXLighting.h>
#include <dolphin/gx/GXPixel.h>
#include <dolphin/gx/GXTev.h>
#include <dolphin/gx/GXTexture.h>
#include <dolphin/gx/GXTransform.h>
#include <sysdolphin/baselib/class.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/state.h>

/* 022650 */ static void fn_80022650(void);
/* 022940 */ static void fn_80022940(void);

enum { REFRACT_PARAMETER_CAPACITY = 32 };

static lbRefractData refract_storage;
static f32 refract_parameters[REFRACT_PARAMETER_CAPACITY];
static lbRefractData* refract_data;

static inline void lbRefract_WriteTexCoord(lbRefract_CallbackData* cb, s32 row,
                                           u32 col, f32 y, f32 x, f32 param0)
{
    u32 y_tex = 127.0f * (y * param0) + 128.0f;
    cb->callback0(cb, row, col, 0, 0, (u8) y_tex,
                  (u8) (127.0f * (x * param0) + 128.0f));
}

/// @todo reconcile with MSL fmodf
static inline float my_fmodf(float a, float b)
{
    s64 quotient;
    float fb, fa;

    fa = fabsf(a);
    fb = fabsf(b);

    if (fb > fa) {
        return a;
    }
    quotient = a / b;
    return a - b * quotient;
}

static void lbRefract_80021CE8(lbRefract_CallbackData* cb, s32 arg1)
{
    int param_idx = arg1 * 2;
    float x_step = 2.0f / (cb->width - 1);
    float y_step = 2.0f / (cb->height - 1);
    int row, col;
    float y = -1.0f;
    f32 param0;

    PAD_STACK(12);

    for (row = 0; row < cb->height; row++) {
        float x = -1.0f;
        float y_sq = y * y;
        for (col = 0; col < cb->width; col++) {
            float dist = sqrtf(x * x + y_sq);
            if (dist > 1.0f) {
                dist = 1.0f;
            }
            param0 = refract_data->parameters[param_idx];
            if (param0) {
                param0 = dist * my_fmodf(dist, param0);
            } else {
                param0 = dist;
            }
            param0 *= refract_data->parameters[param_idx + 1];
            if (param0 > 1.0f) {
                param0 = 1.0f;
            }
            lbRefract_WriteTexCoord(cb, col, row, y, x, param0);
            x += x_step;
        }
        y += y_step;
    }
}

HSD_ImageDesc imagedesc0 = { NULL, 0, 0, 4, 0, 0.0F, 0.0F };
HSD_TexLODDesc loddesc0 = { 1, 0.0F, 1, 1, 0 };

HSD_TObjDesc tobjdesc0 = {
    NULL,
    NULL,
    0,
    0,
    { 0.0F, 0.0F, 0.0F },
    { 1.0F, 1.0F, 1.0F },
    { 0.0F, 0.0F, 0.0F },
    0,
    0,
    1,
    1,
    0x83,
    1.0F,
    1,
    &imagedesc0,
    NULL,
    &loddesc0,
    NULL,
};

HSD_ImageDesc imagedesc1 = { NULL, 0, 0, 3, 0, 0.0F, 0.0F };
HSD_TexLODDesc loddesc1 = { 1, 0.0F, 1, 1, 0 };

HSD_TObjDesc tobjdesc1 = {
    NULL,
    &tobjdesc0,
    1,
    1,
    { 0.0F, 0.0F, 0.0F },
    { 1.0F, 1.0F, 1.0F },
    { 0.0F, 0.0F, 0.0F },
    0,
    0,
    1,
    1,
    0x81,
    1.0F,
    1,
    &imagedesc1,
    NULL,
    &loddesc1,
    NULL,
};

void lbRefract_800222A4(void)
{
    int const image_width = 320;
    int const image_height = 240;

    lbRefract_CallbackData cb;
    size_t i;
    void* buf;
    PAD_STACK(4);

    lbRefractState.refractionUserCount = 0;
    {
        size_t length;
        size_t size = lbFileGetSize("LbRf.dat");
        void* raw = lbHeap_80015BD0(0, OSRoundUp32B(size));
        HSD_ASSERT(0, raw);
        lbFile_8001668C("LbRf.dat", raw, &length);
        HSD_ASSERT(0, lbRefractData_Import(raw, length,
                                 &refract_storage, refract_parameters,
                                 ARRAY_SIZE(refract_parameters)));
        refract_data = &refract_storage;
    }
    {
        size_t buf_size =
            GXGetTexBufferSize(image_width, image_height, GX_TF_RGB565, 0, 0);
        lbRefractState.image_ptr = HSD_MemAlloc(buf_size);
        memset(lbRefractState.image_ptr, 0, buf_size);
    }
    lbRefractState.tobj_list =
        HSD_MemAlloc(refract_data->count * sizeof(HSD_TObj*));
    lbRefractState.imagedesc =
        HSD_MemAlloc(refract_data->count * sizeof(HSD_ImageDesc));

    for (i = 0; i < refract_data->count; i++) {
        buf = HSD_MemAlloc(GXGetTexBufferSize(32, 32, GX_TF_IA8, 0, 0));
        lbRefract_8002219C(&cb, buf, GX_TF_IA8, 32, 32);
        lbRefract_80021CE8(&cb, i);

        lbRefractState.imagedesc[i] = imagedesc0;
        tobjdesc1.imagedesc = &lbRefractState.imagedesc[i];
        lbRefractState.tobj_list[i] = HSD_TObjLoadDesc(&tobjdesc1);

        imagedesc0.image_ptr = lbRefractState.image_ptr;
        imagedesc0.format = GX_TF_RGB565;
        imagedesc0.width = image_width;
        imagedesc0.height = image_height;

        lbRefractState.imagedesc[(s32) i].image_ptr = buf;
        lbRefractState.imagedesc[(s32) i].format = GX_TF_IA8;
        lbRefractState.imagedesc[(s32) i].width = 32;
        lbRefractState.imagedesc[(s32) i].height = 32;
    }
}

/// @brief Copy framebuffer to refraction source texture.
void lbRefract_8002247C(HSD_CObj* cobj)
{
    if (lbRefractState.refractionUserCount == 0) {
        return;
    }

    switch (HSD_CObjGetProjectionType(cobj)) {
    case PROJ_PERSPECTIVE:
        C_MTXLightPerspective(lbRefractState.texture_mtx,
                            cobj->projection_param.perspective.fov,
                            cobj->projection_param.perspective.aspect, 0.5F,
                            -0.5F, 0.5F, 0.5F);
        break;
    case PROJ_FRUSTUM:
        C_MTXLightFrustum(lbRefractState.texture_mtx,
                        cobj->projection_param.frustum.top,
                        cobj->projection_param.frustum.bottom,
                        cobj->projection_param.frustum.left,
                        cobj->projection_param.frustum.right, cobj->near, 0.5F,
                        -0.5F, 0.5F, 0.5F);
        break;
    case PROJ_ORTHO:
    default:
        C_MTXLightOrtho(
            lbRefractState.texture_mtx, cobj->projection_param.ortho.top,
            cobj->projection_param.ortho.bottom,
            cobj->projection_param.ortho.left,
            cobj->projection_param.ortho.right, 0.5F, -0.5F, 0.5F, 0.5F);
        break;
    }
}

void lbRefract_80022560(void)
{
    if (lbRefractState.refractionUserCount != 0) {
        GXSetTexCopySrc(0, 0, 0x280, 0x1E0);
        GXSetTexCopyDst(0x140, 0xF0, 4, 1);
        GXCopyTex(lbRefractState.image_ptr, 0);
        GXPixModeSync();
        GXInvalidateTexAll();
    }
}

/// @brief Reset TEV and indirect texture stages.
void lbRefract_800225D4(void)
{
    GXSetTevDirect(0);
    GXSetNumIndStages(0);
    HSD_StateInvalidate(-1);
}

static void lbRefract_DObjDispReset(HSD_DObj* dobj, Mtx vmtx, Mtx pmtx,
                                    u32 rendermode)
{
    hsdDObj.disp(dobj, vmtx, pmtx, rendermode);
    GXSetTevDirect(0);
    GXSetNumIndStages(0);
    HSD_StateInvalidate(HSD_STATE_ALL);
}

HSD_DObjInfo lbRefractDObjInfo = { fn_80022650 };
HSD_PObjInfo lbRefractPObjInfo = { fn_80022940 };

static void fn_80022650(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&lbRefractDObjInfo), HSD_CLASS_INFO(&hsdDObj),
                     "refract_class_library", "refract_dobj",
                     sizeof(HSD_DObjInfo), sizeof(HSD_DObj));
    lbRefractDObjInfo.disp = lbRefract_DObjDispReset;
}

static void fn_80022940(void)
{
    hsdInitClassInfo(HSD_CLASS_INFO(&lbRefractPObjInfo), HSD_CLASS_INFO(&hsdPObj),
                     "refract_class_library", "refract_pobj",
                     sizeof(HSD_PObjInfo), sizeof(HSD_PObj));
    lbRefractPObjInfo.load = lbRefract_PObjLoad;
}

/// @brief Increment refraction effect user count.
void lbRefract_80022BB8(void)
{
    lbRefractState.refractionUserCount += 1;
}

/// @brief Decrement refraction effect user count.
void lbRefSetUnuse(void)
{
    lbRefractState.refractionUserCount -= 1;
    if (lbRefractState.refractionUserCount < 0) {
        HSD_ASSERTREPORT(0x31c, 0, "lbRefSetUnuse error!\n");
    }
}
