#ifndef MELEE_MN_ASSETS_H
#define MELEE_MN_ASSETS_H

// Menu-scene owned decoded assets. Load after the scene heap is established.
void mnAssets_Load(void);
void* mnAssets_Get(const char* name);

#endif
