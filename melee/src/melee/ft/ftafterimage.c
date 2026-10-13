/// @file
/// @brief Creates trailing "afterimages" for swords in motion

#include "ftafterimage.h"

#include <placeholder.h>

#include "inlines.h"
#include "ftparts.h"
#include "fighter.h"
#include "native_fighter_runtime.h"
#include "native_fighter_trail.h"
#ifdef MELEE_NATIVE
#include "native_fighter_lifetime.h"
#include "native_source_package.h"
#include <sysdolphin/baselib/class.h>
#endif
#include "kinds/ftLink/types.h"
#include "kinds/ftMars/types.h"
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <melee/it/it_26B1.h>
#include <melee/it/kinds/itsword.h>
#include <melee/lb/lb_00B0.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/cobj.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/state.h>

typedef struct AfterimageVtx {
    f32 x, y, z;
    u8 r, g, b, a;
} AfterimageVtx;

#define AFTERIMAGE_ANGLE_STEP 0.0872664600610733f

static bool read_trail(Fighter_GObj* gobj, NativeFighterTrail* trail,
                       HSD_JObj** anchor)
{
    Fighter* fp = GET_FIGHTER(gobj);
    const itSword_UnkBytes* material = NULL;
    *trail = (NativeFighterTrail){ .bone = -1 };
    if (fp->x2101_bits_8) {
        if (fp->item_gobj == NULL) return false;
        HSD_ASSERTREPORT(0x7C, itGetKind(fp->item_gobj) == It_Kind_Sword,
                         "no afterimage item!\n");
        material = it_80285300(fp->item_gobj);
        it_802852B8(fp->item_gobj, &trail->inner_distance,
                     &trail->outer_distance);
        trail->axis = 1;
        *anchor = it_80285314(fp->item_gobj);
    } else {
        const struct SwordAttrs* attrs = NULL;
#ifdef MELEE_NATIVE
        const NativeSourcePackage* package = nativeFighterRuntime_SourcePackage(fp->kind);
        if (package != NULL) {
            NativeFighterLifetime source;
            if (!nativeFighterLifetime_Capture(fp, &source) || !source.source_owned ||
                !nativeScriptFighter_HasHook(gobj, NativeScriptFighter_Trail))
                return false;
            NativeScriptRuntimeContext context = { .source = gobj,
                .source_class = NativeCallbackSource_Fighter,
                .generation = source.generation, .scene_epoch = source.reference.lifetime };
            if (nativeScriptBindings_SourceFighterLayout(&context, NULL) !=
                    &source.program->bank->motion_layout)
                return false;
            FighterBone* parts = fp->parts;
            unsigned count = package->assets.parts.parts_num;
            if (!parts || !count || count > MAX_FT_PARTS ||
                package->assets.facts.joints > count ||
                package->assets.facts.omission_count != count - package->assets.facts.joints ||
                !ftPartsTable || ftPartsTable[fp->kind] != &package->assets.parts ||
                package->assets.parts.part_to_joint != package->assets.part_to_joint ||
                package->assets.parts.joint_to_part != package->assets.joint_to_part)
                return false;
            NativeFighterTrail candidate = { .bone = -1 };
            NativeScriptEventRecordIO record = { &candidate, sizeof(candidate) };
            if (nativeScriptFighter_Run(gobj, NativeScriptFighter_Trail, NULL, 0,
                    &record, NULL) != NativeScriptVM_Ok ||
                !nativeFighterLifetime_Current(&source) || fp->parts != parts ||
                nativeFighterRuntime_SourcePackage(source.kind) != package ||
                ftPartsTable[source.kind] != &package->assets.parts ||
                package->assets.parts.parts_num != count ||
                !nativeFighterTrail_Valid(&candidate, count))
                return false;
            HSD_JObj* joint = parts[candidate.bone].joint;
            if (!joint) return false;
            HSD_ClassInfo* info = HSD_CLASS_METHOD(joint);
            if (!info || info->head.obj_size < (int) sizeof(*joint) ||
                !hsdObjIsDescendantOf((HSD_Obj*) joint, HSD_CLASS_INFO(&hsdJObj)) ||
                !nativeFighterLifetime_Current(&source) || fp->parts != parts ||
                parts[candidate.bone].joint != joint || HSD_CLASS_METHOD(joint) != info ||
                nativeFighterRuntime_SourcePackage(source.kind) != package ||
                ftPartsTable[source.kind] != &package->assets.parts ||
                package->assets.parts.parts_num != count ||
                package->assets.parts.part_to_joint != package->assets.part_to_joint ||
                package->assets.parts.joint_to_part != package->assets.joint_to_part ||
                nativeScriptBindings_SourceFighterLayout(&context, NULL) !=
                    &source.program->bank->motion_layout)
                return false;
            *trail = candidate;
            *anchor = joint;
            return true;
        }
        const NativeFighterDefinition* definition =
            nativeFighterRuntime_Definition(fp->kind);
        if (definition != NULL) {
            if (definition->on_trail == NULL) return false;
            definition->on_trail(gobj, trail);
            if (!nativeFighterTrail_Valid(trail, definition->joint_count))
                return false;
            *anchor = fp->parts[trail->bone].joint;
            return *anchor != NULL;
        }
#endif
        switch (fp->kind) {
        case Ft_Kind_Link:
        case Ft_Kind_CLink: {
            ftLk_DatAttrs* da = fp->dat_attrs;
            attrs = &da->x64;
            break;
        }
        case Ft_Kind_Mars:
        case Ft_Kind_Emblem: {
            MarsAttributes* da = fp->dat_attrs;
            attrs = &da->x78;
            break;
        }
        default:
            return false;
        }
        material = (const itSword_UnkBytes*) attrs;
        trail->bone = attrs->x14;
        trail->inner_distance = attrs->x18;
        trail->outer_distance = attrs->x1C;
        if (trail->bone < 0 || trail->bone >= ftPartsTable[fp->kind]->parts_num)
            return false;
        *anchor = fp->parts[trail->bone].joint;
    }
    trail->inner_fade = material->x0;
    trail->outer_fade = material->x4;
    trail->start_alpha = material->x8;
    trail->end_alpha = material->x9;
    trail->inner_red = material->xA;
    trail->inner_green = material->xB;
    trail->inner_blue = material->xC;
    trail->outer_red = material->xE;
    trail->outer_green = material->xF;
    trail->outer_blue = material->x10;
    return *anchor != NULL;
}

