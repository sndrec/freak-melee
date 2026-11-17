#pragma once
#include "native_fighter_snapshot.h"

namespace NativeFighterBuild {
// The effective fighter data, independent of selection, saved markers and undo.
// Editable sources/parameters take precedence when provided. Portrait bytes are
// identified separately by the canonical package ID.
bool revision(const NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    std::string& result, std::string& error);
bool matches(const NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const NativeFighterPackage&, const NativeFighterPackageData&, std::string& error);
bool verify_cached(const NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const char* content_root, const NativeContentId&, std::string& error);
// Accept a verified complete build and save the family once. Preserves live DAT
// allocations and editor history. Failure retains the old accepted source.
bool accept(NativeFighterFamilyProject&,
    NativeFighterSources::Session* const sources[NATIVE_FIGHTER_FAMILY_MEMBERS],
    const char* content_root, const NativeContentId&, const std::string& expected_disk,
    std::string& after_disk, std::string& error);
}
