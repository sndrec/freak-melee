#include "native_account_internal.h"
#include <bcrypt.h>
#include <monocypher-ed25519.h>
#include <cstdio>
#include <cstring>

bool nativeAccount_GenerateKey(char (&encoded)[44])
{
    BYTE seed[32];
    bool ok = BCryptGenRandom(nullptr, seed, sizeof(seed), BCRYPT_USE_SYSTEM_PREFERRED_RNG) >= 0 &&
        nativeAccount_Base64Encode(seed, sizeof(seed), encoded, sizeof(encoded));
    crypto_wipe(seed, sizeof(seed)); return ok;
}

bool nativeAccount_KeyPublic(const char* encoded, char (&output)[44])
{
    BYTE seed[32], secret_key[64], public_key[32]; DWORD length = 0;
    bool ok = nativeAccount_Base64Decode(encoded, seed, sizeof(seed), length) && length == sizeof(seed);
    if (ok) {
        crypto_ed25519_key_pair(secret_key, public_key, seed);
        ok = nativeAccount_Base64Encode(public_key, sizeof(public_key), output, sizeof(output));
    }
    crypto_wipe(seed, sizeof(seed)); crypto_wipe(secret_key, sizeof(secret_key)); return ok;
}

bool nativeAccount_KeyProof(yyjson_val* response, const NativeAccountEndpoint& endpoint,
    const char* handle, const char* purpose, const char* encoded, NativeAccountJson& json,
    yyjson_mut_val* body, char (&error)[192])
{
    char origin[512], attempt[44], challenge[44], user_id[44], public_id[44], kind[9], expected[44];
    bool ok = nativeAccount_String(yyjson_obj_get(response, "origin"), origin, sizeof(origin)) && !std::strcmp(origin, endpoint.origin) &&
        nativeAccount_String(yyjson_obj_get(response, "purpose"), kind, sizeof(kind)) && !std::strcmp(kind, purpose) &&
        nativeAccount_String(yyjson_obj_get(response, "attempt"), attempt, sizeof(attempt)) && nativeAccount_Secret(attempt) &&
        nativeAccount_String(yyjson_obj_get(response, "challenge"), challenge, sizeof(challenge)) && nativeAccount_Secret(challenge) &&
        nativeAccount_String(yyjson_obj_get(response, "user_id"), user_id, sizeof(user_id)) && nativeAccount_Secret(user_id) &&
        nativeAccount_String(yyjson_obj_get(response, "public_key"), public_id, sizeof(public_id)) &&
        nativeAccount_KeyPublic(encoded, expected) && !std::strcmp(expected, public_id);
    if (!ok) { std::snprintf(error, sizeof(error), "The account service returned invalid key settings."); return false; }
    char message[1024];
    const int size = std::snprintf(message, sizeof(message), "Freak Melee account key\n%s\n%s\n%s\n%s\n%s\n%s\n%s",
        origin, handle, user_id, kind, public_id, attempt, challenge);
    if (size < 0 || size >= sizeof(message)) return false;
    BYTE seed[32], secret_key[64], public_key[32], signature[64]; DWORD length = 0;
    ok = nativeAccount_Base64Decode(encoded, seed, sizeof(seed), length) && length == sizeof(seed);
    char signature_text[87];
    if (ok) {
        crypto_ed25519_key_pair(secret_key, public_key, seed);
        crypto_ed25519_sign(signature, secret_key, reinterpret_cast<const BYTE*>(message), size);
        ok = nativeAccount_Base64Encode(signature, sizeof(signature), signature_text, sizeof(signature_text)) &&
            yyjson_mut_obj_add_strcpy(json.output, body, "signature", signature_text);
    }
    crypto_wipe(seed, sizeof(seed)); crypto_wipe(secret_key, sizeof(secret_key));
    if (!ok) std::snprintf(error, sizeof(error), "The account key could not sign this request.");
    return ok;
}
