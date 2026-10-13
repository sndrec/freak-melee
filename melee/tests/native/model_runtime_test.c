#include <sysdolphin/baselib/video.h>
#include <sysdolphin/baselib/dat_import.h>
#include <sysdolphin/baselib/id.h>
#include <sysdolphin/baselib/memory.h>
#include <sysdolphin/baselib/object.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <sysdolphin/baselib/sislib.h>
#include <sysdolphin/baselib/tobj.h>
#include <dolphin/gx.h>
#include <melee/ft/ftparts.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static unsigned joint_count, polygon_count, weight_count, material_count;
static unsigned texture_resources, palette_resources;

static u32 texture_identity(GXTexObj* texture)
{
    u8 commands[2048];
    GXBeginDisplayList(commands, sizeof(commands));
    GXLoadTexObj(texture, GX_TEXMAP0);
    u32 length = GXEndDisplayList();
    for (u32 offset = 0; offset < length;) {
        if (commands[offset] == 0x61) { offset += 5; continue; }
        require(length - offset >= 37 && commands[offset] == 0x50 &&
                    commands[offset + 1] == 0 && commands[offset + 2] == 0x30,
                "Unexpected command before Aurora texture metadata");
        const u8* id = commands + offset + 29;
        return (u32) id[0] << 24 | (u32) id[1] << 16 | (u32) id[2] << 8 | id[3];
    }
    require(false, "Missing Aurora texture identity");
    return 0;
}

static void check_material_reuse(HSD_TObj* head)
{
    GXTexObj textures[8];
    GXTlutObj palettes[8];
    HSD_TObj* objects[8];
    unsigned count = 0;
    static u8 commands[65536];
    for (HSD_TObj* obj = head; obj; obj = obj->next) {
        if (!obj->native_texture_ready) continue;
        require(count < 8, "Material exceeded GX texture resource count");
        objects[count] = obj;
        textures[count] = obj->native_texture;
        palettes[count] = obj->native_palette;
        ++count;
        ++texture_resources;
        palette_resources += obj->native_palette_ready;
    }
    for (unsigned repeat = 0; repeat < 3; ++repeat) {
        GXBeginDisplayList(commands, sizeof(commands));
        HSD_TObjSetup(head);
        GXEndDisplayList();
        for (unsigned i = 0; i < count; ++i) {
            require(memcmp(&textures[i], &objects[i]->native_texture, sizeof(GXTexObj)) == 0 &&
                        memcmp(&palettes[i], &objects[i]->native_palette, sizeof(GXTlutObj)) == 0,
                    "Unchanged material setup recreated or mutated native texture resources");
        }
    }
}

static void check_texture_objects(void)
{
    struct { u64 before; GXTexObj object; u64 after[8]; } texture;
    struct { u64 before; GXTlutObj object; u64 after[8]; } palette;
    u8 pixels[32] = { 0 }, colors[32] = { 0 }, commands[2048] = { 0 };
    memset(&texture, 0xA5, sizeof(texture));
    memset(&palette, 0xA5, sizeof(palette));
    GXInitTexObjCI(&texture.object, pixels, 8, 8, GX_TF_C4,
                   GX_REPEAT, GX_CLAMP, GX_FALSE, GX_TLUT0);
    GXInitTexObjLOD(&texture.object, GX_LINEAR, GX_LINEAR, 0, 0, 0,
                    GX_FALSE, GX_FALSE, GX_ANISO_1);
    GXInitTexObjUserData(&texture.object, pixels);
    GXInitTlutObj(&palette.object, colors, GX_TL_RGB5A3, 16);
    require(texture.before == 0xA5A5A5A5A5A5A5A5ULL &&
                palette.before == 0xA5A5A5A5A5A5A5A5ULL,
            "Texture initialization overwrote preceding memory");
    for (unsigned i = 0; i < 8; ++i) {
        require(texture.after[i] == 0xA5A5A5A5A5A5A5A5ULL &&
                    palette.after[i] == 0xA5A5A5A5A5A5A5A5ULL,
                "Native texture or palette object is undersized");
    }
    require(GXGetTexObjUserData(&texture.object) == pixels,
            "Texture user-data pointer lost native precision");
    GXBeginDisplayList(commands, sizeof(commands));
    GXLoadTlut(&palette.object, GX_TLUT0);
    GXLoadTexObj(&texture.object, GX_TEXMAP0);
    require(GXEndDisplayList() > 0, "Texture commands were not recorded");
    puts("Native texture and palette object storage passed");
}

