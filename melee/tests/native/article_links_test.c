#include <melee/it/native_article.h>
#include <melee/it/types.h>
#include <melee/it/inlines.h>
#include <melee/it/item.h>
#include <sysdolphin/baselib/gobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../tools/native_fighter_item_links.h"

static Item items[3];
static Item_GObj objects[3];
static HSD_GObj* heads[32];

static void require(bool condition, const char* reason)
{
    if (condition) return;
    fprintf(stderr, "%s\n", reason);
    exit(1);
}

int main(void)
{
    HSD_GObjPLinkHead = heads;
    heads[HSD_GOBJ_PLINK_ITEM] = objects;
    for (unsigned i = 0; i < 3; ++i) {
        objects[i].user_data = &items[i];
        objects[i].next = i == 2 ? NULL : &objects[i + 1];
        items[i].native_article_author_kind = Ft_Kind_Pikachu;
        items[i].x1C = 100 + i;
        for (unsigned j = 0; j < 4; ++j) items[i].native_article_links[j] = UINT32_MAX;
    }
    require(nativeArticle_Link(objects, 1, &objects[1]) &&
            items[0].native_article_links[1] == 101,
            "Explicit one-way link was not installed");
    require(nativeArticle_Link(objects, 1, &objects[1]) &&
            !nativeArticle_Link(objects, 1, &objects[2]),
            "Occupied slot accepted a different destination");
    require(items[1].native_article_links[1] == UINT32_MAX,
            "One-way link unexpectedly created a backlink");
    require(!nativeArticle_Link(objects, 0, objects) &&
            !nativeArticle_Link(objects, 4, &objects[1]) &&
            !nativeArticle_Link(NULL, 0, &objects[1]),
            "Invalid explicit link accepted");
    items[0].native_article_links[1] = 999;
    require(nativeArticle_Link(objects, 1, &objects[1]),
            "A stale slot could not be reused");
    nativeArticle_Unlink(objects, 1);
    items[0].native_article_links[0] = 101;
    items[1].native_article_links[2] = 100;
    items[1].native_article_links[3] = 102;
    items[1].pos = (Vec3) { 4, 5, 6 };
    items[1].xDD4_itemVar.native_article.surface_normal = (Vec3) { 0, 1, 0 };
    items[1].facing_dir = -1;
    require(nativeArticle_LinkReciprocal(objects, 0, 2), "Reciprocal link not resolved");
    require(!nativeArticle_LinkReciprocal(objects, 0, 1), "Wrong backlink accepted");
    require(nativeArticle_LinkValue(objects, 0, NativeArticleLink_PositionY) == 5,
            "Linked world position differs");
    require(nativeArticle_LinkValue(objects, 0, NativeArticleLink_NormalY) == 1,
            "Linked surface normal differs");
    require(nativeArticle_LinkValue(objects, 0, NativeArticleLink_Facing) == -1,
            "Linked facing differs");
    require(nativeArticle_LinkId(objects, 0) == 101 &&
                nativeArticle_LinkId(objects, 4) == -1,
            "Link ID or missing sentinel differs");
    items[1].x1C = UINT32_C(0x80000001);
    items[0].native_article_links[0] = items[1].x1C;
    require(nativeArticle_LinkId(objects, 0) == INT32_MIN + 1,
            "Link ID high bits lost");
    items[1].x1C = 101;
    items[0].native_article_links[0] = 101;
    require(nativeArticle_LinkValue(objects, 4, NativeArticleLink_Exists) == 0,
            "Out-of-range link slot accepted");
    items[1].x378_itemColl.env_flags = 0x1234;
    nativeArticle_CopyLinkCollision(objects, 0);
    require(!memcmp(&items[0].x378_itemColl, &items[1].x378_itemColl, sizeof(CollData)),
            "Collision snapshot not copied completely");
    // Changing damage ownership must not change the immutable author identity.
    items[1].owner = &objects[2];
    require(nativeArticle_LinkReciprocal(objects, 0, 2), "Ownership transfer broke links");
    items[1].native_article_author_kind = Ft_Kind_Pichu;
    require(!nativeArticle_Link(objects, 1, &objects[1]),
            "Explicit link crossed package authors");
    require(!nativeArticle_LinkValue(objects, 0, NativeArticleLink_Exists),
            "Link crossed package authors");
    items[1].native_article_author_kind = Ft_Kind_Pikachu;
    // A reused object address with a new creation ID cannot satisfy the old link.
    items[1].x1C = 200;
    require(!nativeArticle_LinkValue(objects, 0, NativeArticleLink_Exists),
            "Stale creation ID resolved to reused object");
    items[1].x1C = 101;
    objects[0].next = &objects[2];
    require(!nativeArticle_Link(objects, 1, &objects[1]) &&
            !nativeArticle_Link(&objects[1], 1, objects),
            "Explicit link accepted a removed object address");
    require(!nativeArticle_LinkValue(objects, 0, NativeArticleLink_Exists),
            "Removed object still resolved");
    objects[0].next = &objects[1];
    nativeArticle_Unlink(objects, 0);
    require(items[0].native_article_links[0] == UINT32_MAX &&
            items[1].native_article_links[2] == UINT32_MAX &&
            items[1].native_article_links[3] == 102,
            "Unlink failed or removed an unrelated relationship");
    require(nativeArticle_BoneValue(objects, 100, 0) == 0 &&
            nativeArticle_LinkBoneValue(objects, 0, 6, 0) == 0,
            "Missing/out-of-range bone read did not return zero");
    nativeArticle_Unlink(NULL, 0);
    Item_GObj empty = { 0 };
    nativeArticle_Unlink(&empty, 0);
    SpawnItem invalid = { 0 };
    invalid.kind = It_Kind_Pikachu_TJolt_Ground;
    invalid.x10 = 137;
    require(Item_CreateAuthored(NULL, Ft_Kind_Pikachu,0) == NULL &&
            Item_CreateAuthored(&invalid, Ft_Kind_Pikachu,0) == NULL && invalid.x10 == 137,
            "Missing authored definition fell back to a stock item or mutated the spawn");

    char output[256];
    const char* after;
    require(nativeItemLinks_Expression("item.link_bone(3, 99).rotation.z + 1", output,
            sizeof(output), &after) && !strcmp(after, " + 1"), "Bone expression boundary differs");
    require(!nativeItemLinks_Expression("item.link(4).exists", output, sizeof(output), &after) &&
            !nativeItemLinks_Expression("item.bone(100).position.x", output, sizeof(output), &after) &&
            !nativeItemLinks_Expression("item.bone(0).position", output, sizeof(output), &after) &&
            !nativeItemLinks_Expression("item.bone(0).position.", output, sizeof(output), &after) &&
            !nativeItemLinks_Expression("item.link(0).exists_extra", output, sizeof(output), &after),
            "Invalid link/bone expression accepted");
    require(nativeItemLinks_Action("item.unlink(3)", output, sizeof(output)) &&
            !nativeItemLinks_Action("item.unlink(4)", output, sizeof(output)) &&
            !nativeItemLinks_Action("item.copy_link_collision(0)garbage", output, sizeof(output)),
            "Invalid link operation accepted");
    puts("Article creation-ID links, ownership transfer, removal and parser bounds pass");
    return 0;
}
