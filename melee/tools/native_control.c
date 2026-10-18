#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <melee/db/dbnativecontrol_protocol.h>
#include <melee/gm/native_signaling_wire.h>
#include <melee/gm/native_profile_input.h>
#include <dolphin/pad.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char* executable)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s [--id NAME] screenshot\n"
            "  %s [--id NAME] clear [port]\n"
            "  %s [--id NAME] tap <port> <input>... [frames=N]\n"
            "  %s [--id NAME] hold <port> <input>...\n"
            "  %s [--id NAME] ui-inspect <element-id>\n"
            "  %s [--id NAME] ui-click <x> <y> [right|middle]\n"
            "  %s [--id NAME] ui-drag <x0> <y0> <x1> <y1>\n"
            "  %s [--id NAME] ui-text <text>\n"
            "  %s [--id NAME] ui-move <x> <y>\n"
            "  %s [--id NAME] ui-key <letter|left|right|up|down|home|end|pageup|pagedown|tab|space|escape|enter|backspace|delete> "
            "[shift|ctrl|alt]...\n"
            "  %s [--id NAME] room-rotation <manual|winner-stays|winner-out>\n"
            "  %s [--id NAME] room-stage <local-seat> <0..5|none>\n"
            "Inputs: a b x y z l r start up down left right\n"
            "        short-hop no-tap-jump tilt-stick\n"
            "        sx=N sy=N cx=N cy=N lt=N rt=N aa=N ab=N\n",
            executable, executable, executable, executable, executable,
            executable, executable, executable, executable, executable, executable, executable);
}

static bool parse_long(const char* text, long minimum, long maximum,
                       long* result)
{
    char* end;
    errno = 0;
    long value = strtol(text, &end, 10);
    if (errno != 0 || *text == 0 || *end != 0 || value < minimum ||
        value > maximum)
    {
        return false;
    }
    *result = value;
    return true;
}

static bool parse_assignment(const char* argument, const char* name,
                             long minimum, long maximum, long* result)
{
    size_t length = strlen(name);
    return _strnicmp(argument, name, length) == 0 && argument[length] == '=' &&
           parse_long(argument + length + 1, minimum, maximum, result);
}

static bool add_input(DbNativeControlMessage* message, const char* input)
{
    if (_stricmp(input, "a") == 0) {
        message->buttons |= PAD_BUTTON_A;
    } else if (_stricmp(input, "b") == 0) {
        message->buttons |= PAD_BUTTON_B;
    } else if (_stricmp(input, "x") == 0) {
        message->buttons |= PAD_BUTTON_X;
    } else if (_stricmp(input, "y") == 0) {
        message->buttons |= PAD_BUTTON_Y;
    } else if (_stricmp(input, "z") == 0) {
        message->buttons |= PAD_TRIGGER_Z;
    } else if (_stricmp(input, "l") == 0) {
        message->buttons |= PAD_TRIGGER_L;
        message->trigger_left = 255;
    } else if (_stricmp(input, "r") == 0) {
        message->buttons |= PAD_TRIGGER_R;
        message->trigger_right = 255;
    } else if (_stricmp(input, "start") == 0) {
        message->buttons |= PAD_BUTTON_START;
    } else if (_stricmp(input, "up") == 0) {
        message->buttons |= PAD_BUTTON_UP;
    } else if (_stricmp(input, "down") == 0) {
        message->buttons |= PAD_BUTTON_DOWN;
    } else if (_stricmp(input, "left") == 0) {
        message->buttons |= PAD_BUTTON_LEFT;
    } else if (_stricmp(input, "right") == 0) {
        message->buttons |= PAD_BUTTON_RIGHT;
    } else if (_stricmp(input, "short-hop") == 0) {
        message->extensions |= NATIVE_PROFILE_EXT_SHORT_HOP;
    } else if (_stricmp(input, "no-tap-jump") == 0) {
        message->extensions |= NATIVE_PROFILE_EXT_TAP_JUMP_DISABLED;
    } else if (_stricmp(input, "tilt-stick") == 0) {
        message->extensions |= NATIVE_PROFILE_EXT_CSTICK_TILT;
    } else {
        long value;
        if (parse_assignment(input, "sx", -128, 127, &value)) {
            message->stick_x = value;
        } else if (parse_assignment(input, "sy", -128, 127, &value)) {
            message->stick_y = value;
        } else if (parse_assignment(input, "cx", -128, 127, &value)) {
            message->cstick_x = value;
        } else if (parse_assignment(input, "cy", -128, 127, &value)) {
            message->cstick_y = value;
        } else if (parse_assignment(input, "lt", 0, 255, &value)) {
            message->trigger_left = value;
        } else if (parse_assignment(input, "rt", 0, 255, &value)) {
            message->trigger_right = value;
        } else if (parse_assignment(input, "aa", 0, 255, &value)) {
            message->analog_a = value;
        } else if (parse_assignment(input, "ab", 0, 255, &value)) {
            message->analog_b = value;
        } else if (parse_assignment(input, "frames", 1, 600, &value)) {
            message->frames = value;
        } else {
            return false;
        }
    }
    return true;
}

