#include "native_account_internal.h"
#include <commdlg.h>
#include <cstdio>
#include <cstring>

namespace {
    bool write_file(const char* path, const char* bytes)
    {
        wchar_t filename[1024], temporary[1100]; char nonce[44];
        bool ok = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, filename, 1024) &&
            nativeAccount_NewNonce(nonce) && swprintf_s(temporary, L"%s.%hs.tmp", filename, nonce) > 0;
        HANDLE file = ok ? CreateFileW(temporary, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr) : INVALID_HANDLE_VALUE;
        DWORD count = 0; const DWORD length = DWORD(std::strlen(bytes));
        ok = file != INVALID_HANDLE_VALUE && WriteFile(file, bytes, length, &count, nullptr) && count == length && FlushFileBuffers(file);
        if (file != INVALID_HANDLE_VALUE) {
            CloseHandle(file);
            if (ok) ok = MoveFileExW(temporary, filename, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
            if (!ok) DeleteFileW(temporary);
        }
        return ok;
    }
}

bool nativeAccount_ReadKeyFile(const char* path, const char* origin, NativeAccountSecrets& result, NativeAccountJson& json)
{
    wchar_t filename[1024];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, filename, 1024)) return false;
    HANDLE file = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    char bytes[2049]{}; LARGE_INTEGER size{}; DWORD count = 0;
    bool ok = GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < sizeof(bytes) &&
        ReadFile(file, bytes, DWORD(size.QuadPart), &count, nullptr) && count == size.QuadPart;
    CloseHandle(file);
    auto* value = ok ? json.read(bytes, count) : nullptr;
    NativeAccountSecrets imported{}; char canonical[25], public_key[44];
    ok = value && yyjson_obj_size(value) == 4 && yyjson_equals_str(yyjson_obj_get(value, "application"), "Freak Melee account key") &&
        nativeAccount_String(yyjson_obj_get(value, "service"), imported.origin, sizeof(imported.origin)) && !std::strcmp(imported.origin, origin) &&
        nativeAccount_String(yyjson_obj_get(value, "account"), imported.handle, sizeof(imported.handle)) &&
        nativeAccount_Handle(imported.handle, canonical) && !std::strcmp(canonical, imported.handle) &&
        nativeAccount_String(yyjson_obj_get(value, "seed"), imported.seed, sizeof(imported.seed)) && nativeAccount_KeyPublic(imported.seed, public_key);
    if (ok) result = imported;
    SecureZeroMemory(&imported, sizeof(imported)); SecureZeroMemory(bytes, sizeof(bytes)); return ok;
}

bool nativeAccount_WriteKeyFile(const char* path, const NativeAccountSecrets& identity, NativeAccountJson& json)
{
    char public_key[44], canonical[25]; NativeAccountEndpoint endpoint{};
    if (!nativeAccount_Endpoint(identity.origin, endpoint) || std::strcmp(identity.origin, endpoint.origin) ||
        !nativeAccount_Handle(identity.handle, canonical) || std::strcmp(canonical, identity.handle) ||
        !nativeAccount_KeyPublic(identity.seed, public_key)) return false;
    auto* object = json.object();
    yyjson_mut_obj_add_str(json.output, object, "application", "Freak Melee account key");
    yyjson_mut_obj_add_str(json.output, object, "service", identity.origin);
    yyjson_mut_obj_add_str(json.output, object, "account", identity.handle);
    yyjson_mut_obj_add_str(json.output, object, "seed", identity.seed);
    char bytes[2049]{};
    bool ok = json.write(bytes, sizeof(bytes)) && write_file(path, bytes);
    SecureZeroMemory(bytes, sizeof(bytes)); return ok;
}

void nativeAccount_RunFile(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
{
    if (request.operation == NativeAccountOperation::ExportKey) {
        std::snprintf(view.status, sizeof(view.status), nativeAccount_WriteKeyFile(request.path, w.secrets, w.json) ?
            "Account-key file saved. Keep it private; it grants account access." : "Your account-key file could not be saved.");
    } else if (request.operation == NativeAccountOperation::ImportKey) {
        if (view.signed_in || *w.secrets.recovery) {
            std::snprintf(view.status, sizeof(view.status), "Save your recovery code and sign out before importing another key."); return;
        }
        NativeAccountSecrets imported{}, previous = w.secrets;
        if (!nativeAccount_ReadKeyFile(request.path, w.endpoint.origin, imported, w.json)) {
            std::snprintf(view.status, sizeof(view.status), "This is not a valid account-key file for this service.");
        } else {
            w.secrets = imported;
            NativeAccountRequest login{}; login.operation = NativeAccountOperation::Login;
            nativeAccount_Login(w, login, view);
            if (!view.signed_in) w.secrets = previous;
        }
        SecureZeroMemory(&imported, sizeof(imported)); SecureZeroMemory(&previous, sizeof(previous));
    } else if (request.operation == NativeAccountOperation::ExportRecovery && *w.secrets.recovery) {
        char text[1024];
        std::snprintf(text, sizeof(text), "Freak Melee recovery code\r\nAccount: %s\r\nService: %s\r\n\r\n%s\r\n\r\nKeep this private. It can replace your account keys.\r\n",
            w.secrets.handle, w.secrets.origin, w.secrets.recovery);
        bool ok = write_file(request.path, text);
        SecureZeroMemory(text, sizeof(text));
        if (!ok) { std::snprintf(view.status, sizeof(view.status), "Your recovery code could not be saved."); return; }
        NativeAccountSecrets previous = w.secrets;
        SecureZeroMemory(w.secrets.recovery, sizeof(w.secrets.recovery));
        if (!nativeAccount_Persist(w, view)) { w.secrets = previous; view.recovery_pending = true; }
        else std::snprintf(view.status, sizeof(view.status), "Recovery code saved. Keep it somewhere safe.");
        SecureZeroMemory(&previous, sizeof(previous));
    }
}

bool nativeAccount_ChooseFile(NativeAccountOperation operation, void* window)
{
    if (operation != NativeAccountOperation::ImportKey && operation != NativeAccountOperation::ExportKey &&
        operation != NativeAccountOperation::AddKey && operation != NativeAccountOperation::ExportRecovery) return false;
    const bool importing = operation == NativeAccountOperation::ImportKey;
    const bool recovery = operation == NativeAccountOperation::ExportRecovery;
    wchar_t path[1024]{};
    if (!importing) wcscpy_s(path, recovery ? L"Freak Melee recovery code.txt" : L"Freak Melee account.fmakey");
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog); dialog.hwndOwner = static_cast<HWND>(window);
    dialog.lpstrFile = path; dialog.nMaxFile = 1024;
    dialog.lpstrTitle = importing ? L"Import account key" : recovery ? L"Save recovery code" :
        operation == NativeAccountOperation::AddKey ? L"Save a new account key for another device" : L"Export account key";
    dialog.lpstrFilter = recovery ? L"Text files\0*.txt\0\0" : L"Freak Melee account keys\0*.fmakey\0\0";
    dialog.lpstrDefExt = recovery ? L"txt" : L"fmakey";
    dialog.Flags = OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | (importing ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT);
    if (!(importing ? GetOpenFileNameW(&dialog) : GetSaveFileNameW(&dialog))) return false;
    NativeAccountRequest request{}; request.operation = operation;
    return WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, path, -1, request.path, sizeof(request.path), nullptr, nullptr) &&
        nativeAccount_Submit(request);
}
