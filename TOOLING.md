# Local development tooling

Updated: 2026-10-13. For portable build instructions use
[native_build.md](melee/docs/native_build.md); for outstanding work use
[PROJECT_PLAN.md](PROJECT_PLAN.md). This file records this PC's setup, not
dependencies that every clone must contain.

## Native build on this PC

The cache at `melee/build/native` uses Clang 22.1.6, Ninja and RelWithDebInfo.
Its dependencies use MSVC 14.44.35207. Select that library environment before
building; an inherited older `VCToolsVersion` can otherwise select incompatible
libraries and produce unresolved `__std_*` or thread-runtime symbols.

From a Command Prompt at the workspace root:

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44
cmake --build melee/build/native --target melee_native native_content_tool -j 4
```

Local build inputs:

- `Super Smash Bros. Melee (USA) (v1.02).iso` in the workspace root.
- `.local/dsp_coef.bin`, the 4,096-byte DSP coefficient table.
- Modified Aurora in `aurora/`; dependency downloads stay in the build tree.

Use explicit paths/configuration for a new checkout. These local inputs are
excluded from the upstream repository; do not silently copy them into source.
The top-level `Launch Freak Melee.bat` opens the current native build and ISO.

## Tests and inspection

- Routine native test launches use `--hidden --mute` and the game's control
  channel. Do not send keyboard/mouse input to the user's desktop.
- The accelerated path retains CPU effects and state required by gameplay.
  Production rollback suppresses GPU output; see
  [rollback architecture](melee/docs/native_rollback_current.md).
- Use the built-in screenshot path for whole-UI PNG captures. A successful
  build or isolated fixture is not gameplay/pixel acceptance.
- Keep profilers/samplers separate from uninstrumented timing acceptance.
  Optimization is currently deferred.
- New tests should target a changed contract; avoid repeated broad launches
  when a scoped check suffices.

## Dolphin and Blender

The configured `dolphin_fzgx` MCP also supports Melee. Its name does not require
F-ZERO GX. Use a task-specific Dolphin user directory, identify the exact owned
PID and force-kill it after use. Normal close can leave a spinning process.
Do not claim reference parity merely from booting the emulator.

The Blender MCP operates the user's live Blender. Use it only for deliberate
authoring/inspection, preserving unsaved user work. Native model import uses
cgltf; HSDLib is a reference, not an ordinary runtime/import dependency.

## Source ownership and storage

The root Git repository pushes to `sndrec/freak-melee`, which is public. Nested
`melee` and `aurora` retain their original local Git histories and donor remotes.
Use explicit paths for commits; never push those donor remotes. The root index
contains actual source files, not embedded-repository links.

Excluded reference repositories remain local: HSDLib, m-ex, GGPO, Slippi and
Gecko. They are not prerequisites of the native client CMake build.

Before-doc-cleanup source, byte hashes and GitHub verification are at
`C:/FreakMelee-source-backups/2026-10-13-before-doc-cleanup/`.
Large current test outputs belong outside the repository. A: was nearly full
at this checkpoint, so do not follow old unconditional A:-archive instructions.
Check free space first. Retain active baselines, repro inputs and relevant failures;
do not make redundant copies of the whole SDK for each test.
