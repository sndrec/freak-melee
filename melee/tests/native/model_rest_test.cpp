#include <melee/ft/native_fighter_model_editor.h>
#include <melee/ft/native_asset_dat.h>
extern "C" {
#include <sysdolphin/baselib/dat_import.h>
}

#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>

namespace {
unsigned checks;
void check(bool ok, unsigned line=__builtin_LINE()) { ++checks; if (!ok) throw std::runtime_error("rest compensation check failed at line "+std::to_string(line)); }
struct Dat {
    std::vector<u8> data = std::vector<u8>(32);
    std::vector<u32> links;
    u32 allocate(unsigned n) { u32 at = ((u32) data.size() + 3) & ~3u; data.resize(at + n); return at; }
    void word(u32 at, u32 v) { NativeAssetDat::be32(data.data() + at, v); }
    void half(u32 at, u16 v) { NativeAssetDat::be16(data.data() + at, v); }
    void real(u32 at, float v) { u32 bits; std::memcpy(&bits, &v, 4); word(at, bits); }
    void link(u32 at, u32 v) { word(at, v); if (v) links.push_back(at); }
    std::vector<u8> finish(u32 root) {
        std::sort(links.begin(), links.end());
        std::vector<u8> file(32 + data.size() + links.size() * 4 + 13);
        NativeAssetDat::be32(file.data(), (u32) file.size());
        NativeAssetDat::be32(file.data() + 4, (u32) data.size());
        NativeAssetDat::be32(file.data() + 8, (u32) links.size());
        NativeAssetDat::be32(file.data() + 12, 1);
        std::memcpy(file.data() + 32, data.data(), data.size());
        size_t tail = 32 + data.size();
        for (u32 slot : links) { NativeAssetDat::be32(file.data() + tail, slot); tail += 4; }
        NativeAssetDat::be32(file.data() + tail, root);
        std::memcpy(file.data() + tail + 8, "rig\0", 4);
        return file;
    }
};
struct Fixture { std::vector<u8> file; u32 bones[3], mesh[2], polygon[2], bind; };
Fixture fixture(unsigned format, unsigned indexing, unsigned nbt, bool xy, bool envelope)
{
    Dat d; Fixture f = {};
    for (u32& b : f.bones) b = d.allocate(64);
    for (u32& m : f.mesh) m = d.allocate(16);
    for (u32& p : f.polygon) p = d.allocate(24);
    f.bind = d.allocate(48);
    for (unsigned r = 0; r < 3; ++r) d.real(f.bind + (r * 4 + r) * 4, 1);
    for (unsigned i = 0; i < 3; ++i) {
        const u32 b = f.bones[i]; d.word(b + 4, 8 | (envelope ? i == 0 ? 2 : 1 : 0));
        for (unsigned a = 0; a < 3; ++a) {
            d.real(b + 32 + a * 4, 1);
            d.real(b + 44 + a * 4, i ? float(i + a + 1) : 0);
        }
        if (i < 2) d.link(b + 8, f.bones[i + 1]);
        d.link(b + 56, f.bind); // Deliberately shared inverse-bind storage.
    }
    d.link(f.bones[0] + 16, f.mesh[0]); d.link(f.bones[1] + 16, f.mesh[1]);
    for (u32 m : f.mesh) d.link(m + 12, f.polygon[0]); // Shared polygon chain.
    d.link(f.polygon[0] + 4, f.polygon[1]);
    d.link(f.polygon[0] + 20, f.bones[0]);
    d.link(f.polygon[1] + 20, f.bones[2]); // Second rigid matrix slot.
    const unsigned scalar_bytes = format <= 1 ? 1 : format <= 3 ? 2 : 4;
    const unsigned pos_components = xy ? 2 : 3;
    const unsigned normal_components = nbt == 1 ? 9 : 3;
    const unsigned normal_count = nbt == 2 ? 9 : 3;
    const u32 pos = d.allocate(3 * pos_components * scalar_bytes);
    const u32 normals = d.allocate(normal_count * normal_components * scalar_bytes);
    auto scalar = [&](u32 at, float v, bool normal) {
        if (format == 4) d.real(at, v);
        else {
            const int scaled = (int) std::lround(std::ldexp(v, normal ? format == 1 ? 6 : 14 : 3));
            if (scalar_bytes == 1) d.data[at] = (u8) scaled;
            else d.half(at, (u16) scaled);
        }
    };
    for (unsigned v = 0; v < 3; ++v)
        for (unsigned a = 0; a < pos_components; ++a)
            scalar(pos + (v * pos_components + a) * scalar_bytes, float(v + a + 1) * .25f, false);
    for (unsigned v = 0; v < normal_count; ++v)
        for (unsigned a = 0; a < normal_components; ++a)
            scalar(normals + (v * normal_components + a) * scalar_bytes, float((v + a) % 3 + 1) * .125f, true);
    const u32 uv = d.allocate(24);
    for (unsigned i = 0; i < 6; ++i) d.real(uv + i * 4, float(i) / 6);
    const u32 attrs = d.allocate(6 * 24);
    // Reverse descriptor order: GX fetches hardware order, not list order.
    const unsigned attr[] = { 13, 11, 10, 9, 0 };
    for (unsigned i = 0; i < 5; ++i) {
        const u32 at = attrs + i * 24; d.word(at, attr[i]);
        d.word(at + 4, attr[i] == 0 || attr[i] == 11 ? 1 : attr[i] == 13 ? 2 : indexing);
        d.word(at + 8, attr[i] == 9 ? !xy : attr[i] == 10 ? nbt : 1);
        d.word(at + 12, attr[i] == 11 ? 5 : attr[i] == 13 ? 4 : format);
        d.data[at + 16] = attr[i] == 9 ? 3 : 0; // Normal frac is defined by GX, not this byte.
        d.half(at + 18, attr[i] == 9 ? pos_components * scalar_bytes : attr[i] == 10 ? normal_components * scalar_bytes : 8);
        if (attr[i] != 0 && attr[i] != 11) d.link(at + 20, attr[i] == 9 ? pos : attr[i] == 10 ? normals : uv);
    }
    d.word(attrs + 5 * 24, 255);
    std::vector<u8> commands = { 0x90, 0, 3 };
    for (unsigned v = 0; v < 3; ++v) {
        commands.push_back(v == 1 ? 3 : 0);
        for (unsigned a : {9u, 10u}) {
            const unsigned repeats = a == 10 && nbt == 2 ? 3 : 1;
            const unsigned bytes = a == 9 ? pos_components * scalar_bytes : normal_components * scalar_bytes;
            const u32 base = a == 9 ? pos : normals;
            for (unsigned k = 0; k < repeats; ++k) {
                const unsigned index = v * repeats + k;
                if (indexing == 1) commands.insert(commands.end(), d.data.begin() + base + index * bytes, d.data.begin() + base + (index + 1) * bytes);
                else { if (indexing == 3) commands.push_back(0); commands.push_back((u8) index); }
            }
        }
        commands.insert(commands.end(), { (u8) (30 + v), 50, 90, 255, (u8) v });
    }
    commands.resize((commands.size() + 31) & ~size_t(31));
    const u32 display = d.allocate((u32) commands.size());
    std::memcpy(d.data.data() + display, commands.data(), commands.size());
    for (u32 p : f.polygon) {
        d.link(p + 8, attrs); d.half(p + 14, (u16) (commands.size() / 32)); d.link(p + 16, display);
    }
    if (envelope) {
        const u32 table = d.allocate(12), single = d.allocate(16), blend = d.allocate(24);
        d.link(table, single); d.link(table + 4, blend);
        d.link(single, f.bones[2]); d.real(single + 4, 1);
        d.link(blend, f.bones[0]); d.real(blend + 4, .5f);
        d.link(blend + 8, f.bones[2]); d.real(blend + 12, .5f);
        // Replace previous union relocations rather than duplicating slots.
        for (u32 p : f.polygon) { d.word(p + 20, table); d.half(p + 12, 0x2000); }
    }
    f.file = d.finish(f.bones[0]); return f;
}

using Matrix = std::array<std::array<double, 4>, 3>;
Matrix multiply(const Matrix& a, const Matrix& b) {
    Matrix c = {};
    for (unsigned r = 0; r < 3; ++r) for (unsigned j = 0; j < 4; ++j) {
        c[r][j] = j == 3 ? a[r][3] : 0;
        for (unsigned k = 0; k < 3; ++k) c[r][j] += a[r][k] * b[k][j];
    }
    return c;
}
Matrix world(const NativeFighterModelEditor& e, unsigned joint) {
    const auto& b = e.joints[joint];
    const double x = b.rotation[0], y = b.rotation[1], z = b.rotation[2];
    const double sx = sin(x), cx = cos(x), sy = sin(y), cy = cos(y), sz = sin(z), cz = cos(z);
    Matrix local = {{{cz*cy, cz*sx*sy-cx*sz, cz*cx*sy+sx*sz, b.position[0]},
                     {sz*cy, sz*sx*sy+cx*cz, sz*cx*sy-sx*cz, b.position[1]},
                     {-sy, cy*sx, cy*cx, b.position[2]}}};
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 3; ++c) local[r][c] *= b.scale[c];
    return b.parent < 0 ? local : multiply(world(e, (unsigned) b.parent), local);
}
Matrix normal_matrix(const Matrix& m) {
    const double a=m[0][0], b=m[0][1], c=m[0][2], d=m[1][0], e=m[1][1], f=m[1][2], g=m[2][0], h=m[2][1], i=m[2][2];
    const double determinant=a*(e*i-f*h)-b*(d*i-f*g)+c*(d*h-e*g);
    Matrix result={{{e*i-f*h, f*g-d*i, d*h-e*g,0}, {c*h-b*i, a*i-c*g,b*g-a*h,0}, {b*f-c*e,c*d-a*f,a*e-b*d,0}}};
    for (auto& row:result) for (double& v:row) v/=determinant;
    return result;
}
Matrix inverse(const Matrix& m) {
    const Matrix transpose=normal_matrix(m); Matrix result={};
    for(unsigned r=0;r<3;++r){for(unsigned c=0;c<3;++c)result[r][c]=transpose[c][r];
        for(unsigned c=0;c<3;++c)result[r][3]-=result[r][c]*m[c][3];}
    return result;
}
Matrix envelope_matrix(const NativeFighterModelEditor& e,unsigned owner,u32 polygon,unsigned slot) {
    auto word=[&](u32 at){return NativeAssetDat::read32(e.dat.data+at);};
    auto bind=[&](unsigned bone){Matrix m={};u32 at=word(e.joints[bone].offset+56);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float f;check(HSD_DatReadF32(&e.dat,at+(r*4+c)*4,&f));m[r][c]=f;}return m;};
    const bool right=!(e.joints[owner].flags&2);
    u32 entry=word(word(polygon+20)+slot*4);float first;check(HSD_DatReadF32(&e.dat,entry+4,&first));
    Matrix result={};
    while(word(entry)) {
        unsigned bone=0;while(bone<e.joint_count&&e.joints[bone].offset!=word(entry))++bone;check(bone<e.joint_count);
        float weight;check(HSD_DatReadF32(&e.dat,entry+4,&weight));
        Matrix m=first>=1&&!right?world(e,bone):multiply(world(e,bone),bind(bone));
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)result[r][c]+=m[r][c]*(first>=1?1:weight);
        if(first>=1)break;
        entry+=8;
    }
    if(right){int skeleton=(int)owner;while(skeleton>=0&&!(e.joints[skeleton].flags&3))skeleton=e.joints[skeleton].parent;check(skeleton>=0);
        Matrix factor=skeleton==(int)owner?inverse(bind(owner)):
            multiply(inverse(e.joints[skeleton].flags&2?world(e,skeleton):multiply(world(e,skeleton),bind(skeleton))),world(e,owner));
        result=multiply(result,factor);}
    return result;
}
std::vector<double> geometry(const NativeFighterModelEditor& e) {
    auto word=[&](u32 at) {u32 v; check(HSD_DatReadU32(&e.dat,at,&v)); return v;};
    auto half=[&](u32 at) {u16 v; check(HSD_DatReadU16(&e.dat,at,&v)); return v;};
    auto real=[&](u32 at) {float v; check(HSD_DatReadF32(&e.dat,at,&v)); return v;};
    std::vector<double> result;
    for (unsigned mesh=0;mesh<e.mesh_count;++mesh) for(u32 p=word(e.meshes[mesh].offset+12);p;p=word(p+4)) {
        std::vector<u32> attrs;
        for(u32 a=word(p+8);word(a)!=255;a+=24) attrs.push_back(a);
        std::sort(attrs.begin(),attrs.end(),[&](u32 a,u32 b){return word(a)<word(b);});
        u32 cursor=word(p+16); check(e.dat.data[cursor++]==0x90); unsigned count=half(cursor);cursor+=2;
        for(unsigned v=0;v<count;++v) {
            unsigned slot=0;
            for(u32 a:attrs) {
                unsigned attr=word(a),type=word(a+4),n=word(a+8),fmt=word(a+12);
                if(attr==0) {slot=e.dat.data[cursor++]/3;continue;}
                if(attr==11) {for(unsigned k=0;k<4;++k)result.push_back(e.dat.data[cursor++]);continue;}
                if(attr==13) {result.push_back(e.dat.data[cursor++]);continue;}
                unsigned width=fmt<=1?1:fmt<=3?2:4,components=attr==9?(n?3:2):n==1?9:3;
                unsigned repeats=attr==10&&n==2?3:1;
                for(unsigned part=0;part<repeats;++part) {
                    u32 at=cursor;
                    if(type!=1) {unsigned index=type==2?e.dat.data[cursor]:half(cursor);cursor+=type==2?1:2;at=word(a+20)+index*half(a+18);}
                    else cursor+=width*components;
                    for(unsigned k=0;k<components;k+=3) {
                        double vector[3]={};
                        for(unsigned c=0;c<3&&k+c<components;++c) {
                            const u32 offset=at+(k+c)*width;
                            if(fmt==4) vector[c]=real(offset);
                            else {int raw=fmt==1?(int8_t)e.dat.data[offset]:(int16_t)half(offset);
                                vector[c]=std::ldexp(double(raw),-(int)(attr==10?(fmt==1?6:14):e.dat.data[a+16]));}
                        }
                        unsigned bone=e.meshes[mesh].joint;
                        if((half(p+12)&0x3000)!=0x2000&&slot&&word(p+20)) {for(bone=0;bone<e.joint_count&&e.joints[bone].offset!=word(p+20);++bone){} check(bone<e.joint_count);}
                        const Matrix model=(half(p+12)&0x3000)==0x2000?envelope_matrix(e,e.meshes[mesh].joint,p,slot):world(e,bone);
                        const Matrix matrix=attr==9?model:normal_matrix(model);
                        for(unsigned r=0;r<3;++r) {double value=attr==9?matrix[r][3]:0;for(unsigned c=0;c<3;++c)value+=matrix[r][c]*vector[c];result.push_back(value);}
                    }
                }
            }
        }
    }
    return result;
}
void run(unsigned format,unsigned indexing,unsigned nbt,bool xy,unsigned edited,unsigned channel,bool envelope) {
    Fixture f=fixture(format,indexing,nbt,xy,envelope);
    NativeFighterProject project={};std::strcpy(project.facts.costume_joint,"rig");
    project.costume_dat=(u8*)std::malloc(f.file.size());project.costume_size=f.file.size();std::memcpy(project.costume_dat,f.file.data(),f.file.size());
    auto e=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(e.get(),&project));
    const auto before=geometry(*e);
    std::vector<Matrix> skin_before;
    for(unsigned j=0;j<e->joint_count;++j) {
        Matrix bind={};u32 at=NativeAssetDat::read32(e->dat.data+e->joints[j].offset+56);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float v;check(HSD_DatReadF32(&e->dat,at+(r*4+c)*4,&v));bind[r][c]=v;}
        skin_before.push_back(multiply(world(*e,j),bind));
    }
    check(nativeFighterModel_SetJoint(e.get(),edited,channel,channel==1?0:2,channel==0?.73f:channel==1?1.7f:8.25f));
    check(nativeFighterModel_CompensateRest(e.get(),f.file.data(),f.file.size(),edited));
    const auto after=geometry(*e);check(before.size()==after.size());
    for(unsigned j=0;j<e->joint_count;++j) {
        Matrix bind={};u32 at=NativeAssetDat::read32(e->dat.data+e->joints[j].offset+56);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c){float v;check(HSD_DatReadF32(&e->dat,at+(r*4+c)*4,&v));bind[r][c]=v;}
        const Matrix current=multiply(world(*e,j),bind);
        for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)check(std::fabs(current[r][c]-skin_before[j][r][c])<.0001);
    }
    for(unsigned i=0;i<before.size();++i)check(std::fabs(before[i]-after[i])<.0001);
    // The shared original polygon/vertex storage was never modified.
    for(u32 p:f.polygon)check(!std::memcmp(project.costume_dat+32+p,f.file.data()+32+p,24));
    if(edited==1)check(NativeAssetDat::read32(project.costume_dat+32+f.bones[0]+56)==f.bind);
    check(nativeFighterModel_Open(e.get(),&project));check(geometry(*e)==after);
    std::free(project.costume_dat);
}
Fixture shape_fixture(unsigned format, unsigned indexing, unsigned normals, bool additive) {
    Fixture f=fixture(4,3,0,false,false);
    NativeAssetDat d;check(d.open(f.file.data(),f.file.size()));
    auto real=[&](u32 at,float v){u32 bits;std::memcpy(&bits,&v,4);d.word(at,bits);};
    const unsigned width=format<2?1:format<4?2:4;
    auto scalar=[&](u32 at,float v){if(format==4)real(at,v);else if(width==1)d.data[at]=(u8)std::lround(v*4);else d.half(at,(u16)std::lround(v*4));};
    const u32 shape=d.allocate(28);
    d.half(shape,additive?2:1);d.half(shape+2,additive?2:3);d.word(shape+4,3);
    if(normals)d.word(shape+16,3);
    for(unsigned attribute=0;attribute<(normals?2u:1u);++attribute){
        const unsigned vectors=attribute&&normals==2?3:1;
        const u32 descriptor=d.allocate(48),table=d.allocate(12),array=d.allocate(6*vectors*3*width);
        d.link(shape+8+attribute*12,descriptor);d.link(shape+12+attribute*12,table);
        d.word(descriptor,attribute?(normals==2?25:10):9);d.word(descriptor+4,indexing);
        d.word(descriptor+8,attribute?0:1);d.word(descriptor+12,format);
        d.data[descriptor+16]=2;d.half(descriptor+18,(u16)(vectors*3*width));d.link(descriptor+20,array);
        d.word(descriptor+24,255);
        for(unsigned entry=0;entry<6;++entry)for(unsigned part=0;part<vectors;++part)for(unsigned axis=0;axis<3;++axis)
            scalar(array+((entry*vectors+part)*3+axis)*width,float(1+entry+part+axis)*.25f);
        for(unsigned target=0;target<3;++target){
            const u32 indices=d.allocate(indexing==2?3:6);d.link(table+target*4,indices);
            for(unsigned i=0;i<3;++i){unsigned index=(target*2+i)%6;
                if(indexing==2)d.data[indices+i]=(u8)index;else d.half(indices+i*2,(u16)index);}
        }
    }
    const u32 attrs=d.allocate((normals?6:5)*24);
    std::vector<unsigned> ids={0,9};if(normals)ids.push_back(normals==2?25:10);ids.push_back(11);ids.push_back(13);
    for(unsigned i=0;i<ids.size();++i){u32 at=attrs+i*24;d.word(at,ids[i]);
        d.word(at+4,ids[i]==0||ids[i]==11?1:indexing);d.word(at+8,ids[i]==9||ids[i]==13?1:0);
        d.word(at+12,ids[i]==11?5:4);}
    d.word(attrs+(u32)ids.size()*24,255);
    std::vector<u8> commands={0x90,0,3};
    for(unsigned i=0;i<3;++i){commands.push_back(0);
        for(unsigned k=0;k<(normals?2u:1u);++k){if(indexing==3)commands.push_back(0);commands.push_back((u8)i);}
        commands.insert(commands.end(),{10,20,30,255});if(indexing==3)commands.push_back(0);commands.push_back((u8)i);}
    commands.resize((commands.size()+31)&~size_t(31));u32 display=d.allocate(commands.size());
    std::memcpy(d.data.data()+display,commands.data(),commands.size());
    for(u32 polygon:f.polygon){d.half(polygon+12,0x1000);d.link(polygon+8,attrs);d.link(polygon+16,display);
        d.half(polygon+14,(u16)(commands.size()/32));d.link(polygon+20,shape);}
    std::vector<u8> result;check(d.finish(&result));f.file=std::move(result);return f;
}

