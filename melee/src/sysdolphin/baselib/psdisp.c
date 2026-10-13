#include "psdisp_internal.h"
#include "psdisp_state_internal.h"
#include "native_render_phase.h"

#include <string.h>

#include "cobj.h"
#include "fog.h"
#include "forward.h"
#include "lobj.h"
#include "mtx.h"
#include "particle.h"
#include "psdisptev.h"
#include "psstructs.h"
#include "state.h"
#include "util.h"
#include <dolphin/gx.h>

// MSL/math.h defines a non-IEEE FLT_EPSILON
#undef FLT_EPSILON
#define FLT_EPSILON 1.19209290e-07F

typedef struct {
    HSD_Particle* head;
    HSD_Particle* tail;
} psdisp_ParticleSortBucket;

typedef struct {
    GXTlutFmt fmt;
    u32 tlut_name;
    u16 n_entries;
} psdisp_Tlut;

typedef struct {
    Mtx mtx;
} psdisp_Mtx;

/* 39FA28 */ static void getColorPrimEnv(HSD_Particle*, GXColor*, GXColor*);
/* 39FB74 */ static void getColorMatAmb(HSD_Particle*, GXColor*, GXColor*);

/* 3B9628 */ static const psdisp_Mtx HSD_PSDisp_803B9628 = {
    { { 1.0F, 0.0F, 0.0F, 0.0F },
      { 0.0F, 1.0F, 0.0F, 0.0F },
      { 0.0F, 0.0F, 1.0F, 0.0F } },
};
/** @remarks Orphaned #ref_INC assertion literals: no caller survives in this
 * unit, but GNT4 emits the live #psSetBillboardCamera body in its generator
 * unit with the same `__assert` strings. */
/* 40C300 */ static char HSD_PSDisp_8040C300[] = "object.h";
/* 40C30C */ static char HSD_PSDisp_8040C30C[] =
    "HSD_OBJ(o)->ref_count != HSD_OBJ_NOREF";
/* 40C334 */ static u8 HSD_PSDisp_8040C334[0xC] = { 0 };
/* 40C340 */ static u8 HSD_PSDisp_8040C340[0x20] = {
    0, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1,
    0, 0, 0, 1, 1, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 0,
};
/* 4D0908 */ extern HSD_Particle* hsd_804D0908[16];
#ifdef MELEE_NATIVE
#pragma clang section data = "" bss = ""
#endif
ParticleDrawState particleDraw;
#ifdef MELEE_NATIVE
#pragma clang section data = ".mstate$m"
#endif
/* 4D7908 */ static HSD_Fog* HSD_PSDisp_804D7908;

ASSERT_SIZE(HSD_PSDisp_8040C340, 0x20);
ASSERT_SIZE(particle_sort_frames, 0x10);

void setVtxDesc(s32 fmt)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    GXClearVtxDesc();
    switch (fmt) {
    case 0:
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
        return;
    case 1:
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        return;
    case 2:
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_INDEX8);
        return;
    case 3:
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        return;
    case 4:
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        return;
    case 5:
        GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
        GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
        GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        return;
    }
}



static void getColorPrimEnv(HSD_Particle* pp, GXColor* primCol,
                            GXColor* envCol)
{
    if (pp->primColCount) {
        int scale = 65536 * pp->primColRemain / pp->primColCount;
        primCol->r = ((pp->primColTarget.r << 16) +
                      (pp->primCol.r - pp->primColTarget.r) * scale) >>
                     16;
        primCol->g = ((pp->primColTarget.g << 16) +
                      (pp->primCol.g - pp->primColTarget.g) * scale) >>
                     16;
        primCol->b = ((pp->primColTarget.b << 16) +
                      (pp->primCol.b - pp->primColTarget.b) * scale) >>
                     16;
        primCol->a = ((pp->primColTarget.a << 16) +
                      (pp->primCol.a - pp->primColTarget.a) * scale) >>
                     16;
    } else {
        *primCol = pp->primCol;
    }
    if (pp->envColCount) {
        int scale = 65536 * pp->envColRemain / pp->envColCount;
        envCol->r = ((pp->envColTarget.r << 16) +
                     (pp->envCol.r - pp->envColTarget.r) * scale) >>
                    16;
        envCol->g = ((pp->envColTarget.g << 16) +
                     (pp->envCol.g - pp->envColTarget.g) * scale) >>
                    16;
        envCol->b = ((pp->envColTarget.b << 16) +
                     (pp->envCol.b - pp->envColTarget.b) * scale) >>
                    16;
        envCol->a = ((pp->envColTarget.a << 16) +
                     (pp->envCol.a - pp->envColTarget.a) * scale) >>
                    16;
    } else {
        *envCol = pp->envCol;
    }
}

