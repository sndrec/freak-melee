# Profiles and optional controls: 2026-10-19

Custom digital Shield bindings now retain full strength when a profile also
maps analog triggers. Previously, sampling a released or partially pressed
trigger after the Shield action could overwrite that action's strength and
digital click. The input merge now preserves an explicitly held Shield action
after sampling the axes. Default Shield bindings retain their analog strength.
This change adds no mutable state or runtime allocations.

## Local checks

- `native_profile`, `native_profile_input` and `native_rumble_output` pass.
  The new shielding regression failed before the fix and passes afterward.
  It covers 72 combinations of trigger travel, button state, default-binding
  presence and digital-click emulation. Default profiles preserve all 65,536
  button masks across four canonical ports, with varied sticks, trigger strengths,
  analog A/B values and unrelated extension bits.
- Two hidden, muted native clients create, save and reopen an isolated profile.
  Saved tap-jump and attack-stick choices persist. Calibration round-trips
  17.5% main-stick dead zone and 0% / 100% trigger activation. Invalid text leaves
  the saved bytes intact. Background controller input stays out of CSS while
  the editor is open. Delete cancellation preserves the file; confirmation
  removes it. The disconnected-port check rejects binding capture immediately.
  Full-window screenshots were inspected; both clients stopped.
- Three hidden, muted Ganondorf practice runs each consume 2,200 recorded
  controller samples. The full jump rises about 27.3 units and the forced short
  hop about 16.4 units. Tap jump works by default and remains disabled with its
  profile flag. Side, up and down C-stick inputs select smashes by default and
  tilts with tilt-stick enabled. All three runs finish normally.
- The existing motor-output test checks suppression during reconstruction and
  routing to physical ports. It uses a mocked motor interface.

The gameplay recordings begin at the canonical input boundary, after device
mapping. They verify fighter behavior for the recorded profile flags. The
profile-input test separately exercises raw binding samples through mocked
device accessors.

## Reproduction and receipts

`tools/review_native_profile_controls.py` runs the saved-profile UI and gameplay
checks using a copied profile directory and isolated LocalAppData. It creates
its own recordings and keeps every launch hidden and muted. Supply `--exe`,
`--iso`, `--profile` and a new `--output` directory.

- `.local/native-profile-controls-2/receipt.json`: UI and gameplay results,
  replay hashes, screenshots and binary identity. UI acceptance completed
  first; gameplay was then resumed after correcting the runner's launch name
  from `ganon` to `ganondorf`. Separate runner hashes retain that distinction.
- `.local/native-profile-gameplay-2.log`: resumed gameplay results.
- `.local/native-profile-review-core-1.log`: the three focused native tests.
- `.local/native-profile-review-before-1.log`: the failing Shield regression.
- `.local/build-native-profile-game-1.log`: current native client build.

Client SHA-256:
`8110c948a0a474cab8bed13a154689be65508931ad2a6b0da59b8416a189dda6`.

## Hands-on acceptance

Raw SDL binding capture, physical GameCube adapter response and actual motor
feedback remain open. Canonical input injection does not exercise those device
paths. The overall profile/control plan item therefore stays unchecked.
