#pragma once
#ifdef MELEE_NATIVE
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

// An exclusive authoring lease follows the physical directories, including both
// members of a family. The OS releases it when the owning process exits.
typedef struct NativeFighterDraftLease {
    void* handles[3];
    unsigned count;
} NativeFighterDraftLease;
bool nativeFighterDraft_Acquire(const char* path, NativeFighterDraftLease*,
                                char* error, size_t capacity);
void nativeFighterDraft_Release(NativeFighterDraftLease*);

#ifdef __cplusplus
}
#include <string>
#include <vector>

namespace NativeDraftAccess {
struct Lease {
    NativeFighterDraftLease value{};
    Lease() = default;
    Lease(const Lease&) = delete;
    Lease& operator=(const Lease&) = delete;
    ~Lease() { nativeFighterDraft_Release(&value); }
    bool acquire(const char* path, std::string& error);
};
struct FileEdit {
    std::string file; // Project-relative path; never traverses a reparse point.
    std::string bytes;
    bool remove = false;
};
// Hash names and bytes in a stable order. Temporary history/journal files are
// excluded. Reparse points inside the draft are rejected.
bool revision(const std::string& root, std::string& hash, std::string& error);
// Caller holds the root's exclusive lease. A durable journal restores the old
// files after failure/interruption; completion preserves the whole batch.
bool apply(const std::string& root, const std::string& expected,
           const std::vector<FileEdit>& edits, std::string& after,
           std::string& error);
bool recover(const std::string& root, std::string& error);
}
#endif
#endif
