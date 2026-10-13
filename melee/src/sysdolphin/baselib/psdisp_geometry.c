#include "psdisp_internal.h"
#include "psdisp_state_internal.h"
#include "native_render_phase.h"
#include "mtx.h"
#include "state.h"
#include "util.h"

#undef FLT_EPSILON
#define FLT_EPSILON 1.19209290e-07F

static void calcTornadoLastPos(HSD_Particle* pp, f32* x, f32* y, f32* z)
{
    f32 radius;
    f32 px, py, pz;
    f32 sina, sinb, cosa, cosb;
    f32 vx0, vz0;
    HSD_Generator* gp;

    gp = pp->gen;
    /// @bug The following should be accessing pp not gp
    if (gp == NULL) {
        *x = gp->pos.x;
        *y = gp->pos.y;
        *z = gp->pos.z;
        return;
    }

    sina = sinf(pp->grav);
    sinb = sinf(pp->fric);
    cosa = cosf(pp->grav);
    cosb = cosf(pp->fric);

    vz0 = pp->vel.z - gp->aux.tornado.vel;
    vx0 = pp->vel.x - gp->grav;

    radius = ABS(gp->radius);
    radius += vz0 * tanf(ABS(gp->angle));
    radius *= pp->vel.y;
    px = radius * cosf(vx0);
    py = radius * sinf(vx0);
    pz = vz0;

    *x = px * cosb + pz * sinb + gp->pos.x;
    *y = -px * sina * sinb + py * cosa + pz * sina * cosb + gp->pos.y;
    *z = -px * cosa * sinb - py * sina + pz * cosa * cosb + gp->pos.z;
}

static bool psPointBatchMatches(const HSD_Particle* first,
                                const HSD_Particle* candidate, bool trail)
{
    if (candidate->size != first->size || candidate->appsrt != NULL ||
        ((candidate->kind ^ first->kind) & 0xC0100400) ||
        (candidate->kind & DispPoint)) {
        return false;
    }
    return trail ||
        (candidate->primColCount == 0 &&
         candidate->primCol.r == first->primCol.r &&
         candidate->primCol.g == first->primCol.g &&
         candidate->primCol.b == first->primCol.b &&
         candidate->primCol.a == first->primCol.a &&
         (!(first->kind & DispLighting) ||
          (candidate->matColCount == 0 && candidate->ambColCount == 0 &&
           candidate->matRGB == first->matRGB && candidate->matA == first->matA &&
           candidate->ambRGB == first->ambRGB && candidate->ambA == first->ambA)));
}

HSD_Particle* psDispSubPoint(HSD_Particle* pp)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) {
        ++nativeRenderParticleBatchesSkipped;
        HSD_Particle* last = pp;
        while (last->next && psPointBatchMatches(pp, last->next, false)) {
            last = last->next;
        }
        return last;
    }
#endif
    Vec3 buf[16];
    Vec3* p;
    HSD_Particle* last;
    HSD_Particle* q;
    s32 count;
    s32 i;
    u8 w;

    psSetParticleDrawMtx(GX_PNMTX0);
    w = (pp->size > 42.5) ? 255.0f : 6.0f * pp->size;
    if (particleDraw.point_size != (s32) w) {
        particleDraw.point_size = w;
        GXSetPointSize(w, GX_TO_ONE);
    }
    last = pp;
    p = buf;
    p->x = pp->pos.x;
    p->y = pp->pos.y;
    p->z = pp->pos.z;
    p++;
    count = 1;
    q = pp->next;
    while (q != NULL) {
        if (psPointBatchMatches(pp, q, false))
        {
            count++;
            p->x = q->pos.x;
            p->y = q->pos.y;
            p->z = q->pos.z;
            p++;
            if (count == 16) {
                p = buf;
                if (pp->kind & DispTexture) {
                    setVtxDesc(0);
                    GXBegin(GX_POINTS, GX_VTXFMT0, 16U);
                } else {
                    setVtxDesc(1);
                    GXBegin(GX_POINTS, GX_VTXFMT1, 16U);
                }
                for (i = count; i != 0; i--) {
                    GXPosition3f32(p->x, p->y, p->z);
                    p++;
                    if (pp->kind & DispTexture) {
                        GXTexCoord1x8(1);
                    }
                }
                p = buf;
                count = 0;
            }
            q = (last = q)->next;
        } else {
            break;
        }
    }
    if (count != 0) {
        p = buf;
        if (pp->kind & DispTexture) {
            setVtxDesc(0);
            GXBegin(GX_POINTS, GX_VTXFMT0, (u16) count);
        } else {
            setVtxDesc(1);
            GXBegin(GX_POINTS, GX_VTXFMT1, (u16) count);
        }
        for (i = count; i != 0; i--) {
            GXPosition3f32(p->x, p->y, p->z);
            p++;
            if (pp->kind & DispTexture) {
                GXTexCoord1x8(1);
            }
        }
        /* Keeping the walker alive to the end of the batch is what gives the
         * flush pointer and `last` their retail registers. */
        if (q != NULL) {
        }
    }
    return last;
}

HSD_Particle* psDispSubPointTrail(HSD_Particle* pp)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) {
        ++nativeRenderParticleBatchesSkipped;
        HSD_Particle* last = pp;
        while (last->next && psPointBatchMatches(pp, last->next, true)) {
            last = last->next;
        }
        return last;
    }