static void check_vertex_submission(void)
{
    u8 commands[128] = { 0 };
    f32 vertices[3] = { 1, 2, 3 };
    GXBeginDisplayList(commands, sizeof(commands));
    GXSetArray(GX_VA_POS, vertices, sizeof(vertices), sizeof(vertices), true);
    GXParam1f32(1.0f);
    u32 size = GXEndDisplayList();
    require(size == 32 && commands[0] == 0x50 && commands[1] == 0 && commands[2] == 0x10,
            "Aurora vertex-array command was not recorded");
    uintptr_t address = 0;
    for (unsigned i = 3; i < 11; ++i) address = address << 8 | commands[i];
    require(address == (uintptr_t) vertices && commands[11] == 0 &&
                commands[12] == 0 && commands[13] == 0 && commands[14] == 12 && commands[15] == 1,
            "Vertex-array pointer, size, or byte-order ABI is incorrect");
    require(commands[16] == 8 && commands[17] == 0xB0 && commands[21] == 12 &&
                commands[22] == 0x3F && commands[23] == 0x80 && commands[24] == 0 && commands[25] == 0,
            "Vertex stride or native float command encoding is incorrect");
    puts("Aurora vertex-array ABI and FIFO float submission passed");
}

void test_native_fighter_material_setup(HSD_MObj* material);
void test_native_fighter_matrices(HSD_JObj* joint);
void test_native_fighter_interpolation(HSD_JObj*, HSD_Joint*);

static void check_model(HSD_JObj* joint, HSD_Joint* desc)
{
    while (desc) {
        require(joint && joint->id == (uintptr_t) desc,
                "Runtime skeleton differs from imported descriptor tree");
        require(HSD_IDGetData((uintptr_t) desc, NULL) == joint,
                "Runtime joint is missing from the reference table");
        ++joint_count;
        HSD_JObjSetupMatrix(joint);
        test_native_fighter_matrices(joint);
        for (unsigned row = 0; row < 3; ++row) {
            for (unsigned col = 0; col < 4; ++col) {
                require(isfinite(joint->mtx[row][col]), "Nonfinite joint matrix");
            }
        }
        require(union_type_dobj(joint), "This model test requires ordinary geometry joints");
        for (HSD_DObj* draw = joint->u.dobj; draw; draw = draw->next) {
            if (draw->mobj && draw->mobj->tobj) {
                static u8 material_commands[65536];
                GXBeginDisplayList(material_commands, sizeof(material_commands));
                HSD_TObjSetup(draw->mobj->tobj);
                require(GXEndDisplayList() > 0, "Model texture setup emitted no commands");
                check_material_reuse(draw->mobj->tobj);
                test_native_fighter_material_setup(draw->mobj);
                ++material_count;
            }
            for (HSD_PObj* polygon = draw->pobj; polygon; polygon = polygon->next) {
                ++polygon_count;
                if (pobj_type(polygon) == POBJ_ENVELOPE) {
                    for (HSD_SList* list = polygon->u.envelope_list; list; list = list->next) {
                        for (HSD_Envelope* env = list->data; env; env = env->next) {
                            require(env->jobj && HSD_IDGetData(env->jobj->id, NULL) == env->jobj,
                                    "Skinning weight has an unresolved joint");
                            require(isfinite(env->weight), "Nonfinite skinning weight");
                            ++weight_count;
                        }
                    }
                }
            }
        }
        check_model(joint->child, desc->child);
        joint = joint->next;
        desc = desc->next;
    }
    require(joint == NULL, "Unexpected extra runtime joint");
}

void test_native_figatree(void);
void test_native_gobj(void);
void test_native_moving_collision(void);
void test_native_fighter_material(void);
void test_native_fighter_parts(HSD_JObj*, const char*, const char*);
void test_native_article_model(void);

