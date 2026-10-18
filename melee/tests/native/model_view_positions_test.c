#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/pobj.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks;
static void check(bool condition)
{
    ++checks;
    if (!condition) { fprintf(stderr, "Model vertex query failed at check %u\n", checks); exit(1); }
}
static void near(float a, float b) { check(fabsf(a - b) < .00001f); }

int main(void)
{
    HSD_JObj owner = { 0 }, bone = { 0 };
    Mtx bind;
    C_MTXIdentity(owner.mtx); C_MTXIdentity(bone.mtx); C_MTXIdentity(bind);
    owner.flags = JOBJ_SKELETON_ROOT;
    bone.envelopemtx = bind;
    owner.mtx[0][3] = 20;
    bone.mtx[0][0] = 2; bone.mtx[1][1] = 3; bone.mtx[2][2] = 4;
    bone.mtx[0][3] = 10; bone.mtx[1][3] = 5; bone.mtx[2][3] = -2;
    bind[0][3] = -1;
    const float values[3][3] = { { 1, 2, 3 }, { -4, 5, 2 }, { 3, -2, 7 } };
    u8 data[sizeof(values)];
    for (unsigned i = 0; i < sizeof(values) / 4; ++i) {
        u32 bits; memcpy(&bits, (const u8*) values + i * 4, 4);
        data[i*4] = bits >> 24; data[i*4+1] = bits >> 16;
        data[i*4+2] = bits >> 8; data[i*4+3] = bits;
    }
    HSD_VtxDescList attrs[3] = { 0 };
    attrs[0].attr = GX_VA_PNMTXIDX; attrs[0].attr_type = GX_DIRECT;
    attrs[1].attr = GX_VA_POS; attrs[1].attr_type = GX_INDEX16;
    attrs[1].comp_cnt = GX_POS_XYZ; attrs[1].comp_type = GX_F32;
    attrs[1].vertex = data; attrs[1].vertex_size = sizeof(data); attrs[1].stride = 12;
    attrs[2].attr = GX_VA_NULL;
    u8 display[32] = { 0x90, 0, 3, 0, 0, 2, 0, 0, 0, 0, 0, 1 };
    HSD_Envelope env[2] = { { &env[1], &bone, .5f }, { NULL, &bone, .5f } };
    HSD_SList palette = { NULL, env };
    HSD_PObj polygon = { 0 };
    polygon.verts = attrs; polygon.flags = POBJ_ENVELOPE;
    polygon.n_display = 1; polygon.display = display; polygon.u.envelope_list = &palette;
    HSD_DObj mesh = { 0 }; mesh.pobj = &polygon;
    Vec3 output[3]; u8 present[3];
    nativeModelView_Begin(1, true);
    nativeModelView_Mesh(0, &owner, &mesh, false, false, &bone);
    check(nativeModelView_VertexPositions(0, output, present, 3));
    for (unsigned i = 0; i < 3; ++i) {
        check(present[i]); near(output[i].x, (values[i][0]-1)*2+10);
        near(output[i].y, values[i][1]*3+5); near(output[i].z, values[i][2]*4-2);
    }
    // A weight immediately below 1 still takes HSD's inverse-bind path.
    env[0].weight = nextafterf(1, 0); env[0].next = NULL;
    check(nativeModelView_VertexPositions(0, output, present, 3));
    near(output[0].x, 10*env[0].weight);
    env[0].weight = 1;
    check(nativeModelView_VertexPositions(0, output, present, 3));
    near(output[0].x, 12); // Exact 1 bypasses the inverse bind on a root owner.
    check(!nativeModelView_VertexPositions(0, output, present, 2));
    nativeModelView_Mesh(0, &owner, &mesh, true, false, &bone);
    check(!nativeModelView_VertexPositions(0, output, present, 3));
    nativeModelView_Mesh(0, &owner, &mesh, false, false, NULL);
    polygon.flags = POBJ_SHAPEANIM;
    check(!nativeModelView_VertexPositions(0, output, present, 3));
    nativeModelView_Clear();
    check(!nativeModelView_VertexPositions(0, output, present, 3));
    printf("PASS: %u posed vertex, inverse-bind, index and visibility checks\n", checks);
    return 0;
}