#endif
    Vec3 vbuf[32];
    GXColor cbuf[32];
    Vec3* p;
    GXColor* c;
    HSD_Particle* last;
    HSD_Particle* q;
    s32 count;
    s32 i;
    u8 w;

    (void) c;
    psSetParticleDrawMtx(GX_PNMTX0);
    w = (pp->size > 42.5) ? 255.0f : 6.0f * pp->size;
    if (particleDraw.line_width != (s32) w) {
        particleDraw.line_width = w;
        GXSetLineWidth(w, GX_TO_ONE);
    }
    p = vbuf;
    c = cbuf;
    {
        Vec3* dst = p++;
        dst->x = pp->pos.x;
        dst->y = pp->pos.y;
        dst->z = pp->pos.z;
    }
    if (pp->kind & Tornado) {
        f32 x, y, z;
        calcTornadoLastPos(pp, &x, &y, &z);
        p->x = x;
        p->y = y;
        p->z = z;
        p++;
    } else {
        p->x = pp->pos.x - pp->vel.x;
        p->y = pp->pos.y - pp->vel.y;
        p->z = pp->pos.z - pp->vel.z;
        p++;
    }
    psGetTrailColor(pp, c);
    c[1] = c[0];
    count = 1;
    c[1].a = (u8) ((f32) c[1].a * pp->trail);
    c += 2;
    last = pp;
    q = pp->next;
    while (q != NULL) {
        if (psPointBatchMatches(pp, q, true))
        {
            {
                Vec3* dst = p++;
                dst->x = q->pos.x;
                dst->y = q->pos.y;
                dst->z = q->pos.z;
            }
            if (q->kind & Tornado) {
                f32 x, y, z;
                calcTornadoLastPos(q, &x, &y, &z);
                p->x = x;
                p->y = y;
                p->z = z;
                p++;
            } else {
                p->x = q->pos.x - q->vel.x;
                p->y = q->pos.y - q->vel.y;
                p->z = q->pos.z - q->vel.z;
                p++;
            }
            psGetTrailColor(q, c);
            c[1] = c[0];
            count++;
            c[1].a = (u8) ((f32) c[1].a * q->trail);
            c += 2;
            if (count == 16) {
                p = vbuf;
                c = cbuf;
                if (pp->kind & DispTexture) {
                    setVtxDesc(2);
                    GXBegin(GX_LINES, GX_VTXFMT2, 0x20U);
                } else {
                    setVtxDesc(3);
                    GXBegin(GX_LINES, GX_VTXFMT3, 0x20U);
                }
                for (i = count; i != 0; i--) {
                    GXPosition3f32(p[1].x, p[1].y, p[1].z);
                    GXColor4u8(c[1].r, c[1].g, c[1].b, c[1].a);
                    if (pp->kind & DispTexture) {
                        GXTexCoord1x8(0);
                    }
                    GXPosition3f32(p[0].x, p[0].y, p[0].z);
                    GXColor4u8(c[0].r, c[0].g, c[0].b, c[0].a);
                    if (pp->kind & DispTexture) {
                        GXTexCoord1x8(1);
                    }
                    p += 2;
                    c += 2;
                }
                p = vbuf;
                c = cbuf;
                count = 0;
            }
            last = q;
            q = q->next;
        } else {
            break;
        }
    }
    if (count != 0) {
        GXColor* draw_colors;

        p = vbuf;
        draw_colors = cbuf;
        if (pp->kind & DispTexture) {
            setVtxDesc(2);
            GXBegin(GX_LINES, GX_VTXFMT2, count * 2);
        } else {
            setVtxDesc(3);
            GXBegin(GX_LINES, GX_VTXFMT3, count * 2);
        }
        for (i = count; i != 0; i--) {
            GXPosition3f32(p[1].x, p[1].y, p[1].z);
            GXColor4u8(draw_colors[1].r, draw_colors[1].g, draw_colors[1].b,
                       draw_colors[1].a);
            if (pp->kind & DispTexture) {
                GXTexCoord1x8(0);
            }
            GXPosition3f32(p[0].x, p[0].y, p[0].z);
            GXColor4u8(draw_colors[0].r, draw_colors[0].g, draw_colors[0].b,
                       draw_colors[0].a);
            if (pp->kind & DispTexture) {
                GXTexCoord1x8(1);
            }
            p += 2;
            draw_colors += 2;
        }
    }
    (void) p;
    return last;
}



void psSetParticleDrawMtx(GXPosNrmMtx idx)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) return;
#endif
    if (particleDraw.current_matrix != idx) {
        particleDraw.current_matrix = idx;
        GXSetCurrentMtx(idx);
    }
}

