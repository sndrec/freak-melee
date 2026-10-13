#include <melee/mp/mplib.h>
#include <melee/mp/mpcoll.h>
#include <melee/ft/ft_081B.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/fighter.h>
#include <sysdolphin/baselib/gobj.h>
#include <melee/lb/types.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/memory.h>
#include <stdio.h>
#include <stdlib.h>

static void require(bool ok, const char* message)
{
    if (!ok) { fprintf(stderr, "%s\n", message); exit(1); }
}

static void check_skeleton_ecb(void)
{
    HSD_Joint markers[6] = { 0 };
    const Vec3 positions[6] = {
        { -4, 2, 0 }, { 4, 2, 0 }, { -3, 10, 0 },
        { 3, 10, 0 }, { 0, 0, 0 }, { 0, 12, 0 },
    };
    for (int i = 0; i < 6; ++i) {
        markers[i].scale = (Vec3) { 1, 1, 1 };
        markers[i].position = positions[i];
        markers[i].next = i < 5 ? &markers[i + 1] : NULL;
    }
    HSD_Joint root = { .scale = { 1, 1, 1 }, .child = markers,
        .position = { 3, 20, 0 } };
    HSD_JObj* model = HSD_JObjLoadJoint(&root);
    require(model != NULL, "Cannot load ECB skeleton");
    HSD_JObj* joints[6];
    HSD_JObj* current = model->child;
    for (int i = 0; i < 6; ++i, current = current->next) {
        require(current != NULL, "Missing ECB skeleton marker");
        joints[i] = current;
        HSD_JObjSetupMatrix(current);
    }
    const u8 bytes[60] = { [3] = 60, [7] = 28,
        [33] = 1, [35] = 2, [37] = 3, [39] = 4, [41] = 5, [43] = 6,
        [48] = 0x40, [49] = 0x80, [52] = 0x40, [53] = 0xA0,
        [56] = 0x40, [57] = 0xC0 };
    HSD_Dat dat;
    ftData_x44_t parameters;
    require(HSD_DatOpen(&dat, bytes, sizeof(bytes)) == HSD_DAT_OK &&
        ftData_DecodeCollision(&dat, 0, &parameters) == HSD_DAT_OK,
        "Cannot decode fighter collision parameters");
    dat.data_size = 27;
    require(ftData_DecodeCollision(&dat, 0, &parameters) == HSD_DAT_DESCRIPTOR &&
        parameters.unkA == 6 && parameters.ledge_snap_height == 6,
        "Truncated fighter collision record changed output");
    dat.data_size = 28;
    FighterBone parts[7] = { 0 };
    parts[0].joint = model;
    for (int i = 0; i < 6; ++i) parts[i + 1].joint = joints[i];
    ftData fighter_data = { .x44 = &parameters };
    Fighter fighter = { .cur_pos = { 3, 20, 0 }, .x34_scale = { 1, 1, 1 },
        .parts = parts, .ft_data = &fighter_data };
    fighter.co_attrs.weight = 100;
    HSD_GObj object = { .user_data = &fighter, .classifier = HSD_GOBJ_CLASS_FIGHTER };
    ft_80081B38(&object);
    CollData body = fighter.coll_data;
    require(body.ecb_source.x10C_joint[5] == joints[5] && body.x50 == 100 &&
        body.ledge_snap_x == 4 && body.ledge_snap_y == 5 && body.ledge_snap_height == 6,
        "Original fighter collision initializer lost marker or ledge parameters");
    long available = OSCheckHeap(HSD_GetHeap());
    mpColl_LoadECB_JObj(&body, 4);
    require(body.desired_ecb.left.x == -4 && body.desired_ecb.right.x == 4 &&
        body.desired_ecb.bottom.y == 0 && body.desired_ecb.top.y == 12 &&
        body.desired_ecb.left.y == 6,
        "Joint-derived ECB did not subtract character world position");
    Vec3 raised = { 0, 18, 0 };
    HSD_JObjSetTranslateWithMtxDirty(joints[5], &raised);
    mpColl_LoadECB_JObj(&body, 4);
    require(body.desired_ecb.top.y == 18 && body.desired_ecb.left.y == 9,
        "ECB did not follow a changed skeleton pose");
    Vec3 falling = { 3, -5, 0 };
    HSD_JObjSetTranslateWithMtxDirty(model, &falling);
    body.cur_pos = falling;
    require(mpColl_800471F8(&body) && body.floor.index == 0 &&
        body.cur_pos.y == 4.0001F && body.floor.flags == 7,
        "Skeleton-derived ECB failed to land on transformed platform");
    require(OSCheckHeap(HSD_GetHeap()) == available,
        "Skeleton ECB evaluation allocated or damaged heap");
    HSD_JObjUnref(model);
    puts("Original skeleton-derived ECB pose updates and moving-platform landing passed");
}

