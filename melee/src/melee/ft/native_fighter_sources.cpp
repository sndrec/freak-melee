#include "native_fighter_sources.h"

#ifdef MELEE_NATIVE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <melee/ft/native_fighter_behavior_source.h>
#include <melee/ft/native_fighter_project.h>
#include <melee/ft/native_fighter_template.h>
#include <melee/ft/native_fighter_parameters.h>
#include "native_fighter_draft_access.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace NativeFighterSources {
namespace {
bool read_file(const std::string& path, std::string* text)
{
    FILE* input = std::fopen(path.c_str(), "rb");
    if (input == nullptr) return false;
    long length = -1;
    bool ok = std::fseek(input, 0, SEEK_END) == 0 &&
              (length = std::ftell(input)) >= 0 &&
              length <= NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY &&
              std::fseek(input, 0, SEEK_SET) == 0;
    if (ok) {
        text->resize((size_t) length);
        ok = std::fread(text->data(), 1, (size_t) length, input) ==
             (size_t) length && text->find('\0') == std::string::npos;
    }
    std::fclose(input);
    if (!ok) text->clear();
    else text->erase(std::remove(text->begin(), text->end(), '\r'), text->end());
    return ok;
}

bool source_revision(const std::string& root, std::string& revision)
{
    std::vector<std::string> names;
    auto exists = [&](const char* name) { return GetFileAttributesA((root + "\\" + name).c_str()) != INVALID_FILE_ATTRIBUTES; };
    if (exists("fighter_state.fms")) {
        names.push_back("fighter_state.fms");
        WIN32_FIND_DATAA item{};
        HANDLE search = FindFirstFileA((root + "\\moves\\*.fms").c_str(), &item);
        if (search != INVALID_HANDLE_VALUE) {
            do {
                if (!(item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) names.push_back(std::string("moves\\") + item.cFileName);
            } while (FindNextFileA(search, &item));
            FindClose(search);
        }
    } else if (exists("behavior.draft.fms")) names.push_back("behavior.draft.fms");
    else if (exists("behavior.fms")) names.push_back("behavior.fms");
    std::sort(names.begin(), names.end());
    std::string bytes;
    for (const auto& name : names) {
        std::string text;
        if (!read_file(root + "\\" + name, &text)) return false;
        bytes += name; bytes += '\0'; bytes += text; bytes += '\0';
        if (bytes.size() > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY + 65536) return false;
    }
    NativeContentId hash{}; char hex[65];
    if (!nativeContent_Hash(bytes.data(), bytes.size(), &hash)) return false;
    nativeContentStore_IdHex(&hash, hex); revision = hex; return true;
}

std::string move_filename(const std::string& declaration, size_t ordinal)
{
    std::string name;
    const size_t begin = declaration.find_first_not_of(" \t", 5);
    const size_t end = declaration.find(':', begin);
    if (begin != std::string::npos && end != std::string::npos)
        for (size_t at = begin; at < end; ++at) {
            const unsigned char ch = (unsigned char) declaration[at];
            if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                (ch >= '0' && ch <= '9') || ch == '_') name += (char) ch;
        }
    if (name.empty()) name = "Move" + std::to_string(ordinal);
    return name;
}

void split_seed(Session& session, const std::string& seed)
{
    session.files.push_back({ "Fighter state", session.root + "\\fighter_state.fms", "", "" });
    size_t current = 0;
    size_t start = 0;
    while (start < seed.size()) {
        const size_t newline = seed.find('\n', start);
        const size_t end = newline == std::string::npos ? seed.size() :
                                                          newline + 1;
        const std::string line = seed.substr(start, end - start);
        if (line.compare(0, 5, "move ") == 0 &&
            line.find(':') != std::string::npos) {
            const std::string name = move_filename(line, session.files.size());
            char ordinal[8];
            std::snprintf(ordinal, sizeof(ordinal), "%03zu_", session.files.size());
            session.files.push_back({ name,
                session.root + "\\moves\\" + ordinal + name + ".fms", "", "" });
            current = session.files.size() - 1;
        }
        session.files[current].text += line;
        start = end;
    }
    for (SourceFile& file : session.files) {
        file.text.erase(std::remove(file.text.begin(), file.text.end(), '\r'), file.text.end());
        file.saved = file.text;
    }
}
}

