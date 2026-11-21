#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/ft/ft_081B.h>
#include <melee/mp/mpcoll.h>
#include <math.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftmaterial.h>
#include <melee/ft/ftparts.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <sysdolphin/baselib/mobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/aobj.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/native_render_phase.h>
#include <stdio.h>
#include <stdlib.h>

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static unsigned gather(HSD_JObj* node, HSD_JObj** joints, unsigned* depths,
                       unsigned count, unsigned depth)
{
    for (; node; node = node->next) {
        check(count < 256, "Too many fixture joints");
        joints[count] = node;
        depths[count++] = depth;
        if (!(node->flags & JOBJ_INSTANCE)) {
            count = gather(node->child, joints, depths, count, depth + 1);
        }
    }
    return count;
}

static void check_collision(Fighter* fighter, const char* path)
{
    FILE* file = fopen(path, "rb");
    check(file && fseek(file, 0, SEEK_END) == 0, "Cannot open fighter DAT");
    long size = ftell(file);
    check(size > 0 && fseek(file, 0, SEEK_SET) == 0, "Cannot size fighter DAT");
    void* bytes = malloc(size);
    check(bytes && fread(bytes, 1, size, file) == size, "Cannot read fighter DAT");
    fclose(file);
    HSD_Dat dat;
    u32 root, offset;
    const char* external;
    ftData_x44_t parameters;
    check(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
          HSD_DatFindRoot(&dat, "ftDataMario", &root) &&
          HSD_DatResolve(&dat, root + 0x44, &offset, &external) == HSD_DAT_REF_LOCAL &&
          ftData_DecodeCollision(&dat, offset, &parameters) == HSD_DAT_OK,
          "Cannot decode fighter ECB parameters");
    free(bytes);
    const s16 indices[] = { parameters.unk0, parameters.unk2, parameters.unk4,
        parameters.unk6, parameters.unk8, parameters.unkA };
    for (unsigned i = 0; i < 6; ++i) {
        check(indices[i] < ftPartsTable[0]->parts_num && fighter->parts[indices[i]].joint,
              "Collision parameter references a missing fighter part");
    }
    ftData data = { .x44 = &parameters };
    fighter->ft_data = &data;
    fighter->x34_scale = (Vec3) { 1, 1, 1 };
    HSD_JObj* model = fighter->parts[0].joint;
    Vec3 initial = model->translate;
    fighter->cur_pos = initial;
    long before = OSCheckHeap(HSD_GetHeap());
    ft_80081B38(fighter->gobj);
    CollData* body = &fighter->coll_data;
    for (unsigned i = 0; i < 6; ++i) {
        check(body->ecb_source.x10C_joint[i] == fighter->parts[indices[i]].joint,
              "Fighter ECB initializer selected the wrong bound part");
    }
    check(body->ledge_snap_x == parameters.ledge_snap_x &&
          body->ledge_snap_y == parameters.ledge_snap_y &&
          body->ledge_snap_height == parameters.ledge_snap_height,
          "Fighter ECB initializer changed ledge snap parameters");
    mpColl_LoadECB_JObj(body, 4);
    ftECB original = body->desired_ecb;
    check(isfinite(original.top.y) && isfinite(original.bottom.y) &&
          isfinite(original.left.x) && isfinite(original.right.x) &&
          original.top.y > original.bottom.y && original.right.x > original.left.x,
          "Original model produced an invalid ECB");
    Vec3 moved = { initial.x + 17, initial.y + 23, initial.z };
    HSD_JObjSetTranslateWithMtxDirty(model, &moved);
    body->cur_pos = moved;
    mpColl_LoadECB_JObj(body, 4);
    check(fabsf(body->desired_ecb.top.y - original.top.y) < 0.0001f &&
          fabsf(body->desired_ecb.bottom.y - original.bottom.y) < 0.0001f &&
          fabsf(body->desired_ecb.left.x - original.left.x) < 0.0001f &&
          fabsf(body->desired_ecb.right.x - original.right.x) < 0.0001f,
          "Moving the fighter changed its local ECB shape");
    HSD_JObjSetTranslateWithMtxDirty(model, &initial);
    fighter->cur_pos = initial;
    body->cur_pos = initial;
    mpColl_LoadECB_JObj(body, 4);
    check(OSCheckHeap(HSD_GetHeap()) == before, "Bound fighter ECB evaluation allocated");
    fighter->ft_data = NULL;
    printf("Original Mario model ECB: left %.6f right %.6f bottom %.6f top %.6f; translation invariant\n",
           original.left.x, original.right.x, original.bottom.y, original.top.y);
}

