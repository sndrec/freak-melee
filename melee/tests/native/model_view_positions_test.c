#include <sysdolphin/baselib/native_model_view.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/list.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/shape.h>

#include <math.h>
#include <float.h>
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

static float source_value(unsigned format,unsigned fraction,unsigned index,unsigned component)
{
    int value=(index*13+component*7)%31+1;
    if((format==GX_S8 || format==GX_S16) && (index+component)%2)value=-value;
    return format==GX_F32?value*.125f:ldexpf((float)value,-(int)fraction);
}
static void shape_formats(void)
{
    for(unsigned format=GX_U8;format<=GX_F32;++format)
    for(unsigned little=0;little<2;++little)
    for(unsigned wide=0;wide<2;++wide)
    for(unsigned nbt=0;nbt<2;++nbt)
    for(unsigned f=0;f<3;++f) {
        const unsigned fraction=f==2?31:f*3,width=format<2?1:format<4?2:4,stride=9*width+4;
        u8 data[264*40]={0},index_data[4][6]={0};u8* indices[4];unsigned logical[4][3];
        for(unsigned i=0;i<264;++i)for(unsigned c=0;c<9;++c) {
            float value=source_value(format,fraction,i,c);u32 bits;
            if(format==GX_F32)memcpy(&bits,&value,4);
            else bits=(u32)(s32)ldexpf(value,(int)fraction);
            for(unsigned b=0;b<width;++b)data[i*stride+c*width+b]=(u8)(bits>>((little?b:width-b-1)*8));
        }
        for(unsigned target=0;target<4;++target) {
            indices[target]=index_data[target];
            for(unsigned i=0;i<3;++i) {
                const unsigned index=(wide?257:2)+(target+i)%5;logical[target][i]=index;
                if(wide){index_data[target][i*2]=(u8)(index>>8);index_data[target][i*2+1]=(u8)index;}
                else index_data[target][i]=(u8)index;
            }
        }
        HSD_VtxDescList position={0};position.attr=GX_VA_POS;position.attr_type=wide?GX_INDEX16:GX_INDEX8;
        position.comp_cnt=GX_POS_XYZ;position.comp_type=format;position.frac=(u8)fraction;position.stride=(u16)stride;
        position.vertex=data;position.vertex_little_endian=little;position.vertex_size=sizeof(data);
        HSD_VtxDescList normal=position;normal.attr=nbt?GX_VA_NBT:GX_VA_NRM;normal.comp_cnt=GX_NRM_XYZ;
        HSD_ShapeSet shape={0};shape.flags=SHAPESET_AVERAGE;shape.nb_shape=3;shape.nb_vertex_index=3;
        shape.vertex_desc=&position;shape.vertex_idx_list=indices;shape.nb_normal_index=3;
        shape.normal_desc=&normal;shape.normal_idx_list=indices;
        const float blends[]={-5,.5f,1.5f,4,FLT_MAX};
        for(unsigned mode=0;mode<6;++mode) {
            float weights[]={-.5f,.25f,1.125f};
            if(mode==5){shape.flags=SHAPESET_ADDITIVE;shape.blend.bp=weights;}
            else shape.blend.bl=blends[mode];
            const HSD_ShapeSet before=shape;
            float positions[4][3],normals[10][3];
            for(unsigned i=0;i<4;++i)for(unsigned c=0;c<3;++c)positions[i][c]=1234;
            for(unsigned i=0;i<10;++i)for(unsigned c=0;c<3;++c)normals[i][c]=1234;
            check(HSD_ShapeSetEvaluate(&shape,positions,normals));check(!memcmp(&before,&shape,sizeof(shape)));
            for(unsigned attr=0;attr<2;++attr)for(unsigned i=0;i<3;++i)
            for(unsigned c=0;c<(attr&&nbt?9u:3u);++c) {
                float values[4];for(unsigned t=0;t<4;++t)values[t]=source_value(format,fraction,logical[t][i],c);
                const float expected=mode==0?values[0]:mode==1?.5f*values[0]+.5f*values[1]:
                    mode==2?.5f*values[1]+.5f*values[2]:mode<5?values[2]:values[0]+.25f*values[2]+1.125f*values[3];
                check((attr?normals[i*(nbt?3:1)+c/3][c%3]:positions[i][c])==expected);
            }
            check(positions[3][0]==1234 && normals[nbt?9:3][0]==1234);
            check(HSD_ShapeSetEvaluate(&shape,positions,NULL));
        }
    }
    HSD_ShapeSet invalid={0};float output[1][3]={{1234,1234,1234}};
    check(!HSD_ShapeSetEvaluate(NULL,output,NULL));
    check(!HSD_ShapeSetEvaluate(&invalid,output,NULL));check(output[0][0]==1234);
    invalid.flags=3;invalid.nb_shape=1;
    check(!HSD_ShapeSetEvaluate(&invalid,output,NULL));check(output[0][0]==1234);
}

