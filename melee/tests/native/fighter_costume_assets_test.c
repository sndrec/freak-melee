#include <stdint.h>

#include <aurora/dvd.h>
#include <dolphin/dvd.h>
#include <melee/ft/ftcostume_assets.h>
#include <melee/ft/ftcostume_bindings.h>
#include <melee/ft/ftdata.h>
#include <melee/lb/lbfile.h>
#include <sysdolphin/baselib/spline.h>
#include <sysdolphin/baselib/native_constraint.h>
#include <sysdolphin/baselib/robj.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(bool condition, const char* message)
{
    if (!condition) {
        fputs(message, stderr);
        exit(1);
    }
}

static void test_model_bindings(void)
{
    HSD_ImageDesc image = {.format = GX_TF_C8};
    HSD_TlutDesc palette = {0};
    HSD_TObjDesc texture = {.imagedesc = &image, .tlutdesc = &palette};
    HSD_Material material = {0};
    HSD_MObjDesc surface = {.mat = &material, .texdesc = &texture};
    HSD_DObjDesc mesh = {.mobjdesc = &surface};
    HSD_Joint joint = {.u.dobjdesc = &mesh};
    FtModelCounts counts;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, &counts) == FT_MODEL_VALID &&
            counts.joints == 1 && counts.construction_units == 4,
            "single-mesh model with an indexed image and palette");
    HSD_TObjTevDesc tev = {0};
    HSD_TexAnim cycle = {.id = GX_TEXMAP0, .needs_tev = true, .selects_indexed_images = true};
    HSD_MatAnim colors = {.texanim = &cycle};
    HSD_MatAnimJoint material_animation = {.matanim = &colors};
    HSD_AObjDesc controller = {0};
    FtCostumeAssets costume = {.joint = &joint, .joint_count = 1,
                               .matanim_joint = &material_animation};
    FtCostumeBindings slots = {.joints = 1, .required_draws = 1, .texture_count = 1};
    cycle.aobjdesc = &controller;
    require(ftCostumeBindings_Fit(&slots, &costume), "the fighter's texture slot has an animated layer");
    slots.textures[0] = 1;
    require(!ftCostumeBindings_Fit(&slots, &costume), "a slot beyond the costume's texture count");
    slots.textures[0] = 0;
    cycle.aobjdesc = NULL;
    require(!ftCostumeBindings_Fit(&slots, &costume), "the selected layer has no animation controller");
    slots.texture_count = 0;
    require(ftCostumeBindings_Fit(&slots, &costume), "a fully static fighter can use a static costume");
    slots.required_draws = 2;
    require(!ftCostumeBindings_Fit(&slots, &costume), "the parent's visibility list needs a second mesh");
    texture.id = GX_TEXMAP4; // Binding uses the layer assigned at material construction.
    texture.tev = &tev;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, &material_animation, 1, &counts) == FT_MODEL_VALID &&
            counts.construction_units > 4,
            "animated colors and indexed frames bind to the layer's color registers and palette");
    texture.tev = NULL;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, &material_animation, 1, NULL) ==
            FT_MODEL_INVALID_MATERIAL_ANIMATION, "the selected layer needs color registers");
    cycle.needs_tev = false;
    image.format = GX_TF_I8;
    texture.tlutdesc = NULL;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, &material_animation, 1, NULL) ==
            FT_MODEL_INVALID_MATERIAL_ANIMATION, "indexed animation frames need a palette on this layer");
    image.format = GX_TF_C8;
    texture.tlutdesc = &palette;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 0, NULL) == FT_MODEL_INVALID_JOINT,
            "attachment must fit its draw object list");
    texture.tlutdesc = NULL;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_INVALID_JOINT,
            "indexed image requires a palette before drawing");
    image.format = GX_TF_RGB565;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_VALID,
            "direct color image requires only its image descriptor");

    HSD_Joint bone = {0};
    HSD_PObjDesc polygon = {.flags = POBJ_SKIN, .u.joint = &bone};
    mesh.pobjdesc = &polygon;
    joint.child = &bone;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_VALID,
            "a shared position matrix can use a child bone");
    joint.child = NULL;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_INVALID_JOINT,
            "a shared position matrix requires a bone from this model");
    joint.child = &bone;
    joint.flags = JOBJ_SKELETON_ROOT;
    HSD_EnvelopeDesc weights[] = {{.joint = &bone, .weight = 1.0f}, {0}};
    HSD_EnvelopeDesc* position_slots[] = {weights, NULL};
    polygon.flags = POBJ_ENVELOPE;
    polygon.u.envelope_p = position_slots;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_VALID,
            "a rigid envelope on the skeleton root can omit its inverse bind");
    weights[0].weight = 0.5f;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_INVALID_JOINT,
            "a blended influence needs an inverse bind");
    Mtx inverse = {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}};
    bone.mtx = inverse;
    HSD_EnvelopeDesc fixed = {0};
    HSD_EnvelopeDesc* texture_slots[] = {&fixed, NULL};
    HSD_EnvelopeDesc blended[] = {{&bone, 0.5f}, {&bone, 0.5f}, {0}};
    position_slots[0] = blended;
    HSD_EnvelopeMatricesDesc matrices = {.positions = position_slots, .textures = texture_slots,
                                          .constant_mask = 1};
    polygon.flags = POBJ_SEPARATE_ENVELOPES;
    polygon.u.matrices = &matrices;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_VALID,
            "a fixed texture matrix can accompany weighted positions");
    HSD_PObjDesc shape_polygon = {.flags = POBJ_SHAPEANIM};
    HSD_ShapeSetDesc shape = {.flags = SHAPESET_SKIN, .skin = &polygon};
    shape_polygon.u.shape_set = &shape;
    mesh.pobjdesc = &shape_polygon;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_VALID,
            "shape batches resolve their influences against the same skeleton");
    joint.child = NULL;
    require(ftCostumeAssets_ValidateModel(&joint, NULL, NULL, 1, NULL) == FT_MODEL_INVALID_JOINT,
            "shape batches require their bones in the constructed model");
}

