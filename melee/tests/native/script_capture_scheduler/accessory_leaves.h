#include <melee/ft/native_script_binding_presentation.h>
#include <melee/ft/ftaccessory_assets.h>

static Fighter* accessory_fighter;
static HSD_JObj accessory_model[2];
static HSD_AObj accessory_clocks[4];
static HSD_DObj accessory_draw;
static HSD_MObj accessory_material;
static HSD_TObj accessory_texture;
static FighterBone accessory_parts[MAX_FT_PARTS];
static unsigned accessory_order[16], accessory_order_count, accessory_retire;
static unsigned accessory_variant, accessory_frees;
static float accessory_frame;
static struct {
    FtAccessoryAsset recipe;
    HSD_Joint root;
    HSD_AnimJoint base, variants[4];
    HSD_MatAnimJoint material;
    HSD_DatImportRecord records[7];
} accessory_assets;
static HSD_Dat accessory_dat;

static void accessory_observe(unsigned step)
{
    CHECK(accessory_order_count < 16);
    accessory_order[accessory_order_count++] = step;
    if (accessory_retire == step) ++accessory_fighter->x8_spawnNum;
}
static HSD_JObj* accessory_load(HSD_Joint* descriptor)
{
    CHECK(accessory_case && descriptor == &accessory_assets.root);
    accessory_observe(1);
    return accessory_model;
}
static void accessory_remove(HSD_JObj* model)
{
    CHECK(accessory_case && model == accessory_model);
    ++accessory_frees;
}
static void accessory_follow(HSD_JObj* model, HSD_JObj* bone)
{
    CHECK(model == accessory_model && bone == accessory_fighter->parts[51].joint);
    accessory_observe(6);
}
void HSD_JObjAddAnimAll(HSD_JObj* model, HSD_AnimJoint* joint,
                       HSD_MatAnimJoint* material, HSD_ShapeAnimJoint* shape)
{
    CHECK(accessory_case && model == accessory_model && !shape);
    if (joint == &accessory_assets.base) {
        CHECK(material == &accessory_assets.material); accessory_observe(2);
    } else {
        CHECK(joint == &accessory_assets.variants[accessory_variant] && !material);
        accessory_observe(3);
    }
}
void HSD_JObjReqAnimAll(HSD_JObj* model, float frame)
{
    CHECK(accessory_case && model == accessory_model);
    accessory_frame = frame; accessory_observe(4);
}
void HSD_JObjAnimAll(HSD_JObj* model)
{
    CHECK(accessory_case && model == accessory_model); accessory_observe(5);
}

// Exact current native constructor/rate policy, with only entry names changed.
// Construction, animation loading/evaluation and follow-matrix are observers.
#include "accessory_reference.inc"
