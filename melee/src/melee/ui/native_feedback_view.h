#pragma once
#include <melee/gm/native_feedback_capture.h>

enum class NativeFeedbackAction { None, ConfirmSend, ConfirmDiscard, Send, Retry, Discard, Close };
bool nativeFeedbackView_Open(NativeFeedbackDraft& draft);
void nativeFeedbackView_Close();
void nativeFeedbackView_Update();
bool nativeFeedbackView_Key(int scancode, bool shift);
NativeFeedbackAction nativeFeedbackView_Action();
void nativeFeedbackView_Status(const char* message, bool locked, bool retry, bool complete);
void nativeFeedbackView_Confirm(bool discard = false);
