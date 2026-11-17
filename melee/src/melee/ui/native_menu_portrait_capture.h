#pragma once
#include <melee/ft/native_fighter_presentation.h>
enum class NativePortraitCaptureState { Idle, Queued, Saving, Saved, Failed };
struct NativePortraitSubject { unsigned slot; NativePortraitCamera camera; };
bool nativeMenuPortraitCapture_Request(const NativePortraitSubject*, unsigned count, const char* path,
                                       unsigned width = 256, unsigned height = 256, bool shadow = false);
NativePortraitCaptureState nativeMenuPortraitCapture_State();
void nativeMenuPortraitCapture_Draw();
void nativeMenuPortraitCapture_Inspect();
// Authoring-channel capture: id,kind,costume,animation,frame,yaw,pitch,zoom,panX,panY.
// Writes menu-shot-ID.png in the isolated client's working directory.
void nativeMenuPortraitCapture_Stage(const char* specification);
// Draw the same square framing on the native menu's 640x480 canvas.
void nativeMenuPortraitCapture_Preview(const NativePortraitSubject*, unsigned count,
                                       float x, float y, float width, float height, bool shadow = false, float image_aspect = 0);