static void test_particle_instances(void)
{
    HSD_SList emitters[256] = {0};
    for (unsigned i = 1; i < 256; ++i) emitters[i - 1].next = &emitters[i];
    HSD_Joint parts[4] = {0};
    HSD_Joint root = {.child = parts};
    for (unsigned i = 0; i < 4; ++i) {
        parts[i].flags = JOBJ_PTCL;
        parts[i].u.ptcl = emitters;
        if (i) parts[i - 1].next = &parts[i];
    }
    require(ftCostumeAssets_ValidateModel(&root, NULL, NULL, 1, NULL) == FT_MODEL_VALID,
            "Four particle joints fit the combined model budget");
    HSD_Joint instance = {.flags = JOBJ_INSTANCE, .child = parts};
    parts[3].next = &instance;
    require(ftCostumeAssets_ValidateModel(&root, NULL, NULL, 1, NULL) == FT_MODEL_INVALID_JOINT,
            "An instance also draws its target's particle entries");
}

static void test_effect_animation_bindings(void)
{
    Vec3 points[] = {{0, 0, 0}, {1, 0, 0}};
    f32 lengths[] = {0, 1};
    HSD_Spline spline = {.numcv = 2, .cv = points, .totalLength = 1, .segLength = lengths};
    HSD_Joint path = {.flags = JOBJ_SPLINE, .u.spline = &spline};
    HSD_Joint model = {.child = &path};
    HSD_FObjDesc track = {.type = HSD_A_J_PATH};
    HSD_AObjDesc controller = {.fobjdesc = &track, .obj_id = (uintptr_t)&path,
                               .obj_id_is_joint = true};
    HSD_AnimJoint animation = {.aobjdesc = &controller};
    require(ftCostumeAssets_ValidateModel(&model, &animation, NULL, 1, NULL) == FT_MODEL_VALID,
            "effect movement can follow the model's spline");
    model.child = NULL;
    require(ftCostumeAssets_ValidateModel(&model, &animation, NULL, 1, NULL) ==
            FT_MODEL_INVALID_JOINT_ANIMATION, "movement references a path in this effect");
    model.child = &path;
    controller.obj_id = (uintptr_t)&model;
    require(ftCostumeAssets_ValidateModel(&model, &animation, NULL, 1, NULL) ==
            FT_MODEL_INVALID_JOINT_ANIMATION, "movement needs a spline on the target joint");
    controller.obj_id = 0;
    track.type = HSD_A_J_ROTX;
    HSD_AnimJoint root = {.child = &animation};
    require(ftCostumeAssets_ValidateModel(&model, &root, NULL, 1, NULL) == FT_MODEL_VALID,
            "a child joint can animate its Euler rotation");
    path.flags |= JOBJ_USE_QUATERNION;
    require(ftCostumeAssets_ValidateModel(&model, &root, NULL, 1, NULL) ==
            FT_MODEL_INVALID_JOINT_ANIMATION, "Euler channels require an Euler joint");
}

