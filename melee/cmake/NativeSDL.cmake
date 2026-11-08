# Freak Melee measures controller centers and ranges itself. Build SDL from
# source so the GameCube driver's reports keep the fixed device coordinate scale.
set(AURORA_SDL3_PROVIDER vendor CACHE STRING "Build SDL with stable GameCube reports" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build the SDL test support library" FORCE)
set(SDL_TESTS OFF CACHE BOOL "Build SDL tests" FORCE)

if(WIN32)
    # The official adapter uses WinUSB. Import the native libusb DLL so runtime
    # dependency discovery includes it when building and packaging the client.
    set(_usb_sdk "${CMAKE_BINARY_DIR}/_deps/native_libusb")
    file(DOWNLOAD
        "https://github.com/libusb/libusb/releases/download/v1.0.29/libusb-1.0.29.7z"
        "${_usb_sdk}/libusb-1.0.29.7z"
        EXPECTED_HASH SHA256=964a38152ca9a104cd00ec8d2f0617b89cd814f9b635e29763c68563d951521d
        TLS_VERIFY ON)
    # libusb's solid archive uses compression filters unsupported by libarchive.
    find_program(NATIVE_7ZIP NAMES 7z 7zz 7za HINTS "$ENV{ProgramFiles}/7-Zip" REQUIRED)
    execute_process(COMMAND "${NATIVE_7ZIP}" x -y "-o${_usb_sdk}"
        "${_usb_sdk}/libusb-1.0.29.7z"
        "include/libusb.h" "VS2022/MS64/dll/libusb-1.0.lib" "VS2022/MS64/dll/libusb-1.0.dll"
        OUTPUT_QUIET COMMAND_ERROR_IS_FATAL ANY)
    set(LibUSB_INCLUDE_PATH "${_usb_sdk}/include" CACHE PATH "libusb headers" FORCE)
    set(LibUSB_LIBRARY "${_usb_sdk}/VS2022/MS64/dll/libusb-1.0.lib" CACHE FILEPATH "libusb import library" FORCE)
    add_library(LibUSB::LibUSB SHARED IMPORTED GLOBAL)
    set_target_properties(LibUSB::LibUSB PROPERTIES
        IMPORTED_LOCATION "${_usb_sdk}/VS2022/MS64/dll/libusb-1.0.dll"
        IMPORTED_IMPLIB "${LibUSB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LibUSB_INCLUDE_PATH}")
    set(SDL_HIDAPI_LIBUSB ON CACHE BOOL "Enable GameCube adapter USB reports" FORCE)
    set(SDL_HIDAPI_LIBUSB_SHARED OFF CACHE BOOL "Link libusb normally instead of loading it by name" FORCE)
endif()
