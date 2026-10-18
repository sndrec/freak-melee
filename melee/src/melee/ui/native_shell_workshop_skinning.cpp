#include "native_shell_workshop_skinning.h"
#include <sysdolphin/baselib/native_model_view.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

void nativeWorkshopSkinning_Clear(NativeWorkshopSkinStroke* stroke)
{
    stroke->active = false;
    stroke->vertices.clear(); stroke->screens.clear(); stroke->visible.clear();
    stroke->centers.clear(); stroke->indices.clear();
}

bool nativeWorkshopSkinning_Begin(NativeWorkshopSkinStroke* stroke,
                                  const NativeFighterProject* project, unsigned mesh)
{
    nativeWorkshopSkinning_Clear(stroke);
    unsigned corner_count = 0;
    const unsigned count = nativeFighterSkin_Vertices(project, mesh, nullptr, 0,
                                                       nullptr, 0, &corner_count);
    if (!count || !corner_count) return false;
    stroke->vertices.resize(count); stroke->screens.resize(count); stroke->visible.resize(count, 0);
    std::vector<unsigned> corners(corner_count);
    if (nativeFighterSkin_Vertices(project, mesh, stroke->vertices.data(), count,
                                   corners.data(), corner_count, nullptr) != count) return false;
    std::vector<Vec3> positions(corner_count);
    if (!nativeModelView_CornerPositions(mesh, positions.data(), corner_count)) return false;
    for (unsigned c = 0; c < corner_count; ++c) {
        const unsigned i = corners[c];
        if (i >= count) return false;
        if (stroke->visible[i]) continue;
        auto& screen = stroke->screens[i];
        stroke->visible[i] = nativeHome_WorkshopWorldScreen(&positions[c].x, &screen) &&
            screen.x >= 0 && screen.x <= 1 && screen.y >= 0 && screen.y <= 1;
    }
    stroke->active = true;
    return true;
}

void nativeWorkshopSkinning_Append(NativeWorkshopSkinStroke* stroke,
                                   float x, float y, float width, float height)
{
    if (!stroke->active || stroke->centers.size() == 4096 ||
        x < 0 || x > width || y < 0 || y > height) return;
    float closest_squared = 48.0f * 48.0f;
    int closest = -1;
    for (unsigned i = 0; i < stroke->vertices.size(); ++i) {
        if (!stroke->visible[i]) continue;
        const float dx = stroke->screens[i].x * width - x;
        const float dy = stroke->screens[i].y * height - y;
        const float distance = dx * dx + dy * dy;
        if (distance < closest_squared) { closest_squared = distance; closest = (int) i; }
    }
    if (closest < 0 || std::find(stroke->indices.begin(), stroke->indices.end(),
                                (unsigned) closest) != stroke->indices.end()) return;
    stroke->indices.push_back((unsigned) closest);
    NativeSkinBrushPoint point = {};
    // Projection uses the deformed surface; the weight writer uses bind space.
    std::memcpy(point.position, stroke->vertices[closest].position, sizeof(point.position));
    stroke->centers.push_back(point);
}

void nativeWorkshopSkinning_Inspect(const NativeFighterProject* project, unsigned mesh)
{
    NativeWorkshopSkinStroke query;
    const bool ready = nativeWorkshopSkinning_Begin(&query, project, mesh);
    std::fprintf(stderr, "[workshop-model-surface] mesh=%u ready=%u vertices=%zu\n",
                 mesh, ready, query.vertices.size());
    if (!ready) return;
    bool joints[512] = { true, true, true };
    for (unsigned i = 0; i < query.vertices.size(); ++i) {
        const auto& v = query.vertices[i]; const auto& p = query.screens[i];
        for (unsigned b = 0; b < v.skin.count; ++b)
            if (v.skin.joints[b] < 512) joints[v.skin.joints[b]] = true;
        std::fprintf(stderr, "[workshop-model-vertex] index=%u visible=%u screen=%.9f,%.9f bind=%.9f,%.9f,%.9f\n",
                     i, query.visible[i], p.x, p.y, v.position[0], v.position[1], v.position[2]);
    }
    for (unsigned i = 0; i < 512; ++i) {
        if (!joints[i]) continue;
        unsigned ancestor = i;
        for (unsigned depth = 0; depth < 512; ++depth) {
            unsigned parent;
            if (!nativeHome_WorkshopBoneParent(ancestor, &parent) || parent >= 512) break;
            joints[parent] = true;
            ancestor = parent;
        }
    }
    for (unsigned i = 0; i < 512; ++i) {
        if (!joints[i]) continue;
        float matrix[3][4]; NativeHomeWorkshopBoneLocal local;
        if (!nativeHome_WorkshopBoneMatrix(i, matrix) ||
            !nativeHome_WorkshopBoneLocal(i, &local)) continue;
        std::fprintf(stderr, "[workshop-model-joint] index=%u matrix=", i);
        for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c)
            std::fprintf(stderr, "%.9f,", matrix[r][c]);
        std::fprintf(stderr, " local=");
        for (unsigned a = 0; a < 3; ++a)
            std::fprintf(stderr, "%.9f,%.9f,%.9f,", local.position[a], local.rotation[a], local.scale[a]);
        std::fprintf(stderr, "\n");
    }
}
