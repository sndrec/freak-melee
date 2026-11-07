# First startup

Updated: 2026-11-07. The guided setup uses the in-game Melee menu scene and hand.
It starts automatically for a fresh installation. Existing players can open
**Settings > First startup**. Existing fighters and account keys stay available.

Startup uses the original animated tournament setup backdrop, selected by the
user from [native scenery captures](native_menu_references.md). Each step has
centered controls, compact progress markers and open space around it. The
artwork is loaded from the ISO and retains its original animation loop.
The [current revision list](native_first_startup_revision.md) tracks the user's
latest feedback and the remaining carousel, portrait and placement work.

## The sequence

1. The opening screen shows Freak Melee, the startup indicator and Continue.
2. Enter a display name and an optional bio.
3. Create/import an online account, or choose **Continue as guest**. Guests can
   play online and use chat. Accounts keep friends, letters and invitations.
4. Choose a controller port, or press A on the controller you want to use.
5. Release the sticks and triggers to measure their resting positions.
6. Move both sticks around their edges, visiting all four directions.
7. Press L and R fully, then release them.
8. Try the measured calibration. **Calibrate again** repeats the capture;
   **Save & continue** saves a controller profile and applies it to that port.
9. Clone a vanilla fighter and give the copy a name. If an installed roster
   already exists, **Use my roster** continues with it.
10. Finish on character select. Choose the fighter, place its token and press
    START to enter practice.

Controller setup can be deferred with **Set up later**. Keyboard text editing
and mouse clicks work throughout. B goes back; setup keeps progress across
restarts. An interrupted calibration restarts at controller selection. A
disconnected or replaced controller also returns to that page.
Back from the first-fighter page returns to controller selection, including
when calibration was skipped. Selecting an unassigned port starts with default
bindings; an assigned port keeps its existing bindings.

## Calibration

The profile stores a resting center and positive/negative travel for each
stick axis, plus released/full trigger travel. It measures rest noise to choose
small stick dead zones. Live meters show the input during capture; the test
page shows the normalized result and applies it to hand movement.
During measurements, stick motion updates only the input displays; hand
movement and menu tilt are suppressed. The displays trace measured stick
outlines and the review page includes scaled and raw axis values. Trigger
maximums still use full travel while the agreed pre-click capture is built.

Measured default and remapped axes share the same conversion. Full stick travel
maps to Melee's 80-unit input and full analog trigger travel to 140. The game
keeps its original radial clamp and input thresholds. Profiles with no measured
capture retain their existing conversion. Changing an axis binding clears the
measured calibration, so the new mapping can be calibrated again.

Setup saves a controller profile under the normal profile store. Its ID and port
are recorded in `content/startup.txt` and reapplied on later launches. Choosing a
different profile through Controls still applies that choice for the current
session. The measured profile format is 191 bytes; the user approved recreating
the previous development test profile. Old profile files are left untouched.

## Current verification

The first native build and five focused contracts pass:
`.local/startup-build-1.log`, `.local/startup-build-2.log`,
`.local/startup-contracts-1.log`. These cover saved measured ranges, neutral,
half/full travel, overtravel, incomplete captures, mapped/default axes and the
actual SDL/Aurora provider, alongside the existing input ownership checks.

Four native UI checks pass in `.local/startup-ui-1/receipt.json`. The hidden,
muted run starts with an empty store, saves profile info without an online
service, resumes controller selection after restart, clones and installs one
independent Mario, finishes on CSS, and reopens setup from Settings after a
completed restart. Seven PNGs were captured; welcome, optional account and
controller selection were visually inspected.

The tournament-background revision builds in
`.local/startup-quiet-build-1.log` and `.local/startup-quiet-build-2.log`.
Its walkthrough passes in `.local/startup-quiet-ui-1/receipt.json`, including
Back after skipping calibration, restart/resume, cloning and the return to CSS.
The second build puts the fixed header/footer above page transitions and
removes an intermediate Workshop transition after cloning during setup.
Five settled screenshots from that build are in
`.local/startup-quiet-final-captures/images.json`: welcome, profile, optional
account, account creation form and controller selection. The welcome, profile
and account creation screens were visually inspected. No account was created
by that capture run.

The UI runs skip hardware calibration. Physical capture and the user's design
and usability pass remain open. The prepared
`.local/startup-playtest/Launch Startup Test.cmd` opens the current native build
with a separate empty user-data folder for that review. It leaves the existing
installation's roster, profiles and account files in place.
