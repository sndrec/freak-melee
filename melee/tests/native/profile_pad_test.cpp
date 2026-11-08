#include <aurora/aurora.h>
#include <dolphin/pad.h>
#include <SDL3/SDL.h>
#include "input.hpp"
#include <melee/gm/native_profile_input.h>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

namespace aurora { extern AuroraConfig g_config; }

static void require(bool condition, const char* message)
{
    if (!condition) {
        std::fprintf(stderr, "%s: %s\n", message, SDL_GetError());
        std::exit(1);
    }
}

struct Device {
    SDL_JoystickID id;
    SDL_Joystick* joystick;
};

static Device attach(const char* name, unsigned port)
{
    SDL_VirtualJoystickDesc description;
    SDL_INIT_INTERFACE(&description);
    description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    description.naxes = SDL_GAMEPAD_AXIS_COUNT;
    description.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    description.axis_mask = (1u << SDL_GAMEPAD_AXIS_COUNT) - 1;
    description.button_mask = (1u << SDL_GAMEPAD_BUTTON_COUNT) - 1;
    description.name = name;
    Device device {SDL_AttachVirtualJoystick(&description), nullptr};
    require(device.id != 0, "attach virtual SDL controller");
    require(aurora::input::add_controller(device.id) == device.id,
            "open only the test controller through Aurora");
    auto& controller = aurora::input::g_GameControllers.at(device.id);
    require(SDL_SetGamepadPlayerIndex(controller.m_controller, port), "assign test port");
    device.joystick = SDL_GetGamepadJoystick(controller.m_controller);
    for (unsigned axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; ++axis)
        require(SDL_SetJoystickVirtualAxis(device.joystick, axis,
                    axis >= SDL_GAMEPAD_AXIS_LEFT_TRIGGER ? -32768 : 0), "release test axes");
    SDL_UpdateGamepads();
    return device;
}

static void detach(Device device)
{
    aurora::input::remove_controller(device.id);
    require(SDL_DetachVirtualJoystick(device.id), "detach virtual SDL controller");
}

static PADStatus read(const Device& device, unsigned port, int x, bool mapped = false)
{
    require(SDL_SetJoystickVirtualAxis(device.joystick, SDL_GAMEPAD_AXIS_LEFTX,
                                      static_cast<Sint16>(x)), "sample main stick");
    SDL_UpdateGamepads();
    PADStatus pads[4];
    PADRead(pads);
    if (mapped) nativeProfileInput_Apply(pads);
    return pads[port];
}

