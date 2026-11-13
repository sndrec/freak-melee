#include "native_community_upload.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <yyjson.h>
#include <array>
#include <cstdio>
#include <cstring>

bool nativeCommunity_SendReport(void* connection, bool secure, const NativeCommunityUpload& upload,
    NativeCommunityReceipt& result, NativeCommunityProgress progress, void* owner)
{
    result = {}; std::snprintf(result.id, sizeof(result.id), "%s", upload.id);
    FILE* files[3]{}; uint32_t lengths[3]{};
    const char* paths[]{upload.metadata, upload.image, upload.context};
    const uint32_t limits[]{512 * 1024, 16 * 1024 * 1024, 8 * 1024 * 1024};
    bool ok = true;
    for (unsigned i = 0; i < 3 && ok; ++i) {
        files[i] = std::fopen(paths[i], "rb");
        const __int64 size = files[i] && !_fseeki64(files[i], 0, SEEK_END) ? _ftelli64(files[i]) : -1;
        ok = size > 0 && size <= limits[i] && !_fseeki64(files[i], 0, SEEK_SET);
        if (ok) lengths[i] = uint32_t(size);
    }
    HINTERNET request = nullptr;
    if (!ok) std::snprintf(result.error, sizeof(result.error), "A captured attachment is missing, empty, or too large. The report has been kept on this PC.");
    if (ok) {
        request = WinHttpOpenRequest(connection, L"POST", L"/v1/feedback", nullptr,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, secure ? WINHTTP_FLAG_SECURE : 0);
        ok = request != nullptr;
    }
    const uint32_t total = 20 + lengths[0] + lengths[1] + lengths[2];
    if (ok) {
        DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        DWORD disabled = WINHTTP_DISABLE_COOKIES | WINHTTP_DISABLE_AUTHENTICATION;
        ok = WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects)) &&
            WinHttpSetOption(request, WINHTTP_OPTION_DISABLE_FEATURE, &disabled, sizeof(disabled)) &&
            WinHttpSendRequest(request, L"Content-Type: application/octet-stream\r\nAccept: application/json\r\n",
                DWORD(-1), nullptr, 0, total, 0);
    }
    uint64_t sent = 0, reported = 0;
    const uint64_t began = GetTickCount64();
    uint64_t last_report = began;
    std::array<unsigned char, 128 * 1024> buffer;
    std::memcpy(buffer.data(), "FMREPORT", 8);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned byte = 0; byte < 4; ++byte) buffer[8+i*4+byte] = uint8_t(lengths[i] >> (byte*8));
    auto write = [&](const void* bytes, DWORD size) {
        DWORD written = 0;
        const bool valid = GetTickCount64() - began < 60000 &&
            WinHttpWriteData(request, bytes, size, &written) && written == size;
        if (valid) {
            sent += written;
            const uint64_t now = GetTickCount64();
            if (now - last_report >= 500 || sent == total) {
                if (!progress(owner, sent, total)) return false;
                last_report = now; reported = sent;
            }
        }
        return valid;
    };
    if (ok) ok = progress(owner, 0, total) && write(buffer.data(), 20);
    for (unsigned i = 0; i < 3 && ok; ++i) {
        uint32_t remaining = lengths[i];
        while (ok && remaining) {
            const DWORD count = remaining < buffer.size() ? remaining : DWORD(buffer.size());
            ok = std::fread(buffer.data(), 1, count, files[i]) == count && write(buffer.data(), count);
            remaining -= count;
        }
        // Files are finalized before publication to this worker. Reject any
        // accidental mutation rather than extending the submitted capture.
        if (ok) ok = std::fgetc(files[i]) == EOF && !std::ferror(files[i]);
    }
    for (FILE* file : files) if (file) std::fclose(file);
    DWORD status = 0, size = sizeof(status);
    ok = ok && WinHttpReceiveResponse(request, nullptr) &&
        WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    unsigned received = 0;
    while (ok) {
        DWORD available = 0, read = 0;
        if (GetTickCount64() - began >= 60000 || !WinHttpQueryDataAvailable(request, &available)) { ok = false; break; }
        if (!available) break;
        if (available >= buffer.size() - received || !WinHttpReadData(request, buffer.data() + received, available, &read) || !read) {
            ok = false; break;
        }
        received += read;
    }
    if (request) WinHttpCloseHandle(request);
    if (ok) {
        auto* doc = yyjson_read(reinterpret_cast<char*>(buffer.data()), received, 0);
        auto* root = doc ? yyjson_doc_get_root(doc) : nullptr;
        const char* id = yyjson_get_str(yyjson_obj_get(root, "id"));
        result.stored = (status == 200 || status == 201) && id && !std::strcmp(id, upload.id) &&
            yyjson_is_true(yyjson_obj_get(root, "stored"));
        if (!result.stored) {
            const char* error = yyjson_get_str(yyjson_obj_get(root, "error"));
            std::snprintf(result.error, sizeof(result.error), "%s", error ? error : "The service could not store this report. Please retry.");
        }
        yyjson_doc_free(doc);
    } else if (!*result.error) std::snprintf(result.error, sizeof(result.error), "Could not reach the feedback service. Retry sends the same captured report.");
    result.sent = sent; result.total = total;
    if (reported != sent) progress(owner, sent, total);
    return result.stored;
}
