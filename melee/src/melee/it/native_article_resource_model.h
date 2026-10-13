#ifndef MELEE_IT_NATIVE_ARTICLE_RESOURCE_MODEL_H
#define MELEE_IT_NATIVE_ARTICLE_RESOURCE_MODEL_H
#include "native_article_resource_internal.h"

// Scene transitions only, after class initialization and before history starts.
bool nativeArticleResourceModel_Prepare(void);
bool nativeArticleResourceModel_Retire(void);
uint64_t nativeArticleResourceModel_SnapshotIdentity(void);
bool nativeArticleResourceModel_BufferIndependent(const void*,size_t);
bool nativeArticleResourceModel_SnapshotProgram(const NativeItemLifetime*);
bool nativeArticleResourceModel_SnapshotGraph(const ResourceNode*,const NativeScriptItemProgram*);
bool nativeArticleResourceModel_OwnerReady(const NativeItemLifetime*);
// Teardown uses retained physical nodes, even after logical actor retirement.
// This is idempotent and clears the GObj graph before ordinary object freeing.
bool nativeArticleResourceModel_Remove(ResourceCollection*,ResourceNode*);
#endif
