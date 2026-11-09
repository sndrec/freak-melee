#include "native_netplay_diagnostics.h"

#ifdef NATIVE_NETPLAY_DIAGNOSTICS_URL
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winhttp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

enum { REPORT_LIMIT = 256 * 1024, REPORT_HTTP_TIMEOUT_MS = 1500 };

FILE* nativeNetplayDiagnostics_Open(uint64_t session, unsigned peer)
{
    wchar_t app_data[MAX_PATH], product[MAX_PATH], folder[MAX_PATH], path[MAX_PATH];
    DWORD size = GetEnvironmentVariableW(L"LOCALAPPDATA", app_data, MAX_PATH);
    if (!size || size >= MAX_PATH ||
        swprintf(product, MAX_PATH, L"%ls\\Freak Melee", app_data) < 0 ||
        swprintf(folder, MAX_PATH, L"%ls\\netplay-diagnostics", product) < 0 ||
        swprintf(path, MAX_PATH, L"%ls\\%016llx-%u.log", folder,
                 (unsigned long long)session, peer) < 0) return NULL;
    if ((!CreateDirectoryW(product, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) ||
        (!CreateDirectoryW(folder, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)) return NULL;
    FILE* report = _wfopen(path, L"w+b");
    if (report != NULL) fputs("freak-melee-netplay-report\nbuild="
                              NATIVE_NETPLAY_DIAGNOSTICS_BUILD "\n", report);
    return report;
}

void nativeNetplayDiagnostics_Send(FILE* report, uint64_t session_id, unsigned peer)
{
    static const wchar_t endpoint[] = NATIVE_NETPLAY_DIAGNOSTICS_URL;
    wchar_t host[256], object[560];
    URL_COMPONENTS parts = {
        .dwStructSize = sizeof(parts),
        .lpszHostName = host,
        .dwHostNameLength = sizeof(host) / sizeof(host[0]),
        .lpszUrlPath = object,
        .dwUrlPathLength = sizeof(object) / sizeof(object[0]),
    };
    long bytes = ftell(report);
    void* body = NULL;
    HINTERNET session = NULL, connection = NULL, request = NULL;
    DWORD status = 0;
    if (bytes <= 0 || bytes > REPORT_LIMIT || fseek(report, 0, SEEK_SET) ||
        !WinHttpCrackUrl(endpoint, 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS) goto done;
    if (swprintf(object + parts.dwUrlPathLength,
                 sizeof(object) / sizeof(object[0]) - parts.dwUrlPathLength,
                 L"/%016llx/%u", (unsigned long long)session_id, peer) < 0) goto done;
    body = malloc(bytes);
    if (body == NULL || fread(body, 1, bytes, report) != (size_t)bytes) goto done;
    session = WinHttpOpen(L"Freak Melee/0.1", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (session == NULL || !WinHttpSetTimeouts(session, REPORT_HTTP_TIMEOUT_MS,
            REPORT_HTTP_TIMEOUT_MS, REPORT_HTTP_TIMEOUT_MS, REPORT_HTTP_TIMEOUT_MS)) goto done;
    connection = WinHttpConnect(session, host, parts.nPort, 0);
    if (connection == NULL) goto done;
    request = WinHttpOpenRequest(connection, L"POST", object, NULL, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (request == NULL || !WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY,
            &redirects, sizeof(redirects)) || !WinHttpSendRequest(request,
            L"Content-Type: text/plain; charset=utf-8\r\n", (DWORD)-1,
            body, (DWORD)bytes, (DWORD)bytes, 0) || !WinHttpReceiveResponse(request, NULL)) goto done;
    DWORD status_size = sizeof(status);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX);
done:
    if (request != NULL) WinHttpCloseHandle(request);
    if (connection != NULL) WinHttpCloseHandle(connection);
    if (session != NULL) WinHttpCloseHandle(session);
    free(body);
    fclose(report);
    fprintf(stderr, "[netplay-diagnostics] %s %016llx-%u.log (HTTP %lu)\n",
            status == 204 ? "sent" : "saved locally",
            (unsigned long long)session_id, peer, (unsigned long)status);
}
#else
FILE* nativeNetplayDiagnostics_Open(uint64_t session, unsigned peer)
{
    (void)session;
    (void)peer;
    return NULL;
}

void nativeNetplayDiagnostics_Send(FILE* report, uint64_t session, unsigned peer)
{
    (void)session;
    (void)peer;
    fclose(report);
}
#endif
