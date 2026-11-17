#pragma once
#include "native_fighter_snapshot.h"
#include "native_authoring_reply.h"

namespace NativeFighterAuthoringProcess {
// Check an immutable copy in the installed native tool, keeping candidate
// preparation and its runtime registries out of the running game's process.
// Returns structured source diagnostics. Never saves, accepts or installs edits.
void check(const NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    unsigned member, NativeAuthoring::Reply&);
// Build all editable members in the isolated tool and verify its cached output
// against these exact live inputs. Cache membership remains non-selectable.
void build(const NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const void* portrait, size_t portrait_size, const char* content_root,
    NativeAuthoring::Reply&);
}