// Independently evaluate the runtime's base+delta and adjacent-target blends,
// then compare world-space positions and lighting vectors after the rest edit.
std::vector<double> shape_geometry(const NativeFighterModelEditor& e) {
    auto word=[&](u32 at){u32 v;check(HSD_DatReadU32(&e.dat,at,&v));return v;};
    auto half=[&](u32 at){u16 v;check(HSD_DatReadU16(&e.dat,at,&v));return v;};
    auto value=[&](u32 descriptor,unsigned index,unsigned component){
        u32 at=word(descriptor+20)+index*half(descriptor+18);unsigned format=word(descriptor+12);
        if(format==4){float v;check(HSD_DatReadF32(&e.dat,at+component*4,&v));return double(v);}
        double raw;
        if(format<2){u8 byte=e.dat.data[at+component];raw=format==1?(int8_t)byte:byte;}
        else{u16 v=(u16)half(at+component*2);raw=format==3?(int16_t)v:v;}
        return std::ldexp(raw,-int(e.dat.data[descriptor+16]));
    };
    std::vector<double> result;
    for(unsigned mesh=0;mesh<e.mesh_count;++mesh)for(u32 p=word(e.meshes[mesh].offset+12);p;p=word(p+4)){
        u32 shape=word(p+20);bool additive=(half(shape)&2)!=0;
        const Matrix position=world(e,e.meshes[mesh].joint),normal=normal_matrix(position);
        for(double blend:{-1.0,0.0,.25,.8,1.0,1.4,2.0,3.0})for(unsigned attr=0;attr<2;++attr){
            unsigned count=word(shape+4+attr*12);if(!count)continue;
            u32 descriptor=word(shape+8+attr*12),table=word(shape+12+attr*12);
            unsigned vectors=word(descriptor)==25?3:1;
            auto sample=[&](unsigned target,unsigned index,unsigned component){
                u32 list=word(table+target*4);unsigned source=word(descriptor+4)==2?e.dat.data[list+index]:half(list+index*2);
                return value(descriptor,source,component);};
            for(unsigned i=0;i<count;++i)for(unsigned part=0;part<vectors;++part){
                double v[3];for(unsigned axis=0;axis<3;++axis){unsigned component=part*3+axis;
                    if(additive){v[axis]=sample(0,i,component)+sample(1,i,component)*std::fmax(0.0,blend)+sample(2,i,component)*std::fmax(0.0,1.3-blend);}
                    else{unsigned first=(unsigned)std::fmin(std::fmax(0.0,std::trunc(blend)),half(shape+2)-1.0);
                        unsigned next=std::min(first+1,unsigned(half(shape+2)-1));double t=std::fmin(std::fmax(0.0,blend-first),1.0);
                        v[axis]=sample(first,i,component)*(1-t)+sample(next,i,component)*t;}}
                const Matrix& m=attr?normal:position;
                for(unsigned row=0;row<3;++row)result.push_back(m[row][0]*v[0]+m[row][1]*v[1]+m[row][2]*v[2]+m[row][3]);
            }
        }
    }return result;
}
void import_shapes(const NativeFighterModelEditor& e){
    std::vector<u8> arena(256*1024);
    for(unsigned mesh=0;mesh<e.mesh_count;++mesh){
        u32 polygon=NativeAssetDat::read32(e.dat.data+e.meshes[mesh].offset+12);
        const u32 shape=NativeAssetDat::read32(e.dat.data+polygon+20);
        HSD_DatImport loader;
        check(HSD_DatImportInit(&loader,&e.dat,arena.data(),arena.size(),256)==HSD_DAT_OK);
        auto* imported=(HSD_ShapeSetDesc*)HSD_DatImportGraph(&loader,shape,DAT_SHAPE_SET);
        check(imported&&loader.status==HSD_DAT_OK);
        check(imported->vertex_desc->comp_cnt==GX_POS_XYZ&&imported->vertex_idx_list);
    }
}
void run_shape(unsigned format,unsigned indexing,unsigned normals,bool additive,unsigned edited,unsigned channel){
    Fixture f=shape_fixture(format,indexing,normals,additive);
    NativeFighterProject p={};std::strcpy(p.facts.costume_joint,"rig");p.costume_size=f.file.size();p.costume_dat=(u8*)std::malloc(p.costume_size);std::memcpy(p.costume_dat,f.file.data(),p.costume_size);
    auto e=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(e.get(),&p));import_shapes(*e);const auto before=shape_geometry(*e);
    check(nativeFighterModel_SetJoint(e.get(),edited,channel,channel==1?0:2,channel==0?.73f:channel==1?1.7f:8.25f));
    check(nativeFighterModel_CompensateRest(e.get(),f.file.data(),f.file.size(),edited));
    import_shapes(*e);const auto after=shape_geometry(*e);check(before.size()==after.size());
    for(unsigned i=0;i<before.size();++i)check(std::fabs(before[i]-after[i])<.0001);
    for(u32 polygon:f.polygon)check(!std::memcmp(p.costume_dat+32+polygon,f.file.data()+32+polygon,24));
    if(edited==1)check(NativeAssetDat::read32(p.costume_dat+32+f.mesh[0]+12)==f.polygon[0]);
    check(nativeFighterModel_Open(e.get(),&p));check(shape_geometry(*e)==after);std::free(p.costume_dat);
}
void shape_rejection(unsigned fault){
    Fixture f=shape_fixture(4,3,1,true);
    u8* data=f.file.data()+32;
    u32 shape=NativeAssetDat::read32(data+f.polygon[0]+20);
    u32 descriptor=NativeAssetDat::read32(data+shape+8);
    switch(fault){
    case 0:NativeAssetDat::be16(data+shape,3);break; // Inconsistent blend modes.
    case 1:NativeAssetDat::be32(data+shape+4,2001);break;
    case 2:NativeAssetDat::be32(data+shape+16,2001);break;
    case 3:NativeAssetDat::be32(data+descriptor+4,1);break; // Shapes require indices.
    case 4:NativeAssetDat::be32(data+NativeAssetDat::read32(data+descriptor+20),0x7fc00000);break;
    case 5:{u32 table=NativeAssetDat::read32(data+shape+12);
        NativeAssetDat::be32(data+table,NativeAssetDat::read32(f.file.data()+4)-1);break;}
    }
    NativeFighterProject p={};std::strcpy(p.facts.costume_joint,"rig");p.costume_size=f.file.size();p.costume_dat=(u8*)std::malloc(p.costume_size);std::memcpy(p.costume_dat,f.file.data(),p.costume_size);
    auto e=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(e.get(),&p));
    check(nativeFighterModel_SetJoint(e.get(),0,2,0,8));
    const std::vector<u8> changed(p.costume_dat,p.costume_dat+p.costume_size);
    check(!nativeFighterModel_CompensateRest(e.get(),f.file.data(),f.file.size(),0));
    check(p.costume_size==changed.size()&&!std::memcmp(p.costume_dat,changed.data(),changed.size()));std::free(p.costume_dat);
}
void rejection() {
    auto f=fixture(4,3,0,false,false);
    NativeAssetDat::be16(f.file.data()+32+f.polygon[1]+12,0x1000);
    NativeFighterProject p={};std::strcpy(p.facts.costume_joint,"rig");p.costume_size=f.file.size();p.costume_dat=(u8*)std::malloc(p.costume_size);std::memcpy(p.costume_dat,f.file.data(),p.costume_size);
    auto e=std::make_unique<NativeFighterModelEditor>();check(nativeFighterModel_Open(e.get(),&p));
    check(nativeFighterModel_SetJoint(e.get(),1,2,0,8));
    const std::vector<u8> changed(p.costume_dat,p.costume_dat+p.costume_size);
    check(!nativeFighterModel_CompensateRest(e.get(),f.file.data(),f.file.size(),1));
    check(p.costume_size==changed.size()&&!std::memcmp(p.costume_dat,changed.data(),changed.size()));std::free(p.costume_dat);
}
}
int main() {
    try {
        for(unsigned format:{1u,3u,4u})for(unsigned indexing:{1u,2u,3u})for(unsigned nbt:{0u,1u,2u})for(bool xy:{false,true})for(unsigned bone:{0u,1u})for(unsigned channel:{0u,1u,2u})for(bool envelope:{false,true})run(format,indexing,nbt,xy,bone,channel,envelope);
        for(unsigned format:{0u,1u,2u,3u,4u})for(unsigned indexing:{2u,3u})for(unsigned normals:{0u,1u,2u})for(bool additive:{false,true})for(unsigned bone:{0u,1u})for(unsigned channel:{0u,1u,2u})run_shape(format,indexing,normals,additive,bone,channel);
        for(unsigned fault=0;fault<6;++fault)shape_rejection(fault);
        rejection();std::printf("Model rest compensation: %u checks passed.\n",checks);return 0;
    }catch(const std::exception& error){std::fprintf(stderr,"%s after %u checks\n",error.what(),checks);return 1;}
}