#ifdef MUST_MATCH
#pragma push
#pragma dont_inline on
#endif
static void getColorMatAmb(HSD_Particle* pp, GXColor* matCol, GXColor* ambCol)
{
    if (pp->matColCount) {
        int scale = 65536 * pp->matColRemain / pp->matColCount;
        matCol->r = matCol->g = matCol->b =
            ((pp->matRGBTarget << 16) +
             (pp->matRGB - pp->matRGBTarget) * scale) >>
            16;
        matCol->a =
            ((pp->matATarget << 16) + (pp->matA - pp->matATarget) * scale) >>
            16;
    } else {
        matCol->r = matCol->g = matCol->b = pp->matRGB;
        matCol->a = pp->matA;
    }
    if (pp->ambColCount) {
        int scale = 65536 * pp->ambColRemain / pp->ambColCount;
        ambCol->r = ambCol->g = ambCol->b =
            ((pp->ambRGBTarget << 16) +
             (pp->ambRGB - pp->ambRGBTarget) * scale) >>
            16;
        ambCol->a =
            ((pp->ambATarget << 16) + (pp->ambA - pp->ambATarget) * scale) >>
            16;
    } else {
        ambCol->r = ambCol->g = ambCol->b = pp->ambRGB;
        ambCol->a = pp->ambA;
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif

void psGetTrailColor(HSD_Particle* pp, GXColor* color)
{
    GXColor env_color;

    switch (pp->kind & (DispLighting | PrimEnv)) {
    case 0:
    case DispLighting:
        getColorPrimEnv(pp, color, &env_color);
        break;
    case PrimEnv:
    case DispLighting | PrimEnv:
        color->r = color->g = color->b = color->a = 0xFF;
        break;
    }
}

static inline void psSetColor(GXColor* color, u8 value)
{
    color->r = value;
    color->g = value;
    color->b = value;
    color->a = value;
}

static inline void psSetupVtxFormat(GXVtxFmt format, bool has_color,
                                    bool has_texture, GXCompType texture_type)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    GXSetVtxAttrFmt(format, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0U);
    if (has_color) {
        GXSetVtxAttrFmt(format, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0U);
    }
    if (has_texture) {
        GXSetVtxAttrFmt(format, GX_VA_TEX0, GX_TEX_ST, texture_type, 0U);
    }
}

static inline void setupChanCtrl(HSD_Particle* pp)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    u32 chan_state = pp->kind & (DispLighting | Trail);

    if (chan_state != (u32) particleDraw.channel_mode) {
        particleDraw.channel_mode = chan_state;
        GXSetNumChans(1);
        switch (particleDraw.channel_mode) {
        case Trail:
            GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, 0,
                          GX_DF_NONE, GX_AF_NONE);
            break;
        case DispLighting:
            GXSetChanCtrl(GX_COLOR0, GX_TRUE, GX_SRC_REG, GX_SRC_REG,
                          HSD_LObjGetLightMaskDiffuse(), GX_DF_NONE,
                          HSD_LObjGetLightMaskAttnFunc() ? GX_AF_SPOT
                                                         : GX_AF_NONE);
            GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
                          GX_DF_NONE, GX_AF_NONE);
            break;
        case DispLighting | Trail:
            GXSetChanCtrl(GX_COLOR0, GX_TRUE, GX_SRC_REG, GX_SRC_REG,
                          HSD_LObjGetLightMaskDiffuse(), GX_DF_NONE,
                          HSD_LObjGetLightMaskAttnFunc() ? GX_AF_SPOT
                                                         : GX_AF_NONE);
            GXSetChanCtrl(GX_ALPHA0, GX_FALSE, GX_SRC_VTX, GX_SRC_VTX, 0,
                          GX_DF_NONE, GX_AF_NONE);
            break;
        default:
            GXSetChanCtrl(GX_COLOR0A0, GX_FALSE, GX_SRC_REG, GX_SRC_REG, 0,
                          GX_DF_NONE, GX_AF_NONE);
            break;
        }
    }
}