int main(int argc, char** argv)
{
    require(argc == 2, "provide an isolated input-fixture directory");
    std::filesystem::create_directories(argv[1]);
    aurora::g_config.userPath = argv[1];
    // No window, audio, renderer or physical controller is opened. Native HID
    // is disabled so this test cannot claim the user's GameCube adapter.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    require(SDL_Init(SDL_INIT_GAMEPAD), "initialize SDL gamepad subsystem");
    require(PADInit(), "initialize actual Aurora PAD provider");
    NativeProfile profile;
    nativeProfile_Default(&profile, "Hotplug fixture");
    profile.calibration.main_stick_dead_zone = 1200;
    profile.calibration.c_stick_dead_zone = 2000;
    profile.calibration.left_trigger_activation = 10000;
    profile.calibration.right_trigger_activation = 20000;
    require(nativeProfileInput_Assign(0, &profile), "assign profile before controller connects");
    Device first = attach("Profile test A", 0);
    require(read(first, 0, 5000).stickX == 5000 / 256,
            "new controller uses the port's previously assigned calibration");
    require(PADGetDeadZones(0)->stickDeadZone == 8000,
            "profile calibration preserves the device's own settings");
    require(SDL_SetJoystickVirtualAxis(first.joystick, SDL_GAMEPAD_AXIS_RIGHTX, 3000) &&
                SDL_SetJoystickVirtualAxis(first.joystick, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0) &&
                SDL_SetJoystickVirtualAxis(first.joystick, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0),
            "sample the C-stick and partially pressed triggers");
    const auto calibrated = read(first, 0, 5000);
    require(calibrated.substickX == 3000 / 256 &&
                (calibrated.button & PAD_TRIGGER_L) && !(calibrated.button & PAD_TRIGGER_R),
            "the port's C-stick and separate trigger thresholds reach PADRead");
    require(PADGetMappedButtonState(0, PAD_TRIGGER_L) == -1,
            "an emulated PAD trigger is not a physical click");
    PADSetButtonMapping(0, {SDL_GAMEPAD_BUTTON_MISC3, PAD_TRIGGER_L});
    require(PADGetMappedButtonState(0, PAD_TRIGGER_L) == 0,
            "an independent mapped trigger click reports released");
    require(SDL_SetJoystickVirtualButton(first.joystick, SDL_GAMEPAD_BUTTON_MISC3, true), "press independent trigger click");
    SDL_UpdateGamepads();
    require(PADGetMappedButtonState(0, PAD_TRIGGER_L) == 1,
            "physical trigger click is available before analog processing");
    require(SDL_SetJoystickVirtualButton(first.joystick, SDL_GAMEPAD_BUTTON_MISC3, false), "release independent trigger click");
    PADSetButtonMapping(0, {PAD_NATIVE_BUTTON_INVALID, PAD_TRIGGER_L});

    profile.calibration.main_stick_dead_zone = 6000;
    require(nativeProfileInput_Assign(0, &profile), "replace the assigned calibration");
    require(read(first, 0, 5000).stickX == 0, "replacement calibration takes effect");
    nativeProfileInput_Clear(0);
    require(read(first, 0, 5000).stickX == 0 && read(first, 0, 12000).stickX == 12000 / 256,
            "clearing the profile restores the current device's calibration");

    profile.calibration.main_stick_dead_zone = 1200;
    require(nativeProfileInput_Assign(0, &profile), "reassign port calibration");
    require(SDL_SetGamepadPlayerIndex(aurora::input::g_GameControllers.at(first.id).m_controller, 2),
            "move the first device to another physical port");
    Device second = attach("Profile test B", 0);
    require(read(second, 0, 5000).stickX == 5000 / 256 && read(first, 2, 5000).stickX == 0,
            "calibration follows the assigned port rather than the old device");
    detach(second);
    Device replacement = attach("Profile test C", 0);
    require(read(replacement, 0, 5000).stickX == 5000 / 256,
            "replacement controller inherits the port calibration");
    PADGetDeadZones(0)->stickDeadZone = 10000;
    nativeProfileInput_Clear(0);
    require(read(replacement, 0, 9000).stickX == 0 && read(replacement, 0, 12000).stickX == 12000 / 256 &&
                PADGetDeadZones(0)->stickDeadZone == 10000,
            "clearing retains settings belonging to the replacement device");

    profile.actions[NativeProfileAction_Shield] = {};
    profile.actions[NativeProfileAction_Shield].count = 1;
    profile.actions[NativeProfileAction_Shield].bindings[0] = {
        .native_input = SDL_GAMEPAD_BUTTON_BACK, .type = NativeProfileBinding_Button
    };
    profile.axes[NativeProfileAxis_LeftTrigger] = {
        .native_axis = SDL_GAMEPAD_AXIS_LEFT_TRIGGER, .direction = 1, .assigned = true
    };
    require(nativeProfileInput_Assign(0, &profile), "assign raw SDL shield binding");
    require(SDL_SetJoystickVirtualButton(replacement.joystick, SDL_GAMEPAD_BUTTON_BACK, true),
            "press raw mapped Shield button");
    auto shield = read(replacement, 0, 0, true);
    require(shield.triggerLeft == 255 && (shield.button & PAD_TRIGGER_L),
            "digital Shield survives the actual SDL analog-trigger path");
    require(SDL_SetJoystickVirtualButton(replacement.joystick, SDL_GAMEPAD_BUTTON_BACK, false),
            "release raw mapped Shield button");
    shield = read(replacement, 0, 0, true);
    require(shield.triggerLeft == 0 && !(shield.button & PAD_TRIGGER_L),
            "releasing digital Shield restores the raw released trigger");
    require(SDL_SetJoystickVirtualAxis(replacement.joystick, SDL_GAMEPAD_AXIS_LEFTY, 12000), "sample mapped vertical axis");
    SDL_UpdateGamepads();
    require(PADGetMappedAxisState(0, 1) == -12000, "logical calibration Y points up before filtering");
    const s32 raw_bindings[6]={-1,-1,-1,-1,SDL_GAMEPAD_AXIS_LEFTY,-1};
    const s8 raw_directions[6]={1,1,1,1,-1,1}; s16 raw_snapshot[6];
    PADReadRawAxes(0,raw_bindings,raw_directions,raw_snapshot);
    require(raw_snapshot[1]==-12000 && raw_snapshot[4]==-12000,
            "coherent raw snapshot respects logical Y and explicitly inverted native bindings");
    profile.calibration.measured = true;
    for (unsigned axis = 0; axis < 6; ++axis) {
        profile.calibration.center[axis] = 0;
        profile.calibration.minimum[axis] = axis < 4 ? -24000 : 0;
        profile.calibration.maximum[axis] = 24000;
    }
    require(nativeProfileInput_Assign(0, &profile), "apply measured profile through actual SDL provider");
    auto measured = read(replacement, 0, 12000, true);
    require(measured.stickX == 39 && measured.stickY == -39, "raw mapped axes share the calibrated 80-unit scale");
    measured = read(replacement, 0, 24000, true);
    require(measured.stickX == 80, "full measured travel reaches full Melee input");
    nativeProfileInput_ClearAll();
    detach(first);
    detach(replacement);
    require(aurora::input::g_GameControllers.empty(), "all task controllers closed");
    SDL_Quit();
    std::puts("Actual SDL profile calibration, replacement and mixed shielding passed");
    return 0;
}