static inline void psDispSubMakePolygon(HSD_Particle* pp, u8* texform, f32 x,
                                        f32 y, f32 z, f32 x0, f32 y0, f32 z0,
                                        f32 x1, f32 y1, f32 z1, GXColor* color,
                                        f32* prev_x, f32* prev_y, f32* prev_z)
{
    Vec2 right;
    f32 right_z;
    Vec2 up;
    f32 up_z;
    u8* it = texform;
    /* The embedded assignments below keep three corner arguments out of the
     * inline parameter homes, which is how the retail frame counts them. */
    f32 cx;

    right.x = x0;
    right.y = y0;
    right_z = z0;
    up.x = x1;
    up.y = y1;
    up_z = z1;

    psSetParticleDrawMtx(GX_PNMTX0);
    if (pp->kind & Trail) {
        if (pp->kind & Tornado) {
            calcTornadoLastPos(pp, prev_x, prev_y, prev_z);
        } else {
            *prev_x = x - pp->vel.x;
            *prev_y = y - pp->vel.y;
            *prev_z = z - pp->vel.z;
        }
        psGetTrailColor(pp, color);
        if (it == NULL) {
            if (pp->kind & DispTexture) {
                setVtxDesc(2);
                GXBegin(GX_QUADS, GX_VTXFMT2, 4);
            } else {
                setVtxDesc(3);
                GXBegin(GX_QUADS, GX_VTXFMT3, 4);
            }
            GXPosition3f32((*prev_x) - right.x, (*prev_y) - right.y,
                           (*prev_z) - right_z);
            {
                u8 a = color->a;
                u8 b = color->b;
                u8 g = color->g;
                u16 r = color->r;
                GXColor4u8(r, g, b, (u8) ((f32) a * pp->trail));
            }
            if (pp->kind & DispTexture) {
                u8 tex_base = (pp->kind >> 16) & 0xC;

                GXParam1u8(tex_base);
            }
            GXPosition3f32(x - up.x, y - up.y, z - up_z);
            {
                u8 a = color->a;
                u8 b = color->b;
                u8 g = color->g;
                u16 r = color->r;
                GXColor4u8(r, g, b, a);
            }
            if (pp->kind & DispTexture) {
                GXParam1u8(((pp->kind >> 16) & 0xC) + 1);
            }
            GXPosition3f32(x + right.x, y + right.y, z + right_z);
            {
                u8 a = color->a;
                u8 b = color->b;
                u8 g = color->g;
                u16 r = color->r;
                GXColor4u8(r, g, b, a);
            }
            if (pp->kind & DispTexture) {
                GXParam1u8(((pp->kind >> 16) & 0xC) + 2);
            }
            GXPosition3f32((*prev_x) + up.x, (*prev_y) + up.y,
                           (*prev_z) + up_z);
            {
                u8 a = color->a;
                f32 alpha = (f32) a * pp->trail;
                u8 b = color->b;
                u8 g = color->g;
                u16 r = color->r;
                GXColor4u8(r, g, b, (u8) alpha);
            }
            if (pp->kind & DispTexture) {
                GXParam1u8(((pp->kind >> 16) & 0xC) + 3);
            }
        } else {
            f32 trail_alpha = 255.0f * (1.0f - pp->trail);
            f32 up_len = sqrtf(up.x * up.x + up.y * up.y + up_z * up_z);

            if (up_len != 0.0f) {
                f32 dz = z - (*prev_z);
                f32 dx = x - (*prev_x);
                f32 dy = y - (*prev_y);
                f32 xl = dx * dx;
                f32 yl = dy * dy;
                f32 zl = dz * dz;
                f32 segment_len = sqrtf(zl + (xl + yl));
                f32 ratio = segment_len / up_len;
                u32 primitive_count = *(u32*) it;

                it += sizeof(u32);
                up.x *= ratio;
                up.y *= ratio;
                up_z *= ratio;
                for (; primitive_count != 0; primitive_count--) {
                    GXPrimitive primitive = it[0];
                    u8 count = it[1];
                    s32 i;

                    it += 4;
                    if (pp->kind & DispTexture) {
                        setVtxDesc(5);
                        GXBegin(primitive, GX_VTXFMT5, count);
                    } else {
                        setVtxDesc(3);
                        GXBegin(primitive, GX_VTXFMT3, count);
                    }
                    for (i = count; i > 0; i--) {
                        f32 s = *(f32*) &it[0];
                        f32 sx = 2.0f * (s - 0.5f);
                        f32 t;
                        f32 tx;
                        s32 converted_alpha;
                        s32 alpha;

                        if (pp->kind & TexFlipS) {
                            s = 1.0f - s;
                        }
                        t = *(f32*) &it[4];
                        it += 8;
                        alpha = (s32) (255.0f - t * trail_alpha);
                        converted_alpha = (s32) (255.0f - t * trail_alpha);
                        if (converted_alpha < 0) {
                            alpha = 0;
                        }
                        if (alpha > 0xFF) {
                            alpha = 0xFF;
                        }
                        tx = 2.0f * (t - 0.5f);
                        if (pp->kind & TexFlipT) {
                            t = 1.0f - t;
                        }
                        GXParam1f32(up.x * tx + (right.x * sx + x));
                        GXParam1f32(up.y * tx + (right.y * sx + y));
                        GXParam1f32(up_z * tx + (right_z * sx + z));
                        GXColor4u8(color->r, color->g, color->b, alpha);
                        if (pp->kind & DispTexture) {
                            GXParam1f32(s);
                            GXParam1f32(t);
                        }
                    }
                }
            }
        }
    } else if (it == NULL) {
        if (pp->kind & DispTexture) {
            setVtxDesc(0);
            GXBegin(GX_QUADS, GX_VTXFMT0, 4);
        } else {
            setVtxDesc(1);
            GXBegin(GX_QUADS, GX_VTXFMT1, 4);
        }
        GXPosition3f32((cx = x - right.x), y - right.y, z - right_z);
        if (pp->kind & DispTexture) {
            GXParam1u8((pp->kind >> 16) & 0xC);
        }
        GXPosition3f32((cx = x - up.x), y - up.y, z - up_z);
        if (pp->kind & DispTexture) {
            GXParam1u8(((pp->kind >> 16) & 0xC) + 1);
        }
        GXPosition3f32((cx = x + right.x), y + right.y, z + right_z);
        if (pp->kind & DispTexture) {
            GXParam1u8(((pp->kind >> 16) & 0xC) + 2);
        }
        GXPosition3f32(x + up.x, y + up.y, z + up_z);
        if (pp->kind & DispTexture) {
            GXParam1u8(((pp->kind >> 16) & 0xC) + 3);
        }
    } else {
        u32 primitive_count = *(u32*) it;
        it += sizeof(u32);
        for (; primitive_count != 0; primitive_count--) {
            GXPrimitive primitive = it[0];
            u8 count = it[1];
            s32 i;

            it += 4;
            if (pp->kind & DispTexture) {
                setVtxDesc(4);
                GXBegin(primitive, GX_VTXFMT4, count);
            } else {
                setVtxDesc(1);
                GXBegin(primitive, GX_VTXFMT1, count);
            }
            for (i = count; i > 0; i--) {
                f32 s = *(f32*) &it[0];
                f32 sx = 2.0f * (s - 0.5f);
                f32 t;
                f32 tx;

                if (pp->kind & TexFlipS) {
                    s = 1.0f - s;
                }
                t = *(f32*) &it[4];
                it += 8;
                tx = 2.0f * (t - 0.5f);
                if (pp->kind & TexFlipT) {
                    t = 1.0f - t;
                }
                GXPosition3f32(x + right.x * sx + up.x * tx,
                               y + right.y * sx + up.y * tx,
                               z + right_z * sx + up_z * tx);
                if (pp->kind & DispTexture) {
                    GXParam1f32(s);
                    GXParam1f32(t);
                }
            }
        }
    }
    (void) (up.x + up.y + up_z);
}

