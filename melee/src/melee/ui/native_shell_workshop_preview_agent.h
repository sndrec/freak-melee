#pragma once

#include <melee/ft/native_authoring_reply.h>

void nativeWorkshopPreview_Describe(NativeAuthoring::Reply&, yyjson_mut_val* object);
void nativeWorkshopPreview_Configure(yyjson_val*, NativeAuthoring::Reply&);
void nativeWorkshopPreview_Capture(yyjson_val*, yyjson_mut_val* state, NativeAuthoring::Reply&);
void nativeWorkshopPreview_CaptureStatus(yyjson_val*, NativeAuthoring::Reply&);
void nativeWorkshopPreview_UpdateCapture();
void nativeWorkshopPreview_Shutdown();