static bool send_message(const DbNativeControlMessage* message,
                         const char* control_id)
{
    char channel_name[DB_NATIVE_CONTROL_CHANNEL_BYTES];
    if (!dbNativeControl_FormatChannel(channel_name, control_id)) {
        fprintf(stderr, "Invalid native control ID.\n");
        return false;
    }
    HANDLE channel =
        CreateFileA(channel_name, GENERIC_WRITE, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (channel == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Freak Melee control channel is not available (%lu).\n",
                GetLastError());
        return false;
    }
    DWORD bytes_written = 0;
    bool ok = WriteFile(channel, message, sizeof(*message), &bytes_written, NULL) &&
              bytes_written == sizeof(*message);
    CloseHandle(channel);
    if (!ok) {
        fprintf(stderr, "Could not send the native control command.\n");
    }
    return ok;
}

int wmain(int argc, wchar_t** arguments)
{
    // Windows supplies UTF-16 arguments. Convert once so ui-text sends the
    // same UTF-8 bytes as SDL text input, independently of the system code page.
    char bytes[32768 * 3];
    char* argv[256];
    size_t used = 0;
    if (argc > 256) return 1;
    for (int i = 0; i < argc; ++i) {
        argv[i] = bytes + used;
        int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            arguments[i], -1, argv[i], (int)(sizeof(bytes) - used), NULL, NULL);
        if (!size) {
            fprintf(stderr, "Could not encode command arguments as UTF-8.\n");
            return 1;
        }
        used += size;
    }
    int command_index = 1;
    const char* control_id = NULL;
    if (argc >= 3 && _stricmp(argv[1], "--id") == 0) {
        control_id = argv[2];
        command_index = 3;
    }
    if (argc <= command_index) {
        usage(argv[0]);
        return 1;
    }
    DbNativeControlMessage message = { .magic = DB_NATIVE_CONTROL_MAGIC };
    int command_args = argc - command_index;
    char** command = &argv[command_index];
    if (_stricmp(command[0], "screenshot") == 0 && command_args == 1) {
        message.command = DB_NATIVE_CONTROL_SCREENSHOT;
    } else if ((_stricmp(command[0], "ui-click") == 0 ||
                _stricmp(command[0], "ui-move") == 0) &&
               (command_args == 3 || command_args == 4))
    {
        long x, y;
        if (!parse_long(command[1], 0, 32767, &x) ||
            !parse_long(command[2], 0, 32767, &y)) {
            usage(argv[0]);
            return 1;
        }
        message.command = _stricmp(command[0], "ui-move") == 0
                              ? DB_NATIVE_CONTROL_UI_MOVE
                              : DB_NATIVE_CONTROL_UI_CLICK;
        message.ui_x = x;
        message.ui_y = y;
        if(command_args==4) {
            if(_stricmp(command[0],"ui-click")!=0)return 1;
            if(_stricmp(command[3],"right")==0)message.buttons=1;
            else if(_stricmp(command[3],"middle")==0)message.buttons=2;
            else return 1;
        }
    } else if (_stricmp(command[0], "ui-inspect") == 0 && command_args == 2) {
        if (strlen(command[1]) >= sizeof(message.text)) return 1;
        message.command = DB_NATIVE_CONTROL_UI_INSPECT;
        strcpy(message.text, command[1]);
    } else if (_stricmp(command[0], "ui-key") == 0 && command_args >= 2 &&
               command_args <= 5)
    {
        if (strlen(command[1]) >= sizeof(message.text)) {
            return 1;
        }
        message.command = DB_NATIVE_CONTROL_UI_KEY;
        strcpy(message.text, command[1]);
        for (int i = 2; i < command_args; ++i) {
            if (_stricmp(command[i], "shift") == 0) {
                message.buttons |= 1;
            } else if (_stricmp(command[i], "ctrl") == 0) {
                message.buttons |= 2;
            } else if (_stricmp(command[i], "alt") == 0) {
                message.buttons |= 4;
            } else {
                return 1;
            }
        }
    } else if (_stricmp(command[0], "ui-drag") == 0 && command_args == 5) {
        long x0, y0, x1, y1;
        if (!parse_long(command[1], 0, 32767, &x0) ||
            !parse_long(command[2], 0, 32767, &y0) ||
            !parse_long(command[3], 0, 32767, &x1) ||
            !parse_long(command[4], 0, 32767, &y1)) {
            usage(argv[0]);
            return 1;
        }
        message.command = DB_NATIVE_CONTROL_UI_DRAG;
        message.ui_x = x0;
        message.ui_y = y0;
        message.ui_end_x = x1;
        message.ui_end_y = y1;
    } else if (_stricmp(command[0], "ui-text") == 0 && command_args == 2) {
        if (strlen(command[1]) >= sizeof(message.text)) {
            fprintf(stderr, "UI text is limited to 127 bytes.\n");
            return 1;
        }
        message.command = DB_NATIVE_CONTROL_UI_TEXT;
        memcpy(message.text, command[1], strlen(command[1]) + 1);
    } else if (_stricmp(command[0], "room-stage") == 0 && command_args == 3) {
        long seat, stage;
        if (!parse_long(command[1], 0, 1, &seat) ||
            (_stricmp(command[2], "none") && !parse_long(command[2], 0, 5, &stage))) {
            usage(argv[0]); return 1;
        }
        message.command = DB_NATIVE_CONTROL_ROOM_STAGE;
        message.port = (u32) seat;
        message.ui_x = !_stricmp(command[2], "none") ? 255 : (s32) stage;
    } else if (_stricmp(command[0], "room-rotation") == 0 && command_args == 2)
    {
        if (_stricmp(command[1], "manual") == 0) {
            message.rotation_policy = NativeRoomRotation_Manual;
        } else if (_stricmp(command[1], "winner-stays") == 0) {
            message.rotation_policy = NativeRoomRotation_WinnerStays;
        } else if (_stricmp(command[1], "winner-out") == 0) {
            message.rotation_policy = NativeRoomRotation_WinnerOut;
        } else {
            usage(argv[0]);
            return 1;
        }
        message.command = DB_NATIVE_CONTROL_ROOM_ROTATION;
    } else if (_stricmp(command[0], "clear") == 0 && command_args <= 2) {
        long port = 0;
        if (command_args == 2 &&
            !parse_long(command[1], 0, PAD_MAX_CONTROLLERS - 1, &port))
        {
            usage(argv[0]);
            return 1;
        }
        message.command = DB_NATIVE_CONTROL_CLEAR_PAD;
        message.port = port;
    } else if ((_stricmp(command[0], "tap") == 0 ||
                _stricmp(command[0], "hold") == 0) &&
               command_args >= 3)
    {
        long port;
        if (!parse_long(command[1], 0, PAD_MAX_CONTROLLERS - 1, &port)) {
            usage(argv[0]);
            return 1;
        }
        message.command = DB_NATIVE_CONTROL_SET_PAD;
        message.port = port;
        message.frames = _stricmp(command[0], "tap") == 0 ? 2 : 0;
        for (int i = 2; i < command_args; ++i) {
            if (!add_input(&message, command[i])) {
                fprintf(stderr, "Unknown input: %s\n", command[i]);
                usage(argv[0]);
                return 1;
            }
        }
    } else {
        usage(argv[0]);
        return 1;
    }
    return send_message(&message, control_id) ? 0 : 1;
}