static void put_word(u8* at, u32 value)
{
    at[0] = (u8)(value >> 24);
    at[1] = (u8)(value >> 16);
    at[2] = (u8)(value >> 8);
    at[3] = (u8)value;
}

static void test_attachment_constraint_scope(void)
{
    enum { prop = 64, target = 128, aim = 192, expression = 204,
           input = 212, code = 228, data_bytes = 232 };
    static const u32 slots[] = {8, prop + 60, target + 8, aim + 8, expression,
                                expression + 4, input + 4};
    static const char names[] = "body\0prop";
    u8 file[32 + data_bytes + sizeof(slots) + 16 + sizeof(names)] = {0};
    u8* data = file + 32;
    u8* table = data + data_bytes;
    u8* symbols = table + sizeof(slots);
    put_word(file, sizeof(file));
    put_word(file + 4, data_bytes);
    put_word(file + 8, sizeof(slots) / sizeof(slots[0]));
    put_word(file + 12, 2);
    put_word(symbols + 8, prop);
    put_word(symbols + 12, 5);
    memcpy(symbols + 16, names, sizeof(names));
    for (unsigned i = 0; i < sizeof(slots) / sizeof(slots[0]); ++i)
        put_word(table + 4*i, slots[i]);
    put_word(data + 8, target);
    put_word(data + target + 8, prop);
    put_word(data + prop + 60, aim);
    put_word(data + expression, code);
    put_word(data + expression + 4, input);
    put_word(data + input, 0x10);
    put_word(data + input + 4, target);
    data[code] = Cns_Argument;
    data[code + 3] = Cns_Return;
    for (unsigned i = 0; i < 2; ++i) {
        put_word(data + aim + 4, i ? 0x80000000u | REFTYPE_BYTECODE | HSD_A_J_TRAX :
                                     0x80000000u | REFTYPE_JOBJ | 1);
        put_word(data + aim + 8, i ? expression : target);
        FtCostumeAssets assets;
        u8 arena[8192];
        require(ftCostumeAssets_Import(file, sizeof(file), "body", NULL,
                                       arena, sizeof(arena), 32, &assets),
                "The whole model owns the prop's target");
        HSD_Joint single = *assets.joint->child->child;
        single.child = single.next = NULL;
        require(ftCostumeAssets_ValidateModel(&single, NULL, NULL, 124, NULL) == FT_MODEL_INVALID_JOINT,
                "A single copied fighter part must own all its constraint inputs");
        single.robjdesc = NULL;
        require(ftCostumeAssets_ValidateModel(&single, NULL, NULL, 124, NULL) == FT_MODEL_VALID,
                "A part with independent transforms can be installed by itself");
        size_t used = assets.import.arena_used;
        require(!ftCostumeAssets_LoadAttachment(&assets, "prop") &&
                assets.import.status == HSD_DAT_OK && assets.import.arena_used == used,
                "A cached prop needs its own target to load independently");
        require(ftCostumeAssets_LoadAttachment(&assets, "body") == assets.joint,
                "The original constrained model is still available");
        require(!HSD_DatImportGraph(&assets.import, prop, DAT_JOINT) &&
                assets.import.status == HSD_DAT_DESCRIPTOR,
                "The DAT root check catches a target outside the selected tree");
    }
}

