#include "native_feedback_agent.h"
#include "native_feedback_capture.h"
#include "native_feedback.h"
#include "native_community.h"
#include "native_content_store.h"
#include <melee/ui/native_hub_catalog.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace NativeFeedbackAgent {
namespace {
using NativeAuthoring::Reply;
using NativeAuthoring::add_string;
constexpr unsigned MaxReports = 16;
constexpr uint64_t MaxLocalBytes = 128 * 1024 * 1024;
std::string in_flight;
std::string pending_capture;
NativeCommunityReceipt receipt{};

const char* text(yyjson_val* object, const char* key)
{ const char* value = yyjson_get_str(yyjson_obj_get(object, key)); return value ? value : ""; }
const char* text(yyjson_mut_val* object, const char* key)
{ const char* value = yyjson_mut_get_str(yyjson_mut_obj_get(object, key)); return value ? value : ""; }
bool identifier(const std::string& id)
{
    if (id.size() != 36) return false;
    for (unsigned i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) { if (id[i] != '-') return false; }
        else if (!((id[i] >= '0' && id[i] <= '9') || (id[i] >= 'a' && id[i] <= 'f'))) return false;
    }
    return true;
}
std::string folder()
{
    char root[512]; const char* content = nativeHubCatalog_Root(root);
    return content ? std::string(content) + "/feedback" : "";
}
bool ordinary(const std::string& path)
{
    DWORD flags = GetFileAttributesA(path.c_str());
    return flags != INVALID_FILE_ATTRIBUTES && !(flags & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}
bool read(const std::string& path, size_t limit, std::string& bytes)
{
    if (!ordinary(path)) return false;
    FILE* file = std::fopen(path.c_str(), "rb");
    long size = file && !std::fseek(file, 0, SEEK_END) ? std::ftell(file) : -1;
    bool ok = size >= 0 && size <= limit && !std::fseek(file, 0, SEEK_SET);
    if (ok) { bytes.resize(size); ok = std::fread(bytes.data(), 1, size, file) == size; }
    if (file) ok = std::fclose(file) == 0 && ok;
    return ok;
}
struct Record {
    yyjson_mut_doc* doc = nullptr;
    yyjson_mut_val* data = nullptr;
    std::string base;
    ~Record() { yyjson_mut_doc_free(doc); }
    bool load(const std::string& id) {
        if (!identifier(id) || folder().empty()) return false;
        base = folder() + '/' + id;
        std::string bytes;
        if (!read(base + ".agent.json", 128 * 1024, bytes)) return false;
        auto* original = yyjson_read(bytes.data(), bytes.size(), 0);
        if (original) { doc = yyjson_doc_mut_copy(original, nullptr); yyjson_doc_free(original); }
        data = doc ? yyjson_mut_doc_get_root(doc) : nullptr;
        return yyjson_mut_is_obj(data) && id == text(data, "id") && yyjson_mut_is_obj(yyjson_mut_obj_get(data, "agent_report"));
    }
    void value(const char* key, const char* value) {
        yyjson_mut_obj_remove_str(data, key);
        yyjson_mut_obj_add_strcpy(doc, data, key, value);
    }
    bool save(bool initial = false) {
        size_t size = 0; char* bytes = yyjson_mut_write(doc, 0, &size);
        const std::string path = base + ".agent.json", temporary = base + ".agent.pending";
        HANDLE file = bytes && size <= 128 * 1024 ? CreateFileA(temporary.c_str(), GENERIC_WRITE, 0, nullptr,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr) : INVALID_HANDLE_VALUE;
        DWORD written = 0;
        bool ok = file != INVALID_HANDLE_VALUE && WriteFile(file, bytes, (DWORD)size, &written, nullptr) && written == size && FlushFileBuffers(file);
        if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
        if (ok) ok = MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_WRITE_THROUGH | (initial ? 0 : MOVEFILE_REPLACE_EXISTING));
        if (file != INVALID_HANDLE_VALUE && !ok) DeleteFileA(temporary.c_str());
        std::free(bytes); return ok;
    }
    NativeCommunityUpload upload() const {
        NativeCommunityUpload result{};
        std::snprintf(result.id, sizeof(result.id), "%s", text(data, "id"));
        std::snprintf(result.image, sizeof(result.image), "%s.png", base.c_str());
        std::snprintf(result.context, sizeof(result.context), "%s.context.json", base.c_str());
        std::snprintf(result.metadata, sizeof(result.metadata), "%s.notes.json", base.c_str());
        return result;
    }
};
std::vector<std::string> records()
{
    std::vector<std::string> ids;
    if (folder().empty()) return ids;
    WIN32_FIND_DATAA entry;
    HANDLE search = FindFirstFileA((folder() + "/*.agent.json").c_str(), &entry);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            std::string name = entry.cFileName;
            if (!(entry.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) && name.size() == 47 && identifier(name.substr(0, 36)))
                ids.push_back(name.substr(0, 36));
        } while (FindNextFileA(search, &entry));
        FindClose(search);
    }
    return ids;
}
bool digest(const NativeCommunityUpload& upload, std::string& result, bool include_image = true, const char* expected_image = nullptr)
{
    std::string parts, bytes;
    const char* paths[] = {upload.metadata, upload.image, upload.context};
    const size_t limits[] = {512 * 1024, 16 * 1024 * 1024, 8 * 1024 * 1024};
    for (unsigned i = 0; i < 3; ++i) {
        if (i == 1 && !include_image) continue;
        NativeContentId id{};
        if (!read(paths[i], limits[i], bytes) || bytes.empty() || !nativeContent_Hash(bytes.data(), bytes.size(), &id)) return false;
        if (i == 1 && expected_image) {
            char hex[65]; nativeContentStore_IdHex(&id, hex);
            if (std::strcmp(hex, expected_image)) return false;
        }
        parts.append((const char*)id.bytes, sizeof(id.bytes));
    }
    NativeContentId hash{}; char hex[65];
    if (!nativeContent_Hash(parts.data(), parts.size(), &hash)) return false;
    nativeContentStore_IdHex(&hash, hex); result = hex; return true;
}
bool ready(Record& record)
{
    auto upload = record.upload();
    AuroraPresentedCapture capture{}; aurora_presented_capture_status(&capture);
    const bool same = pending_capture == text(record.data, "id") &&
        capture.capture_id == yyjson_mut_get_uint(yyjson_mut_obj_get(record.data, "capture_id"));
    if (same && capture.status == AURORA_CAPTURE_PENDING) return false;
    if (!yyjson_mut_get_bool(yyjson_mut_obj_get(record.data, "image_complete"))) {
        // File existence after a process exit cannot prove that GPU readback
        // finished. Persist completion while the capture owner can confirm it.
        if (!same || capture.status != AURORA_CAPTURE_READY) return false;
        std::string bytes; NativeContentId hash{}; char hex[65];
        if (!read(upload.image, 16 * 1024 * 1024, bytes) || bytes.empty() || !nativeContent_Hash(bytes.data(), bytes.size(), &hash)) return false;
        nativeContentStore_IdHex(&hash, hex); record.value("image_hash", hex);
        yyjson_mut_obj_remove_str(record.data, "image_complete");
        yyjson_mut_obj_add_bool(record.doc, record.data, "image_complete", true);
        if (!record.save()) return false;
    }
    return ordinary(upload.image) && ordinary(upload.context) && ordinary(upload.metadata);
}
void describe(Record& record, Reply& reply)
{
    const bool attachments_ready = ready(record);
    const char* status = text(record.data, "status");
    // A process exit cannot prove whether the server stored an upload. Retrying
    // the sealed files under the same ID resolves that uncertainty safely.
    if (!std::strcmp(status, "sending") && in_flight != text(record.data, "id")) status = "retry";
    reply.result = yyjson_mut_val_mut_copy(reply.document, record.data);
    yyjson_mut_obj_remove_str(reply.result, "status"); add_string(reply, reply.result, "status", status);
    auto upload = record.upload();
    add_string(reply, reply.result, "screenshot", upload.image);
    add_string(reply, reply.result, "context", upload.context);
    add_string(reply, reply.result, "notes", upload.metadata);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "attachments_ready", attachments_ready);
    yyjson_mut_obj_add_bool(reply.document, reply.result, "stored", !std::strcmp(status, "stored"));
    if (in_flight == text(record.data, "id")) {
        yyjson_mut_obj_add_uint(reply.document, reply.result, "sent_bytes", receipt.sent);
        yyjson_mut_obj_add_uint(reply.document, reply.result, "total_bytes", receipt.total);
    }
}
void pin(NativeFeedbackDraft& draft, const std::string& message, float x = .05f, float y = .1f)
{
    if (message.empty() || draft.count == 64) return;
    auto& note = draft.pins[draft.count++];
    note = {++draft.next_pin, x, y, {}};
    // Server notes are limited to 2048 UTF-16 units. Full diagnostics remain in
    // the context; keep each pin under that bound, ending on a UTF-8 boundary.
    size_t size = std::min<size_t>(message.size(), 1900);
    while (size < message.size() && size && ((unsigned char)message[size] & 0xc0) == 0x80) --size;
    std::memcpy(note.text, message.data(), size); note.text[size] = 0;
}
void prepare(yyjson_val* args, yyjson_mut_val* snapshot, Reply& reply)
{
    if (nativeFeedback_IsOpen()) { reply.fail("feedback_busy", "Finish the open F2 report before capturing another report."); return; }
    if (folder().empty()) { reply.fail("capture_failed", "The local feedback folder is unavailable."); return; }
    auto ids = records();
    // Task keys suppress duplicates within this installation. Report IDs stay
    // random, so two players using the same agent prompt cannot collide online.
    for (const auto& id : ids) {
        Record previous;
        if (!previous.load(id)) continue;
        auto* report = yyjson_mut_obj_get(previous.data, "agent_report");
        if (!std::strcmp(text(report, "task_id"), text(args, "task_id")) && !std::strcmp(text(report, "issue_key"), text(args, "issue_key"))) {
            describe(previous, reply); yyjson_mut_obj_add_bool(reply.document, reply.result, "duplicate", true); return;
        }
    }
    uint64_t total = 0;
    WIN32_FIND_DATAA found;
    HANDLE search = FindFirstFileA((folder() + "/*").c_str(), &found);
    if (search != INVALID_HANDLE_VALUE) {
        do { if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) total += (uint64_t(found.nFileSizeHigh) << 32) | found.nFileSizeLow; } while (FindNextFileA(search, &found));
        FindClose(search);
    }
    if (ids.size() >= MaxReports || total + 25 * 1024 * 1024 > MaxLocalBytes) {
        reply.fail("feedback_capacity", "Local feedback storage is full. Inspect and discard finished agent reports before preparing more."); return;
    }
    Record record;
    record.doc = yyjson_mut_doc_new(nullptr);
    if (!record.doc) { reply.fail("capture_failed", "Could not allocate the report record."); return; }
    record.data = yyjson_mut_obj(record.doc); yyjson_mut_doc_set_root(record.doc, record.data);
    auto* agent = yyjson_mut_obj(record.doc);
    yyjson_mut_obj_add_val(record.doc, record.data, "agent_report", agent);
    yyjson_mut_obj_add_str(record.doc, agent, "author", "agent");
    for (const char* field : {"task_id", "issue_key", "category", "summary", "operation", "expected", "observed", "reproduction", "diagnostics", "capability", "alternatives"})
        yyjson_mut_obj_add_strcpy(record.doc, agent, field, text(args, field));
    auto* state = yyjson_mut_obj(record.doc); yyjson_mut_obj_add_val(record.doc, agent, "captured_workshop", state);
    for (const char* field : {"revision", "fighter_revision", "member", "workspace", "selected_source", "selected_action", "frame", "source_revision", "build_identity", "build",
                             "saved_disk_revision", "loaded_preview_package_id", "preview_fighter_revision", "built_package_id", "installed_package_id", "preview_ready", "testing"})
        if (auto* value = yyjson_mut_obj_get(snapshot, field)) yyjson_mut_obj_add_val(record.doc, state, field, yyjson_mut_val_mut_copy(record.doc, value));
    auto* immutable = yyjson_mut_val_imut_copy(agent, nullptr);
    auto draft = std::make_unique<NativeFeedbackDraft>(); NativeCommunityUpload upload{}; bool complete = false;
    bool captured = immutable && NativeFeedbackCapture::create(*draft, upload, false, complete, yyjson_doc_get_root(immutable));
    yyjson_doc_free(immutable);
    if (!captured) { reply.fail("capture_failed", "Could not reserve the displayed frame. Wait for the current capture to finish, then retry this task and issue key."); return; }
    struct CaptureFiles {
        const NativeFeedbackDraft& draft;
        const NativeCommunityUpload& upload;
        bool published = false;
        ~CaptureFiles() {
            if (published) return;
            AuroraPresentedCapture current{}; aurora_presented_capture_status(&current);
            if (current.capture_id == draft.image.capture_id) aurora_release_presented_capture();
            DeleteFileA(upload.metadata); DeleteFileA(upload.context); DeleteFileA(upload.image);
        }
    } files{*draft, upload};
    if (!complete) {
        auto* partial = yyjson_mut_val_imut_copy(agent, nullptr);
        bool saved = partial && NativeFeedbackCapture::fallback(*draft, yyjson_doc_get_root(partial));
        yyjson_doc_free(partial);
        if (!saved) { reply.fail("capture_failed", "Could not save capture-time diagnostics."); return; }
    }
    pin(*draft, std::string("Agent ") + text(args, "category") + ": " + text(args, "summary"));
    for (const char* field : {"expected", "observed", "reproduction", "capability", "alternatives", "diagnostics"})
        if (*text(args, field)) pin(*draft, std::string(field) + ": " + text(args, field), .05f, .15f + .05f * draft->count);
    size_t i, count; yyjson_val* note;
    yyjson_arr_foreach(yyjson_obj_get(args, "pins"), i, count, note)
        pin(*draft, text(note, "text"), float(yyjson_get_num(yyjson_obj_get(note, "x"))), float(yyjson_get_num(yyjson_obj_get(note, "y"))));
    if (!NativeFeedbackCapture::notes(*draft, upload, complete)) { reply.fail("capture_failed", "Could not save the report notes."); return; }
    const std::string id = draft->id;
    record.base = folder() + '/' + id;
    record.value("id", id.c_str()); record.value("capture_at", draft->captured_at); record.value("status", "prepared");
    yyjson_mut_obj_add_uint(record.doc, record.data, "capture_id", draft->image.capture_id);
    yyjson_mut_obj_add_uint(record.doc, record.data, "displayed_frame", draft->image.displayed_frame);
    yyjson_mut_obj_add_bool(record.doc, record.data, "context_complete", complete);
    yyjson_mut_obj_add_bool(record.doc, record.data, "image_complete", false);
    yyjson_mut_obj_add_bool(record.doc, record.data, "fighter_files_attached", false);
    std::string evidence;
    if (!digest(upload, evidence, false)) { reply.fail("capture_failed", "Could not fingerprint the captured context and notes."); return; }
    record.value("evidence_hash", evidence.c_str());
    if (!record.save(true)) { reply.fail("capture_failed", "The capture was saved, but its local record could not be committed."); return; }
    files.published = true; pending_capture = id;
    describe(record, reply);
}
}

