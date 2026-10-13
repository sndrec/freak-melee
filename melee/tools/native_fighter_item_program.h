#ifndef NATIVE_FIGHTER_ITEM_PROGRAM_H
#define NATIVE_FIGHTER_ITEM_PROGRAM_H
#include "native_fighter_item_script.h"
#include <melee/ft/native_fighter_runtime.h>
#include "native_fighter_item_tables.h"
#include "native_fighter_article_resources.h"
#include "native_fighter_article_operations.h"
enum {
    ITEM_LIMIT = 24, ARTICLE_LIMIT = SCRIPT_ARTICLE_DECLARATIONS, ARTICLE_STATE_LIMIT = 32,
    ARTICLE_ATTRIBUTE_LIMIT = 32, CALLBACK_LIMIT = 64, ITEM_LINE_BYTES = 1024
};
enum { ITEM_THROWN, ITEM_TICK, ITEM_GROUND, ITEM_HIT, ITEM_CALLBACKS };
enum { ARTICLE_ANIMATION, ARTICLE_PHYSICS, ARTICLE_COLLISION,
       ARTICLE_STATE_CALLBACKS };
enum { ARTICLE_SPAWNED, ARTICLE_DESTROYED, ARTICLE_PICKED_UP,
       ARTICLE_DROPPED, ARTICLE_THROWN, ARTICLE_DAMAGE_DEALT,
       ARTICLE_DAMAGE_RECEIVED, ARTICLE_ENTERED_AIR, ARTICLE_REFLECTED,
       ARTICLE_CLANKED, ARTICLE_ABSORBED, ARTICLE_SHIELD_BOUNCED,
       ARTICLE_HIT_SHIELD, ARTICLE_INTERACTION, ARTICLE_LAUNCHED, ARTICLE_ACCESSORY, ARTICLE_CREATED,
       ARTICLE_STAGE_CONTACT, ARTICLE_STAGE_LANDED, ARTICLE_STAGE_LEFT,
       ARTICLE_OUT_OF_BOUNDS, ARTICLE_HELD_STATUS, ARTICLE_HELD_REQUEST,
       ARTICLE_HOLDER_NEUTRAL,
       ARTICLE_LOGIC_CALLBACKS };

typedef struct ItemCallback {
    ScriptExpressionType result_type;
    bool helper;
    char lines[CALLBACK_LIMIT][ITEM_LINE_BYTES];
    unsigned count;
    unsigned block_indent[8];
    bool block_else[8];
    unsigned block_depth;
    bool needs_body;
    char locals[SCRIPT_CODE_LOCALS][32];
    ScriptExpressionType local_types[SCRIPT_CODE_LOCALS];
    unsigned local_indent[SCRIPT_CODE_LOCALS];
    unsigned local_count;
    uint32_t function_calls;
    uint32_t signal_calls;
} ItemCallback;

typedef struct ItemDeclaration {
    char name[48];
    unsigned damage, angle, lifetime;
    char icon[128];
    float billboard_size;
    float hitbox_size;
    unsigned knockback_growth, base_knockback;
    ItemCallback callbacks[ITEM_CALLBACKS];
} ItemDeclaration;

typedef struct ArticleStateDeclaration {
    int animation;
    unsigned callback_state;
    bool declared;
    ItemCallback callbacks[ARTICLE_STATE_CALLBACKS];
} ArticleStateDeclaration;

struct ArticleDeclaration {
    char kind[64];
    unsigned index;
    ScriptItemTable tables[SCRIPT_ITEM_TABLE_LIMIT];
    unsigned table_count;
    struct {
        char name[48];
        uint32_t hash;
        ItemCallback callback;
    } signals[NATIVE_FIGHTER_ARTICLE_SIGNAL_LIMIT];
    unsigned signal_count;
    const ScriptFunctionProgram* functions;
    bool in_signal;
    ItemCallback* expression_callback;
    struct NativeItemFunctionProgram* item_functions;
    const NativeScriptArticleCatalog* catalog;
    const ScriptStateTable* fighter_state;
    ScriptStateTable local_state;
    unsigned state_count;
    struct {
        char name[48];
        unsigned offset;
        bool integer;
    } attributes[ARTICLE_ATTRIBUTE_LIMIT];
    unsigned attribute_count;
    NativeArticleScriptNames model_roots;
    NativeArticleScriptNames model_animations;
    NativeArticleScriptResources resources;
    bool standard_render;
    bool attachment_declared;
    NativeFighterAttachmentSpawn attachment;
    bool projectile_declared;
    int projectile_appearance;
    unsigned projectile_state;
    char projectile_values[3][192];
    bool projectile_auto_lifetime;
    bool projectile_initial_velocity;
    bool projectile_initial_update;
    ArticleStateDeclaration states[ARTICLE_STATE_LIMIT];
    ItemCallback callbacks[ARTICLE_LOGIC_CALLBACKS];
};

struct NativeFighterItemProgram {
    NativeScriptArticleCatalog catalog;
    ScriptStateTable fighter_state;
    ItemDeclaration items[ITEM_LIMIT];
    unsigned count;
    ArticleDeclaration articles[ARTICLE_LIMIT];
    unsigned article_count;
};

bool nativeItemStatements_Finish(ItemCallback*);
bool nativeItemStatements_Add(ItemCallback*, const char*, unsigned, bool,
    ArticleDeclaration*, bool, const NativeFighterParameters*);
bool nativeFighterItemScript_Action(const char*, unsigned, const ArticleDeclaration*,
    bool, const NativeFighterParameters*, char*, size_t);
void nativeItemStatements_Emit(FILE*, const ItemCallback*, const ScriptFunctionProgram*, bool);
bool nativeItemStatements_MayInvalidate(const ItemCallback*, const ScriptFunctionProgram*);
#endif
