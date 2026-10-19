# Port calibration through controller replacement

2026-10-19. Assigned profile calibration now follows its physical controller
port throughout the session. A controller connected later receives the same
dead zones and trigger thresholds. A replacement device does too. Moving a
device to another port applies the destination port's calibration.

Previously, assignment modified only the device connected at that instant.
Assignment while disconnected applied no calibration. Device replacement lost
the profile's settings, and clearing the assignment could write the previous
device's saved settings into its replacement.

Aurora's `PADSetPortDeadZones` stores four session overrides. `PADRead` chooses
the active port override or the device's own settings when sampling sticks and
triggers. Assignment sets the override; clearing removes it. Device settings
stay intact. The old saved-device calibration fields are removed from the
profile slots. This adds no frame allocations and leaves stick scaling unchanged.
These host input settings remain outside rollback state.

## Verification

`native_profile_pad_test` uses SDL virtual gamepads and the real Aurora PAD
provider. It initializes only the gamepad subsystem, disables HID access and
opens its own virtual device IDs. It creates no window, audio output or renderer.
Its preference directory is inside the build's isolated test fixture.

The assignment-before-connection regression failed against the previous code
and passes with port overrides. The complete check covers:

- Profile assignment while disconnected, followed by controller connection.
- Main-stick and C-stick dead zones and separate left/right trigger thresholds.
- Replacing an assigned profile and clearing it back to device calibration.
- Moving the first device to a different physical port while another takes its
  original port, then disconnecting and replacing that second device.
- Preserving the replacement device's own settings when clearing the profile.
- A custom digital Shield button combined with a mapped analog trigger through
  actual SDL button/axis samples, including release back to zero strength.

All four focused tests pass: `native_profile`, `native_profile_input`,
`native_profile_pad` and `native_rumble_output`. The full native client builds.
The saved-profile UI and all three 2,200-sample Ganondorf gameplay cases also
pass together on that client. Both UI clients stop, and each gameplay run
reaches the end of its recording. Test launches stay hidden and muted.

## Evidence

- `.local/native-profile-pad-before-1.log`: the original failing connection case.
- `.local/native-profile-pad-after-2.log`: all four focused tests pass.
- `.local/build-native-profile-pad-3.log`: full client/provider build.
- `.local/build-native-profile-pad-4.log`: final virtual-device test build.
- `.local/native-profile-controls-3/receipt.json`: complete current UI/gameplay
  run with hashes, saved values and screenshot paths.
- `.local/native-profile-port-receipt.json`: source and binary identities.

Reproduce the device check with CTest's `^native_profile_pad$` filter. No ISO is
needed. `tools/review_native_profile_controls.py` runs the UI/gameplay cohort
with the ISO and a copied profile directory; see the
[controls checkpoint](native_profile_controls_checkpoint.md) for its arguments.

## Remaining acceptance

SDL virtual-device results cover the actual software input provider. Physical
GameCube adapter range/feel, raw binding capture through the UI and real motor
feedback still require hands-on verification. The native adapter scaling and
dead-zone audit remains open; this change preserves its existing conversion.
