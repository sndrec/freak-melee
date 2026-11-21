#include "native_fighter_draft_access.h"
#ifdef MELEE_NATIVE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>

namespace NativeDraftAccess {
namespace {
constexpr size_t MaxFiles = 512, MaxBytes = 256 * 1024 * 1024;
constexpr size_t MaxManifestBytes = 12 + MaxFiles * (8 + 400);
const char* Journal = ".workshop-transaction";
const char* JournalOwner = "Freak Melee authoring transaction\n";
struct Original { std::string file; bool exists; };
bool fail(std::string& error, const std::string& text) { error = text; return false; }
bool exists(const std::string& path) { return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES; }

bool safe_path(const std::string& root, const std::string& relative)
{
    if (relative.empty() || relative.size() > 400 || relative.front() == '/' ||
        relative.find_first_of("\\:<>\"|?*\r\n\t") != std::string::npos) return false;
    for (unsigned char ch : relative) if (ch < 32) return false;
    std::string current = root;
    for (size_t start = 0; start < relative.size();) {
        size_t end = relative.find('/', start);
        if (end == std::string::npos) end = relative.size();
        std::string part = relative.substr(start, end - start);
        if (part.empty() || part.front() == '.' || part.back() == '.' || part.back() == ' ') return false;
        std::string device = part.substr(0, part.find('.'));
        for (auto& ch : device) ch = (char)std::toupper((unsigned char)ch);
        if (device == "CON" || device == "PRN" || device == "AUX" || device == "NUL" ||
            (device.size() == 4 && (device.compare(0, 3, "COM") == 0 || device.compare(0, 3, "LPT") == 0) && device[3] >= '0' && device[3] <= '9')) return false;
        current += '/' + part;
        DWORD flags = GetFileAttributesA(current.c_str());
        if (flags != INVALID_FILE_ATTRIBUTES && (flags & FILE_ATTRIBUTE_REPARSE_POINT)) return false;
        if (end != relative.size() && flags != INVALID_FILE_ATTRIBUTES && !(flags & FILE_ATTRIBUTE_DIRECTORY)) return false;
        start = end + 1;
    }
    return relative.back() != '/';
}

bool read(const std::string& path, std::string& bytes, size_t limit)
{
    HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{};
    BY_HANDLE_FILE_INFORMATION info{};
    bool ok = GetFileType(file) == FILE_TYPE_DISK && GetFileInformationByHandle(file, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) &&
        GetFileSizeEx(file, &size) && size.QuadPart >= 0 && (uint64_t)size.QuadPart <= limit;
    if (ok) {
        bytes.resize((size_t)size.QuadPart);
        DWORD count = 0;
        ok = ReadFile(file, bytes.data(), (DWORD)bytes.size(), &count, nullptr) && count == bytes.size();
    }
    CloseHandle(file); return ok;
}

bool write(const std::string& path, const std::string& bytes)
{
    // Recovery may retry with a stale scratch file. Unlink it before creating
    // the replacement so a second name for the old file keeps its contents.
    if (!DeleteFileA(path.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND) return false;
    HANDLE file = CreateFileA(path.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD count = 0;
    bool ok = WriteFile(file, bytes.data(), (DWORD)bytes.size(), &count, nullptr) &&
        count == bytes.size() && FlushFileBuffers(file);
    return CloseHandle(file) && ok;
}

bool mark(const std::string& directory, const char* name, const char* text)
{
    const std::string destination = directory + '/' + name;
    const std::string temporary = destination + ".next";
    return write(temporary, text) && MoveFileExA(temporary.c_str(), destination.c_str(),
        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
}

bool parents(const std::string& root, const std::string& relative)
{
    for (size_t at = relative.find('/'); at != std::string::npos; at = relative.find('/', at + 1)) {
        std::string parent = root + '/' + relative.substr(0, at);
        if (!CreateDirectoryA(parent.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS) return false;
    }
    return true;
}

bool journal_file(const std::string& name)
{
    if (name == "owner" || name == "manifest" || name == "prepared" ||
        name == "committed" || name == "restore" || name == "owner.next" ||
        name == "prepared.next" || name == "committed.next") return true;
    size_t dot = name.find('.');
    if (dot < 1 || dot > 3 || (dot > 1 && name[0] == '0') ||
        (name.substr(dot) != ".old" && name.substr(dot) != ".new")) return false;
    unsigned index = 0;
    for (size_t i = 0; i < dot; ++i) {
        if (name[i] < '0' || name[i] > '9') return false;
        index = 10*index + (name[i] - '0');
    }
    return index < MaxFiles;
}

bool known_journal_contents(const std::string& directory)
{
    WIN32_FIND_DATAA data{};
    HANDLE search = FindFirstFileA((directory + "/*").c_str(), &data);
    if (search == INVALID_HANDLE_VALUE) return false;
    bool ok = true;
    do {
        if (!std::strcmp(data.cFileName, ".") || !std::strcmp(data.cFileName, "..")) continue;
        if ((data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
            !journal_file(data.cFileName)) { ok = false; break; }
    } while (FindNextFileA(search, &data));
    if (ok) ok = GetLastError() == ERROR_NO_MORE_FILES;
    FindClose(search);
    return ok;
}

// This removes only regular journal files, never a tree or a reparse point.
bool clean(const std::string& directory)
{
    if (!known_journal_contents(directory)) return false;
    // Remove the recovery marker before its backups. If cleanup is interrupted,
    // the remaining directory can be removed without attempting another restore.
    if (exists(directory + "/prepared") && !DeleteFileA((directory + "/prepared").c_str())) return false;
    WIN32_FIND_DATAA data{};
    HANDLE search = FindFirstFileA((directory + "/*").c_str(), &data);
    if (search == INVALID_HANDLE_VALUE) return false;
    bool ok = true;
    do {
        if (!std::strcmp(data.cFileName, ".") || !std::strcmp(data.cFileName, "..")) continue;
        if (!std::strcmp(data.cFileName, "owner")) continue;
        if (!journal_file(data.cFileName) ||
            (data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
            !DeleteFileA((directory + '/' + data.cFileName).c_str())) { ok = false; break; }
    } while (FindNextFileA(search, &data));
    if (ok) ok = GetLastError() == ERROR_NO_MORE_FILES;
    FindClose(search);
    if (ok && exists(directory + "/owner")) ok = DeleteFileA((directory + "/owner").c_str()) != 0;
    return ok && RemoveDirectoryA(directory.c_str());
}

void integer(std::string& bytes, uint32_t value) { bytes.append((const char*)&value, sizeof(value)); }
bool decode(const std::string& root, const std::string& bytes, std::vector<Original>& files)
{
    if (bytes.size() < 12 || bytes.compare(0, 8, "FMWORKTX")) return false;
    size_t at = 8;
    auto next = [&](uint32_t& value) {
        if (bytes.size() - at < 4) return false;
        std::memcpy(&value, bytes.data() + at, 4); at += 4; return true;
    };
    uint32_t count;
    if (!next(count) || !count || count > MaxFiles) return false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t length, existed;
        if (!next(length) || !next(existed) || existed > 1 || length > 400 ||
            length > bytes.size() - at) return false;
        std::string path = bytes.substr(at, length); at += length;
        if (!safe_path(root, path)) return false;
        for (const auto& original : files)
            if (!_stricmp(original.file.c_str(), path.c_str())) return false;
        files.push_back({path, existed != 0});
    }
    return at == bytes.size();
}
}

bool recover(const std::string& root, std::string& error)
{
    const std::string directory = root + '/' + Journal;
    DWORD flags = GetFileAttributesA(directory.c_str());
    if (flags == INVALID_FILE_ATTRIBUTES) return GetLastError() == ERROR_FILE_NOT_FOUND ||
        GetLastError() == ERROR_PATH_NOT_FOUND || fail(error, "Could not inspect authoring journal.");
    if (!(flags & FILE_ATTRIBUTE_DIRECTORY) || (flags & FILE_ATTRIBUTE_REPARSE_POINT))
        return fail(error, "Invalid authoring journal; original files retained.");
    std::string owner;
    if (!read(directory + "/owner", owner, std::strlen(JournalOwner)) || owner != JournalOwner) {
        // A crash between directory creation and its first write can leave an
        // empty directory. Never clear somebody else's nonempty directory.
        if (RemoveDirectoryA(directory.c_str())) return true;
        return fail(error, "Unrecognized authoring journal; its contents were retained.");
    }
    if (!known_journal_contents(directory))
        return fail(error, "The authoring journal contains an unexpected entry. Its files were retained.");
    if (exists(directory + "/prepared") && !exists(directory + "/committed")) {
        std::string manifest;
        std::vector<Original> originals;
        if (!read(directory + "/manifest", manifest, MaxManifestBytes) || !decode(root, manifest, originals))
            return fail(error, "Unreadable authoring journal; original backups retained for recovery.");
        // Verify the entire backup set before restoring the first destination.
        // A draft revision already limits all original files together to 1 GiB.
        uint64_t total = 0;
        for (size_t i = 0; i < originals.size(); ++i) {
            if (!originals[i].exists) continue;
            WIN32_FILE_ATTRIBUTE_DATA info{};
            const std::string backup = directory + '/' + std::to_string(i) + ".old";
            if (!GetFileAttributesExA(backup.c_str(), GetFileExInfoStandard, &info) ||
                (info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) ||
                info.nFileSizeHigh || info.nFileSizeLow > MaxBytes ||
                (total += info.nFileSizeLow) > 1024ull * 1024 * 1024)
                return fail(error, "An authoring backup is missing or exceeds the draft limits.");
        }
        for (size_t i = 0; i < originals.size(); ++i) {
            const auto& entry = originals[i];
            const std::string destination = root + '/' + entry.file;
            if (entry.exists) {
                // Keep the durable backup until every restore has completed.
                std::string bytes;
                const std::string temporary = directory + "/restore";
                if (!read(directory + '/' + std::to_string(i) + ".old", bytes, MaxBytes) ||
                    !write(temporary, bytes) || !MoveFileExA(temporary.c_str(), destination.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
                    return fail(error, "Could not restore an interrupted edit; recovery backups retained.");
            } else if (exists(destination) && !DeleteFileA(destination.c_str()))
                return fail(error, "Could not remove an interrupted new file; recovery journal retained.");
        }
        // Distinguish a completed restore from a still-recoverable journal if
        // cleanup is interrupted after some backup files have been removed.
        if (!mark(directory, "committed", "restored"))
            return fail(error, "Restored files but could not finalize the recovery journal.");
    }
    return clean(directory) || fail(error, "Recovered the draft; journal cleanup must finish before editing.");
}

bool apply(const std::string& root, const std::string& expected,
           const std::vector<FileEdit>& edits, std::string& after, std::string& error)
{
    std::string current;
    if (!revision(root, current, error)) return false;
    if (current != expected) { after = current; return fail(error, "Draft revision changed. Inspect it before editing."); }
    if (edits.empty() || edits.size() > MaxFiles) return fail(error, "An edit batch needs 1-512 files.");
    std::vector<std::string> names;
    size_t bytes = 0;
    for (const auto& edit : edits) {
        if (!safe_path(root, edit.file)) return fail(error, "Invalid project-relative edit path: " + edit.file);
        std::string name = edit.file;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return (char)std::tolower(ch); });
        if (std::find(names.begin(), names.end(), name) != names.end()) return fail(error, "Duplicate file in edit batch.");
        names.push_back(name);
        bytes += edit.bytes.size();
        if (bytes > MaxBytes) return fail(error, "Edit batch exceeds 256 MiB.");
    }
    const std::string directory = root + '/' + Journal;
    if (!CreateDirectoryA(directory.c_str(), nullptr)) return fail(error, "Authoring journal is already present or cannot be created.");
    if (!mark(directory, "owner", JournalOwner)) {
        clean(directory);
        return fail(error, "Could not identify the new authoring journal. Original files retained.");
    }
    std::string manifest = "FMWORKTX";
    integer(manifest, (uint32_t)edits.size());
    bool staged = true;
    for (size_t i = 0; staged && i < edits.size(); ++i) {
        const auto& edit = edits[i];
        const std::string original = root + '/' + edit.file;
        const bool present = exists(original);
        std::string old;
        staged = (!present || (read(original, old, MaxBytes) && write(directory + '/' + std::to_string(i) + ".old", old))) &&
            (edit.remove || write(directory + '/' + std::to_string(i) + ".new", edit.bytes));
        integer(manifest, (uint32_t)edit.file.size()); integer(manifest, present ? 1 : 0); manifest += edit.file;
    }
    // Check once more after staging. A player editing raw source in another
    // program should receive a conflict before any destination is replaced.
    staged = staged && revision(root, current, error) && current == expected &&
        write(directory + "/manifest", manifest) && mark(directory, "prepared", "ready");
    if (!staged) {
        clean(directory);
        return fail(error, "Could not stage this batch, or draft changed during staging. Original files retained.");
    }
    bool applied = true;
    for (size_t i = 0; applied && i < edits.size(); ++i) {
        const auto& edit = edits[i];
        const std::string destination = root + '/' + edit.file;
        if (edit.remove) applied = !exists(destination) || DeleteFileA(destination.c_str());
        else applied = parents(root, edit.file) && MoveFileExA(
            (directory + '/' + std::to_string(i) + ".new").c_str(), destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    if (applied) applied = revision(root, after, error) && mark(directory, "committed", "complete");
    if (!applied) {
        std::string recovery;
        if (!recover(root, recovery)) return fail(error, recovery);
        return fail(error, "Could not commit the batch. Previous files restored.");
    }
    // A committed marker makes delayed cleanup safe on the next exclusive open.
    clean(directory);
    return true;
}
}
#endif
