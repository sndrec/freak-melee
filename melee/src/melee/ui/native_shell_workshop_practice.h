#pragma once
#include <melee/ft/native_authoring_reply.h>
bool nativeWorkshopPractice_Start(yyjson_val*, yyjson_mut_val* inspected, NativeAuthoring::Reply&);
void nativeWorkshopPractice_Agent(const char* operation, yyjson_val*, NativeAuthoring::Reply&);
