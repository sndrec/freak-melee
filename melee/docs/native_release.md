# Prepared Windows releases

Updated: 2026-11-14. The public playtest download channel carries
`2026-11-13-community-1`, including fighter deletion, news, announcements and F2
feedback. The October local acceptance below covers installation, Workshop
authoring, and launcher updates.

The manifest and download URLs are public distribution endpoints. Their path
component is a channel name, not a password or an access-control mechanism.
Publishing is performed separately with the maintainer's Cloudflare access.
The optional startup HPS is packaged from the local build output; its source
audio is excluded from Git. See [disc assets](native_disc_assets.md).

The community release contains 205 payload files. Its 37,387,668-byte update
archive and complete setup archive passed preparation checks and remote hash
verification through both R2 and the public download routes. A hidden, muted
packaged launch with Windows-only PATH loaded the hosted news article and
captured F2 feedback with the correct release and source identity. The source
revision is `6e491992b293f2d0c80959ef8983f3a586ee2a5a`; the executable SHA-256 is
`0c0a817ed6d2348a29b68866afc60401a2514dfaec8076397534e9b6daec427c`.
Hosted administration is at
`https://freak-melee-community.sndrec32exe.workers.dev/admin/`.

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

Use the repository's [$publish-build workflow](../../.agents/skills/publish-build/SKILL.md)
when asking an agent to publish. This coordinates GitHub Releases, an unpublished
News draft, and the public auto-update channel. Builds run on the maintainer's
PC, where the private ISO and optional startup music are available. Creating or
editing this workflow performs no publication.

