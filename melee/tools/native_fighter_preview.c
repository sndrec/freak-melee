#include <melee/ft/fighter.h>
#include <melee/ft/ftanim.h>
#include <melee/lb/lbanim.h>
#include <sysdolphin/baselib/aobj.h>
#include <melee/ft/ftdat.h>
#include <melee/ft/ftdata.h>
#include <melee/ft/ftparts.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <stdio.h>
#include <stdlib.h>

// Diagnostic scene storage; one Mario model lives until the viewer exits.
static Fighter fighter;
static ftCo_DatAttrs preview_attributes;
static struct ftData_x8 preview_model_data;
static ftData preview_data = { .x0 = &preview_attributes, .x8 = &preview_model_data };
static FighterBone bones[256];
static HSD_DObj* draws[124];
static FighterPartsTable table;
static FighterPartsTable* tables[] = { &table };
static struct Fighter_804D6540_t omitted;
static struct Fighter_804D6540_t* omissions[] = { &omitted };
static struct Fighter_804D6540_x0_t records[32];
static u8 joints[256], parts[FT_COMMON_PART_COUNT];

static void require(int ok)
{
    if (!ok) {
        fprintf(stderr, "Cannot initialize native fighter preview from PlCo.dat\n");
        exit(1);
    }
}

static void install_joint_class(HSD_JObj* joint)
{
    for (; joint; joint = joint->next) {
        ftParts_80073758(joint);
        if (!(joint->flags & JOBJ_INSTANCE)) install_joint_class(joint->child);
    }
}

void native_fighter_preview_bind(HSD_GObj* object, const char* common_path, FigaTree* clip,
                                 HSD_Joint* costume, float blend_frames, const char* fighter_path)
{
    FILE* file = fopen(common_path, "rb");
    require(file && fseek(file, 0, SEEK_END) == 0);
    long size = ftell(file);
    require(size > 0 && fseek(file, 0, SEEK_SET) == 0);
    void* bytes = malloc(size);
    require(bytes && fread(bytes, 1, size, file) == size);
    fclose(file);
    HSD_Dat dat;
    u32 root, table_offset, descriptor, omission_offset;
    const char* external;
    require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
        HSD_DatFindRoot(&dat, "ftLoadCommonData", &root) &&
        HSD_DatResolve(&dat, root + 16, &table_offset, &external) == HSD_DAT_REF_LOCAL &&
        HSD_DatResolve(&dat, table_offset, &descriptor, &external) == HSD_DAT_REF_LOCAL &&
        ftData_DecodeParts(&dat, descriptor, &table, joints, 256, parts) == HSD_DAT_OK &&
        HSD_DatResolve(&dat, root + 20, &omission_offset, &external) == HSD_DAT_REF_LOCAL);
    HSD_DatReferenceKind ref = HSD_DatResolve(&dat, omission_offset, &descriptor, &external);
    require(ref == HSD_DAT_REF_NULL || (ref == HSD_DAT_REF_LOCAL &&
        ftData_DecodePartOmissions(&dat, descriptor, &omitted, records, 32,
                                  table.parts_num) == HSD_DAT_OK));
    free(bytes);
    file = fopen(fighter_path, "rb");
    require(file && fseek(file, 0, SEEK_END) == 0);
    size = ftell(file);
    require(size > 0 && fseek(file, 0, SEEK_SET) == 0);
    bytes = malloc(size);
    require(bytes && fread(bytes, 1, size, file) == size);
    fclose(file);
    u32 attributes, model_data;
    require(HSD_DatOpen(&dat, bytes, size) == HSD_DAT_OK &&
        HSD_DatFindRoot(&dat, "ftDataMario", &root) &&
        HSD_DatResolve(&dat, root, &attributes, &external) == HSD_DAT_REF_LOCAL &&
        ftData_DecodeCommonAttrs(&dat, attributes, &preview_attributes) == HSD_DAT_OK &&
        HSD_DatResolve(&dat, root + 8, &model_data, &external) == HSD_DAT_REF_LOCAL);
    const u8* model_parts = HSD_DatBytes(&dat, model_data + 16, 5);
    require(model_parts && model_parts[0] < table.parts_num && preview_attributes.model_scaling > 0);
    preview_model_data.x10 = model_parts[0];
    preview_model_data.x11 = model_parts[1];
    preview_model_data.x12 = model_parts[2];
    preview_model_data.x13 = model_parts[3];
    preview_model_data.x14 = model_parts[4];
    free(bytes);
    fighter.co_attrs = preview_attributes;
    printf("Decoded Mario attributes: weight %.1f, model scale %.3f, scale part %u\n",
           preview_attributes.weight, preview_attributes.model_scaling, preview_model_data.x10);
    ftPartsTable = tables;
    Fighter_804D6540 = omissions;
    fighter.gobj = object;
    fighter.ft_data = &preview_data;
    fighter.x108_costume_joint = costume;
    fighter.parts = bones;
    fighter.dobj_list.data = draws;
    fighter.x34_scale = (Vec3) { 1, 1, 1 };
    fighter.x61D = 255;
    object->user_data = &fighter;
    ftParts_SetupParts(object);
    install_joint_class(GET_JOBJ(object));
    u32 nodes = 0;
    while (clip->nodes[nodes] != -1) ++nodes;
    require(nodes == table.parts_num);
    fighter.x590 = clip;
    fighter.x594_b1_loop = true;
    ftAnim_8006FE48(object);
    ftAnim_8006EBE8(object, 0, 1, 0);
    if (blend_frames > 0) {
        ftAnim_8006E9B4(object);
        ftAnim_8006EBE8(object, clip->frames * 0.5f, 1, blend_frames);
        printf("Started original fighter transition over %.1f frames toward idle phase %.1f\n",
               blend_frames, clip->frames * 0.5f);
    }
    puts("Original fighter animation dispatcher bound decoded idle tracks");
    printf("Fighter preview bound %lu parts and %lu draw objects\n",
           table.parts_num, fighter.dobj_list.count);
}
