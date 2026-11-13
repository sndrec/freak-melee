#include "native_community.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <yyjson.h>
#include <cstdio>
#include <cstring>
#include <new>

namespace {
    constexpr unsigned response_limit = 512 * 1024;
    enum class Operation { Feed, Page, Article, Announcements, Announcement };
    struct Request { Operation operation; char value[96]; };
    struct Worker {
        SRWLOCK lock = SRWLOCK_INIT;
        HANDLE wake = nullptr, thread = nullptr;
        HINTERNET session = nullptr, connection = nullptr;
        bool stop = false, queued = false, secure = true, upload_queued = false;
        bool announcement_queued = false;
        Request request{};
        Request announcement_request{};
        NativeCommunityUpload upload{};
        NativeCommunityReceipt receipt{};
        NativeCommunityView view{};
        char origin[512], cache[1024], etag[128]{};
        char receive[response_limit + 1];
        alignas(16) char json_pool[4 * 1024 * 1024];
    };
    Worker* worker;

    bool string(yyjson_val* value, char* output, size_t capacity, bool optional = false)
    {
        if (optional && yyjson_is_null(value)) { *output = 0; return true; }
        const char* text = yyjson_get_str(value);
        const size_t length = yyjson_get_len(value);
        if (!text || length >= capacity || std::memchr(text, 0, length)) return false;
        std::memcpy(output, text, length); output[length] = 0;
        return true;
    }
    bool identifier(const char* id)
    {
        if (std::strlen(id) != 36) return false;
        for (unsigned i = 0; i < 36; ++i)
            if (!std::strchr("0123456789abcdef-", id[i])) return false;
        return true;
    }
    bool headline(yyjson_val* value, NativeNewsHeadline& result)
    {
        yyjson_val* date = yyjson_obj_get(value, "published_at");
        return string(yyjson_obj_get(value, "id"), result.id, sizeof(result.id)) && identifier(result.id) &&
            string(yyjson_obj_get(value, "title"), result.title, sizeof(result.title)) &&
            string(yyjson_obj_get(value, "ticker"), result.ticker, sizeof(result.ticker)) &&
            yyjson_is_uint(date) && (result.published_at = yyjson_get_uint(date), true);
    }
    yyjson_doc* json(Worker& w, char* bytes, size_t size)
    {
        yyjson_alc allocator;
        if (!yyjson_alc_pool_init(&allocator, w.json_pool, sizeof(w.json_pool))) return nullptr;
        return yyjson_read_opts(bytes, size, 0, &allocator, nullptr);
    }
    bool cursor(yyjson_val* value, char (&output)[96])
    {
        if (!string(value, output, sizeof(output), true)) return false;
        for (const char* c = output; *c; ++c) if (!std::strchr("0123456789abcdef,-", *c)) return false;
        return true;
    }
    bool announcement(yyjson_val* value, NativeAnnouncement& n)
    {
        yyjson_val* expiry = yyjson_obj_get(value, "expires_at");
        const bool valid = string(yyjson_obj_get(value, "id"), n.id, sizeof(n.id)) && identifier(n.id) &&
            string(yyjson_obj_get(value, "delivery_id"), n.delivery_id, sizeof(n.delivery_id)) && identifier(n.delivery_id) &&
            string(yyjson_obj_get(value, "title"), n.title, sizeof(n.title)) &&
            string(yyjson_obj_get(value, "body"), n.body, sizeof(n.body)) &&
            string(yyjson_obj_get(value, "news_id"), n.news_id, sizeof(n.news_id), true) &&
            (!*n.news_id || identifier(n.news_id)) && (yyjson_is_null(expiry) || yyjson_is_uint(expiry));
        if (valid) n.expires_at = yyjson_get_uint(expiry);
        return valid;
    }
    bool announcements(yyjson_val* rows, NativeAnnouncement (&entries)[64], unsigned& length)
    {
        if (!yyjson_is_arr(rows) || yyjson_arr_size(rows) > 64) return false;
        size_t i, count; yyjson_val* row;
        yyjson_arr_foreach(rows, i, count, row) if (!announcement(row, entries[i])) return false;
        length = unsigned(yyjson_arr_size(rows)); return true;
    }
    bool feed(yyjson_val* root, NativeCommunityFeed& result)
    {
        yyjson_val* title = yyjson_obj_get(root, "headline");
        if (!yyjson_is_null(title) && !headline(title, result.headline)) return false;
        if (!announcements(yyjson_obj_get(root, "announcements"), result.announcements, result.count) ||
            !cursor(yyjson_obj_get(root, "announcements_next"), result.announcements_next)) return false;
        result.available = true;
        return true;
    }
    bool page(yyjson_val* root, NativeNewsPage& result)
    {
        yyjson_val* rows = yyjson_obj_get(root, "entries");
        if (!yyjson_is_arr(rows) || yyjson_arr_size(rows) > 25 ||
            !string(yyjson_obj_get(root, "next"), result.next, sizeof(result.next), true)) return false;
        for (const char* c = result.next; *c; ++c)
            if (!std::strchr("0123456789abcdef,-", *c)) return false;
        size_t i, count; yyjson_val* row;
        yyjson_arr_foreach(rows, i, count, row) if (!headline(row, result.entries[i])) return false;
        result.count = unsigned(yyjson_arr_size(rows));
        result.available = true;
        return true;
    }
    bool endpoint(Worker& w, const char* url, wchar_t (&host)[256], INTERNET_PORT& port)
    {
        wchar_t wide[512];
        if (!url || std::strlen(url) >= sizeof(w.origin) ||
            !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, url, -1, wide, 512)) return false;
        URL_COMPONENTS parts{}; parts.dwStructSize = sizeof(parts);
        parts.lpszHostName = host; parts.dwHostNameLength = 256;
        parts.dwUserNameLength = parts.dwPasswordLength = parts.dwUrlPathLength = parts.dwExtraInfoLength = DWORD(-1);
        if (!WinHttpCrackUrl(wide, 0, 0, &parts) || parts.dwUserNameLength || parts.dwPasswordLength ||
            parts.dwExtraInfoLength || (parts.dwUrlPathLength &&
                (parts.dwUrlPathLength != 1 || *parts.lpszUrlPath != L'/'))) return false;
        w.secure = parts.nScheme == INTERNET_SCHEME_HTTPS;
        if (!w.secure && !(parts.nScheme == INTERNET_SCHEME_HTTP &&
            (!wcscmp(host, L"localhost") || !wcscmp(host, L"127.0.0.1") || !wcscmp(host, L"[::1]")))) return false;
        port = parts.nPort;
        std::snprintf(w.origin, sizeof(w.origin), "%s", url);
        const size_t length = std::strlen(w.origin);
        if (length && w.origin[length - 1] == '/') w.origin[length - 1] = 0;
        return true;
    }

    void load_cache(Worker& w)
    {
        FILE* file = std::fopen(w.cache, "rb");
        if (!file) return;
        char origin[512];
        const bool first = std::fgets(origin, sizeof(origin), file) != nullptr;
        if (first) origin[std::strcspn(origin, "\r\n")] = 0;
        const size_t size = first ? std::fread(w.receive, 1, response_limit + 1, file) : 0;
        const bool valid = first && !std::strcmp(origin, w.origin) && size <= response_limit && !std::ferror(file);
        std::fclose(file);
        if (!valid) return;
        yyjson_doc* doc = json(w, w.receive, size);
        if (doc && feed(yyjson_doc_get_root(doc), w.view.feed)) {
            w.view.feed.offline = true;
            ++w.view.feed_revision;
        } else w.view.feed = {};
        yyjson_doc_free(doc);
    }
    void save_cache(Worker& w, size_t size)
    {
        char temporary[1030];
        if (std::snprintf(temporary, sizeof(temporary), "%s.tmp", w.cache) >= sizeof(temporary)) return;
        FILE* file = std::fopen(temporary, "wb");
        if (!file) return;
        bool ok = std::fprintf(file, "%s\n", w.origin) > 0 && std::fwrite(w.receive, 1, size, file) == size;
        ok = std::fclose(file) == 0 && ok;
        if (!ok || !MoveFileExA(temporary, w.cache, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            DeleteFileA(temporary);
    }

    // The worker owns HTTP and parsing buffers. Menus only exchange a bounded
    // request and completed view under the lock, never wait for network I/O.
    bool fetch(Worker& w, const Request& command, size_t& received, DWORD& status, char (&etag)[128])
    {
        wchar_t path[256], headers[256];
        if (command.operation == Operation::Feed) wcscpy_s(path, L"/v1/feed");
        else if (command.operation == Operation::Article) swprintf_s(path, L"/v1/news/%hs", command.value);
        else if (command.operation == Operation::Page) swprintf_s(path, L"/v1/news%s%hs", *command.value ? L"?before=" : L"", command.value);
        else if (command.operation == Operation::Announcement) swprintf_s(path, L"/v1/announcements/%hs", command.value);
        else swprintf_s(path, L"/v1/announcements?before=%hs", command.value);
        HINTERNET request = WinHttpOpenRequest(w.connection, L"GET", path, nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, w.secure ? WINHTTP_FLAG_SECURE : 0);
        if (!request) return false;
        DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        DWORD disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
        bool ok = WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)) &&
            WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled));
        swprintf_s(headers, L"Accept: application/json\r\n%hs%hs%hs",
            command.operation == Operation::Feed && *w.etag ? "If-None-Match: " : "",
            command.operation == Operation::Feed ? w.etag : "",
            command.operation == Operation::Feed && *w.etag ? "\r\n" : "");
        ok = ok && WinHttpSendRequest(request, headers, DWORD(-1), nullptr, 0, 0, 0) &&
            WinHttpReceiveResponse(request, nullptr);
        DWORD size = sizeof(status);
        ok = ok && WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
        wchar_t tag[128]; size = sizeof(tag);
        if (ok && WinHttpQueryHeaders(request, WINHTTP_QUERY_ETAG, WINHTTP_HEADER_NAME_BY_INDEX,
            tag, &size, WINHTTP_NO_HEADER_INDEX)) {
            // Accept the short quoted hex digests generated by this service.
            bool valid = wcslen(tag) < sizeof(etag);
            for (const wchar_t* c = tag; *c; ++c)
                if (!wcschr(L"\"0123456789abcdef", *c)) valid = false;
            if (valid) WideCharToMultiByte(CP_UTF8, 0, tag, -1, etag, sizeof(etag), nullptr, nullptr);
        }
        received = 0;
        const ULONGLONG began = GetTickCount64();
        while (ok && status != 304) {
            DWORD available = 0, count = 0;
            if (GetTickCount64() - began > 15000 || !WinHttpQueryDataAvailable(request, &available)) { ok = false; break; }
            if (!available) break;
            if (available > response_limit - received ||
                !WinHttpReadData(request, w.receive + received, available, &count) || !count) { ok = false; break; }
            received += count;
        }
        WinHttpCloseHandle(request);
        w.receive[received] = 0;
        return ok;
    }

    DWORD WINAPI run(void* argument)
    {
        auto& w = *static_cast<Worker*>(argument);
        uint64_t next_feed = 0;
        // Parsing results stay on this worker's stack until published.
        NativeCommunityFeed fresh_feed;
        NativeNewsPage fresh_page;
        NativeNewsArticle fresh_article;
        NativeAnnouncementPage fresh_notices;
        NativeAnnouncementCheck fresh_notice;
        for (;;) {
            Request command{};
            NativeCommunityUpload upload{};
            AcquireSRWLockExclusive(&w.lock);
            const bool stop = w.stop, queued = w.queued;
            const bool sending = w.upload_queued;
            const bool background = !queued && w.announcement_queued;
            if (sending) { upload = w.upload; w.upload_queued = false; }
            else if (queued) { command = w.request; w.queued = false; }
            else if (background) { command = w.announcement_request; w.announcement_queued = false; }
            ReleaseSRWLockExclusive(&w.lock);
            if (stop) break;
            if (sending) {
                NativeCommunityReceipt receipt{};
                nativeCommunity_SendReport(w.connection, w.secure, upload, receipt,
                    [](void* owner, uint64_t sent, uint64_t total) {
                        auto& target = *static_cast<Worker*>(owner);
                        AcquireSRWLockExclusive(&target.lock);
                        target.receipt.sent = sent; target.receipt.total = total; ++target.receipt.revision;
                        const bool running = !target.stop;
                        ReleaseSRWLockExclusive(&target.lock);
                        return running;
                    }, &w);
                AcquireSRWLockExclusive(&w.lock);
                receipt.revision = w.receipt.revision + 1; w.receipt = receipt;
                ReleaseSRWLockExclusive(&w.lock);
                continue;
            }
            const uint64_t now = GetTickCount64();
            if (!queued && !background && now < next_feed) {
                WaitForSingleObject(w.wake, DWORD(next_feed - now));
                continue;
            }
            if (!queued && !background) command.operation = Operation::Feed;
            size_t received = 0; DWORD status = 0; char tag[128]{};
            bool ok = fetch(w, command, received, status, tag);
            yyjson_doc* doc = ok && status == 200 ? json(w, w.receive, received) : nullptr;
            yyjson_val* root = doc ? yyjson_doc_get_root(doc) : nullptr;
            const bool unchanged = ok && status == 304 && command.operation == Operation::Feed;
            if (command.operation == Operation::Feed) {
                fresh_feed = {};
                ok = unchanged || (root && feed(root, fresh_feed));
                if (ok && !unchanged) save_cache(w, received);
                if (ok && *tag) std::memcpy(w.etag, tag, sizeof(tag));
                next_feed = GetTickCount64() + 30000;
            } else if (command.operation == Operation::Page) {
                fresh_page = {};
                ok = root && page(root, fresh_page);
            } else if (command.operation == Operation::Article) {
                fresh_article = {};
                ok = root && headline(root, fresh_article.headline) &&
                    !std::strcmp(fresh_article.headline.id, command.value) &&
                    string(yyjson_obj_get(root, "body"), fresh_article.body, sizeof(fresh_article.body));
                fresh_article.available = ok;
            } else if (command.operation == Operation::Announcements) {
                fresh_notices = {};
                std::snprintf(fresh_notices.before, sizeof(fresh_notices.before), "%s", command.value);
                ok = root && announcements(yyjson_obj_get(root, "entries"), fresh_notices.entries, fresh_notices.count) &&
                    cursor(yyjson_obj_get(root, "next"), fresh_notices.next);
                fresh_notices.available = ok;
            } else {
                fresh_notice = {};
                std::snprintf(fresh_notice.delivery_id, sizeof(fresh_notice.delivery_id), "%s", command.value);
                const bool missing = ok && status == 404;
                ok = missing || (root && announcement(root, fresh_notice.entry) &&
                    !std::strcmp(fresh_notice.entry.delivery_id, command.value));
                fresh_notice.available = ok; fresh_notice.found = ok && !missing;
            }
            yyjson_doc_free(doc);
            AcquireSRWLockExclusive(&w.lock);
            if (command.operation == Operation::Feed) {
                if (ok && !unchanged) w.view.feed = fresh_feed;
                w.view.feed.offline = !ok;
                std::snprintf(w.view.feed.error, sizeof(w.view.feed.error), "%s", ok ? "" : "News is offline. Please try again shortly.");
                ++w.view.feed_revision;
            } else if (command.operation == Operation::Page) {
                if (ok) w.view.page = fresh_page;
                std::snprintf(w.view.page.error, sizeof(w.view.page.error), "%s", ok ? "" : "Could not load this news page. Please try again.");
            } else if (command.operation == Operation::Article) {
                if (ok) w.view.article = fresh_article;
                std::snprintf(w.view.article.error, sizeof(w.view.article.error), "%s", ok ? "" : status == 404 ?
                    "This article is unavailable." : "Could not load the article. Please try again.");
            } else if (command.operation == Operation::Announcements) {
                const unsigned revision = w.view.notices.revision + 1;
                if (ok || std::strcmp(w.view.notices.before, command.value)) w.view.notices = fresh_notices;
                w.view.notices.revision = revision;
                if (!ok) std::snprintf(w.view.notices.error, sizeof(w.view.notices.error), "Could not refresh announcements.");
            } else {
                const unsigned revision = w.view.notice.revision + 1;
                w.view.notice = fresh_notice; w.view.notice.revision = revision;
                if (!ok) std::snprintf(w.view.notice.error, sizeof(w.view.notice.error), "Could not refresh this announcement.");
            }
            if (queued) w.view.busy = false;
            if (background) w.view.announcement_busy = false;
            ++w.view.revision;
            ReleaseSRWLockExclusive(&w.lock);
        }
        return 0;
    }
    bool submit(Operation operation, const char* value)
    {
        if (!worker || (value && std::strlen(value) >= sizeof(Request::value))) return false;
        auto& w = *worker;
        AcquireSRWLockExclusive(&w.lock);
        const bool ok = !w.stop && !w.queued && !w.view.busy;
        if (ok) {
            w.request = {}; w.request.operation = operation;
            std::snprintf(w.request.value, sizeof(w.request.value), "%s", value ? value : "");
            w.queued = w.view.busy = true; ++w.view.revision;
            SetEvent(w.wake);
        }
        ReleaseSRWLockExclusive(&w.lock);
        return ok;
    }
    bool request_announcements(Operation operation, const char* value)
    {
        if (!worker || !value || std::strlen(value) >= sizeof(Request::value)) return false;
        auto& w = *worker; AcquireSRWLockExclusive(&w.lock);
        const bool ok = !w.stop && !w.view.announcement_busy;
        if (ok) {
            w.announcement_request = {}; w.announcement_request.operation = operation;
            std::snprintf(w.announcement_request.value, sizeof(w.announcement_request.value), "%s", value);
            w.announcement_queued = w.view.announcement_busy = true; ++w.view.revision; SetEvent(w.wake);
        }
        ReleaseSRWLockExclusive(&w.lock); return ok;
    }
}

