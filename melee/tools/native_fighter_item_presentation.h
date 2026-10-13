#ifndef NATIVE_FIGHTER_ITEM_PRESENTATION_H
#define NATIVE_FIGHTER_ITEM_PRESENTATION_H
#include "native_fighter_item_script.h"
// Compile article presentation and immutable-author handle dispatch statements.
// Returns 0 for another parser's prefix, 1 on success, -1 for invalid syntax.
int nativeItemPresentation_Action(const char*, const ArticleDeclaration*,
                                  const NativeFighterParameters*, char*, size_t);
#endif