static inline void psMaskAbsF32(f32* value)
{
    *(s32*) value &= 0x7FFFFFFF;
}

/* AppSRT tests its magnitudes through these: the compare temp then belongs
 * to this expansion (one inline level below the caller), which is where the
 * retail frame keeps it. */
static inline bool psMaskAbsLtF32(f32 value, f32 limit)
{
    psMaskAbsF32(&value);
    return value < limit;
}

static inline bool psMaskAbsGtF32(f32 value, f64 limit)
{
    psMaskAbsF32(&value);
    return value > limit;
}

/* psDispSub tests its magnitudes through the value helper instead: the retail
 * frame keeps those compare temps one inline level deeper than AppSRT's, after
 * psDispSubMakePolygon's own locals. */
static inline bool psAbsLtF32(f32 value, f32 limit)
{
    *(s32*) &value &= 0x7FFFFFFF;
    return value < limit;
}

static inline bool psAbsGtF32(f32 value, f64 limit)
{
    *(s32*) &value &= 0x7FFFFFFF;
    return value > limit;
}

static inline bool psDispSubAbsLtF32(f32 value, f32 limit)
{
    return psAbsLtF32(value, limit);
}

static inline bool psDispSubAbsGtF32(f32 value, f64 limit)
{
    return psAbsGtF32(value, limit);
}

void psDispSub(HSD_Particle* pp, u8* texform)
{
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) {
        ++nativeRenderParticleBatchesSkipped;
        return;
    }
#endif
    f32 right_y;
    f32 right_x;
    f32 right_z;
    f32 up_x;
    f32 up_y;
    f32 up_z;
    f32 angle;
    f32 x;
    f32 y;
    f32 z;

    x = pp->pos.x;
    y = pp->pos.y;
    z = pp->pos.z;
    if (texform != NULL) {
        right_x = particleDraw.inverse_view[0][0] * pp->size;
        up_x = -particleDraw.inverse_view[0][1] * pp->size;
        right_y = particleDraw.inverse_view[1][0] * pp->size;
        up_y = -particleDraw.inverse_view[1][1] * pp->size;
        right_z = particleDraw.inverse_view[2][0] * pp->size;
        up_z = -particleDraw.inverse_view[2][1] * pp->size;
    } else {
        right_x = particleDraw.right.x * pp->size;
        up_x = particleDraw.up.x * pp->size;
        right_y = particleDraw.right.y * pp->size;
        up_y = particleDraw.up.y * pp->size;
        right_z = particleDraw.right.z * pp->size;
        up_z = particleDraw.up.z * pp->size;
    }
    if ((pp->kind & Trail) || (pp->kind & DirVec)) {
        f32 x;
        f32 y;

        if (0.0f == particleDraw.projection[0]) {
            f32 prev_x;
            f32 prev_y;
            f32 prev_z;
            f32 w0;
            f32 w0inv;
            f32 w1;
            f32 w1inv;
            f32 pv13;
            f32 pv03;

            if (pp->kind & Tornado) {
                calcTornadoLastPos(pp, &prev_x, &prev_y, &prev_z);
            } else {
                prev_x = pp->pos.x - pp->vel.x;
                prev_y = pp->pos.y - pp->vel.y;
                prev_z = pp->pos.z - pp->vel.z;
            }
            w0 = particleDraw.view[2][3] +
                 (particleDraw.view[2][2] * pp->pos.z +
                  (particleDraw.view[2][0] * pp->pos.x + particleDraw.view[2][1] * pp->pos.y));
            if (0.0f == w0) {
                return;
            }
            w0inv = -1.0f / w0;
            w1 = particleDraw.view[2][3] + (particleDraw.view[2][2] * prev_z +
                               (particleDraw.view[2][0] * prev_x + particleDraw.view[2][1] * prev_y));
            if (0.0f == w1) {
                return;
            }
            w1inv = -1.0f / w1;
            pv03 = particleDraw.projected_view[0][3];
            pv13 = particleDraw.projected_view[1][3];
            x = w0inv * (pv03 + (particleDraw.projected_view[0][2] * pp->pos.z +
                                 (particleDraw.projected_view[0][0] * pp->pos.x +
                                  particleDraw.projected_view[0][1] * pp->pos.y))) -
                w1inv *
                    (pv03 + (particleDraw.projected_view[0][2] * prev_z +
                             (particleDraw.projected_view[0][0] * prev_x + particleDraw.projected_view[0][1] * prev_y)));
            y = w0inv * (pv13 + (particleDraw.projected_view[1][2] * pp->pos.z +
                                 (particleDraw.projected_view[1][0] * pp->pos.x +
                                  particleDraw.projected_view[1][1] * pp->pos.y))) -
                w1inv *
                    (pv13 + (particleDraw.projected_view[1][2] * prev_z +
                             (particleDraw.projected_view[1][0] * prev_x + particleDraw.projected_view[1][1] * prev_y)));
            /* The second read pools the y-row product into a shared temp,
             * which is what keeps it out of the spill pair. */
            (void) (particleDraw.projected_view[1][1] * prev_y);
        } else if (pp->kind & Tornado) {
            f32 prev_x;
            f32 prev_y;
            f32 prev_z;
            f32 dx;
            f32 dy;
            f32 dz;

            calcTornadoLastPos(pp, &prev_x, &prev_y, &prev_z);
            dx = pp->pos.x - prev_x;
            dy = pp->pos.y - prev_y;
            dz = pp->pos.z - prev_z;
            x = particleDraw.projected_view[0][2] * dz + (particleDraw.projected_view[0][0] * dx + particleDraw.projected_view[0][1] * dy);
            y = particleDraw.projected_view[1][2] * dz + (particleDraw.projected_view[1][0] * dx + particleDraw.projected_view[1][1] * dy);
        } else {
            x = particleDraw.projected_view[0][2] * pp->vel.z +
                (particleDraw.projected_view[0][0] * pp->vel.x + particleDraw.projected_view[0][1] * pp->vel.y);
            y = particleDraw.projected_view[1][2] * pp->vel.z +
                (particleDraw.projected_view[1][0] * pp->vel.x + particleDraw.projected_view[1][1] * pp->vel.y);
        }
        if (psDispSubAbsLtF32(y, FLT_MIN)) {
            angle = (x >= 0.0f) ? 1.5707964f : -1.5707964f;
        } else {
            angle = atan2f(x, y);
        }
        if (pp->kind & DirVec) {
            angle += pp->rotate;
        }
    } else {
        angle = pp->rotate;
    }
    if (psDispSubAbsGtF32(angle, 0.01)) {
        Mtx mtx;
        Vec3 axis;

        f32 rx = right_x;
        f32 ry = right_y;
        f32 rz = right_z;
        f32 ux = up_x;
        f32 uz = up_z;
        f32 uy = up_y;
        f32 ax;
        f32 ay;
        f32 az;

        ax = ry * uz - rz * uy;
        {
            f32 axis_y_product = rx * uz;
            ay = rz * ux - axis_y_product;
        }
        az = rx * uy - ry * ux;
        axis.x = ax;
        axis.y = ay;
        axis.z = az;
        C_MTXRotAxisRad(mtx, &axis, angle);
        right_x = mtx[0][2] * rz + (mtx[0][0] * rx + mtx[0][1] * ry);
        right_y = mtx[1][2] * rz + (mtx[1][0] * rx + mtx[1][1] * ry);
        right_z = mtx[2][2] * rz + (mtx[2][0] * rx + mtx[2][1] * ry);
        up_x = mtx[0][2] * uz + (mtx[0][0] * ux + mtx[0][1] * uy);
        up_y = mtx[1][2] * uz + (mtx[1][0] * ux + mtx[1][1] * uy);
        up_z = mtx[2][2] * uz + (mtx[2][0] * ux + mtx[2][1] * uy);
    }
    {
        GXColor color;
        f32 prev_x;
        f32 prev_y;
        f32 prev_z;

        psDispSubMakePolygon(pp, texform, x, y, z, right_x, right_y, right_z,
                             up_x, up_y, up_z, &color, &prev_x, &prev_y,
                             &prev_z);
    }
}