void test_native_moving_collision(void)
{
    Vec2 vertices[] = { { -10, 0 }, { 10, 0 } };
    MapLine line = { 0, 1, -1, -1, -1, -1, CollLine_Floor, 7 };
    MapJoint joint = { .floor_count = 1, .left_bound = -10, .right_bound = 10,
        .vtx_count = 2 };
    MapCollData map = { .verts = vertices, .vert_count = 2, .lines = &line,
        .line_count = 1, .floor_count = 1, .joints = &joint, .joint_count = 1 };
    HSD_Joint child_desc = { .scale = { 1, 1, 1 } };
    HSD_Joint root_desc = { .scale = { 1, 1, 1 }, .child = &child_desc };
    HSD_JObj* model = HSD_JObjLoadJoint(&root_desc);
    require(model && model->child, "Cannot create moving collision joint");
    mpLibLoad(&map);
    mpLib_800552B0(0, model, 0);
    require(mpGetGroundCollJoint()[0].x20 == model->child,
            "Original joint binding selected the wrong joint");
    Vec3 translation = { 3, 4, 0 }, scale = { 2, 3, 1 };
    HSD_JObjSetTranslateWithMtxDirty(model->child, &translation);
    // First matrix setup may allocate; prepare it before checking steady frames.
    HSD_JObjSetupMatrix(model->child);
    long available = OSCheckHeap(HSD_GetHeap());
    mpLib_80055E9C(0);
    CollVtx* runtime = mpGetGroundCollVtx();
    require(runtime[0].pos.x == -7 && runtime[1].pos.x == 13 &&
        runtime[0].pos.y == 4 && runtime[1].pos.y == 4 &&
        runtime[0].x10 == -10 && runtime[0].x14 == 0,
        "Translated collision lost current or previous coordinates");
    HSD_JObjSetScaleWithMtxDirty(model->child, &scale);
    mpLib_80055E9C(0);
    require(runtime[0].pos.x == -17 && runtime[1].pos.x == 23 &&
        runtime[0].pos.y == 4 && runtime[0].x10 == -7 && runtime[0].x14 == 4,
        "Nonuniform joint matrix did not update original collision vertices");
    Vec3 hit, normal; int line_id; u32 flags;
    require(mpCheckFloor(20, 10, 20, -10, 0, &hit, &line_id, &flags, &normal,
        -1, -1, -1, NULL, NULL) && hit.x == 20 && hit.y == 4 && flags == 7,
        "Floor query did not use transformed stage geometry");
    HSD_JObjSetFlags(model->child, JOBJ_HIDDEN);
    mpLib_80055E9C(0);
    require(!mpCheckFloor(20, 10, 20, -10, 0, &hit, &line_id, &flags, &normal,
        -1, -1, -1, NULL, NULL), "Hidden stage joint remained collidable");
    HSD_JObjClearFlags(model->child, JOBJ_HIDDEN);
    mpLib_80055E9C(0);
    require(mpCheckFloor(20, 10, 20, -10, 0, &hit, &line_id, &flags, &normal,
        -1, -1, -1, NULL, NULL), "Visible stage joint did not restore collision");
    require(OSCheckHeap(HSD_GetHeap()) == available,
            "Moving collision update allocated or damaged the heap");
    check_skeleton_ecb();
    mpGetGroundCollJoint()[0].x20 = NULL;
    HSD_JObjUnref(model);
    puts("Original moving collision binding, transforms, visibility and floor queries passed");
}