static bool open_files(Session& session, const char* project_path, const std::string& seed)
{
    close(session);
    session.root = project_path;
    SourceFile state = { "Fighter state", session.root + "\\fighter_state.fms",
                         "", "" };
    if (read_file(state.path, &state.text)) {
        session.disk_sources = true;
        state.saved = state.text;
        session.files.push_back(std::move(state));
        const std::string mask = session.root + "\\moves\\*.fms";
        WIN32_FIND_DATAA item;
        HANDLE search = FindFirstFileA(mask.c_str(), &item);
        if (search != INVALID_HANDLE_VALUE) {
            do {
                if ((item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                    continue;
                SourceFile move = { item.cFileName,
                    session.root + "\\moves\\" + item.cFileName, "", "" };
                if (!read_file(move.path, &move.text)) {
                    session.error = "Could not read move source: " + move.path;
                    FindClose(search);
                    return false;
                }
                move.name.resize(move.name.size() - 4);
                if (move.name.size() > 4 &&
                    move.name[0] >= '0' && move.name[0] <= '9' &&
                    move.name[1] >= '0' && move.name[1] <= '9' &&
                    move.name[2] >= '0' && move.name[2] <= '9' &&
                    move.name[3] == '_')
                    move.name.erase(0, 4);
                move.saved = move.text;
                session.files.push_back(std::move(move));
            } while (FindNextFileA(search, &item));
            FindClose(search);
        }
        std::sort(session.files.begin() + 1, session.files.end(),
            [](const SourceFile& a, const SourceFile& b) {
                return a.path < b.path;
            });
    } else if (GetFileAttributesA(state.path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        session.error = "Could not read fighter state: " + state.path;
        return false;
    } else {
        session.disk_sources = false;
        split_seed(session, seed);
    }
    if (compose(session).size() > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY) {
        session.error = "Combined fighter source exceeds " NATIVE_FIGHTER_BEHAVIOR_SOURCE_LIMIT_TEXT ".";
        return false;
    }
    return true;
}

bool open_project(Session& session, const char* project_path,
                  const NativeFighterProject& project)
{
    std::string seed;
    Session::Origin origin = Session::Validated;
    const std::string root(project_path);
    std::string before;
    if (!source_revision(root, before)) {
        close(session); session.error = "Could not identify the saved source set."; return false;
    }
    if (GetFileAttributesA((root + "\\fighter_state.fms").c_str()) != INVALID_FILE_ATTRIBUTES) {
        origin = Session::SeparateFiles;
    } else if (GetFileAttributesA((root + "\\behavior.draft.fms").c_str()) != INVALID_FILE_ATTRIBUTES) {
        if (!read_file(root + "\\behavior.draft.fms", &seed)) {
            close(session);
            session.error = "Could not read saved script draft.";
            return false;
        }
        origin = Session::Draft;
    } else if (project.script_size) {
        seed.assign((const char*)project.script_text, project.script_size);
    } else {
        char* source = nullptr;
        size_t size = 0;
        if (nativeFighterTemplate_Read(project.base_kind, &source, &size)) {
            seed.assign(source, size);
            origin = Session::Template;
        }
        std::free(source);
    }
    if (!open_files(session, project_path, seed)) return false;
    if (!source_revision(root, session.saved_source_revision) || before != session.saved_source_revision) {
        session.error = "Source files changed while loading. Reopen the draft."; return false;
    }
    session.origin = origin;
    if (project.parameter_size)
        session.parameters.assign((const char*)project.parameter_text, project.parameter_size);
    return true;
}

static bool merge_parameters(std::string& existing, const char* imported, size_t size)
{
    NativeFighterParameters current, added;
    if (!nativeFighterParameters_Parse(existing.data(), existing.size(), &current, nullptr) ||
        !nativeFighterParameters_Parse(imported, size, &added, nullptr)) return false;
    size_t start = 0;
    for (unsigned i = 0; i < added.count; ++i) {
        size_t end = start;
        while (end < size && imported[end] != '\n') ++end;
        unsigned found = 0;
        while (found < current.count && std::strcmp(current.fields[found].name, added.fields[i].name)) ++found;
        if (found < current.count) {
            if (current.fields[found].type != added.fields[i].type) return false;
        } else {
            if (!existing.empty() && existing.back() != '\n') existing += '\n';
            existing.append(imported + start, end - start);
            existing += '\n';
        }
        start = end < size ? end + 1 : end;
    }
    return existing.size() <= NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY;
}

bool import_parameters(Session& session, const NativeFighterProject& project)
{
    Session candidate = session;
    candidate.error.clear(); candidate.error_file = 0; candidate.error_line = 0;
    for (size_t i = 0; i < candidate.files.size(); ++i) {
        auto& file = candidate.files[i];
        const auto& text = file.text;
        if (candidate.origin != Session::Template &&
            text.compare(0, 10, "attribute ") &&
            text.find("\nattribute ") == std::string::npos &&
            text.find("\n    attribute ") == std::string::npos) continue;
        char *converted = nullptr, *parameters = nullptr;
        size_t converted_size = 0, parameter_size = 0;
        unsigned line = 0;
        bool ok = nativeFighterParameters_Import(project.fighter_dat, project.fighter_size,
            project.fighter_root, project.base_kind, text.data(), text.size(),
            &converted, &converted_size, &parameters, &parameter_size, &line);
        if (ok) {
            // Saved overrides win over template defaults. Type changes remain
            // errors; reading a draft must never overwrite the player's tuning.
            ok = merge_parameters(candidate.parameters, parameters, parameter_size);
        }
        if (!ok) {
            std::free(converted); std::free(parameters);
            session.error = "Could not import parameters in " + file.name + " at line " + std::to_string(line) + ". Original sources retained.";
            session.error_file = i; session.error_line = line;
            return false;
        }
        file.text.assign(converted, converted_size);
        std::free(converted); std::free(parameters);
    }
    candidate.imported_parameters = candidate.parameters != session.parameters;
    for (size_t i = 0; i < candidate.files.size(); ++i)
        candidate.imported_parameters |= candidate.files[i].text != session.files[i].text;
    session = std::move(candidate);
    return true;
}

void close(Session& session)
{
    session.files.clear();
    session.root.clear();
    session.disk_sources = false;
    session.error.clear();
    session.error_file = 0;
    session.error_line = 0;
    session.parameters.clear();
    session.saved_source_revision.clear();
    session.imported_parameters = false;
    session.origin = Session::Validated;
}

size_t count(const Session& session) { return session.files.size(); }
const std::string& name(const Session& session, size_t index) { return session.files[index].name; }
const std::string& source(const Session& session, size_t index) { return session.files[index].text; }
void set_source(Session& session, size_t index, const std::string& text)
{
    if (index < session.files.size()) session.files[index].text = text;
}

std::string compose(const Session& session)
{
    std::string result;
    for (const SourceFile& file : session.files) {
        if (!result.empty() && result.back() != '\n') result += '\n';
        result += file.text;
    }
    return result;
}

bool save(Session& session)
{
    NativeDraftAccess::Lease lease;
    if (!lease.acquire(session.root.c_str(), session.error)) return false;
    std::string current;
    if (!source_revision(session.root, current) || current != session.saved_source_revision) {
        session.error = "Source files changed outside the Workshop. Reopen before saving."; return false;
    }
    std::string expected, after;
    return NativeDraftAccess::revision(session.root, expected, session.error) &&
        save_batch(session, expected, nullptr, after);
}

bool save_batch(Session& session, const std::string& expected,
                const std::string* parameters, std::string& after)
{
    if (session.files.empty() || compose(session).size() > NATIVE_FIGHTER_BEHAVIOR_SOURCE_CAPACITY ||
        (parameters && parameters->size() > NATIVE_FIGHTER_PARAMETER_TEXT_CAPACITY)) {
        session.error = "Source set or parameter text exceeds its capacity."; return false;
    }
    if (!session.disk_sources) {
        WIN32_FIND_DATAA item{};
        HANDLE search = FindFirstFileA((session.root + "\\moves\\*.fms").c_str(), &item);
        bool known = true;
        if (search != INVALID_HANDLE_VALUE) {
            do {
                if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                std::string existing = session.root + "\\moves\\" + item.cFileName;
                if (std::none_of(session.files.begin(), session.files.end(), [&](const SourceFile& file) {
                    return _stricmp(file.path.c_str(), existing.c_str()) == 0;
                })) { known = false; break; }
            } while (FindNextFileA(search, &item));
            FindClose(search);
        }
        if (!known) {
            session.error = "Untracked move files already exist. Restore fighter_state.fms or move those files aside before saving this source set.";
            return false;
        }
    }
    std::vector<NativeDraftAccess::FileEdit> edits;
    for (const auto& file : session.files) {
        std::string relative = file.path.substr(session.root.size() + 1);
        std::replace(relative.begin(), relative.end(), '\\', '/');
        edits.push_back({relative, file.text});
    }
    if (parameters) edits.push_back({"fighter_parameters.fmp", *parameters});
    if (GetFileAttributesA((session.root + "\\behavior.draft.fms").c_str()) != INVALID_FILE_ATTRIBUTES)
        edits.push_back({"behavior.draft.fms", "", true});
    if (!NativeDraftAccess::apply(session.root, expected, edits, after, session.error)) return false;
    for (auto& file : session.files) file.saved = file.text;
    session.disk_sources = true;
    session.origin = Session::SeparateFiles;
    if (parameters) session.parameters = *parameters;
    source_revision(session.root, session.saved_source_revision);
    session.error.clear();
    return true;
}

bool dirty(const Session& session)
{
    for (const SourceFile& file : session.files)
        if (file.text != file.saved) return true;
    return false;
}

bool loaded_from_files(const Session& session) { return session.disk_sources; }

bool locate_line(const Session& session, unsigned combined_line,
                 size_t* file, unsigned* local_line)
{
    if (combined_line == 0 || file == nullptr || local_line == nullptr ||
        session.files.empty()) return false;
    std::string prefix;
    unsigned start_line = 1;
    *file = 0;
    for (size_t index = 0; index < session.files.size(); ++index) {
        if (!prefix.empty() && prefix.back() != '\n') prefix += '\n';
        const unsigned next_start = 1u + (unsigned) std::count(
            prefix.begin(), prefix.end(), '\n');
        if (next_start > combined_line) break;
        *file = index;
        start_line = next_start;
        prefix += session.files[index].text;
    }
    *local_line = combined_line - start_line + 1;
    return true;
}
}

#endif
