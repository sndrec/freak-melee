#pragma once

#ifdef MELEE_NATIVE

#include <cstddef>
#include <string>
#include <vector>

namespace WorkshopScriptFiles {
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
};

void open(Session& session, const char* project_path, const std::string& seed);
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
