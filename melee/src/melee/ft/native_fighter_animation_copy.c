#include "native_fighter_animation_copy.h"
#include <sysdolphin/baselib/dat.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum { COPY_CAPACITY = 1024 };

typedef struct CopyClip {
    const char* name;
    const uint8_t* bytes;
    uint32_t size;
    uint32_t offset;
} CopyClip;

typedef struct CopyTable {
    HSD_Dat dat;
    u32 offset;
    const char* names[COPY_CAPACITY];
    u32 sizes[COPY_CAPACITY];
} CopyTable;

typedef struct CopyPatch {
    uint32_t action;
    uint32_t name_offset;
    uint32_t clip_offset;
    uint32_t clip_size;
    const char* name;
    bool add_relocation;
} CopyPatch;

static uint32_t read32(const uint8_t* p)
{ return (uint32_t)p[0]<<24 | (uint32_t)p[1]<<16 | (uint32_t)p[2]<<8 | p[3]; }
static void write32(uint8_t* p, uint32_t x)
{ p[0]=(uint8_t)(x>>24); p[1]=(uint8_t)(x>>16); p[2]=(uint8_t)(x>>8); p[3]=(uint8_t)x; }

static int compare_slots(const void* a, const void* b)
{
    // Unsigned big-endian words sort in byte order.
    return memcmp(a, b, 4);
}

static bool open_table(const NativeFighterAnimationCopyInput* input, CopyTable* table)
{
    if (!input || !input->fighter_root || !input->action_count ||
        input->action_count > COPY_CAPACITY ||
        HSD_DatOpen(&table->dat,input->fighter_dat,input->fighter_size)!=HSD_DAT_OK)
        return false;
    u32 root, target;
    const char* external;
    if (!HSD_DatFindRoot(&table->dat,input->fighter_root,&root) ||
        !HSD_DatBytes(&table->dat,root,16) ||
        HSD_DatResolve(&table->dat,root+12,&table->offset,&external)!=HSD_DAT_REF_LOCAL ||
        !HSD_DatBytes(&table->dat,table->offset,(size_t)input->action_count*24)) return false;
    // Reject duplicate relocation slots: the new table is a set, not an
    // ambiguous list. Open has already checked targets, root/ref names and all
    // external chains. Root names must identify exactly one descriptor.
    unsigned roots=0;
    for (uint32_t i=0;i<table->dat.root_count;++i) {
        const char* name;
        if (!HSD_DatSymbol(&table->dat,false,i,&target,&name)) return false;
        roots += strcmp(name,input->fighter_root)==0;
    }
    if (roots!=1) return false;
    // Remember one bit per aligned slot while rebuilding the table.
    u8* used = calloc(table->dat.data_size / 32 + 1, 1);
    if (!used) return false;
    for (uint32_t i=0;i<table->dat.relocation_count;++i) {
        u32 a; HSD_DatRelocation(&table->dat,i,&a);
        u8 mask = (u8)(1u << ((a / 4) % 8));
        if (used[a / 32] & mask) {
            free(used);
            return false;
        }
        used[a / 32] |= mask;
    }
    free(used);
    for (uint32_t i=0;i<input->action_count;++i) {
        uint32_t at=table->offset+i*24;
        HSD_DatReferenceKind kind=HSD_DatResolve(&table->dat,at,&target,&external);
        table->names[i]=NULL;
        if (kind==HSD_DAT_REF_LOCAL) {
            const uint8_t* text=HSD_DatBytes(&table->dat,target,1);
            if (!text || !memchr(text,0,table->dat.data_size-target)) return false;
            table->names[i]=(const char*)text;
        } else if (kind!=HSD_DAT_REF_NULL) return false;
        if (!HSD_DatReadU32(&table->dat,at+8,&table->sizes[i]) ||
            (table->sizes[i] && (!table->names[i] || table->sizes[i]>INT32_MAX))) return false;
    }
    return true;
}

static bool index_bank(const void* bytes, size_t size, CopyClip* clips, uint32_t* count)
{
    if ((!bytes && size) || size>INT32_MAX) return false;
    const uint8_t* bank=bytes;
    *count=0;
    for (size_t at=0;at<size;) {
        if (bank[at]==255) { ++at; continue; }
        if (size-at<32) return false;
        uint32_t length=read32(bank+at);
        HSD_Dat dat;
        if (length<32 || length>size-at || HSD_DatOpen(&dat,bank+at,length)!=HSD_DAT_OK)
            return false;
        for (uint32_t i=0;i<dat.root_count;++i) {
            u32 root; const char* name;
            if (*count==COPY_CAPACITY || !HSD_DatSymbol(&dat,false,i,&root,&name)) return false;
            for (uint32_t j=0;j<*count;++j)
                if (!strcmp(clips[j].name,name)) return false;
            clips[(*count)++]=(CopyClip){name,bank+at,length,(uint32_t)at};
        }
        at+=length;
    }
    return true;
}

