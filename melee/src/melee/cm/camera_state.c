#include "camera.h"
#include "camera_state_internal.h"
#include <math.h>
#include <placeholder.h>
#include <melee/gr/grlib.h>
#include <melee/lb/lbvector.h>
#include <sysdolphin/baselib/debug.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjgxlink.h>
#include <sysdolphin/baselib/wobj.h>

// Actual mutable camera values. Graph payloads stay with their HSD owners.
Camera game_camera;
CameraDebugMode cm_80453004;
HSD_CObj* cm_804D6464;
/* 3BCCA0 */ CameraUnkGlobals cm_803BCCA0 = {
    83.0f,  1000.0f, -30.0f,  5.0f,  -7.0f,  17.5f,  -17.5f, 0.0f,  0.0682f,
    60.0f,  120.0f,  0.05f,   0.1f,  120.0f, 900.0f, 0.15f,  38.0f, 0.1f,
    0.1f,   0.001f,  0.1f,    1.0f,  1.0f,   0.6f,   0.6f,   0.05f, 0.1f,
    29.0f,  0.1f,    0.1f,    0.1f,  0.1f,   0.5f,   0.5f,   0.4f,  -11.0f,
    400.0f, 0.2f,    4.0f,    0.05f, 1.0f,   -7.0f,  7.0f,   0.5f,  0.5f,
    0.004f, 0.2f,    0.025f,  0.2f,  0.003f, 0.2f,   0.025f, 0.2f,  0.02f,
    1.0f,   0.14f,   1200.0f, -0.2f, 1.2f,   0.0f,
};

#ifdef MELEE_NATIVE
const Camera* Camera_GetState(void)
{
    return &game_camera;
}
#endif

void Camera_80030730(f32 arg8)
{
    cm_803BCCA0.x40 = arg8;
}

void Camera_SetBackgroundColor(u8 r, u8 g, u8 b)
{
    game_camera.background_color.r = r;
    game_camera.background_color.g = g;
    game_camera.background_color.b = b;
}

GXColor Camera_GetBackgroundColor(void)
{
    GXColor color;

    color.r = game_camera.background_color.r;
    color.g = game_camera.background_color.g;
    color.b = game_camera.background_color.b;
    return color;
}

void Camera_GetTransformPosition(Vec3* arg0)
{
    *arg0 = game_camera.transform.position;
}

void Camera_GetTransformInterest(Vec* arg0)
{
    *arg0 = game_camera.transform.interest;
}

static inline void project_ground_x(f32* value, Vec3* eye_pos, f32 dir_x,
                                    f32 dir_z)
{
    *value = -((dir_x * (eye_pos->z / dir_z)) - eye_pos->x);
}

static inline bool same_side(f32 fwd_z, f32 dir_z)
{
    return (fwd_z * dir_z) > 0.0L;
}

bool Camera_800307D0(f32* left, f32* center, f32* right)
{
    HSD_CObj* cobj;
    f32 half_fov;
    Vec3 forward;
    Vec3 interest_pos;
    Vec3 eye_pos;
    bool result;
    f32 s;
    f32 c;
    f32 edge_x;
    f32 edge_z;
    f32 s2;
    f32 c2;
    f32 edge_x2;
    f32 edge_z2;

    cobj = GET_COBJ(game_camera.gobj);
    half_fov =
        0.5 * (MTXDegToRad(HSD_CObjGetFov(cobj)) * HSD_CObjGetAspect(cobj));

    result = true;
    HSD_CObjGetEyePosition(cobj, &eye_pos);
    HSD_CObjGetEyeVector(cobj, &forward);
    HSD_CObjGetInterest(cobj, &interest_pos);

    if (ABS(forward.x) > 1E-4L && ABS(forward.z) > 1E-4L) {
        // ray casts?
        forward.x *= -1.0f;
        forward.y *= -1.0f;
        forward.z *= -1.0f;
        project_ground_x(center, &eye_pos, forward.x, forward.z);

        /// @todo would make more sense if these two blocks were the inlines
        s = sinf(half_fov);
        c = cosf(half_fov);
        edge_x = (forward.x * c) + (forward.z * s);
        edge_z = (forward.z * c) - (forward.x * s);
        if ((ABS(edge_z) > 0.0001L) && same_side(forward.z, edge_z)) {
            project_ground_x(left, &eye_pos, edge_x, edge_z);
        } else if (edge_x > 0.0f) {
            *left = 8.5070587e37f;
        } else {
            *left = -8.5070587e37f;
        }

        s2 = sinf(-half_fov);
        c2 = cosf(-half_fov);
        edge_x2 = (forward.x * c2) + (forward.z * s2);
        edge_z2 = (forward.z * c2) - (forward.x * s2);
        if ((ABS(edge_z2) > 0.0001L) && same_side(forward.z, edge_z2)) {
            project_ground_x(right, &eye_pos, edge_x2, edge_z2);
        } else if (edge_x2 > 0.0f) {
            *right = 8.5070587e37f;
        } else {
            *right = -8.5070587e37f;
        }
    } else {
        *left = -8.5070587e37f;
        *right = 8.5070587e37f;
        *center = 0.0f;
        result = false;
    }
    return result;
}

