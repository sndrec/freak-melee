#include <melee/gm/native_content_transfer.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    enum { PACKAGE_BYTES = 2 * 1024 * 1024 + 103 };
    uint8_t* package = malloc(PACKAGE_BYTES);
    assert(package != NULL);
    for (size_t i = 0; i < PACKAGE_BYTES; ++i) {
        package[i] = (uint8_t) (i * 37 + i / 127);
    }
    NativeContentId id = { .bytes = { 0x5A, 0x91, 0xE3 } };
    NativeContentTransferSend send = { 0 };
    NativeContentTransferReceive receive = { 0 };
    assert(nativeContentTransfer_StartSend(&send, &id, package,
                                            PACKAGE_BYTES));
    nativeContentTransfer_StartReceive(&receive, &id);

    unsigned sent = 0;
    unsigned controls = 0;
    for (uint64_t now = 0; now < 180000 && !send.complete; now += 10) {
        uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
        size_t size = nativeContentTransfer_NextReceive(&receive, now, packet);
        if (size != 0 && ++controls % 17 != 0) {
            nativeContentTransfer_SendPacket(&send, packet, size, now);
        }
        for (unsigned burst = 0; burst < 8; ++burst) {
            size = nativeContentTransfer_NextSend(&send, now, packet);
            if (size == 0) {
                break;
            }
            assert(size <= sizeof(packet));
            if (++sent % 13 != 0) {
                nativeContentTransfer_ReceivePacket(&receive, packet, size,
                                                    now);
            }
        }
    }
    assert(send.complete && receive.complete && !receive.failed);
    assert(receive.size == PACKAGE_BYTES);
    assert(memcmp(receive.bytes, package, PACKAGE_BYTES) == 0);
    assert(sent > PACKAGE_BYTES / NATIVE_CONTENT_TRANSFER_CHUNK_BYTES);

    // Keep later chunks in their final storage while an early chunk is
    // missing. Filling that hole should advance through the whole window,
    // without requiring those later chunks to be transmitted again.
    NativeContentTransferSend ordered_send={0};
    NativeContentTransferReceive reordered={0};
    enum { REORDER_BYTES=4*512-13 };
    assert(nativeContentTransfer_StartSend(&ordered_send,&id,package,REORDER_BYTES));
    nativeContentTransfer_StartReceive(&reordered,&id);
    uint8_t reordered_packets[4][576],control[576];size_t lengths[4];
    size_t control_size=nativeContentTransfer_NextSend(&ordered_send,1,control);
    nativeContentTransfer_ReceivePacket(&reordered,control,control_size,1);
    control_size=nativeContentTransfer_NextReceive(&reordered,2,control);
    nativeContentTransfer_SendPacket(&ordered_send,control,control_size,2);
    for(unsigned i=0;i<4;++i)lengths[i]=nativeContentTransfer_NextSend(&ordered_send,3,reordered_packets[i]);
    for(unsigned i=1;i<4;++i) {
        nativeContentTransfer_ReceivePacket(&reordered,reordered_packets[i],lengths[i],4);
        assert(!reordered.received && !reordered.complete && !reordered.failed);
    }
    nativeContentTransfer_ReceivePacket(&reordered,reordered_packets[2],lengths[2],5);
    assert(!reordered.received && !reordered.failed);
    // Time out and retransmit the missing first chunk. Its acknowledgement
    // can jump past the retransmission cursor to previously sent later data.
    control_size=nativeContentTransfer_NextSend(&ordered_send,1000,control);
    nativeContentTransfer_ReceivePacket(&reordered,control,control_size,1000);
    assert(reordered.complete && reordered.received==REORDER_BYTES && !reordered.received_window);
    assert(!memcmp(package,reordered.bytes,REORDER_BYTES));
    control_size=nativeContentTransfer_NextReceive(&reordered,1001,control);
    nativeContentTransfer_SendPacket(&ordered_send,control,control_size,1001);
    assert(ordered_send.complete);
    nativeContentTransfer_StopReceive(&reordered);

    nativeContentTransfer_StartReceive(&reordered,&id);
    ordered_send=(NativeContentTransferSend){0};
    assert(nativeContentTransfer_StartSend(&ordered_send,&id,package,REORDER_BYTES));
    control_size=nativeContentTransfer_NextSend(&ordered_send,1,control);
    nativeContentTransfer_ReceivePacket(&reordered,control,control_size,1);
    nativeContentTransfer_ReceivePacket(&reordered,reordered_packets[1],lengths[1],2);
    reordered_packets[1][lengths[1]-1]^=1;
    nativeContentTransfer_ReceivePacket(&reordered,reordered_packets[1],lengths[1],3);
    assert(reordered.failed && !reordered.received);
    nativeContentTransfer_StopReceive(&reordered);

    NativeContentId different = id;
    different.bytes[0] ^= 1;
    uint8_t packet[NATIVE_CONTENT_TRANSFER_PACKET_CAPACITY];
    size_t size = nativeContentTransfer_Missing(&different, packet);
    nativeContentTransfer_ReceivePacket(&receive, packet, size, 180000);
    assert(!receive.failed);
    size = nativeContentTransfer_Missing(&id, packet);
    nativeContentTransfer_ReceivePacket(&receive, packet, size, 180000);
    assert(receive.failed);
    nativeContentTransfer_StopReceive(&receive);
    free(package);
    puts("Reliable package transfer survives loss, reordered/duplicate chunks, short final chunks and conflicting data");
    return 0;
}
