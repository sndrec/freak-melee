#include <sysdolphin/baselib/native_gobj_pool_storage.h>

// Epoch-zero execution fixtures have no scene GObj allocator. Actual retained
// object operations are exercised by script_receiver and Item lifecycle tests.
NativeMatchPool nativeGObjPool;