HSD_GObj* Camera_80030A50(void)
{
    return game_camera.gobj;
}

void Camera_80030A60(bool arg0)
{
    game_camera.x399_b3 = arg0;
}

bool Camera_80030A78(void)
{
    return game_camera.x399_b3;
}

void Camera_80030A8C(bool arg0)
{
    game_camera.x399_b4 = arg0;
}

void Camera_SetStageVisible(int arg0)
{
    int temp;
    game_camera.x399_b5 = (temp = 0) == arg0;
}

bool Camera_80030AC4(void)
{
    int temp;
    return (temp = 0) == game_camera.x399_b5;
}

void Camera_80030AE0(bool arg0)
{
    game_camera.x399_b2 = arg0;
}

bool Camera_80030AF8(void)
{
    return game_camera.x399_b2;
}

void Camera_80030B0C(bool arg0)
{
    game_camera.x399_b7 = arg0;
}

bool Camera_80030B24(void)
{
    return game_camera.x399_b7;
}

void Camera_80030B38(bool arg0)
{
    game_camera.x39A_b0 = arg0;
}

bool Camera_80030B50(void)
{
    return game_camera.x39A_b0;
}

void Camera_80030B64(bool arg0)
{
    game_camera.x39A_b1 = arg0;
}

bool Camera_80030B7C(void)
{
    return game_camera.x39A_b1;
}

void Camera_80030B90(bool arg0)
{
    game_camera.x39A_b2 = arg0;
}

bool Camera_80030BA8(void)
{
    return game_camera.x39A_b2;
}

/// Camera_ToScreen
bool Camera_80030BBC(Vec3* arg0, S32Vec2* arg1)
{
    Vec3 point;
    HSD_CObj* cobj;
    Scissor scissor;
    s32 px;
    s32 py;
    PAD_STACK(4);

    cobj = GET_COBJ(game_camera.gobj);
    if (lbVector_WorldToScreen(cobj, arg0, &point, 1) == NULL) {
        return false;
    }

    if ((point.x > 2.1474836e9f) || (point.x < -2.1474836e9f) ||
        (point.y > 2.1474836e9f) || (point.y < -2.1474836e9f))
    {
        return false;
    }

    px = point.x;
    py = point.y;
    if (arg1 != NULL) {
        arg1->x = px;
        arg1->y = py;
    }
    HSD_CObjGetScissor(cobj, &scissor);
    if ((px < scissor.left) || (px >= scissor.right) || (py < scissor.top) ||
        (py >= scissor.bottom))
    {
        return false;
    }
    return true;
}

bool Camera_80030CD8(CmSubject* arg0, S32Vec2* arg1)
{
    return Camera_80030BBC(&arg0->bone_pos, arg1);
}

/// Camera_IsSubjectOnScreen
bool Camera_80030CFC(CmSubject* cam_box, f32 tolerance)
{
    HSD_CObj* cobj;
    Vec3 eye_pos;
    Vec3 interest;
    Vec3 sp38;
    u8 _PAD[12];
    Vec3 sp20;
    f32 range;
    PAD_STACK(2);

    cobj = GET_COBJ(game_camera.gobj);
    HSD_CObjGetEyePosition(cobj, &eye_pos);
    HSD_CObjGetInterest(cobj, &interest);
    if (lbVector_8000E838(&interest, &eye_pos, &cam_box->bone_pos, &sp38) <
        (cam_box->ext.v.z + tolerance))
    {
        return 1;
    }
    lbVector_Diff(&sp38, &cam_box->bone_pos, &sp20);
    lbVector_Normalize(&sp20);
    range = cam_box->ext.v.z + tolerance;
    sp20.x *= range;
    sp20.y *= range;
    sp20.z *= range;
    lbVector_Add(&sp20, &cam_box->bone_pos);
    return Camera_80030BBC(&sp20, NULL);
}

void Camera_80030DE4(f32 arg8, f32 arg9)
{
    game_camera.translation.x = arg8;
    game_camera.translation.y = arg9;
}

void Camera_80030DF8(void)
{
    game_camera.translation.x = game_camera.translation.y = 0;
}

float Camera_80030E10(void)
{
    if (game_camera.x2B8 < 1) {
        return 10000.f;
    }
    return game_camera.x2B0;
}

void Camera_SetQuakeScale(f32 scale)
{
    game_camera.quake_scale = scale;
}

