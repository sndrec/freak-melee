# Prepared Windows releases

> Current release-readiness gap (2026-10-13): Workshop source checking requires native_content_tool.exe beside the client, but prepare_native_release.py currently omits it. Older preview/update checks below are dated evidence, not acceptance of the current interpreted Workshop release or a verified latest hosted version.

Build `melee_native` in release mode first, then prepare a release from that
build folder using the commands below.

The preparation command creates an update ZIP, `latest.json`, an `install/`
folder, and a `freak-melee-setup-<release>.zip` first-install download. Friends
extract the setup ZIP once and run `Freak Melee.bat` from it.
Their `Freak Melee.bat` checks `channel.txt` before each launch, installs any
complete new release, and starts the selected version. Host `latest.json` and
the ZIP in the same HTTPS directory; the manifest uses the ZIP filename
relative to its own location. Publishing a new release means uploading the ZIP
first and replacing `latest.json` last. No update is applied to a running game.

The Cloudflare release channel is
`https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/latest.json`.
Its private R2 bucket is named `freak-melee-releases`; the Worker streams only
the channel manifest and release ZIPs. The channel path is an unlisted invite
link for friends, rather than a user login. Before the first publication, the
empty channel returned 404. The current private preview,
`2026-09-24-preview-4`, is now published. Its [one-time setup ZIP](https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/freak-melee-setup-2026-09-24-preview-4.zip)
can be shared with a friend for testing. To prepare a later build, use
the following commands from the `melee` checkout:

```powershell
python tools/prepare_native_release.py --build build/native --output build/native/releases/2026-09-24-a --release 2026-09-24-a --channel https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/latest.json
python tools/publish_native_release.py build/native/releases/2026-09-24-a --dry-run
python tools/publish_native_release.py build/native/releases/2026-09-24-a
```

The final command uploads both ZIPs to R2, downloads each to verify its hash,
then publishes the manifest. Run it only for a build intended for friends.
Send friends the unlisted setup URL
`https://freak-melee-releases.sndrec32exe.workers.dev/3ShgcrpBLwN2237YIwwm6kX0/freak-melee-setup-<release>.zip`.
The ISO is absent from both ZIPs. The release service lives in `services/releases`;
run `npm ci` there before publishing. A new release ID and output directory
are required for each build. Once the first release is published, the channel
URL above can be placed in existing friends' `install/channel.txt` as well.

The ISO is deliberately absent from every release. On first launch the friend
selects their own USA v1.02 ISO. Its path is remembered under
`%LOCALAPPDATA%\Freak Melee\iso-path.txt`; profiles and content already live
under that user-data directory. Builds remain under `install/versions/` and
can be replaced without touching user data. The game starts with its version
folder as its working directory so its `ui/` files resolve normally.

To recover the previous build, run `Freak Melee.bat -Rollback`. This switches
to the retained version and launches it. The rejected release is skipped on
subsequent launches until the channel points to a newer release. To download
and activate without opening the game, use `Freak Melee.bat -CheckOnly`.

The preview's HTTPS manifest and setup ZIP were fetched successfully, and the
downloaded setup hash matched the prepared archive. A fresh extraction ran the
friend-facing launcher and loaded the game executable's help path. An isolated
earlier installation updated through the batch launcher, rolled back, and
skipped the rejected version on its next check. Update archives now carry the
launcher script, which replaces the installed script on activation. The first
two previews shipped an older launcher that can fail to find Windows
PowerShell's `Get-FileHash` through the batch path on this machine; use the
preview-4 setup ZIP for a fresh install. A clean preview-3 install updated to
preview 4 through its batch launcher and retained preview 3 for rollback;
the installed launcher script matched the new version. The build has no code signature or
publisher identity; test it on a separate Windows PC before calling it
friend-ready.
