# Build and launch the Windows native client

Updated: 2026-11-15. The full gameplay/HSD tree builds as the runnable
`melee_native` client; the initial compile/link bring-up is finished.
This guide describes the current source layout. Old compile-audit history is
preserved in the private repository's pre-cleanup commit, not a work list.

## Requirements

- Windows x86-64.
- LLVM Clang targeting MSVC, with Visual Studio C++ libraries and a Windows SDK.
  MinGW Clang is not the supported toolchain.
- CMake 3.25 or newer, Ninja, 7-Zip and Python 3.
- The release payload tests use the Python `cryptography` package. On Windows
  install `pywin32` as well to test the publisher key backup and restore.
- This repository's modified Aurora at sibling `aurora/`; CMake obtains the
  configured third-party dependencies. Reference-only folders are unnecessary.
- A local USA v1.02 Melee ISO and a local 4,096-byte DSP coefficient table.

The development cache uses Clang 22.1.6, MSVC 14.44 and RelWithDebInfo. A consistent
MSVC environment matters: libraries built with a different STL/toolset can fail
to link. Use explicit compiler paths if another Clang is earlier on PATH.

SDL is built from the pinned source so GameCube input retains a fixed coordinate
scale. The Windows build extracts the official libusb headers, import library
and runtime DLL with 7-Zip. The normal DLL-copy and release steps include libusb.

Connect the Nintendo Switch 2 GameCube controller directly with a USB-C data
cable. SDL reads both sticks, analog trigger travel and the separate L/R clicks.
On Windows, libusb needs WinUSB on the controller's vendor interface
(`057E:2073`, interface 1). SDL 3.4.10 supports this controller over USB.

The ISO and DSP table are private local inputs, not included in Git or releases.
`prepare_native_assets.py` extracts/validates the ISO executable and embedded
font data into the build tree. `prepare_audio_coefficients.py` consumes the DSP
table; it does not extract that table from the ISO. Source backup is therefore
not a claim that the only external input is an ISO.

## Configure and build

Open a Visual Studio developer Command Prompt at the repository root. On the
current PC select the established library environment explicitly:

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44
cmake -S melee -B melee/build/native -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DMELEE_ISO="C:/local/Melee-USA-v1.02.iso" -DMELEE_DSP_COEFFICIENTS="C:/local/dsp_coef.bin"
cmake --build melee/build/native --target melee_native native_content_tool -j 4
```

Supply real local input paths. `MELEE_AURORA_SOURCE` overrides the sibling
framework path only when deliberately building another checkout. Do not point
a fresh clone at untracked reference repositories to hide a missing dependency.

Build both executables: Workshop source checks/builds invoke
`native_content_tool.exe` beside the client. Ordinary fighter scripts validate
and lower to bytecode locally; they do not compile to C or behavior DLLs.
Release/RelWithDebInfo use native ThinLTO while retaining strict floating-point
settings; Debug is a separate debugging configuration.

The native game, script runtime and Aurora matrix library use the pinned
transcendental implementations in [CORE-MATH](../extern/core-math/README.md)
and the game's inverse trig routines. The simulation thread resets to
nearest-even rounding with subnormal support before each tick. To verify the
compiled math and check for accidental CRT imports:

```bat
cmake --build melee/build/native --target native_math_test
ctest --test-dir melee/build/native -R "^native_math$" --output-on-failure
python melee/tools/audit_native_math.py melee/build/native
```

For an already configured development tree:

```bat
cmake --build melee/build/native --target melee_native native_content_tool -j 4
```

Do not reuse a CMake cache from another absolute checkout, operating system or
toolchain. Configure a separate build directory rather than editing the cache.

## Launch

```bat
melee\build\native\melee_native.exe --iso "C:/local/Melee-USA-v1.02.iso"
```

Normal boot reaches home character select. START enters grid practice. Online
lobbies begin a three-second countdown when all active fighters are ready,
then proceed to stage selection with locked-in choices. The local
top-level batch launcher uses this workspace's build and sibling ISO.
See [home flow](native_home_flow.md) and [diagnostic match options](native_match_launch.md).

User data under `%LOCALAPPDATA%\Freak Melee` is separate from replaceable builds:
profiles, installed collection markers, cached packages and the CSS layout.
For tests use isolated stores, hidden/muted launches and the native input channel.

With an editor draft open, the native control tool can import a fighter package
as a separate draft:

```bat
melee\build\native\melee_native_control.exe --id review workshop-import "C:/local/fighter.fmc"
```

Use the control ID supplied when launching that client. Import saves the open
draft, opens the imported fighter for editing, and leaves installation to
**Build & Install**. The editor's **Fighter** menu offers the same import command;
**Import model** opens the model importer for the current draft.

The Fighter menu also imports transparent PNGs for **Stock icon** and **HUD
emblem**. These belong to the selected family member. Original stock icons are
24×24 pixels. White emblem artwork takes the HUD's player color. Use Build &
Install after importing. The control equivalents are `workshop-image stock
"C:/local/icon.png"` and `workshop-image emblem "C:/local/emblem.png"`.

For rendering and bone-physics diagnostics, `ui-inspect shadow-state` reports
fighter shadows, receiving stage materials and bone collision spheres.
`ui-inspect dynamics-state` reports active wind fields and their remaining
lifetime. These commands inspect the running scene without changing it.

## Verification

Build the selected test targets before running their CTests:

```bat
ctest --test-dir melee/build/native -R "^native_(script_bytecode|script_fighter|script_execution|confirmed_checkpoint|rollback_input)$" --output-on-failure
```

This is a focused example, not a full all-target acceptance command. Some old
generated-C reference and standalone fixtures have recorded stale-dependency
failures; do not label an unbuilt/skipped fixture passing. Source and game tests
live under `tests/native/`; dated receipts are linked from the
[evidence index](checkpoint_index.md).

No gameplay test is required for documentation-only changes. Build success does
not establish character fidelity, UI usability, network routes or friend readiness.
The tracked-source-only export and clean extracted-install checks passed locally
on 2026-10-17. See the release guide for their inputs and evidence boundaries.

## Source and release boundaries

The private root repository contains `melee/` and the exact modified `aurora/`
source, native tools, tests, templates, UI and service source. It excludes ISOs,
extracted DAT/DOL data, DSP ROM, binaries, build caches, screenshots and profiling
archives. Existing nested histories remain local; do not push donor repositories.

See [release preparation](native_release.md). Preparation includes both native
executables and the DLL/UI lists generated by CMake. The extracted-release
check covers Workshop validation, building and installation with bundled tools.
