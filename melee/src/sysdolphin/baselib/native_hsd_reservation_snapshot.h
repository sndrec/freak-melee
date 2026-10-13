#ifndef HSD_NATIVE_RESERVATION_SNAPSHOT_H
#define HSD_NATIVE_RESERVATION_SNAPSHOT_H

#include "native_hsd_reservation.h"

// Private in-memory records, not an asset/network format. No format versions.
typedef struct NativeHsdReservationSnapshotHeader {
    uint32_t owners, failed;
} NativeHsdReservationSnapshotHeader;

typedef struct NativeHsdReservationSnapshotOwner {
    const NativeHsdReservation* owner;
    const void* backing;
    size_t backing_bytes;
    uint32_t slot, bins;
} NativeHsdReservationSnapshotOwner;

typedef struct NativeHsdReservationSnapshotBin {
    const HSD_ObjAllocData* allocator;
    size_t alignment, bytes;
} NativeHsdReservationSnapshotBin;

#endif
