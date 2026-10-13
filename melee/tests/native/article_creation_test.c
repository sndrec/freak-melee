#include <melee/ft/native_fighter_runtime.h>
#include <melee/ft/types.h>
#include <melee/ft/ftcommon.h>
#include <melee/ft/ftparts.h>
#include <melee/it/native_article.h>
#include <melee/it/item.h>
#include <melee/it/types.h>
#include <melee/lb/lbcollision.h>
#include <melee/lb/lb_00B0.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/jobj.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Fighter author, reflector;
static FighterBone author_bones[8];
static HSD_GObj owner, reflected_owner, parent, created;
static Item source, result;
static Article assets;
static SpawnItem observed;
static FighterKind observed_author;
static unsigned creations, callbacks;
static bool fail_creation, remove_in_callback;
static bool creator_live = true;
static bool inspect_child, remove_parent_in_callback;
static bool remove_owner_in_callback, remove_on_pickup;
static unsigned pickups;
static HSD_GObj* live_heads[32];
uint32_t nativeFighterTarget_Id(HSD_GObj* object)
{
    if (object == &owner) return author.x8_spawnNum;
    if (object == &reflected_owner) return reflector.x8_spawnNum;
    return 0;
}
HSD_GObj* ftLib_8008741C(u32 number)
{
    if (!creator_live) return NULL;
    if (number != 0 && number == (u32)author.x8_spawnNum) return &owner;
    if (number != 0 && number == (u32)reflector.x8_spawnNum) return &reflected_owner;
    return NULL;
}
// Boundaries belonging to other operations in native_article.c must never be
// reached by these constructor tests. Abort rather than simulate their behavior.
FighterPartsTable** ftPartsTable;
HSD_GObj** HSD_GObjPLinkHead;
MtxPtr ftCommon_8007F804(Fighter* fighter) { abort(); }
bool lbColl_80008248(HitCapsule* hit, HurtCapsule* hurt, Mtx matrix,
                    float item_scale, float fighter_scale, float z) { abort(); }
