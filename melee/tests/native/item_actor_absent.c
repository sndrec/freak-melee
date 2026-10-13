// Controlled boundary for fixtures that link scene pools but construct no
// Item userdata. The real destructor is exercised by native_item_pool and
// connected gameplay tests. Accidental item retirement is a fixture failure.
#include <melee/it/item.h>
#include <stdlib.h>
void Item_OnUserDataRemove(void* actor){(void)actor;abort();}
