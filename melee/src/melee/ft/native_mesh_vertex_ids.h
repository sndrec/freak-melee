#ifndef MELEE_NATIVE_MESH_VERTEX_IDS_H
#define MELEE_NATIVE_MESH_VERTEX_IDS_H
#include <sysdolphin/baselib/dat.h>
#include <vector>

struct NativeAssetDat;
inline constexpr const char* NATIVE_MESH_VERTEX_IDS_ROOT = "freak_vertex_ids";
// IDs follow display-list corner order. They are scoped to a logical mesh;
// several polygons can share IDs without welding separate coincident vertices.
struct NativeMeshVertexIdRow {
    uint32_t polygon;
    std::vector<uint32_t> ids;
};
bool nativeMeshVertexIds_Read(const HSD_Dat&, std::vector<NativeMeshVertexIdRow>*);
const NativeMeshVertexIdRow* nativeMeshVertexIds_Find(
    const std::vector<NativeMeshVertexIdRow>&, uint32_t polygon);
bool nativeMeshVertexIds_Write(NativeAssetDat&, const std::vector<NativeMeshVertexIdRow>&);
// Serializes into an already allocated eight-byte header. Used by compaction
// after remapping polygon references and discarding retired polygons.
bool nativeMeshVertexIds_Store(NativeAssetDat&, uint32_t header,
                              const std::vector<NativeMeshVertexIdRow>&);
#endif