static void check_native_video_init(void)
{
    HSD_VIStatus config = { .rmode = GXNtsc480IntDf, .black = false,
        .vf = true, .gamma = GX_GM_1_0, .clear_clr = { 12, 34, 56, 255 },
        .clear_z = GX_MAX_Z24, .update_clr = true, .update_z = true };
    memset(&HSD_VIData, 0xA5, sizeof(HSD_VIData));
    HSD_VIInit(&config);
    require(HSD_VIGetNbXFB() == 0 && !HSD_VIData.drawdone.waiting &&
        !HSD_VIData.pre_cb && !HSD_VIData.post_cb && !HSD_VIData.drawdone.cb,
        "Native video initialization retained console queues or callbacks");
    require(HSD_VIGetRenderMode()->fbWidth == 640 &&
        HSD_VIGetRenderMode()->efbHeight == 480 &&
        HSD_VIGetVIStatus()->clear_clr.g == 34 &&
        HSD_VIData.efb.vi_all.vi.clear_z == GX_MAX_Z24,
        "Native video initialization lost original render settings");
    for (int i = 0; i < HSD_VI_XFB_MAX; ++i) {
        require(!HSD_VIData.xfb[i].buffer && HSD_VIData.xfb[i].status == HSD_VI_XFB_NONE,
            "Native video initialization retained a CPU framebuffer");
    }
    GXRenderModeObj changed = GXNtsc480Int;
    HSD_VISetConfigure(&changed);
    require(memcmp(HSD_VIGetRenderMode(), &changed, sizeof(changed)) == 0 &&
        HSD_VIGetVIStatus()->clear_clr.g == 34 &&
        !HSD_VIData.current.chg_flag,
        "Native video reconfiguration lost settings or deferred to retrace");
    changed.fbWidth = 0;
    require(HSD_VIGetRenderMode()->fbWidth == 640,
        "Native video reconfiguration retained caller-owned storage");
    HSD_VISetBlack(true);
    require(HSD_VIGetVIStatus()->black && !HSD_VIData.current.chg_flag,
        "Native video blanking deferred to retrace");
    HSD_VISetBlack(false);
    require(!HSD_VIGetVIStatus()->black, "Native video did not leave blank state");
    puts("Native HSD video settings initialize without console framebuffer queues");
}

static void check_native_sis_allocator(void)
{
    HSD_SisLib_803A6048(0x2400);
    HSD_Text* text = HSD_SisLib_803A6754(0, -1);
    require(text != NULL && text->alloc_data != NULL &&
                text->alloc_data->data != NULL &&
                text->alloc_data->cursor == text->alloc_data->data &&
                text->alloc_data->capacity == 0x80 &&
                text->string_buffer != NULL,
            "Native SIS text allocation corrupted its arena");
    require(used_head != NULL && used_head->next != NULL,
            "Native SIS arena lost its allocation chain");
    HSD_SisLib_803A5CC4(text);
    require(HSD_SisLib_804D7978 == NULL,
            "Native SIS text release retained its list entry");
    HSD_SisLib_803A5FBC();
    puts("Native SIS arena supports pointer-sized text metadata");
}