void update()
{
    if (in_flight.empty()) return;
    nativeCommunity_PollReceipt(receipt);
    if (in_flight != receipt.id || receipt.busy || !receipt.revision) return;
    Record record;
    if (record.load(in_flight)) {
        record.value("status", receipt.stored ? "stored" : "retry"); record.value("error", receipt.error);
        // Preserve the inflight identity until the durable receipt is written.
        if (!record.save()) return;
    }
    in_flight.clear();
}

void list(Reply& reply)
{
    update();
    auto* rows = yyjson_mut_arr(reply.document); yyjson_mut_obj_add_val(reply.document, reply.result, "reports", rows);
    for (const auto& id : records()) {
        Record record;
        if (!record.load(id)) continue;
        Reply item(reply.document); describe(record, item); yyjson_mut_arr_append(rows, item.result);
    }
    yyjson_mut_obj_add_uint(reply.document, reply.result, "capacity", MaxReports);
    yyjson_mut_obj_add_uint(reply.document, reply.result, "storage_limit_bytes", MaxLocalBytes);
}

void handle(const char* method, yyjson_val* arguments, yyjson_mut_val* snapshot, Reply& reply)
{
    update();
    if (!std::strcmp(method, "feedback.prepare")) { prepare(arguments, snapshot, reply); return; }
    if (!std::strcmp(method, "feedback.list")) { list(reply); return; }
    Record record;
    if (!record.load(text(arguments, "report_id"))) { reply.fail("unknown_report", "Choose a report ID returned by feedback.prepare or feedback.list."); return; }
    const std::string id = text(record.data, "id");
    if (!std::strcmp(method, "feedback.submit")) {
        if (!std::strcmp(text(record.data, "status"), "stored") || in_flight == id) { describe(record, reply); return; }
        if (!in_flight.empty()) { reply.fail("feedback_busy", "Wait for the current feedback upload, then submit this saved report."); return; }
        if (!ready(record)) { reply.fail("capture_pending", "The original screenshot is pending or unavailable. Inspect the saved capture before retrying."); return; }
        const auto upload = record.upload(); std::string current, evidence;
        if (!digest(upload, evidence, false) || evidence != text(record.data, "evidence_hash")) {
            reply.fail("capture_changed", "Capture-time diagnostics or notes changed. Preserve the original report files."); return;
        }
        if (!digest(upload, current, true, text(record.data, "image_hash"))) { reply.fail("invalid_capture", "A captured attachment changed, is unreadable or exceeds the F2 upload limit."); return; }
        const std::string sealed = text(record.data, "attachment_hash");
        if (!sealed.empty() && sealed != current) { reply.fail("capture_changed", "A sealed attachment changed. Retry must preserve the original report bytes."); return; }
        record.value("attachment_hash", current.c_str()); record.value("status", "sending"); record.value("error", "");
        if (!record.save()) { reply.fail("receipt_failed", "Could not save the report's retry identity. Nothing was uploaded."); return; }
        if (!NativeFeedbackCapture::submit(upload)) {
            record.value("status", "retry"); record.value("error", "The F2 feedback uploader is busy or unavailable."); record.save();
        } else { in_flight = id; receipt = {}; }
    } else if (!std::strcmp(method, "feedback.discard")) {
        if (in_flight == id) { reply.fail("feedback_busy", "Wait for this upload to finish before discarding its local copy."); return; }
        if (!ready(record)) {
            AuroraPresentedCapture capture{}; aurora_presented_capture_status(&capture);
            if (pending_capture == id && capture.capture_id == yyjson_mut_get_uint(yyjson_mut_obj_get(record.data, "capture_id")) && capture.status == AURORA_CAPTURE_PENDING) {
                reply.fail("capture_pending", "Wait for screenshot writing before discarding the local report."); return;
            }
        }
        auto upload = record.upload(); bool removed = true;
        for (const auto& path : {std::string(upload.metadata), std::string(upload.context), std::string(upload.image), record.base + ".agent.json"}) {
            if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) continue;
            if (!ordinary(path) || !DeleteFileA(path.c_str())) { removed = false; break; }
        }
        if (!removed) { reply.fail("discard_failed", "Could not remove every local report file. Unknown files were preserved."); return; }
        add_string(reply, reply.result, "report_id", id); add_string(reply, reply.result, "status", "discarded_local"); return;
    }
    describe(record, reply);
}
}
