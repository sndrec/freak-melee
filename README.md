# Freak Melee

Private Windows native Melee client and character-authoring workshop. The full
Melee decompilation is the game base, with our modified Aurora as the platform
and renderer. Ordinary fighter behavior runs as typed interpreted scripts.

## Source layout

- `melee/`: game/HSD source, native interpreter, Workshop, UI, tools, tests,
  fighter templates and optional signaling/release service source.
- `aurora/`: the exact modified framework required by this client; its CMake
  configuration obtains pinned third-party dependencies.
- `PROJECT_PLAN.md`: product contract and progress evidence.
- `TOOLING.md`: local tooling and build notes.

This repository is a source snapshot, not a copy of the development machine.
It excludes the ISO, extracted game assets, SDK/tool binaries, build products,
screenshots, profiling archives and reference-only repositories. The initial
commit preserves the project documents before their cleanup.

## Build

Use Windows x86-64, LLVM Clang targeting MSVC, Visual Studio C++ libraries and
Windows SDK, CMake 3.25 or newer, Ninja, and Python 3. Select a consistent MSVC
library environment before configuring. The current development build uses
Clang 22.1.6 and MSVC 14.44.

Two private local inputs are required: the USA v1.02 Melee ISO, and a 4,096-byte
DSP resampling coefficient table. Neither is uploaded here. Supply both paths
explicitly; the current coefficient preparation tool consumes a local table
and does not extract it from the ISO.

From a Visual Studio developer Command Prompt at this repository root:

```bat
cmake -S melee -B melee/build/native -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DMELEE_ISO="C:/local/Melee-USA-v1.02.iso" -DMELEE_DSP_COEFFICIENTS="C:/local/dsp_coef.bin"
cmake --build melee/build/native --target melee_native native_content_tool -j 4
melee\build\native\melee_native.exe --iso "C:/local/Melee-USA-v1.02.iso"
```

The game and content tool build together for Workshop use. No external C
compiler is needed to execute ordinary character scripts. This snapshot has
not yet been built in a fresh clone; existing-checkout build evidence is not
a clean-clone acceptance claim.

## Provenance

The game source derives from [doldecomp/melee](https://github.com/doldecomp/melee).
Aurora derives from [encounter/aurora](https://github.com/encounter/aurora);
its MIT license is retained at `aurora/LICENSE`. Third-party code retains its
own notices, including cgltf. See `SOURCE_PROVENANCE.json` for the exact local
base commits. No blanket license is asserted for the whole combined tree.
