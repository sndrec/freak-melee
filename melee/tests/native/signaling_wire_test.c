#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <melee/gm/native_signaling_wire.h>

static void put_u16(uint8_t* bytes, uint16_t value)
{
    bytes[0] = (uint8_t) (value >> 8);
    bytes[1] = (uint8_t) value;
}

static void put_u64(uint8_t* bytes, uint64_t value)
{
    for (int i = 7; i >= 0; --i) {
        bytes[i] = (uint8_t) value;
        value >>= 8;
    }
}

static size_t header(uint8_t* bytes, uint8_t type)
{
    memcpy(bytes, "FMS1", 4);
    bytes[4] = 5;
    bytes[5] = type;
    bytes[6] = 0;
    bytes[7] = 0;
    return 8;
}

static size_t string8(uint8_t* bytes, size_t offset, const char* value)
{
    size_t length = strlen(value);
    bytes[offset++] = (uint8_t) length;
    memcpy(bytes + offset, value, length);
    return offset + length;
}

static void finish(uint8_t* bytes, size_t size)
{
    put_u16(bytes + 6, (uint16_t) (size - 8));
}

static void test_client_encoding(void)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES];
    const uint8_t selection[] = { 0x46, 0x4D, 0x53, 0x4C };
    NativeSignalingUpdate update = {
        .selection = selection,
        .selection_size = sizeof(selection),
        .local_players = 2,
        .active_mask = 1,
        .waiting_mask = 2,
        .ready = true,
        .has_active_mask = true,
        .has_waiting_mask = true,
        .has_local_players = true,
        .has_ready = true,
        .has_selection = true,
    };
    size_t size = nativeSignalingWire_EncodeUpdate(&update, bytes,
                                                   sizeof(bytes));
    const uint8_t expected[] = {
        'F', 'M', 'S', '1', 5, 1, 0, 11, 0x3E, 1, 2, 1,
        0, 4, 0x46, 0x4D, 0x53, 0x4C, 2,
    };
    assert(size == sizeof(expected));
    assert(memcmp(bytes, expected, sizeof(expected)) == 0);

    uint8_t full_roster[335] = { 0 };
    update.selection = full_roster;
    update.selection_size = sizeof(full_roster);
    size = nativeSignalingWire_EncodeUpdate(&update, bytes, sizeof(bytes));
    assert(size == 8 + 1 + 1 + 1 + 1 + 2 + sizeof(full_roster) + 1);
    assert(memcmp(bytes + size - 1 - sizeof(full_roster), full_roster,
                  sizeof(full_roster)) == 0);
    update.selection_size = NATIVE_SIGNALING_MAX_SELECTION_BYTES + 1;
    assert(nativeSignalingWire_EncodeUpdate(&update, bytes,
                                           sizeof(bytes)) == 0);

    const uint8_t candidate[] = { 0x04, 0x72, 0x00, 0xFF };
    size = nativeSignalingWire_EncodeSignal(
        "peer_two", candidate, sizeof(candidate), bytes, sizeof(bytes));
    assert(size == 8 + 1 + 8 + 2 + sizeof(candidate));
    assert(bytes[5] == 2 && bytes[8] == 8);
    assert(memcmp(bytes + 9, "peer_two", 8) == 0);
    assert(memcmp(bytes + size - sizeof(candidate), candidate,
                  sizeof(candidate)) == 0);

    size = nativeSignalingWire_EncodeRequestPresence(bytes, sizeof(bytes));
    assert(size == 8 && bytes[5] == 3 && bytes[6] == 0 && bytes[7] == 0);

    const uint8_t match[] = { 0x4D, 0x41, 0x54, 0x43, 0x48 };
    size = nativeSignalingWire_EncodeSetMatch(match, sizeof(match), bytes,
                                              sizeof(bytes));
    assert(size == 8 + 2 + sizeof(match));
    assert(bytes[5] == 4 && bytes[8] == 0 && bytes[9] == sizeof(match));
    assert(memcmp(bytes + 10, match, sizeof(match)) == 0);
    assert(nativeSignalingWire_EncodeSetMatch(
               match, NATIVE_SIGNALING_MAX_MATCH_BYTES + 1, bytes,
               sizeof(bytes)) == 0);

    size = nativeSignalingWire_EncodeAssignSeats(
        "peer_two", 2, 1, bytes, sizeof(bytes));
    const uint8_t assignment[] = {
        'F', 'M', 'S', '1', 5, 5, 0, 11,
        8, 'p', 'e', 'e', 'r', '_', 't', 'w', 'o', 2, 1,
    };
    assert(size == sizeof(assignment));
    assert(memcmp(bytes, assignment, sizeof(assignment)) == 0);
    assert(nativeSignalingWire_EncodeAssignSeats(
               "peer_two", 16, 0, bytes, sizeof(bytes)) == 0);

    size = nativeSignalingWire_EncodeSetRotation(
        NativeRoomRotation_WinnerStays, bytes, sizeof(bytes));
    assert(size == 9 && bytes[5] == 6 && bytes[8] == 1);
    assert(nativeSignalingWire_EncodeSetRotation(3, bytes, sizeof(bytes)) == 0);

    size = nativeSignalingWire_EncodeReportMatch(
        42, "peer_two", 1, true, 0, bytes, sizeof(bytes));
    assert(size == 8 + 8 + 1 + 8 + 1 + 1 + 1);
    assert(bytes[5] == 7 && bytes[15] == 42 && bytes[16] == 8);
    assert(bytes[size - 3] == 1 && bytes[size - 2] == 1 && bytes[size - 1] == 0);
    assert(nativeSignalingWire_EncodeReportMatch(
               42, "peer_two", 4, true, 0, bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodeReportMatch(
               42, "peer_two", 1, true, 4, bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePickLobbyStage(0, 2, 32, bytes, sizeof(bytes)) == 18);
    assert(bytes[5] == 17 && bytes[16] == 2 && bytes[17] == 32);
    assert(!nativeSignalingWire_EncodePickLobbyStage(0, 4, 32, bytes, sizeof(bytes)));
    assert(!nativeSignalingWire_EncodePickLobbyStage(0, 0, 255, bytes, sizeof(bytes)));
}

static void test_server_decoding(void)
{
    uint8_t bytes[NATIVE_SIGNALING_MAX_MESSAGE_BYTES] = { 0 };
    const uint8_t selection[] = { 0x46, 0x4D, 0x53, 0x4C };
    size_t size = header(bytes, 0x82);
    put_u64(bytes + size, 42);
    size += 8;
    size = string8(bytes, size, "peer_one");
    bytes[size++] = 2;

    size = string8(bytes, size, "peer_one");
    size = string8(bytes, size, "Alice");
    size = string8(bytes, size, "build-a");
    bytes[size++] = 2;
    bytes[size++] = 1;
    bytes[size++] = 2;
    bytes[size++] = 1;
    put_u64(bytes + size, 7);
    size += 8;
    size_t first_rank_offset = size;
    bytes[size++] = 0;
    bytes[size++] = 1;
    bytes[size++] = 0;
    bytes[size++] = 0;
    put_u16(bytes + size, sizeof(selection));
    size += 2;
    memcpy(bytes + size, selection, sizeof(selection));
    size += sizeof(selection);

    const size_t account_offset = size;
    size = string8(bytes, size, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
    size = string8(bytes, size, "twilight");

    size = string8(bytes, size, "peer_two");
    size = string8(bytes, size, "Bob");
    size = string8(bytes, size, "build-a");
    bytes[size++] = 0;
    bytes[size++] = 0;
    bytes[size++] = 0;
    bytes[size++] = 0;
    put_u64(bytes + size, 9);
    size += 8;
    memset(bytes + size, 0, 4);
    size += 4;
    put_u16(bytes + size, 0);
    size += 2;
    size = string8(bytes, size, "");
    size = string8(bytes, size, "");
    put_u64(bytes + size, 3);
    size += 8;
    const uint8_t match[] = { 0x46, 0x4D, 0x4C };
    put_u16(bytes + size, sizeof(match));
    size += 2;
    memcpy(bytes + size, match, sizeof(match));
    size += sizeof(match);
    bytes[size++] = NativeRoomRotation_WinnerOut;
    size = string8(bytes, size, "peer_one");
    bytes[size++] = 0;
    bytes[size++] = 32;
    finish(bytes, size);

    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_Presence);
    assert(decoded.data.presence.revision == 42);
    assert(strcmp(decoded.data.presence.owner_peer_id, "peer_one") == 0);
    assert(decoded.data.presence.participant_count == 2);
    assert(strcmp(decoded.data.presence.participants[0].name, "Alice") == 0);
    assert(!strcmp(decoded.data.presence.participants[0].account_id, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"));
    assert(!strcmp(decoded.data.presence.participants[0].account_handle, "twilight"));
    assert(!*decoded.data.presence.participants[1].account_id && !*decoded.data.presence.participants[1].account_handle);
    assert(decoded.data.presence.participants[0].local_players == 2);
    assert(decoded.data.presence.participants[0].active_mask == 1);
    assert(decoded.data.presence.participants[0].waiting_mask == 2);
    assert(decoded.data.presence.participants[0].waiting_ranks[1] == 1);
    assert(decoded.data.presence.participants[0].ready);
    assert(decoded.data.presence.participants[0].selection_size ==
           sizeof(selection));
    assert(memcmp(decoded.data.presence.participants[0].selection, selection,
                  sizeof(selection)) == 0);
    assert(decoded.data.presence.participants[1].active_mask == 0);
    assert(decoded.data.presence.participants[1].join_order == 9);
    assert(decoded.data.presence.participants[1].selection_size == 0);
    assert(decoded.data.presence.match_epoch == 3);
    assert(decoded.data.presence.match_size == sizeof(match));
    assert(decoded.data.presence.rotation_policy == NativeRoomRotation_WinnerOut);
    assert(strcmp(decoded.data.presence.stage_peer_id, "peer_one") == 0);
    assert(decoded.data.presence.stage_seat == 0 && decoded.data.presence.stage == 32);
    assert(memcmp(decoded.data.presence.match, match, sizeof(match)) == 0);

    assert(!nativeSignalingWire_DecodeServer(bytes, size - 1, &decoded));
    bytes[account_offset + 1] = '+';
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[account_offset + 1] = 'A';
    bytes[first_rank_offset + 1] = 0;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[first_rank_offset + 1] = 1;
    bytes[4] = 2;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
}

static void test_chat_wire(void)
{
    uint8_t bytes[1024];
    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_EncodePostChat(0, "Hello", bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePostChat(1, "   ", bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePostChat(1, "bad\ntext", bytes, sizeof(bytes)) == 0);
    assert(nativeSignalingWire_EncodePostChat(1, "\xc0\xaf", bytes, sizeof(bytes)) == 0);
    size_t size = nativeSignalingWire_EncodePostChat(1, "Hello", bytes, sizeof(bytes));
    assert(size == 19 && bytes[5] == 8 && bytes[11] == 1 && bytes[13] == 5);
    assert(nativeSignalingWire_EncodePostChat(1, "Hello", bytes, 18) == 0);
    size = header(bytes, 0x85);
    put_u64(bytes + size, 42); size += 8;
    bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 7;
    size = string8(bytes, size, "spectator_two");
    size = string8(bytes, size, "Reader");
    put_u16(bytes + size, 5); size += 2;
    memcpy(bytes + size, "Hello", 5); size += 5;
    finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_Chat);
    assert(decoded.data.chat.kind == NativeRoomChatEvent_Entry);
    assert(decoded.data.chat.entry.sequence == 42 && decoded.data.chat.entry.nonce == 7);
    assert(!strcmp(decoded.data.chat.entry.text, "Hello"));
    for (size_t i = 0; i < size; ++i) assert(!nativeSignalingWire_DecodeServer(bytes, i, &decoded));
    bytes[size - 1] = 0xff;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x86); bytes[size++] = 64; finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[8] = 65;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x87); put_u64(bytes + size, 42); size += 8; finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x88); memset(bytes + size, 0, 4); size += 4;
    size = string8(bytes, size, "Wait a moment."); finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.data.chat.kind == NativeRoomChatEvent_Rejected);
}