void Item_8026AB54(HSD_GObj* item, HSD_GObj* fighter, Fighter_Part part)
{
    if (item != &created || fighter != &owner || part != 7) abort();
    ++pickups;
    if (remove_on_pickup) item->user_data = NULL;
}
void Item_80268D34(HSD_GObj* item, struct ItemStateDesc* state) { abort(); }
void lb_8000B1CC(HSD_JObj* joint, Vec3* local, Vec3* world) { abort(); }
static HSD_JObj attachment_joint, descriptor_root;
static HSD_AnimJoint* observed_anim;
static HSD_MatAnimJoint* observed_mat;
static HSD_ShapeAnimJoint* observed_shape;
static unsigned add_calls, matrix_calls, bone_calls;
void HSD_JObjAddAnimAll(HSD_JObj* joint, HSD_AnimJoint* anim,
                         HSD_MatAnimJoint* mat, HSD_ShapeAnimJoint* shape)
{
    if (joint != &descriptor_root) abort();
    ++add_calls; observed_anim = anim; observed_mat = mat; observed_shape = shape;
}
HSD_JObj* ftLib_80086630(Fighter_GObj* fighter, Fighter_Part bone)
{
    if (fighter != &owner || bone != 7) abort();
    ++bone_calls; return &attachment_joint;
}
void HSD_MtxGetRotation(Mtx matrix, Vec3* output)
{
    if (matrix != attachment_joint.mtx) abort();
    ++matrix_calls; *output = (Vec3){ matrix[0][0], matrix[1][0], matrix[2][0] };
}
static void on_created(Item_GObj* item)
{
    ++callbacks;
    if (inspect_child) {
        Item* child = item->user_data;
        if (child->native_article_links[2] != UINT32_C(0x80000001) ||
            child->xDD4_itemVar.native_article.creation_value != 5 ||
            child->xDD4_itemVar.native_article.creator_spawn_id != 10 ||
            child->xDD4_itemVar.native_article.attachment_owner != &owner ||
            child->xDD4_itemVar.native_article.attachment_facing != -1)
            abort();
    }
    if (remove_parent_in_callback) {
        parent.user_data = NULL;
        live_heads[HSD_GOBJ_PLINK_ITEM] = NULL;
    }
    if (remove_in_callback) item->user_data = NULL;
    if (remove_owner_in_callback) {
        owner.user_data = NULL;
        live_heads[HSD_GOBJ_PLINK_FIGHTER] = NULL;
    }
}
static NativeFighterArticleLogic logic = {
    .kind = It_Kind_Ness_PKFire, .on_created = on_created,
};
static NativeFighterDefinition definition = {
    .article_logic_count = 1, .article_logics = &logic,
};
static NativeFighterDefinition other_definition;
const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (kind == Ft_Kind_CustomFirst || kind == Ft_Kind_CustomFirst + 2) return &definition;
    return kind == Ft_Kind_CustomFirst + 1 ? &other_definition : NULL;
}
Article* nativeFighterRuntime_ArticleAssets(FighterKind kind, ItemKind item)
{ return kind == Ft_Kind_CustomFirst && item == logic.kind ? &assets : NULL; }
void it_8026BB68(HSD_GObj* item_owner, Vec3* position)
{ *position = ((Fighter*)item_owner->user_data)->cur_pos; }
Item_GObj* Item_CreateAuthored(SpawnItem* spawn, FighterKind kind,uint32_t source_owner)
{
    if(source_owner)abort();
    ++creations; observed = *spawn; observed_author = kind;
    if (fail_creation) return NULL;
    memset(&result, 0, sizeof(result));
    result.x1C = 77;
    for (unsigned i = 0; i < 4; ++i) result.native_article_links[i] = UINT32_MAX;
    created.user_data = &result;
    created.next = &parent;
    live_heads[HSD_GOBJ_PLINK_ITEM] = &created;
    return &created;
}
#define REQUIRE(test) do { if (!(test)) { fprintf(stderr, "Creation contract failed line %d\n", __LINE__); return 1; } } while (0)
int main(void)
{
    owner.user_data = &author; author.kind = Ft_Kind_CustomFirst;
    author.x8_spawnNum = 10;
    author.cur_pos = (Vec3){ 12, 13, 0 };
    reflected_owner.user_data = &reflector;
    reflector.kind = Ft_Kind_CustomFirst + 1;
    reflector.x8_spawnNum = 20;
    parent.user_data = &source;
    source.native_article_author_kind = Ft_Kind_CustomFirst;
    Vec3 position = { 3, 4, 0 }, velocity = { -5, 6, 0 };
    REQUIRE(nativeArticle_Create(&owner, logic.kind, &position, &velocity, -1, true, 9, 0.25f, true) == 77);
    REQUIRE(creations == 1 && callbacks == 1 && observed_author == author.kind);
    REQUIRE(!memcmp(&observed.prev_pos, &position, sizeof(position)) &&
            !memcmp(&observed.pos, &author.cur_pos, sizeof(position)) &&
            !memcmp(&observed.vel, &velocity, sizeof(velocity)));
    REQUIRE(observed.x0_parent_gobj == &owner && observed.x4_parent_gobj2 == &owner &&
            observed.x40 == 9 && observed.x44_flag.b0 && observed.facing_dir == -1);
    REQUIRE(result.xDD4_itemVar.native_article.creation_value == 0.25f);
    REQUIRE(result.xDD4_itemVar.native_article.creator_spawn_id == 10);
    result.native_article_author_kind = author.kind;
    REQUIRE(nativeArticle_Creator(&created) == &owner);
    REQUIRE(nativeArticle_Create(&owner, logic.kind, &position, &velocity, -1, false, 9, 0.25f, false) == 77);
    REQUIRE(!observed.x44_flag.b0 && observed.x40 == 9 && callbacks == 2);
    REQUIRE(!memcmp(&observed.pos, &position, sizeof(position)) &&
            !memcmp(&observed.vel, &velocity, sizeof(velocity)));
    result.native_article_author_kind = author.kind;
    // Descriptor installation selects the requested immutable three-graph
    // bundle, without implicit Remove/Req/Anim or command-stream mutation.
    ItemStateDesc descriptors[2] = { 0 };
    ItemStateArray descriptor_list = { .count = 2, .x0_itemStateDesc = descriptors };
    descriptors[1].x0_anim_joint = (HSD_AnimJoint*)1;
    descriptors[1].x4_matanim_joint = (HSD_MatAnimJoint*)2;
    descriptors[1].x8_parameters = (HSD_ShapeAnimJoint*)3;
    assets.xC_itemStates = &descriptor_list;
    result.xC4_article_data = &assets; created.hsd_obj = &descriptor_root;
    result.x524_cmd.u = (void*)4;
    nativeArticle_AddAnimationDescriptor(&created, 1);
    REQUIRE(add_calls == 1 && result.xD0_itemStateDesc == &descriptors[1]);
    REQUIRE(observed_anim == descriptors[1].x0_anim_joint &&
        observed_mat == descriptors[1].x4_matanim_joint &&
        observed_shape == descriptors[1].x8_parameters && result.x524_cmd.u == (void*)4);
    nativeArticle_AddAnimationDescriptor(&created, 2);
    nativeArticle_AddAnimationDescriptor(NULL, 0);
    created.hsd_obj = NULL;
    nativeArticle_AddAnimationDescriptor(&created, 0);
    REQUIRE(add_calls == 1 && result.xD0_itemStateDesc == &descriptors[1]);
    created.hsd_obj = &descriptor_root;
    nativeArticle_AddAnimationDescriptor(&created, 0);
    REQUIRE(add_calls == 2 && observed_anim == NULL && observed_mat == NULL && observed_shape == NULL);
    // Existing-matrix read validates original creator and active runtime parts;
    // transferred ownership remains legal and no matrix setup is performed.
    FighterPartsTable part_table = { .parts_num = 8 };
    FighterPartsTable* part_tables[Ft_Kind_RuntimeCapacity] = { 0 };
    part_tables[author.kind] = &part_table; ftPartsTable = part_tables;
    result.xDC4 = 7;
    attachment_joint.mtx[0][0] = .25f; attachment_joint.mtx[1][0] = -.5f;
    attachment_joint.mtx[2][0] = .75f; attachment_joint.flags = JOBJ_MTX_DIRTY;
    Vec3 euler = { 9, 9, 9 };
    REQUIRE(nativeArticle_ReadAttachmentMatrixRotation(&created, &euler));
    REQUIRE(euler.x == .25f && euler.y == -.5f && euler.z == .75f &&
        matrix_calls == 1 && bone_calls == 1 && attachment_joint.flags == JOBJ_MTX_DIRTY);
    result.owner = &reflected_owner;
    REQUIRE(nativeArticle_ReadAttachmentMatrixRotation(&created, &euler));
    REQUIRE(matrix_calls == 2 && bone_calls == 2);
    result.xDC4 = 8;
    REQUIRE(!nativeArticle_ReadAttachmentMatrixRotation(&created, &euler));
    REQUIRE(euler.x == 0 && euler.y == 0 && euler.z == 0 && matrix_calls == 2);
    result.xDC4 = 7; creator_live = false;
    REQUIRE(!nativeArticle_ReadAttachmentMatrixRotation(&created, &euler));
    creator_live = true;
    REQUIRE(!nativeArticle_ReadAttachmentMatrixRotation(NULL, &euler));
    REQUIRE(!nativeArticle_ReadAttachmentMatrixRotation(&created, NULL));
    REQUIRE(matrix_calls == 2 && bone_calls == 2);
    ftPartsTable = NULL; assets.xC_itemStates = NULL;
    result.owner = &reflected_owner;
    REQUIRE(nativeArticle_Creator(&created) == &owner);
    creator_live = false;
    REQUIRE(nativeArticle_Creator(&created) == NULL);
    creator_live = true;
    author.x8_spawnNum = 11;
    REQUIRE(nativeArticle_Creator(&created) == NULL);
    author.x8_spawnNum = 10;
    result.xDD4_itemVar.native_article.attachment_owner = (HSD_GObj*)1;
    REQUIRE(nativeArticle_Creator(&created) == NULL);
    result.xDD4_itemVar.native_article.attachment_owner = &owner;
    author.kind = Ft_Kind_CustomFirst + 1;
    REQUIRE(nativeArticle_Creator(&created) == NULL);
    author.kind = Ft_Kind_CustomFirst + 2;
    REQUIRE(nativeArticle_Creator(&created) == &owner);
    author.kind = Ft_Kind_CustomFirst;
    for (unsigned transferred = 0; transferred < 3; ++transferred) {
        source.owner = transferred == 0 ? &owner : transferred == 1 ? &reflected_owner : NULL;
        REQUIRE(nativeArticle_CreateAtPosition(&parent, logic.kind, &position, -1) == 77);
        REQUIRE(observed_author == author.kind && observed.x0_parent_gobj == source.owner &&
                observed.x4_parent_gobj2 == &parent);
        REQUIRE(!memcmp(&observed.pos, &position, sizeof(position)) &&
                !memcmp(&observed.prev_pos, &position, sizeof(position)));
        REQUIRE(observed.vel.x == 0 && observed.vel.y == 0 && observed.vel.z == 0 &&
                observed.x40 == 0 && observed.x44_flag.b0);
        REQUIRE(result.xDD4_itemVar.native_article.attachment_owner == source.owner);
        REQUIRE(result.xDD4_itemVar.native_article.creator_spawn_id ==
                (transferred == 0 ? 10 : transferred == 1 ? 20 : 0));
        result.native_article_author_kind = author.kind;
        REQUIRE(nativeArticle_Creator(&created) == (transferred == 0 ? &owner : NULL));
        for (unsigned i = 0; i < 4; ++i) REQUIRE(result.native_article_links[i] == UINT32_MAX);
    }
    HSD_GObjPLinkHead = live_heads;
    live_heads[HSD_GOBJ_PLINK_ITEM] = &parent;
    source.x1C = UINT32_C(0x80000001);
    source.xDD4_itemVar.native_article.creator_spawn_id = author.x8_spawnNum;
    source.xDD4_itemVar.native_article.attachment_owner = &owner;
    source.owner = &reflected_owner;
    inspect_child = true;
    for (unsigned transfer = 0; transfer < 3; ++transfer) {
        const uint32_t damage_owner = transfer == 0 ? 0 : transfer == 1 ? 10 : 20;
        REQUIRE(nativeArticle_CreateChild(&parent, logic.kind, &position, -1,
                                          damage_owner, 1234, 5, 2) == 77);
        REQUIRE(observed_author == author.kind && observed.x4_parent_gobj2 == &parent &&
                observed.x0_parent_gobj == (transfer == 0 ? NULL : transfer == 1 ? &owner : &reflected_owner));
        REQUIRE(!memcmp(&observed.pos, &position, sizeof(position)) &&
                !memcmp(&observed.prev_pos, &position, sizeof(position)) &&
                observed.vel.x == 0 && observed.vel.y == 0 && observed.vel.z == 0 &&
                observed.x40 == 1234 && observed.x44_flag.b0);
        for (unsigned i = 0; i < 4; ++i)
            if (i != 2) REQUIRE(result.native_article_links[i] == UINT32_MAX);
    }
    unsigned child_creations = creations;
    REQUIRE(nativeArticle_CreateChild(&parent, logic.kind, &position, -1, 999, 0, 5, 2) == -1);
    REQUIRE(nativeArticle_CreateChild(&parent, logic.kind, &position, -1, 0, 0, NAN, 2) == -1);
    REQUIRE(nativeArticle_CreateChild(&parent, logic.kind, &position, -1, 0, 0, 5, 4) == -1);
    live_heads[HSD_GOBJ_PLINK_ITEM] = NULL;
    REQUIRE(nativeArticle_CreateChild(&parent, logic.kind, &position, -1, 0, 0, 5, 2) == -1);
    REQUIRE(creations == child_creations);
    live_heads[HSD_GOBJ_PLINK_ITEM] = &parent;
    remove_in_callback = remove_parent_in_callback = true;
    REQUIRE(nativeArticle_CreateChild(&parent, logic.kind, &position, -1, 0, 1234, 5, 2) == 77);
    REQUIRE(parent.user_data == NULL && created.user_data == NULL);
    parent.user_data = &source;
    live_heads[HSD_GOBJ_PLINK_ITEM] = &parent;
    inspect_child = remove_parent_in_callback = false;
    remove_in_callback = true;
    REQUIRE(nativeArticle_CreateAtPosition(&parent, logic.kind, &position, 1) == 77);
    REQUIRE(created.user_data == NULL);
    // Held construction must not pick up or register a deleted child/owner.
    fail_creation = remove_in_callback = false;
    ftPartsTable = part_tables;
    author.parts = author_bones;
    author.parts[7].joint = &attachment_joint;
    live_heads[HSD_GOBJ_PLINK_FIGHTER] = &owner;
    REQUIRE(nativeArticle_CreateHeldAtCenter(&owner, logic.kind, &position, 7, -1) == 77);
    REQUIRE(pickups == 1 && author.item_gobj == &created);
    author.item_gobj = NULL;
    remove_in_callback = true;
    REQUIRE(nativeArticle_CreateHeldAtCenter(&owner, logic.kind, &position, 7, -1) == -1);
    REQUIRE(pickups == 1 && author.item_gobj == NULL);
    remove_in_callback = false;
    remove_on_pickup = true;
    REQUIRE(nativeArticle_CreateHeldAtCenter(&owner, logic.kind, &position, 7, -1) == -1);
    REQUIRE(pickups == 2 && author.item_gobj == NULL);
    remove_on_pickup = false;
    remove_owner_in_callback = true;
    REQUIRE(nativeArticle_CreateHeldAtCenter(&owner, logic.kind, &position, 7, -1) == -1);
    REQUIRE(pickups == 2 && author.item_gobj == NULL);
    remove_owner_in_callback = false;
    owner.user_data = &author;
    live_heads[HSD_GOBJ_PLINK_FIGHTER] = &owner;
    unsigned prior = callbacks;
    fail_creation = true;
    REQUIRE(nativeArticle_CreateAtPosition(&parent, logic.kind, &position, 1) == -1);
    REQUIRE(callbacks == prior);
    prior = creations;
    Vec3 invalid = { NAN, 0, 0 };
    REQUIRE(nativeArticle_Create(&owner, logic.kind, &position, &invalid, 1, false, 0, 0, true) == -1);
    REQUIRE(nativeArticle_CreateAtPosition(&parent, logic.kind, &invalid, 1) == -1);
    source.native_article_author_kind = Ft_Kind_None;
    REQUIRE(nativeArticle_CreateAtPosition(&parent, logic.kind, &position, 1) == -1);
    REQUIRE(creations == prior);
    puts("Velocity, reflected/unowned child ownership and callback removal contracts pass");
    return 0;
}
