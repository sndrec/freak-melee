#pragma once
#include <aurora/aurora.h>

enum class NativeFeedbackAction { None, ConfirmSend, Send, Retry, Discard, Close };
struct NativeFeedbackPin { unsigned id; float x, y; char text[8193]; };
struct NativeFeedbackDraft {
    char id[37], captured_at[40], image_path[1024], context_path[1024];
    AuroraPresentedCapture image;
    NativeFeedbackPin pins[64];
    unsigned count, next_pin;
};
bool nativeFeedbackView_Open(NativeFeedbackDraft& draft);
void nativeFeedbackView_Close();
void nativeFeedbackView_Update();
bool nativeFeedbackView_Key(int scancode, bool shift);
NativeFeedbackAction nativeFeedbackView_Action();
void nativeFeedbackView_Status(const char* message, bool locked, bool retry, bool complete);
void nativeFeedbackView_Confirm();