static void test_quick_wire(void)
{
    uint8_t bytes[160];
    NativeSignalingServerMessage decoded;
    size_t size = nativeSignalingWire_EncodeConfigureQuick(2, bytes, sizeof(bytes));
    assert(size == 9 && bytes[5] == 10 && bytes[8] == 2);
    assert(!nativeSignalingWire_EncodeConfigureQuick(3, bytes, sizeof(bytes)));
    assert(!nativeSignalingWire_EncodeConfigureQuick(1, bytes, 8));
    size = nativeSignalingWire_EncodeQueueQuick(true, bytes, sizeof(bytes));
    assert(size == 9 && bytes[5] == 11 && bytes[8] == 1);
    size = nativeSignalingWire_EncodeQueueQuick(false, bytes, sizeof(bytes));
    assert(size == 9 && bytes[8] == 0);
    assert(nativeSignalingWire_EncodeRequestQuick(bytes, sizeof(bytes)) == 8 && bytes[5] == 12);

    size = header(bytes, 0x89);
    bytes[size++] = 2; bytes[size++] = 2;
    put_u64(bytes + size, UINT64_C(0x0102030405060708)); size += 8;
    size = string8(bytes, size, "ABCDEF123456");
    size = string8(bytes, size, "0123456789abcdef0123456789abcdef");
    size_t controls = size;
    bytes[size++] = 1; bytes[size++] = 5; bytes[size++] = 1;
    put_u16(bytes + size, 3000); size += 2;
    put_u64(bytes + size, 42); size += 8;
    bytes[size++] = 0; bytes[size++] = 3; bytes[size++] = 255; bytes[size++] = 255; bytes[size++] = 255;
    finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_QuickState);
    assert(decoded.data.quick.generation == UINT64_C(0x0102030405060708));
    assert(decoded.data.quick.team == 1 && decoded.data.quick.stage == 5);
    assert(decoded.data.quick.phase == 1 && decoded.data.quick.remaining_ms == 3000);
    assert(decoded.data.quick.completed_epoch == 42 && decoded.data.quick.chooser == 0 && decoded.data.quick.votes[0] == 3);
    assert(!strcmp(decoded.data.quick.match_room, "ABCDEF123456"));
    assert(!strcmp(decoded.data.quick.admission, "0123456789abcdef0123456789abcdef"));
    for (size_t i = 0; i < size; ++i) assert(!nativeSignalingWire_DecodeServer(bytes, i, &decoded));
    bytes[controls + 1] = 6;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[controls + 1] = 5; bytes[controls] = 2;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[controls] = 1; bytes[19] = '/';
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[19] = 'A'; bytes[32] = '&';
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));

    size = header(bytes, 0x89);
    bytes[size++] = 1; bytes[size++] = 1; put_u64(bytes + size, 9); size += 8;
    bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0;
    bytes[size++] = 0; bytes[size++] = 0; bytes[size++] = 0;
    put_u64(bytes + size, 0); size += 8;
    memset(bytes + size, 255, 5); size += 5;
    finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.data.quick.state == NativeQuickState_Searching);
    size = header(bytes, 0x8A);
    size = string8(bytes, size, "Only the party owner can enter the queue."); finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_QuickRejected);
    assert(strstr(decoded.data.error, "party owner"));
    const uint8_t digest[32] = {1};
    size = nativeSignalingWire_EncodePrepareQuick(7, true, digest, bytes, sizeof(bytes));
    assert(size == 51 && bytes[5] == 13 && bytes[15] == 7 && bytes[16] == 1);
    assert(bytes[17] == 0 && bytes[18] == 32 && bytes[19] == 1);
    assert(nativeSignalingWire_EncodePrepareQuick(7, false, NULL, bytes, sizeof(bytes)) == 19);
    assert(!nativeSignalingWire_EncodePrepareQuick(7, true, NULL, bytes, sizeof(bytes)));
    size = nativeSignalingWire_EncodeReportQuick(42, 1, bytes, sizeof(bytes));
    assert(size == 17 && bytes[5] == 14 && bytes[15] == 42 && bytes[16] == 1);
    assert(!nativeSignalingWire_EncodeReportQuick(42, 2, bytes, sizeof(bytes)));
    assert(!nativeSignalingWire_EncodeReportQuick(0, 255, bytes, sizeof(bytes)));
    size = nativeSignalingWire_EncodeVoteQuickStage(42, 1, 5, bytes, sizeof(bytes));
    assert(size == 18 && bytes[5] == 15 && bytes[15] == 42 && bytes[16] == 1 && bytes[17] == 5);
    assert(!nativeSignalingWire_EncodeVoteQuickStage(42, 2, 0, bytes, sizeof(bytes)));
    assert(!nativeSignalingWire_EncodeVoteQuickStage(42, 0, 6, bytes, sizeof(bytes)));
    size = nativeSignalingWire_EncodeUnreadyQuick(0, bytes, sizeof(bytes));
    assert(size == 16 && bytes[5] == 16 && bytes[15] == 0);
    size = nativeSignalingWire_EncodeUnreadyQuick(42, bytes, sizeof(bytes));
    assert(size == 16 && bytes[15] == 42);
    assert(!nativeSignalingWire_EncodeUnreadyQuick(42, bytes, 15));
}

