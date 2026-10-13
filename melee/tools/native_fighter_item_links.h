#ifndef NATIVE_FIGHTER_ITEM_LINKS_H
#define NATIVE_FIGHTER_ITEM_LINKS_H
#include <stdbool.h>
#include <stddef.h>

// Compile literal link slots/bone indices into generic engine queries.
bool nativeItemLinks_Expression(const char*, char*, size_t, const char**);
bool nativeItemLinks_Action(const char*, char*, size_t);
#endif
