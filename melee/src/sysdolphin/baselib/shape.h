#ifndef SYSDOLPHIN_BASELIB_SHAPE_H
#define SYSDOLPHIN_BASELIB_SHAPE_H
#include "pobj.h"

// Evaluate an admitted shape graph in its local coordinate space. Positions
// need nb_vertex_index rows; optional normals need nb_normal_index rows, or
// three times that count for NBT. Uses no allocation and changes no game state.
bool HSD_ShapeSetEvaluate(const HSD_ShapeSet*, f32 (*positions)[3], f32 (*normals)[3]);
#endif
