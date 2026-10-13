#ifndef MELEE_IT_ITARTICLE_DATA_H
#define MELEE_IT_ITARTICLE_DATA_H

#include "types.h"
#include "itcommand_data.h"

#include <sysdolphin/baselib/dat_import.h>

typedef struct ItArticleImportStats {
    u32 article_count;
    u32 state_count;
    u32 script_count;
} ItArticleImportStats;

bool itArticle_IsDescriptor(const HSD_Dat* dat, u32 source,
                            bool allow_null_attributes);
Article* itArticle_Import(HSD_DatImport* import, u32 source,
                          ItArticleImportStats* stats,
                          ItScriptCollect* scripts,
                          bool allow_null_attributes,
                          s32 item_kind, const u32* sibling_articles,
                          u32 sibling_article_count);
bool itArticle_ImportTable(HSD_DatImport* import, u32 source,
                           Article** output, u32 count,
                           ItArticleImportStats* stats, ItScriptCollect* scripts,
                           s32 nullable_attribute_index,
                           s32 first_item_kind);

#ifdef MELEE_NATIVE
struct NativeFighterArticleModelRoot;
struct NativeFighterArticleModelAnimation;
struct NativeFighterArticleAnimationComponent;
typedef struct ItArticleModelAnimationBundle {
    HSD_AnimJoint* anim;
    HSD_MatAnimJoint* matanim;
    HSD_ShapeAnimJoint* shapeanim;
} ItArticleModelAnimationBundle;
// Resolve three exact typed components in the Article's actual owning arena.
// Canonical omitted components, NULL direct pointers and NULL indexed elements
// remain NULL. False preserves output; an unpublished import arena may advance.
bool itArticle_ImportModelAnimation(HSD_DatImport*, const Article*,
    const struct NativeFighterArticleAnimationComponent* components,
    ItArticleModelAnimationBundle* output);
// Resolve one exact special-attribute pointer into its actual owning import
// arena. Declared NULL succeeds with a NULL descriptor; false preserves output.
bool itArticle_ImportModelRoot(HSD_DatImport*, const Article*, u32 offset,
                              HSD_Joint** output);
bool itArticle_ImportModelAnimations(HSD_DatImport*, Article*,
    const struct NativeFighterArticleModelAnimation*, unsigned count);
// Resolve declared immutable roots while the owning character import arena is
// active. Runtime model loading only reads the published native descriptors.
bool itArticle_ImportModelRoots(HSD_DatImport*, Article*,
                                const struct NativeFighterArticleModelRoot*,
                                unsigned count);
#endif

#endif