static void check_matrix_cache(HSD_JObj* model)
{
    void* object;
    u32 mark;
    HSD_PObjClearMtxMark(model, HSD_MTX_RIGID);
    HSD_PObjSetMtxMark(1, model->child, HSD_MTX_ENVELOPE);
    HSD_PObjGetMtxMark(1, &object, &mark);
    check(object == model->child && mark == HSD_MTX_ENVELOPE,
          "Matrix cache setter ignored a valid slot");
    HSD_PObjSetMtxMark(-1, NULL, 99);
    HSD_PObjSetMtxMark(2, NULL, 99);
    HSD_PObjGetMtxMark(0, &object, &mark);
    check(object == model && mark == HSD_MTX_RIGID, "Invalid cache index changed valid slot");
    HSD_JObj* previous = HSD_JObjGetCurrent();
    HSD_JObjSetCurrent(model);
    HSD_PObj polygon = { .flags = POBJ_SKIN, .u.jobj = model };
    Mtx view;
    C_MTXIdentity(view);
    static u8 commands[8192];
    for (unsigned path = 0; path < 2; ++path) {
        // A matching mode with the wrong joint must miss, as must a matching
        // joint with the wrong mode. A complete match must skip the upload.
        for (unsigned miss = 0; miss < 2; ++miss) {
            HSD_PObjClearMtxMark(miss ? model : NULL, miss ? HSD_MTX_ENVELOPE : HSD_MTX_RIGID);
            GXBeginDisplayList(commands, sizeof(commands));
            if (path) ftPartsSetupSharedVtxMtx(&polygon, view, view, 0);
            else hsdPObj.setup_mtx(&polygon, view, view, 0);
            u32 uploaded = GXEndDisplayList();
            check(uploaded > 0, "Shared matrix cache reused a mismatched entry");
            GXBeginDisplayList(commands, sizeof(commands));
            if (path) ftPartsSetupSharedVtxMtx(&polygon, view, view, 0);
            else hsdPObj.setup_mtx(&polygon, view, view, 0);
            check(GXEndDisplayList() < uploaded, "Shared matrix cache failed to retain uploaded joints");
        }
    }
    HSD_PObjClearMtxMark(NULL, 0);
    // The fighter override must also use pose-only preparation when Z scaling
    // is active. Its normal path still uploads matrices for rendering.
    Fighter render_fighter = { .x34_scale = { 1, 1, 0.5f } };
    HSD_GObj render_object = { .user_data = &render_fighter };
    HSD_GObj* previous_render_object = HSD_GObj_804D7814;
    bool previous_geometry = nativeRenderGeometryEnabled;
    HSD_GObj_804D7814 = &render_object;
    Mtx position;
    HSD_JObjSetupMatrix(model);
    ftParts_JObjMakePositionMtx(model, view, position);
    for (unsigned shared = 0; shared < 2; ++shared) {
        polygon.u.jobj = shared ? model : NULL;
        nativeRenderGeometryEnabled = false;
        HSD_PObjClearMtxMark(NULL, 0);
        GXBeginDisplayList(commands, sizeof(commands));
        ftParts_PObjSetupMtx(&polygon, view, position, 0);
        check(GXEndDisplayList() == 0,
              "Scaled fighter uploaded render matrices in pose-only mode");
        HSD_PObjGetMtxMark(0, &object, &mark);
        check(object == model && mark == HSD_MTX_RIGID,
              "Scaled fighter pose-only preparation lost its matrix mark");
        nativeRenderGeometryEnabled = true;
        HSD_PObjClearMtxMark(NULL, 0);
        GXBeginDisplayList(commands, sizeof(commands));
        ftParts_PObjSetupMtx(&polygon, view, position, 0);
        check(GXEndDisplayList() > 0,
              "Scaled fighter normal rendering omitted its matrices");
    }
    // A converted mesh can keep a texture matrix fixed while its positions
    // follow a bone. The fixed slot has no joint to prepare during rollback.
    HSD_Envelope second_half = {.jobj = model, .weight = 0.5f};
    HSD_Envelope first_half = {.next = &second_half, .jobj = model, .weight = 0.5f};
    HSD_SList position_slot = {.data = &first_half};
    HSD_SList fixed_texture_slot = {0};
    HSD_EnvelopeMatrices separate = {.positions = &position_slot, .textures = &fixed_texture_slot,
                                     .constant_mask = 1};
    HSD_PObj weighted = {.flags = POBJ_SEPARATE_ENVELOPES, .u.matrices = &separate};
    nativeRenderGeometryEnabled = false;
    GXBeginDisplayList(commands, sizeof(commands));
    ftParts_PObjSetupMtx(&weighted, view, position, 0);
    check(GXEndDisplayList() == 0, "Fixed texture slot uploaded geometry during pose-only preparation");
    nativeRenderGeometryEnabled = previous_geometry;
    HSD_GObj_804D7814 = previous_render_object;
    render_fighter.x34_scale.z = 1;
    HSD_GObj_804D7814 = &render_object;
    ftParts_JObjMakePositionMtx(model, view, position);
    HSD_GObj_804D7814 = previous_render_object;
    HSD_PObjClearMtxMark(NULL, 0);
    HSD_JObjSetCurrent(previous);
    puts("Original HSD and fighter matrix caches distinguish joint identity and matrix mode");
}

