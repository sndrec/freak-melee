#include <melee/ft/native_source_resources.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { Capacity=NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES+1024 };
static uint8_t output[Capacity],alternate[Capacity],saved[Capacity],changed[Capacity];
static uint8_t icons[8][NATIVE_SOURCE_RESOURCE_PNG_BYTES];static size_t icon_bytes[8];
static NativeSourceResources view,old;static unsigned checks,cases;static char diagnostic[256];
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"resource line %u: %s (%s)\n",__LINE__,#x,diagnostic);exit(1);}}while(0)
static uint32_t read32(const uint8_t* p){return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void word(uint8_t* p,uint32_t value){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(8*i));}
static void big(uint8_t* p,uint32_t value){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(24-8*i));}
static uint32_t crc(const uint8_t* p,size_t n)
{uint32_t value=UINT32_MAX;for(size_t i=0;i<n;++i){value^=p[i];for(unsigned b=0;b<8;++b)value=(value>>1)^((value&1)?UINT32_C(0xedb88320):0);}return value^UINT32_MAX;}
static size_t chunk(uint8_t* p,const char* type,const void* payload,size_t size)
{big(p,(uint32_t)size);memcpy(p+4,type,4);if(size)memcpy(p+8,payload,size);big(p+8+size,crc(p+4,size+4));return size+12;}
static size_t png(uint8_t* out,unsigned width,unsigned height,unsigned color,unsigned depth)
{
    const uint8_t signature[]={137,80,78,71,13,10,26,10};memcpy(out,signature,8);
    uint8_t header[13]={0};big(header,width);big(header+4,height);header[8]=(uint8_t)depth;header[9]=(uint8_t)color;
    size_t at=8;at+=chunk(out+at,"IHDR",header,13);uint8_t compressed[]={120,156,0};
    at+=chunk(out+at,"IDAT",compressed,sizeof(compressed));at+=chunk(out+at,"IEND",NULL,0);return at;
}
static void reject_read(const void* data,size_t size)
{old=view;CHECK(!nativeSourceResources_Read(data,size,&view,diagnostic,sizeof(diagnostic)));CHECK(!memcmp(&view,&old,sizeof(view)));}
static void reject_write(NativeSourceResourceInput* input,unsigned count,size_t capacity)
{
    size_t measured=123456,written=654321;memcpy(saved,output,sizeof(output));
    CHECK(!nativeSourceResources_Write(input,count,output,capacity,&written,diagnostic,sizeof(diagnostic)));
    CHECK(written==654321&&!memcmp(saved,output,sizeof(output)));
    // A too-small write destination is still a valid measurable resource set.
    if(capacity==sizeof(output)){CHECK(!nativeSourceResources_Measure(input,count,&measured,diagnostic,sizeof(diagnostic)));CHECK(measured==123456);}
}
static void names(void)
{
    const char* valid[]={"items/snack0.png","a",".hidden/blob.bin","A.B/c-2_d","dir/file.png"};
    const char* invalid[]={"","/absolute","a/","a//b",".","..","a/../b","a/./b","a\\b","C:/file","a:b",
        "a b","a\tb","a\nb","a%2fb","a?b","a#b","file.","a/CON","con.png","aux.foo","NUL", "prn", "COM1.bin","lpt9.png","a/..hidden.","a\x80"};
    for(unsigned i=0;i<sizeof(valid)/sizeof(*valid);++i)CHECK(nativeSourceResources_NameValid(valid[i]));
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);++i)CHECK(!nativeSourceResources_NameValid(invalid[i]));
    char longest[129];memset(longest,'a',127);longest[127]=0;CHECK(nativeSourceResources_NameValid(longest));longest[127]='a';longest[128]=0;CHECK(!nativeSourceResources_NameValid(longest));++cases;
}
static void roundtrip(const char* root)
{
    NativeSourceResourceInput inputs[9],permuted[9];char names[8][32];
    for(unsigned i=0;i<8;++i) {
        char path[1024];snprintf(path,sizeof(path),"%s/examples/rook_workshop/items/snack%u.png",root,i);
        FILE* file=fopen(path,"rb");CHECK(file);icon_bytes[i]=fread(icons[i],1,sizeof(icons[i]),file);CHECK(!ferror(file));fclose(file);
        uint16_t w=0,h=0;CHECK(nativeSourceResources_PNG(icons[i],icon_bytes[i],&w,&h,diagnostic,sizeof(diagnostic)));CHECK(w&&h);
        snprintf(names[i],sizeof(names[i]),"items/snack%u.png",i);inputs[i]=(NativeSourceResourceInput){names[i],icons[i],icon_bytes[i],NativeSourceResource_PNG};
    }
    const uint8_t blob[]={0,1,255,0,17};inputs[8]=(NativeSourceResourceInput){"blueprints/snack.bin",blob,sizeof(blob),NativeSourceResource_Blob};
    size_t bytes=0,measured=0;CHECK(nativeSourceResources_Measure(inputs,9,&measured,diagnostic,sizeof(diagnostic)));
    CHECK(nativeSourceResources_Write(inputs,9,output,sizeof(output),&bytes,diagnostic,sizeof(diagnostic))&&bytes==measured);
    CHECK(nativeSourceResources_Read(output,bytes,&view,diagnostic,sizeof(diagnostic))&&view.count==9&&view.bytes==output&&view.size==bytes);
    for(unsigned i=0;i<9;++i) {
        const NativeSourceResourceEntry* entry=nativeSourceResources_Find(&view,inputs[i].name,inputs[i].kind);CHECK(entry);
        CHECK(entry->size==inputs[i].size&&!memcmp(entry->bytes,inputs[i].bytes,entry->size));
        CHECK((const uint8_t*)entry->name>=output&&entry->bytes>=output&&entry->bytes+entry->size<=output+bytes);
    }
    CHECK(!nativeSourceResources_Find(&view,"Items/snack0.png",NativeSourceResource_PNG));
    CHECK(!nativeSourceResources_Find(&view,"items/snack0.png",NativeSourceResource_Blob));
    CHECK(!nativeSourceResources_Find(&view,"items/missing.png",NativeSourceResource_PNG));
    for(unsigned shift=0;shift<9;++shift) {
        for(unsigned i=0;i<9;++i)permuted[i]=inputs[(8-i+shift)%9];size_t n=0;
        CHECK(nativeSourceResources_Write(permuted,9,alternate,sizeof(alternate),&n,diagnostic,sizeof(diagnostic))&&n==bytes);
        CHECK(!memcmp(output,alternate,bytes));
    }
    for(size_t n=0;n<bytes;++n)reject_read(output,n);
    memcpy(changed,output,bytes);changed[0]='X';reject_read(changed,bytes);
    memcpy(changed,output,bytes);word(changed+4,65);reject_read(changed,bytes);
    memcpy(changed,output,bytes);word(changed+8,(uint32_t)bytes+1);reject_read(changed,bytes);
    memcpy(changed,output,bytes);changed[12+127]=1;reject_read(changed,bytes);
    memcpy(changed,output,bytes);word(changed+12+128,99);reject_read(changed,bytes);
    memcpy(changed,output,bytes);word(changed+12+132,read32(changed+12+132)+1);reject_read(changed,bytes);
    memcpy(changed,output,bytes);word(changed+12+136,UINT32_MAX);reject_read(changed,bytes);
    memcpy(changed,output,bytes);memcpy(changed+12+140,changed+12,128);reject_read(changed,bytes);
    memcpy(changed,output,bytes);memset(changed+12,0,128);memcpy(changed+12,"ITEMS/snack0.png",16);
    memset(changed+12+140,0,128);memcpy(changed+12+140,"items/snack0.png",16);reject_read(changed,bytes);
    memcpy(changed,output,bytes);memcpy(changed+12+140,"../bad",7);reject_read(changed,bytes);
    memcpy(changed,output,bytes);changed[bytes]=0;word(changed+8,(uint32_t)bytes+1);reject_read(changed,bytes+1);
    NativeSourceResourceInput duplicates[]={inputs[0],inputs[0]};reject_write(duplicates,2,sizeof(output));
    duplicates[1].name="ITEMS/snack0.png";reject_write(duplicates,2,sizeof(output));
    duplicates[1].name="a/../snack.png";reject_write(duplicates,2,sizeof(output));
    duplicates[1]=inputs[0];duplicates[1].name="other.png";duplicates[1].kind=99;reject_write(duplicates,2,sizeof(output));
    duplicates[1].kind=NativeSourceResource_PNG;duplicates[1].size=0;reject_write(duplicates,2,sizeof(output));
    reject_write(inputs,9,bytes-1);++cases;
}
static void png_rejections(void)
{
    uint8_t data[2048],original[2048];size_t n=png(data,1,1,6,8);memcpy(original,data,n);uint16_t w=99,h=77;
    // Structurally sound deliberately incomplete compressed pixels are accepted
    // here. Pixel decoding is separate; a permissive decoder is not an integrity
    // proof and this test deliberately makes no decompression-success claim.
    CHECK(nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic))&&w==1&&h==1);
    for(size_t cut=0;cut<n;++cut){w=99;h=77;CHECK(!nativeSourceResources_PNG(data,cut,&w,&h,diagnostic,sizeof(diagnostic)));CHECK(w==99&&h==77);}
    const unsigned corrupt[]={0,8,12,16,32,33,41,44,48,56};
    for(unsigned i=0;i<sizeof(corrupt)/sizeof(*corrupt);++i){memcpy(data,original,n);data[corrupt[i]]^=1;w=99;h=77;CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));CHECK(w==99&&h==77);}
    CHECK(!nativeSourceResources_PNG(data,n,NULL,&h,diagnostic,sizeof(diagnostic)));
    const unsigned invalid_dims[]={0,1025,UINT32_MAX};
    for(unsigned i=0;i<3;++i){n=png(data,invalid_dims[i],1,6,8);CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));}
    n=png(data,1024,1024,6,16);CHECK(nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic))&&w==1024&&h==1024);
    n=png(data,1,1,3,8);CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic))); // No palette.
    n=png(data,1,1,6,4);CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));
    n=png(data,1,1,1,8);CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));
    n=png(data,1,1,6,8);data[28]=1;big(data+29,crc(data+12,17));CHECK(nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic))); // Legal interlace.
    for(unsigned i=26;i<=28;++i){n=png(data,1,1,6,8);data[i]=(uint8_t)(i==28?2:1);big(data+29,crc(data+12,17));CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));}
    n=png(data,1,1,6,8);memcpy(data+n-12+4,"ABCD",4);big(data+n-4,crc(data+n-8,4));CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));
    n=png(data,1,1,6,8);data[n++]=0;CHECK(!nativeSourceResources_PNG(data,n,&w,&h,diagnostic,sizeof(diagnostic)));++cases;
}
static void png_chunk_protocol(void)
{
    uint8_t data[2048],header[25],palette[]={1,2,3,4,5,6},alpha[]={0,255},compressed[]={120,156,0};uint16_t w,h;
    png(data,1,1,3,1);memcpy(header,data+8,25);size_t at=33;
    at+=chunk(data+at,"PLTE",palette,sizeof(palette));at+=chunk(data+at,"tRNS",alpha,sizeof(alpha));
    at+=chunk(data+at,"IDAT",compressed,sizeof(compressed));at+=chunk(data+at,"IEND",NULL,0);
    CHECK(nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    // Palette size, duplication, order and transparency all have valid CRCs.
    const unsigned palettes[]={0,2,9};
    for(unsigned i=0;i<3;++i){memcpy(data+8,header,25);at=33;uint8_t colors[9]={0};at+=chunk(data+at,"PLTE",colors,palettes[i]);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));}
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"PLTE",palette,6);at+=chunk(data+at,"PLTE",palette,6);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"PLTE",palette,6);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"tRNS",alpha,2);at+=chunk(data+at,"PLTE",palette,6);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    png(data,1,1,6,8);memcpy(header,data+8,25);at=33;at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"tEXt","note",4);at+=chunk(data+at,"IEND",NULL,0);CHECK(nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"tEXt","note",4);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"tRNS",alpha,2);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"IHDR",header+8,13);at+=chunk(data+at,"IDAT",compressed,3);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));
    at=33;memcpy(data+8,header,25);at+=chunk(data+at,"IDAT",NULL,0);at+=chunk(data+at,"IEND",NULL,0);CHECK(!nativeSourceResources_PNG(data,at,&w,&h,diagnostic,sizeof(diagnostic)));++cases;
}
static void bounds_and_aliases(void)
{
    NativeSourceResourceInput entries[65];char names[65][24];uint8_t byte=1;
    for(unsigned i=0;i<65;++i){snprintf(names[i],24,"blob/%02u.bin",i);entries[i]=(NativeSourceResourceInput){names[i],&byte,1,NativeSourceResource_Blob};}
    size_t bytes=0;CHECK(nativeSourceResources_Write(entries,64,output,sizeof(output),&bytes,diagnostic,sizeof(diagnostic)));
    CHECK(nativeSourceResources_Read(output,bytes,&view,diagnostic,sizeof(diagnostic))&&view.count==64);
    reject_write(entries,65,sizeof(output));entries[0].size=NATIVE_SOURCE_RESOURCE_BLOB_BYTES+1;reject_write(entries,1,sizeof(output));
    entries[0].bytes=alternate;entries[0].size=NATIVE_SOURCE_RESOURCE_BLOB_BYTES;
    CHECK(nativeSourceResources_Write(entries,1,output,sizeof(output),&bytes,diagnostic,sizeof(diagnostic)));++cases;
    for(unsigned i=0;i<9;++i){entries[i].bytes=alternate;entries[i].size=NATIVE_SOURCE_RESOURCE_BLOB_BYTES;}
    reject_write(entries,9,sizeof(output));++cases;
    entries[1].size=NATIVE_SOURCE_RESOURCE_BLOB_BYTES-12-2*140;
    CHECK(nativeSourceResources_Write(entries,2,output,sizeof(output),&bytes,diagnostic,sizeof(diagnostic))&&bytes==NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES);
    CHECK(nativeSourceResources_Read(output,bytes,&view,diagnostic,sizeof(diagnostic))&&view.count==2);
    ++entries[1].size;reject_write(entries,2,sizeof(output));++cases;
    CHECK(nativeSourceResources_Read(NULL,0,&view,diagnostic,sizeof(diagnostic)));memset(&old,0,sizeof(old));CHECK(!memcmp(&old,&view,sizeof(view)));
    CHECK(!nativeSourceResources_Find(&view,"a",NativeSourceResource_Blob));
    CHECK(nativeSourceResources_Write(NULL,0,output,sizeof(output),&bytes,diagnostic,sizeof(diagnostic))&&bytes==12);
    CHECK(nativeSourceResources_Read(output,bytes,&view,diagnostic,sizeof(diagnostic))&&!view.count&&view.size==12);
    reject_read(NULL,12);reject_read(output,0);++cases;
    memcpy(saved,output,sizeof(output));size_t written=99;
    NativeSourceResourceInput alias_entry={"a",output,12,NativeSourceResource_Blob};
    CHECK(!nativeSourceResources_Write(&alias_entry,1,output,sizeof(output),&written,diagnostic,sizeof(diagnostic))&&written==99&&!memcmp(saved,output,sizeof(output)));
    CHECK(!nativeSourceResources_Write(NULL,0,output,sizeof(output),(size_t*)output,diagnostic,sizeof(diagnostic))&&!memcmp(saved,output,sizeof(output)));
    CHECK(!nativeSourceResources_Read(output,12,(NativeSourceResources*)output,diagnostic,sizeof(diagnostic))&&!memcmp(saved,output,sizeof(output)));
    CHECK(!nativeSourceResources_Read(output,12,&view,(char*)&view,sizeof(view)));CHECK(view.count==0&&view.size==12);
    CHECK(!nativeSourceResources_Write(NULL,0,output,sizeof(output),&written,(char*)output,sizeof(output))&&!memcmp(saved,output,sizeof(output)));++cases;
    size_t measure=77;alias_entry.size=12;CHECK(!nativeSourceResources_Measure(&alias_entry,1,&alias_entry.size,diagnostic,sizeof(diagnostic))&&alias_entry.size==12);
    CHECK(!nativeSourceResources_Measure(NULL,65,&measure,diagnostic,sizeof(diagnostic))&&measure==77);
    uint16_t dimension=123;CHECK(!nativeSourceResources_PNG(icons[0],icon_bytes[0],&dimension,&dimension,diagnostic,sizeof(diagnostic))&&dimension==123);++cases;
}
int main(int argc,char** argv)
{CHECK(argc==2);names();roundtrip(argv[1]);png_rejections();png_chunk_protocol();bounds_and_aliases();printf("{\"status\":\"PASS source resource codec\",\"checks\":%u,\"cases\":%u,\"actual_pngs\":8}\n",checks,cases);return 0;}
