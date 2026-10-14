# Glim

A little lantern spirit who fights by spending its own light. Glim starts
from Jigglypuff's rig and ordinary movement, with two aerial jumps, a teal and
brass costume, a floating petal halo, and new special moves and animation poses.

| Input | Move | Use |
| --- | --- | --- |
| Neutral B | Lantern Flash | Hold to charge, release for a short-range burst. |
| Side B | Firefly Skip | Aim a quick dash with the stick; one use per airtime. |
| Up B | Updraft | Rise on a burst of light, then enter special fall. |
| Down B | Wick Ward | Briefly reflect projectiles. A reflection adds charge to the next Flash. |

The project is an authoring example. Its damage, speed, timing and recovery
are provisional gameplay choices; it is not intended as a balanced tournament
fighter. Ordinary attacks retain the cloned base except for the documented
custom moves. The original stock fighter and existing user drafts are preserved.

## Build and edit

The checked-in files contain original source, tuning and generated artwork.
The local ISO supplies copied stock rig, clips and common assets. Generated DATs
and packaged copies stay outside Git.

Run `python characters/glim/build.py --iso "path/to/Melee.iso"` from the repo
root. The completed local project appears in the Workshop as **Glim**. The
builder refuses to overwrite an existing installed Workshop draft unless
explicitly told to replace it. Edit that draft freely; rerunning the recipe
does not replace your edits by default.

The recipe requires a built `native_content_tool.exe`, Python, NumPy and Pillow.

The [Workshop notes](WORKSHOP_NOTES.md) record authoring limitations found
during this character's creation, along with any workarounds used.

## Try Glim

Open **Workshop**, select **Glim**, and choose **Test Character**. Its installed
package is also available through the character catalog for ordinary matches.
The four specials are separate editable files in Scripting. Their tuning is in
Fighter state; the Parameters panel can override those defaults.

Custom normal attacks are neutral air (a spinning early/late hit), forward air
(a reaching strike), down air (a meteor followed by a weaker upward hit), and
forward smash (a fixed-power electric burst). Forward smash does not charge.
The other normal attacks use the copied Jigglypuff data.

Flash takes at least six charge frames. Its burst lasts six frames and deals
5 damage plus 0.22 per charged tick, rounded down to a whole damage value.
Skip has twelve landing frames. Updraft ends in special fall and has sixteen
landing frames. Ward reflects during animation frames 3 through 13.
Its shipped radius override is 9 game units; reset the parameter to restore
the source's default of 8.

The original halo mesh and portrait are in `assets/`; they can be reused
directly from the Model workspace. The costume retains the copied facial
details and expressions.

For a finite gameplay exercise, run `playtest.py` with `--iso`, the installed
`--package` ID, and an `--output` directory. It runs hidden and muted, checks
the move transitions and actual charged/skip damage, and saves a receipt.
