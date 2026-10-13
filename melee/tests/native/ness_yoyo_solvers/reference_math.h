// Mechanically extracted unchanged Dolphin C math bodies. See math_original_inputs.json.
void C_MTXIdentity(Mtx m)
{
    ASSERTMSGLINE(0xA1, m, "MtxIdentity():  NULL Mtx 'm' ");
    m[0][0] = 1;
    m[0][1] = 0;
    m[0][2] = 0;
    m[0][3] = 0;
    m[1][0] = 0;
    m[1][1] = 1;
    m[1][2] = 0;
    m[1][3] = 0;
    m[2][0] = 0;
    m[2][1] = 0;
    m[2][2] = 1;
    m[2][3] = 0;
}

void C_MTXCopy(Mtx src, Mtx dst)
{
    ASSERTMSGLINE(0xDE, src, "MTXCopy():  NULL MtxPtr 'src' ");
    ASSERTMSGLINE(0xDF, dst, "MTXCopy():  NULL MtxPtr 'dst' ");
    if (src != dst) {
        dst[0][0] = src[0][0];
        dst[0][1] = src[0][1];
        dst[0][2] = src[0][2];
        dst[0][3] = src[0][3];
        dst[1][0] = src[1][0];
        dst[1][1] = src[1][1];
        dst[1][2] = src[1][2];
        dst[1][3] = src[1][3];
        dst[2][0] = src[2][0];
        dst[2][1] = src[2][1];
        dst[2][2] = src[2][2];
        dst[2][3] = src[2][3];
    }
}

void C_MTXConcat(Mtx a, Mtx b, Mtx ab)
{
    Mtx mTmp;
    f32(*m)[4];

    ASSERTMSGLINE(0x128, a, "MTXConcat():  NULL MtxPtr 'a'  ");
    ASSERTMSGLINE(0x129, b, "MTXConcat():  NULL MtxPtr 'b'  ");
    ASSERTMSGLINE(0x12A, ab, "MTXConcat():  NULL MtxPtr 'ab' ");

    if (ab == a || ab == b) {
        m = mTmp;
    } else {
        m = ab;
    }

    m[0][0] =
        0 + a[0][2] * b[2][0] + ((a[0][0] * b[0][0]) + (a[0][1] * b[1][0]));
    m[0][1] =
        0 + a[0][2] * b[2][1] + ((a[0][0] * b[0][1]) + (a[0][1] * b[1][1]));
    m[0][2] =
        0 + a[0][2] * b[2][2] + ((a[0][0] * b[0][2]) + (a[0][1] * b[1][2]));
    m[0][3] = a[0][3] +
              (a[0][2] * b[2][3] + (a[0][0] * b[0][3] + (a[0][1] * b[1][3])));

    m[1][0] =
        0 + a[1][2] * b[2][0] + ((a[1][0] * b[0][0]) + (a[1][1] * b[1][0]));
    m[1][1] =
        0 + a[1][2] * b[2][1] + ((a[1][0] * b[0][1]) + (a[1][1] * b[1][1]));
    m[1][2] =
        0 + a[1][2] * b[2][2] + ((a[1][0] * b[0][2]) + (a[1][1] * b[1][2]));
    m[1][3] = a[1][3] +
              (a[1][2] * b[2][3] + (a[1][0] * b[0][3] + (a[1][1] * b[1][3])));

    m[2][0] =
        0 + a[2][2] * b[2][0] + ((a[2][0] * b[0][0]) + (a[2][1] * b[1][0]));
    m[2][1] =
        0 + a[2][2] * b[2][1] + ((a[2][0] * b[0][1]) + (a[2][1] * b[1][1]));
    m[2][2] =
        0 + a[2][2] * b[2][2] + ((a[2][0] * b[0][2]) + (a[2][1] * b[1][2]));
    m[2][3] = a[2][3] +
              (a[2][2] * b[2][3] + (a[2][0] * b[0][3] + (a[2][1] * b[1][3])));

    if (m == mTmp) {
        C_MTXCopy(mTmp, ab);
    }
}

void C_MTXMultVec(Mtx44 m, Vec* src, Vec* dst)
{
    Vec vTmp;

    ASSERTMSGLINE(0x39, m, "MTXMultVec():  NULL MtxPtr 'm' ");
    ASSERTMSGLINE(0x3A, src, "MTXMultVec():  NULL MtxPtr 'src' ");
    ASSERTMSGLINE(0x3B, dst, "MTXMultVec():  NULL MtxPtr 'dst' ");

    vTmp.x = m[0][3] +
             ((m[0][2] * src->z) + ((m[0][0] * src->x) + (m[0][1] * src->y)));
    vTmp.y = m[1][3] +
             ((m[1][2] * src->z) + ((m[1][0] * src->x) + (m[1][1] * src->y)));
    vTmp.z = m[2][3] +
             ((m[2][2] * src->z) + ((m[2][0] * src->x) + (m[2][1] * src->y)));
    dst->x = vTmp.x;
    dst->y = vTmp.y;
    dst->z = vTmp.z;
}

void C_VECCrossProduct(Vec* a, Vec* b, Vec* axb)
{
    Vec vTmp;

    ASSERTMSGLINE(0x20F, a, "VECCrossProduct():  NULL VecPtr 'a' ");
    ASSERTMSGLINE(0x210, b, "VECCrossProduct():  NULL VecPtr 'b' ");
    ASSERTMSGLINE(0x211, axb, "VECCrossProduct():  NULL VecPtr 'axb' ");

    vTmp.x = (a->y * b->z) - (a->z * b->y);
    vTmp.y = (a->z * b->x) - (a->x * b->z);
    vTmp.z = (a->x * b->y) - (a->y * b->x);
    axb->x = vTmp.x;
    axb->y = vTmp.y;
    axb->z = vTmp.z;
}

