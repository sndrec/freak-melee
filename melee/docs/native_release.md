# Prepared Windows releases

Updated: 2026-11-09. The private playtest channel carries the desync diagnostics
build. The October local acceptance below covers installation, Workshop authoring,
and launcher updates.

## Prepare

Build both `melee_native` and `native_content_tool` in Release or RelWithDebInfo.
See [the build guide](native_build.md) for toolchain and private input requirements.
CMake generates `native_release_runtime.txt` from the executables' DLL dependencies
and `native_release_ui.txt` from the current UI, documentation, fonts, portraits and
fighter templates. Preparation requires these lists and every declared file.

The payload includes the game, Workshop tool, their runtime DLLs, current UI assets
and launcher. Test DLLs, stale build assets, debug tools, ISO and user data stay out
of the release. The setup ZIP contains a complete first installation; the update
ZIP contains one complete version. Each needs a new release ID and output folder.

Run from the `melee` checkout, substituting the intended release ID:

```powershell
cmake --build build/native --target melee_native native_content_tool -j 4
python tools/prepare_native_release.py --build build/native --output build/native/releases/2026-10-17-a --release 2026-10-17-a --channel https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/latest.json
python tools/publish_native_release.py build/native/releases/2026-10-17-a --dry-run
```

Preparation creates an update ZIP, `latest.json`, an `install/` folder and
`freak-melee-setup-<release>.zip`. The publisher's dry run verifies both archives,
their exact file lists, initial version selection and every version-file hash.
It performs no upload.

## Launch and update

Extract the setup ZIP and run `Freak Melee.bat`. On first launch choose a local
Melee USA v1.02 ISO. Its path is stored in
`%LOCALAPPDATA%\Freak Melee\iso-path.txt`. Profiles, installed fighters, drafts and
the CSS layout live in the same user-data directory. Replaceable builds live
under `install/versions/`. The launcher uses the selected version as the game's
working directory so its assets resolve beside the executable.

Before launch, the launcher reads `channel.txt`, verifies a complete update and
activates it while retaining the previous build. It refuses concurrent launcher
runs. Standalone installs with an empty `channel.txt` can join the hosted channel
by saving the manifest URL above on the first line of that file. To download and
activate without starting the game:

```powershell
.\Freak Melee.bat -CheckOnly
```

To restore the previous version:

```powershell
.\Freak Melee.bat -Rollback
```

The rejected version is skipped until the channel offers another release.
`-Rollback -CheckOnly` changes the selected version without launching.
`-Hidden -Mute` launches a hidden, muted client for automated checks.
An explicit `-Channel <local latest.json>` supports isolated local update tests.

## Publish

The private channel uses the `services/releases` Worker and the
`freak-melee-releases` R2 bucket. The channel URL above is an unlisted invite link.
Install its service dependencies with `npm ci` before publishing. For an approved
build intended for friends, run:

```powershell
python tools/publish_native_release.py build/native/releases/2026-10-17-a
```

This uploads both ZIPs, downloads and hashes each, and publishes `latest.json`
last. Share the setup URL ending in `freak-melee-setup-<release>.zip`.
The channel's `latest.json` shows the current release.

## Playtest desync reports

The diagnostics build sends one report when an active online match aborts. It
includes the build, session and peer numbers, stage and fighter IDs, recent
controller samples and state hashes, and the failure reason. It keeps the inputs
each tick actually used, the RNG seed, and whether rollback replayed the tick.
Near the mismatch it records exact fighter hitbox and hurtbox values, including
cached positions. Reports stay in
the private release bucket under `netplay-diagnostics/` for 14 days. Each client
also saves its report under `%LOCALAPPDATA%\Freak Melee\netplay-diagnostics`.

Run `python tools/compare_netplay_reports.py <peer-0.log> <peer-1.log>` to find
the first shared confirmed frame with a different checksum and see the fields
that differ.

To build with reporting, configure `NATIVE_NETPLAY_DIAGNOSTICS_URL` with the
hosted channel's `/diagnostics` endpoint and `NATIVE_NETPLAY_DIAGNOSTICS_BUILD`
with the release ID. A fresh build defaults to reporting off. To turn it off in
an existing build directory, clear `NATIVE_NETPLAY_DIAGNOSTICS_URL` and rebuild.

## Local acceptance — 2026-10-17

The current native build and Workshop helper built successfully. Seven packaging
contract tests passed, including missing tools/assets, fixture exclusion, corrupt
setup assets and matching update/setup payloads. Both prepared local releases
passed the publisher's dry run. Their 165 payload files include seven runtime DLLs
and 26 fighter-form script templates.

`review_native_hub.py --script-check-build` ran the actual executable from an
extracted setup ZIP in `.local/native friend install`, with the development control
tool outside that installation. Its child PATH contained only Windows and
System32. In `.local/native-release-workshop-acceptance-3`, it verified:

- Empty first launch; playable starter clone and CSS placement.
- Rename replacing the installed revision; an independent second fighter copy.
- CRLF source remaining unchanged when merely opened in the frozen preview.
- Workshop Check, Build and Build & install using the packaged helper.
- Reinstallation keeping two installed fighters rather than adding a duplicate.
- Profile saving and controller return from local character select.

The first expanded test missed the attributes drawer's scrollbar because UI
inspection reports its content box. Correcting the test's scrollbar coordinates
allowed the actual install button to be exercised. An earlier screenshot run in
`.local/native-release-workshop-acceptance-1` was visually reviewed.

`review_native_release.py` extracted a fresh setup ZIP into a path with spaces,
launched through the PowerShell launcher, reached the native hub and closed its
own hidden window normally. It then used the batch entry point to update from
`2026-10-17-local-c` to `2026-10-17-local-d`, verify all installed file hashes,
roll back, and skip the rejected version on a subsequent update check.
All 36 player-data files retained their hashes, including profiles, installed
content, drafts and CSS layout. Evidence is in
`.local/native-release-launcher-acceptance-2`.

The minimal-PATH test also exposed the batch file's reliance on finding
PowerShell through PATH. It now uses Windows' installed PowerShell directly and
preserves the launcher's exit code.

### Fresh source

A Git archive of `eec1d55` was extracted into
`.local/native-source-eec1d55`. This contains tracked source files, with no old
build, nested Git history or untracked reference repositories. CMake configured
a separate RelWithDebInfo build with Clang 22.1.6 / MSVC 14.44, explicit ISO and
DSP-table inputs, and its own dependency downloads. Both `melee_native` and
`native_content_tool` built successfully; the release-payload CTest also passed.

The compile-command audit covers all 6,340 configured entries. Every source
resides in the export or its own downloaded dependencies, and no compile flags
refer back to the development source/build. The logs are
`.local/native-source-configure.log`, `.local/native-source-build.log` and
`.local/native-source-path-audit.txt`.

The resulting `2026-10-17-source-eec1d55` release passed publisher dry-run
validation and was extracted into `.local/native-source-release-extracted`.
With a minimal PATH, the same native first-launch and Workshop suite passed
in `.local/native-source-workshop-acceptance-1`, including Check, Build and
Build & install. Whole-UI PNGs were reviewed, including the script workspace
and successful installation status. The development control tool remained
outside the installation. No build products from the old tree were copied in.

Separate friend-PC acceptance remains open. These same-PC checks do not
establish separate-network behavior, physical controller usability, code
signing or current hosted-service compatibility. Quick Play's new service
protocol is still local and undeployed.
