#include "psdisp_internal.h"
#include "mtx.h"

void psPrepareParticleAppSRT(HSD_psAppSRT* transform, Mtx view, u8 frame)
{
    if (transform->frameNum == frame) {
        return;
    }
    if (transform->status != PS_APPSTATUS_STILL) {
        HSD_MtxSRT(transform->mmtx, &transform->scale,
                   (Vec3*) &transform->rot, &transform->translate, NULL);
    }
    if (transform->status == PS_APPSTATUS_ONCE) {
        transform->status = PS_APPSTATUS_STILL;
    }
    C_MTXConcat(view, transform->mmtx, (MtxPtr) &transform->ssx);
    f32 scale_x = transform->ssx * transform->ssx +
                  transform->x74 * transform->x74 +
                  transform->x84 * transform->x84;
    transform->x94 = sqrtf(scale_x);
    f32 scale_y = transform->ssy * transform->ssy +
                  transform->x78 * transform->x78 +
                  transform->x88 * transform->x88;
    transform->x98 = sqrtf(scale_y);
    if (transform->xA2 != 0) {
        Mtx translated_view;
        Vec3 scale;
        C_MTXIdentity(translated_view);
        translated_view[0][3] = transform->translate.x;
        translated_view[1][3] = transform->translate.y;
        translated_view[2][3] = transform->translate.z;
        C_MTXConcat(view, translated_view, translated_view);
        HSD_MtxGetScale(translated_view, &scale);
        C_MTXScale((MtxPtr) &transform->ssx, scale.x, scale.y, scale.z);
        transform->x70 = translated_view[0][3];
        transform->x80 = translated_view[1][3];
        transform->x90 = translated_view[2][3];
    }
    transform->frameNum = frame;
}
