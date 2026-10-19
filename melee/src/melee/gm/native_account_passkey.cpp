#include "native_account_internal.h"
#include <cstring>
#include <cstdio>

bool NativeAccountPasskeys::load()
{
    library = LoadLibraryExW(L"webauthn.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!library) return false;
#define LOAD_API(member, symbol) member = reinterpret_cast<decltype(member)>(GetProcAddress(library, #symbol))
    LOAD_API(version, WebAuthNGetApiVersionNumber);
    LOAD_API(cancellation_id, WebAuthNGetCancellationId);
    LOAD_API(cancel, WebAuthNCancelCurrentOperation);
    LOAD_API(create, WebAuthNAuthenticatorMakeCredential);
    LOAD_API(get, WebAuthNAuthenticatorGetAssertion);
    LOAD_API(free_attestation, WebAuthNFreeCredentialAttestation);
    LOAD_API(free_assertion, WebAuthNFreeAssertion);
#undef LOAD_API
    if (version && cancellation_id && cancel && create && get && free_attestation && free_assertion && version() >= 1 &&
        SUCCEEDED(cancellation_id(&cancellation))) return true;
    unload();
    return false;
}
void NativeAccountPasskeys::unload()
{
    if (library) FreeLibrary(library);
    *this = {};
}
namespace {
    bool wide(yyjson_val* value, wchar_t* output, unsigned capacity)
    {
        return yyjson_is_str(value) && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
            yyjson_get_str(value), -1, output, capacity) != 0;
    }
    bool credential_list(yyjson_val* options, bool registration, WEBAUTHN_CREDENTIAL (&entries)[8],
        BYTE (&ids)[8][1050], WEBAUTHN_CREDENTIALS& list)
    {
        auto* values = yyjson_obj_get(options, registration ? "excludeCredentials" : "allowCredentials");
        if (!yyjson_is_arr(values) || yyjson_arr_size(values) > 8) return false;
        list = {DWORD(yyjson_arr_size(values)), entries};
        for (unsigned i = 0; i < list.cCredentials; ++i) {
            auto* item = yyjson_arr_get(values, i);
            char id[1401]; DWORD size;
            if (!yyjson_equals_str(yyjson_obj_get(item, "type"), "public-key") ||
                !nativeAccount_String(yyjson_obj_get(item, "id"), id, sizeof(id)) ||
                !nativeAccount_Base64Decode(id, ids[i], sizeof(ids[i]), size)) return false;
            entries[i] = {WEBAUTHN_CREDENTIAL_CURRENT_VERSION, size, ids[i], WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY};
        }
        return true;
    }
    bool add_binary(NativeAccountJson& json, yyjson_mut_val* object, const char* key,
        const BYTE* bytes, DWORD length)
    {
        char encoded[32769];
        return nativeAccount_Base64Encode(bytes, length, encoded, sizeof(encoded)) &&
            yyjson_mut_obj_add_strcpy(json.output, object, key, encoded);
    }
}
bool nativeAccount_PasskeyOptions(yyjson_val* response, const NativeAccountEndpoint& endpoint, bool registration)
{
    auto* options = yyjson_obj_get(response, "options");
    char origin[512], rp[256], challenge[257];
    wchar_t rp_wide[256]; BYTE decoded[192]; DWORD size;
    if (!nativeAccount_String(yyjson_obj_get(response, "origin"), origin, sizeof(origin)) || std::strcmp(origin, endpoint.origin) ||
        !nativeAccount_String(yyjson_obj_get(options, "challenge"), challenge, sizeof(challenge)) ||
        !nativeAccount_Base64Decode(challenge, decoded, sizeof(decoded), size) || size < 16 ||
        !nativeAccount_String(registration ? yyjson_obj_get(yyjson_obj_get(options, "rp"), "id") :
            yyjson_obj_get(options, "rpId"), rp, sizeof(rp)) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, rp, -1, rp_wide, 256) ||
        _wcsicmp(rp_wide, endpoint.host)) return false;
    WEBAUTHN_CREDENTIAL entries[8]{}; BYTE ids[8][1050]{}; WEBAUTHN_CREDENTIALS list{};
    if (!credential_list(options, registration, entries, ids, list)) return false;
    if (!registration) return list.cCredentials > 0 && yyjson_equals_str(yyjson_obj_get(options, "userVerification"), "required");
    auto* user = yyjson_obj_get(options, "user");
    wchar_t name[65], display[65]; char user_id[87];
    auto* algorithms = yyjson_obj_get(options, "pubKeyCredParams");
    if (!yyjson_is_arr(algorithms) || !yyjson_arr_size(algorithms) || yyjson_arr_size(algorithms) > 2 ||
        !wide(yyjson_obj_get(user, "name"), name, 65) || !wide(yyjson_obj_get(user, "displayName"), display, 65) ||
        !nativeAccount_String(yyjson_obj_get(user, "id"), user_id, sizeof(user_id)) ||
        !nativeAccount_Base64Decode(user_id, decoded, 64, size) || !size ||
        !yyjson_equals_str(yyjson_obj_get(options, "attestation"), "none") ||
        !yyjson_equals_str(yyjson_obj_get(yyjson_obj_get(options, "authenticatorSelection"), "userVerification"), "required")) return false;
    for (size_t i = 0; i < yyjson_arr_size(algorithms); ++i) {
        auto* parameter = yyjson_arr_get(algorithms, i);
        auto* algorithm = yyjson_obj_get(parameter, "alg");
        if (!yyjson_equals_str(yyjson_obj_get(parameter, "type"), "public-key") || !yyjson_is_int(algorithm) ||
            (yyjson_get_sint(algorithm) != -7 && yyjson_get_sint(algorithm) != -257)) return false;
    }
    return true;
}
bool nativeAccount_Passkey(NativeAccountPasskeys& api, HWND window, yyjson_val* response,
    const NativeAccountEndpoint& endpoint, bool registration, NativeAccountJson& json,
    yyjson_mut_val* body, char (&error)[192])
{
    if (!nativeAccount_PasskeyOptions(response, endpoint, registration)) {
        std::snprintf(error, sizeof(error), "The account service returned invalid passkey settings."); return false;
    }
    auto* options = yyjson_obj_get(response, "options");
    char challenge[257]; nativeAccount_String(yyjson_obj_get(options, "challenge"), challenge, sizeof(challenge));
    // Challenge/origin were validated and contain no JSON metacharacters.
    char client[1024];
    int length = std::snprintf(client, sizeof(client), "{\"type\":\"webauthn.%s\",\"challenge\":\"%s\",\"origin\":\"%s\",\"crossOrigin\":false}",
        registration ? "create" : "get", challenge, endpoint.origin);
    if (length <= 0 || length >= sizeof(client)) return false;
    WEBAUTHN_CLIENT_DATA client_data{WEBAUTHN_CLIENT_DATA_CURRENT_VERSION, DWORD(length),
        reinterpret_cast<BYTE*>(client), WEBAUTHN_HASH_ALGORITHM_SHA_256};
    WEBAUTHN_CREDENTIAL entries[8]{}; BYTE ids[8][1050]{}; WEBAUTHN_CREDENTIALS list{};
    if (!credential_list(options, registration, entries, ids, list)) return false;
    auto* credential = yyjson_mut_obj(json.output);
    auto* proof = yyjson_mut_obj(json.output);
    yyjson_mut_obj_add_val(json.output, body, "credential", credential);
    yyjson_mut_obj_add_val(json.output, credential, "response", proof);
    yyjson_mut_obj_add_str(json.output, credential, "type", "public-key");
    yyjson_mut_obj_add_val(json.output, credential, "clientExtensionResults", yyjson_mut_obj(json.output));
    bool ok = add_binary(json, proof, "clientDataJSON", client_data.pbClientDataJSON, client_data.cbClientDataJSON);
    HRESULT status;
    if (registration) {
        wchar_t rp_name[65], user_name[65], display[65]; BYTE user_id[64]; DWORD user_length;
        auto* user = yyjson_obj_get(options, "user"); char user_encoded[87];
        if (!wide(yyjson_obj_get(yyjson_obj_get(options, "rp"), "name"), rp_name, 65) ||
            !wide(yyjson_obj_get(user, "name"), user_name, 65) || !wide(yyjson_obj_get(user, "displayName"), display, 65) ||
            !nativeAccount_String(yyjson_obj_get(user, "id"), user_encoded, sizeof(user_encoded)) ||
            !nativeAccount_Base64Decode(user_encoded, user_id, sizeof(user_id), user_length)) return false;
        WEBAUTHN_RP_ENTITY_INFORMATION rp{1, endpoint.host, rp_name, nullptr};
        WEBAUTHN_USER_ENTITY_INFORMATION person{1, user_length, user_id, user_name, nullptr, display};
        WEBAUTHN_COSE_CREDENTIAL_PARAMETER algorithms[2]{};
        auto* params = yyjson_obj_get(options, "pubKeyCredParams");
        for (unsigned i = 0; i < yyjson_arr_size(params); ++i)
            algorithms[i] = {1, WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY, LONG(yyjson_get_sint(yyjson_obj_get(yyjson_arr_get(params, i), "alg")))};
        WEBAUTHN_COSE_CREDENTIAL_PARAMETERS parameters{DWORD(yyjson_arr_size(params)), algorithms};
        WEBAUTHN_AUTHENTICATOR_MAKE_CREDENTIAL_OPTIONS settings{};
        settings.dwVersion = api.version() >= 3 ? 4 : 2;
        settings.dwTimeoutMilliseconds = 120000;
        settings.CredentialList = list;
        settings.dwUserVerificationRequirement = WEBAUTHN_USER_VERIFICATION_REQUIREMENT_REQUIRED;
        settings.dwAttestationConveyancePreference = WEBAUTHN_ATTESTATION_CONVEYANCE_PREFERENCE_NONE;
        settings.bPreferResidentKey = TRUE;
        settings.pCancellationId = &api.cancellation;
        PWEBAUTHN_CREDENTIAL_ATTESTATION attestation = nullptr;
        status = api.create(window, &rp, &person, &parameters, &client_data, &settings, &attestation);
        if (SUCCEEDED(status) && attestation) {
            ok = ok && add_binary(json, credential, "id", attestation->pbCredentialId, attestation->cbCredentialId) &&
                add_binary(json, credential, "rawId", attestation->pbCredentialId, attestation->cbCredentialId) &&
                add_binary(json, proof, "attestationObject", attestation->pbAttestationObject, attestation->cbAttestationObject);
        } else ok = false;
        if (attestation) api.free_attestation(attestation);
    } else {
        WEBAUTHN_AUTHENTICATOR_GET_ASSERTION_OPTIONS settings{};
        settings.dwVersion = 3;
        settings.dwTimeoutMilliseconds = 120000;
        settings.CredentialList = list;
        settings.dwUserVerificationRequirement = WEBAUTHN_USER_VERIFICATION_REQUIREMENT_REQUIRED;
        settings.pCancellationId = &api.cancellation;
        PWEBAUTHN_ASSERTION assertion = nullptr;
        status = api.get(window, endpoint.host, &client_data, &settings, &assertion);
        if (SUCCEEDED(status) && assertion) {
            ok = ok && add_binary(json, credential, "id", assertion->Credential.pbId, assertion->Credential.cbId) &&
                add_binary(json, credential, "rawId", assertion->Credential.pbId, assertion->Credential.cbId) &&
                add_binary(json, proof, "authenticatorData", assertion->pbAuthenticatorData, assertion->cbAuthenticatorData) &&
                add_binary(json, proof, "signature", assertion->pbSignature, assertion->cbSignature);
            if (assertion->cbUserId) ok = ok && add_binary(json, proof, "userHandle", assertion->pbUserId, assertion->cbUserId);
        } else ok = false;
        if (assertion) api.free_assertion(assertion);
    }
    if (!ok) std::snprintf(error, sizeof(error), FAILED(status) ?
        "Passkey approval was cancelled or could not be completed. Try again when ready." : "The passkey response was too large.");
    return ok;
}