static void test_attachment_shares_vertices(void)
{
    // Both models draw from one vertex array. The prop's 33 draw objects fit
    // an ordinary model, but a fighter attachment has room for 32.
    enum { prop = 64, body_mesh = 128, prop_mesh = 144, mesh_count = 33,
           body_polygon = prop_mesh + 16 * mesh_count, prop_polygon = body_polygon + 24,
           attrs = prop_polygon + 24, body_draw = attrs + 48, prop_draw = body_draw + 32,
           positions = prop_draw + 32, data_bytes = positions + 36,
           relocations = 2 + 1 + 2 * mesh_count - 1 + 4 + 1 };
    static const char names[] = "body\0prop";
    u8 file[32 + data_bytes + 4 * relocations + 16 + sizeof(names)] = {0};
    u8* data = file + 32;
    u8* table = data + data_bytes;
    u8* symbols = table + 4 * relocations;
    put_word(file, sizeof(file));
    put_word(file + 4, data_bytes);
    put_word(file + 8, relocations);
    put_word(file + 12, 2);
    put_word(symbols + 8, prop);
    put_word(symbols + 12, 5);
    memcpy(symbols + 16, names, sizeof(names));
    u32 targets[data_bytes / 4] = {0};
    targets[16 / 4] = body_mesh;
    targets[(prop + 16) / 4] = prop_mesh;
    targets[(body_mesh + 12) / 4] = body_polygon;
    for (unsigned i = 0; i < mesh_count; ++i) {
        unsigned mesh = prop_mesh + i * 16;
        if (i + 1 < mesh_count) targets[(mesh + 4) / 4] = mesh + 16;
        targets[(mesh + 12) / 4] = prop_polygon;
    }
    targets[(body_polygon + 8) / 4] = targets[(prop_polygon + 8) / 4] = attrs;
    targets[(body_polygon + 16) / 4] = body_draw;
    targets[(prop_polygon + 16) / 4] = prop_draw;
    targets[(attrs + 20) / 4] = positions;
    unsigned n = 0;
    for (unsigned i = 0; i < data_bytes / 4; ++i) {
        if (!targets[i]) continue;
        require(n < relocations, "fixture relocation capacity");
        put_word(table + 4 * n++, i * 4);
        put_word(data + 4 * i, targets[i]);
    }
    require(n == relocations, "fixture relocation count");
    put_word(data + body_polygon + 12, 1);
    put_word(data + prop_polygon + 12, 1);
    put_word(data + attrs, GX_VA_POS);
    put_word(data + attrs + 4, GX_INDEX8);
    put_word(data + attrs + 8, GX_POS_XYZ);
    put_word(data + attrs + 12, GX_F32);
    put_word(data + attrs + 16, 12);
    put_word(data + attrs + 24, GX_VA_NULL);
    data[body_draw] = data[prop_draw] = GX_TRIANGLES;
    data[body_draw + 2] = data[prop_draw + 2] = 3;
    data[prop_draw + 4] = 1;
    data[prop_draw + 5] = 2;
    FtCostumeAssets assets;
    u8 arena[16384];
    require(ftCostumeAssets_Import(file, sizeof(file), "body", NULL, arena, sizeof(arena), 80, &assets),
            "import body with one indexed vertex");
    HSD_VtxDescList* position = assets.joint->u.dobjdesc->pobjdesc->verts;
    require(position->vertex_size == 12, "body draws the first vertex");
    require(!ftCostumeAssets_LoadAttachment(&assets, "prop") && position->vertex_size == 12,
            "a rejected prop restores the shared vertex array");
    require(ftCostumeAssets_LoadAttachment(&assets, "body") == assets.joint,
            "the original model remains usable");
}

