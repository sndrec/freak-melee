#include "native_mesh_vertex_ids.h"
#include "native_asset_dat.h"

namespace {
bool reference(const HSD_Dat& dat, uint32_t slot, u32* target)
{
    const char* external;
    return HSD_DatResolve(&dat, slot, target, &external) == HSD_DAT_REF_LOCAL;
}
}

bool nativeMeshVertexIds_Read(const HSD_Dat& dat, std::vector<NativeMeshVertexIdRow>* output)
{
    if (!output) return false;
    u32 header, count, table;
    if (!HSD_DatFindRoot(&dat, NATIVE_MESH_VERTEX_IDS_ROOT, &header)) {
        output->clear(); return true;
    }
    if (!HSD_DatBytes(&dat, header, 8) || !HSD_DatReadU32(&dat, header, &count) ||
        count > dat.data_size / 12) return false;
    std::vector<NativeMeshVertexIdRow> rows;
    uint64_t total_ids = 0;
    if (count) {
        if (!reference(dat, header+4, &table) || !HSD_DatBytes(&dat, table, size_t(count)*12)) return false;
        rows.reserve(count);
        for (u32 i = 0; i < count; ++i) {
            const u32 at = table+i*12;
            u32 polygon, corners, ids;
            if (!reference(dat, at, &polygon) || !HSD_DatBytes(&dat, polygon, 24) ||
                !HSD_DatReadU32(&dat, at+4, &corners) || !corners || corners > dat.data_size/4 ||
                !reference(dat, at+8, &ids) || !HSD_DatBytes(&dat, ids, size_t(corners)*4)) return false;
            total_ids += corners;
            if (total_ids > NATIVE_CONTENT_MAX_ASSET_BYTES/4) return false;
            NativeMeshVertexIdRow row = { (uint32_t)polygon, {} };
            row.ids.resize(corners);
            for (u32 c = 0; c < corners; ++c)
                row.ids[c] = NativeAssetDat::read32(dat.data+ids+c*4);
            rows.push_back(std::move(row));
        }
        std::sort(rows.begin(), rows.end(), [](const auto& a, const auto& b) { return a.polygon < b.polygon; });
        for (size_t i = 1; i < rows.size(); ++i)
            if (rows[i-1].polygon == rows[i].polygon) return false;
    } else {
        const char* external;
        if (HSD_DatResolve(&dat, header+4, &table, &external) != HSD_DAT_REF_NULL) return false;
    }
    *output = std::move(rows); return true;
}

const NativeMeshVertexIdRow* nativeMeshVertexIds_Find(
    const std::vector<NativeMeshVertexIdRow>& rows, uint32_t polygon)
{
    auto found = std::lower_bound(rows.begin(), rows.end(), polygon,
        [](const auto& row, uint32_t at) { return row.polygon < at; });
    return found != rows.end() && found->polygon == polygon ? &*found : nullptr;
}

bool nativeMeshVertexIds_Store(NativeAssetDat& dat, uint32_t header,
                              const std::vector<NativeMeshVertexIdRow>& rows)
{
    if (header > dat.data.size() || dat.data.size()-header < 8 || rows.size() > UINT32_MAX/12) return false;
    dat.word(header, (uint32_t)rows.size());
    if (rows.empty()) { dat.link(header+4, 0); return true; }
    const uint32_t table = dat.allocate(rows.size()*12);
    if (table == UINT32_MAX) return false;
    dat.link(header+4, table);
    uint64_t total_ids = 0;
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& row = rows[i];
        if (row.ids.empty() || row.ids.size() > UINT32_MAX/4 || row.polygon > dat.data.size() ||
            dat.data.size()-row.polygon < 24) return false;
        total_ids += row.ids.size();
        if (total_ids > NATIVE_CONTENT_MAX_ASSET_BYTES/4) return false;
        const uint32_t ids = dat.allocate(row.ids.size()*4);
        if (ids == UINT32_MAX) return false;
        dat.link_local(table+(uint32_t)i*12, row.polygon);
        dat.word(table+(uint32_t)i*12+4, (uint32_t)row.ids.size());
        dat.link(table+(uint32_t)i*12+8, ids);
        for (size_t c = 0; c < row.ids.size(); ++c) dat.word(ids+(uint32_t)c*4, row.ids[c]);
    }
    return true;
}

bool nativeMeshVertexIds_Write(NativeAssetDat& dat, const std::vector<NativeMeshVertexIdRow>& rows)
{
    u32 header;
    if (!HSD_DatFindRoot(&dat.original, NATIVE_MESH_VERTEX_IDS_ROOT, &header)) {
        if (rows.empty()) return true;
        header = dat.allocate(8);
        if (header == UINT32_MAX || !dat.add_root(NATIVE_MESH_VERTEX_IDS_ROOT, header)) return false;
    }
    return nativeMeshVertexIds_Store(dat, (uint32_t)header, rows);
}