static void test_lobby_directory(void)
{
    uint8_t bytes[2048];
    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_EncodeLobbies(8, bytes, sizeof(bytes)) == 10);
    assert(bytes[5] == 19 && bytes[8] == 0 && bytes[9] == 8);
    assert(nativeSignalingWire_EncodeListing("Twilight's lobby", true, bytes, sizeof(bytes)));
    assert(bytes[5] == 18 && bytes[8] == 16);
    assert(!nativeSignalingWire_EncodeListing("", true, bytes, sizeof(bytes)));
    assert(!nativeSignalingWire_EncodeListing("bad\nname", true, bytes, sizeof(bytes)));
    size_t size = header(bytes, 0x8B);
    put_u16(bytes + size, 8); size += 2;
    put_u16(bytes + size, 9); size += 2;
    bytes[size++] = 1;
    size = string8(bytes, size, "BROWSE77");
    size = string8(bytes, size, "Twilight's lobby");
    size = string8(bytes, size, "Twilight");
    size_t counts = size;
    bytes[size++] = 3; bytes[size++] = 1; bytes[size++] = 2; bytes[size++] = 4; bytes[size++] = 0;
    finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.type == NativeSignalingServerMessage_Lobbies);
    assert(decoded.data.directory.offset == 8 && decoded.data.directory.total == 9);
    assert(!strcmp(decoded.data.directory.entries[0].title, "Twilight's lobby"));
    for (size_t cut = 0; cut < size; ++cut) assert(!nativeSignalingWire_DecodeServer(bytes, cut, &decoded));
    bytes[counts] = 5;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    bytes[counts] = 3; bytes[12] = 9;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    size = header(bytes, 0x8C); size = string8(bytes, size, "A lobby"); bytes[size++] = 1; finish(bytes, size);
    assert(nativeSignalingWire_DecodeServer(bytes, size, &decoded));
    assert(decoded.data.listing.listed && !strcmp(decoded.data.listing.title, "A lobby"));
    bytes[size - 1] = 2;
    assert(!nativeSignalingWire_DecodeServer(bytes, size, &decoded));
}

