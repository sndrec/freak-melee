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
    puts("Reliable package transfer survives lost data and acknowledgements");
    return 0;
}
