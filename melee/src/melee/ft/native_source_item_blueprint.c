#include "native_source_item_blueprint.h"
#include "native_source_resources.h"
#include "native_script_ir.h"
#include <melee/it/native_item_model_admission.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool error(NativeScriptDiagnostic* d,const char* text)
{if(d)snprintf(d->message,sizeof(d->message),"%s",text);return false;}
static uint32_t word(const uint8_t* p)
{return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void write_word(uint8_t* p,uint32_t value)
{for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(value>>(8*i));}
static bool independent_buffers(const void* a,size_t an,const void* b,size_t bn)
{uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an<=UINTPTR_MAX-x&&bn<=UINTPTR_MAX-y&&(x+an<=y||y+bn<=x);}
bool nativeSourceItemBlueprint_Write(const void* bytes,size_t size,uint32_t offset,
    void* output,size_t capacity,size_t* written,NativeScriptDiagnostic* d)
{
    HSD_Dat dat;
    if(!bytes||!output||!written||size<32||size>NATIVE_SOURCE_RESOURCE_BLOB_BYTES-NATIVE_SOURCE_BLUEPRINT_HEADER||
       capacity<NATIVE_SOURCE_BLUEPRINT_HEADER+size||!independent_buffers(bytes,size,output,capacity)||
       !independent_buffers(output,capacity,written,sizeof(*written))||
       HSD_DatOpen(&dat,bytes,size)!=HSD_DAT_OK||!itArticle_IsDescriptor(&dat,offset,false))
        return error(d,"Item blueprint needs a bounded original DAT and an actual Article descriptor");
    uint8_t* destination=output;memcpy(destination,"FMIB",4);write_word(destination+4,(uint32_t)size);
    write_word(destination+8,offset);write_word(destination+12,0);
    memcpy(destination+NATIVE_SOURCE_BLUEPRINT_HEADER,bytes,size);*written=NATIVE_SOURCE_BLUEPRINT_HEADER+size;return true;
}
void nativeSourceItemBlueprint_Release(NativeSourceItemBlueprint* blueprint)
{
    if(!blueprint)return;free(blueprint->arena);memset(blueprint,0,sizeof(*blueprint));
}
bool nativeSourceItemBlueprint_Import(const void* bytes,size_t size,NativeSourceItemBlueprint* output,NativeScriptDiagnostic* d)
{
    if(!output)return false;
    const uint8_t* vacancy=(const uint8_t*)output;
    for(size_t i=0;i<sizeof(*output);++i)if(vacancy[i])return error(d,"Item blueprint owner is not vacant");
    const uint8_t* p=bytes;
    if(!p||size<NATIVE_SOURCE_BLUEPRINT_HEADER+32||size>NATIVE_SOURCE_RESOURCE_BLOB_BYTES||
       !independent_buffers(p,size,output,sizeof(*output))||memcmp(p,"FMIB",4)||
       word(p+4)!=size-NATIVE_SOURCE_BLUEPRINT_HEADER||word(p+12))
        return error(d,"Item blueprint envelope is malformed");
    output->address=output;output->bytes=p+NATIVE_SOURCE_BLUEPRINT_HEADER;output->size=size-NATIVE_SOURCE_BLUEPRINT_HEADER;
    output->arena=malloc(NATIVE_SOURCE_BLUEPRINT_ARENA);
    if(!output->arena||HSD_DatOpen(&output->dat,output->bytes,output->size)!=HSD_DAT_OK||
       HSD_DatImportInit(&output->import,&output->dat,output->arena,NATIVE_SOURCE_BLUEPRINT_ARENA,4096)!=HSD_DAT_OK)goto failure;
    ItScriptEntry entries[256];ItScriptCollect scripts={entries,0,256};
    output->article=itArticle_Import(&output->import,word(p+8),&output->stats,&scripts,false,It_Kind_NativeHoldable,NULL,0);
    if(!output->article||!output->article->x0_common_attr||!output->article->x10_modelDesc||
       !output->article->x10_modelDesc->x0_joint||output->import.status!=HSD_DAT_OK||
       !itCommandData_Import(&output->import,&scripts,&output->commands))goto failure;
    return true;
failure:
    nativeSourceItemBlueprint_Release(output);return error(d,"Item blueprint Article/model/command graph import failed");
}
bool nativeSourceItemBlueprint_Validate(const void* bytes,size_t size,NativeScriptDiagnostic* diagnostic)
{
    NativeSourceItemBlueprint* owner=calloc(1,sizeof(*owner));
    if(!owner)return error(diagnostic,"Could not allocate the item blueprint admission owner");
    bool valid=nativeSourceItemBlueprint_Import(bytes,size,owner,diagnostic);
    if(valid && owner->article->x10_modelDesc->x4_bone_count) {
        u32 populated;
        valid=nativeItemModel_Admit(&owner->import,owner->article->x10_modelDesc,0,&populated);
        if(!valid)error(diagnostic,"Item blueprint model exceeds native indexed graph topology or capacity");
    }
    nativeSourceItemBlueprint_Release(owner);free(owner);return valid;
}
