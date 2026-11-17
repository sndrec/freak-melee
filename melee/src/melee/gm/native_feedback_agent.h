#pragma once
#include <melee/ft/native_authoring_reply.h>

namespace NativeFeedbackAgent {
void handle(const char* method, yyjson_val* arguments, yyjson_mut_val* captured_state,
            NativeAuthoring::Reply&);
void list(NativeAuthoring::Reply&);
void update();
}