void test_native_fighter_parts(HSD_JObj* model, const char* common_path,
                               const char* fighter_path)
{
    HSD_JObj* joints[256];
    unsigned depths[256];
    unsigned count = gather(model, joints, depths, 0, 0);
    u8 joint_map[256], part_map[FT_COMMON_PART_COUNT];
    FighterPartsTable table = { 0 };
    table.parts_num = count;
    struct Fighter_804D6540_t omitted = { 0 };
    struct Fighter_804D6540_x0_t records[32];
    if (common_path) {
        FILE* file = fopen(common_path, "rb");
        check(file && fseek(file, 0, SEEK_END) == 0, "Cannot open common fighter DAT");
        long size = ftell(file);
        check(size > 0 && fseek(file, 0, SEEK_SET) == 0, "Cannot size common fighter DAT");
        void* bytes = malloc(size);
        check(bytes && fread(bytes, 1, size, file) == size, "Cannot read common fighter DAT");
        fclose(file);
        HSD_Dat dat;
        u32 root, tables, descriptor, omissions;
        const char* external;
        check(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
              HSD_DatFindRoot(&dat, "ftLoadCommonData", &root) &&
              HSD_DatResolve(&dat, root + 16, &tables, &external) == HSD_DAT_REF_LOCAL &&
              HSD_DatResolve(&dat, tables, &descriptor, &external) == HSD_DAT_REF_LOCAL &&
              ftData_DecodeParts(&dat, descriptor, &table, joint_map, 256, part_map) == HSD_DAT_OK &&
              HSD_DatResolve(&dat, root + 20, &omissions, &external) == HSD_DAT_REF_LOCAL,
              "Cannot decode Mario part table");
        HSD_DatReferenceKind ref = HSD_DatResolve(&dat, omissions, &descriptor, &external);
        check(ref == HSD_DAT_REF_NULL || (ref == HSD_DAT_REF_LOCAL &&
              ftData_DecodePartOmissions(&dat, descriptor, &omitted, records, 32,
                  table.parts_num) == HSD_DAT_OK), "Cannot decode Mario omissions");
        free(bytes);
    }
    FighterPartsTable* tables[] = { &table };
    struct Fighter_804D6540_t* omissions[] = { &omitted };
    FighterPartsTable** old_tables = ftPartsTable;
    struct Fighter_804D6540_t** old_omissions = Fighter_804D6540;
    ftPartsTable = tables;
    Fighter_804D6540 = omissions;
    Fighter fighter = { 0 };
    FighterBone bones[256] = { 0 };
    HSD_DObj* draws[124] = { 0 };
    HSD_GObj owner = { 0 };
    owner.hsd_obj = model;
    owner.user_data = &fighter;
    fighter.gobj = &owner;
    fighter.parts = bones;
    fighter.dobj_list.data = draws;
    long before = OSCheckHeap(HSD_GetHeap());
    ftParts_SetupParts(&owner);
    check(OSCheckHeap(HSD_GetHeap()) == before, "Part binding unexpectedly allocated");
    unsigned joint = 0, draw = 0;
    for (unsigned part = 0; part < table.parts_num; ++part) {
        if (ftParts_8007506C(0, part)) {
            check(!bones[part].joint, "Omitted fighter part received a joint");
            continue;
        }
        check(joint < count && bones[part].joint == joints[joint] &&
              bones[part].xC == depths[joint] && bones[part].flags_b1,
              "Fighter part joint order or tree depth changed");
        u32 flags = joints[joint]->flags;
        check(bones[part].flags_b7 == !!(flags & JOBJ_LIGHTING) &&
              bones[part].flags2_b0 == !!(flags & JOBJ_TEXGEN) &&
              bones[part].flags2_b1 == !!(flags & JOBJ_SPECULAR),
              "Fighter part rendering flags changed");
        unsigned start = draw;
        for (HSD_DObj* obj = joints[joint]->u.dobj; obj; obj = obj->next) {
            check(draw < 124 && draws[draw++] == obj, "Fighter draw-object order changed");
            check(!obj->mobj || HSD_MOBJ_METHOD(obj->mobj)->setup == ftMaterial_800BF2B8,
                  "Part setup did not install fighter material class");
        }
        check(bones[part].flags2_b6 == (draw != start) &&
              bones[part].xD == (draw ? draw - 1 : 0), "Fighter part draw range changed");
        ++joint;
    }
    check(joint == count && fighter.dobj_list.count == draw, "Fighter part binding lost objects");
    check(!model->aobj, "Playback fixture expected an unanimated root");
    model->aobj = HSD_AObjAlloc();
    HSD_AObjSetRate(model->aobj, 0.5f);
    HSD_AObjSetEndFrame(model->aobj, 20);
    ftAnim_80070710(model, 2);
    long before_tick = OSCheckHeap(HSD_GetHeap());
    ftAnim_8006E9B4(&owner);
    check(model->aobj->curr_frame == 2 && fighter.cur_anim_frame == 2, "First fighter tick advanced past requested frame");
    ftAnim_8006E9B4(&owner);
    check(model->aobj->curr_frame == 2.5f && fighter.cur_anim_frame == 2.5f,
          "Fighter tick ignored playback rate or current frame");
    fighter.anim_id = -1;
    ftAnim_8006E9B4(&owner);
    check(model->aobj->curr_frame == 2.5f, "Disabled fighter animation advanced");
    fighter.anim_id = 0;
    bones[0].flags_b5 = true;
    ftAnim_8006E9B4(&owner);
    check(model->aobj->curr_frame == 2.5f, "Excluded fighter part advanced animation");
    bones[0].flags_b5 = false;
    check(OSCheckHeap(HSD_GetHeap()) == before_tick, "Fighter animation traversal allocated");
    ftAnim_80070758(model);
    check(!model->aobj && ftAnim_8006F3DC(&owner) == 0,
          "Fighter animation removal retained tracks or an undefined frame");
    Vec3 saved_position = model->translate;
    Vec3 motion = { 4, 6, 8 };
    HSD_JObjSetTranslateWithMtxDirty(model, &motion);
    fighter.x34_scale.y = 1;
    fighter.co_attrs.model_scaling = 2;
    ftAnim_8006E054(&fighter, model, model, NULL);
    check(fighter.x68C_transNPos.x == 8 && fighter.x68C_transNPos.y == 12 &&
          fighter.x68C_transNPos.z == 16 && fighter.x6A4_transNOffset.x == 8 &&
          model->translate.x == 0 && model->translate.y == 0 && model->translate.z == 0,
          "Original root motion extraction lost scale, displacement, or pose reset");
    HSD_JObjSetTranslateWithMtxDirty(model, &saved_position);
    puts("Original fighter frame bookkeeping, disabled state and root motion extraction passed");
    puts("Original fighter frame requests, rate, exclusions and animation removal passed");
    check_matrix_cache(model);
    if (fighter_path) check_collision(&fighter, fighter_path);
    ftPartsTable = old_tables;
    Fighter_804D6540 = old_omissions;
    printf("Original fighter part setup bound %u joints and %u draw objects%s\n",
           count, draw, common_path ? " using decoded PlCo.dat" : "");
}

