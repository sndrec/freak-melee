#ifndef SYSDOLPHIN_BASELIB_DAT_IMPORT_H
#define SYSDOLPHIN_BASELIB_DAT_IMPORT_H

#include "dat.h"
#include "mobj.h"
#include "aobj.h"
#include "fobj.h"
#include "jobj.h"
#include "dobj.h"
#include "pobj.h"
#include "robj.h"
#include "cobj.h"
#include "wobj.h"
#include "fog.h"
#include "lobj.h"

typedef enum HSD_DatImportType {
    DAT_MOBJ = 1, DAT_TOBJ, DAT_MATERIAL, DAT_PE, DAT_IMAGE, DAT_PALETTE,
    DAT_LOD, DAT_TEV, DAT_MATANIM_JOINT, DAT_MATANIM, DAT_TEXANIM,
    DAT_AOBJ, DAT_FOBJ, DAT_IMAGE_TABLE, DAT_PALETTE_TABLE,
    DAT_JOINT, DAT_DOBJ, DAT_POBJ, DAT_MATRIX, DAT_VTXDESC,
    DAT_ENVELOPE_TABLE, DAT_ENVELOPE, DAT_ROBJ, DAT_IKHINT,
    DAT_ANIMJOINT, DAT_ROBJ_ANIM,
    DAT_CAMERA, DAT_WOBJ, DAT_VECTOR, DAT_FOG, DAT_FOG_ADJ,
    DAT_LIGHT, DAT_LIGHT_POINT, DAT_LIGHT_SPOT, DAT_LIGHT_ATTN,
    DAT_SHAPE_JOINT, DAT_SHAPE_DOBJ, DAT_SHAPE_ANIM,
    DAT_LIGHT_ANIM, DAT_WOBJ_ANIM, DAT_LIGHT_ANIM_TABLE, DAT_SHAPE_SET,
    DAT_SLIST, DAT_SPLINE, DAT_EXP, DAT_BYTECODE_EXP, DAT_ENVELOPE_MATRICES,
    DAT_NORMAL_BASES,
} HSD_DatImportType;

typedef struct HSD_DatImportRecord {
    u32 offset;
    u32 type;
    u32 count;
    void* object;
} HSD_DatImportRecord;

typedef struct HSD_DatImport {
    const HSD_Dat* dat;
    u8* arena;
    size_t arena_size;
    size_t arena_used;
    HSD_DatImportRecord* records;
    u32* slots;
    u32* sorted_targets;
    u32 slot_count;
    u32 record_capacity;
    u32 record_count;
    u32 decoded_count;
    u32 animation_stream_bytes;
    // RGBA8-equivalent bytes for all image and palette descriptors in the graph.
    size_t decoded_texture_bytes;
    size_t texture_byte_limit;
    size_t display_list_bytes;
    size_t display_list_limit;
    u32 unresolved_external_count;
    bool allow_unresolved_externals;
    bool has_joint_bindings;
    HSD_DatResult status;
    u32 error_offset;
    const char* external_symbol;
    // Optional authoring inspection, supplied after Init. Called for complete
    // serialized records and payload ranges used by the imported graph.
    // Type is HSD_DatImportType for records, zero for raw payloads.
    void (*source_range)(void* context, u32 offset, size_t size, u32 type);
    void* source_context;
} HSD_DatImport;

// The caller owns all returned data. Arena and DAT bytes outlive all returned
// objects. A failed context must be reset before reuse; no partial root returns.
HSD_DatResult HSD_DatImportInit(HSD_DatImport* import, const HSD_Dat* dat,
                               void* arena, size_t arena_size, u32 max_records);
u32 HSD_DatImportNextTarget(const HSD_DatImport* import, u32 offset);
void* HSD_DatImportGraph(HSD_DatImport* import, u32 offset, HSD_DatImportType type);

#endif
