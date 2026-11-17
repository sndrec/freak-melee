#pragma once
#ifdef MELEE_NATIVE
#include "native_fighter_draft_access.h"
#include "native_fighter_sources.h"
#include "native_fighter_family_project.h"

namespace NativeFighterSnapshot {
// Sources may be null for unopened members. Their saved source files remain intact.
// A capture owns its bytes and never registers fighters or changes a live runtime.
// It encodes the current data, including unfinished source, for isolated checks.
struct Capture { std::vector<NativeDraftAccess::FileEdit> files; };
bool capture(const NativeFighterFamilyProject&,
             NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
             Capture&, std::string& error);
// Caller owns the family lease. Check the disk revision observed on opening it,
// validate every member, then commit all files through one recovery journal.
// Failure preserves the live family and source save markers.
bool save(NativeFighterFamilyProject&,
          NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
          const std::string& expected, std::string& after, std::string& error,
          const std::vector<NativeDraftAccess::FileEdit>& additional = {});
}
#endif
