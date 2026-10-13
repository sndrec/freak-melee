# Build and launch the Windows native client

Updated: 2026-10-13. The full gameplay/HSD tree builds as the runnable
`melee_native` client; the initial compile/link bring-up is finished.
This guide describes the current source layout. Old compile-audit history is
preserved in the private repository's pre-cleanup commit, not a work list.

## Requirements

- Windows x86-64.
- LLVM Clang targeting MSVC, with Visual Studio C++ libraries and a Windows SDK.
  MinGW Clang is not the supported toolchain.
- CMake 3.25 or newer, Ninja and Python 3.
- This repository's modified Aurora at sibling `aurora/`; CMake obtains the
  configured third-party dependencies. Reference-only folders are unnecessary.
- A local USA v1.02 Melee ISO and a local 4,096-byte DSP coefficient table.

The development cache uses Clang 22.1.6, MSVC 14.44 and RelWithDebInfo. A consistent
MSVC environment matters: libraries built with a different STL/toolset can fail
to link. Use explicit compiler paths if another Clang is earlier on PATH.

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

Normal boot reaches home character select. START enters grid practice; local
and online Start match lead to stage select with locked-in choices. The local
top-level batch launcher uses this workspace's build and sibling ISO.
See [home flow](native_home_flow.md) and [diagnostic match options](native_match_launch.md).

User data under `%LOCALAPPDATA%\Freak Melee` is separate from replaceable builds:
profiles, installed collection markers, cached packages and the CSS layout.
For tests use isolated stores, hidden/muted launches and the native input channel.

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
Fresh-clone build and clean-install Workshop checks remain explicit release work.

## Source and release boundaries

The private root repository contains `melee/` and the exact modified `aurora/`
source, native tools, tests, templates, UI and service source. It excludes ISOs,
extracted DAT/DOL data, DSP ROM, binaries, build caches, screenshots and profiling
archives. Existing nested histories remain local; do not push donor repositories.

See [release preparation](native_release.md). A known current packaging gap is
the omitted `native_content_tool.exe`; fix and test it before claiming a friend
can use the Workshop from an extracted release.
