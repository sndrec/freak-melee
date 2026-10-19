#ifndef SYSDOLPHIN_BASELIB_DAT_IMPORT_INTERNAL_H
#define SYSDOLPHIN_BASELIB_DAT_IMPORT_INTERNAL_H

#include "dat_import.h"

void* HSD_DatImportAlloc(HSD_DatImport* import, size_t size);
void HSD_DatImportSource(HSD_DatImport*, u32 offset, size_t size, u32 type);
void* HSD_DatImportObject(HSD_DatImport* import, u32 offset, u32 type, u32 count);
void* HSD_DatImportReference(HSD_DatImport* import, u32 slot, u32 type, u32 count);
char* HSD_DatImportString(HSD_DatImport* import, u32 slot);
u8* HSD_DatImportBytes(HSD_DatImport* import, u32 slot, size_t size);
void* HSD_DatImportList(HSD_DatImport* import, u32 slot, u32 type);
void HSD_DatDecodeMaterialRecord(HSD_DatImport* import,
                                const HSD_DatImportRecord* record);
void HSD_DatDecodeAnimationRecord(HSD_DatImport* import,
                                 const HSD_DatImportRecord* record);
void HSD_DatDecodeModelRecord(HSD_DatImport* import,
                             const HSD_DatImportRecord* record);
bool HSD_DatValidateGeometry(HSD_DatImport* import, HSD_PObjDesc* polygon);
u32 HSD_DatVertexElementBytes(const HSD_VtxDescList*);
bool HSD_DatValidateShapeSkin(HSD_DatImport*, HSD_PObjDesc*);
bool HSD_DatValidateEnvelopeMatrices(HSD_PObjDesc*);
bool HSD_DatValidateShape(HSD_DatImport*, HSD_ShapeSetDesc*);
void HSD_DatDecodeSceneRecord(HSD_DatImport* import,
                             const HSD_DatImportRecord* record);

#endif
