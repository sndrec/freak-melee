// Victim-only capture fixtures must not masquerade as captured-Item coverage.
#include <melee/it/native_capture_item.h>
#include <stdlib.h>
bool nativeCaptureItem_Retain(Fighter_GObj* object,NativeCaptureItemPair* pair)
{ (void)object;(void)pair;abort(); }
bool nativeCaptureItem_Attach(Item_GObj* object,Fighter_GObj* holder,unsigned bone)
{ (void)object;(void)holder;(void)bone;abort(); }
bool nativeCaptureItem_Release(Item_GObj* object,unsigned bone,bool clear)
{ (void)object;(void)bone;(void)clear;abort(); }
