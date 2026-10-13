#ifndef MELEE_IT_NATIVE_ARTICLE_RESOURCE_SCENE_H
#define MELEE_IT_NATIVE_ARTICLE_RESOURCE_SCENE_H

#include <melee/gm/native_match_pool.h>
#include <melee/it/itCharItems.h>

// Recipe scene publication/retirement. Actual ItemLink records belong to the
// shared nativeItemLinkPool; borrower/model state has separate owners.
bool nativeArticleResourceScene_Prepare(void);
bool nativeArticleResourceScene_Retire(void);
bool nativeArticleResourceScene_Ready(void);
uint64_t nativeArticleResourceScene_SnapshotIdentity(void);
bool nativeArticleResourceScene_BufferIndependent(const void*,size_t);

#endif