static inline void setupChanReg(HSD_Particle* pp)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    GXColor prim_color;
    GXColor amb_color;
    GXColor mat_color;
    HSD_LObj* lobj;

    if (pp->kind & DispLighting) {
        getColorMatAmb(pp, &mat_color, &amb_color);
        if (pp->kind & PrimEnv) {
            prim_color.r = prim_color.g = prim_color.b = 0xFF;
        } else {
            getColorPrimEnv(pp, &prim_color, &mat_color);
            amb_color.r = (u8) ((amb_color.r * prim_color.r) >> 8);
            amb_color.g = (u8) ((amb_color.g * prim_color.g) >> 8);
            amb_color.b = (u8) ((amb_color.b * prim_color.b) >> 8);
        }
        if (prim_color.r != particleDraw.channel_material.r || prim_color.g != particleDraw.channel_material.g ||
            prim_color.b != particleDraw.channel_material.b)
        {
            particleDraw.channel_material = prim_color;
            GXSetChanMatColor(GX_COLOR0, particleDraw.channel_material);
        }
        lobj = HSD_LObjGetActiveByID(GX_MAX_LIGHT);
        if (lobj != NULL) {
            HSD_MulColor(&amb_color, &lobj->color, &amb_color);
        } else {
            amb_color.r = amb_color.g = amb_color.b = 0;
        }
        if (amb_color.r != particleDraw.channel_ambient.r || amb_color.g != particleDraw.channel_ambient.g ||
            amb_color.b != particleDraw.channel_ambient.b)
        {
            particleDraw.channel_ambient = amb_color;
            GXSetChanAmbColor(GX_COLOR0, particleDraw.channel_ambient);
        }
    }
}

static inline void setupTevReg(HSD_Particle* pp)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    GXColor prim_color;
    GXColor env_color;
    GXColor mat_color;
    GXColor amb_color;

    getColorPrimEnv(pp, &prim_color, &env_color);
    if ((pp->kind & PrimEnv) ||
        (!(pp->kind & DispLighting) && !(pp->kind & Trail)))
    {
        if (particleDraw.primary_color.r != prim_color.r ||
            particleDraw.primary_color.g != prim_color.g ||
            particleDraw.primary_color.b != prim_color.b || particleDraw.primary_color.a != prim_color.a)
        {
            particleDraw.primary_color = prim_color;
            GXSetTevColor(GX_TEVREG0, particleDraw.primary_color);
        }
        if (pp->kind & PrimEnv) {
            if (particleDraw.environment_color.r != env_color.r ||
                particleDraw.environment_color.g != env_color.g ||
                particleDraw.environment_color.b != env_color.b || particleDraw.environment_color.a != env_color.a)
            {
                particleDraw.environment_color = env_color;
                GXSetTevColor(GX_TEVREG1, particleDraw.environment_color);
            }
        } else if (particleDraw.environment_color.r != 0 || particleDraw.environment_color.g != 0 ||
                   particleDraw.environment_color.b != 0 || particleDraw.environment_color.a != 0)
        {
            particleDraw.environment_color.r = particleDraw.environment_color.g = particleDraw.environment_color.b = particleDraw.environment_color.a =
                0;
            GXSetTevColor(GX_TEVREG1, particleDraw.environment_color);
        }
    }
    if (pp->kind & DispLighting) {
        getColorMatAmb(pp, &mat_color, &amb_color);
        if (pp->kind & PrimEnv) {
            if (particleDraw.material_color.r != mat_color.r ||
                particleDraw.material_color.g != mat_color.g ||
                particleDraw.material_color.b != mat_color.b || particleDraw.material_color.a != mat_color.a)
            {
                particleDraw.material_color = mat_color;
                GXSetTevColor(GX_TEVREG2, particleDraw.material_color);
            }
        } else {
            mat_color.a = (u8) ((mat_color.a * prim_color.a) >> 8);
            if (particleDraw.material_color.r != mat_color.r ||
                particleDraw.material_color.g != mat_color.g ||
                particleDraw.material_color.b != mat_color.b || particleDraw.material_color.a != mat_color.a)
            {
                particleDraw.material_color = mat_color;
                GXSetTevColor(GX_TEVREG2, particleDraw.material_color);
            }
        }
    }
}

