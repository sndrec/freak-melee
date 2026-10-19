#include "dat_import_internal.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

typedef struct ShapeStream {
    const HSD_VtxDescList* attributes[26];
    unsigned widths[26];
    const u8* cursor;
    const u8* end;
    unsigned remaining;
} ShapeStream;

static bool stream_begin(ShapeStream* stream,const HSD_PObjDesc* polygon)
{
    memset(stream,0,sizeof(*stream));
    if(!polygon->verts || !polygon->display || !polygon->n_display)return false;
    stream->cursor=polygon->display;
    stream->end=polygon->display+(size_t)polygon->n_display*32;
    for(const HSD_VtxDescList* attr=polygon->verts;attr->attr!=GX_VA_NULL;++attr) {
        if(attr->attr_type==GX_NONE)continue;
        const unsigned index=attr->attr==GX_VA_NBT?GX_VA_NRM:attr->attr;
        if(index>GX_VA_TEX7 || stream->attributes[index])return false;
        stream->attributes[index]=attr;
        stream->widths[index]=attr->attr_type==GX_INDEX8?1:attr->attr_type==GX_INDEX16?2:
            HSD_DatVertexElementBytes(attr);
        if(!stream->widths[index])return false;
    }
    return stream->attributes[GX_VA_POS]!=NULL;
}

// 1 is a corner, 0 is the end, -1 is invalid input. Native weighted shapes
// triangulate once while authoring, preserving one ordered corner stream.
static int stream_next(ShapeStream* stream,const u8* values[26])
{
    memset(values,0,26*sizeof(*values));
    while(!stream->remaining) {
        if(stream->cursor==stream->end)return 0;
        const unsigned command=*stream->cursor++;
        if(command==GX_NOP)continue;
        if(command!=GX_TRIANGLES || stream->end-stream->cursor<2)return -1;
        stream->remaining=(unsigned)stream->cursor[0]<<8|stream->cursor[1];
        stream->cursor+=2;
        if(!stream->remaining || stream->remaining%3)return -1;
    }
    for(unsigned attr=0;attr<=GX_VA_TEX7;++attr) {
        if(!stream->attributes[attr])continue;
        if((size_t)(stream->end-stream->cursor)<stream->widths[attr])return -1;
        values[attr]=stream->cursor;stream->cursor+=stream->widths[attr];
    }
    --stream->remaining;
    return 1;
}

static bool formats_equal(const ShapeStream* a,const ShapeStream* b)
{
    if(b->attributes[GX_VA_PNMTXIDX]==NULL ||
       b->attributes[GX_VA_PNMTXIDX]->attr_type!=GX_DIRECT)return false;
    for(unsigned attr=GX_VA_TEX0MTXIDX;attr<=GX_VA_TEX7;++attr) {
        const HSD_VtxDescList* x=a->attributes[attr];
        const HSD_VtxDescList* y=b->attributes[attr];
        if((x==NULL)!=(y==NULL))return false;
        if(!x)continue;
        if(x->attr!=y->attr || x->attr_type!=y->attr_type || x->comp_cnt!=y->comp_cnt ||
           x->comp_type!=y->comp_type || x->frac!=y->frac || x->stride!=y->stride ||
           a->widths[attr]!=b->widths[attr])return false;
        // Positions/normals are generated. Other arrays remain the same data.
        if(attr!=GX_VA_POS && attr!=GX_VA_NRM && x->attr_type>GX_DIRECT &&
           (x->vertex!=y->vertex || x->vertex_little_endian!=y->vertex_little_endian))return false;
    }
    return true;
}

static bool bindings_valid(HSD_EnvelopeDesc** table,unsigned* count)
{
    *count=0;
    if(!table)return false;
    while(table[*count]) {
        if(*count==10)return false;
        const HSD_EnvelopeDesc* entry=table[(*count)++];
        // Native batches always include inverse binds. A sole full influence
        // is encoded as two halves instead of taking HSD's rigid shortcut.
        if(entry->weight>=1.0f)return false;
        double sum=0;unsigned influences=0;
        for(;entry->joint;++entry) {
            if(++influences>16 || !isfinite(entry->weight) || entry->weight<0)return false;
            if(!entry->joint->mtx)return false;
            for(unsigned r=0;r<3;++r)for(unsigned c=0;c<4;++c)
                if(!isfinite(entry->joint->mtx[r][c]))return false;
            sum+=entry->weight;
        }
        if(!influences || fabs(sum-1)>.01)return false;
    }
    return *count!=0;
}

