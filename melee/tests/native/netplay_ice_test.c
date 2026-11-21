#define WIN32_LEAN_AND_MEAN
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include <melee/gm/native_netplay_ice.h>

enum {
    TEST_TIMEOUT_MS = 5000
};

static void require(bool condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}

static bool connected(const NativeNetplayIcePeer* peer)
{
    NativeNetplayIceState state = nativeNetplayIce_State(peer);
    return state == NativeNetplayIceState_Connected ||
           state == NativeNetplayIceState_Completed;
}

static bool wait_for_gathering(const NativeNetplayIcePeer* first,
                               const NativeNetplayIcePeer* second)
{
    uint64_t deadline = GetTickCount64() + TEST_TIMEOUT_MS;
    while (GetTickCount64() < deadline) {
        if (nativeNetplayIce_GatheringDone(first) &&
            nativeNetplayIce_GatheringDone(second))
        {
            return true;
        }
        Sleep(1);
    }
    return false;
}

static bool wait_for_connection(const NativeNetplayIcePeer* first,
                                const NativeNetplayIcePeer* second)
{
    uint64_t deadline = GetTickCount64() + TEST_TIMEOUT_MS;
    while (GetTickCount64() < deadline) {
        if (connected(first) && connected(second)) {
            return true;
        }
        if (nativeNetplayIce_State(first) == NativeNetplayIceState_Failed ||
            nativeNetplayIce_State(second) == NativeNetplayIceState_Failed)
        {
            return false;
        }
        Sleep(1);
    }
    return false;
}

static bool wait_for_datagram(NativeNetplayIcePeer* peer,
                              NativeNetplayIceDatagram* datagram)
{
    uint64_t deadline = GetTickCount64() + TEST_TIMEOUT_MS;
    while (GetTickCount64() < deadline) {
        if (nativeNetplayIce_PollDatagram(peer, datagram)) {
            return true;
        }
        Sleep(1);
    }
    return false;
}

int main(void)
{
    NativeNetplayIceConfig config = { 0 };
    NativeNetplayIcePeer first;
    NativeNetplayIcePeer second;
    require(nativeNetplayIce_Init(&first, &config) &&
                nativeNetplayIce_Init(&second, &config),
            "create two allocation-free game-facing ICE peers");
    require(nativeNetplayIce_StartGathering(&first) &&
                nativeNetplayIce_StartGathering(&second),
            "start both candidate gatherers");
    require(wait_for_gathering(&first, &second),
            "host candidate gathering completes");

    char first_description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    char second_description[NATIVE_NETPLAY_ICE_DESCRIPTION_CAPACITY];
    require(nativeNetplayIce_GetLocalDescription(&first, first_description,
                                                 sizeof(first_description)) &&
                nativeNetplayIce_GetLocalDescription(
                    &second, second_description, sizeof(second_description)),
            "read complete local ICE descriptions");
    require(
        nativeNetplayIce_SetRemoteDescription(&first, second_description) &&
            nativeNetplayIce_SetRemoteDescription(&second,
                                                  first_description) &&
            nativeNetplayIce_SetRemoteGatheringDone(&first) &&
            nativeNetplayIce_SetRemoteGatheringDone(&second),
        "exchange descriptions without trickle signaling");
    require(wait_for_connection(&first, &second),
            "two local ICE agents select a candidate pair");

    uint8_t first_message[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
    for (size_t i = 0; i < sizeof(first_message); ++i) {
        first_message[i] = (uint8_t)(37 * i);
    }
    static const uint8_t second_message[] = { 0x46, 0x4D, 0x49, 0x31, 0x5A };
    require(
        nativeNetplayIce_Send(&first, first_message, sizeof(first_message)) == NativeNetplayIceSend_Sent &&
            nativeNetplayIce_Send(&second, second_message,
                                 sizeof(second_message)) == NativeNetplayIceSend_Sent,
        "send datagrams through the selected ICE pair");
    NativeNetplayIceDatagram first_received;
    NativeNetplayIceDatagram second_received;
    require(wait_for_datagram(&first, &first_received) &&
                wait_for_datagram(&second, &second_received),
            "game thread drains both fixed receive queues");
    require(first_received.size == sizeof(second_message) &&
                memcmp(first_received.bytes, second_message,
                       sizeof(second_message)) == 0 &&
                second_received.size == sizeof(first_message) &&
                memcmp(second_received.bytes, first_message,
                       sizeof(first_message)) == 0,
            "ICE preserves full-size fighter datagrams and small input datagrams");

    char first_local[NATIVE_NETPLAY_ICE_ADDRESS_CAPACITY];
    char first_remote[NATIVE_NETPLAY_ICE_ADDRESS_CAPACITY];
    require(nativeNetplayIce_GetSelectedAddresses(
                &first, first_local, sizeof(first_local), first_remote,
                sizeof(first_remote)),
            "selected addresses are available for diagnostics");
    require(nativeNetplayIce_DroppedDatagrams(&first) == 0 &&
                nativeNetplayIce_DroppedDatagrams(&second) == 0,
            "fixed callback queues do not overflow");

    nativeNetplayIce_Shutdown(&second);
    nativeNetplayIce_Shutdown(&first);
    printf("ICE selected %s -> %s and exchanged fixed-queue datagrams\n",
           first_local, first_remote);
    return 0;
}
