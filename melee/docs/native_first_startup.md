# First startup

Updated: 2026-11-08. Guided setup helps a new player choose a name, calibrate a
controller and make their first fighter. It opens automatically on a fresh
installation. Existing players can open **Settings > First startup**.

The pages use the original animated tournament-setup background, Melee's menu
lettering and the Melee hand. The [revision record](native_first_startup_revision.md)
contains implementation details, evidence and the remaining review items.

## Account and profile

After the welcome page, choose **Sign-Up**, **Login** or **Continue as Guest**.
Hover a choice to read what it does. Login offers the saved key and account-key
file import. Your existing account supplies your display name and bio. Recover
uses a recovery code to replace lost keys and sign out the old devices.

Guests enter a display name and optional bio on the next page. The name starts
empty, with faded Player text as an example. Guests can play online and use
chat. An account keeps friends, letters and invitations. Account management is
available later in Settings. Errors appear in a dismissible notice.

## Controller

Choose a port, or press A on the controller you want to use. **Set up later**
continues to fighter selection.

1. Release the sticks and triggers and hold the controller still.
2. Move both sticks around their edges, visiting up, down, left and right.
3. Slowly squeeze L and R through their digital clicks, then release them.
   Drag each bar's Start and End markers with A and the Melee hand, or the mouse.
   Start sets the released reading to 0%; End sets full analog travel to 100%.
   Put End just before the digital click. The measured positions are suggestions.
4. Try the result. **Test** stops controller-driven hand movement and menu tilt.
   Press START or click **Stop test** to return to editing. The displays show
   measured stick outlines, raw input and calibrated values. The trigger markers
   remain adjustable outside the test.
5. Choose **Save & continue**. **Calibrate again** repeats the measurements.

During stick measurement, sticks move only the calibration displays. The
outlines trace the boundary of actual samples. On the trigger page, the main
stick moves the hand so you can adjust the markers. Moving the hand keeps the
completed stick measurements intact. Controllers without a separate digital
click use full analog travel. Small movements should feel precise, and resting
sticks and released triggers should read zero.
The measured stick calibration already applies while setting the trigger ranges.

Calibration is saved in a controller profile and applied to the selected port
on later launches. Changing an axis binding clears the measured calibration so
it can be captured again. An interrupted measurement or a changed controller
returns to controller selection.

## First fighter

Browse the rotating fighter carousel and confirm a starter. Ice Climbers appear
as a pair; Zelda's fighter includes Sheik. Kirby is excluded while his editable
clone is deferred.

Frame the portrait with the Trophy Gallery controls:

- C-stick: orbit.
- A + main stick over the portrait, or D-pad: pan.
- X/Y: zoom.
- Mouse: middle-drag to orbit, Shift + middle-drag to pan, wheel to zoom.

The fighter stays still while framing. Enter its short tile name on this page;
the name-bar height and framing update together. A second preview shows the
tile at CSS size. Zoom magnifies the orthographic view around the framed point,
and panning has no artificial position limit. **Reset view** restores the
starting camera. **Use portrait** saves the picture and opens placement on the
actual CSS. The hand stays above the tile, and the visible player cards mark
the lower boundary of the roster area. A or a click places the tile. B goes
back. Dropping it finishes setup on character select.

The fighter owns editable moves, scripts, animations and model data. Its short
tile name is saved separately from its full fighter name and travels with a
shared package. Open the Workshop to edit the fighter or rename it.
New stock clones use the original Melee portrait on their player card. In the
Workshop, **Portrait & tile** opens the shot editor. Its CSS tile and Player
card modes edit separate images and retain their camera framing. The detailed
fighter editor also links to this tool from the portrait controls.

Character select begins empty. **Ready / START**, **Workshop** and **Back** sit
in its footer. Choose a fighter, place its token and press START to practice.

## Add the Melee roster

**Workshop > Add Melee roster** opens a confirmation with native empty/full CSS
previews. Confirming creates 24 editable fighters and uses the original tile
arrangement, leaving Kirby's position empty. Existing fighters stay in the
Workshop. Repeating the shortcut reuses its defaults; edited fighters are kept
as separate entries.

## Review

Run `.local/startup-revision-playtest/Launch Startup Review.cmd` for a fresh
setup each time. Every launch creates a separate folder under `runs`, keeping
earlier data available. The tracked source is `melee/tools/launch_startup_review.cmd`.
The wizard plays the supplied mode-select track through the regular music
player and returns to menu music when setup finishes.

The revision record lists the native walkthroughs and focused checks. Physical
controller calibration, camera feel and the user's visual/usability review are
part of the next hands-on pass. The attract-style introduction will be
storyboarded together after the playable flow.
