#ifndef MELEE_CM_NATIVE_CAMERA_SUBJECT_SNAPSHOT_H
#define MELEE_CM_NATIVE_CAMERA_SUBJECT_SNAPSHOT_H

#include "types.h"
#include <stdint.h>
#include <stddef.h>

enum { NATIVE_CAMERA_SUBJECT_CAPACITY = 256 };
typedef struct NativeCameraSubjectSnapshotHeader {
    uint64_t epoch, object_epoch;
    CmSubject* backing;
    CmSubject* free_head;
    CmSubject* active_head;
    CmSubject* active_tail;
    uint32_t count, reserved;
} NativeCameraSubjectSnapshotHeader;

// Complete requested prefix, including free records and original links, follows
// the header. Borrowing fighter/item/Ground records are separate companions.
size_t nativeCameraSubjects_SnapshotBytes(void);
void nativeCameraSubjects_BeginScene(void);
void nativeCameraSubjects_EndScene(void);
bool nativeCameraSubjects_Save(void*, size_t capacity, size_t* written);
bool nativeCameraSubjects_Validate(const void*, size_t bytes);
bool nativeCameraSubjects_Load(const void*, size_t bytes);

#endif