void test_native_fighter_matrices(HSD_JObj* joint)
{
    Fighter fighter = { 0 };
    HSD_GObj owner = { .user_data = &fighter };
    HSD_GObj* previous = HSD_GObj_804D7814;
    HSD_JObj* previous_joint = HSD_JObjGetCurrent();
    HSD_GObj_804D7814 = &owner;
    HSD_JObjSetCurrent(joint);
    ftParts_80073758(joint);
    check(HSD_JOBJ_METHOD(joint)->make_pmtx == ftParts_JObjMakePositionMtx,
          "Fighter joint class installation failed");
    Mtx view, position;
    C_MTXIdentity(view);
    static u8 commands[65536];
    long before = OSCheckHeap(HSD_GetHeap());
    for (unsigned variant = 0; variant < 2; ++variant) {
        fighter.x34_scale = (Vec3) { 1, 1, variant ? 2 : 1 };
        HSD_JOBJ_METHOD(joint)->make_pmtx(joint, view, position);
        for (unsigned row = 0; row < 3; ++row) {
            for (unsigned col = 0; col < 4; ++col) {
                check(isfinite(position[row][col]), "Nonfinite fighter position matrix");
            }
        }
        for (HSD_DObj* draw = joint->u.dobj; draw; draw = draw->next) {
            for (HSD_PObj* polygon = draw->pobj; polygon; polygon = polygon->next) {
                check(HSD_POBJ_METHOD(polygon)->setup_mtx == ftParts_PObjSetupMtx,
                      "Model did not load the original fighter polygon class");
                HSD_PObjClearMtxMark(NULL, 0);
                GXBeginDisplayList(commands, sizeof(commands));
                HSD_POBJ_METHOD(polygon)->setup_mtx(polygon, view, position,
                    draw->mobj ? draw->mobj->rendermode : 0);
                check(GXEndDisplayList() > 0, "Fighter polygon matrix setup emitted no commands");
            }
        }
    }
    check(OSCheckHeap(HSD_GetHeap()) == before, "Fighter matrix setup allocated");
    HSD_GObj_804D7814 = previous;
    HSD_JObjSetCurrent(previous_joint);
}

