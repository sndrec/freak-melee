#undef NDEBUG
#include <melee/ft/native_fighter_rig_animation.h>
#include <melee/ft/native_asset_dat.h>
#include <melee/ft/ftparts.h>
#include <melee/lb/lbanim_dat.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <vector>

using Bytes=std::vector<uint8_t>;
static unsigned long long checks;
static unsigned cases,clips;
#define CHECK(x) do {++checks;if(!(x)){std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
static char error[256];
static Bytes load(const char* path)
{std::ifstream f(path,std::ios::binary);CHECK(f.good());return Bytes(std::istreambuf_iterator<char>(f),{});}
static HSD_Dat open(const void* p,size_t n)
{HSD_Dat d;CHECK(HSD_DatOpen(&d,p,n)==HSD_DAT_OK);return d;}
static uint32_t word(const HSD_Dat& d,uint32_t at)
{u32 v;CHECK(HSD_DatReadU32(&d,at,&v));return (uint32_t)v;}
static uint32_t ref(const HSD_Dat& d,uint32_t at)
{u32 v;const char* name;CHECK(HSD_DatResolve(&d,at,&v,&name)==HSD_DAT_REF_LOCAL);return (uint32_t)v;}
static uint32_t root(const HSD_Dat& d,const char* name)
{u32 at;CHECK(HSD_DatFindRoot(&d,name,&at));return (uint32_t)at;}
struct Tree {FigaTree tree={};FigaTrack tracks[4096];u32 count;unsigned starts[513]={};};
static void decode(const HSD_Dat& d,unsigned at,Tree& t)
{
 CHECK(lbAnim_DecodeFigaTree(&d,at,&t.tree,t.tracks,4096,&t.count)==HSD_DAT_OK&&t.count<=512);
 t.starts[0]=0;for(unsigned i=0;i<t.count;++i)t.starts[i+1]=t.starts[i]+(uint8_t)t.tree.nodes[i];
}
static bool present(const NativeSourceAssetFacts& f,unsigned p,unsigned mask)
{for(unsigned i=0;i<f.omission_count;++i)if(f.omissions[i].joint==p)return (mask>>i)&1;return true;}
struct Clip {HSD_Dat dat;uint32_t root;};
static bool locate(const uint8_t* bytes,size_t size,const char* name,int offset,int length,Clip& out)
{
 if(offset>=0&&length>=32&&(size_t)offset<=size&&(size_t)length<=size-(size_t)offset){
  HSD_Dat d;u32 at;if(HSD_DatOpen(&d,bytes+offset,length)==HSD_DAT_OK&&HSD_DatFindRoot(&d,name,&at)){out={d,(uint32_t)at};return true;}}
 bool found=false;
 for(size_t at=0;at<size;){if(bytes[at]==255){++at;continue;}
  CHECK(size-at>=32);unsigned n=NativeAssetDat::read32(bytes+at);CHECK(n>=32&&n<=size-at);
  auto d=open(bytes+at,n);u32 r;if(HSD_DatFindRoot(&d,name,&r)){CHECK(!found);found=true;out={d,(uint32_t)r};}at+=n;}
 return found;
}
static void compare(const NativeFighterProject& p,const NativeSourceAssetFacts& next,
 const std::vector<uint16_t>& order,const NativeFighterRigAnimation& result)
{
 auto a=open(p.fighter_dat,p.fighter_size),b=open(result.fighter,result.fighter_size);
 CHECK(p.fighter_size==result.fighter_size);auto ar=root(a,p.fighter_root),br=root(b,p.fighter_root);
 std::vector<uint8_t> allowed(p.fighter_size,0);
 const unsigned counts[]={p.action_count,p.demo_count};
 for(unsigned bank=0;bank<2;++bank)if(counts[bank]){
  auto at=ref(a,ar+(bank?20:12)),bt=ref(b,br+(bank?20:12));
  for(unsigned i=0;i<counts[bank];++i){
   auto x=at+i*24,y=bt+i*24;CHECK(x==y);if(!word(a,x+8))continue;
   const char* name=(const char*)a.data+ref(a,x);CHECK(!std::strcmp(name,(const char*)b.data+ref(b,y)));
   Clip ca,cb;if(!locate(p.animation_dat,p.animation_size,name,(int32_t)word(a,x+4),(int32_t)word(a,x+8),ca)){
    CHECK(bank==1&&!std::memcmp(a.data+x,b.data+y,24));continue;}
   CHECK(locate(result.animation,result.animation_size,name,(int32_t)word(b,y+4),(int32_t)word(b,y+8),cb));
   // Every resolved descriptor points directly to its complete new archive.
   CHECK(HSD_DatOpen(&cb.dat,result.animation+word(b,y+4),word(b,y+8))==HSD_DAT_OK);
   cb.root=root(cb.dat,name);for(unsigned n=4;n<12;++n)allowed[32+x+n]=1;
   Tree ta,tb;decode(ca.dat,ca.root,ta);decode(cb.dat,cb.root,tb);
   CHECK(!std::memcmp(ca.dat.data+ca.root,cb.dat.data+cb.root,12));
   const unsigned flags=word(a,x+16),mask=(flags>>9)&8191;
   std::vector<uint16_t> expected;
   if((flags&63)!=p.facts.animation_tag){for(unsigned n=0;n<ta.count;++n)expected.push_back((uint16_t)n);}
   else {
    std::vector<unsigned> before,after;
    for(unsigned n=0;n<p.facts.joints+p.facts.omission_count;++n)if(present(p.facts,n,mask))before.push_back(n);
    for(unsigned n=0;n<next.joints+next.omission_count;++n)if(present(next,n,mask))after.push_back(n);
    CHECK(before.size()==ta.count);
    for(auto n:after){if(order[n]==UINT16_MAX)expected.push_back(UINT16_MAX);
     else {auto match=std::find(before.begin(),before.end(),order[n]);CHECK(match!=before.end());expected.push_back((uint16_t)(match-before.begin()));}}
   }
   CHECK(tb.count==expected.size());
   for(unsigned n=0;n<expected.size();++n){
    if(expected[n]==UINT16_MAX){CHECK(tb.tree.nodes[n]==0);continue;}
    unsigned old=expected[n];CHECK(tb.tree.nodes[n]==ta.tree.nodes[old]);
    for(unsigned k=0;k<(uint8_t)tb.tree.nodes[n];++k){
     const auto& s=ta.tracks[ta.starts[old]+k];const auto& d=tb.tracks[tb.starts[n]+k];
     CHECK(s.length==d.length&&s.startframe==d.startframe&&s.obj_type==d.obj_type&&s.frac_value==d.frac_value&&s.frac_slope==d.frac_slope);
     CHECK(!std::memcmp(s.ad_head,d.ad_head,s.length));
     CHECK(!std::memcmp(ca.dat.data+ref(ca.dat,ca.root+16)+(ta.starts[old]+k)*12,
                       cb.dat.data+ref(cb.dat,cb.root+16)+(tb.starts[n]+k)*12,8));
    }
   }++clips;
  }
 }
 for(size_t n=0;n<p.fighter_size;++n)if(!allowed[n])CHECK(p.fighter_dat[n]==result.fighter[n]);
}
static void exercise(NativeFighterProject& p)
{
 const Bytes fighter(p.fighter_dat,p.fighter_dat+p.fighter_size),animation(p.animation_dat,p.animation_dat+p.animation_size);
 const unsigned parts=p.facts.joints+p.facts.omission_count;
 for(unsigned mode=0;mode<3;++mode){
  std::vector<uint16_t> order;NativeSourceAssetFacts next=p.facts;unsigned insertion=parts/2;
  if(mode==0){for(unsigned n=0;n<parts;++n)order.push_back((uint16_t)n);}
  if(mode==1){CHECK(parts<MAX_FT_PARTS);++next.joints;for(unsigned n=0;n<parts+1;++n)order.push_back(n==insertion?UINT16_MAX:(uint16_t)(n>insertion?n-1:n));}
  if(mode==2){for(unsigned n=parts;n;--n)order.push_back((uint16_t)(n-1));}
  unsigned inverse[MAX_FT_PARTS];std::fill_n(inverse,MAX_FT_PARTS,UINT_MAX);
  for(unsigned n=0;n<order.size();++n)if(order[n]!=UINT16_MAX)inverse[order[n]]=n;
  for(unsigned i=0;i<next.omission_count;++i){next.omissions[i].joint=(uint8_t)inverse[next.omissions[i].joint];next.omissions[i].parent=(uint8_t)inverse[next.omissions[i].parent];}
  for(unsigned i=0;i<next.bone_count;++i)next.bones[i].joint=(uint8_t)inverse[next.bones[i].joint];
  NativeFighterRigAnimation out={};bool ok=nativeFighterRig_RemapAnimationBank(&p,&next,order.data(),(unsigned)order.size(),&out,error,sizeof(error));
  if(!ok)std::fprintf(stderr,"Mode %u: %s\n",mode,error);CHECK(ok);compare(p,next,order,out);
  if(mode==0)CHECK(out.animation_size==p.animation_size&&!std::memcmp(out.animation,p.animation_dat,p.animation_size));
  if(mode==1){
   NativeFighterProject inserted=p;inserted.facts=next;inserted.fighter_dat=out.fighter;inserted.fighter_size=out.fighter_size;inserted.animation_dat=out.animation;inserted.animation_size=out.animation_size;
   std::vector<uint16_t> restore;for(unsigned n=0;n<parts;++n)restore.push_back((uint16_t)inverse[n]);
   NativeFighterRigAnimation restored={};CHECK(nativeFighterRig_RemapAnimationBank(&inserted,&p.facts,restore.data(),parts,&restored,error,sizeof(error)));
   compare(inserted,p.facts,restore,restored);std::free(restored.fighter);std::free(restored.animation);++cases;
  }
  std::free(out.fighter);std::free(out.animation);++cases;
 }
 CHECK(!std::memcmp(p.fighter_dat,fighter.data(),fighter.size())&&!std::memcmp(p.animation_dat,animation.data(),animation.size()));
 std::vector<uint16_t> invalid(parts,0);NativeFighterRigAnimation out={};
 CHECK(!nativeFighterRig_RemapAnimationBank(&p,&p.facts,invalid.data(),parts,&out,error,sizeof(error)));
 CHECK(!out.fighter&&!out.animation&&!out.fighter_size&&!out.animation_size);++cases;
}
static NativeAssetDat empty(Bytes& source)
{source.assign(32,0);NativeAssetDat::be32(source.data(),32);NativeAssetDat dat;CHECK(dat.open(source.data(),source.size()));return dat;}
static Bytes make_clip(const std::vector<unsigned>& counts,const std::vector<const char*>& names,bool alias=false)
{
 Bytes source,result;auto d=empty(source);
 for(unsigned t=0;t<counts.size();++t){unsigned n=counts[t];auto at=d.allocate(20),nodes=d.allocate(n+1),tracks=d.allocate(n*12);
  d.word(at,1);d.word(at+8,0x42440000);d.link_local(at+12,nodes);d.link_local(at+16,tracks);
  for(unsigned i=0;i<n;++i){d.data[nodes+i]=1;auto stream=d.allocate(6);d.data[stream]=2;d.data[stream+5]=1;
   d.half(tracks+i*12,6);d.data[tracks+i*12+4]=1;d.link_local(tracks+i*12+8,stream);}
  d.data[nodes+n]=255;CHECK(d.add_root(names[t],at));
 }if(alias)CHECK(d.add_root("alias",0));CHECK(d.finish(&result));return result;
}
static void preserved(const NativeFighterProject& p,const NativeFighterRigAnimation& result,const char* name)
{
 Clip a,b;CHECK(locate(p.animation_dat,p.animation_size,name,-1,0,a));CHECK(locate(result.animation,result.animation_size,name,-1,0,b));
 Tree x,y;decode(a.dat,a.root,x);decode(b.dat,b.root,y);CHECK(x.count==y.count&&!std::memcmp(a.dat.data+a.root,b.dat.data+b.root,12));
 for(unsigned n=0;n<x.count;++n){CHECK(x.tree.nodes[n]==y.tree.nodes[n]);for(unsigned k=x.starts[n];k<x.starts[n+1];++k){
  CHECK(!std::memcmp(a.dat.data+ref(a.dat,a.root+16)+k*12,b.dat.data+ref(b.dat,b.root+16)+k*12,8));
  CHECK(x.tracks[k].length==y.tracks[k].length&&!std::memcmp(x.tracks[k].ad_head,y.tracks[k].ad_head,x.tracks[k].length));}}
}
static void synthetic()
{
 auto ab=make_clip({2,3,5},{"a","b","spare"},true),c=make_clip({4},{"c"});
 Bytes animation(3,255);animation.insert(animation.end(),ab.begin(),ab.end());animation.insert(animation.end(),5,255);unsigned c_at=(unsigned)animation.size();animation.insert(animation.end(),c.begin(),c.end());animation.insert(animation.end(),7,255);
 Bytes source,fighter;auto d=empty(source);auto at=d.allocate(96),actions=d.allocate(4*24),demos=d.allocate(24);
 d.link_local(at+12,actions);d.link_local(at+20,demos);CHECK(d.add_root("fighter",at));
 for(unsigned i=0;i<5;++i){unsigned row=i==4?demos:actions+i*24;const char* name=i==0||i==3?"a":i==1?"b":i==2?"c":"external";
  auto text=d.allocate(std::strlen(name)+1,1);std::memcpy(d.data.data()+text,name,std::strlen(name)+1);d.link_local(row,text);
  d.word(row+4,i==4?100000:i==2?c_at:3);d.word(row+8,i==4?32:i==2?(uint32_t)c.size():(uint32_t)ab.size());d.word(row+16,i==1?512:i==2?1:0);
 }CHECK(d.finish(&fighter));
 NativeFighterProject p={};std::strcpy(p.fighter_root,"fighter");p.action_count=4;p.demo_count=1;
 p.facts.joints=2;p.facts.omission_count=1;p.facts.omissions[0]={1,0,1,255};
 p.fighter_dat=fighter.data();p.fighter_size=fighter.size();p.animation_dat=animation.data();p.animation_size=animation.size();exercise(p);
 auto next=p.facts;++next.joints;next.omissions[0].joint=2;
 uint16_t order[]={0,UINT16_MAX,1,2};NativeFighterRigAnimation result={};
 CHECK(nativeFighterRig_RemapAnimationBank(&p,&next,order,4,&result,error,sizeof(error)));
 preserved(p,result,"spare");preserved(p,result,"c");
 Clip changed;CHECK(locate(result.animation,result.animation_size,"a",-1,0,changed));CHECK(root(changed.dat,"alias")==changed.root);
 CHECK(result.owned_roots==2&&result.foreign_roots==1&&result.unreferenced_roots==2&&result.descriptors==4);
 std::free(result.fighter);std::free(result.animation);
 // A foreign descriptor sharing the same tree would need a different map
 // after insertion. Reject the complete pair, retaining both source buffers.
 const Bytes original=fighter;NativeAssetDat::be32(fighter.data()+32+actions+3*24+16,1);
 result={};CHECK(!nativeFighterRig_RemapAnimationBank(&p,&next,order,4,&result,error,sizeof(error)));
 CHECK(!result.fighter&&!result.animation&&!result.fighter_size&&!result.animation_size);fighter=original;p.fighter_dat=fighter.data();++cases;
 const size_t length=p.animation_size;p.animation_size-=8;
 CHECK(!nativeFighterRig_RemapAnimationBank(&p,&next,order,4,&result,error,sizeof(error)));
 CHECK(!result.fighter&&!result.animation&&!result.fighter_size&&!result.animation_size);p.animation_size=length;++cases;
 CHECK(fighter==original);
}
int main(int argc,char** argv)
{
 if(argc==1)synthetic();
 else {CHECK(argc==4);auto record=load(argv[1]),fighter=load(argv[2]),animation=load(argv[3]);NativeFighterProject p={};NativeScriptDiagnostic diagnostic={};
  CHECK(nativeSourceAssets_Read(record.data(),record.size(),&p.facts,&diagnostic));std::strcpy(p.fighter_root,p.facts.fighter_root);
  p.action_count=p.facts.actions;p.demo_count=p.facts.demo_count;p.fighter_dat=fighter.data();p.fighter_size=fighter.size();p.animation_dat=animation.data();p.animation_size=animation.size();exercise(p);}
 std::printf("PASS %u cases %u clips %llu checks\n",cases,clips,checks);
}
