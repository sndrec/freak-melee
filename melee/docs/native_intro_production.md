# Intro FMV production

Updated: 2026-11-08. The user authorized a finished draft using real native
Workshop/gameplay footage, appropriate credited mods and original fighter edits.
Pause the goal when the draft is ready for critique. Commit locally; do not push.

The [caption script](../assets/intro/storyboard.json) retains the user's exact
wording. Split cards and adjust their holds as needed. The
[storyboard](native_intro_storyboard.md) supplies the scene intentions; its
static blocking is a reference for filming.

## Draft for review

The moving draft is
`.local/intro-production/final-cut-1/freak-melee-intro-draft.mp4`.
It runs **98.017 seconds**, at **960 × 720, 60 fps**, with stereo AAC sound.
The movie is approximately **46 MB**. The seventeen supplied captions retain
their original wording and order. Cuts, framing and holds follow the
[edit recipe](../assets/intro/edit.json).

The film includes native carousel selection; real hitbox, pose and script edits;
the edited moves in gameplay; a tile dragged to another player's card; actual
receipt and installation on the second client; a 1% exaggerated-knockback KO;
the deliberately oversized censor bar; the actual appearance setting and
hurtbox wireframe receiving Sing; and a four-fighter outfit lineup. The final
outfits are original, Scarlet and Forest Falco, followed by Tom Nook's Island
Shirt. The ending credits identify LarsBars and the Melee assets/music.

The review boundary is this standalone movie. Startup playback integration and
the user's pacing, visual and humor critique follow its review.

| Time | Scene |
| --- | --- |
| 00:00 | Your first fighter and clone selection |
| 00:15 | Workshop, hitboxes, animation and scripts |
| 00:44 | Send and receive an edited fighter |
| 00:55 | Absurd knockback and censored appearance |
| 01:04 | Appearance setting, smoke, wireframe and Sing |
| 01:10 | Stock fighter and outfit lineup |
| 01:28 | Closing title and credits |

The full movie decodes without errors. Its audit records 5,881 video frames,
48 kHz stereo audio, a -21.1 dB mean and a -2.1 dB peak. One-second contact sheets
cover the whole cut; the framing review corrected the growing Mario taunt,
caption/timer overlap and clipped edge of the costume lineup. These media checks
do not replace the user's creative review.

## Recorded sources

- Continuous native video capture works through the complete rendered client
  and an encoder pipe. A 156-frame native menu-transition capture was inspected.
  Normal screenshots stay unchanged. Wall-clock timestamps preserve real-time
  pacing when recording load reduces the present rate.
- Offline studio controls for camera, HUD, pause, actor poses and falling actors
  work through the local native control channel. A 189-frame fixed-camera
  gameplay clip shows Falco falling, landing with smoke and taunting beside
  Mario. These controls reject online matches and replay playback.
- Mixed PCM recording before the playback mute gain works. The muted studio
  pass recorded a valid 32 kHz stereo WAV with landing/taunt audio after stage
  music was stopped. Film clients can supply their sound effects to the edit.
- Third-party costume assets downloaded into `.local/intro-production/assets`.
  Tom Nook's Island Shirt and Sweater models import and render through ordinary
  owned costume packages. Rude Falco and the Cowboy Spacies models expose a
  missing-texture-coordinate shader path in the port; keep them out of the cut.
- Settings now offers **Custom fighter models: Show / Neutral**. Neutral uses
  posed hurtbox wireframes for source fighters and installed costumes during
  play; the frozen Workshop editor retains the authored model. It preserves the
  normal gameplay/render callbacks and reads the hurtbox/bone data for drawing.
  A custom Mario becomes neutral and receives Jigglypuff's Sing in
  `.local/intro-production/appearance-probe.mp4`. Its native captures were
  inspected. A 400-sample comparison matches all 1280 gameplay trace lines with
  the setting on/off, including 400 RNG and 800 fighter-state records; see
  `.local/intro-production/filter-comparison/receipt.json`. This is one Mario
  input sequence, rather than a complete roster audit.