static inline void psScaleAppSRTAxes(HSD_Particle* pp, Mtx mtx)
{
    mtx[0][0] *= pp->size;
    mtx[1][0] *= pp->size;
    mtx[2][0] *= pp->size;
    mtx[0][1] *= pp->size;
    mtx[1][1] *= pp->size;
    mtx[2][1] *= pp->size;
    mtx[0][2] *= pp->size;
    mtx[1][2] *= pp->size;
    mtx[2][2] *= pp->size;
}

void psDispSubAPPSRTPoint(HSD_Particle* pp)
{
    f32 cur_x;
    f32 cur_y;
    f32 cur_z;
    f32 prev_x;
    f32 prev_y;
    f32 prev_z;
    u8 w;

    psSetParticleDrawMtx(GX_PNMTX1);
    psPrepareParticleAppSRT(pp->appsrt, particleDraw.view, particle_display_frame);
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) {
        ++nativeRenderParticleBatchesSkipped;
        return;
    }
#endif
    cur_x = pp->appsrt->x70 +
            (pp->appsrt->x6C * pp->pos.z +
             (pp->appsrt->ssx * pp->pos.x + pp->appsrt->ssy * pp->pos.y));
    cur_y = pp->appsrt->x80 +
            (pp->appsrt->x7C * pp->pos.z +
             (pp->appsrt->x74 * pp->pos.x + pp->appsrt->x78 * pp->pos.y));
    cur_z = pp->appsrt->x90 +
            (pp->appsrt->x8C * pp->pos.z +
             (pp->appsrt->x84 * pp->pos.x + pp->appsrt->x88 * pp->pos.y));
    (void) (pp->pos.x, pp->pos.y);
    (void) (pp->pos.z, pp->size);
    if (pp->kind & Tornado) {
        f32 x;
        f32 y;
        f32 z;

        calcTornadoLastPos(pp, &x, &y, &z);
        prev_x =
            pp->appsrt->x70 + (pp->appsrt->x6C * z +
                               (pp->appsrt->ssx * x + pp->appsrt->ssy * y));
        prev_y =
            pp->appsrt->x80 + (pp->appsrt->x7C * z +
                               (pp->appsrt->x74 * x + pp->appsrt->x78 * y));
        prev_z =
            pp->appsrt->x90 + (pp->appsrt->x8C * z +
                               (pp->appsrt->x84 * x + pp->appsrt->x88 * y));
    } else {
        prev_x =
            pp->appsrt->x70 + (pp->appsrt->x6C * (pp->pos.z - pp->vel.z) +
                               (pp->appsrt->ssx * (pp->pos.x - pp->vel.x) +
                                pp->appsrt->ssy * (pp->pos.y - pp->vel.y)));
        prev_y =
            pp->appsrt->x80 + (pp->appsrt->x7C * (pp->pos.z - pp->vel.z) +
                               (pp->appsrt->x74 * (pp->pos.x - pp->vel.x) +
                                pp->appsrt->x78 * (pp->pos.y - pp->vel.y)));
        prev_z =
            pp->appsrt->x90 + (pp->appsrt->x8C * (pp->pos.z - pp->vel.z) +
                               (pp->appsrt->x84 * (pp->pos.x - pp->vel.x) +
                                pp->appsrt->x88 * (pp->pos.y - pp->vel.y)));
    }

    w = (pp->size > 42.5) ? 255.0f : 6.0f * pp->size;
    if (pp->kind & Trail) {
        GXColor draw_color;

        if (particleDraw.line_width != (s32) w) {
            particleDraw.line_width = w;
            GXSetLineWidth(w, GX_TO_ONE);
        }
        psGetTrailColor(pp, &draw_color);
        if (pp->kind & DispTexture) {
            setVtxDesc(2);
            GXBegin(GX_LINES, GX_VTXFMT2, 2);
        } else {
            setVtxDesc(3);
            GXBegin(GX_LINES, GX_VTXFMT3, 2);
        }
        GXPosition3f32(prev_x, prev_y, prev_z);
        GXColor4u8(draw_color.r, draw_color.g, draw_color.b,
                   (u8) ((f32) draw_color.a * pp->trail));
        if (pp->kind & DispTexture) {
            GXTexCoord1x8(0);
        }
        GXPosition3f32(cur_x, cur_y, cur_z);
        GXColor4u8(draw_color.r, draw_color.g, draw_color.b, draw_color.a);
        if (pp->kind & DispTexture) {
            GXTexCoord1x8(1);
        }
    } else {
        if (particleDraw.point_size != (s32) w) {
            particleDraw.point_size = w;
            GXSetPointSize(w, GX_TO_ONE);
        }
        if (pp->kind & DispTexture) {
            setVtxDesc(0);
            GXBegin(GX_POINTS, GX_VTXFMT0, 1);
        } else {
            setVtxDesc(1);
            GXBegin(GX_POINTS, GX_VTXFMT1, 1);
        }
        GXPosition3f32(cur_x, cur_y, cur_z);
        if (pp->kind & DispTexture) {
            GXTexCoord1x8(1);
        }
    }
}

