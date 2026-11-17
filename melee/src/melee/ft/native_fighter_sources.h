#pragma once

#ifdef MELEE_NATIVE

#include <cstddef>
#include <string>
#include <vector>
struct NativeFighterProject;

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
    bool imported_parameters = false;
    enum Origin { Validated, Draft, Template, SeparateFiles } origin = Validated;
};

bool open(Session& session, const char* project_path, const std::string& seed);
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
bool dirty(const Session& session);
bool loaded_from_files(const Session& session);
bool locate_line(const Session& session, unsigned combined_line,
                 size_t* file, unsigned* local_line);
}

#endif
