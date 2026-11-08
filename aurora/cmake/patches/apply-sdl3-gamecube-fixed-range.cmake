if(NOT DEFINED SDL_SOURCE_DIR)
    message(FATAL_ERROR "SDL_SOURCE_DIR is required")
endif()
set(_file "${SDL_SOURCE_DIR}/src/joystick/hidapi/SDL_hidapi_gamecube.c")
file(READ "${_file}" _source)
set(_marker "Aurora: keep device coordinates stable across calibration sweeps.")
string(FIND "${_source}" "${_marker}" _already)
if(NOT _already EQUAL -1)
    return()
endif()

# Remove adaptive range state, its initializer and its connect/reset call sites.
string(REGEX REPLACE "    Uint8 (min_axis|max_axis)\\[[^\n]*\n" "" _source "${_source}")
string(REGEX REPLACE "static void ResetAxisRange\\([^)]*\\)\n\\{[^}]*\\}\n" "" _source "${_source}")
string(REGEX REPLACE "[ \t]*ResetAxisRange\\(ctx, [^;]*;\n" "" _source "${_source}")

set(_pc [=[/* Aurora: keep device coordinates stable across calibration sweeps.
 * Applications measure center/range; reaching a new extreme must not move
 * previously sampled positions. The adapter reports six unsigned byte axes. */
#define READ_AXIS(off, axis, invert) \
    v = (invert) ? (0xff - packet[off]) : packet[off]; \
    axis_value = (Sint16)((int)v * 257 - 32768); \
    SDL_SendJoystickAxis(timestamp, joystick, axis, axis_value);
    READ_AXIS(2, SDL_GAMEPAD_AXIS_LEFTX, 0)
    READ_AXIS(3, SDL_GAMEPAD_AXIS_LEFTY, 1)
    READ_AXIS(5, SDL_GAMEPAD_AXIS_RIGHTX, invert_c_stick ? 1 : 0)
    READ_AXIS(4, SDL_GAMEPAD_AXIS_RIGHTY, invert_c_stick ? 0 : 1)
    READ_AXIS(6, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0)
    READ_AXIS(7, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0)
#undef READ_AXIS]=])
set(_nintendo [=[#define READ_AXIS(off, axis) \
    axis_value = (Sint16)((int)curSlot[off] * 257 - 32768); \
    SDL_SendJoystickAxis(timestamp, joystick, axis, axis_value);
        READ_AXIS(3, SDL_GAMEPAD_AXIS_LEFTX)
        READ_AXIS(4, SDL_GAMEPAD_AXIS_LEFTY)
        READ_AXIS(5, SDL_GAMEPAD_AXIS_RIGHTX)
        READ_AXIS(6, SDL_GAMEPAD_AXIS_RIGHTY)
        READ_AXIS(7, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)
        READ_AXIS(8, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)
#undef READ_AXIS]=])
string(REGEX MATCH "#define READ_AXIS\\(off, axis, invert\\)[^#]*#undef READ_AXIS" _old_pc "${_source}")
string(REGEX MATCH "#define READ_AXIS\\(off, axis\\)[^#]*#undef READ_AXIS" _old_nintendo "${_source}")
if(NOT _old_pc OR NOT _old_nintendo)
    message(FATAL_ERROR "SDL GameCube report parser changed; review the fixed-range patch")
endif()
string(REPLACE "${_old_pc}" "${_pc}" _source "${_source}")
string(REPLACE "${_old_nintendo}" "${_nintendo}" _source "${_source}")
if(_source MATCHES "min_axis|max_axis|ResetAxisRange")
    message(FATAL_ERROR "SDL GameCube adaptive range state remains after patching")
endif()
file(WRITE "${_file}" "${_source}")
message(STATUS "Patched SDL GameCube reports to use the fixed device range")
