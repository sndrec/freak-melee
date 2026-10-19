extern "C" {
#include <sysdolphin/baselib/shape.h>
}
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
unsigned checks;
void check(bool value,unsigned line=__builtin_LINE())
{++checks;if(!value)throw std::runtime_error("shape capacity check at "+std::to_string(line));}
using Vector=std::array<float,3>;
float component(unsigned index,unsigned axis)
{return float(index%997)*.03125f+float(axis)*.125f;}

void evaluate(unsigned count,unsigned normal_vectors,bool additive)
{
    std::vector<float> positions(size_t(count)*3),normals(size_t(count)*normal_vectors*3);
    std::vector<u8> index_bytes[2];u8* indices[2];
    for(unsigned target=0;target<2;++target) {
        index_bytes[target].resize(size_t(count)*2);indices[target]=index_bytes[target].data();
        for(unsigned i=0;i<count;++i) {
            const unsigned index=target?count-i-1:i;
            indices[target][i*2]=(u8)(index>>8);indices[target][i*2+1]=(u8)index;
        }
    }
    for(unsigned i=0;i<count;++i)for(unsigned c=0;c<3;++c) {
        positions[i*3+c]=component(i,c);
        for(unsigned v=0;v<normal_vectors;++v)normals[(i*normal_vectors+v)*3+c]=component(i,c+v*3);
    }
    HSD_VtxDescList position={};position.attr=GX_VA_POS;position.attr_type=GX_INDEX16;
    position.comp_cnt=GX_POS_XYZ;position.comp_type=GX_F32;position.stride=12;
    position.vertex=positions.data();position.vertex_little_endian=true;
    HSD_VtxDescList normal=position;normal.attr=normal_vectors==3?GX_VA_NBT:GX_VA_NRM;
    normal.comp_cnt=GX_NRM_XYZ;normal.stride=(u16)(normal_vectors*12);normal.vertex=normals.data();
    HSD_ShapeSet shape={};shape.flags=additive?SHAPESET_ADDITIVE|SHAPESET_SIGNED_WEIGHTS:SHAPESET_AVERAGE;
    shape.nb_shape=additive?1:2;shape.nb_vertex_index=count;shape.vertex_desc=&position;shape.vertex_idx_list=indices;
    shape.nb_normal_index=normal_vectors?count:0;shape.normal_desc=&normal;shape.normal_idx_list=indices;
    float weight=-.375f;if(additive)shape.blend.bp=&weight;else shape.blend.bl=.375f;
    const Vector guard={1234,5678,9012};
    std::vector<Vector> output(count+2,guard),lighting(size_t(count)*normal_vectors+2,guard);
    auto* output_rows=reinterpret_cast<float(*)[3]>(output.data()+1);
    auto* lighting_rows=reinterpret_cast<float(*)[3]>(lighting.data()+1);
    const HSD_ShapeSet before=shape;
    check(HSD_ShapeSetEvaluate(&shape,output_rows,lighting_rows));
    check(std::memcmp(&shape,&before,sizeof(shape))==0);
    for(unsigned i=0;i<count;++i)for(unsigned c=0;c<3;++c) {
        const float a=component(i,c),b=component(count-i-1,c);
        check(output[i+1][c]==(additive?a+b*weight:(b-a)*.375f+a));
        for(unsigned v=0;v<normal_vectors;++v) {
            const float n=component(i,c+v*3),m=component(count-i-1,c+v*3);
            check(lighting[size_t(i)*normal_vectors+v+1][c]==(additive?n+m*weight:(m-n)*.375f+n));
        }
    }
    check(output.front()==guard&&output.back()==guard&&lighting.front()==guard&&lighting.back()==guard);
    check(HSD_ShapeSetEvaluate(&shape,output_rows,nullptr));
    shape.nb_vertex_index=HSD_MAX_SHAPE_VERTICES+1;output[1]=guard;
    check(!HSD_ShapeSetEvaluate(&shape,output_rows,lighting_rows)&&output[1]==guard);
    shape.nb_vertex_index=count;shape.nb_normal_index=HSD_MAX_SHAPE_NORMALS+1;
    check(!HSD_ShapeSetEvaluate(&shape,output_rows,lighting_rows)&&output[1]==guard);
    shape.nb_normal_index=-1;
    check(!HSD_ShapeSetEvaluate(&shape,output_rows,lighting_rows)&&output[1]==guard);
}
}
int main()
{
    try {
        for(unsigned count:{2001u,65535u,65536u})for(unsigned normals:{0u,1u,3u})
            for(bool additive:{false,true})evaluate(count,normals,additive);
        std::printf("PASS: %u large morph XYZ/NBT, final-index, guarded-output and capacity checks\n",checks);return 0;
    } catch(const std::exception& error){std::fprintf(stderr,"FAIL: %s\n",error.what());return 1;}
}
