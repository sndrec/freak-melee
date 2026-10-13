#include "grpractice.h"

#include "grdatfiles.h"
#include "inlines.h"
#include <melee/lb/lbheap.h>
#include <melee/mp/types.h>
#include <sysdolphin/baselib/dobj.h>
#include <sysdolphin/baselib/pobj.h>
#include <math.h>
#include <string.h>

// Authored in game units, independently of the retail stage archives. All
// geometry and descriptors live in one match-lifetime allocation.
enum { MAX_VERTICES = 32, POINT_COUNT = 15, GRID_SIZE = 256,
       GRID_TEXTURE_BYTES = 88000 };
typedef struct PracticeStage {
    UnkStageDat map;
    struct UnkStageDat_x8_t scene;
    GroundParam ground;
    StageParam parameters;
    MapCollData collision;
    MapJoint collision_joint;
    MapLine floor;
    Vec2 floor_vertices[2];
    HSD_Joint root;
    HSD_Joint points[POINT_COUNT];
    GroundJointMapSet point_map;
    GroundJointMapPair point_ids[POINT_COUNT];
    HSD_DObjDesc mesh;
    HSD_MObjDesc material_desc;
    HSD_Material material;
    HSD_PObjDesc polygon;
    HSD_VtxDescList attributes[4];
    Vec3 vertices[MAX_VERTICES];
    GXColor colors[MAX_VERTICES];
    Vec2 texcoords[MAX_VERTICES];
    HSD_ImageDesc image;
    HSD_TObjDesc texture;
    HSD_TexLODDesc lod;
    u8 grid_texture[GRID_TEXTURE_BYTES];
    u8 display[(3 + MAX_VERTICES * 6 + 31) & ~31];
    u16 vertex_count;
} PracticeStage;

static bool editor_preview;

void grPractice_SetEditorPreview(bool enabled)
{
    editor_preview = enabled;
}

static void add_quad(PracticeStage* stage, Vec3 a, Vec3 b, Vec3 c, Vec3 d,
                     GXColor color)
{
    HSD_ASSERT(0, stage->vertex_count + 4 <= MAX_VERTICES);
    const Vec3 vertices[] = { a, b, c, d };
    for (int i = 0; i < 4; ++i) {
        u16 index = stage->vertex_count++;
        stage->vertices[index] = vertices[i];
        stage->colors[index] = color;
        // Axis strips use a white point between the texture's grid lines.
        stage->texcoords[index] = (Vec2){ .05f, .05f };
        // GX display-list indices and command lengths remain big endian;
        // the authored vertex arrays below explicitly use native floats.
        u8* out = &stage->display[3 + index * 6];
        out[0] = out[2] = out[4] = index >> 8;
        out[1] = out[3] = out[5] = index & 255;
    }
}

static Vec3 plane_point(Vec3 origin, Vec3 across, Vec3 up, float x, float y)
{
    return (Vec3){ origin.x + across.x * x + up.x * y,
                   origin.y + across.y * x + up.y * y,
                   origin.z + across.z * x + up.z * y };
}

static void plane_rect(PracticeStage* stage, Vec3 origin, Vec3 across,
                       Vec3 up, float x, float y, float w, float h,
                       GXColor color)
{
    add_quad(stage, plane_point(origin, across, up, x, y),
             plane_point(origin, across, up, x + w, y),
             plane_point(origin, across, up, x + w, y + h),
             plane_point(origin, across, up, x, y + h), color);
}

static void axis_ribbon(PracticeStage* stage, Vec3 origin, Vec3 across,
                        Vec3 up, float width, float height, GXColor color)
{
    Vec3 a = origin;
    Vec3 b = plane_point(origin, across, up, width, 0);
    Vec3 c = plane_point(origin, across, up, width, height);
    Vec3 d = plane_point(origin, across, up, 0, height);
    add_quad(stage, a, b, c, d, color);
    add_quad(stage, d, c, b, a, color);
}

