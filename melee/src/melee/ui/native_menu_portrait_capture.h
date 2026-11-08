#pragma once
struct NativePortraitCamera {
    float yaw = -.35f, pitch = .12f, zoom = 1, pan_x = 0, pan_y = 0, frame = 0;
};
enum class NativePortraitCaptureState { Idle, Queued, Saving, Saved, Failed };
struct NativePortraitSubject { unsigned slot; NativePortraitCamera camera; };
bool nativeMenuPortraitCapture_Request(const NativePortraitSubject*, unsigned count, const char* path);
NativePortraitCaptureState nativeMenuPortraitCapture_State();
void nativeMenuPortraitCapture_Draw();
void nativeMenuPortraitCapture_Inspect();