bool nativeCommunity_Start(const char* root, const char* url)
{
    if (worker) return true;
    Worker* w = new (std::nothrow) Worker;
    if (!w) return false;
    wchar_t host[256]{}; INTERNET_PORT port;
    bool ok = root && endpoint(*w, url, host, port) &&
        std::snprintf(w->cache, sizeof(w->cache), "%s/community-feed.json", root) < sizeof(w->cache);
    if (ok) {
        w->session = WinHttpOpen(L"FreakMelee/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (w->session) {
            WinHttpSetTimeouts(w->session, 3000, 3000, 5000, 5000);
            w->connection = WinHttpConnect(w->session, host, port, 0);
        }
        w->wake = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (w->connection && w->wake) {
            load_cache(*w); w->view.revision = 1;
            w->thread = CreateThread(nullptr, 0, run, w, 0, nullptr);
        }
    }
    if (!w->thread) {
        if (w->wake) CloseHandle(w->wake);
        if (w->connection) WinHttpCloseHandle(w->connection);
        if (w->session) WinHttpCloseHandle(w->session);
        delete w; return false;
    }
    worker = w;
    return true;
}
bool nativeCommunity_Poll(NativeCommunityView& view)
{
    if (!worker) return false;
    AcquireSRWLockShared(&worker->lock);
    const bool changed = view.revision != worker->view.revision;
    if (changed) view = worker->view;
    ReleaseSRWLockShared(&worker->lock);
    return changed;
}
bool nativeCommunity_RequestPage(const char* before)
{
    for (const char* c = before; c && *c; ++c) if (!std::strchr("0123456789abcdef,-", *c)) return false;
    return submit(Operation::Page, before);
}
bool nativeCommunity_RequestArticle(const char* id)
{
    return id && identifier(id) && submit(Operation::Article, id);
}
bool nativeCommunity_Refresh() { return submit(Operation::Feed, nullptr); }
bool nativeCommunity_RequestAnnouncements(const char* before)
{
    if (!before || !*before) return false;
    for (const char* c = before; *c; ++c) if (!std::strchr("0123456789abcdef,-", *c)) return false;
    return request_announcements(Operation::Announcements, before);
}
bool nativeCommunity_CheckAnnouncement(const char* id)
{ return id && identifier(id) && request_announcements(Operation::Announcement, id); }
bool nativeCommunity_RequestUpload(const NativeCommunityUpload& upload)
{
    if (!worker || !identifier(upload.id) || !*upload.metadata || !*upload.image || !*upload.context) return false;
    auto& w = *worker;
    AcquireSRWLockExclusive(&w.lock);
    const bool ok = !w.stop && !w.receipt.busy && !w.upload_queued;
    if (ok) {
        w.upload = upload; w.upload_queued = true;
        const unsigned revision = w.receipt.revision + 1;
        w.receipt = {}; w.receipt.revision = revision; w.receipt.busy = true;
        std::snprintf(w.receipt.id, sizeof(w.receipt.id), "%s", upload.id);
        SetEvent(w.wake);
    }
    ReleaseSRWLockExclusive(&w.lock); return ok;
}
bool nativeCommunity_PollReceipt(NativeCommunityReceipt& receipt)
{
    if (!worker) return false;
    AcquireSRWLockShared(&worker->lock);
    const bool changed = receipt.revision != worker->receipt.revision;
    if (changed) receipt = worker->receipt;
    ReleaseSRWLockShared(&worker->lock); return changed;
}
void nativeCommunity_Shutdown()
{
    if (!worker) return;
    auto& w = *worker;
    AcquireSRWLockExclusive(&w.lock); w.stop = true; SetEvent(w.wake); ReleaseSRWLockExclusive(&w.lock);
    WaitForSingleObject(w.thread, INFINITE);
    CloseHandle(w.thread); CloseHandle(w.wake);
    WinHttpCloseHandle(w.connection); WinHttpCloseHandle(w.session);
    delete worker; worker = nullptr;
}
