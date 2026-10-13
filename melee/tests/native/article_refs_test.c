#include <melee/it/native_article_refs.h>
#include <melee/it/types.h>
#include <melee/it/kinds/types.h>
#include <melee/ft/types.h>
#include <melee/ft/native_fighter_runtime.h>
#include <sysdolphin/baselib/gobj.h>
#include <sysdolphin/baselib/gobjplink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HSD_GObj** HSD_GObjPLinkHead;
static HSD_GObj* heads[32];
static Fighter fighter;
static HSD_GObj fighter_object, caller, target;
static Item source, destination;
static ItemLogicTable item_logic, wrong_logic;
static NativeFighterStateField fields[] = {
    { "ready", 0, 1, NativeFighterState_Bool, 1, false },
    { "handles", 4, 3, NativeFighterState_Int, 4, true },
    { "positions", 16, 3, NativeFighterState_Float, 4, true },
};
static NativeFighterArticleLogic article = {
    .kind = It_Kind_Ness_PKFire, .logic = &item_logic,
    .local_state_bytes = 28, .state_field_count = 3, .state_fields = fields,
};
static NativeFighterDefinition package = {
    .article_logic_count = 1, .article_logics = &article,
};
static NativeFighterDefinition foreign_package;

const NativeFighterDefinition* nativeFighterRuntime_Definition(FighterKind kind)
{
    if (kind == Ft_Kind_CustomFirst || kind == Ft_Kind_CustomFirst + 1)
        return &package;
    return kind == Ft_Kind_CustomFirst + 2 ? &foreign_package : NULL;
}

static void require(bool result, const char* message)
{
    if (result) return;
    fprintf(stderr, "Article reference: %s\n", message);
    exit(1);
}