static void test_shared_lobby_rules(void)
{
    NativeRoomRules rules={ .mode=1,.time=6,.stocks=4,.damage=10,.stage_policy=4,
        .stock_time=6,.pause=1,.sd_penalty=2,.item_frequency=255,.teams=1,
        .item_mask=UINT64_C(0x1122334455667788),.stage_mask=0x1FFFFFFF };
    uint8_t record[NATIVE_ROOM_RULES_BYTES],bytes[128];
    NativeRoomRules decoded_rules;
    assert(nativeRoomRules_Encode(&rules,record));
    assert(nativeRoomRules_Decode(&decoded_rules,record,sizeof(record)));
    assert(decoded_rules.item_mask==rules.item_mask && decoded_rules.sd_penalty==2 && decoded_rules.stage_mask==rules.stage_mask);
    assert(nativeSignalingWire_EncodeRequestLobbyState(bytes,sizeof(bytes))==8 && bytes[5]==21);
    assert(nativeSignalingWire_EncodeLobbyRules(&rules,bytes,sizeof(bytes))==35 && bytes[5]==20);
    assert(bytes[8]==0 && bytes[9]==25 && !memcmp(bytes+10,record,sizeof(record)));
    size_t size=header(bytes,0x8D);
    put_u64(bytes+size,17);size+=8;bytes[size++]=1;put_u16(bytes+size,3000);size+=2;
    put_u16(bytes+size,sizeof(record));size+=2;memcpy(bytes+size,record,sizeof(record));size+=sizeof(record);finish(bytes,size);
    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_DecodeServer(bytes,size,&decoded));
    assert(decoded.type==NativeSignalingServerMessage_LobbyState && decoded.data.lobby.has_rules);
    assert(decoded.data.lobby.phase==1 && decoded.data.lobby.remaining_ms==3000 && decoded.data.lobby.revision==17);
    for (size_t cut=0;cut<size;++cut) assert(!nativeSignalingWire_DecodeServer(bytes,cut,&decoded));
    bytes[size-25+5]=5;
    assert(!nativeSignalingWire_DecodeServer(bytes,size,&decoded));
    rules.stocks=0;assert(!nativeRoomRules_Encode(&rules,record));
}