static const CopyClip* find_clip(const CopyClip* clips, uint32_t count, const char* name)
{
    for (uint32_t i=0;i<count;++i) if (!strcmp(clips[i].name,name)) return clips+i;
    return NULL;
}

static bool add_blob(const CopyClip* selected, const CopyClip* source, uint32_t source_count,
                     CopyClip* merged, uint32_t* merged_count, size_t* bank_size,
                     uint32_t* copied_blobs, uint32_t* offset)
{
    // A member can have several root names. Reuse a complete identical member
    // or append all its roots once; never introduce overlapping duplicate names.
    const CopyClip* existing=NULL;
    for (uint32_t i=0;i<source_count;++i) if (source[i].bytes==selected->bytes) {
        const CopyClip* match=find_clip(merged,*merged_count,source[i].name);
        if (match) {
            if (match->size!=selected->size || memcmp(match->bytes,selected->bytes,selected->size)) return false;
            if (existing && existing->bytes!=match->bytes) return false;
            existing=match;
        }
    }
    if (existing) {
        for (uint32_t i=0;i<source_count;++i) if (source[i].bytes==selected->bytes &&
            !find_clip(merged,*merged_count,source[i].name)) return false;
        *offset=existing->offset;
        return true;
    }
    size_t aligned=(*bank_size+31)&~(size_t)31;
    if (aligned>INT32_MAX || selected->size>(size_t)INT32_MAX-aligned) return false;
    *offset=(uint32_t)aligned;
    for (uint32_t i=0;i<source_count;++i) if (source[i].bytes==selected->bytes) {
        if (*merged_count==COPY_CAPACITY) return false;
        merged[*merged_count]=source[i]; merged[(*merged_count)++].offset=*offset;
    }
    *bank_size=aligned+selected->size;
    ++*copied_blobs;
    return true;
}

