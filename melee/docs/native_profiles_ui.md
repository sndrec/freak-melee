# Controller profiles

Open **Settings > Controls > Player profiles & bindings**. Use the Melee hand or
mouse to select controls, and the keyboard to enter a profile name or number.

Choose a saved profile from the left column, or choose **New**. The arrow buttons
show more saved profiles. Choose the controller port at the top of the screen.
**Save & use** saves the profile and assigns it to that port for this launch.
First startup also remembers its chosen profile and port for future launches.

**Use defaults** gives the chosen port its default controls. **Reset** resets the
profile being edited. **Delete** asks for confirmation before removing a saved
profile. Assigned controls stay active until you choose another profile or use
the defaults.

## Gameplay

- **Tap jump** lets you jump by pushing the main stick up.
- **Action buffer** remembers early actions for 0–7 frames. Off uses zero frames.
- **Attack stick** selects smash attacks or tilts for grounded C-stick attacks.
- A dedicated **Short hop** input can be assigned on the Buttons page.

## Buttons and axes

On **Buttons**, choose **Bind** to replace an action's inputs, **+** to add an
input, or **Clear** to remove them. An action can have four inputs. Release the
controls, press the input you want, then release it again to finish. Buttons and
positive or negative axis directions can be bound. Escape, **Cancel binding**, or
holding START cancels capture and preserves the previous binding. A short START
press can itself be bound.

On **Axes**, bind the direction named on each row. For example, choose **Move up**
and move the stick upward. **Default** restores an axis's standard assignment.
Calibrate again after changing an axis.

## Calibration

The Calibration tab contains stick dead zones and trigger click thresholds as
percentages. Click a field, edit it, then press Enter to confirm. Invalid values
restore the previous value. The click setting chooses physical trigger clicks or
clicks generated at the chosen analog threshold.

Choose **Calibrate** for a guided capture:

1. Release both sticks and triggers and leave the controller still.
2. Move both sticks around their edges, visiting all four directions on each.
3. Slowly squeeze the triggers. Drag each **Start** marker past its released
   reading, and put **End** just before the physical click. A controller without
   a separate click can use its full analog travel.
4. Review the readings. **Test** disables hand movement and menu tilt while you
   try the controls. Press START or click **Stop test** to finish testing.

Stick calibration takes effect on the trigger page. The display shows raw
readings alongside calibrated values. Calibration estimates small dead zones
from the resting capture; these remain adjustable on the Calibration tab.

**Use calibration** returns to the profile with the measured values. **Save & use**
saves and applies them. Cancelling calibration restores the controls that were
active before the capture. Disconnecting or replacing the selected controller
also ends capture. **Test controller** opens the review display for the current
profile without taking new measurements.

## Verification

The current review is recorded in
[native_ui_feedback_2026_11_15.md](native_ui_feedback_2026_11_15.md).
`tools/review_native_profile_controls.py` uses hidden, muted clients, an SDL
virtual gamepad and recorded gameplay inputs. Physical controller feel and rumble
still benefit from a hands-on check. The earlier provider and input-policy work
is recorded in [the controls checkpoint](native_profile_controls_checkpoint.md)
and [the port-calibration checkpoint](native_profile_hotplug_checkpoint.md).