int main(int argc, char** argv)
{
    static const char* purin_hats[] = {
        NULL,
        "PlyPurinReHat_TopN_joint",
        "PlyPurinBuHat_TopN_joint",
        "PlyPurinGrHat_TopN_joint",
        "PlyPurinYeHat_TopN_joint",
    };
    u32 imported = 0;
    u32 max_records = 0;
    size_t max_arena_used = 0;
    size_t max_texture_bytes = 0;
    size_t max_display_bytes = 0;
    require(argc == 2, "usage: native_fighter_costume_assets_test <iso>\n");
    test_model_bindings();
    test_particle_instances();
    test_effect_animation_bindings();
    test_attachment_shares_vertices();
    test_attachment_constraint_scope();
    require(aurora_dvd_open(argv[1]), "open ISO\n");

    for (FighterKind kind = Ft_Kind_Mario; kind < Ft_Kind_StockCount; ++kind) {
        for (u32 costume_id = 0;
             costume_id < CostumeListsForeachCharacter[kind].numCostumes;
             ++costume_id)
        {
            const Fighter_CostumeStrings* names =
                &ftData_803C2360[kind][costume_id];
            DVDFileInfo file;
            char path[64];
            require(names->dat_filename != NULL && names->joint_name != NULL,
                    "costume archive metadata\n");
            snprintf(path, sizeof(path), "/%s",
                     lbFileGetFullName(names->dat_filename));
            if (!DVDOpen(path, &file)) {
                fprintf(stderr, "open costume archive: kind=%u costume=%lu "
                                "file=%s\n", kind, costume_id, path);
                return 1;
            }
            size_t size = file.length;
            size_t padded = (size + 31) & ~(size_t) 31;
            void* bytes = _aligned_malloc(padded, 32);
            void* arena = _aligned_malloc(FT_COSTUME_IMPORT_ARENA_SIZE,
                                          sizeof(void*));
            require(bytes != NULL && arena != NULL &&
                        DVDReadPrio(&file, bytes, padded, 0, 2) >= (s32) size &&
                        DVDClose(&file),
                    "read costume archive\n");
            FtCostumeAssets assets;
            if (!ftCostumeAssets_Import(
                    bytes, size, names->joint_name,
                    names->matanim_joint_name, arena,
                    FT_COSTUME_IMPORT_ARENA_SIZE,
                    FT_COSTUME_IMPORT_RECORD_CAPACITY, &assets))
            {
                fprintf(stderr,
                        "%s import failed: status=%d offset=0x%lX "
                        "records=%lu arena=%zu root=%s\n",
                        names->dat_filename, assets.import.status,
                        assets.import.error_offset, assets.import.record_count,
                        assets.import.arena_used, names->joint_name);
                return 1;
            }
            require(assets.joint != NULL &&
                        (names->matanim_joint_name == NULL ||
                         assets.matanim_joint != NULL),
                    "decoded costume roots\n");
            FtCostumeBindings bindings;
            if (!ftCostumeBindings_Stock(kind, costume_id, &bindings) || !ftCostumeBindings_Fit(&bindings, &assets)) {
                fprintf(stderr, "Costume bindings disagree for %s (kind %d, color %u)\n",
                        names->joint_name, (int)kind, (unsigned)costume_id);
                return 1;
            }
            FtCostumeAssets owned;
            require(ftCostumeAssets_Load(bytes, size, names->joint_name,
                        names->matanim_joint_name, &owned) &&
                        owned.import.record_count == assets.import.record_count &&
                        owned.import.arena_used == assets.import.arena_used &&
                        owned.joint_count == assets.joint_count &&
                        owned.arena == owned.import.arena &&
                        owned.import.dat == &owned.dat,
                    "owned costume import matches caller-supplied storage\n");
            free(owned.arena);
            if (kind == Ft_Kind_Purin && purin_hats[costume_id] != NULL) {
                const HSD_DatImport saved = assets.import;
                assets.import.record_capacity = saved.record_count + 1;
                require(ftCostumeAssets_LoadAttachment(&assets, purin_hats[costume_id]) == NULL &&
                        assets.import.status == HSD_DAT_OK &&
                        assets.import.record_count == saved.record_count &&
                        assets.import.decoded_count == saved.decoded_count &&
                        assets.import.arena_used == saved.arena_used &&
                        assets.import.decoded_texture_bytes == saved.decoded_texture_bytes &&
                        assets.import.display_list_bytes == saved.display_list_bytes,
                        "a partial attachment load returns its storage to the costume\n");
                for (u32 i = 0; i < saved.slot_count; ++i)
                    require(saved.slots[i] <= saved.record_count, "stale attachment record\n");
                assets.import.record_capacity = saved.record_capacity;
                HSD_Joint* hat = ftCostumeAssets_LoadAttachment(&assets, purin_hats[costume_id]);
                require(hat != NULL,
                        "Jigglypuff hat fits the attachment renderer\n");
                const size_t used = assets.import.arena_used;
                require(ftCostumeAssets_LoadAttachment(&assets, purin_hats[costume_id]) == hat &&
                        assets.import.arena_used == used, "a repeated hat reuses its model\n");
            }
            if (assets.import.record_count > max_records) {
                max_records = assets.import.record_count;
            }
            if (assets.import.arena_used > max_arena_used) {
                max_arena_used = assets.import.arena_used;
            }
            if (assets.import.decoded_texture_bytes > max_texture_bytes)
                max_texture_bytes = assets.import.decoded_texture_bytes;
            if (assets.import.display_list_bytes > max_display_bytes)
                max_display_bytes = assets.import.display_list_bytes;
            ++imported;
            _aligned_free(arena);
            _aligned_free(bytes);
        }
    }
    aurora_dvd_close();
    require(imported == 133, "all retail costume archives covered\n");
    printf("imported=%lu max_records=%lu max_arena=%zu max_texture_bytes=%zu max_display_bytes=%zu\n",
           imported, max_records, max_arena_used, max_texture_bytes, max_display_bytes);
    return 0;
}