bool nativeFighterAnimationCopy_Materialize(
    const NativeFighterAnimationCopyInput* destination,
    const NativeFighterAnimationCopyInput* source,
    NativeFighterAnimationCopyOutput* output)
{
    if (!output || output->fighter_dat || output->animation_dat || output->fighter_size ||
        output->animation_size || output->filled_actions || output->copied_blobs) return false;
    // Scratch is authoring-only and bounded independently of source file sizes.
    CopyTable* tables=calloc(2,sizeof(*tables));
    CopyClip* clips=calloc(COPY_CAPACITY*2,sizeof(*clips));
    CopyPatch* patches=calloc(COPY_CAPACITY,sizeof(*patches));
    if (!tables || !clips || !patches) { free(tables); free(clips); free(patches); return false; }
    NativeFighterAnimationCopyOutput result={0};
    CopyTable* dst=tables; CopyTable* src=tables+1;
    CopyClip* merged=clips; CopyClip* originals=clips+COPY_CAPACITY;
    uint32_t merged_count, original_count;
    bool valid=open_table(destination,dst) && open_table(source,src) &&
        index_bank(destination->animation_dat,destination->animation_size,merged,&merged_count) &&
        index_bank(source->animation_dat,source->animation_size,originals,&original_count);
    if (!valid) goto done;
    // Every already populated destination action must have its physical clip.
    for (uint32_t i=0;i<destination->action_count;++i) if (dst->sizes[i]) {
        const CopyClip* clip=find_clip(merged,merged_count,dst->names[i]);
        if (!clip || clip->size!=dst->sizes[i]) goto done;
    }
    size_t data_size=dst->dat.data_size;
    size_t bank_size=destination->animation_size;
    uint32_t added_relocations=0;
    for (uint32_t i=0;i<destination->action_count && i<source->action_count;++i) {
        if (dst->sizes[i] || !src->sizes[i]) continue;
        const CopyClip* clip=find_clip(originals,original_count,src->names[i]);
        CopyPatch* patch=patches+result.filled_actions;
        if (!clip || clip->size!=src->sizes[i] ||
            !add_blob(clip,originals,original_count,merged,&merged_count,&bank_size,
                      &result.copied_blobs,&patch->clip_offset)) goto done;
        patch->action=i; patch->name=src->names[i]; patch->clip_size=clip->size;
        bool found=false;
        for (uint32_t j=0;j<destination->action_count && !found;++j)
            if (dst->names[j] && !strcmp(dst->names[j],patch->name)) {
                patch->name_offset=(uint32_t)((const uint8_t*)dst->names[j]-dst->dat.data); found=true;
            }
        for (uint32_t j=0;j<result.filled_actions && !found;++j)
            if (!strcmp(patches[j].name,patch->name)) { patch->name_offset=patches[j].name_offset; found=true; }
        if (!found) {
            size_t length=strlen(patch->name)+1;
            if (data_size>UINT32_MAX || length>UINT32_MAX-data_size) goto done;
            patch->name_offset=(uint32_t)data_size; data_size+=length;
        }
        u32 target; const char* external;
        patch->add_relocation=HSD_DatResolve(&dst->dat,dst->offset+i*24,&target,&external)==HSD_DAT_REF_NULL;
        added_relocations+=patch->add_relocation;
        ++result.filled_actions;
    }
    if (data_size>UINT32_MAX-3u) goto done;
    if (data_size!=dst->dat.data_size) data_size=(data_size+3)&~(size_t)3;
    size_t tail_size=destination->fighter_size-32-dst->dat.data_size-(size_t)dst->dat.relocation_count*4;
    size_t relocation_count=(size_t)dst->dat.relocation_count+added_relocations;
    uint64_t total64=32ull+data_size+(uint64_t)relocation_count*4+tail_size;
    if (data_size>UINT32_MAX || relocation_count>UINT32_MAX || total64>UINT32_MAX || total64>SIZE_MAX) goto done;
    size_t total=(size_t)total64;
    result.fighter_dat=malloc(total); result.animation_dat=malloc(bank_size?bank_size:1);
    if (!result.fighter_dat || !result.animation_dat) goto done;
    result.fighter_size=total; result.animation_size=bank_size;
    uint8_t* dat=result.fighter_dat; uint8_t* bank=result.animation_dat;
    memcpy(dat,destination->fighter_dat,32+dst->dat.data_size);
    memset(dat+32+dst->dat.data_size,0,data_size-dst->dat.data_size);
    write32(dat,(uint32_t)total); write32(dat+4,(uint32_t)data_size); write32(dat+8,(uint32_t)relocation_count);
    uint8_t* relocation_table=dat+32+data_size;
    memcpy(relocation_table,dst->dat.relocations,(size_t)dst->dat.relocation_count*4);
    memcpy(relocation_table+relocation_count*4,dst->dat.roots,tail_size);
    if (destination->animation_size) memcpy(bank,destination->animation_dat,destination->animation_size);
    memset(bank+destination->animation_size,255,bank_size-destination->animation_size);
    for (uint32_t i=0;i<merged_count;++i) if (merged[i].offset>=destination->animation_size) {
        bool copied=false;
        for (uint32_t j=0;j<i;++j) if (merged[j].offset==merged[i].offset) { copied=true; break; }
        if (!copied) memcpy(bank+merged[i].offset,merged[i].bytes,merged[i].size);
    }
    uint32_t added=dst->dat.relocation_count;
    for (uint32_t i=0;i<result.filled_actions;++i) {
        const CopyPatch* patch=patches+i; uint32_t at=dst->offset+patch->action*24;
        if (patch->name_offset>=dst->dat.data_size)
            memcpy(dat+32+patch->name_offset,patch->name,strlen(patch->name)+1);
        write32(dat+32+at,patch->name_offset); write32(dat+32+at+4,patch->clip_offset);
        write32(dat+32+at+8,patch->clip_size);
        if (patch->add_relocation) write32(relocation_table+(size_t)added++*4,at);
    }
    HSD_Dat checked;
    HSD_DatResult status = HSD_DatOpen(&checked, dat, total);
    if (status != HSD_DAT_OK && added_relocations) {
        // An unordered input can cross the lookup budget as new slots are
        // appended. Only the newly owned output table needs reordering.
        qsort(relocation_table, relocation_count, 4, compare_slots);
        status = HSD_DatOpen(&checked, dat, total);
    }
    if (status != HSD_DAT_OK) goto done;
    valid=true;
    *output=result; result=(NativeFighterAnimationCopyOutput){0};
done:
    free(result.fighter_dat); free(result.animation_dat);
    free(tables); free(clips); free(patches);
    return valid && output->fighter_dat!=NULL;
}

void nativeFighterAnimationCopy_Free(NativeFighterAnimationCopyOutput* output)
{
    if (!output) return;
    free(output->fighter_dat); free(output->animation_dat);
    *output=(NativeFighterAnimationCopyOutput){0};
}