int main(void)
{
    HSD_GObjPLinkHead = heads;
    heads[HSD_GOBJ_PLINK_FIGHTER] = &fighter_object;
    heads[HSD_GOBJ_PLINK_ITEM] = &caller;
    fighter_object.user_data = &fighter;
    fighter.kind = Ft_Kind_CustomFirst;
    caller.user_data = &source;
    caller.next = &target;
    target.user_data = &destination;
    source.native_article_author_kind = Ft_Kind_CustomFirst;
    destination.native_article_author_kind = Ft_Kind_CustomFirst;
    destination.kind = article.kind;
    destination.xB8_itemLogicTable = &item_logic;
    uint8_t* state = destination.xDD4_itemVar.native_article.state;
    state[0] = 1;
    const int32_t handles[] = { 16777217, INT32_MIN + 1, -1 };
    const float positions[] = { 1.25f, -19.5f, 3.75f };
    memcpy(state + 4, handles, sizeof(handles));
    memcpy(state + 16, positions, sizeof(positions));

    const uint32_t ids[] = { 0, 16777217, 16777218, UINT32_C(0x80000001),
                             UINT32_C(0xfffffffe) };
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i) {
        destination.x1C = ids[i];
        require(nativeArticleRef_Exists(&caller, ids[i], article.kind),
                "full-bit creation identity lost");
        require(nativeArticleRef_Exists(&fighter_object, ids[i], article.kind),
                "fighter context failed");
        require(nativeArticleRef_Bool(&caller, ids[i], article.kind, 0, 0),
                "bool field read failed");
        for (int32_t index = 0; index < 3; ++index) {
            require(nativeArticleRef_Int(&caller, ids[i], article.kind, 1, index)
                        == handles[index], "integer state bits lost");
            require(nativeArticleRef_Float(&caller, ids[i], article.kind, 2, index)
                        == positions[index], "float state read failed");
        }
    }
    const uint32_t id = destination.x1C;
    require(!nativeArticleRef_Exists(&caller, UINT32_MAX, article.kind),
            "failure sentinel resolved");
    require(!nativeArticleRef_Exists(&caller, id - 1, article.kind),
            "adjacent creation ID resolved");
    require(!nativeArticleRef_Exists(&caller, id, It_Kind_Ness_PKThunder1),
            "wrong expected kind resolved");
    require(nativeArticleRef_Int(&caller, id, article.kind, 1, -1) == 0 &&
                nativeArticleRef_Int(&caller, id, article.kind, 1, 3) == 0 &&
                nativeArticleRef_Int(&caller, id, article.kind, 3, 0) == 0 &&
                nativeArticleRef_Float(&caller, id, article.kind, 1, 0) == 0 &&
                !nativeArticleRef_Bool(&caller, id, article.kind, 0, 1),
            "field/type/array bounds not enforced");

    // Damage ownership can change or disappear without altering author identity.
    destination.owner = &fighter_object;
    fighter.x8_spawnNum = (s32)UINT32_C(0xFFFFFFF1);
    source.owner = NULL;
    require(nativeArticleRef_Exists(&caller, id, article.kind), "owner transfer broke reference");
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == UINT32_C(0xFFFFFFF1),
            "current owner lost high ID bits");
    destination.owner = NULL;
    require(nativeArticleRef_CurrentOwner(&fighter_object, id, article.kind) == 0,
            "absent owner did not clear live query");
    HSD_GObj removed_owner = { .user_data = (void*)1 };
    destination.owner = &removed_owner;
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == 0,
            "removed owner was dereferenced");
    destination.owner = &fighter_object;
    destination.native_removal_in_progress = true;
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == 0,
            "retiring article returned owner");
    destination.native_removal_in_progress = false;
    destination.native_article_author_kind = Ft_Kind_CustomFirst + 1;
    require(nativeArticleRef_Exists(&caller, id, article.kind), "same package alias rejected");
    destination.native_article_author_kind = Ft_Kind_CustomFirst + 2;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "foreign package accepted");
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == 0,
            "foreign package owner accepted");
    destination.native_article_author_kind = Ft_Kind_CustomFirst;
    destination.xB8_itemLogicTable = &wrong_logic;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "unmatched live logic accepted");
    destination.xB8_itemLogicTable = &item_logic;
    destination.kind = It_Kind_Ness_PKThunder1;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "wrong live kind accepted");
    destination.kind = article.kind;
    destination.x1C = id + 1;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "recycled generation accepted");
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == 0,
            "recycled article owner accepted");
    destination.x1C = id;
    caller.next = NULL;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "removed target accepted");
    heads[HSD_GOBJ_PLINK_ITEM] = &target;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "removed caller dereferenced");
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == 0,
            "removed caller queried current owner");
    heads[HSD_GOBJ_PLINK_ITEM] = &caller;
    caller.next = &target;
    fields[1].stride = 1;
    require(nativeArticleRef_Int(&caller, id, article.kind, 1, 0) == 0, "bad stride accepted");
    fields[1].stride = 4;
    fields[1].offset = 26;
    require(nativeArticleRef_Int(&caller, id, article.kind, 1, 0) == 0, "truncated field accepted");
    fields[1].offset = 4;
    article.local_state_bytes = 513;
    require(nativeArticleRef_Int(&caller, id, article.kind, 1, 0) == 0, "oversized state accepted");
    article.local_state_bytes = 28;
    article.state_fields = NULL;
    require(nativeArticleRef_Int(&caller, id, article.kind, 1, 0) == 0, "missing metadata accepted");
    require(nativeArticleRef_Int(NULL, id, article.kind, 1, 0) == 0, "null caller accepted");
    HSD_GObjPLinkHead = NULL;
    require(!nativeArticleRef_Exists(&caller, id, article.kind), "absent registry accepted");
    require(nativeArticleRef_CurrentOwner(&caller, id, article.kind) == 0,
            "absent registry returned owner");
    puts("Typed article references preserve IDs and validate authors, logic, generations and bounds");
    return 0;
}