HSD_Particle* particleSort(s32 arg0, u8 arg1, HSD_Particle** arg2,
                           HSD_Particle** arg3)
{
    psdisp_ParticleSortBucket buckets[16];
    HSD_Particle** new_var;
    HSD_Particle* var_r28;
    HSD_Particle* var_r3;
    HSD_Particle* var_r4;
    HSD_Particle* var_r5;
    HSD_Particle* var_r7;
    HSD_Particle** temp_r29;
    HSD_Particle** var_r6_2;
    HSD_Particle** var_r7_2;
    s32 i;
    u32 temp_r3;
    u32 temp_r3_2;
    s32 temp_r4;
    s32 var_r0;
    s32 var_r0_2;
    s32 var_r6;
    u8* temp_r9;

    ASSERT_SIZE(buckets[0], 8);

    temp_r9 = &particle_sort_frames[arg0];
    temp_r29 = (new_var = &hsd_804D0908[arg0]);
    var_r28 = *temp_r29;
    if (*temp_r9 == arg1) {
        *arg2 = var_r28;
        *arg3 = particle_non_edge_heads[arg0];
        return var_r28;
    }

    *temp_r9 = arg1;
    if (var_r28 == NULL) {
        particle_non_edge_heads[arg0] = NULL;
        *arg2 = NULL;
        *arg3 = NULL;
        return NULL;
    }

    memset(buckets, 0, sizeof(buckets));
    temp_r3 = var_r28->kind;
    if (temp_r3 & 8) {
        var_r0 = 0;
    } else {
        var_r0 = 1;
    }
    temp_r4 = ((temp_r3 >> 0x19) & 7) + (var_r0 * 8);
    buckets[temp_r4].head = var_r28;
    var_r6 = temp_r4;
    var_r7 = var_r28->next;

    while (var_r7 != NULL) {
        if ((var_r28->kind ^ var_r7->kind) & 0x0E000008) {
            buckets[var_r6].tail = var_r28;
            temp_r3_2 = var_r7->kind;
            if (temp_r3_2 & 8) {
                var_r0_2 = 0;
            } else {
                var_r0_2 = 1;
            }
            var_r6 = ((temp_r3_2 >> 0x19) & 7) + (var_r0_2 * 8);
            if (buckets[var_r6].head == NULL) {
                buckets[var_r6].head = var_r7;
            } else {
                buckets[var_r6].tail->next = var_r7;
            }
        }
        var_r28 = var_r7;
        var_r7 = var_r7->next;
    }
    buckets[var_r6].tail = var_r28;

    var_r6_2 = NULL;
    var_r4 = NULL;
    var_r7_2 = NULL;
    var_r5 = NULL;

    for (i = 0; i < 8; i++) {
        if (buckets[i].head != NULL) {
            if (var_r4 == NULL) {
                var_r4 = buckets[i].head;
            } else {
                *var_r6_2 = buckets[i].head;
            }
            var_r6_2 = &buckets[i].tail->next;
        }
    }

    for (i = 8; i < 16; i++) {
        if (buckets[i].head != NULL) {
            if (var_r5 == NULL) {
                var_r5 = buckets[i].head;
            } else {
                *var_r7_2 = buckets[i].head;
            }
            var_r7_2 = &buckets[i].tail->next;
        }
    }

    var_r3 = NULL;
    if (var_r6_2 != NULL) {
        var_r3 = var_r4;
        *var_r6_2 = var_r5;
    }
    if (var_r7_2 != NULL) {
        if (var_r3 == NULL) {
            var_r3 = var_r5;
        }
        *var_r7_2 = NULL;
    }

    *temp_r29 = var_r3;
    particle_non_edge_heads[arg0] = var_r5;
    *arg2 = var_r3;
    *arg3 = var_r5;
    return var_r3;
}

static inline void setBlendMode(int blend_mode)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    if (particleDraw.blend_mode != blend_mode) {
        particleDraw.blend_mode = blend_mode;
        switch (blend_mode) {
        case 0:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA,
                           GX_LO_CLEAR);
            break;
        case 1:
            GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE,
                           GX_LO_CLEAR);
            break;
        default:
            OSReport("Particle:setBlendMode:Unknown mode\n");
            break;
        }
    }
}

