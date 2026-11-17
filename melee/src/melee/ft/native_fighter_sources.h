#pragma once

#ifdef MELEE_NATIVE

#include <cstddef>
#include <string>
#include <vector>
struct NativeFighterProject;
namespace NativeDraftAccess { struct FileEdit; }

namespace NativeFighterSources {
struct SourceFile {
    std::string name;
    std::string path;
    std::string text;
    std::string saved;
};

struct Session {
    std::string root;
    std::vector<SourceFile> files;
    bool disk_sources = false;
    std::string error;
    size_t error_file = 0;
    unsigned error_line = 0;
    std::string parameters;
    std::string saved_source_revision;
    bool imported_parameters = false;
    enum Origin { Validated, Draft, Template, SeparateFiles } origin = Validated;
};
struct SourceEdit { std::string file, text; };
struct EditResult { bool ok; const char* code; std::string message; };
// Stage and publish a complete in-memory source/parameter batch. Failure leaves
// the session unchanged. Source syntax may remain unfinished for later checking.
EditResult edit(Session&, const NativeFighterProject&,
                const std::vector<SourceEdit>&, const std::string* parameters);

bool open_project(Session&, const char* project_path, const NativeFighterProject&);
// Copy stock import declarations into owned values without changing disk files.
// A failure keeps all source files and parameter overrides unchanged.
bool import_parameters(Session&, const NativeFighterProject&);
void close(Session& session);
size_t count(const Session& session);
const std::string& name(const Session& session, size_t index);
const std::string& source(const Session& session, size_t index);
void set_source(Session& session, size_t index, const std::string& text);
std::string compose(const Session& session);
bool save(Session& session);
// Save the source set and optional parameter text as one recoverable edit.
// Caller holds an authoring lease and supplies the inspected disk revision.
bool save_batch(Session&, const std::string& expected_revision,
                const std::string* parameters, std::string& after_revision);
// Encode managed sources for a larger project transaction without writing files.
bool stage_save(Session&, std::vector<NativeDraftAccess::FileEdit>&);
// Called only after the complete transaction has committed.
void mark_saved(Session&);
bool dirty(const Session& session);
bool loaded_from_files(const Session& session);
bool locate_line(const Session& session, unsigned combined_line,
                 size_t* file, unsigned* local_line);
}

#endif
