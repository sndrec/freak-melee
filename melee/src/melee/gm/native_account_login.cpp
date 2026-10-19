#include "native_account_internal.h"
#include <cstdio>
#include <cstring>

bool nativeAccount_Login(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
{
    const auto op = request.operation;
    const bool registration = op == NativeAccountOperation::Register;
    const bool recovery = op == NativeAccountOperation::Recover;
    const bool adding = op == NativeAccountOperation::AddKey;
    const bool authorized = adding || op == NativeAccountOperation::Approve;
    const char* purpose = registration ? "register" : recovery ? "recover" : adding ? "add" : authorized ? "reauth" : "login";
    char handle[25], route[64], public_key[44], attempt[44];
    if (!nativeAccount_Handle(authorized || (!registration && !recovery) ? w.secrets.handle : request.handle, handle)) {
        std::snprintf(view.status, sizeof(view.status), "Use 3-24 letters, numbers or underscores for your account name."); return false;
    }
    if (recovery && !nativeAccount_Secret(request.recovery)) {
        std::snprintf(view.status, sizeof(view.status), "Enter your complete saved recovery code."); return false;
    }
    if (adding) {
        NativeAccountRequest approval{}; approval.operation = NativeAccountOperation::Approve;
        if (!nativeAccount_Login(w, approval, view)) return false;
    }
    NativeAccountSecrets identity = w.secrets;
    struct Wipe { NativeAccountSecrets& value; ~Wipe() { SecureZeroMemory(&value, sizeof(value)); } } wipe{identity};
    if (registration || recovery || adding) {
        // Reuse an unfinished registration's key if its response was lost.
        const bool reuse = registration && !std::strcmp(identity.handle, handle) && *identity.seed && !*identity.token;
        if (!reuse && !nativeAccount_GenerateKey(identity.seed)) {
            std::snprintf(view.status, sizeof(view.status), "Could not generate your account key."); return false;
        }
        std::snprintf(identity.origin, sizeof(identity.origin), "%s", w.endpoint.origin);
        std::snprintf(identity.handle, sizeof(identity.handle), "%s", handle);
    }
    if (!nativeAccount_KeyPublic(identity.seed, public_key)) {
        std::snprintf(view.status, sizeof(view.status), "Import your account-key file to sign in on this PC."); return false;
    }
    if (adding && !nativeAccount_WriteKeyFile(request.path, identity, w.json)) {
        std::snprintf(view.status, sizeof(view.status), "The new key file could not be saved. No key was added."); return false;
    }
    auto* body = w.json.object();
    yyjson_mut_obj_add_str(w.json.output, body, "public_key", public_key);
    yyjson_mut_obj_add_str(w.json.output, body, "name", *request.name ? request.name : handle);
    yyjson_mut_obj_add_str(w.json.output, body, "key_name", adding ? "Additional account key" : "Account key");
    if (recovery) yyjson_mut_obj_add_str(w.json.output, body, "recovery_code", request.recovery);
    if (!w.json.write(w.send, sizeof(w.send))) return false;
    std::snprintf(route, sizeof(route), "%s/options", purpose);
    unsigned status = 0;
    auto* response = nativeAccount_Http(w, handle, route, true, authorized, view, &status);
    if (!response) {
        // A prior registration may have committed before its response reached us.
        if (registration && status == 409 && !std::strcmp(w.secrets.seed, identity.seed)) {
            NativeAccountRequest login{}; login.operation = NativeAccountOperation::Login;
            return nativeAccount_Login(w, login, view);
        }
        return false;
    }
    if (!nativeAccount_String(yyjson_obj_get(response, "attempt"), attempt, sizeof(attempt)) || !nativeAccount_Secret(attempt)) {
        std::snprintf(view.status, sizeof(view.status), "The account service returned an invalid sign-in attempt."); return false;
    }
    body = w.json.object();
    yyjson_mut_obj_add_str(w.json.output, body, "attempt", attempt);
    yyjson_mut_obj_add_str(w.json.output, body, "device", "Freak Melee PC");
    if (nativeAccount_Cancelled(w) || !nativeAccount_KeyProof(response, w.endpoint, handle, purpose, identity.seed, w.json, body, view.status) ||
        !w.json.write(w.send, sizeof(w.send))) return false;
    if (registration || recovery) {
        // Save the private key before the server can accept it. An interrupted
        // connection must never leave a committed account with an unknown key.
        SecureZeroMemory(identity.token, sizeof(identity.token));
        SecureZeroMemory(identity.recovery, sizeof(identity.recovery));
        identity.signed_out = false;
        if (!nativeAccount_Store(w.path, identity)) {
            std::snprintf(view.status, sizeof(view.status), "Could not save your account key. Sign-in was not completed."); return false;
        }
        w.secrets = identity; view.has_key = view.saved = true;
        std::snprintf(view.handle, sizeof(view.handle), "%s", handle);
    }
    AcquireSRWLockExclusive(&w.lock);
    if (w.cancelled || w.stop) { ReleaseSRWLockExclusive(&w.lock); return false; }
    view.cancellable = w.view.cancellable = false;
    ReleaseSRWLockExclusive(&w.lock);
    std::snprintf(view.status, sizeof(view.status), "Completing sign-in...");
    nativeAccount_Publish(w, view);
    std::snprintf(route, sizeof(route), "%s/finish", purpose);
    response = nativeAccount_Http(w, handle, route, true, authorized, view);
    if (!response) return false;
    if (!authorized) {
        bool valid = nativeAccount_SetProfile(response, view) && !std::strcmp(view.handle, handle) &&
            nativeAccount_String(yyjson_obj_get(response, "token"), identity.token, sizeof(identity.token)) && nativeAccount_Secret(identity.token);
        if (registration || recovery) valid = valid &&
            nativeAccount_String(yyjson_obj_get(response, "recovery_code"), identity.recovery, sizeof(identity.recovery)) && nativeAccount_Secret(identity.recovery);
        if (!valid) { std::snprintf(view.status, sizeof(view.status), "The account service returned an incomplete sign-in."); return false; }
        identity.signed_out = false; w.secrets = identity;
        view.signed_in = true; ++view.profile_revision;
        nativeAccount_Persist(w, view);
    }
    if (!nativeAccount_Refresh(w, view)) return false;
    if (view.saved) std::snprintf(view.status, sizeof(view.status), adding ? "New account-key file saved and registered." :
        view.recovery_pending ? "Signed in. Save your recovery code somewhere safe." : "Signed in.");
    return true;
}