static inline void psUpdateBillboardAxes(const Mtx inv_view)
{
    f32 right_x;
    f32 up_x;
    f32 right_y;
    f32 up_y;
    f32 right_z;
    f32 up_z;

    right_x = inv_view[0][0];
    up_x = inv_view[0][1];
    particleDraw.right.x = right_x + up_x;
    particleDraw.up.x = right_x - up_x;
    right_y = inv_view[1][0];
    up_y = inv_view[1][1];
    particleDraw.right.y = right_y + up_y;
    particleDraw.up.y = right_y - up_y;
    right_z = inv_view[2][0];
    up_z = inv_view[2][1];
    particleDraw.right.z = right_z + up_z;
    particleDraw.up.z = right_z - up_z;
}

#ifdef MUST_MATCH
#pragma push
#pragma inline_depth(3)
#endif
void psDispParticles(u32 target_link, u32 sw)
{
    s32 sp7B4;
    void* sp7B0;
    u32 sp7AC;
    u32 sp7A8;
    u8 sp7A5;
    u8 sp7A4;
    s32 needs_setup;
    void* sp79C;
    psdisp_Tlut tlut_obj;
    UNUSED s32 stack_pad;
    GXTexObj sp764;
    HSD_Particle* sorted_particles;
    HSD_Particle* non_edge_particles;
    psdisp_Mtx billboard_mtx;
    f32 y2;
    GXTlutObj gx_tlut_obj;
    s32 alpha_compare_mode;
    s32 prev_tex_interp_near;
    u32 prev_kind = 0;
    HSD_Particle* pp;
    bool draw = true;
#ifdef MELEE_NATIVE
    draw = nativeRenderGraphicsEnabled;
#endif

    alpha_compare_mode = 0;
    prev_tex_interp_near = 0;
    sp7A5 = 0;
    sp7A4 = 0xFF;
    needs_setup = 1;
    if (sw == 0) {
        if (particle_display_frame < 0xFFU) {
            particle_display_frame += 1;
            return;
        }
        particle_display_frame = 1;
        return;
    }
    sp7B4 = 0;
    do {
        if (target_link & (1 << sp7B4)) {
            particleSort(sp7B4, particle_display_frame, &sorted_particles,
                         &non_edge_particles);
            if (sw == 1) {
                pp = sorted_particles;
            } else {
                pp = non_edge_particles;
            }
            while (pp != NULL) {
                HSD_PSTexGroup* tex_group = NULL;
                HSD_PSFormGroup* form_group = NULL;
                u8* form = NULL;
                void* image;
                void* tlut;
                u32 blend_mode;
                u8 alpha0;
                u8 alpha1;
                GXTexWrapMode wrap_s;
                GXTexWrapMode wrap_t;
                f32 scale_s;
                f32 scale_t;
                GXTexFmt fmt;
                u8** tex_table;
                u32 width;
                u32 height;

                if ((sw == 1) && !(pp->kind & TexEdge)) {
                    break;
                }
                if (!(pp->size < FLT_EPSILON)) {
                    if (needs_setup != 0) {
                        sp79C = NULL;
                        particleDraw.point_size = -1;
                        sp7B0 = NULL;
                        particleDraw.line_width = -1;
                        particleDraw.channel_mode = -1;
                        psSetupTevInvalidState();
                        sp7A8 = (u32) -1;
                        prev_kind &= 0xFEFFFFFF;
                        sp7AC = (u32) -1;
                        HSD_FogSet(NULL);
                        particleDraw.channel_material.r = particleDraw.channel_material.g = particleDraw.channel_material.b = 0xFF;
                        particleDraw.channel_ambient.r = particleDraw.channel_ambient.g = particleDraw.channel_ambient.b = 0xFF;
                        particleDraw.channel_material.a = particleDraw.channel_ambient.a = 0xFF;
                        if (draw) {
                            GXSetChanMatColor(GX_COLOR0A0, particleDraw.channel_material);
                            GXSetChanAmbColor(GX_COLOR0A0, particleDraw.channel_ambient);
                        }
                        psSetupTevInvalidState();
                        psSetupTevCommon();
                        psSetColor(&particleDraw.primary_color, 0xFF);
                        psSetColor(&particleDraw.environment_color, 0);
                        psSetColor(&particleDraw.material_color, 0xFF);
                        if (draw) {
                            GXSetTevColor(GX_TEVREG0, particleDraw.primary_color);
                            GXSetTevColor(GX_TEVREG1, particleDraw.environment_color);
                            GXSetTevColor(GX_TEVREG2, particleDraw.material_color);
                        }
                        particleDraw.blend_mode = -1;
                        if (draw) GXSetZCompLoc(GX_FALSE);
                        HSD_CObjGetViewingMtx(HSD_CObjGetCurrent(), particleDraw.view);
                        if (draw) {
                            C_MTXInverse(particleDraw.view, particleDraw.inverse_view);
                            {
                                f32 w0;
                                f32 x_offset;
                                f32 x_scale;
                                f32 w1;
                                f32 y_offset;
                                f32 y_scale;
                                f32 w2;
                                f32 w3;
                                f32 y0;
                                f32 y1;

                                GXGetProjectionv(particleDraw.projection);
                                if (0.0f == particleDraw.projection[0]) {
                                    x_scale = particleDraw.projection[1];
                                    x_offset = particleDraw.projection[2];
                                    w0 = particleDraw.view[2][0];
                                    particleDraw.projected_view[0][0] =
                                        x_scale * particleDraw.view[0][0] + x_offset * w0;
                                    w1 = particleDraw.view[2][1];
                                    particleDraw.projected_view[0][1] =
                                        x_scale * particleDraw.view[0][1] + x_offset * w1;
                                    w2 = particleDraw.view[2][2];
                                    particleDraw.projected_view[0][2] =
                                        x_scale * particleDraw.view[0][2] + x_offset * w2;
                                    w3 = particleDraw.view[2][3];
                                    particleDraw.projected_view[0][3] =
                                        x_scale * particleDraw.view[0][3] + x_offset * w3;
                                    y_scale = particleDraw.projection[3];
                                    y_offset = particleDraw.projection[4];
                                    y0 = y_offset * w0;
                                    y1 = y_offset * w1;
                                    y2 = y_offset * w2;
                                    particleDraw.projected_view[1][0] = y_scale * particleDraw.view[1][0] + y0;
                                    particleDraw.projected_view[1][1] = y_scale * particleDraw.view[1][1] + y1;
                                    particleDraw.projected_view[1][2] = y_scale * particleDraw.view[1][2] + y2;
                                    particleDraw.projected_view[1][3] =
                                        y_scale * particleDraw.view[1][3] + (y_offset * w3);
                                } else {
                                    particleDraw.projected_view[0][0] = particleDraw.projection[1] * particleDraw.view[0][0] + particleDraw.projection[2];
                                    particleDraw.projected_view[0][1] = particleDraw.projection[1] * particleDraw.view[0][1] + particleDraw.projection[2];
                                    particleDraw.projected_view[0][2] = particleDraw.projection[1] * particleDraw.view[0][2] + particleDraw.projection[2];
                                    particleDraw.projected_view[0][3] = particleDraw.projection[1] * particleDraw.view[0][3] + particleDraw.projection[2];
                                    particleDraw.projected_view[1][0] = particleDraw.projection[3] * particleDraw.view[1][0] + particleDraw.projection[4];
                                    particleDraw.projected_view[1][1] = particleDraw.projection[3] * particleDraw.view[1][1] + particleDraw.projection[4];
                                    particleDraw.projected_view[1][2] = particleDraw.projection[3] * particleDraw.view[1][2] + particleDraw.projection[4];
                                    particleDraw.projected_view[1][3] = particleDraw.projection[3] * particleDraw.view[1][3] + particleDraw.projection[4];
                                }
                                /// @todo Passing @c particleDraw.inverse_view directly swaps the first
                                /// two axis loads.
                                psUpdateBillboardAxes(*(const Mtx*) particleDraw.inverse_view);
                            }
                            GXLoadPosMtxImm(particleDraw.view, GX_PNMTX0);
                            billboard_mtx = HSD_PSDisp_803B9628;
                            GXLoadPosMtxImm(billboard_mtx.mtx, GX_PNMTX1);
                            particleDraw.current_matrix = GX_PNMTX1;
                            psSetParticleDrawMtx(GX_PNMTX0);
                            GXEnableTexOffsets(GX_TEXCOORD0, GX_TRUE, GX_TRUE);
                            GXSetCullMode(GX_CULL_BACK);
                            GXSETARRAY(GX_VA_TEX0, HSD_PSDisp_8040C340,
                                       sizeof(HSD_PSDisp_8040C340), 2, true);
                            psSetupVtxFormat(GX_VTXFMT0, false, true, GX_RGB565);
                            psSetupVtxFormat(GX_VTXFMT1, false, false, GX_RGB565);
                            psSetupVtxFormat(GX_VTXFMT2, true, true, GX_RGB565);
                            psSetupVtxFormat(GX_VTXFMT3, true, false, GX_RGB565);
                            psSetupVtxFormat(GX_VTXFMT4, false, true, GX_RGBA6);
                            psSetupVtxFormat(GX_VTXFMT5, true, true, GX_RGBA6);
                        }
                        needs_setup = 0;
                    }

                    if (draw) {
                        blend_mode = (pp->kind >> 0x16U) & 3;
                        setBlendMode(blend_mode);

                        if (pp->aCmpCount != 0) {
                            s32 scale = (65536 * pp->aCmpRemain) / pp->aCmpCount;
                            alpha0 = ((pp->aCmpParam1Target << 16) +
                                      scale * (pp->aCmpParam1 -
                                               pp->aCmpParam1Target)) >>
                                     16;
                            alpha1 = ((pp->aCmpParam2Target << 16) +
                                      scale * (pp->aCmpParam2 -
                                               pp->aCmpParam2Target)) >>
                                     16;
                        } else {
                            alpha0 = pp->aCmpParam1;
                            alpha1 = pp->aCmpParam2;
                        }
                        if ((alpha_compare_mode != pp->aCmpMode) ||
                            (sp7A5 != alpha0) || (sp7A4 != alpha1))
                        {
                            sp7A5 = alpha0;
                            alpha_compare_mode = pp->aCmpMode;
                            sp7A4 = alpha1;
                            GXSetAlphaCompare((alpha_compare_mode >> 3) & 7, sp7A5,
                                              (alpha_compare_mode >> 6) & 3,
                                              alpha_compare_mode & 7, sp7A4);
                        }

                    }
                    psSetupTev(pp);
                    setupChanCtrl(pp);
                    setupChanReg(pp);
                    setupTevReg(pp);
                    if (draw) {
                        if ((pp->kind & TexEdge) != sp7AC) {
                            sp7AC = pp->kind & TexEdge;
                            if ((s32) sp7AC != 0) {
                                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
                            } else {
                                GXSetZMode(GX_TRUE, GX_LEQUAL, GX_FALSE);
                            }
                        }
                        if (((pp->kind ^ prev_kind) & DispFog) != 0) {
                            if (pp->kind & DispFog) {
                                HSD_FogSet(HSD_PSDisp_804D7908);
                            } else {
                                HSD_FogSet(NULL);
                            }
                        }

                    }
                    if (draw && ((HSD_PSFormGroup***) psNumCmdList)[pp->bank] !=
                            NULL &&
                        (form_group =
                             ((HSD_PSFormGroup***)
                                  psNumCmdList)[pp->bank][pp->texGroup]) !=
                            NULL
#ifdef MUST_MATCH
                        && form_group->formTable != NULL
#endif
                    )
                    {
                        form = form_group->formTable[pp->poseNum];
                    } else {
                        form = NULL;
                    }

                    if (pp->kind & DispTexture) {
                        if (pp->kind & MirrorS) {
                            scale_s = 2.0f;
                            wrap_s = GX_MIRROR;
                        } else {
                            scale_s = 1.0f;
                            wrap_s = GX_CLAMP;
                        }
                        if (pp->kind & MirrorT) {
                            scale_t = 2.0f;
                            wrap_t = GX_MIRROR;
                        } else {
                            scale_t = 1.0f;
                            wrap_t = GX_CLAMP;
                        }
                        if ((pp->kind & (MirrorS | MirrorT)) != sp7A8) {
                            Mtx temp_mtx;

                            sp7A8 = pp->kind & (MirrorS | MirrorT);
                            sp7B0 = NULL;
                            if (draw) {
                                C_MTXScale(temp_mtx, scale_s, scale_t, 1.0f);
                                if (pp->kind & MirrorT) {
                                    temp_mtx[1][3] = 1.0f;
                                }
                                GXLoadTexMtxImm(temp_mtx, GX_TEXMTX0, GX_MTX2x4);
                                GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4,
                                                  GX_TG_TEX0, GX_TEXMTX0, GX_FALSE,
                                                  GX_PTIDENTITY);
                            }
                        }
                        tex_group = psTexGroupArray[pp->bank][pp->texGroup];
                        if (tex_group != NULL) {
                            fmt = tex_group->fmt;
                            tex_table = tex_group->texTable;
                            width = tex_group->width;
                            height = tex_group->height;
                        } else {
                            fmt = 0;
                            height = 0;
                            width = 0;
                            tex_table = NULL;
                        }
                        if (tex_table != NULL) {
                            image = tex_table[pp->poseNum];
                        } else {
                            image = NULL;
                        }
                        if ((fmt == GX_TF_C4) || (fmt == GX_TF_C8)) {
                            if (tex_table != NULL) {
                                void** palettes =
                                    (void**) &tex_table[tex_group->num];
                                if (palettes != NULL) {
                                    if (pp->palNum != 0xFF) {
                                        tlut = palettes[pp->palNum];
                                    } else if (!(pp->kind & ComTLUT)) {
                                        tlut = palettes[pp->poseNum];
                                    } else {
                                        tlut = palettes[0];
                                    }
                                    if (tlut != sp79C) {
                                        tlut_obj.fmt = (GXTlutFmt) (u8)
                                                           tex_group->tlutfmt;
                                        tlut_obj.tlut_name = GX_TLUT0;
                                        tlut_obj.n_entries =
                                            (fmt == GX_TF_C4) ? 0x10 : 0x100;
                                        GXInitTlutObj(&gx_tlut_obj, tlut,
                                                      tlut_obj.fmt,
                                                      tlut_obj.n_entries);
                                        GXLoadTlut(&gx_tlut_obj,
                                                   tlut_obj.tlut_name);
                                    }
                                    sp7B0 = NULL;
                                }
                            }
                        }
                        if ((sp7B0 != image) && (image != NULL)) {
                            sp7B0 = image;
                            switch (fmt) {
                            case GX_TF_C4:
                            case GX_TF_C8:
                                GXInitTexObjCI(&sp764, image, width, height,
                                               fmt, wrap_s, wrap_t, GX_FALSE,
                                               GX_TLUT0);
                                break;
                            case GX_TF_I4:
                            case GX_TF_I8:
                            case GX_TF_IA4:
                            case GX_TF_IA8:
                            case GX_TF_RGB565:
                            case GX_TF_RGB5A3:
                            case GX_TF_RGBA8:
                            case GX_TF_CMPR:
                                GXInitTexObj(&sp764, image, width, height, fmt,
                                             wrap_s, wrap_t, GX_FALSE);
                                break;
                            default:
                                HSD_ASSERT(0x8AA, 0);
                                break;
                            }
                            prev_tex_interp_near = pp->kind & TexInterpNear;
                            GXInitTexObjLOD(
                                &sp764,
                                (prev_tex_interp_near != 0) ? GX_NEAR
                                                            : GX_LINEAR,
                                (pp->kind & TexInterpNear) ? GX_NEAR
                                                           : GX_LINEAR,
                                0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE,
                                GX_ANISO_1);
                            GXLoadTexObj(&sp764, GX_TEXMAP0);
                        }
                        if ((u32) prev_tex_interp_near !=
                            (pp->kind & TexInterpNear))
                        {
                            prev_tex_interp_near = pp->kind & TexInterpNear;
                            GXInitTexObjLOD(
                                &sp764,
                                (prev_tex_interp_near != 0) ? GX_NEAR
                                                            : GX_LINEAR,
                                (s32) (pp->kind & TexInterpNear) != 0
                                    ? GX_NEAR
                                    : GX_LINEAR,
                                0.0f, 0.0f, 0.0f, GX_FALSE, GX_FALSE,
                                GX_ANISO_1);
                            GXLoadTexObj(&sp764, GX_TEXMAP0);
                        }
                    }

                    if (pp->kind & DispPoint) {
                        if (pp->appsrt != NULL) {
                            psDispSubAPPSRTPoint(pp);
                        } else {
                            if (pp->kind & Trail) {
                                pp = psDispSubPointTrail(pp);
                            } else {
                                pp = psDispSubPoint(pp);
                            }
                        }
                    } else if (pp->appsrt != NULL) {
                        psDispSubAppSRT(pp, form);
                    } else {
                        psDispSub(pp, form);
                    }
                }

                prev_kind = pp->kind;
                pp = pp->next;
            }
        }
        sp7B4 += 1;
    } while (sp7B4 < 0x10);
    if (needs_setup == 0) {
        HSD_StateInvalidate(-1);
    }
}
#ifdef MUST_MATCH
#pragma pop
#endif
