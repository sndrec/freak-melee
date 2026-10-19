#include "native_account_internal.h"
#include <cstring>
#include <cstdio>

namespace {
    bool lookup(NativeAccountWorker& w, const char* name, NativeAccountView& view)
    {
        view.social.profile = {}; view.social.profile_blocked = false;
        std::snprintf(view.social.relationship, sizeof(view.social.relationship), "none");
        char handle[25];
        if (!nativeAccount_Handle(name, handle)) { std::snprintf(view.status, sizeof(view.status), "Enter a valid account name."); return false; }
        yyjson_val* result;
        if (view.signed_in) {
            auto* body = w.json.object(); yyjson_mut_obj_add_str(w.json.output, body, "to", handle);
            if (!w.json.write(w.send, sizeof(w.send))) return false;
            result = nativeAccount_Http(w, w.secrets.handle, "social/profile", true, true, view);
        } else result = nativeAccount_Http(w, handle, "profile", false, false, view);
        if (!result) return false;
        if (!nativeAccount_ReadProfile(result, view.social.profile) || std::strcmp(handle, view.social.profile.handle)) {
            std::snprintf(view.status, sizeof(view.status), "The player profile could not be read."); return false;
        }
        if (view.signed_in) {
            view.social.profile_blocked = yyjson_get_bool(yyjson_obj_get(result, "blocked"));
            nativeAccount_String(yyjson_obj_get(result, "relationship"), view.social.relationship, sizeof(view.social.relationship));
        }
        ++view.social.revision; return true;
    }
    bool number(yyjson_val* root, const char* key, unsigned& value, unsigned limit)
    {
        auto* item = yyjson_obj_get(root, key);
        if (!yyjson_is_uint(item) || yyjson_get_uint(item) > limit) return false;
        value = unsigned(yyjson_get_uint(item)); return true;
    }
    bool mail(yyjson_val* item, NativeSocialMail& result)
    {
        unsigned unread;
        auto* created = yyjson_obj_get(item, "created");
        char canonical[25];
        bool ok = nativeAccount_String(yyjson_obj_get(item, "id"), result.id, sizeof(result.id)) && nativeAccount_Secret(result.id) &&
            nativeAccount_String(yyjson_obj_get(item, "peer"), result.peer, sizeof(result.peer)) && nativeAccount_Secret(result.peer) &&
            nativeAccount_String(yyjson_obj_get(item, "handle"), result.handle, sizeof(result.handle)) &&
            nativeAccount_Handle(result.handle, canonical) && !std::strcmp(canonical, result.handle) &&
            nativeAccount_String(yyjson_obj_get(item, "name"), result.name, sizeof(result.name)) &&
            nativeAccount_String(yyjson_obj_get(item, "title"), result.title, sizeof(result.title)) &&
            nativeAccount_String(yyjson_obj_get(item, "kind"), result.kind, sizeof(result.kind)) &&
            (!std::strcmp(result.kind, "letter") || !std::strcmp(result.kind, "friend_request")) &&
            nativeAccount_String(yyjson_obj_get(item, "status"), result.status, sizeof(result.status)) &&
            number(item, "folder", result.folder, 1) && number(item, "unread", unread, 1) &&
            yyjson_is_uint(created) && yyjson_get_uint(created) <= 9007199254740991ULL;
        if (ok) { result.created = yyjson_get_uint(created); result.unread = unread != 0; }
        return ok;
    }
    bool refresh(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
    {
        auto* body = w.json.object();
        yyjson_mut_obj_add_uint(w.json.output, body, "folder", request.folder);
        yyjson_mut_obj_add_uint(w.json.output, body, "offset", request.offset);
        yyjson_mut_obj_add_uint(w.json.output, body, "friends_offset", request.friends_offset);
        if (!w.json.write(w.send, sizeof(w.send))) return false;
        auto* result = nativeAccount_Http(w, w.secrets.handle, "social/state", true, true, view);
        if (!result) return false;
        NativeSocialView next = view.social;
        auto* messages = yyjson_obj_get(result, "messages"), *friends = yyjson_obj_get(result, "friends");
        bool ok = yyjson_is_arr(messages) && yyjson_arr_size(messages) <= 8 && yyjson_is_arr(friends) && yyjson_arr_size(friends) <= 8 &&
            number(result, "folder", next.folder, 1) && number(result, "offset", next.offset, 2048) &&
            number(result, "friends_offset", next.friends_offset, 256) && number(result, "total", next.total, 2048) &&
            number(result, "unread", next.unread, 2048) && number(result, "friends_total", next.friends_total, 256);
        next.count = unsigned(yyjson_arr_size(messages)); next.friend_count = unsigned(yyjson_arr_size(friends));
        for (unsigned i = 0; ok && i < next.count; ++i) ok = mail(yyjson_arr_get(messages, i), next.messages[i]);
        for (unsigned i = 0; ok && i < next.friend_count; ++i) {
            auto* item = yyjson_arr_get(friends, i); auto& entry = next.friends[i]; char canonical[25];
            ok = nativeAccount_String(yyjson_obj_get(item, "peer"), entry.id, sizeof(entry.id)) && nativeAccount_Secret(entry.id) &&
                nativeAccount_String(yyjson_obj_get(item, "handle"), entry.handle, sizeof(entry.handle)) &&
                nativeAccount_Handle(entry.handle, canonical) && !std::strcmp(canonical, entry.handle) &&
                nativeAccount_String(yyjson_obj_get(item, "name"), entry.name, sizeof(entry.name)) &&
                nativeAccount_String(yyjson_obj_get(item, "state"), entry.state, sizeof(entry.state)) &&
                (!std::strcmp(entry.state, "incoming") || !std::strcmp(entry.state, "outgoing") || !std::strcmp(entry.state, "friend"));
        }
        if (!ok) { std::snprintf(view.status, sizeof(view.status), "The mailbox response could not be read."); return false; }
        for (unsigned i = 0; i < next.count; ++i)
            if (!std::strcmp(next.messages[i].id, next.opened.id)) next.opened = next.messages[i];
        next.loaded = true; ++next.revision; view.social = next;
        return true;
    }
}
bool nativeAccount_RunSocial(NativeAccountWorker& w, const NativeAccountRequest& request, NativeAccountView& view)
{
    const auto operation = request.operation;
    if (operation < NativeAccountOperation::RefreshSocial) return false;
    if (operation == NativeAccountOperation::LookupProfile) {
        if (lookup(w, request.handle, view)) std::snprintf(view.status, sizeof(view.status), "Player profile loaded.");
        return true;
    }
    if (!view.signed_in) { std::snprintf(view.status, sizeof(view.status), "Sign in to use friends and your inbox."); return true; }
    if (operation == NativeAccountOperation::RefreshSocial) {
        if (refresh(w, request, view)) std::snprintf(view.status, sizeof(view.status), "Mailbox updated.");
        return true;
    }
    auto* body = w.json.object();
    const char* route = nullptr;
    switch (operation) {
    case NativeAccountOperation::SendLetter:
        yyjson_mut_obj_add_str(w.json.output, body, "to", request.handle);
        yyjson_mut_obj_add_str(w.json.output, body, "title", request.subject);
        yyjson_mut_obj_add_str(w.json.output, body, "body", request.text);
        yyjson_mut_obj_add_str(w.json.output, body, "nonce", request.nonce);
        route = "social/send"; break;
    case NativeAccountOperation::ReadLetter: case NativeAccountOperation::DeleteLetter:
        yyjson_mut_obj_add_str(w.json.output, body, "id", request.target);
        route = operation == NativeAccountOperation::ReadLetter ? "social/read" : "social/delete"; break;
    case NativeAccountOperation::ChangeFriend:
        yyjson_mut_obj_add_str(w.json.output, body, "to", request.handle);
        yyjson_mut_obj_add_str(w.json.output, body, "action", request.action);
        route = "social/friend"; break;
    case NativeAccountOperation::BlockPlayer:
        yyjson_mut_obj_add_str(w.json.output, body, "to", request.handle);
        yyjson_mut_obj_add_bool(w.json.output, body, "blocked", request.blocked);
        route = "social/block"; break;
    default: return true;
    }
    if (!w.json.write(w.send, sizeof(w.send))) return true;
    unsigned status;
    auto* root = nativeAccount_Http(w, w.secrets.handle, route, true, true, view, &status);
    if (!root) { if (status == 401) { view.signed_in = false; view.social = {}; } return true; }
    if (operation == NativeAccountOperation::ReadLetter) {
        auto* item = yyjson_obj_get(root, "message"); NativeSocialMail opened{};
        if (!mail(item, opened) || !nativeAccount_String(yyjson_obj_get(item, "body"), view.social.body, sizeof(view.social.body), true)) {
            std::snprintf(view.status, sizeof(view.status), "The letter could not be read."); return true;
        }
        view.social.opened = opened; ++view.social.revision;
    }
    if (operation == NativeAccountOperation::SendLetter) {
        char id[44];
        if (!nativeAccount_String(yyjson_obj_get(root, "id"), id, sizeof(id)) || !nativeAccount_Secret(id)) {
            std::snprintf(view.status, sizeof(view.status), "Delivery is unconfirmed. Retry this letter to check it."); return true;
        }
        std::memcpy(view.social.last_sent_nonce, request.nonce, sizeof(request.nonce));
    }
    if (!refresh(w, request, view)) return true;
    if ((operation == NativeAccountOperation::ChangeFriend || operation == NativeAccountOperation::BlockPlayer) && !lookup(w, request.handle, view)) return true;
    std::snprintf(view.status, sizeof(view.status), operation == NativeAccountOperation::SendLetter ? "Letter saved in Sent. Queued letters will retry automatically." :
        operation == NativeAccountOperation::ChangeFriend ? "Friend request updated." : operation == NativeAccountOperation::DeleteLetter ? "Letter deleted." :
        operation == NativeAccountOperation::BlockPlayer ? (request.blocked ? "Player blocked." : "Player unblocked.") : "Letter opened.");
    return true;
}
