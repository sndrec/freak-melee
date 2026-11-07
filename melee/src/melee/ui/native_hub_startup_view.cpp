#include "native_hub_startup.h"
#include "native_hub_account.h"
#include <melee/gm/native_profile_input.h>
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>

namespace {
    constexpr uint32_t gold = 0xE5C361FF, muted = 0xB9ADBDFF;
    void line(const char* text, float y, float size = 11, uint32_t color = 0xEEE9EDFF, bool centered = true)
    { nativeMenuPaint_Text(text, 124, y, 390, size, color, NativeMenuFont_Notice, centered); }
    void next(const char* label, bool enabled = true)
    {
        bool single = nativeHubStartup.step == NativeStartup_Welcome || nativeHubStartup.step == NativeStartup_Done;
        nativeHub_Button("setup-next", label, single ? 220 : 314, 402, 200, 28, HubAction::StartupNext, 0, true, enabled);
    }
    void stick(float x, float y, unsigned first)
    {
        const auto& s = nativeHubStartup;
        bool test = s.step == NativeStartup_Test;
        float vx = test ? nativeControllerCalibration_Normalize(&s.profile.calibration, first, s.raw[first]) : s.raw[first];
        float vy = test ? nativeControllerCalibration_Normalize(&s.profile.calibration, first + 1, s.raw[first + 1]) : s.raw[first + 1];
        nativeMenuPaint_Round(x - 1, y - 1, 104, 104, 52, 0x9A90AE66);
        nativeMenuPaint_Round(x, y, 102, 102, 51, 0x080910D8);
        nativeMenuPaint_Rect(x + 50, y + 4, 1, 94, 0x766E7FFF);
        nativeMenuPaint_Rect(x + 4, y + 50, 94, 1, 0x766E7FFF);
        nativeMenuPaint_Round(x + 47 + vx * 44 / 32767, y + 47 - vy * 44 / 32767, 8, 8, 4, gold);
        nativeMenuPaint_Text(first == 0 ? "Main stick" : "C-stick", x, y + 108, 102, 10, muted, NativeMenuFont_Notice, true);
        if (s.step == NativeStartup_Sticks) {
            char progress[64]; unsigned directions = 0;
            for (unsigned axis = first; axis < first + 2; ++axis) {
                int center = (int(s.capture.rest_low[axis]) + s.capture.rest_high[axis]) / 2;
                directions += s.capture.low[axis] < center - 8192;
                directions += s.capture.high[axis] > center + 8192;
            }
            std::snprintf(progress, sizeof(progress), "%u / 4 directions", directions);
            nativeMenuPaint_Text(progress, x, y + 123, 102, 9, gold, NativeMenuFont_Notice, true);
        }
    }
    void triggers(float y)
    {
        const auto& s = nativeHubStartup;
        for (unsigned i = 0; i < 2; ++i) {
            float x = 137 + i * 197;
            int value = s.step == NativeStartup_Test
                ? nativeControllerCalibration_Normalize(&s.profile.calibration, 4 + i, s.raw[4 + i]) : s.raw[4 + i];
            nativeMenuPaint_Text(i ? "R" : "L", x, y, 20, 11, gold);
            nativeMenuPaint_Round(x + 22, y, 142, 13, 3, 0x080910FF);
            nativeMenuPaint_Rect(x + 24, y + 2, std::clamp(value * 138.0f / 32767, 0.f, 138.f), 9, gold);
            if (s.step == NativeStartup_Triggers) {
                bool pressed = s.capture.high[4 + i] - s.capture.rest_high[4 + i] >= 8192;
                nativeMenuPaint_Text(pressed ? "Pressed" : "Press fully", x + 22, y + 17, 142, 9, pressed ? gold : muted);
            }
        }
    }
}
void nativeHubStartup_RenderFrame()
{
    const auto& s = nativeHubStartup;
    const unsigned group = s.step == NativeStartup_Welcome ? 0 : s.step <= NativeStartup_Account ? 1
        : s.step <= NativeStartup_Test ? 2 : 3;
    nativeMenuPaint_Text("FIRST STARTUP", 204, 51, 232, 11, muted, NativeMenuFont_MenuOption, true);
    for (unsigned i = 0; i < 4; ++i)
        nativeMenuPaint_Round(295 + i * 15, 74, 5, 5, 2.5f, i == group ? gold : 0x655E7AFF);
    if (s.step != NativeStartup_Welcome && s.step != NativeStartup_Done)
        nativeHub_Button("setup-back", "< Back", 124, 402, 90, 28, HubAction::StartupBack);
    if (s.step == NativeStartup_Account) {
        line("Optional: keep friends, letters and invitations with an online account.", 98, 8, muted);
        const auto& account = nativeHubAccount_View();
        nativeHub_Button("setup-next", account.signed_in ? "Continue" : "Continue as guest",
            314, 402, 200, 28, HubAction::StartupNext, 0, true, !account.busy && !account.recovery_pending);
    }
    if (s.step == NativeStartup_Clone && nativeHubStartup_HasFighter())
        nativeHub_Button("setup-next", "Use my roster", 314, 402, 200, 28, HubAction::StartupNext);
    const char* status = s.error[0] ? s.error :
        SDL_GetTicks() < hub.notice_until ? hub.notice : "A Choose   B Back";
    line(status, 449, 9, s.error[0] ? 0xFFAE92FF : muted);
}
void nativeHubStartup_Render()
{
    const auto& s = nativeHubStartup;
    switch (s.step) {
    case NativeStartup_Welcome:
        line("Welcome to Freak Melee", 138, 22, gold);
        line("Set up your profile and controller.", 185);
        line("Then pick a fighter to start your own roster.", 208);
        line("Your fighter is yours to edit in the Workshop.", 244);
        line("Use the hand to choose buttons. Type with your keyboard.", 285, 9, muted);
        next("Let's get started");
        break;
    case NativeStartup_Profile:
        line("What should we call you?", 134, 18, gold);
        line("Display name", 178, 10, muted, false);
        nativeHub_Field("setup-name", hub.name, 124, 197, 390, 29, HubAction::FieldName);
        line("Bio (optional)", 244, 10, muted, false);
        nativeHub_Field("setup-bio", hub.bio, 124, 262, 390, 50, HubAction::FieldBio);
        next("Continue", hub.name[0] && std::strspn(hub.name, " \t\r\n") != std::strlen(hub.name));
        break;
    case NativeStartup_Controller: {
        line("Choose your controller", 134, 18, gold);
        line("Press A on your controller, or choose its port.", 171, 10);
        for (unsigned port = 0; port < 4; ++port) {
            char id[32], label[80];
            const bool connected = PADGetIndexForPort(port) >= 0;
            std::snprintf(id, sizeof(id), "setup-port-%u", port);
            std::snprintf(label, sizeof(label), "%u  %s", port + 1, connected ? PADGetName(port) : "No controller");
            nativeHub_Button(id, label, 124, 199 + port * 32, 390, 26, HubAction::StartupPort, port, port == s.port);
        }
        nativeHub_Button("setup-skip-controller", "Set up later", 124, 369, 165, 28, HubAction::StartupSkipController);
        next("Calibrate controller", PADGetIndexForPort(s.port) >= 0);
        break;
    }
    case NativeStartup_Rest:
        line("Let go of the sticks and triggers", 134, 17, gold);
        line("Keep the controller still for a moment.", 171, 10);
        stick(167, 204, 0); stick(372, 204, 2);
        line(s.capture.samples >= 60 ? "Center captured. Continue when ready." : "Measuring the resting position...", 346, 10);
        next("Continue", PADGetIndexForPort(s.port) >= 0 && s.capture.samples >= 60 && SDL_GetTicks() - s.started_at >= 1000);
        break;
    case NativeStartup_Sticks: {
        line("Move both sticks around their edges", 134, 17, gold);
        line("Visit up, down, left and right on each stick.", 171, 10);
        stick(167, 202, 0); stick(372, 202, 2);
        bool complete = true;
        for (unsigned axis = 0; axis < 4; ++axis) {
            int center = (int(s.capture.rest_low[axis]) + s.capture.rest_high[axis]) / 2;
            complete &= s.capture.high[axis] - center >= 8192 && center - s.capture.low[axis] >= 8192;
        }
        next("Continue", complete);
        break;
    }
    case NativeStartup_Triggers:
        line("Press L and R all the way in", 134, 18, gold);
        line("Release each trigger after pressing it.", 177, 10);
        triggers(237);
        line("Both trigger meters should fill as you press.", 300, 10, muted);
        nativeHub_Button("setup-retry", "Start over", 124, 369, 165, 28, HubAction::StartupRetry);
        next("Try calibration", s.capture.high[4] - s.capture.rest_high[4] >= 8192 && s.capture.high[5] - s.capture.rest_high[5] >= 8192);
        break;
    case NativeStartup_Test:
        line("Try your controller", 132, 18, gold);
        line("Small movements should be precise. Rest should be still.", 166, 10);
        stick(167, 192, 0); stick(372, 192, 2);
        triggers(323);
        nativeHub_Button("setup-retry", "Calibrate again", 124, 369, 165, 28, HubAction::StartupRetry);
        next("Save & continue");
        break;
    case NativeStartup_Done:
        line("Your roster is ready", 136, 22, gold);
        line("Choose your fighter on character select.", 190);
        line("Place your token and press START to practice.", 217);
        line("Use the Workshop to change moves, animations and models.", 266, 10, muted);
        next("Character select");
        break;
    default: break;
    }
}