- Independent Mario, Mario Remix, exaggerated-knockback Ganondorf and Falco
  projects are built in `.local/intro-production/cast-8`. Their interpreted
  scripts validate. `tools/prepare_intro_fighters.py` reproduces the cast.
  Mario Remix changes tilt radii, transfers an aerial-jump pose into forward
  air and gives fireballs upward acceleration. Ganondorf's 242 attack-hitbox
  references deal 1% with base knockback 400 for the deliberately absurd gag.
- `tools/record_intro_gameplay.py` records directed takes with normal physics
  and installed fighters. The closer gameplay takes are in
  `.local/intro-production/gameplay-take-3`. The final cut also uses take 7's
  stock-camera KO and Falco hero shot, take 8's smoke/filter/Sing sequence, and
  take 9's wider Mario taunt and four-actor costume lineup.
- Actual hitbox editing and pose transfer are recorded in `workshop-take-5`.
  Its saved animation DAT matches the authored Mario Remix DAT exactly.
  `workshop-take-6/script-edit-2.mp4` records the fireball physics edit and a
  successful source check. Earlier rejected takes stay out of the film.
- `tools/record_intro_menus.py` records the native first-fighter carousel and
  appearance setting in `menus-take-1`. `tools/record_intro_sharing.py` records
  tile-to-player delivery and acceptance in `sharing-take-5`; its receipt proves
  the previously uninstalled edited Mario is installed on the second client.
  Both clients use a loopback lobby service and isolated data.
- The original `/audio/howto.hps` is recorded through the native mixer as
  `.local/intro-production/howto-music.wav` (124.24 seconds, 32 kHz stereo).
- `tools/assemble_native_intro.py` reads the original caption script and edit
  recipe, trims the recorded plates, adds the censor/title/credit graphics and
  mixes the native effects with the soundtrack. `tools/audit_native_intro.py`
  decodes the result, measures its audio and makes visual-review contact sheets.
  Generated images, footage, music and fighter packages remain local.

## Issues found while filming

- Long script lines wrapped in the text area, while the syntax layer drew them
  unwrapped. Setting the native text area to `wrap="nowrap"` aligns text, caret,
  line numbers and keyboard navigation. The recorded script edit validates.
- Polygon descriptors sharing a backing vertex array used different inferred
  prefix lengths. The renderer uploaded the same arrays repeatedly, overflowing
  both 8 MiB and a temporary 16 MiB render buffer. Import now gives descriptors
  for one array their largest proven extent, using transient caller-arena
  scratch. The render buffer remains **8 MiB**. Native DAT tests pass, and the
  Tom Nook lineup records without that overflow.
- Pipeline configurations were persisted before compilation completed. Failed
  shaders could then crash unrelated launches during cache warmup. Pending
  records now publish after successful compilation. Cache loading skips GX
  configurations whose required vertex inputs are absent. A read-only backup of
  the existing user cache was tested in isolated film data: one invalid row was
  skipped and the native client survived warmup. The original database was kept.
- Accepting an already-cached fighter from a CSS button attempted preparation
  inside a GObj callback. The online session now queues acceptance for its pump,
  retaining the exact offer's ID, peer and serial. The cached gift installs
  through the real CSS flow in `sharing-take-4`.

## Asset credits

Record these credits in the finished film and its accompanying text. The files
are source material for the local draft, rather than repository assets.

