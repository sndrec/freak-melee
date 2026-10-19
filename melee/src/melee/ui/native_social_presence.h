#pragma once
#ifdef __cplusplus
extern "C" {
#endif
struct NativeChatView;
struct NativeSignalingPresence;
typedef enum NativeSocialPresence {
    NativeSocialPresence_Unknown,
    NativeSocialPresence_Offline,
    NativeSocialPresence_Online,
} NativeSocialPresence;
// Published snapshots contain verified account IDs. Display names are unrelated
// to identity. A lobby member is present even if global chat is reconnecting.
NativeSocialPresence nativeSocialPresence_Account(const struct NativeChatView* chat,
    const struct NativeSignalingPresence* lobby, const char* account_id);
#ifdef __cplusplus
}
#endif
