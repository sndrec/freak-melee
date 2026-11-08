# Intro FMV production

Updated: 2026-11-08. The user authorized a finished draft using real native
Workshop/gameplay footage, appropriate credited mods and original fighter edits.
Pause the goal when the draft is ready for critique. Commit locally; do not push.

The [caption script](../assets/intro/storyboard.json) retains the user's exact
wording. Split cards and adjust their holds as needed. The
[storyboard](native_intro_storyboard.md) supplies the scene intentions; its
static blocking is a reference for filming.

## Current work

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
  Import them through the normal costume/owned-fighter paths before filming.
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
  projects are built in `.local/intro-production/cast-4`. Their interpreted
  scripts validate, and all three credited Falco outfits import as ordinary
  owned costume packages. `tools/prepare_intro_fighters.py` reproduces the cast.
  Mario Remix changes tilt radii, transfers an aerial-jump pose into forward
  air and gives fireballs upward acceleration. Ganondorf's 242 attack-hitbox
  references deal 1% with base knockback 400 for the deliberately absurd gag.
- `tools/record_intro_gameplay.py` records directed takes with normal physics
  and installed fighters. Six first takes are in
  `.local/intro-production/gameplay-take-1`; the costume-lineup take exposed
  renderer failures and is being repaired before filming. These takes are
  source footage, not a finished cut.
- The original `/audio/howto.hps` is recorded through the native mixer as
  `.local/intro-production/howto-music.wav` (124.24 seconds, 32 kHz stereo).
- Still required: Workshop recording, sharing footage, costume-lineup take,
  finished gag/composition, final cut and end-to-end visual/audio review.

## Asset credits

Record these credits in the finished film and its accompanying text. The files
are source material for the local draft, rather than repository assets.

| Asset | Credit | Source | Download SHA-256 |
| --- | --- | --- | --- |
| Rude Falco | knifeinsideasock3t_ — texture edit of a baggy-pants model, as described on the post | [SSBM Nucleus](https://ssbmnucleus.net/post/17642/rude-falco) | `ffba18d04d1791c553cbf68978c4a3baec68f8a03eddaf9a8f6382ccd210530f` |
| Tom Nook Falco — Island Shirt and Sweater | LarsBars / @RealLarsBars | [SSBM Nucleus](https://ssbmnucleus.net/post/17634/tom-nook-falco) | `9dfe246e6d01e37c229bed7524aaf349905b7b97a31ec8052dbb5299a5dd3725` |

The Rude Falco post does not identify the underlying baggy-pants model's author.
Try to identify that contributor before the final credits. A separate cowboy
pack was inspected and downloaded, but is not selected for the draft; it mixes
many works without clear individual attribution.

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
  using a uniform scale. The film uses generator 202 for the transformation puff.

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
