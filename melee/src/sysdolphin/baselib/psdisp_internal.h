#ifndef HSD_PSDISP_INTERNAL_H
#define HSD_PSDISP_INTERNAL_H
#include "psdisp.h"
#include "psstructs.h"
#include <dolphin/gx.h>
#include <dolphin/mtx.h>

// Rebuilt by each nonempty display pass. These are drawing caches, not match
// state; attachment matrices and particle list order remain on their owners.
typedef struct ParticleDrawState {
    Mtx view, inverse_view, projected_view;
    f32 projection[GX_PROJECTION_SZ];
    Vec3 right, up;
    s32 point_size, line_width, blend_mode, channel_mode;
    GXColor channel_material, channel_ambient;
    GXColor primary_color, environment_color, material_color;
    s32 current_matrix;
} ParticleDrawState;
extern ParticleDrawState particleDraw;

void psGetTrailColor(HSD_Particle*, GXColor*);
void psSetParticleDrawMtx(GXPosNrmMtx);
HSD_Particle* psDispSubPoint(HSD_Particle*);
HSD_Particle* psDispSubPointTrail(HSD_Particle*);
void psDispSub(HSD_Particle*, u8*);
void psDispSubAPPSRTPoint(HSD_Particle*);
void psDispSubAppSRT(HSD_Particle*, u8*);
void psPrepareParticleAppSRT(HSD_psAppSRT*, Mtx view, u8 frame);
#endif