The channel uses the `services/releases` Worker and the `freak-melee-releases`
R2 bucket. Install that service's dependencies with `npm ci`. GitHub publishing
needs an authenticated tool, GitHub CLI or browser. News uses the existing
[protected administration page](https://freak-melee-community.sndrec32exe.workers.dev/admin/).
Use one publishing run at a time.

### 1. Identify the shipped baseline

Download and save the channel's current `latest.json` before changing anything.
Its release ID identifies the previous shipped build. Obtain that build's
`source_revision` from its GitHub `release-info.json` asset. Check that the
record's archive hash matches the live manifest. Also check repository identity,
the intended source revision and existing release tags.

The current pre-workflow build has a verified baseline record at
[releases/2026-11-13-community-1.json](releases/2026-11-13-community-1.json).
Its original source revision was mapped through both history rewrites; the
source trees match apart from the removed OTF/HPS inputs, and its executable
hash matches the live channel. Use the mapped `source_revision` for comparisons.
For a different older build without metadata, recover its source revision from
build receipts or diagnostics. Resolve rewritten revisions through the rewrite
maps. Ask for a baseline if it cannot be established; do not guess from dates
or the previous Git push.

### 2. Inspect changes and write patch notes

Review the entire range from the previous shipped source to the intended source:

```powershell
git log --reverse --format="%h %s" <previous-source>..<source>
git diff --stat <previous-source> <source>
git diff <previous-source> <source>
```

Inspect all changed files and relevant commits, including game code, scripts,
Workshop, menus, assets, services, build tools, tests and documentation. Use
commit messages as context and the final diff to establish what actually ships.
Include packaged local asset changes when their content changed outside Git.

Write `melee/docs/releases/<release>.md` with concise player-facing patch notes.
Group changes under useful headings such as Gameplay, Workshop and Online;
omit empty categories. Describe what changed for the player. Summarize or omit
maintenance details that have no player-visible effect. Include known limitations
that materially affect the release. Distinguish service changes already deployed
from changes introduced by this client build. Do not dump commit subjects or
claim improvements from reverted/unfinished work. Keep the News body within its
32,000-character limit.

Use that file verbatim as the GitHub release body and the initial News body.
Commit the notes and intended source changes, push them, and pin the resulting
full source commit. Do not modify the tracked build inputs after pinning it.

### 3. Build, check and prepare

Configure again after the source commit is pinned so the embedded diagnostic
source revision is current. Build `melee_native` and `native_content_tool`, run
checks appropriate to the changes, and review the prepared installation with a
hidden, muted client. Ask or wait if the owner's running game locks the build.
Check that the embedded/configured source identity matches the pinned commit
and is not marked modified. Preserve the optional local startup HPS for official
builds. Keep game data, account keys and player profiles out of the payload.

Choose a new release ID; never reuse an ID for different bytes. Prepare both
ZIPs with the hosted channel URL, then run the existing publication dry run.
Store the captured old manifest, hashes and stage results in a local
`publication-receipt.json` beside the prepared output. Receipts contain no tokens
or account keys. Do not create an unrelated smoke test on the live channel.

### 4. Stage the updater archives

Check whether the chosen archive names already exist at the public download
paths. Existing objects must match this run's expected sizes and hashes; stop
and use a new release ID if they contain different bytes. Reusing an identical
staged upload is safe when resuming the same release.

From the `melee` directory:

```powershell
python tools/publish_native_release.py <prepared-directory> --stage-only
```

This uploads the setup/update ZIPs, downloads both and verifies their SHA-256
hashes. It leaves `latest.json` unchanged. Record successful staging.

### 5. Prepare GitHub and the News draft

Create a GitHub draft release in `sndrec/freak-melee` using the release ID as its
tag, the pinned source commit as its target, and the patch-notes file as its body.
Upload the setup ZIP, update ZIP, `latest.json` and `patch-notes.md`. Verify asset
names, sizes and hashes using GitHub's asset digests or authenticated downloads.
Avoid autogenerated commit-log release notes.

In the administration page, create a News article titled `Freak Melee <release>`.
Its body starts with only the patch-notes file's text. Leave ticker text empty,
Published unchecked, publication time empty and Archived unchecked. Save the
article to the server and reopen it to verify the saved body and unpublished
state. A form draft in browser storage alone is insufficient. Record the article
ID when available, otherwise its exact unique title and admin URL. Create no
announcement and do not publish or schedule News during this workflow.

Write `release-info.json` beside the prepared output and attach it to the GitHub
draft. Include `release`, `source_revision`, `previous_release`,
`previous_source_revision`, `archive`, `size`, `sha256`, `setup_sha256`,
`executable_sha256`, `patch_notes_sha256`, and the saved News draft's title/ID.
Copy archive values from the validated manifest and hash the actual setup ZIP
and notes file. This record identifies the shipped source for the next workflow.

### 6. Publish GitHub, then activate the updater

Publish the verified GitHub release. Confirm its tag resolves to the pinned
source, its patch notes are correct, and its downloads are available. Re-read
the live updater manifest: it must still match the captured previous manifest,
or already match this exact release on a resumed run. If another release went
live in the meantime, stop before replacing it.

```powershell
python tools/publish_native_release.py <prepared-directory> --activate-only
```

Activation downloads and verifies both staged remote archives again, then writes
`latest.json` last. It does not upload replacement archive bytes. Re-read the
public manifest and verify its release ID, archive name, size, hash and file list
against the prepared manifest. Verify the public setup/update download paths
and complete the receipt. Check that the saved News article remains unpublished
unless the owner independently changed it.

Return the GitHub release, setup download, source revision, updater status and
the News draft title/admin link. The owner can now add personal notes and publish
News at their convenience.

### Failure and retry

GitHub, News and R2 are separate services, so this is a staged operation rather
than a single transaction. Before activation, failures leave the old updater
manifest intact; uploaded ZIPs and drafts may remain. Save their identifiers and
report the partial state. If GitHub publication succeeded but updater activation
failed, say so explicitly and retry activation after verifying all prerequisites.

Resume from the receipt. Verify existing release targets and asset hashes instead
of creating duplicates or replacing different bytes. Reuse the recorded News
article; preserve any personal edits or publication the owner has made. Never
reset an existing article to the generated patch notes. A retry that is already
fully live verifies the result and reports it without issuing another News post.

Without a stage flag, the publisher still performs its complete R2 upload,
verification and activation sequence. Coordinated releases use the staged
procedure above so the GitHub release and News draft are ready first.

## Playtest desync reports

The diagnostics build sends one report when an active online match aborts. It
includes the build, session and peer numbers, stage and fighter IDs, recent
controller samples and state hashes, and the failure reason. It keeps the inputs
each tick actually used, the RNG seed, and whether rollback replayed the tick.
Near the mismatch it records exact fighter hitbox and hurtbox values, including
cached positions. For hurtboxes refreshed on a recorded tick, it also records the
joint transform, parent matrix and floating-point mode at the refresh. Reports stay in
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
