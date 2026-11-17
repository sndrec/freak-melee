#pragma once
#include "native_fighter_snapshot.h"
#include "native_authoring_reply.h"

namespace NativeFighterValidationProcess {
// Check an immutable copy in the installed native tool, keeping candidate
// preparation and its runtime registries out of the running game's process.
// Returns structured source diagnostics. Never saves, accepts or installs edits.
void check(const NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    unsigned member, NativeAuthoring::Reply&);
}
