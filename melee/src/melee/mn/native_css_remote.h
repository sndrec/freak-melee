#pragma once

#include <melee/gm/native_signaling_wire.h>
#include <melee/sc/forward.h>

void nativeCssRemote_Begin(const StaticModelDesc* hand, const StaticModelDesc* token);
void nativeCssRemote_End(void);
void nativeCssRemote_Publish(const NativeCssPointers* pointers);
void nativeCssRemote_Inspect(void);