static void test_stage_cursor(void)
{
    NativeStageCursor cursor = { .epoch=7, .revision=13, .sequence=41,
                                 .x=-27*256, .y=19*256, .tile=29, .seat=1 };
    uint8_t bytes[128], request[64];
    size_t size = nativeSignalingWire_EncodeStageCursor(&cursor, request, sizeof(request));
    assert(size == 34 && request[5] == 22 && request[28] == 0xE5 && request[29] == 0);
    size_t length = header(bytes, 0x8E);
    length = string8(bytes, length, "chooser");
    memcpy(bytes+length, request+8, size-8); length += size-8; finish(bytes,length);
    NativeSignalingServerMessage decoded;
    assert(nativeSignalingWire_DecodeServer(bytes,length,&decoded));
    assert(decoded.type == NativeSignalingServerMessage_StageCursor);
    const NativeStageCursor* result = &decoded.data.stage_cursor;
    assert(!strcmp(result->peer_id,"chooser") && result->sequence==41 &&
           result->epoch==7 && result->revision==13 && result->seat==1 &&
           result->x==-27*256 && result->y==19*256 && result->tile==29);
    for(size_t cut=0;cut<length;++cut)assert(!nativeSignalingWire_DecodeServer(bytes,cut,&decoded));
    bytes[length-2]=30;assert(!nativeSignalingWire_DecodeServer(bytes,length,&decoded));
    cursor.sequence=0;assert(!nativeSignalingWire_EncodeStageCursor(&cursor,request,sizeof(request)));
}

int main(void)
{
    test_client_encoding();
    test_server_decoding();
    test_chat_wire();
    test_quick_wire();
    test_lobby_directory();
    test_shared_lobby_rules();
    test_stage_cursor();
    puts("native signaling wire tests passed");
    return 0;
}