void ftCo_800C2600(Fighter_GObj* gobj, u32 arg1)
{
    Fighter* fp;
    NativeFighterTrail trail;
    const NativeFighterTrail* params = &trail;
    HSD_JObj* anchor;
    f32 cumDist[3];
    AfterimageVtx vtx_buf[152];
    f32 d2;
    s32 distIdx;

    if (arg1 != 2) {
        return;
    }

    fp = GET_FIGHTER(gobj);

    if (fp->x2100 <= 1) {
        return;
    }
    if (!read_trail(gobj, &trail, &anchor)) return;

    GXSetColorUpdate(1);
    GXSetAlphaUpdate(0);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
    GXSetZMode(1, GX_LEQUAL, 0);
    GXSetZCompLoc(0);
    GXSetNumTexGens(0);
    GXSetNumTevStages(1);
    GXSetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD_NULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GXSetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GXSetNumChans(1);
    GXSetChanCtrl(GX_COLOR0A0, 0, GX_SRC_REG, GX_SRC_VTX, 0, GX_DF_NONE,
                  GX_AF_NONE);
    GXSetCullMode(GX_CULL_NONE);

    {
        s32 remaining;
        struct Fighter_x20B0_t* curEntry;
        s32 nextIdx;
        f32 x20F8 = fp->x20F8;
        f32 x20FC = fp->x20FC;
        s32 ringIdx;
        f32 totalDist;
        f32* dp;
        Vec3 prevPos, delta, crossProd, tempDir;

        PAD_STACK(0xC);

        {
            s32 idx = fp->x2101_bits_0to6;
            if (idx != 0) {
                ringIdx = idx - 1;
            } else {
                ringIdx = 2;
            }
        }

        cumDist[0] = 0.0f;
        totalDist = 0.0f;
        dp = &cumDist[1];

        {
            s32 i;
            s32 curIdx = ringIdx;

            prevPos.x = prevPos.y = prevPos.z = 0.0f;

            for (i = fp->x2100 - 1; i >= 0; i--) {
                struct Fighter_x20B0_t* entry = &fp->x20B0[curIdx];

                delta.x = entry->xC.x * x20FC + entry->x0.x - prevPos.x;
                delta.y = entry->xC.y * x20FC + entry->x0.y - prevPos.y;
                delta.z = entry->xC.z * x20FC + entry->x0.z - prevPos.z;

                if (i != fp->x2100 - 1) {
                    d2 = delta.z * delta.z +
                         (delta.x * delta.x + delta.y * delta.y);
                    d2 = sqrtf(d2);
                    totalDist += d2;
                    *dp = totalDist;
                    dp++;
                }

                prevPos = delta;

                if (i != 0) {
                    nextIdx = curIdx != 0 ? curIdx - 1 : 2;
                }
                curIdx = nextIdx;
            }
        }

        if (totalDist < 0.00001f) {
            totalDist = 1.0f;
        }

        {
            s32 numVerts;
            f32 scaleDiff;
            s32 curIdx2;
            s32 idx2;
            f32 blendedOuter;
            f32 blendedInner;
            f32 interpFactor;
            f32 innerDiff;
            f32 outerDiff;
            AfterimageVtx* vp;

            scaleDiff = x20FC - x20F8;
            idx2 = fp->x2101_bits_0to6;
            blendedInner = params->inner_fade * scaleDiff + x20F8;
            vp = vtx_buf;
            blendedOuter = params->outer_fade * scaleDiff + x20F8;
            numVerts = 0;

            if (idx2 != 0) {
                curIdx2 = idx2 - 1;
            } else {
                curIdx2 = 2;
            }

            innerDiff = x20F8 - blendedInner;
            interpFactor = 1.0f;
            remaining = fp->x2100 - 1;
            outerDiff = x20FC - blendedOuter;
            distIdx = 0;

            for (; remaining >= 0; remaining--) {
                f32 outerScale, innerScale;
                s32 alpha;
                struct Fighter_x20B0_t* nextEntry;
                s32 numSubdiv;

                curEntry = &fp->x20B0[curIdx2];
                outerScale = interpFactor * outerDiff + blendedOuter;
                innerScale = interpFactor * innerDiff + blendedInner;
                numVerts += 2;
                distIdx++;

                alpha = (s32) (interpFactor * (f32) (params->start_alpha - params->end_alpha) +
                               (f32) params->end_alpha);

                vp->x = curEntry->xC.x * innerScale + curEntry->x0.x;
                vp->y = curEntry->xC.y * innerScale + curEntry->x0.y;
                vp->z = curEntry->xC.z * innerScale + curEntry->x0.z;
                vp->r = params->inner_red;
                vp->g = params->inner_green;
                vp->b = params->inner_blue;
                vp->a = alpha;

                vp[1].x = curEntry->xC.x * outerScale + curEntry->x0.x;
                vp[1].y = curEntry->xC.y * outerScale + curEntry->x0.y;
                vp[1].z = curEntry->xC.z * outerScale + curEntry->x0.z;
                vp[1].r = params->outer_red;
                vp[1].g = params->outer_green;
                vp[1].b = params->outer_blue;
                vp[1].a = alpha;

                vp += 2;

                if (remaining != 0) {
                    s32 nextRingIdx;

                    if (curIdx2 != 0) {
                        nextRingIdx = curIdx2 - 1;
                    } else {
                        nextRingIdx = 2;
                    }
                    nextEntry = &fp->x20B0[nextRingIdx];
                    nextIdx = nextRingIdx;

                    if (lbVector_CrossprodNormalized(
                            &curEntry->xC, &nextEntry->xC, &crossProd) != NULL)
                    {
                        f32 angle =
                            lbVector_Angle(&curEntry->xC, &nextEntry->xC);
                        f32 subdivAngle = angle / AFTERIMAGE_ANGLE_STEP;
                        numSubdiv = (s32) subdivAngle;
                        interpFactor = 1.0f - (cumDist[distIdx] / totalDist);

                        if (numSubdiv) {
                            f32 frac;
                            s32 j;
                            f32 cumAngle = 0.0f;
                            f32 basePosX, basePosY, basePosZ;
                            f32 stepPosX, stepPosY, stepPosZ;
                            f32 interpInner2, interpOuter2;
                            s32 alphaStep;

                            frac = 1.0f / (f32) (numSubdiv + 1);

                            angle *= frac;
                            tempDir = curEntry->xC;
                            interpInner2 =
                                frac *
                                ((interpFactor * innerDiff + blendedInner) -
                                 innerScale);
                            basePosX = curEntry->x0.x;
                            basePosY = curEntry->x0.y;
                            basePosZ = curEntry->x0.z;

                            stepPosX = frac * (nextEntry->x0.x - basePosX);
                            stepPosY = frac * (nextEntry->x0.y - basePosY);
                            stepPosZ = frac * (nextEntry->x0.z - basePosZ);

                            alphaStep =
                                (s32) (frac *
                                       ((interpFactor *
                                             (f32) (params->start_alpha - params->end_alpha) +
                                         (f32) params->end_alpha) -
                                        (f32) alpha));

                            interpOuter2 =
                                frac *
                                ((interpFactor * outerDiff + blendedOuter) -
                                 outerScale);

                            for (j = 0; j < numSubdiv; j++) {
                                cumAngle += angle;
                                basePosX += stepPosX;
                                basePosY += stepPosY;
                                basePosZ += stepPosZ;
                                innerScale += interpInner2;
                                outerScale += interpOuter2;
                                alpha += alphaStep;

                                tempDir = curEntry->xC;
                                lbVector_RotateAboutUnitAxis(
                                    &tempDir, &crossProd, cumAngle);

                                numVerts += 2;
                                vp->x = tempDir.x * innerScale + basePosX;
                                vp->y = tempDir.y * innerScale + basePosY;
                                vp->z = tempDir.z * innerScale + basePosZ;
                                vp->r = params->inner_red;
                                vp->g = params->inner_green;
                                vp->b = params->inner_blue;
                                vp->a = alpha;
                                vp[1].x = tempDir.x * outerScale + basePosX;
                                vp[1].y = tempDir.y * outerScale + basePosY;
                                vp[1].z = tempDir.z * outerScale + basePosZ;
                                vp[1].r = params->outer_red;
                                vp[1].g = params->outer_green;
                                vp[1].b = params->outer_blue;
                                vp[1].a = alpha;
                                vp += 2;
                            }
                        }
                    }
                }

                curIdx2 = nextIdx;
            }

            GXClearVtxDesc();
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_TEX_ST, GX_RGBA6, 0);
            GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_TEX_ST, GX_RGBA8, 0);
            GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
            GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT);
            GXLoadPosMtxImm(
                HSD_CObjGetViewingMtxPtrDirect(HSD_CObjGetCurrent()), 0);
            GXSetCurrentMtx(0);
            GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT0, numVerts + 1);

            GXPosition3f32(vtx_buf[0].x, vtx_buf[0].y, vtx_buf[0].z);
            GXColor4u8(vtx_buf[0].r, vtx_buf[0].g, vtx_buf[0].b, vtx_buf[0].a);
            {
                AfterimageVtx* vtx = &vtx_buf[2];

                GXPosition3f32(vtx->x, vtx->y, vtx->z);
                {
                    u8 r = vtx->r;
                    GXColor4u8(r, vtx->g, vtx->b, vtx->a);
                }
            }

            {
                s32 i;
                AfterimageVtx* vtx = &vtx_buf[1];

                for (i = 1; i < numVerts; i++, vtx++) {
                    GXPosition3f32(vtx->x, vtx->y, vtx->z);
                    {
                        u8 r = vtx->r;
                        GXColor4u8(r, vtx->g, vtx->b, vtx->a);
                    }
                }
            }
        }
    }

    HSD_StateInvalidate(-1);
}

void ftCo_800C2FD8(Fighter_GObj* gobj)
{
    Fighter* fp;
    HSD_JObj* jobj;
    struct Fighter_x20B0_t* entry;
    int axis;
    int nextIndex;
    PAD_STACK(0x8);

    fp = GET_FIGHTER(gobj);
    if (fp->x2100 == -1) {
        return;
    }

    entry = &fp->x20B0[fp->x2101_bits_0to6];
    NativeFighterTrail trail;
    if (!read_trail(gobj, &trail, &jobj)) {
        fp->x2100 = -1;
        return;
    }
    axis = trail.axis;
    fp->x20F8 = trail.inner_distance;
    fp->x20FC = trail.outer_distance;
    lb_8000B1CC(jobj, NULL, &entry->x0);
    entry->xC.x = jobj->mtx[0][axis];
    entry->xC.y = jobj->mtx[1][axis];
    entry->xC.z = jobj->mtx[2][axis];
    if (fp->x2101_bits_0to6 == 2) {
        nextIndex = 0;
    } else {
        nextIndex = fp->x2101_bits_0to6 + 1;
    }
    fp->x2101_bits_0to6 = nextIndex;
    if (fp->x2100 < 3) {
        fp->x2100++;
    }
}
