# Local tooling readiness

Verified 2026-09-10. This is tooling evidence, not native-client or gameplay acceptance.

## Dolphin MCP

The available `dolphin_fzgx` MCP is usable with Melee; its name does not restrict it to F-ZERO GX. No MCP installation or configuration change was necessary.

Verified operations:

- Discovered no pre-existing instrumented instances.
- Launched the supplied USA v1.02 ISO and connected successfully; game ID `GALE01`.
- Runtime identified itself as `Dolphin [codex/fzgx-automation] 2603a-5-dirty`, protocol 5.
- Captured and visually inspected `.local/dolphin-user/ScreenShots/GALE01/melee-tooling-probe.png`.
- Paused, synchronously advanced two rendered frames, and stopped emulation after the probe.

Subsequent user instruction: stopping/closing Dolphin can leave a CPU-spinning process on this PC. After future use, verify the PID belongs to this task and force-kill it with `Stop-Process -Id <pid> -Force`. At the start of native-build work, no remaining `Dolphin.exe` process was found.

The screenshot shows the original missing-memory-card startup prompt. This establishes game boot/rendering and screenshot access, not completion of a match. Controller injection, breakpoints, memory inspection, and the build's numerical suitability as an original-game reference remain untested in this project. The instrumented build is modified; investigate its settings or patches if comparisons disagree.

Use `launch` with the ISO's absolute path and `user_directory = C:/gayms/freak-melee/.local/dolphin-user`. The tool found its configured executable automatically. Keep this user directory separate from existing Dolphin/F-ZERO GX configurations. A later reference-play session should initialize its own save configuration if needed.

No additional MCP is currently justified. The existing tool offers debugging/memory/controller APIs to evaluate when needed. HSDLib and build tools are accessible directly from the shell. Native-client screenshot/debug automation will be evaluated when an executable exists.

## Added references

| Folder | Use |
| --- | --- |
| `HSDLib/` | HSDRaw DAT structures, relocation-aware parsing, Melee and MEX asset definitions; HSDRawViewer source for asset editing. |
| `ggpo/` | Rollback implementation and integration requirements; not yet integrated or built. |
| `slippi-ssbm-asm/` | Melee-side recording, playback, online, and save/load hooks; reference for game-specific behavior. |
| `project-slippi/` | Project/replay documentation, libraries and launcher references. Its README points elsewhere for the emulator implementation; do not mistake this folder for the Slippi Dolphin source. |
| `gecko/` | Gecko assembly/build tooling and bundled PowerPC assembler/objcopy executables; reference/testing tools, not the native character runtime. |

No existing widescreen patch was selected or verified during this inventory.

## Build environment

### Native build checkpoint — 2026-10-02

The existing `melee/build/native` cache uses Clang 22.1.6, Ninja and
`RelWithDebInfo`. Its cached C++ dependencies were built with MSVC 14.44.35207.
Select that library environment explicitly before building; this session's
inherited `VCToolsVersion=14.38` otherwise makes `vcvars64.bat` choose older
libraries, producing unresolved `__std_*` and thread-runtime symbols when
linking the content tool. This is a toolchain mismatch, not a game source error.

From a Command Prompt in `C:\gayms\freak-melee\melee`:

```bat
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" -vcvars_ver=14.44
"C:\Program Files\CMake\bin\cmake.exe" --build build/native -j 4
```

Use the same environment for standalone generated-module/source comparisons.
Native game tests should remain hidden and muted unless a visible inspection
is requested. Accelerated tests retain drawing.

Found on PATH: CMake, Ninja, Clang/clang-cl, Python, Git, and dotnet. Windows SDK library directories include 10.0.26100.0. Dotnet SDKs include 10.0.100. Discovery is not proof that a complete native game toolchain links successfully; that belongs to M0.

HSDRaw targets netstandard2.0 and built successfully with zero warnings/errors:

```powershell
dotnet build HSDLib\HSDRaw\HSDRaw.csproj -c Release -o C:\gayms\freak-melee\.local\tools\hsdraw --nologo -v quiet
```

Output: `.local/tools/hsdraw/HSDRaw.dll`. This verifies the library build, not asset-conversion correctness. The viewer application was not built or launched. Use the library for inspection/import tooling where suitable; a .NET dependency in an offline tool need not become a dependency of the gameplay runtime.

The game source was not modified in this preparation pass. Native toolchain/dependency configuration is the next implementation milestone.

## Test-output storage — 2026-10-03

Keep bulky new test recordings, copied runtimes, and frozen snapshots in
`A:\FreakMelee-test-archives`, rather than accumulating them on C:. Retain recent
baselines and older inputs explicitly used by regression comparisons; retire
redundant compiler/runtime snapshots instead of preserving every intermediate
SDK indefinitely. Keep diagnostic logs and specific failure captures.

The C: cleanup retired 49 unused baselines from the baseline5–60 range. The
still-referenced baseline11, baseline15, baseline42, baseline43, baseline48,
and baseline49 remain accessible through their original `.local` paths, with
verified copies under `A:\FreakMelee-test-archives\C-drive-cleanup-20261003`.
Baselines61–65, current builds, author projects, and failure captures were kept.
The cleanup receipt and retained-copy hashes are in that archive directory.