void Camera_RequestQuake(CmQuakeKind kind, Vec3* pos)
{
    HSD_GObj** pquake;
    s32 quake_length;

    switch (kind) {
    case QuakeKind_Loop:
        pquake = &game_camera.quake_gobj;
        if (game_camera.quake_gobj == NULL) {
            *pquake = grLib_801C9CEC(kind);
        }
        quake_length = 10;
        break;
    case QuakeKind_Small:
        grLib_801C9CEC(kind);
        quake_length = 22;
        break;
    case QuakeKind_Medium:
        grLib_801C9CEC(kind);
        quake_length = 22;
        break;
    case QuakeKind_Large:
        grLib_801C9CEC(kind);
        quake_length = 22;
        break;
    default:
        break;
    }

    game_camera.quake_frames_left[kind] = quake_length;

    {
        // Log this frames request for Camera_UpdateQuakes to snapshot. This is
        // dead bookkeeping as nothing consumes the snapshot row.
        s32 i;
        for (i = 0; i < 16; i++) {
            if (game_camera.quakes[0][i].kind == QuakeKind_None) {
                game_camera.quakes[0][i].kind = kind;
                if (pos != NULL) {
                    game_camera.quakes[0][i].epicenter = *pos;
                } else {
                    game_camera.quakes[0][i].epicenter.z = 0.0f;
                    game_camera.quakes[0][i].epicenter.y = 0.0f;
                    game_camera.quakes[0][i].epicenter.x = 0.0f;
                }
                // No break here, so all slots are filled.
            }
        }
    }
}

void Camera_StopQuake(CmQuakeKind kind)
{
    game_camera.quake_frames_left[kind] = 0;
}

enum_t Camera_80031060(void)
{
    return game_camera.x398_b6_b7;
}

void Camera_80031074(u8 arg0)
{
    game_camera.x398_b6_b7 = arg0;
}

enum_t Camera_8003108C(void)
{
    return game_camera.x399_b0_b1;
}

void Camera_800310A0(u8 arg0)
{
    game_camera.x399_b0_b1 = arg0;
}

HSD_CObj* Camera_800310B8(void)
{
    HSD_CObjSetMtxDirty(cm_804D6464);
    HSD_CObjSetupViewingMtx(cm_804D6464);
    return cm_804D6464;
}

void Camera_800310E8(void)
{
    game_camera.x398_b0 = false;
    game_camera.x398_b1 = false;
    game_camera.x398_b2 = false;
    game_camera.x398_b3 = false;
    game_camera.x398_b4 = false;
    game_camera.x398_b5 = false;
}

f32 Camera_80031144(void)
{
    return game_camera.x2BC;
}

bool Camera_80031154(Vec3* arg0)
{
    if (Camera_80029124(arg0, 0) == CAM_BOUNDS_INSIDE) {
        return true;
    }
    return false;
}

bool Camera_8003118C(Vec3* arg0, float arg1)
{
    if (Camera_80029124(arg0, arg1) == CAM_BOUNDS_INSIDE) {
        return true;
    }
    return false;
}

void Camera_800311CC(f32 arg8)
{
    game_camera.farz = arg8;
}

void Camera_800311DC(f32 arg8)
{
    game_camera.nearz = arg8;
}

static inline s64 inline_cam_gx_b0(void)
{
    if (game_camera.x398_b0) {
        return 0;
    }
    return 0x20;
}

static inline s64 inline_cam_gx_b1(void)
{
    if (game_camera.x398_b1) {
        return 0;
    }
    return 0x40;
}

static inline s64 inline_cam_gx_b4(void)
{
    if (game_camera.x398_b4) {
        return 0;
    }
    return 0x10;
}

void Camera_800311EC(HSD_GObj* gobj, u64 prios)
{
    s64 a;
    s64 b;

    game_camera.x398_b6_b7 = 1;

    a = inline_cam_gx_b1();
    b = inline_cam_gx_b0();
    b = prios | b;
    gobj->gxlink_prios = b | a;
    HSD_GObj_80390ED0(gobj, 3);

    game_camera.x398_b6_b7 = 0;

    a = inline_cam_gx_b0();
    b = inline_cam_gx_b4();
    prios = inline_cam_gx_b1();
    gobj->gxlink_prios = prios | (b | a);
    HSD_GObj_80390ED0(gobj, 3);
}

void Camera_80031328(HSD_GObj* gobj, u64 prios)
{
    s64 a;
    s64 b;
    u64 c;

    game_camera.x398_b6_b7 = 0;

    a = inline_cam_gx_b0();
    b = inline_cam_gx_b4();
    c = inline_cam_gx_b1();
    b = prios | b;
    gobj->gxlink_prios = (b | a) | c;
    HSD_GObj_80390ED0(gobj, 4);
}

void Camera_800313E0(HSD_GObj* gobj, u64 prios)
{
    s64 a;
    s64 b;
    u64 c;

    game_camera.x398_b6_b7 = 1;

    a = inline_cam_gx_b1();
    b = inline_cam_gx_b0();
    b = prios | b;
    gobj->gxlink_prios = b | a;
    HSD_GObj_80390ED0(gobj, 3);

    game_camera.x398_b6_b7 = 0;

    a = inline_cam_gx_b0();
    b = inline_cam_gx_b4();
    c = inline_cam_gx_b1();
    gobj->gxlink_prios = c | (b | a);
    HSD_GObj_80390ED0(gobj, 3);

    game_camera.x398_b6_b7 = 0;

    a = inline_cam_gx_b0();
    b = inline_cam_gx_b4();
    c = inline_cam_gx_b1();
    b = prios | b;
    gobj->gxlink_prios = (b | a) | c;
    HSD_GObj_80390ED0(gobj, 4);
}