int main(int argc, char** argv)
{
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    setvbuf(stdout, NULL, _IONBF, 0);
    check_native_video_init();
    check_vertex_submission();
    check_texture_objects();
    const size_t heap_size = 16 * 1024 * 1024;
    u8* heap = malloc(heap_size);
    require(heap != NULL, "Cannot reserve model heap");
    void* start = OSInitAlloc(heap, heap + heap_size, 1);
    OSHeapHandle handle = OSCreateHeap(start, heap + heap_size);
    require(handle >= 0, "Cannot create model heap");
    HSD_SetHeap(handle);
    OSSetCurrentHeap(handle);
    HSD_ObjSetHeap((u32) heap_size, NULL);
    HSD_ObjInit();
    HSD_IDSetup();
    check_native_sis_allocator();
    test_native_figatree();
    test_native_gobj();
    test_native_moving_collision();
    test_native_fighter_material();
    test_native_article_model();

    HSD_Joint child = { .scale = { 1, 1, 1 }, .position = { 2, 3, 4 } };
    u8 image_bytes[32] = { 0 }, palette_bytes[32] = { 0 };
    HSD_ImageDesc image_desc = { .image_ptr = image_bytes, .width = 8, .height = 8,
                                 .format = GX_TF_C4 };
    HSD_TlutDesc palette_desc = { .lut = palette_bytes, .fmt = GX_TL_RGB5A3, .n_entries = 16 };
    HSD_TObjDesc texture_desc = { .src = GX_TG_TEX0, .scale = { 1, 1, 1 },
        .repeat_s = 1, .repeat_t = 1, .blend_flags = TEX_COLORMAP_MODULATE,
        .magFilt = GX_LINEAR, .imagedesc = &image_desc, .tlutdesc = &palette_desc };
    HSD_Material material = { .alpha = 1 };
    HSD_MObjDesc material_desc = { .texdesc = &texture_desc, .mat = &material };
    HSD_DObjDesc draw_desc = { .mobjdesc = &material_desc };
    HSD_Joint simple = { .child = &child, .scale = { 1, 1, 1 }, .u.dobjdesc = &draw_desc };
    HSD_Joint* descriptor = &simple;
    void* source = NULL;
    void* converted = NULL;
    HSD_Dat dat;
    HSD_DatImport import;
    if (argc >= 3) {
        FILE* file = fopen(argv[1], "rb");
        require(file != NULL, "Cannot open model DAT");
        require(fseek(file, 0, SEEK_END) == 0, "Cannot seek model DAT");
        long size = ftell(file);
        require(size > 0 && fseek(file, 0, SEEK_SET) == 0, "Cannot size model DAT");
        source = malloc(size);
        require(source && fread(source, 1, size, file) == size, "Cannot read model DAT");
        fclose(file);
        converted = malloc(4 * 1024 * 1024);
        require(converted && HSD_DatOpen(&dat, source, size) == HSD_DAT_OK &&
                    HSD_DatImportInit(&import, &dat, converted, 4 * 1024 * 1024, 16384) == HSD_DAT_OK,
                "Cannot prepare native model import");
        descriptor = HSD_DatImportGraph(&import, strtoul(argv[2], NULL, 0), DAT_JOINT);
        require(descriptor != NULL, "Cannot convert model descriptor graph");
        printf("Imported %lu model records\n", import.record_count);
    }
    puts("Loading original HSD joint objects");
    ftPartsPObjSetDefaultClass();
    HSD_JObj* model = HSD_JObjLoadJoint(descriptor);
    ftPartsPObjClearDefaultClass();
    require(model != NULL, "HSD joint loader failed");
    require(model->id == (uintptr_t) descriptor, "Runtime joint lost descriptor identity");
    test_native_fighter_parts(model, argc >= 4 ? argv[3] : NULL,
                              argc >= 5 ? argv[4] : NULL);
    check_model(model, descriptor);
    puts("Original fighter joint/polygon matrix methods passed ordinary and Z-scale branches");
    printf("Checked %u joints, %u polygons, %u skinning weights\n",
           joint_count, polygon_count, weight_count);
    printf("Recorded texture and full fighter material setup for %u model materials\n", material_count);
    if (argc == 1) {
        require(model->child && model->child->id == (uintptr_t) &child,
                "Runtime child reference is incorrect");
        HSD_JObjSetupMatrix(model->child);
        require(model->child->mtx[0][3] == 2 && model->child->mtx[1][3] == 3 &&
                    model->child->mtx[2][3] == 4, "Runtime joint matrix is incorrect");
        HSD_TObj* texture = model->u.dobj->mobj->tobj;
        u32 original_id = texture_identity(&texture->native_texture);
        u8 alternate_pixels[64] = { 0 }, update_commands[8192];
        image_desc.image_ptr = alternate_pixels;
        GXBeginDisplayList(update_commands, sizeof(update_commands));
        HSD_TObjSetup(texture);
        GXEndDisplayList();
        require(texture_identity(&texture->native_texture) == original_id &&
                    texture->native_image.image_ptr == alternate_pixels,
                "Animated image replacement failed to reuse the texture identity");
        image_desc.width = 16;
        GXBeginDisplayList(update_commands, sizeof(update_commands));
        HSD_TObjSetup(texture);
        GXEndDisplayList();
        require(texture_identity(&texture->native_texture) != original_id,
                "Changed image layout failed to replace the texture resource");
        // Restore the source before the automatic buffer leaves scope.
        image_desc.image_ptr = image_bytes;
        image_desc.width = 8;
        GXBeginDisplayList(update_commands, sizeof(update_commands));
        HSD_TObjSetup(texture);
        GXEndDisplayList();
    }
    test_native_fighter_interpolation(model, descriptor);
    puts("Releasing original HSD joint objects");
    static u8 release_commands[65536];
    GXBeginDisplayList(release_commands, sizeof(release_commands));
    HSD_JObjUnref(model);
    u32 release_size = GXEndDisplayList();
    unsigned texture_destroys = 0, palette_destroys = 0;
    for (u32 offset = 0; offset < release_size;) {
        if (!release_commands[offset]) { ++offset; continue; }
        require(release_size - offset >= 7 && release_commands[offset] == 0x50 &&
                    release_commands[offset + 1] == 0,
                "Unexpected command in texture release stream");
        u8 type = release_commands[offset + 2];
        require(type == 0x32 || type == 0x33, "Unexpected texture release command");
        texture_destroys += type == 0x32;
        palette_destroys += type == 0x33;
        offset += 7;
    }
    require(texture_destroys == texture_resources && palette_destroys == palette_resources,
            "Model release failed to destroy each owned native texture resource");
    printf("Released %u textures and %u palettes\n", texture_destroys, palette_destroys);
    require(OSCheckHeap(handle) >= 0, "Model lifecycle damaged the heap");
    hsdForgetClassLibrary(NULL);
    OSDestroyHeap(handle);
    free(converted); free(source); free(heap);
    puts("Native HSD model loading and release passed");
    return 0;
}