static void grid_plane(PracticeStage* stage, Vec3 origin, Vec3 across,
                       Vec3 up, Vec3 normal, int width, int height,
                       GXColor background, int red_x, int red_y)
{
    const GXColor red = { 213, 38, 43, 255 };
    u16 base = stage->vertex_count;
    plane_rect(stage, origin, across, up, 0, 0, width, height, background);
    float u = red_x >= 0 ? -red_x / 50.0f : 0;
    float v = red_y >= 0 ? -red_y / 50.0f : 0;
    float w = width / 50.0f;
    float h = height / 50.0f;
    stage->texcoords[base] = (Vec2){ u, v };
    stage->texcoords[base + 1] = (Vec2){ u + w, v };
    stage->texcoords[base + 2] = (Vec2){ u + w, v + h };
    stage->texcoords[base + 3] = (Vec2){ u, v + h };
    // Keep the line layer clear of the backing plane at Melee's depth-buffer
    // precision, including when the camera pulls back for four players.
    origin.x += normal.x * .5f;
    origin.y += normal.y * .5f;
    origin.z += normal.z * .5f;
    if (red_x >= 0) {
        plane_rect(stage, origin, across, up, red_x - .275f, 0,
                   .55f, height, red);
    }
    if (red_y >= 0) {
        plane_rect(stage, origin, across, up, 0, red_y - .275f,
                   width, .55f, red);
    }
}

static float line_coverage(float position, float spacing, float width)
{
    float distance = fabsf(remainderf(position, spacing));
    const float pixel_width = 50.0f / GRID_SIZE;
    return fminf(1, fmaxf(0, (width * .5f + pixel_width * .5f - distance) /
                               pixel_width));
}

static void make_grid_texture(PracticeStage* stage)
{
    // A filtered, mipmapped tile avoids the disappearing subpixel geometry
    // that a grid of thin polygons produces as the gameplay camera moves.
    u8 pixels[GRID_SIZE * GRID_SIZE];
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            float px = (x + .5f) * 50.0f / GRID_SIZE;
            float py = (y + .5f) * 50.0f / GRID_SIZE;
            float fine = fmaxf(line_coverage(px, 2, .07f),
                              line_coverage(py, 2, .07f)) * .25f;
            float medium = fmaxf(line_coverage(px, 10, .16f),
                                line_coverage(py, 10, .16f)) * .4f;
            float major = fmaxf(line_coverage(px, 50, .35f),
                               line_coverage(py, 50, .35f)) * .6f;
            pixels[y * GRID_SIZE + x] =
                (u8)(255 * (1 - fmaxf(fine, fmaxf(medium, major))));
        }
    }
    size_t output = 0;
    for (int size = GRID_SIZE; size > 0; size /= 2) {
        // GX I8 stores 8x4 tiles, including padding at the smallest mips.
        for (int by = 0; by < size; by += 4) {
            for (int bx = 0; bx < size; bx += 8) {
                HSD_ASSERT(0, output + 32 <= sizeof(stage->grid_texture));
                for (int y = 0; y < 4; ++y) {
                    for (int x = 0; x < 8; ++x) {
                        int sx = (bx + x) % size;
                        int sy = (by + y) % size;
                        stage->grid_texture[output++] = pixels[sy * size + sx];
                    }
                }
            }
        }
        for (int y = 0; y < size / 2; ++y) {
            for (int x = 0; x < size / 2; ++x) {
                int source = y * 2 * size + x * 2;
                pixels[y * (size / 2) + x] =
                    (pixels[source] + pixels[source + 1] +
                     pixels[source + size] + pixels[source + size + 1] + 2) / 4;
            }
        }
    }
    stage->image = (HSD_ImageDesc){
        .image_ptr = stage->grid_texture, .width = GRID_SIZE,
        .height = GRID_SIZE, .format = GX_TF_I8, .mipmap = 1, .maxLOD = 8,
    };
    stage->lod = (HSD_TexLODDesc){ .minFilt = GX_LIN_MIP_LIN,
                                  .max_anisotropy = GX_ANISO_4 };
    stage->texture = (HSD_TObjDesc){
        .src = GX_TG_TEX0, .scale = {1,1,1},
        .wrap_s = GX_REPEAT, .wrap_t = GX_REPEAT, .repeat_s = 1, .repeat_t = 1,
        .blend_flags = TEX_COLORMAP_MODULATE | TEX_LIGHTMAP_DIFFUSE,
        .magFilt = GX_LINEAR, .imagedesc = &stage->image, .lod = &stage->lod,
    };
}