| Asset | Credit | Source | Download SHA-256 |
| --- | --- | --- | --- |
| Tom Nook Falco — Island Shirt | LarsBars / @RealLarsBars | [SSBM Nucleus](https://ssbmnucleus.net/post/17634/tom-nook-falco) | `9dfe246e6d01e37c229bed7524aaf349905b7b97a31ec8052dbb5299a5dd3725` |

Rejected-source provenance: Rude Falco is knifeinsideasock3t_'s texture edit of a
baggy-pants model, [post 17642](https://ssbmnucleus.net/post/17642/rude-falco),
archive SHA-256 `ffba18d04d1791c553cbf68978c4a3baec68f8a03eddaf9a8f6382ccd210530f`.
The Cowboy Spacies compilation is from phant0m5351,
[post 17628](https://ssbmnucleus.net/post/17628/cowboy-spacies), archive SHA-256
`1bbf82fd37b1d1f4e2b0e2713178148abd3c4e2720cd4a7cbf211a67e6c691ea`.
The original [Lazlo author index](https://ssbmtextures.com/author/lazlo/) lists
Cowboy Spacies and baggy-pants Falco. Neither rejected model appears in the cut.
The Han Solo Falco page credits LarsBars, but its download was blocked in
Chrome and no asset was obtained. No costume's visual data was modified while
testing. Color-specific public archive symbols were rebound to the cloned
fighter's stock symbol names for costume import.

## Recording rules

Use isolated film user data, hidden muted clients and no debugger. Capture
gameplay from the native renderer. Music and effects can be mixed into the cut;
muting monitoring should not require playing sound on the user's speakers.
Keep authored projects and input/camera recipes so shots can be revised.

Use the real Workshop UI for each editing demonstration and show its actual
gameplay result. Keep the user's captions in their supplied order. Build the
appearance-filter setting before filming its claimed behavior. The censored
appearance joke can use a covered costume and an oversized bar throughout.

## Tooling

`melee_native_control video-start <path>` streams complete presented frames as
binary PPM. A named pipe feeds FFmpeg without intermediate image files.
`video-stop` flushes the final frame and closes the stream. The Python
`capture_native_video.NativeVideo` context manages the pipe/encoder for a
task-owned `Client`. It captures a sibling 32 kHz stereo WAV before the playback
mute gain. Use `sound=False` for silent plates and `realtime=False` only for
frame-by-frame output whose timeline already advances at a fixed capture rate.
The CLI exposes `audio-start <WAV path>` and `audio-stop` separately.

`melee_native_control studio <command>` accepts these offline direction cues:

- `state`: list live actor kinds, motion states, frames and positions.
- `camera,eyeX,eyeY,eyeZ,targetX,targetY,targetZ,fov`: set a fixed free camera.
- `hud,0` / `hud,1`: hide or show the match HUD.
- `pause,1` / `pause,0`: freeze or resume simulation.
- `fall,port,x,y`: place a fighter in an ordinary falling state.
- `pose,port,motion,frame,x,y,facing`: enter a valid fighter motion at a frame;
  facing is -1 or 1. Motion IDs refer to the loaded fighter's state table.
- `reset`: restore standard camera, HUD and running simulation.
- `music,off`: stop the stage music while retaining voices and effects.
- `music,STEM`: play a disc/native HPS asset from `/audio/STEM.hps`.
- `appearance,0` / `appearance,1`: apply the same saved neutral/show setting
  used by Settings.
- `particle,port,id,scale`: spawn a common-bank particle generator at an actor,
  using a uniform scale. The film uses generator 90 for the transformation smoke.

Ports are zero-based. These cues stage the actors; the game then runs their
usual physics, collisions, animations and authored behavior.

Evidence: `.local/intro-production/capture-probe-2.mp4` contains the native UI
pass. `studio-probe-3.mp4` and its WAV contain the fixed-camera gameplay/audio
pass. The clients' logs record accepted commands and flushed frame/PCM counts.
They verify the recording path; they are not finished movie shots.

`native_content_tool project-stock-template PROJECT_DIRECTORY` applies the same
editable stock script/parameter and owned-hitbox setup used by the Workshop's
clone flow. The film's independent Mario project is in
`.local/intro-production/authoring/Mario`.

`project-hitbox-power PROJECT DAMAGE GROWTH BASE` edits all attack hitboxes in
one authoring transaction and clears fixed-weight knockback. Shared command
graphs become action-owned before editing. Invalid operands or a failed edit
leave the on-disk draft unchanged.

The native control tool also accepts middle/right `ui-drag` gestures with
Shift/Ctrl/Alt modifiers, and `ui-wheel x y steps`. They feed the existing
Workshop mouse listeners without moving the desktop cursor. Positive wheel
steps scroll up or zoom in.

`ui-down x y` and `ui-up x y`, with optional `right` or `middle`, hold and release
the native pointer independently. The sharing take sends intermediate `ui-move`
positions over time, so the hand and fighter tile visibly travel to the recipient.

Rebuild the cut and make review sheets:

```powershell
python melee/tools/assemble_native_intro.py --plates .local/intro-production --output .local/intro-production/final-cut-1
python melee/tools/audit_native_intro.py .local/intro-production/final-cut-1/freak-melee-intro-draft.mp4
```

Keep the source takes and authoring projects for revision after the user's pass.
The local lobby service and all filming clients are stopped at this boundary.
