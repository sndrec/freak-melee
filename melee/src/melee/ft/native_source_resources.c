#include "native_source_resources.h"
#include <stdio.h>
#include <string.h>

enum { HeaderBytes=12, DirectoryBytes=140 };
static bool error(char* output,size_t capacity,const char* message)
{if(output&&capacity)snprintf(output,capacity,"%s",message);return false;}
static bool overlap(const void* a,size_t an,const void* b,size_t bn)
{
    if(!an||!bn)return false;
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return an>UINTPTR_MAX-x||bn>UINTPTR_MAX-y||(x<y+bn&&y<x+an);
}
static uint32_t le32(const uint8_t* p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t be32(const uint8_t* p)
{return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|(uint32_t)p[3];}
static void put32(uint8_t* p,uint32_t value)
{for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(i*8));}
static unsigned lower(unsigned c){return c>='A'&&c<='Z'?c+('a'-'A'):c;}
static bool letter(unsigned c){return (c>='A'&&c<='Z')||(c>='a'&&c<='z');}
static size_t length(const char* text)
{size_t n=0;if(text)while(n<NATIVE_SOURCE_RESOURCE_NAME_BYTES&&text[n])++n;return n;}
static bool alias(const char* a,const char* b)
{for(;*a&&*b;++a,++b)if(lower((unsigned char)*a)!=lower((unsigned char)*b))return false;return *a==*b;}
static bool device(const char* name,size_t count)
{
    size_t stem=0;while(stem<count&&name[stem]!='.')++stem;
    if(stem==3) {
        unsigned a=lower(name[0]),b=lower(name[1]),c=lower(name[2]);
        return (a=='c'&&b=='o'&&c=='n')||(a=='p'&&b=='r'&&c=='n')||
            (a=='a'&&b=='u'&&c=='x')||(a=='n'&&b=='u'&&c=='l');
    }
    return stem==4&&name[3]>='1'&&name[3]<='9'&&
        ((lower(name[0])=='c'&&lower(name[1])=='o'&&lower(name[2])=='m')||
         (lower(name[0])=='l'&&lower(name[1])=='p'&&lower(name[2])=='t'));
}
bool nativeSourceResources_NameValid(const char* name)
{
    size_t n=length(name);if(!n||n==NATIVE_SOURCE_RESOURCE_NAME_BYTES)return false;
    size_t first=0;
    for(size_t i=0;i<=n;++i) {
        unsigned c=(unsigned char)name[i];
        if(!c||c=='/') {
            size_t count=i-first;
            if(!count||name[i-1]=='.'||(count==1&&name[first]=='.')||
               (count==2&&name[first]=='.'&&name[first+1]=='.')||device(name+first,count))return false;
            first=i+1;
        }else if(!letter(c)&&!(c>='0'&&c<='9')&&c!='_'&&c!='-'&&c!='.')return false;
    }
    return true;
}
static uint32_t crc(const uint8_t* bytes,size_t size)
{
    static const uint32_t table[16]={0x00000000,0x1db71064,0x3b6e20c8,0x26d930ac,
        0x76dc4190,0x6b6b51f4,0x4db26158,0x5005713c,0xedb88320,0xf00f9344,
        0xd6d6a3e8,0xcb61b38c,0x9b64c2b0,0x86d3d2d4,0xa00ae278,0xbdbdf21c};
    uint32_t result=UINT32_MAX;
    for(size_t i=0;i<size;++i){result^=bytes[i];result=(result>>4)^table[result&15];result=(result>>4)^table[result&15];}
    return result^UINT32_MAX;
}
static bool depth_valid(unsigned color,unsigned depth)
{
    if(color==0)return depth==1||depth==2||depth==4||depth==8||depth==16;
    if(color==3)return depth==1||depth==2||depth==4||depth==8;
    return (color==2||color==4||color==6)&&(depth==8||depth==16);
}
static bool png(const uint8_t* data,size_t size,uint16_t* width,uint16_t* height)
{
    static const uint8_t signature[8]={137,80,78,71,13,10,26,10};
    if(!data||size<57||size>NATIVE_SOURCE_RESOURCE_PNG_BYTES||memcmp(data,signature,8))return false;
    size_t at=8;uint32_t w=0,h=0,idat_bytes=0;unsigned color=0,depth=0,palette=0;
    bool header=false,image=false,image_end=false,transparency=false;
    while(at<size) {
        if(size-at<12)return false;
        uint32_t count=be32(data+at);if(count>size-at-12)return false;
        const uint8_t* type=data+at+4;const uint8_t* payload=type+4;
        for(unsigned i=0;i<4;++i)if(!letter(type[i]))return false;
        if(type[2]<'A'||type[2]>'Z'||crc(type,(size_t)count+4)!=be32(payload+count))return false;
        if(!memcmp(type,"IHDR",4)) {
            if(header||at!=8||count!=13)return false;
            w=be32(payload);h=be32(payload+4);depth=payload[8];color=payload[9];
            if(!w||!h||w>NATIVE_SOURCE_RESOURCE_PNG_DIMENSION||h>NATIVE_SOURCE_RESOURCE_PNG_DIMENSION||
               !depth_valid(color,depth)||payload[10]||payload[11]||payload[12]>1)return false;
            header=true;
        }else if(!header)return false;
        else if(!memcmp(type,"PLTE",4)) {
            if(palette||image||transparency||!count||count%3||count>768||color==0||color==4)return false;
            palette=count/3;if(color==3&&palette>(1u<<depth))return false;
        }else if(!memcmp(type,"tRNS",4)) {
            if(transparency||image||color==4||color==6||
               (color==0&&count!=2)||(color==2&&count!=6)||
               (color==3&&(!palette||!count||count>palette)))return false;
            transparency=true;
        }else if(!memcmp(type,"IDAT",4)) {
            if(image_end||(color==3&&!palette))return false;
            image=true;idat_bytes+=count;
        }else if(!memcmp(type,"IEND",4)) {
            if(count||!image||!idat_bytes||at+12!=size)return false;
            *width=(uint16_t)w;*height=(uint16_t)h;return true;
        }else {
            if(type[0]>='A'&&type[0]<='Z')return false; // Unknown critical chunk.
            if(image)image_end=true;
        }
        at+=(size_t)count+12;
    }
    return false;
}
bool nativeSourceResources_PNG(const void* bytes,size_t size,uint16_t* width,uint16_t* height,char* d,size_t dn)
{
    if(overlap(d,dn,bytes,size)||overlap(d,dn,width,sizeof(*width))||overlap(d,dn,height,sizeof(*height)))return false;
    uint16_t w,h;
    if(!width||!height||overlap(width,sizeof(*width),height,sizeof(*height))||
       overlap(width,sizeof(*width),bytes,size)||overlap(height,sizeof(*height),bytes,size)||!png(bytes,size,&w,&h))
        return error(d,dn,"Resource PNG framing, CRC, ordering or image dimensions are invalid");
    *width=w;*height=h;return true;
}
static bool diagnostics(const NativeSourceResourceInput* input,uint32_t count,char* d,size_t dn)
{
    if(count>NATIVE_SOURCE_RESOURCE_ENTRIES)return !overlap(d,dn,input,sizeof(*input));
    if(overlap(d,dn,input,sizeof(*input)*count))return false;
    if(input)for(unsigned i=0;i<count;++i)
        if(overlap(d,dn,input[i].name,length(input[i].name)+1)||overlap(d,dn,input[i].bytes,input[i].size))return false;
    return true;
}
static bool collect(const NativeSourceResourceInput* input,uint32_t count,uint8_t order[NATIVE_SOURCE_RESOURCE_ENTRIES],
    size_t* size,char* d,size_t dn)
{
    if(count>NATIVE_SOURCE_RESOURCE_ENTRIES||(count&&!input))return error(d,dn,"Resource entry count exceeds its fixed directory or inputs are absent");
    size_t total=HeaderBytes+(size_t)DirectoryBytes*count;
    for(unsigned i=0;i<count;++i) {
        const NativeSourceResourceInput* entry=&input[i];uint16_t w,h;
        if(!nativeSourceResources_NameValid(entry->name)||!entry->bytes||!entry->size)
            return error(d,dn,"Resource requires a canonical relative name and nonempty owned bytes");
        if((entry->kind==NativeSourceResource_Blob&&entry->size>NATIVE_SOURCE_RESOURCE_BLOB_BYTES)||
           (entry->kind==NativeSourceResource_PNG&&!png(entry->bytes,entry->size,&w,&h))||
           (entry->kind!=NativeSourceResource_Blob&&entry->kind!=NativeSourceResource_PNG))
            return error(d,dn,"Resource kind, size or PNG structure is unsupported");
        if(entry->size>NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES-total)return error(d,dn,"Resource bundle exceeds its reserved byte bound");
        total+=entry->size;
        for(unsigned j=0;j<i;++j)if(alias(input[j].name,entry->name))return error(d,dn,"Duplicate resource name or case alias");
        unsigned at=i;while(at&&strcmp(input[order[at-1]].name,entry->name)>0){order[at]=order[at-1];--at;}order[at]=(uint8_t)i;
    }
    *size=total;return true;
}
bool nativeSourceResources_Measure(const NativeSourceResourceInput* input,uint32_t count,size_t* bytes,char* d,size_t dn)
{
    if(overlap(d,dn,bytes,sizeof(*bytes))||!diagnostics(input,count,d,dn))return false;
    if(!bytes||overlap(bytes,sizeof(*bytes),input,count<=NATIVE_SOURCE_RESOURCE_ENTRIES?sizeof(*input)*count:0))return error(d,dn,"Invalid resource measure output");
    if(input&&count<=NATIVE_SOURCE_RESOURCE_ENTRIES)for(unsigned i=0;i<count;++i)
        if(overlap(bytes,sizeof(*bytes),input[i].name,length(input[i].name)+1)||overlap(bytes,sizeof(*bytes),input[i].bytes,input[i].size))return error(d,dn,"Resource measure output aliases its input");
    uint8_t order[NATIVE_SOURCE_RESOURCE_ENTRIES];size_t size;if(!collect(input,count,order,&size,d,dn))return false;
    *bytes=size;return true;
}
bool nativeSourceResources_Write(const NativeSourceResourceInput* input,uint32_t count,void* output,size_t capacity,size_t* written,char* d,size_t dn)
{
    if(overlap(d,dn,output,capacity)||overlap(d,dn,written,sizeof(*written))||!diagnostics(input,count,d,dn))return false;
    if(!output||!written||overlap(output,capacity,written,sizeof(*written))||
       overlap(output,capacity,input,count<=NATIVE_SOURCE_RESOURCE_ENTRIES?sizeof(*input)*count:0)||
       overlap(written,sizeof(*written),input,count<=NATIVE_SOURCE_RESOURCE_ENTRIES?sizeof(*input)*count:0))return error(d,dn,"Invalid or overlapping resource write output");
    uint8_t order[NATIVE_SOURCE_RESOURCE_ENTRIES];size_t size;if(!collect(input,count,order,&size,d,dn))return false;
    if(size>capacity)return error(d,dn,"Resource write destination is too small");
    for(unsigned i=0;i<count;++i) {
        if(overlap(output,capacity,input[i].bytes,input[i].size)||overlap(output,capacity,input[i].name,length(input[i].name)+1)||
           overlap(written,sizeof(*written),input[i].bytes,input[i].size)||overlap(written,sizeof(*written),input[i].name,length(input[i].name)+1))return error(d,dn,"Resource write output aliases its input");
    }
    uint8_t* data=output;memcpy(data,"FMRS",4);put32(data+4,count);put32(data+8,(uint32_t)size);
    uint32_t at=HeaderBytes+DirectoryBytes*count;
    for(unsigned i=0;i<count;++i) {
        const NativeSourceResourceInput* entry=&input[order[i]];uint8_t* row=data+HeaderBytes+DirectoryBytes*i;
        memset(row,0,NATIVE_SOURCE_RESOURCE_NAME_BYTES);memcpy(row,entry->name,length(entry->name));
        put32(row+128,entry->kind);put32(row+132,at);put32(row+136,(uint32_t)entry->size);
        memcpy(data+at,entry->bytes,entry->size);at+=(uint32_t)entry->size;
    }
    *written=size;return true;
}
bool nativeSourceResources_Read(const void* bytes,size_t size,NativeSourceResources* output,char* d,size_t dn)
{
    if(overlap(d,dn,bytes,size)||overlap(d,dn,output,sizeof(*output)))return false;
    if(!output||overlap(bytes,size,output,sizeof(*output)))return error(d,dn,"Invalid or overlapping resource view destination");
    NativeSourceResources view;memset(&view,0,sizeof(view));if(!bytes&&!size){*output=view;return true;}
    if(!bytes||size<HeaderBytes||size>NATIVE_SOURCE_RESOURCE_BUNDLE_BYTES)return error(d,dn,"Resource bundle size is absent or outside bounds");
    const uint8_t* data=bytes;uint32_t count=le32(data+4);
    if(memcmp(data,"FMRS",4)||count>NATIVE_SOURCE_RESOURCE_ENTRIES||le32(data+8)!=size||
       HeaderBytes+(size_t)DirectoryBytes*count>size)return error(d,dn,"Resource bundle header/directory is not canonical");
    size_t at=HeaderBytes+(size_t)DirectoryBytes*count;
    for(unsigned i=0;i<count;++i) {
        const uint8_t* row=data+HeaderBytes+DirectoryBytes*i;const char* name=(const char*)row;size_t n=length(name);
        if(!nativeSourceResources_NameValid(name))return error(d,dn,"Resource directory name is not a canonical relative path");
        for(size_t j=n+1;j<NATIVE_SOURCE_RESOURCE_NAME_BYTES;++j)if(row[j])return error(d,dn,"Resource name padding must be zero");
        if(i&&strcmp(view.entries[i-1].name,name)>=0)return error(d,dn,"Resource directory must be strictly name-sorted");
        for(unsigned j=0;j<i;++j)if(alias(view.entries[j].name,name))return error(d,dn,"Duplicate resource directory name or case alias");
        uint32_t kind=le32(row+128),offset=le32(row+132),length=le32(row+136);uint16_t w=0,h=0;
        if(offset!=at||!length||length>size-at)return error(d,dn,"Resource payload must be tightly packed and bounded");
        if((kind==NativeSourceResource_Blob&&length>NATIVE_SOURCE_RESOURCE_BLOB_BYTES)||
           (kind==NativeSourceResource_PNG&&!png(data+at,length,&w,&h))||
           (kind!=NativeSourceResource_Blob&&kind!=NativeSourceResource_PNG))return error(d,dn,"Resource payload kind, size or PNG structure is invalid");
        view.entries[i]=(NativeSourceResourceEntry){name,data+at,length,(NativeSourceResourceKind)kind,w,h};at+=length;
    }
    if(at!=size)return error(d,dn,"Resource bundle has trailing bytes");
    view.bytes=data;view.size=size;view.count=count;*output=view;return true;
}
const NativeSourceResourceEntry* nativeSourceResources_Find(const NativeSourceResources* view,const char* name,NativeSourceResourceKind kind)
{
    if(!view||view->count>NATIVE_SOURCE_RESOURCE_ENTRIES||!nativeSourceResources_NameValid(name))return NULL;
    unsigned low=0,high=view->count;
    while(low<high){unsigned mid=low+(high-low)/2;if(strcmp(view->entries[mid].name,name)<0)low=mid+1;else high=mid;}
    return low<view->count&&!strcmp(view->entries[low].name,name)&&view->entries[low].kind==kind?&view->entries[low]:NULL;
}
