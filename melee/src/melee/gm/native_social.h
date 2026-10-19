#pragma once
#include <stdint.h>

struct NativeSocialProfile { char id[44], handle[25], name[33], bio[257]; };
struct NativeSocialMail {
    char id[44], peer[44], handle[25], name[33], title[65], kind[17], status[16];
    char room[13];
    uint64_t created, expires;
    unsigned folder;
    bool unread;
};
struct NativeSocialFriend { char id[44], handle[25], name[33], state[9]; };
struct NativeSocialView {
    unsigned revision, folder, offset, friends_offset, total, unread, friends_total, count, friend_count;
    bool loaded;
    bool profile_blocked;
    char relationship[9];
    NativeSocialProfile profile;
    NativeSocialMail messages[8], opened;
    NativeSocialFriend friends[8];
    char body[2049], last_sent_nonce[44], join_id[44], join_room[13];
};
