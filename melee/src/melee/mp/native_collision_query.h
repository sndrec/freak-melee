#ifndef MELEE_MP_NATIVE_COLLISION_QUERY_H
#define MELEE_MP_NATIVE_COLLISION_QUERY_H

#include "mpcoll.h"

typedef struct NativeCollisionQueryState {
    struct {
        int right[9], left[9];
        Vec3 normal;
        u8 padding[4];
    } scratch;
    int right_count, left_count;
    float max_x;
    int line_id;
    u32 flags;
    bool tiny_ecb;
    bool (*platform_callback)(Fighter_GObj*, int);
    Fighter_GObj* fighter;
    Event event;
} NativeCollisionQueryState;

extern NativeCollisionQueryState nativeCollisionQuery;

#endif
