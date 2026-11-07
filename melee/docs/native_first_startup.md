# First startup

Updated: 2026-11-07. The guided setup uses the in-game Melee menu scene and hand.
It starts automatically for a fresh installation. Existing players can open
**Settings > First startup**. Existing fighters and account keys stay available.

The initial layout uses the hub's menu frame. The user requested a simpler,
quieter design with more empty space and a distinct background. The
[original menu scenery](native_menu_references.md) is available in the native
renderer for that discussion; the revised design remains open.

## The sequence

1. Welcome explains the profile, controller and first-fighter steps.
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

## Calibration

The profile stores a resting center and positive/negative travel for each
stick axis, plus released/full trigger travel. It measures rest noise to choose
small stick dead zones. Live meters show the input during capture; the test
page shows the normalized result and applies it to hand movement.

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

That UI run skips hardware calibration. Physical capture and the user's design
and usability pass remain open. Later small changes clarify the optional
account page, prevent an unfinished draft from satisfying the starter step,
and let the first A press claim the controller before activating a button.
These changes receive a final build before the user's test. No service or
artwork changes are needed.