void psDispSubAppSRT(HSD_Particle* pp, u8* texform)
{
    Mtx draw_mtx;
    /* Retail keeps a dead 12-byte aggregate at this position. */
    Vec3 pad;
    f32 x_extent;
    f32 y_extent;
    f32 ax;
    f32 ay;
    f32 bx;
    f32 by;
    f32 angle;
    u8* it = texform;
    /* The retail spill band orders the projection coefficients below the
     * previous-position webs: the perspective and orthographic coefficient
     * locals are declared here, after the w terms and before prev_pos_*. */
    f32 w0;
    f32 w1;
    f32 w0inv;
    f32 w1inv;
    f32 f11;
    f32 f8;
    f32 f12;
    f32 f13;
    f32 f16;
    f32 f20;
    f32 f17;
    f32 f18;
    f32 f20b;
    f32 s7F8;
    f32 s7FC;
    f32 s800;
    f32 s804;
    f32 s808;
    f32 prev_pos_x;
    f32 prev_pos_y;
    f32 prev_pos_z;
    f32 cur_x;
    f32 cur_y;
    f32 cur_z;

    psPrepareParticleAppSRT(pp->appsrt, particleDraw.view, particle_display_frame);
#ifdef MELEE_NATIVE
    if (!nativeRenderGraphicsEnabled) {
        ++nativeRenderParticleBatchesSkipped;
        return;
    }
#endif
    {
        f32 pos_x;
        f32 pos_y;

        C_MTXCopy((MtxPtr) &pp->appsrt->ssx, draw_mtx);
        cur_x = draw_mtx[0][3] + (draw_mtx[0][2] * pp->pos.z +
                                  (draw_mtx[0][0] * (pos_x = pp->pos.x) +
                                   draw_mtx[0][1] * (pos_y = pp->pos.y)));
        cur_y = draw_mtx[1][3] +
                (draw_mtx[1][2] * pp->pos.z +
                 (draw_mtx[1][0] * pos_x + draw_mtx[1][1] * pos_y));
        cur_z = draw_mtx[2][3] +
                (draw_mtx[2][2] * pp->pos.z +
                 (draw_mtx[2][0] * pos_x + draw_mtx[2][1] * pos_y));
        if (pp->kind & Tornado) {
            f32 x;
            f32 y;
            f32 z;

            calcTornadoLastPos(pp, &x, &y, &z);
            prev_pos_x =
                draw_mtx[0][3] + (draw_mtx[0][2] * z +
                                  (draw_mtx[0][0] * x + draw_mtx[0][1] * y));
            prev_pos_y =
                draw_mtx[1][3] + (draw_mtx[1][2] * z +
                                  (draw_mtx[1][0] * x + draw_mtx[1][1] * y));
            prev_pos_z =
                draw_mtx[2][3] + (draw_mtx[2][2] * z +
                                  (draw_mtx[2][0] * x + draw_mtx[2][1] * y));
        } else {
            f32 dz = pp->pos.z - pp->vel.z;
            f32 dx = pp->pos.x - pp->vel.x;
            f32 dy = pp->pos.y - pp->vel.y;

            prev_pos_x =
                draw_mtx[0][3] + (draw_mtx[0][2] * dz +
                                  (draw_mtx[0][0] * dx + draw_mtx[0][1] * dy));
            prev_pos_y =
                draw_mtx[1][3] + (draw_mtx[1][2] * dz +
                                  (draw_mtx[1][0] * dx + draw_mtx[1][1] * dy));
            prev_pos_z =
                draw_mtx[2][3] + (draw_mtx[2][2] * dz +
                                  (draw_mtx[2][0] * dx + draw_mtx[2][1] * dy));
        }
        psScaleAppSRTAxes(pp, draw_mtx);
        (void) &pad;
    }
    x_extent = pp->appsrt->x94 * pp->size;
    y_extent = pp->appsrt->x98 * pp->size;
    if (it == NULL) {
        ay = y_extent;
        by = -ay;
        ax = bx = x_extent;
    } else {
        ay = 0.0f;
        ax = x_extent;
        by = -y_extent;
        bx = ay;
    }
    if ((pp->kind & Trail) || (pp->kind & DirVec)) {
        f32 vf1;
        f32 vf2;
        /* The retail frame reserves twelve float slots here, matching one
         * unused copy of each AppSRT matrix element. */
        UNUSED f32 m00 = pp->appsrt->ssx;
        UNUSED f32 m01 = pp->appsrt->ssy;
        UNUSED f32 m02 = pp->appsrt->x6C;
        UNUSED f32 m03 = pp->appsrt->x70;
        UNUSED f32 m10 = pp->appsrt->x74;
        UNUSED f32 m11 = pp->appsrt->x78;
        UNUSED f32 m12 = pp->appsrt->x7C;
        UNUSED f32 m13 = pp->appsrt->x80;
        UNUSED f32 m20 = pp->appsrt->x84;
        UNUSED f32 m21 = pp->appsrt->x88;
        UNUSED f32 m22 = pp->appsrt->x8C;
        UNUSED f32 m23 = pp->appsrt->x90;
        if (0.0f == particleDraw.projection[0]) {
            f32 prev_x;
            f32 prev_y;
            f32 prev_z;

            if (pp->kind & Tornado) {
                calcTornadoLastPos(pp, &prev_x, &prev_y, &prev_z);
            } else {
                prev_x = pp->pos.x - pp->vel.x;
                prev_y = pp->pos.y - pp->vel.y;
                prev_z = pp->pos.z - pp->vel.z;
            }
            w0 = pp->appsrt->x90 +
                 (pp->appsrt->x8C * pp->pos.z +
                  (pp->appsrt->x84 * pp->pos.x + pp->appsrt->x88 * pp->pos.y));
            s808 = particleDraw.projection[1] * pp->appsrt->ssx + particleDraw.projection[2] * pp->appsrt->x84;
            s804 = particleDraw.projection[1] * pp->appsrt->ssy + particleDraw.projection[2] * pp->appsrt->x88;
            f16 = particleDraw.projection[1] * pp->appsrt->x6C + particleDraw.projection[2] * pp->appsrt->x8C;
            f20 = particleDraw.projection[1] * pp->appsrt->x70 + particleDraw.projection[2] * pp->appsrt->x90;
            f12 = particleDraw.projection[3] * pp->appsrt->x74 + particleDraw.projection[4] * pp->appsrt->x84;
            f8 = particleDraw.projection[3] * pp->appsrt->x78 + particleDraw.projection[4] * pp->appsrt->x88;
            f11 = particleDraw.projection[3] * pp->appsrt->x7C + particleDraw.projection[4] * pp->appsrt->x8C;
            f13 = particleDraw.projection[3] * pp->appsrt->x80 + particleDraw.projection[4] * pp->appsrt->x90;
            if (0.0f == w0) {
                return;
            }
            w0inv = -1.0f / w0;
            w1 = pp->appsrt->x90 +
                 (pp->appsrt->x8C * prev_z +
                  (pp->appsrt->x84 * prev_x + pp->appsrt->x88 * prev_y));
            if (0.0f == w1) {
                return;
            }
            w1inv = -1.0f / w1;
            vf1 = w0inv * (f20 + (f16 * pp->pos.z +
                                  (s808 * pp->pos.x + s804 * pp->pos.y))) -
                  w1inv *
                      (f20 + (f16 * prev_z + (s808 * prev_x + s804 * prev_y)));
            vf2 =
                w0inv * (f13 + (f11 * pp->pos.z +
                                (f12 * pp->pos.x + f8 * pp->pos.y))) -
                w1inv * (f13 + (f11 * prev_z + (f12 * prev_x + f8 * prev_y)));
        } else {
            s800 = particleDraw.projection[1] * pp->appsrt->ssx + particleDraw.projection[2];
            s7FC = particleDraw.projection[1] * pp->appsrt->ssy + particleDraw.projection[2];
            s7F8 = particleDraw.projection[1] * pp->appsrt->x6C + particleDraw.projection[2];
            f17 = particleDraw.projection[3] * pp->appsrt->x74 + particleDraw.projection[4];
            f18 = particleDraw.projection[3] * pp->appsrt->x78 + particleDraw.projection[4];
            f20b = particleDraw.projection[3] * pp->appsrt->x7C + particleDraw.projection[4];
            if (pp->kind & Tornado) {
                f32 tx;
                f32 ty;
                f32 tz;
                calcTornadoLastPos(pp, &tx, &ty, &tz);
                {
                    f32 dx;
                    f32 dy;
                    f32 dz;

                    dy = pp->pos.y - ty;
                    dx = pp->pos.x - tx;
                    dz = pp->pos.z - tz;
                    vf1 = s7F8 * dz + (s800 * dx + s7FC * dy);
                    vf2 = f20b * dz + (f17 * dx + f18 * dy);
                }
            } else {
                f32 vz;
                f32 vx;
                f32 vy;

                vy = pp->vel.y;
                vx = pp->vel.x;
                vz = pp->vel.z;
                vf1 = s7F8 * vz + (s800 * vx + s7FC * vy);
                vf2 = f20b * vz + (f17 * vx + f18 * vy);
            }
        }
        if (psMaskAbsLtF32(vf2, FLT_MIN)) {
            angle = (-vf1 >= 0.0f) ? 1.5707964f : -1.5707964f;
        } else {
            angle = atan2f(-vf1, vf2);
        }
        if (pp->kind & DirVec) {
            angle += pp->rotate;
        }
    } else {
        angle = pp->rotate;
    }
    if (psMaskAbsGtF32(angle, 0.01)) {
        f32 c = cosf(angle);
        f32 s = sinf(angle);
        f32 old_x = ax;
        ax = c * ax - s * ay;
        ay = s * old_x + c * ay;
        old_x = bx;
        bx = c * bx - s * by;
        by = s * old_x + c * by;
    }
    psSetParticleDrawMtx(GX_PNMTX1);
    // Emit scalar parameters in the original FIFO order.
    if (pp->kind & Trail) {
        f32 xl;
        f32 yl;
        f32 zl;
        GXColor draw_color;

        psGetTrailColor(pp, &draw_color);
        if (it == NULL) {
            if (pp->kind & DispTexture) {
                setVtxDesc(2);
                GXBegin(GX_QUADS, GX_VTXFMT2, 4U);
            } else {
                setVtxDesc(3);
                GXBegin(GX_QUADS, GX_VTXFMT3, 4U);
            }
            GXPosition3f32(-ax + prev_pos_x, -ay + prev_pos_y, prev_pos_z);
            GXColor4u8(draw_color.r, draw_color.g, draw_color.b,
                       (u8) ((f32) draw_color.a * pp->trail));
            if (pp->kind & DispTexture) {
                GXTexCoord1x8((pp->kind >> 16) & 0xC);
            }
            GXPosition3f32(-bx + cur_x, -by + cur_y, cur_z);
            GXColor4u8(draw_color.r, draw_color.g, draw_color.b, draw_color.a);
            if (pp->kind & DispTexture) {
                GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 1);
            }
            GXPosition3f32(ax + cur_x, ay + cur_y, cur_z);
            GXColor4u8(draw_color.r, draw_color.g, draw_color.b, draw_color.a);
            if (pp->kind & DispTexture) {
                GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 2);
            }
            GXPosition3f32(bx + prev_pos_x, by + prev_pos_y, prev_pos_z);
            GXColor4u8(draw_color.r, draw_color.g, draw_color.b,
                       (u8) ((f32) draw_color.a * pp->trail));
            if (pp->kind & DispTexture) {
                GXTexCoord1x8(((pp->kind >> 16) & 0xC) + 3);
            }
        } else {
            f32 trail_alpha = 255.0f * (1.0f - pp->trail);
            f32 axis_len = sqrtf(bx * bx + by * by);

            if (axis_len != 0.0f) {
                f32 dx = cur_x - prev_pos_x;
                f32 dy = cur_y - prev_pos_y;
                f32 dz = cur_z - prev_pos_z;
                u32 primitive_count;

                xl = dx * dx;
                yl = dy * dy;
                zl = dz * dz;
                axis_len = sqrtf(zl + (xl + yl)) / axis_len;
                primitive_count = *(u32*) it;

                bx *= axis_len;
                by *= axis_len;
                it += sizeof(u32);
                for (; primitive_count != 0; primitive_count--) {
                    GXPrimitive primitive = it[0];
                    u8 count = it[1];
                    s32 i;

                    it += 4;
                    if (pp->kind & DispTexture) {
                        setVtxDesc(5);
                        GXBegin(primitive, GX_VTXFMT5, count);
                    } else {
                        setVtxDesc(3);
                        GXBegin(primitive, GX_VTXFMT3, count);
                    }
                    for (i = count; i > 0; i--) {
                        f32 s = *(f32*) &it[0];
                        f32 sx = 2.0f * (s - 0.5f);
                        f32 t;
                        f32 tx;
                        s32 converted_alpha;
                        s32 alpha;

                        if (pp->kind & TexFlipS) {
                            s = 1.0f - s;
                        }
                        t = *(f32*) &it[4];
                        it += 8;
                        alpha = (s32) (255.0f - t * trail_alpha);
                        converted_alpha = (s32) (255.0f - t * trail_alpha);
                        if (converted_alpha < 0) {
                            alpha = 0;
                        }
                        if (alpha > 0xFF) {
                            alpha = 0xFF;
                        }
                        tx = 2.0f * (t - 0.5f);
                        if (pp->kind & TexFlipT) {
                            t = 1.0f - t;
                        }
                        GXPosition3f32(cur_x + ax * sx + bx * tx,
                                       cur_y + ay * sx + by * tx, cur_z);
                        GXColor4u8(draw_color.r, draw_color.g, draw_color.b,
                                   (u8) alpha);
                        if (pp->kind & DispTexture) {
                            GXParam1f32(s);
                            GXParam1f32(t);
                        }
                    }
                }
            }
        }
    } else if (it == NULL) {
        if (pp->kind & DispTexture) {
            setVtxDesc(0);
            GXBegin(GX_QUADS, GX_VTXFMT0, 4U);
        } else {
            setVtxDesc(1);
            GXBegin(GX_QUADS, GX_VTXFMT1, 4U);
        }
        GXParam1f32(-ax + cur_x);
        GXParam1f32(-ay + cur_y);
        GXParam1f32(cur_z);
        if (pp->kind & DispTexture) {
            GXParam1u8((pp->kind >> 16) & 0xC);
        }
        GXParam1f32(-bx + cur_x);
        GXParam1f32(-by + cur_y);
        GXParam1f32(cur_z);
        if (pp->kind & DispTexture) {
            GXParam1u8(((pp->kind >> 16) & 0xC) + 1);
        }
        GXParam1f32(ax + cur_x);
        GXParam1f32(ay + cur_y);
        GXParam1f32(cur_z);
        if (pp->kind & DispTexture) {
            GXParam1u8(((pp->kind >> 16) & 0xC) + 2);
        }
        GXParam1f32(bx + cur_x);
        GXParam1f32(by + cur_y);
        GXParam1f32(cur_z);
        if (pp->kind & DispTexture) {
            GXParam1u8(((pp->kind >> 16) & 0xC) + 3);
        }
    } else {
        u32 primitive_count = *(u32*) it;

        it += sizeof(u32);
        for (; primitive_count != 0; primitive_count--) {
            GXPrimitive prim = it[0];
            u8 count = it[1];
            s32 i;
            it += 4;
            if (pp->kind & DispTexture) {
                setVtxDesc(4);
                GXBegin(prim, GX_VTXFMT4, count);
            } else {
                setVtxDesc(1);
                GXBegin(prim, GX_VTXFMT1, count);
            }
            for (i = count; i > 0; i--) {
                f32 s = *(f32*) &it[0];
                f32 sx = 2.0f * (s - 0.5f);
                f32 t;
                f32 tx;

                if (pp->kind & TexFlipS) {
                    s = 1.0f - s;
                }
                t = *(f32*) &it[4];
                it += 8;
                tx = 2.0f * (t - 0.5f);
                if (pp->kind & TexFlipT) {
                    t = 1.0f - t;
                }
                GXPosition3f32(cur_x + ax * sx + bx * tx,
                               cur_y + ay * sx + by * tx, cur_z);
                if (pp->kind & DispTexture) {
                    GXParam1f32(s);
                    GXParam1f32(t);
                }
            }
        }
    }
}
