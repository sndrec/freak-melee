# Freak Melee

Private Windows native Melee client and character-authoring workshop. The full
Melee decompilation is the game base, with our modified Aurora as the platform
and renderer. Ordinary fighter behavior runs as typed interpreted scripts.

## Source layout

- `melee/`: game/HSD source, native interpreter, Workshop, UI, tools, tests,
  fighter templates and optional signaling/release service source.
- `aurora/`: the exact modified framework required by this client; its CMake
  configuration obtains pinned third-party dependencies.
- `PROJECT_PLAN.md`: current product contract and remaining work.
- `melee/docs/README.md`: current guides and dated evidence index.
- `TOOLING.md`: local tooling and build notes.

This repository is a source snapshot, not a copy of the development machine.
It excludes the ISO, extracted game assets, SDK/tool binaries, build products,
screenshots, profiling archives and reference-only repositories. The initial
commit preserves the project documents before their cleanup; current guides
separate implemented features from remaining work and historical evidence.

## Build

Use Windows x86-64, LLVM Clang targeting MSVC, Visual Studio C++ libraries and
Windows SDK, CMake 3.25 or newer, Ninja, and Python 3 with Pillow
(`python -m pip install -r melee/tools/requirements-build.txt`). Select a consistent MSVC
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

Menu glyph atlases are prepared from the verified ISO, the supplied private
font/reference assets, and Windows Impact. See [font inputs](melee/assets/fonts/README.md).
Starter-fighter portraits are also extracted from the ISO into the build's
`ui/portraits` directory.

The game and content tool build together for Workshop use. No external C
compiler is needed to execute ordinary character scripts. This snapshot has
not yet been built in a fresh clone; existing-checkout build evidence is not
a clean-clone acceptance claim.

## Native hub

Ordinary launches open the game-rendered hub. Workshop lists installed fighters
and editable drafts. Clone fighter creates a playable, independent copy of a
supported vanilla fighter. New installations start with an empty personal
roster; existing collections and saved CSS layouts are preserved.

Lobbies provides room creation and joining. Chatroom shows global messages and
the players currently connected. The Melee hand supports analog controller
navigation; text entry uses the keyboard. F1 or holding B returns from home
character select to the hub. Detailed Workshop editing retains its existing
mouse and keyboard interface.

## Provenance

The game source derives from [doldecomp/melee](https://github.com/doldecomp/melee).
Aurora derives from [encounter/aurora](https://github.com/encounter/aurora);
its MIT license is retained at `aurora/LICENSE`. Third-party code retains its
own notices, including cgltf. See `SOURCE_PROVENANCE.json` for the exact local
base commits. No blanket license is asserted for the whole combined tree.

Current priorities: native social-hub integration, CSS participant and sharing
flows, Quick Play, and the planned account/inbox service. Collaborative Workshop
review continues alongside this work. Optimization,
Kirby cloning and whole-move transfer are deferred. See the
[project plan](PROJECT_PLAN.md) and [documentation index](melee/docs/README.md).