static void load_assets(UnkArchiveStruct* archive)
{
    PracticeStage* stage = lbHeap_80015BD0(0, sizeof(*stage));
    HSD_ASSERT(0, stage != NULL);
    memset(stage, 0, sizeof(*stage));

    // A wide continuous floor. The eventual practice scene resets escaped
    // fighters; stage collision itself uses the normal Melee collision code.
    stage->floor_vertices[0] = (Vec2){ -240, 0 };
    stage->floor_vertices[1] = (Vec2){ 240, 0 };
    stage->floor = (MapLine){ 0, 1, -1, -1, -1, -1,
                             CollLine_Floor, mp_Terrain_Basic };
    stage->collision_joint = (MapJoint){
        .floor_count = 1, .left_bound = -240, .right_bound = 240,
        .vtx_count = 2,
    };
    stage->collision = (MapCollData){
        .verts = stage->floor_vertices, .vert_count = 2,
        .lines = &stage->floor, .line_count = 1, .floor_count = 1,
        .joints = &stage->collision_joint, .joint_count = 1,
    };
    // All-Star rest-area music (1p_qk.hps); -1 denotes an undefined track,
    // not silence, in the stage music selector.
    stage->parameters = (StageParam){ .stkind = St_Kind_Practice,
                                      .x4 = 0, .x8 = -1, .x18 = 100 };
    stage->ground = (GroundParam){
        // x8 feeds the camera's vertical FOV, despite the old tilt label.
        .y = 1, .x4 = 128, .x8 = 30, .xC = 140, .x10 = 1000,
        .x14 = -8, .x18 = 0, .x1C = 0,
        .x20 = 1, .x24 = 1, .x28 = 1,
        .x2E = 40, .x30 = 10, .x34 = 50, .x38 = 100,
        .x3C = 10, .x40 = 10, .x44 = 10, .x48 = 10,
        .x5C = 30, .x60 = 10, .x68 = 100,
        .stage_params = &stage->parameters, .stage_param_count = 1,
    };

    static const struct { int id; Vec3 position; } points[] = {
        { 0, { -60, 0, 0 } }, { 1, { -20, 0, 0 } },
        { 2, { 20, 0, 0 } }, { 3, { 60, 0, 0 } },
        { 4, { -60, 40, 0 } }, { 5, { -20, 40, 0 } },
        { 6, { 20, 40, 0 } }, { 7, { 60, 40, 0 } },
        { 0x94, { 0, 0, 0 } }, { 0x95, { -220, -40, 0 } },
        { 0x96, { 220, 160, 0 } },
        { 0x97, { -300, -120, 0 } }, { 0x98, { 300, 220, 0 } },
        { 0x7F, { -80, 40, 0 } }, { 0x80, { 80, 40, 0 } },
    };
    _Static_assert(ARRAY_SIZE(points) == POINT_COUNT, "stage point count");
    stage->root = (HSD_Joint){ .flags = JOBJ_UNK_B18 | JOBJ_ROOT_OPA,
                               .scale = { 1, 1, 1 },
                               .child = stage->points,
                               .u.dobjdesc = &stage->mesh };
    for (int i = 0; i < POINT_COUNT; ++i) {
        stage->points[i] = (HSD_Joint){
            .scale = { 1, 1, 1 }, .position = points[i].position,
            .next = i + 1 < POINT_COUNT ? &stage->points[i + 1] : NULL,
        };
        stage->point_ids[i] = (GroundJointMapPair){ i + 1, points[i].id };
    }
    stage->point_map = (GroundJointMapSet){ &stage->root, stage->point_ids,
                                          POINT_COUNT };
    stage->scene.unk0 = &stage->root;
    stage->map = (UnkStageDat){ .unk0 = &stage->point_map, .unk4 = 1,
                              .unk8 = &stage->scene, .unkC = 1 };

    if (editor_preview) {
        // The editor fighter spawns at the center-floor point (0, 0, 0).
        // Draw the axes there, with perpendicular ribbons so they remain
        // legible when the preview camera orbits around the fighter.
        const GXColor red = { 156, 84, 84, 255 };
        const GXColor green = { 88, 135, 111, 255 };
        axis_ribbon(stage, (Vec3){ -240, -.35f, 0 },
                    (Vec3){ 1, 0, 0 }, (Vec3){ 0, 1, 0 },
                    480, .7f, red);
        axis_ribbon(stage, (Vec3){ -240, 0, -.35f },
                    (Vec3){ 1, 0, 0 }, (Vec3){ 0, 0, 1 },
                    480, .7f, red);
        axis_ribbon(stage, (Vec3){ -.35f, -170, 0 },
                    (Vec3){ 1, 0, 0 }, (Vec3){ 0, 1, 0 },
                    .7f, 340, green);
        axis_ribbon(stage, (Vec3){ 0, -170, -.35f },
                    (Vec3){ 0, 0, 1 }, (Vec3){ 0, 1, 0 },
                    .7f, 340, green);
    } else {
        grid_plane(stage, (Vec3){-240,0,-50}, (Vec3){1,0,0}, (Vec3){0,1,0},
                   (Vec3){0,0,1}, 480, 220, (GXColor){116,120,125,255}, 240, -1);
        grid_plane(stage, (Vec3){-240,0,-50}, (Vec3){1,0,0}, (Vec3){0,0,1},
                   (Vec3){0,1,0}, 480, 160, (GXColor){143,148,155,255}, 240, 50);
        grid_plane(stage, (Vec3){-240,0,-50}, (Vec3){0,0,1}, (Vec3){0,1,0},
                   (Vec3){1,0,0}, 160, 220, (GXColor){124,128,133,255}, -1, -1);
        grid_plane(stage, (Vec3){240,0,-50}, (Vec3){0,0,1}, (Vec3){0,1,0},
                   (Vec3){-1,0,0}, 160, 220, (GXColor){124,128,133,255}, -1, -1);
    }
    stage->display[0] = GX_QUADS;
    stage->display[1] = stage->vertex_count >> 8;
    stage->display[2] = stage->vertex_count & 255;
    stage->attributes[0] = (HSD_VtxDescList){
        .attr = GX_VA_POS, .attr_type = GX_INDEX16, .comp_cnt = GX_POS_XYZ,
        .comp_type = GX_F32, .stride = sizeof(Vec3),
        .vertex = stage->vertices, .vertex_size = sizeof(stage->vertices),
        .vertex_little_endian = true,
    };
    stage->attributes[1] = (HSD_VtxDescList){
        .attr = GX_VA_CLR0, .attr_type = GX_INDEX16, .comp_cnt = GX_CLR_RGBA,
        .comp_type = GX_RGBA8, .stride = sizeof(GXColor),
        .vertex = stage->colors, .vertex_size = sizeof(stage->colors),
        .vertex_little_endian = true,
    };
    stage->attributes[2] = (HSD_VtxDescList){
        .attr = GX_VA_TEX0, .attr_type = GX_INDEX16, .comp_cnt = GX_TEX_ST,
        .comp_type = GX_F32, .stride = sizeof(Vec2),
        .vertex = stage->texcoords, .vertex_size = sizeof(stage->texcoords),
        .vertex_little_endian = true,
    };
    stage->attributes[3].attr = GX_VA_NULL;
    stage->polygon = (HSD_PObjDesc){
        .verts = stage->attributes, .display = stage->display,
        .n_display = (3 + stage->vertex_count * 6 + 31) / 32,
    };
    make_grid_texture(stage);
    stage->material = (HSD_Material){ .diffuse = {255,255,255,255}, .alpha = 1 };
    stage->material_desc = (HSD_MObjDesc){
        .rendermode = RENDER_VERTEX | RENDER_TEX0, .mat = &stage->material,
        .texdesc = &stage->texture,
    };
    stage->mesh = (HSD_DObjDesc){ .mobjdesc = &stage->material_desc,
                                 .pobjdesc = &stage->polygon };
    archive->unk4 = &stage->map;
    stage_info.coll_data = &stage->collision;
    stage_info.param = &stage->ground;
    stage_info.itemdata = NULL;
    stage_info.ald_yaku_all = NULL;
    stage_info.map_ptcl = NULL;
    stage_info.map_texg = NULL;
    stage_info.yakumono_param = NULL;
    stage_info.map_plit = NULL;
    stage_info.x6C8 = NULL;
    stage_info.quake_model_set = NULL;
}

static StageCallbacks callbacks[1];
static void init(void)
{
    Ground_GObj* map = Ground_GetStageGObj(0);
    HSD_ASSERT(0, map != NULL);
    Ground_SetupStageCallbacks(map, callbacks);
    stage_info.unk8C.b4 = false;
    stage_info.unk8C.b5 = true;
    Ground_801C39C0();
    Ground_801C3BB4();
}

static void no_event(void) {}
static void no_demo(int unused) {}
static bool no_callback(void) { return false; }
static bool check_shadow(Vec3* position, int unused, HSD_JObj* joint)
{
    return false;
}

StageData grPractice_StageData = {
    .grkind = Gr_Kind_Practice, .callbacks = callbacks,
    .on_init = init, .on_demo_init = no_demo, .on_load = no_event,
    .on_start = no_event, .callback4 = no_callback,
    .on_check_shadow_render = check_shadow,
    .load_native = load_assets,
};