int main(void)
{
    shape_formats();
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
    Vec3 output[3];
    const unsigned order[] = { 2, 0, 1 };
    nativeModelView_Begin(1, true);
    nativeModelView_Mesh(0, &owner, &mesh, false, false, &bone);
    check(nativeModelView_CornerPositions(0, output, 3));
    for (unsigned i = 0; i < 3; ++i) {
        near(output[i].x, (values[order[i]][0]-1)*2+10);
        near(output[i].y, values[order[i]][1]*3+5); near(output[i].z, values[order[i]][2]*4-2);
    }
    // A weight immediately below 1 still takes HSD's inverse-bind path.
    env[0].weight = nextafterf(1, 0); env[0].next = NULL;
    check(nativeModelView_CornerPositions(0, output, 3));
    near(output[1].x, 10*env[0].weight);
    env[0].weight = 1;
    check(nativeModelView_CornerPositions(0, output, 3));
    near(output[1].x, 12); // Exact 1 bypasses the inverse bind on a root owner.
    // Descriptor order is independent of GX's hardware fetch order.
    HSD_VtxDescList swap = attrs[0]; attrs[0] = attrs[1]; attrs[1] = swap;
    check(nativeModelView_CornerPositions(0, output, 3));
    near(output[1].x, 12);
    swap = attrs[0]; attrs[0] = attrs[1]; attrs[1] = swap;
    // One source index can have two different posed positions. Return both
    // corners rather than rejecting the entire stock mesh as ambiguous.
    HSD_Envelope other = { NULL, &owner, 1 };
    HSD_SList second = { NULL, &other }; palette.next = &second;
    u8 repeated[32] = { 0x90, 0, 3, 0, 0, 0, 3, 0, 0, 0, 0, 0 };
    polygon.display = repeated;
    check(nativeModelView_CornerPositions(0, output, 3));
    near(output[0].x, 12); near(output[1].x, 21); near(output[2].x, 12);
    palette.next = NULL; polygon.display = display;
    // Direct positions also retain the original corner correspondence.
    u8 direct[64] = { 0x90, 0, 3 };
    for (unsigned i = 0; i < 3; ++i) memcpy(direct + 4 + i * 13, data + i * 12, 12);
    polygon.display = direct; polygon.n_display = 2;
    attrs[1].attr_type = GX_DIRECT;
    check(nativeModelView_CornerPositions(0, output, 3));
    near(output[0].x, 12); near(output[1].x, 2); near(output[2].x, 16);
    attrs[1].attr_type = GX_INDEX16; polygon.display = display; polygon.n_display = 1;
    check(!nativeModelView_CornerPositions(0, output, 2));
    nativeModelView_Mesh(0, &owner, &mesh, true, false, &bone);
    check(!nativeModelView_CornerPositions(0, output, 3));
    nativeModelView_Mesh(0, &owner, &mesh, false, false, NULL);
    polygon.flags = POBJ_SHAPEANIM;
    polygon.u.shape_set=NULL;
    check(!nativeModelView_CornerPositions(0, output, 3));
    // The draw descriptors contain placeholder positions. Queries must sample
    // the shape targets and current blend before applying the owning joint.
    u8 first[3]={0,1,2},next[3]={1,2,0};u8* targets[]={first,next};
    HSD_ShapeSet shape={0};shape.flags=SHAPESET_AVERAGE;shape.nb_shape=2;
    shape.nb_vertex_index=3;shape.vertex_desc=&attrs[1];shape.vertex_idx_list=targets;shape.blend.bl=.25f;
    HSD_VtxDescList shape_desc=attrs[1];shape_desc.attr_type=GX_INDEX8;shape.vertex_desc=&shape_desc;
    polygon.u.shape_set=&shape;
    for(unsigned pose=0;pose<9;++pose) {
        owner.mtx[0][0]=1+.1f*pose;owner.mtx[0][1]=.2f*pose;owner.mtx[1][1]=1+.05f*pose;
        check(nativeModelView_CornerPositions(0,output,3));
        for(unsigned i=0;i<3;++i) {
            const unsigned a=order[i],b=(a+1)%3;
            float local[3];for(unsigned c=0;c<3;++c)local[c]=.75f*values[a][c]+.25f*values[b][c];
            near(output[i].x,owner.mtx[0][0]*local[0]+owner.mtx[0][1]*local[1]+20);
            near(output[i].y,owner.mtx[1][1]*local[1]);near(output[i].z,local[2]);
        }
        Vec3 a=output[0],b=output[1],c=output[2];
        const Vec3 u={b.x-a.x,b.y-a.y,b.z-a.z},v={c.x-a.x,c.y-a.y,c.z-a.z};
        const float direction[3]={u.z*v.y-u.y*v.z,u.x*v.z-u.z*v.x,u.y*v.x-u.x*v.y};
        const float origin[3]={(a.x+b.x+c.x)/3-direction[0],(a.y+b.y+c.y)/3-direction[1],
            (a.z+b.z+c.z)/3-direction[2]};
        check(nativeModelView_Pick(origin,direction)==0);
    }
    nativeModelView_Clear();
    check(!nativeModelView_CornerPositions(0, output, 3));
    printf("PASS: %u posed vertex, inverse-bind, index and visibility checks\n", checks);
    return 0;
}
