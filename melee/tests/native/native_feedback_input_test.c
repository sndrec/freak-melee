#include <melee/gm/native_feedback_input.h>
#include <melee/gm/native_profile_input.h>
#include <stdio.h>
#include <string.h>

static unsigned checks;
#define CHECK(condition) do { ++checks; if (!(condition)) { fprintf(stderr,"line %u: %s\n",__LINE__,#condition); return 1; } } while (0)
int main(void)
{
    PADStatus pads[4] = {0}, original[4];
    const u32 preferences = NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED | NATIVE_PROFILE_EXT_CSTICK_TILT |
        (3u << NATIVE_PROFILE_EXT_ACTION_BUFFER_SHIFT);
    for (unsigned i = 0; i < 4; ++i) {
        pads[i].button = PAD_BUTTON_A; pads[i].stickX = -70; pads[i].substickY = 80;
        pads[i].triggerLeft = 122; pads[i].analogA = 255;
        pads[i].extButton = preferences | NATIVE_PROFILE_EXT_SHORT_HOP;
        pads[i].err = i == 3 ? PAD_ERR_NO_CONTROLLER : PAD_ERR_NONE;
    }
    memcpy(original, pads, sizeof(pads));
    nativeFeedbackInput_SetOpen(true);
    nativeFeedbackInput_SetResimulating(true);
    nativeFeedbackInput_Filter(pads);
    CHECK(!memcmp(pads, original, sizeof(pads)));
    nativeFeedbackInput_SetResimulating(false);
    nativeFeedbackInput_Filter(pads);
    for (unsigned i = 0; i < 4; ++i) {
        CHECK(pads[i].button == 0); CHECK(pads[i].extButton == preferences);
        CHECK(pads[i].stickX == 0 && pads[i].substickY == 0);
        CHECK(pads[i].triggerLeft == 0 && pads[i].analogA == 0);
        CHECK(pads[i].err == original[i].err);
    }
    nativeFeedbackInput_SetOpen(false);
    memcpy(pads, original, sizeof(pads)); nativeFeedbackInput_Filter(pads);
    CHECK(pads[0].button == 0 && pads[0].triggerLeft == 0);
    // A held dismissing button remains consumed, while a fresh button and
    // current analog movement reach the very next physical input sample.
    memcpy(pads, original, sizeof(pads)); pads[0].button |= PAD_BUTTON_B;
    nativeFeedbackInput_Filter(pads);
    CHECK(pads[0].button == PAD_BUTTON_B); CHECK(pads[0].stickX == -70);
    CHECK(!(pads[0].extButton & NATIVE_PROFILE_EXT_SHORT_HOP));
    CHECK(pads[0].triggerLeft == 0);
    CHECK(pads[0].analogA == 0);
    memset(pads, 0, sizeof(pads)); nativeFeedbackInput_Filter(pads);
    memcpy(pads, original, sizeof(pads)); nativeFeedbackInput_Filter(pads);
    CHECK(pads[0].button == PAD_BUTTON_A); CHECK(pads[0].triggerLeft == 122);
    CHECK(pads[0].analogA == 255);
    CHECK(pads[0].extButton == (preferences | NATIVE_PROFILE_EXT_SHORT_HOP));
    printf("{\"status\":\"PASS\",\"checks\":%u}\n", checks); return 0;
}
