#pragma once
#include <stdint.h>
#include "native_social.h"

// Account I/O belongs to the menu, outside match state and rollback.
enum class NativeAccountOperation {
    Refresh, Register, Login, Recover, Approve, AddPasskey, SaveProfile,
    Logout, RevokeSession, RevokeAll, RemovePasskey, RotateRecovery,
    AcknowledgeRecovery, RefreshSocial, LookupProfile, SendLetter, ReadLetter,
    DeleteLetter, ChangeFriend, BlockPlayer
};
struct NativeAccountRequest {
    NativeAccountOperation operation;
    char handle[25], name[33], bio[257], recovery[44], target[1401];
    char subject[65], text[2049], nonce[44], action[16];
    unsigned folder, offset, friends_offset;
    bool blocked;
};
struct NativeAccountKey { char id[1401], name[65]; };
struct NativeAccountSession { char id[44], name[65]; bool current; };
struct NativeAccountView {
    bool available, busy, cancellable, signed_in, saved, recovery_pending;
    unsigned revision, profile_revision, key_count, session_count;
    char handle[25], name[33], bio[257], id[44], status[192];
    NativeAccountKey keys[8];
    NativeAccountSession sessions[16];
    NativeSocialView social;
};
bool nativeAccount_Start(const char* root, const char* endpoint, void* window);
bool nativeAccount_Submit(const NativeAccountRequest& request);
bool nativeAccount_Poll(NativeAccountView& view);
void nativeAccount_Cancel();
void nativeAccount_Shutdown();
// Explicit export only. Ordinary views and diagnostics never contain secrets.
bool nativeAccount_SaveRecovery(void* window);
bool nativeAccount_NewNonce(char (&nonce)[44]);
