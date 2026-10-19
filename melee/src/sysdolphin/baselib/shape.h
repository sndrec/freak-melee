#ifndef SYSDOLPHIN_BASELIB_SHAPE_H
#define SYSDOLPHIN_BASELIB_SHAPE_H
#include "pobj.h"

// Read one vector from an admitted target's index table, preserving its
// component format and fraction. Components is three for XYZ or nine for NBT.
// Index bytes is 1 or 2 for stock tables, or 4 for native wide target tables.
void HSD_ShapeSetReadVector(const HSD_VtxDescList*, const u8* indices,
                            unsigned index_bytes,unsigned vertex, unsigned components, f32 output[9]);

// Evaluate an admitted shape graph in its local coordinate space. Positions
// need nb_vertex_index rows; optional normals need nb_normal_index rows, or
// three times that count for NBT. Uses no allocation and changes no game state.
bool HSD_ShapeSetEvaluate(const HSD_ShapeSet*, f32 (*positions)[3], f32 (*normals)[3]);
#endif
