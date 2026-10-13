# Player profiles in Options

Options opens a full-window panel that slides in from the left. Back in the
upper-left corner returns to Melee and the gameplay sidebars. The settings
screen is authored in `assets/ui/freak_shell.rml` and `freak_profiles.rcss`;
the dynamic binding rows reflect the current draft. It uses the existing native
profile format and controller application code directly.

The left column selects or creates a saved profile, edits its display name,
and explicitly chooses one of four local controller ports. Save & use writes
the profile atomically under LocalAppData and applies it to that port. Merely
editing or selecting a draft does not change live controls. Use default controls
clears the chosen port's assignment without deleting the saved profile.
Delete saved profile requires a second click on Confirm delete. Deleting its
file removes it from the picker but leaves any already assigned in-memory
controls in effect until the player changes that port's assignment.

Gameplay exposes tap jump and smash/tilt attack-stick behavior. Bindings exposes
all six existing actions, including Short hop, and all six analog axes. Bind
replaces an action's bindings, + adds one (up to four), and Clear unbinds it.
Axis rows can be returned individually to their default mapping. Capture waits
for controls to be released, accepts the next button or axis, and waits for its
release; Cancel, Escape, disconnect, and a 15-second timeout end capture.

Calibration exposes the existing dead-zone and trigger settings as percentages.
Unedited display rounding never changes the underlying raw value. Invalid
fields prevent saving or applying the profile. Trigger activation supports the
full 0–100% range of SDL gamepad travel. The original default Shield binding
preserves analog strength; replacing Shield with a digital action requests full
shield. Calibration now shows six live stick/trigger signals for the selected
port. The meters use draft axis mappings where assigned, update at 10 Hz, and
show a dash when no controller is connected.

The editor opens only in home CSS and closes if the scene changes. Controller
input while it is open does not drive the underlying character selection. Settings remain
fixed during matches. The editor's presentation state is excluded from rollback.
Keyboard/gameplay text capture continues to use the existing shell behavior.

## Verification, 2026-09-22

- Release client builds. All 354 discovered native tests pass serially.
- Profile tests cover an empty store, stable IDs/names, rejection of corrupt
  profiles, explicit catalog-capacity failure, persistence, and existing input
  application behavior.
- Created a temporary profile through the UI, changed tap jump and attack-stick
  mode, and saved it to port four. Independently inspected its persisted bytes:
  tap jump off, tilt stick, and unchanged raw calibration values 8000/8000 and
  31150/31150. Reloaded it through the picker after restarting the client.
- Invalid calibration text displayed an error and left the saved bytes intact.
- Inspected full-window Gameplay, Bindings, Calibration, picker, capture prompt,
  validation error, and return-to-VS screenshots. An injected confirm press while
  the editor was open did not enter character select.
- The profile editor object contains no `.mstate` or `.maudio` section.
- Removed the temporary test profile after verification.
- A live GameCube adapter showed released SDL trigger axes at zero. The
  previous profile conversion interpreted zero as half pressed; custom analog
  trigger bindings now map 0 to 0 and 32767 to 255, matching Aurora's default
  pad conversion. The default click threshold 31150 displays as 95.1% instead
  of 97.5%. `calibration-live-final.png`,
  `calibration-live-no-controller.png`, and `calibration-live-1280.png` were
  inspected in the running client.
  Focused tests check released, half, and full trigger travel plus invalid
  thresholds. No physical trigger press was performed during this check.
- Saved a test profile through the running client with an isolated LocalAppData
  root, cancelled a delete, then confirmed it. The picker returned to New
  profile and the test profile file was gone. The compact two-button
  confirmation remained visible at 1440x810 (`profile-delete-confirm-final.png`;
  `profile-delete-done.png` shows the result). The store test checks that
  deletion rejects invalid IDs and leaves a second saved profile intact.

## Remaining acceptance

Physical-device capture through the new UI still needs hands-on verification;
the native diagnostic pad override does not pretend to be a raw SDL controller.
Capture prompt and cancellation are verified, not a complete physical rebind.
Keyboard rebinding, dedicated menu/pause bindings, and controller-only
navigation/text entry remain open.
Assignment is explicit each session; saved profiles persist, port assignments
do not. The picker currently reports a limit of 64 valid profiles. Narrow-window
and high-DPI layouts need additional inspection; the normal rail launcher is
hidden when the window has no room for sidebars.

Home-flow recheck: START and stick input while Options was open stayed out of
practice; Back returned to CSS (`home-options-return-verified.png`). Input
suppression runs after Melee renews its copied controller status.