void test_native_fighter_interpolation(HSD_JObj* model, HSD_Joint* descriptor)
{
    HSD_JObj* original[256];
    unsigned depths[256];
    unsigned count = gather(model, original, depths, 0, 0);
    FighterPartsTable table = { .parts_num = count };
    FighterPartsTable* tables[] = { &table };
    struct Fighter_804D6540_t* omissions[] = { NULL };
    FighterPartsTable** previous_tables = ftPartsTable;
    struct Fighter_804D6540_t** previous_omissions = Fighter_804D6540;
    ftPartsTable = tables;
    Fighter_804D6540 = omissions;
    FighterBone parts[256] = { 0 };
    Fighter fighter = { .parts = parts, .x108_costume_joint = descriptor };
    HSD_GObj object = { .user_data = &fighter };
    ftAnim_8006FE48(&object);
    HSD_JObj* animation[256];
    unsigned animation_depths[256];
    check(fighter.x8AC_animSkeleton &&
          gather(fighter.x8AC_animSkeleton, animation, animation_depths, 0, 0) == count &&
          fighter.x8A4_animBlendFrames == 0 && fighter.x8A8_anim_frame == 0,
          "Original interpolation skeleton initialization failed");
    for (unsigned i = 0; i < count; ++i) {
        check(parts[i].x4_jobj2 == animation[i] && animation[i] != original[i] &&
              animation_depths[i] == depths[i] && animation[i]->u.dobj == NULL,
              "Interpolation skeleton duplicated geometry or changed joint mapping");
    }
    Vec3 saved = original[0]->translate;
    Vec3 moved = { saved.x + 1, saved.y + 2, saved.z + 3 };
    HSD_JObjSetTranslateWithMtxDirty(animation[0], &moved);
    check(original[0]->translate.x == saved.x && original[0]->translate.y == saved.y &&
          original[0]->translate.z == saved.z, "Interpolation pose modified render skeleton");
    for (unsigned i = 0; i < count; ++i) {
        parts[i].joint = original[i];
        parts[i].flags_b1 = true;
    }
    long before_blend = OSCheckHeap(HSD_GetHeap());
    ftAnim_8006FE9C(&fighter, 0, 0.25f, 0.75f);
    check(fabsf(original[0]->translate.x - (saved.x + 0.25f)) < 0.0001f &&
          fabsf(original[0]->translate.y - (saved.y + 0.5f)) < 0.0001f &&
          fabsf(original[0]->translate.z - (saved.z + 0.75f)) < 0.0001f,
          "Original fighter pose blending changed translation weights");
    parts[0].flags_b0 = true;
    ftAnim_8006FF74(&fighter, 0);
    check(fabsf(original[0]->translate.x - (saved.x + 0.25f)) < 0.0001f,
          "Pose copy overwrote an excluded fighter part");
    parts[0].flags_b0 = false;
    parts[0].flags_b4 = true;
    ftAnim_8006FE9C(&fighter, 0, 0.25f, 0.75f);
    check(original[0]->translate.x == moved.x && original[0]->translate.y == moved.y,
          "Direct-copy fighter part incorrectly blended");
    parts[0].flags_b4 = false;
    HSD_JObjClearFlags(original[0], JOBJ_USE_QUATERNION);
    HSD_JObjClearFlags(animation[0], JOBJ_USE_QUATERNION);
    original[0]->rotate = (Quaternion) { 0, 0, 0, 0 };
    animation[0]->rotate = (Quaternion) { 0, 0, 1.57079632679f, 0 };
    ftAnim_8006FE9C(&fighter, 0, 0.5f, 0.5f);
    check((original[0]->flags & JOBJ_USE_QUATERNION) &&
          fabsf(original[0]->rotate.z - 0.38268343f) < 0.0001f &&
          fabsf(original[0]->rotate.w - 0.92387953f) < 0.0001f,
          "Original quaternion blend did not produce a half-angle rotation");
    ftAnim_8006FF74(&fighter, 0);
    check(!(original[0]->flags & JOBJ_USE_QUATERNION) &&
          original[0]->rotate.z == animation[0]->rotate.z &&
          (original[0]->flags & JOBJ_MTX_DIRTY), "Pose copy lost rotation format or dirty state");
    check(OSCheckHeap(HSD_GetHeap()) == before_blend, "Fighter pose blending allocated");
    puts("Original fighter pose blending, exclusion, direct copy and quaternion interpolation passed");
    HSD_JObjUnref(fighter.x8AC_animSkeleton);
    ftPartsTable = previous_tables;
    Fighter_804D6540 = previous_omissions;
    check(OSCheckHeap(HSD_GetHeap()) >= 0, "Interpolation lifecycle damaged heap");
    printf("Original fighter interpolation initialization created %u independent joints without draw objects\n", count);
}