static bool bindings_equal(const HSD_EnvelopeDesc* a,const HSD_EnvelopeDesc* b)
{
    for(;a->joint && b->joint;++a,++b)
        if(a->joint!=b->joint || a->weight!=b->weight)return false;
    return a->joint==b->joint;
}

static unsigned vertex_index(const HSD_VtxDescList* attr,const u8* value)
{
    return attr->attr_type==GX_INDEX8?value[0]:(unsigned)value[0]<<8|value[1];
}

bool HSD_DatValidateShapeSkin(HSD_DatImport* import,HSD_PObjDesc* parent)
{
    const HSD_ShapeSetDesc* shape=parent->u.shape_set;
    if(!shape || !shape->skin || shape->nb_vertex_index<0 ||
       shape->nb_vertex_index>HSD_MAX_SHAPE_VERTICES || shape->nb_normal_index<0 ||
       shape->nb_normal_index>HSD_MAX_SHAPE_NORMALS)return false;
    const HSD_PObjDesc* slow=shape->skin;
    const HSD_PObjDesc* fast=slow;
    while(fast && fast->next) {
        slow=slow->next;fast=fast->next->next;
        if(slow==fast)return false;
    }
    // Construction-only correspondence maps. Size them to this shape and
    // release them before admission returns; no large stack or resident map.
    const size_t entries=(size_t)shape->nb_vertex_index+shape->nb_normal_index;
    const HSD_EnvelopeDesc** positions=calloc(entries,sizeof(*positions));
    if(!positions)return false;
    const HSD_EnvelopeDesc** normals=positions+shape->nb_vertex_index;
    bool valid=false;
    ShapeStream authored;
    if(!stream_begin(&authored,parent))goto cleanup;
    unsigned batches=0,corners=0;
    for(HSD_PObjDesc* batch=shape->skin;batch;batch=batch->next) {
        if(++batches>65535 || pobj_type(batch)!=POBJ_ENVELOPE || batch->class_name ||
           !HSD_DatValidateGeometry(import,batch))goto cleanup;
        unsigned count;
        if(!bindings_valid(batch->u.envelope_p,&count))goto cleanup;
        ShapeStream generated;
        if(!stream_begin(&generated,batch) || !formats_equal(&authored,&generated))goto cleanup;
        const u8* a[26];const u8* b[26];
        int next;
        while((next=stream_next(&generated,b))>0) {
            if(stream_next(&authored,a)!=1)goto cleanup;
            const unsigned matrix=b[GX_VA_PNMTXIDX][0];
            if(matrix%3 || matrix/3>=count || (a[GX_VA_PNMTXIDX] && a[GX_VA_PNMTXIDX][0]))goto cleanup;
            const HSD_EnvelopeDesc* binding=batch->u.envelope_p[matrix/3];
            for(unsigned attr=GX_VA_TEX0MTXIDX;attr<=GX_VA_TEX7;++attr) {
                if(!a[attr])continue;
                if(attr<=GX_VA_TEX7MTXIDX) {
                    const unsigned expected=a[attr][0]==GX_IDENTITY?GX_IDENTITY:GX_TEXMTX0+matrix;
                    if((a[attr][0]!=GX_IDENTITY && a[attr][0]!=GX_TEXMTX0) || b[attr][0]!=expected)goto cleanup;
                } else if(memcmp(a[attr],b[attr],authored.widths[attr]))goto cleanup;
            }
            const unsigned position=vertex_index(authored.attributes[GX_VA_POS],a[GX_VA_POS]);
            if(position>=(unsigned)shape->nb_vertex_index)goto cleanup;
            if(positions[position] && !bindings_equal(positions[position],binding))goto cleanup;
            positions[position]=binding;
            if(a[GX_VA_NRM]) {
                const unsigned normal=vertex_index(authored.attributes[GX_VA_NRM],a[GX_VA_NRM]);
                if(normal>=(unsigned)shape->nb_normal_index)goto cleanup;
                if(normals[normal] && !bindings_equal(normals[normal],binding))goto cleanup;
                normals[normal]=binding;
            }
            ++corners;
        }
        if(next<0)goto cleanup;
    }
    const u8* unused[26];
    valid=corners!=0 && stream_next(&authored,unused)==0;
cleanup:
    free(positions);
    return valid;
}
